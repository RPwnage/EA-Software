/******************************************************************************/
/*!
    SessionReportUploader

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     02/23/2010 (mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "EAStdC/EAMemory.h"
#include "EAStdC/EASprintf.h"
#include "EAStdC/EAByteCrackers.h"
#include "BugSentry/sessionreportuploader.h"
#include "BugSentry/reporter.h"
#include "protohttp.h"
#include "netconn.h"

/*** Variables ****************************************************************/
const char* EA::BugSentry::SessionReportUploader::BUG_SENTRY_WEBSERVICE = "session";

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! SessionReportUploader::UploadReport

            \brief      Uploads the session report to the BugSentry webservice.

            \param      reporter - The BugSentry Reporter object.
            \return      - none.
        */
        /******************************************************************************/
        void SessionReportUploader::UploadReport(const Reporter &reporter)
        {
            if (reporter.mBugData && reporter.mConfig)
            {
                // Setup the URL to post to
                EA::StdC::Memset8(mUrl, 0, sizeof(mUrl));
                EA::StdC::Snprintf(mUrl, sizeof(mUrl), "%s/%s/", GetReportingBaseUrl(reporter.mConfig->mMode), BUG_SENTRY_WEBSERVICE);

                // URL Encode the POST data - also sets the value for mPostSize for our http object
                FormatReport(reporter);

                // setup http module
                mHttp = ProtoHttpCreate(mPostSize);

                if (mHttp)
                {
                    // set timeout value
                    ProtoHttpControl(mHttp, UINT32_FROM_UINT8('t','i','m','e'), PROTO_HTTP_TIMEOUT, 0, NULL);

                    // set verbose mode
                    ProtoHttpControl(mHttp, UINT32_FROM_UINT8('s','p','a','m'), PROTO_HTTP_SPAMLEVEL, 0, NULL);

                    // Post the data
                    if (mPostData && (mPostSize > 0))
                    {
                        PostReport();
                    }

                    // destroy http module
                    ProtoHttpDestroy(mHttp);
                    mHttp = NULL;
                }
            }

            mPostData = NULL;
            mPostSize = 0;
        }

        /******************************************************************************/
        /*! SessionReportUploader::PostReport

            \brief      POST the data to the BugSentry webservice.

            \param       - none.
            \return      - none.
        */
        /******************************************************************************/
        void SessionReportUploader::PostReport()
        {
            int posted = 0;
            int sent = 0;

            // Idle
            HttpIdle();

            // Post the data
            posted = ProtoHttpPost(mHttp, mUrl, mPostData, mPostSize, PROTOHTTP_POST);

            // Idle
            HttpIdle();

            // update the module        
            ProtoHttpUpdate(mHttp);

            // wait for completion
            while(posted < mPostSize)
            {
                // Idle
                HttpIdle();

                // post log to server
                sent = ProtoHttpSend(mHttp, mPostData+posted, mPostSize-posted);
                if (sent < 0)
                {
                    break;
                }

                // increment buffer pointer
                posted += sent;

                // Idle
                HttpIdle();

                // update the module        
                ProtoHttpUpdate(mHttp);
            }

            // Idle
            HttpIdle();

            // update the module        
            ProtoHttpUpdate(mHttp);

            // Idle
            HttpIdle();
        }

        /******************************************************************************/
        /*! SessionReportUploader::HttpIdle

            \brief      Idles and waits for the next polling period.

            \param       - none.
            \return      - none.
        */
        /******************************************************************************/
        void SessionReportUploader::HttpIdle()
        {
            const unsigned int DEFAULT_POLL_PERIOD = 100;
            unsigned int startTime = NetConnElapsed();
            unsigned int difference = 0;

            // wait for next poll period
            while (difference < DEFAULT_POLL_PERIOD)
            {
                // give time to DirtySock idle functions
                NetConnIdle();

                // update the module        
                ProtoHttpUpdate(mHttp);

                // give up CPU time for other threads
                NetConnSleep(1);

                // update timer
                difference = NetConnElapsed() - startTime;
            }
        }
    }
}
/******************************************************************************/
/*!
    CrashReportUploader

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

#pragma warning(disable: 4266) // no override available for virtual member function from base 'CException'; function is hidden

/*** Includes *****************************************************************/
#include "EAStdC/EAMemory.h"
#include "EAStdC/EASprintf.h"
#include "EAStdC/EAString.h"
#include "EAStdC/EAByteCrackers.h"
#include "BugSentry/crashreportuploader.h"
#include "BugSentry/crashreportgenerator.h"
#include "BugSentry/reporter.h"
#include "BugSentry/crashdata.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/dirtysock/netconn.h"

//  11469  [FH] [Telemetry] Context Data submitted with BugSentry crashes contain null characters 
//	Need telemetry to tell us why 7% of our BugSentry reports are truncated
#include "TelemetryAPIDLL.h"

/*** Variables ****************************************************************/
const char* EA::BugSentry::CrashReportUploader::BUG_SENTRY_WEBSERVICE = "submit";

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! CrashReportUploader::UploadReport

            \brief      Uploads the crash report to the BugSentry webservice.

            \param      reporter - The BugSentry Reporter object.
            \return      - none.
        */
        /******************************************************************************/
        void CrashReportUploader::UploadReport(const Reporter &reporter)
        {
            if (reporter.mBugData && reporter.mConfig)
            {
                // Setup the URL to post to
                EA::StdC::Memset8(mUrl, 0, sizeof(mUrl));
                EA::StdC::Snprintf(mUrl, sizeof(mUrl), "%s/%s/", GetReportingBaseUrl(reporter.mConfig->mMode), BUG_SENTRY_WEBSERVICE);

                // URL Encode the POST data - also sets the value for mPostSize for our http object
                FormatReport(reporter);

                // Save off context data information from report that is needed during upload process
                const CrashReportGenerator* crashReportGenerator = static_cast<const CrashReportGenerator*>(reporter.mReportGenerator);
                const CrashData* crashData = static_cast<const CrashData*>(reporter.mBugData);
                mContextDataPosition = crashReportGenerator->GetContextDataPosition();
                mContextDataGameBuffer = crashData->mContextData;
                mContextDataPostSize = 0;

                // If ContextData information was supplied by the game...
                if (mContextDataPosition && mContextDataGameBuffer)
                {
                    // For disconnect reports, we use a second buffer for the context data (allocated already by the game). Include that in the post size.
                    mContextDataPostSize = static_cast<int>(EA::StdC::Strlen(crashData->mContextData)*sizeof(char));
                }

                // setup http module
                mHttp = ProtoHttpCreate(mPostSize+mContextDataPostSize);

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
        /*! CrashReportUploader::PostReport

            \brief      POST the data to the BugSentry webservice.

            \param       - none.
            \return      - none.
        */
        /******************************************************************************/
        void CrashReportUploader::PostReport()
        {
            int posted = 0;
            int contextDataPosted = 0;

            // Idle
            HttpIdle();

            // NOTE: We can't control the amount of data initially posted, ProtoHttpPost will attempt to post as much as possible.
            //       Since we swap buffers mid post, this can't happen, but we must provide the full size we plan to post.
            //       So we supply a NULL pointer to the buffer, but the full size we plan to post to set things up properly.
            //       This keeps the connection open with the server and allows us to follow the flow of sending chunks
            //       which allows us to switch buffers while posting.
            posted = ProtoHttpPost(mHttp, mUrl, NULL, mPostSize+mContextDataPostSize, PROTOHTTP_POST);
            posted = 0;

            // Idle
            HttpIdle();

            // update the module        
            ProtoHttpUpdate(mHttp);

            // send chunks of data, switching between the bugsentry and game buffers as needed
            while((posted+contextDataPosted) < (mPostSize+mContextDataPostSize))
            {
                bool usingGameBuffer = false;
                int sent = 0;
                const char* data = mPostData + posted;
                int sizeToSend = mPostSize - posted;

                // Check if we will need to worry about context data information
                if (mContextDataPosition)
                {
                    // If we are coming up to our switching point, limit the size to that point
                    if (data < mContextDataPosition)
                    {
                        // Trim down the size to go up to our switching point
                        sizeToSend = static_cast<int>(mContextDataPosition - data);
                    }
                    else
                    {
                        // Switch to the game-supplied buffer (until that has finished)
                        if ((data == mContextDataPosition) && (contextDataPosted < mContextDataPostSize))
                        {
                            data = mContextDataGameBuffer + contextDataPosted;
                            sizeToSend = mContextDataPostSize - contextDataPosted;
                            usingGameBuffer = true;
                        }

                        // else, continue on. This will switch back to the remaining portion of our buffer
                    }
                }

                // Idle
                HttpIdle();

                // Send the next chunk
                sent = ProtoHttpSend(mHttp, data, sizeToSend);

                // Handle result
                if (sent < 0)
                {
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("BugSentryError", "");
                    break;
                }
                else
                {
                    if (usingGameBuffer)
                    {
                        contextDataPosted += sent;
                    }
                    else
                    {
                        posted += sent;
                    }
                }

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
        /*! CrashReportUploader::HttpIdle

            \brief      Idles and waits for the next polling period.

            \param       - none.
            \return      - none.
        */
        /******************************************************************************/
        void CrashReportUploader::HttpIdle()
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

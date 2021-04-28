/******************************************************************************/
/*! 
    ServerReportUploader
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/05/2010 (mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "EAStdC/EAMemory.h"
#include "EAStdC/EASprintf.h"
#include "EAStdC/EAString.h"
#include "EAStdC/EAByteCrackers.h"
#include "BugSentry/serverreportuploader.h"
#include "BugSentry/serverreportgenerator.h"
#include "BugSentry/reporter.h"
#include "BugSentry/serverdata.h"
#include "DirtySDK/proto/protohttp.h"

/*** Variables ****************************************************************/
const char* EA::BugSentry::ServerReportUploader::BUG_SENTRY_WEBSERVICE = "submit";

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! ServerReportUploader :: ~ServerReportUploader

            \brief  Destructor.

            \param          - none.
            \return         - void.
        */
        /******************************************************************************/
        ServerReportUploader::~ServerReportUploader()
        {
            if (mHttp)
            {
                ProtoHttpDestroy(mHttp);
            }
        }

        /******************************************************************************/
        /*! ServerReportUploader :: UploadReport

            \brief  Starts uploading report to BugSentry.

            \param      reporter - The BugSentry Reporter object.
            \return         - void.
        */
        /******************************************************************************/
        void ServerReportUploader::UploadReport(const Reporter &reporter)
        {
            if (reporter.mBugData && reporter.mConfig)
            {
                // Setup the URL to post to
                EA::StdC::Memset8(mUrl, 0, sizeof(mUrl));
                EA::StdC::Snprintf(mUrl, sizeof(mUrl), "%s/%s/", GetReportingBaseUrl(reporter.mConfig->mMode), BUG_SENTRY_WEBSERVICE);

                // URL Encode the POST data - also sets the value for mPostSize for our http object
                FormatReport(reporter);

                // Save off context data information from report that is needed during upload process
                const ServerReportGenerator* serverReportGenerator = static_cast<const ServerReportGenerator*>(reporter.mReportGenerator);
                const ServerData* serverData = static_cast<const ServerData*>(reporter.mBugData);
                mContextDataPosition = serverReportGenerator->GetContextDataPosition();
                mContextDataGameBuffer = serverData->mContextData;
                mContextDataPostSize = 0;
                mContextDataPosted = 0;

                // If ContextData information was supplied by the game...
                if (mContextDataPosition && mContextDataGameBuffer)
                {
                    // For disconnect reports, we use a second buffer for the context data (allocated already by the game). Include that in the post size.
                    mContextDataPostSize = static_cast<int>(EA::StdC::Strlen(serverData->mContextData)*sizeof(char));
                }

                // setup http module
                mHttp = ProtoHttpCreate(mPostSize+mContextDataPostSize);

                if (mHttp)
                {
                    // set timeout value
                    ProtoHttpControl(mHttp, UINT32_FROM_UINT8('t','i','m','e'), PROTO_HTTP_TIMEOUT, 0, NULL);

                    // set verbose mode
                    ProtoHttpControl(mHttp, UINT32_FROM_UINT8('s','p','a','m'), PROTO_HTTP_SPAMLEVEL, 0, NULL);

                    // Start sending data if possible
                    if (mPostData && (mPostSize+mContextDataPostSize > 0))
                    {
                        PostReport();
                    }
                    else
                    {
                        // If there is no data to send, destroy HTTP module
                        ProtoHttpDestroy(mHttp);
                        mHttp = NULL;
                    }
                }
            }
        }

        /******************************************************************************/
        /*! ServerReportUploader :: DonePosting

            \brief  Returns whether or not the post has done sending.

            \param          - none.
            \return         - True if the post has finished uploading, false otherwise.
        */
        /******************************************************************************/
        bool ServerReportUploader::DonePosting() const
        {
            bool isDone = true;

            // If the http operation is still pending (value of zero), we are not done posting 
            if(ProtoHttpStatus(mHttp, UINT32_FROM_UINT8('d','o','n','e'), NULL, 0) == 0)
            {
                ProtoHttpUpdate(mHttp);
                isDone = false;
            }

            return isDone;
        }

        /******************************************************************************/
        /*! ServerReportUploader :: SendNextChunk

            \brief  Sends next chunk of data to server

            \param          - none.
            \return         - True if post was successful. False if post failed and should be stopped.
        */
        /******************************************************************************/
        bool ServerReportUploader::SendNextChunk()
        {
            bool success = true;

            // If we still have data to send, send the next chunk (otherwise, we default to success=true).
            // DonePosting() will trigger BugSentryMgrBase to end the polling.
            if ((mPosted < mPostSize) || (mContextDataPosted < mContextDataPostSize))
            {
                bool usingGameBuffer = false;
                int sent = 0;
                const char* data = mPostData + mPosted;
                int sizeToSend = mPostSize - mPosted;

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
                        if ((data == mContextDataPosition) && (mContextDataPosted < mContextDataPostSize))
                        {
                            data = mContextDataGameBuffer + mContextDataPosted;
                            sizeToSend = mContextDataPostSize - mContextDataPosted;
                            usingGameBuffer = true;
                        }

                        // else, continue on. This will switch back to the remaining portion of our buffer
                    }
                }
                
                // Send the next chunk
                sent = ProtoHttpSend(mHttp, data, sizeToSend);

                // Handle result
                if (sent < 0)
                {
                    success = false;
                }
                else
                {
                    if (usingGameBuffer)
                    {
                        mContextDataPosted += sent;
                    }
                    else
                    {
                        mPosted += sent;
                    }
                    ProtoHttpUpdate(mHttp);
                    success = true;
                }
            }

            return success;
        }

        /******************************************************************************/
        /*! ServerReportUploader :: PostReport

            \brief  Starts posting report to server.

            \param          - none.
            \return         - void.
        */
        /******************************************************************************/
        void ServerReportUploader::PostReport()
        {
            // NOTE: We can't control the amount of data initially posted, ProtoHttpPost will attempt to post as much as possible.
            //       Since we swap buffers mid post, this can't happen, but we must provide the full size we plan to post.
            //       So we supply a NULL pointer to the buffer, but the full size we plan to post to set things up properly.
            //       This keeps the connection open with the server and allows us to follow the flow of sending chunks
            //       which allows us to switch buffers while posting.
            mPosted = ProtoHttpPost(mHttp, mUrl, NULL, mPostSize+mContextDataPostSize, PROTOHTTP_POST);
            mPosted = 0;

            // update the module
            ProtoHttpUpdate(mHttp);
        }
    }
}

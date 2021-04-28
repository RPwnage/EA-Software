/******************************************************************************/
/*!
    BugSentryMgrBase

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/bugsentrymgrbase.h"
#include "BugSentry/bugdata.h"
#include "BugSentry/sessiondata.h"
#include "BugSentry/serverdata.h"
#include "BugSentry/desyncreportuploader.h"
#include "coreallocator/icoreallocator_interface.h"
#include "BugSentry/sessionreportgenerator.h"
#include "BugSentry/sessionreportuploader.h"
#include "BugSentry/serverreportgenerator.h"
#include "BugSentry/serverreportuploader.h"
#include "EAStdC/EAString.h"

#include <new>

/*** Variables ****************************************************************/
char EA::BugSentry::BugSentryMgrBase::sSessionId[BugData::SESSION_ID_LENGTH] = {0};

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! BugSentryMgrBase :: BugSentryMgrBase

            \brief  Constructor.

            \param         config - The BugSentry config structure.
            \return         - void.

            Initializes member variables and stores the config.
        */
        /******************************************************************************/
        BugSentryMgrBase::BugSentryMgrBase(BugSentryConfig* config) : mState(STATE_IDLE)
        {
            // Copy the config struct
            mConfig = *config;
        }

        /******************************************************************************/
        /*! BugSentryMgrBase :: ~BugSentryMgrBase

            \brief  Destructor.

            \param          - none.
            \return         - void.

            Destructs and deallocates the report uploader if it has not already been destructed and deallocated.
        */
        /******************************************************************************/
        BugSentryMgrBase::~BugSentryMgrBase()
        {
            if (mReporter.mReportUploader)
            {
                // This will only happen if BugSentry is not polled enough before being destroyed. So cleanup.
                CleanupReportUploader();
            }
        }

        /******************************************************************************/
        /*! BugSentryMgrBase :: ProcessUpload

            \brief  Sends the next chunk of report data if it needs to be sent. This function must be called repeatedly until it returns false.

            \param          - none.
            \return         - Returns true if there is more data to send, false if the report has finished sending, or something went wrong while sending the report.

            This is not applicable for crash reports; crash reports are completely uploaded before ReportCrash() returns.
            This function should be called for processing all other error report uploads.
        */
        /******************************************************************************/
        bool BugSentryMgrBase::ProcessUpload()
        {
            bool continuePolling = false;

            if (mReporter.mReportUploader)
            {
                if (mReporter.mReportUploader->DonePosting() || !mReporter.mReportUploader->SendNextChunk())
                {
                    // Either the posting is complete, or something went wrong when posting.
                    CleanupReportUploader();
                    SetState(STATE_IDLE);

                    // we're done, stop polling.
                    continuePolling = false;
                }
                else
                {
                    // Still waiting for the previous post, or there is another chunk to post.
                    continuePolling = true;
                }
            }

            return continuePolling;
        }

        /******************************************************************************/
        /*! BugSentryMgrBase :: Report

            \brief  Reports to file or server.

            \param          - none.
            \return         - void.

            Assumes the object has been correctly initialized.
        */
        /******************************************************************************/
        void BugSentryMgrBase::Report()
        {
            bool reportGenerated = false;
            ImageInfo* imageInfo = &(mReporter.mBugData->mImageInfo);
            
            // Add the session ID to the BugData for all reports
            EA::StdC::Strncpy(mReporter.mBugData->mSessionId, sSessionId, sizeof(mReporter.mBugData->mSessionId));

            // Give the reporter access to the config
            mReporter.mConfig = &mConfig;

            // Try to get the best screenshot possible while keeping the report size under the maximum size the server accepts.
            for (int downSample = mConfig.mImageDownSample; downSample < IMAGE_DOWNSAMPLE_COUNT; ++downSample)
            {
                imageInfo->mImageDownSample = static_cast<ImageDownSample>(downSample);

                if (mReporter.CalculateReportByteSize() < ReportUploader::MAX_REPORT_SIZE)
                {
                    reportGenerated = true;
                    break;
                }
            }

            // If even the most compressed screen shot was too large to send, send only the call stack.
            if (!reportGenerated)
            {
                // Disable screenshot.
                mConfig.mEnableScreenshot = false;
            }

            if (mConfig.mWriteToFile)
            {
                // Write report to file
                mReporter.CreateFileReport(mConfig.mFilePath);
            }

            // Report error to BugSentry
            mReporter.UploadReport();
        }

        /******************************************************************************/
        /*! BugSentryMgrBase :: CleanupReportUploader

            \brief  Cleans up the report uploader - this is needed when we're done uploading the report, or if BugSentry gets shutdown before finishing the upload.

            \param          - none.
            \return         - none.

            This is not applicable for crash reports.
            This function should be called for all other error reports.
        */
        /******************************************************************************/
        void BugSentryMgrBase::CleanupReportUploader()
        {
            BugSentryState currentState = GetState();

            if (mReporter.mReportUploader &&
                (currentState != STATE_INVALID) &&
                (currentState != STATE_IDLE) &&
                (currentState != STATE_REPORTSESSION) &&
                (currentState != STATE_REPORTCRASH))
            {
                mReporter.mReportUploader->~ReportUploader();
                mConfig.mAllocator->Free(mReporter.mReportUploader);
                mReporter.mReportUploader = NULL;
                mReporter.mBugData = NULL;
                mReporter.mReportGenerator = NULL;
            }
        }
        
        /******************************************************************************/
        /*! BugSentryMgrBase :: ReportSession

            \brief  Reports the session back to EA, using GOS' BugSentry. This should be called once when the game boots.

            \param       sessionType - The type of session to report (boot, game, server, etc.).
            \param       sessionConfig - The session configuration used for reporting the session. Must be supplied, though some session types do not contain config data.
            \return      - void.
        */
        /******************************************************************************/
        void BugSentryMgrBase::ReportSession(BugSentrySession sessionType, BugSentrySessionConfig *sessionConfig)
        {
            if (mState == STATE_IDLE)
            {
                SetState(STATE_REPORTSESSION);

                // Setup BugData object for report
                SessionData sessionData;
                sessionData.mBuildSignature = mConfig.mBuildSignature;
                sessionData.mSku = mConfig.mSku;
                sessionData.mSessionType = sessionType;
                sessionData.mSessionConfig = *sessionConfig;
                mReporter.mBugData = &sessionData;

                // Setup the Session Reporter
                SessionReportGenerator sessionReportGenerator;
                mReporter.mReportGenerator = &sessionReportGenerator;

                // Setup the Session Report Uploader
                SessionReportUploader* sessionReportUploader = new(static_cast<SessionReportUploader*>(mConfig.mAllocator->Alloc(sizeof(SessionReportUploader), "EA::BugSentry::SessionReportUploader", EA::Allocator::MEM_TEMP))) SessionReportUploader();
                mReporter.mReportUploader = sessionReportUploader;

                // Report the session to BugSentry
                Report();

                // Cleanup
                mReporter.mReportGenerator = NULL;
                sessionReportUploader->~SessionReportUploader();
                mConfig.mAllocator->Free(mReporter.mReportUploader);
                mReporter.mReportUploader = NULL;
                SetState(STATE_IDLE);
            }
        }

        /******************************************************************************/
        /*! BugSentryMgrBase :: ReportServerError

            \brief  Reports a server error back to EA, using GOS' BugSentry

            \param  categoryId          - A string to categorize this disconnect (i.e. gameplay state name, etc.), null terminated.
            \param  serverName          - The server name the server error is being reported about. base64 encoded, null terminated. 49 characters max.
            \param  serverType          - The type of server the server error is being reported about (i.e. "blaze", "redirector", etc.). base64 encoded, null terminated. 49 characters max.
            \param  serverError         - One of the BugSentryServerErrorType enumerations for the server error being reported.
            \param  contextData         - Any context data, with each line base64 encoded and null terminated.
            \return                     - void.
        */
        /******************************************************************************/
        void BugSentryMgrBase::ReportServerError( const char* categoryId, const char* serverName, const char* serverType, BugSentryServerErrorType serverError, const char* contextData )
        {
            if (mState == STATE_IDLE)
            {
                SetState(STATE_REPORTSERVERERROR);

                ServerData serverData;
                serverData.mBuildSignature = mConfig.mBuildSignature;
                serverData.mSku = mConfig.mSku;
                serverData.mCategoryId = categoryId;
                serverData.mServerName = serverName;
                serverData.mServerType = serverType;
                serverData.mServerErrorType = serverError;
                serverData.mContextData = contextData;
                mReporter.mBugData = &serverData;

                ServerReportGenerator serverReportGenerator;
                mReporter.mReportGenerator = &serverReportGenerator;

                ServerReportUploader* serverReportUploader = new(static_cast<ServerReportUploader*>(mConfig.mAllocator->Alloc(sizeof(ServerReportUploader), "EA::BugSentry::ServerReportUploader", EA::Allocator::MEM_TEMP))) ServerReportUploader();
                mReporter.mReportUploader = serverReportUploader;
                // serverReportUploader is destructed and freed in BugSentryMgrBase::ProcessUpload or BugSentryMgrBase::~BugSentryMgrBase.
                // It needs to be kept alive for polling.

                // Report the disconnect
                Report();

                // Cleanup (NOTE: the serverReportUploader will be cleaned up when the upload is complete.
                // Do not SetState(STATE_IDLE) - handled by bugsentrymgrbase::ProcessUpload when the report is done posting
            }
        }
    }
}

/******************************************************************************/
/*!
    BugSentryMgr

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/09/2010 (mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/bugsentrymgr.h"
#include "BugSentry/bugdata.h"
#include "BugSentry/crashdata.h"
#include "BugSentry/desyncdata.h"
#include "BugSentry/utils.h"
#include "BugSentry/crashreportgenerator.h"
#include "BugSentry/desyncreportgenerator.h"
#include "BugSentry/crashreportuploader.h"
#include "BugSentry/desyncreportuploader.h"
#include "coreallocator/icoreallocator_interface.h"
#include "EAStdC/EAHashCRC.h"
#include "EAStdC/EAString.h"
#include "EAStdC/EAMemory.h"
#include "EAStdC/EASprintf.h"
#include "netconn.h"

#include <new>

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! BugSentryMgr::BugSentryMgr

            \brief      Constructor
            
            \param      config: The BugSentry config structure specifying game preferences.

        */
        /******************************************************************************/
        BugSentryMgr::BugSentryMgr(BugSentryConfig* config) : BugSentryMgrBase(config), mImageInfo(NULL)
        {
            // Generate a session id
            if (sSessionId[0] == '\0')
            {
                CreateSessionId(sSessionId);
            }
        }

        /******************************************************************************/
        /*! BugSentryMgr::ReportCrash

            \brief      Reports the crash back to EA, using GOS' BugSentry

            \param       exceptionPointers - The platform-specific exception information.
            \param       exceptionCode - The platform-specific exception code.
            \param       contextData - Any additional data specified by the game, base64 encoded and null terminated.
            \param       imageInfo - The ImageInfo structure to generate a screenshot from.
            \param       categoryId - (optional) A null terminated string that the game uses to categorize crashes. If a null pointer is passed, the top of the callstack will be used instead.
            \return      none.
        */
        /******************************************************************************/
        void BugSentryMgr::ReportCrash(void* exceptionPointers, unsigned int exceptionCode, const char* contextData, const ImageInfo* imageInfo, const char* categoryId)
        {
            if (mState == STATE_IDLE)
            {
                mImageInfo = imageInfo;
                ReportCrash(exceptionPointers, exceptionCode, contextData, categoryId);
            }
        }

        /******************************************************************************/
        /*! BugSentryMgr::ReportCrash

            \brief      Reports the crash back to EA, using GOS' BugSentry

            \param       exceptionPointers - The platform-specific exception information.
            \param       exceptionCode - The platform-specific exception code.
            \param       contextData - Any additional data specified by the game, base64 encoded and null terminated.
            \param       categoryId - (optional) A null terminated string that the game uses to categorize crashes. If a null pointer is passed, the top of the callstack will be used instead.
            \return      none.
        */
        /******************************************************************************/
        void BugSentryMgr::ReportCrash(void* exceptionPointers, unsigned int exceptionCode, const char* contextData, const char* categoryId)
        {
            if (mState == STATE_IDLE)
            {
                SetState(STATE_REPORTCRASH);
                
                CrashData crashData;
                crashData.mBuildSignature = mConfig.mBuildSignature;
                crashData.mSku = mConfig.mSku;
                crashData.mExceptionInformation = exceptionPointers;
                crashData.mExceptionCode = exceptionCode;
                crashData.mContextData = contextData;
                crashData.mCategoryId = categoryId;
                if (mImageInfo)
                {
                    crashData.mImageInfo = *mImageInfo;
                }
                mReporter.mBugData = &crashData;

                CrashReportGenerator crashReportGenerator;
                mReporter.mReportGenerator = &crashReportGenerator;

                CrashReportUploader* crashReportUploader = new(static_cast<CrashReportUploader*>(mConfig.mAllocator->Alloc(sizeof(CrashReportUploader), "EA::BugSentry::CrashReportUploader", EA::Allocator::MEM_TEMP))) CrashReportUploader();
                mReporter.mReportUploader = crashReportUploader;

                // Generate a callstack
                Utils::GetCallstack(exceptionPointers, &crashData.mStackAddresses[0], CrashData::MAX_STACK_ADDRESSES, &crashData.mStackAddressesCount);

                // Report the crash
                Report();

                // Cleanup
                mReporter.mReportGenerator = NULL;
                mReporter.mBugData = NULL;
                crashReportUploader->~CrashReportUploader();
                mConfig.mAllocator->Free(crashReportUploader);
                mReporter.mReportUploader = NULL;

                SetState(STATE_IDLE);
            }
        }

        /******************************************************************************/
        /*! BugSentryMgr :: ReportDesync

            \brief  Reports the desync back to EA, using GOS' BugSentry

            \param      categoryId  - A string to categorize this desync (i.e. gameplay state name, etc.), terminated with a '\0' character.
            \param      desyncId    - An id for this desync that all peers report (must be the same on all clients! Reports will be grouped by desync id), terminated with a '\0' character.
            \param      desyncInfo  - Deync logs, with each line base64 encoded and null terminated.
            \param      imageInfo - The ImageInfo structure to generate a screenshot from.
            \return     void.
        */
        /******************************************************************************/
        void BugSentryMgr::ReportDesync(const char* categoryId, const char* desyncId, const char* desyncInfo, const ImageInfo* imageInfo)
        {
            if (mState == STATE_IDLE)
            {
                mImageInfo = imageInfo;
                ReportDesync(categoryId, desyncId, desyncInfo);
            }
        }

        /******************************************************************************/
        /*! BugSentryMgr :: ReportDesync

            \brief  Reports the desync back to EA, using GOS' BugSentry

            \param      categoryId  - A string to categorize this desync (i.e. gameplay state name, etc.), terminated with a '\0' character.
            \param      desyncId    - An id for this desync that all peers report (must be the same on all clients! Reports will be grouped by desync id), terminated with a '\0' character.
            \param      desyncInfo  - Deync logs, with each line base64 encoded and null terminated.
            \return     void.
        */
        /******************************************************************************/
        void BugSentryMgr::ReportDesync(const char* categoryId, const char* desyncId, const char* desyncInfo)
        {
            if (mState == STATE_IDLE)
            {
                SetState(STATE_REPORTDESYNC);
                
                DesyncData desyncData;
                desyncData.mBuildSignature = mConfig.mBuildSignature;
                desyncData.mSku = mConfig.mSku;
                desyncData.mCategoryId = categoryId;
                desyncData.mDesyncId = desyncId;
                desyncData.mDesyncInfo = desyncInfo;
                if (mImageInfo)
                {
                    desyncData.mImageInfo = *mImageInfo;
                }
                mReporter.mBugData = &desyncData;

                DesyncReportGenerator desyncReportGenerator;
                mReporter.mReportGenerator = &desyncReportGenerator;

                DesyncReportUploader* desyncReportUploader = new(static_cast<DesyncReportUploader*>(mConfig.mAllocator->Alloc(sizeof(DesyncReportUploader), "EA::BugSentry::DesyncReportUploader", EA::Allocator::MEM_TEMP))) DesyncReportUploader();
                mReporter.mReportUploader = desyncReportUploader;
                // desyncReportUploader is destructed and freed in BugSentryMgrBase::PollBugSentry or BugSentryMgrBase::~BugSentryMgrBase.
                // It needs to be kept alive for polling.

                // Report the desnyc
                Report();

                // Cleanup (NOTE: the desyncReportUploader will be cleaned up when the upload is complete.
                // Do not SetState(STATE_IDLE) - handled by bugsentrymgrbase::ProcessDesyncUpload when the report is done posting
            }
        }

        /******************************************************************************/
        /*! BugSentryMgr :: CreateSessionId

            \brief  Creates a unique session id for this user's instance of booting the game (for use to link the session report to error reports)

            \param      sessionId - The session specific identifier for this boot
            \return     void.
        */
        /******************************************************************************/
        void BugSentryMgr::CreateSessionId(char* sessionId)
        {
            // Get the MAC address and CRC it
            const char* consoleMacAddressString = NetConnMAC();
            unsigned int consoleMacAddress = EA::StdC::StrtoU32(&consoleMacAddressString[1], NULL, 16);
            unsigned int consoleMacAddressCrc = EA::StdC::CRC32(&consoleMacAddress, sizeof(consoleMacAddress));

            // Get timestamp
            time_t timeStamp = 0;
            time(&timeStamp);

            // Convert MAC Address and timestamp to strings
            char macAddressString[9] = {0};
            char timeStampString[9] = {0};
            EA::StdC::U32toa(consoleMacAddressCrc, &macAddressString[0], 16);
            EA::StdC::I32toa(static_cast<int>(timeStamp), &timeStampString[0], 16);

            // Create session id
            EA::StdC::Sprintf(sessionId, "%s%s", macAddressString, timeStampString);
        }
    }
}

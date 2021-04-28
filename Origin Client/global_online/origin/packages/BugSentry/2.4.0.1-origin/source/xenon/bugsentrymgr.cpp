/******************************************************************************/
/*!
    BugSentryMgr

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
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
        BugSentryMgr::BugSentryMgr(BugSentryConfig* config) : BugSentryMgrBase(config), mDevice(NULL), mFrontBuffer(NULL)
        {
            // Generate a session id
            if (sSessionId[0] == '\0')
            {
                CreateSessionId(sSessionId);
            }
        }

        /******************************************************************************/
        /*! BugSentryMgr::!BugSentryMgr

            \brief      Destructor
        */
        /******************************************************************************/
        BugSentryMgr::~BugSentryMgr()
        {
            mDevice = NULL;
            mFrontBuffer = NULL;
        }

        /******************************************************************************/
        /*! BugSentryMgr::ReportCrash

            \brief      Reports the crash back to EA, using GOS' BugSentry

            \param       exceptionPointers - The platform-specific exception information.
            \param       exceptionCode - The platform-specific exception code.
            \param       contextData - Any additional data specified by the game, base64 encoded and null terminated.
            \param       device - The D3D Device for BugSentry to get the Texture buffer for a screenshot.
            \param       categoryId - (optional) A null terminated string that the game uses to categorize crashes. If a null pointer is passed, the top of the callstack will be used instead.
            \return      none.
        */
        /******************************************************************************/
        void BugSentryMgr::ReportCrash(void* exceptionPointers, unsigned int exceptionCode, const char* contextData, LPDIRECT3DDEVICE9 device, const char* categoryId)
        {
            // Save off D3D device
            mDevice = device;

            // Pass-through
            ReportCrash(exceptionPointers, exceptionCode, contextData, categoryId);
        }

        /******************************************************************************/
        /*! BugSentryMgr::ReportCrash

            \brief      Reports the crash back to EA, using GOS' BugSentry

            \param       exceptionPointers - The platform-specific exception information.
            \param       exceptionCode - The platform-specific exception code.
            \param       contextData - Any additional data specified by the game, base64 encoded and null terminated.
            \param       frontBuffer - The D3D Texture buffer for a screenshot.
            \param       categoryId - (optional) A null terminated string that the game uses to categorize crashes. If a null pointer is passed, the top of the callstack will be used instead.
            \return      none.
        */
        /******************************************************************************/
        void BugSentryMgr::ReportCrash(void* exceptionPointers, unsigned int exceptionCode, const char* contextData, LPDIRECT3DTEXTURE9 frontBuffer, const char* categoryId)
        {
            // Save off front buffer address
            mFrontBuffer = frontBuffer;

            // Pass-through
            ReportCrash(exceptionPointers, exceptionCode, contextData, categoryId);
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
                
                bool frameBufferCaptureAvailable = false;

                CrashData crashData;
                crashData.mBuildSignature = mConfig.mBuildSignature;
                crashData.mSku = mConfig.mSku;
                crashData.mExceptionInformation = exceptionPointers;
                crashData.mExceptionCode = exceptionCode;
                crashData.mContextData = contextData;
                crashData.mCategoryId = categoryId;
                mReporter.mBugData = &crashData;

                CrashReportGenerator crashReportGenerator;
                mReporter.mReportGenerator = &crashReportGenerator;

                CrashReportUploader* crashReportUploader = new(static_cast<CrashReportUploader*>(mConfig.mAllocator->Alloc(sizeof(CrashReportUploader), "EA::BugSentry::CrashReportUploader", EA::Allocator::MEM_TEMP))) CrashReportUploader();
                mReporter.mReportUploader = crashReportUploader;

                ImageInfo* imageInfo = &(mReporter.mBugData->mImageInfo);

                // Start frame buffer access
                frameBufferCaptureAvailable = Utils::StartFrameBufferAccess(imageInfo, mDevice, mFrontBuffer, mConfig.mImageDownSample);

                // Generate a callstack
                Utils::GetCallstack(exceptionPointers, &crashData.mStackAddresses[0], CrashData::MAX_STACK_ADDRESSES, &crashData.mStackAddressesCount);

                // Report the crash
                Report();
                
                // End frame buffer access
                if (frameBufferCaptureAvailable)
                {
                    Utils::EndFrameBufferAccess(imageInfo, mDevice, mFrontBuffer);
                }

                // Cleanup
                mReporter.mBugData = NULL;
                mReporter.mReportGenerator = NULL;
                crashReportUploader->~CrashReportUploader();
                mConfig.mAllocator->Free(mReporter.mReportUploader);
                mReporter.mReportUploader = NULL;

                SetState(STATE_IDLE);
            }
        }


        /******************************************************************************/
        /*! BugSentryMgr::ReportDesync

            \brief      Reports the desync back to EA, using GOS' BugSentry

            \param      categoryId  - A string to categorize this desync (i.e. gameplay state name, etc.), terminated with a '\0' character.
            \param      desyncId    - An id for this desync that all peers report (must be the same on all clients! Reports will be grouped by desync id), terminated with a '\0' character.
            \param      desyncInfo  - Deync logs, with each line base64 encoded and null terminated.
            \param      device      - The D3D Device for BugSentry to get the Texture buffer for a screenshot.
            \return     void.
        */
        /******************************************************************************/
        void BugSentryMgr::ReportDesync(const char* categoryId, const char* desyncId, const char* desyncInfo, LPDIRECT3DDEVICE9 device)
        {
            // Save off D3D device
            mDevice = device;

            // Pass-through
            ReportDesync(categoryId, desyncId, desyncInfo);
        }

        /******************************************************************************/
        /*! BugSentryMgr::ReportDesync

            \brief      Reports the desync back to EA, using GOS' BugSentry

            \param      categoryId  - A string to categorize this desync (i.e. gameplay state name, etc.), terminated with a '\0' character.
            \param      desyncId    - An id for this desync that all peers report (must be the same on all clients! Reports will be grouped by desync id), terminated with a '\0' character.
            \param      desyncInfo  - Deync logs, with each line base64 encoded and null terminated.
            \param      frontBuffer - The D3D Texture buffer for a screenshot.
            \return     void.
        */
        /******************************************************************************/
        void BugSentryMgr::ReportDesync(const char* categoryId, const char* desyncId, const char* desyncInfo, LPDIRECT3DTEXTURE9 frontBuffer)
        {
            // Save off front buffer address
            mFrontBuffer = frontBuffer;

            // Pass-through
            ReportDesync(categoryId, desyncId, desyncInfo);
        }

        /******************************************************************************/
        /*! BugSentryMgr :: ReportDesync

            \brief  Reports the desync back to EA, using GOS' BugSentry

            \param      categoryId  - A string to categorize this desync (i.e. gameplay state name, etc.), null terminated.
            \param      desyncId    - An id for this desync that all peers report (must be the same on all clients! Reports will be grouped by desync id), null terminated.
            \param      desyncInfo  - Deync logs, with each line base64 encoded and null terminated.
            \return     void.
        */
        /******************************************************************************/
        void BugSentryMgr::ReportDesync(const char* categoryId, const char* desyncId, const char* desyncInfo)
        {
            if (mState == STATE_IDLE)
            {
                SetState(STATE_REPORTDESYNC);

                bool frameBufferCaptureAvailable = false;

                DesyncData desyncData;
                desyncData.mBuildSignature = mConfig.mBuildSignature;
                desyncData.mSku = mConfig.mSku;
                desyncData.mCategoryId = categoryId;
                desyncData.mDesyncId = desyncId;
                desyncData.mDesyncInfo = desyncInfo;
                mReporter.mBugData = &desyncData;

                DesyncReportGenerator desyncReportGenerator;
                mReporter.mReportGenerator = &desyncReportGenerator;

                DesyncReportUploader* desyncReportUploader = new(static_cast<DesyncReportUploader*>(mConfig.mAllocator->Alloc(sizeof(DesyncReportUploader), "EA::BugSentry::DesyncReportUploader", EA::Allocator::MEM_TEMP))) DesyncReportUploader();
                mReporter.mReportUploader = desyncReportUploader;
                // desyncReportUploader is destructed and freed in BugSentryMgrBase::ProcessDesyncUpload or BugSentryMgrBase::~BugSentryMgrBase.
                // It needs to be kept alive for polling.

                ImageInfo* imageInfo = &(mReporter.mBugData->mImageInfo);

                // Start frame buffer access
                frameBufferCaptureAvailable = Utils::StartFrameBufferAccess(imageInfo, mDevice, mFrontBuffer, mConfig.mImageDownSample);

                // Report the desnyc
                Report();

                // End frame buffer access
                if (frameBufferCaptureAvailable)
                {
                    Utils::EndFrameBufferAccess(imageInfo, mDevice, mFrontBuffer);
                }

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

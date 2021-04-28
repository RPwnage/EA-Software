/******************************************************************************/
/*!
    BugSentryMgr

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRYMGR_H
#define BUGSENTRYMGR_H

/*** Includes *****************************************************************/
#include <xtl.h>
#include "BugSentry/bugsentrymgrbase.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class BugSentryMgr : public BugSentryMgrBase
        {
        public:
            BugSentryMgr(BugSentryConfig* config);
            virtual ~BugSentryMgr();

            // Crash Error Reporting
            virtual void ReportCrash(void* exceptionPointers, unsigned int exceptionCode, const char* contextData, LPDIRECT3DDEVICE9 device, const char* categoryId = NULL);
            virtual void ReportCrash(void* exceptionPointers, unsigned int exceptionCode, const char* contextData, LPDIRECT3DTEXTURE9 frontBuffer, const char* categoryId = NULL);

            // Desync Error Reporting
            virtual void ReportDesync(const char* categoryId, const char* desyncId, const char* desyncInfo, LPDIRECT3DDEVICE9 device);
            virtual void ReportDesync(const char* categoryId, const char* desyncId, const char* desyncInfo, LPDIRECT3DTEXTURE9 frontBuffer);

        private:
            virtual void ReportCrash(void* exceptionPointers, unsigned int exceptionCode, const char* contextData, const char* categoryId = NULL);
            virtual void ReportDesync(const char* categoryId, const char* desyncId, const char* desyncInfo);
            virtual void CreateSessionId(char* sessionId);

            LPDIRECT3DDEVICE9 mDevice;
            LPDIRECT3DTEXTURE9 mFrontBuffer;
        };
    }
}

#endif // BUGSENTRYMGR_H

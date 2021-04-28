/******************************************************************************/
/*!
    BugSentryMgr

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     02/24/2009 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRYMGR_H
#define BUGSENTRYMGR_H

/*** Includes *****************************************************************/
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
            virtual ~BugSentryMgr() {};

            // Crash Error Reporting
            virtual void ReportCrash(void* exceptionPointers, unsigned int exceptionCode, const char* contextData, const char* categoryId = NULL);

            // Desync Error Reporting
            virtual void ReportDesync(const char* categoryId, const char* desyncId, const char* desyncInfo);

        private:
            virtual void CreateSessionId(char* sessionId);
        };
    }
}

#endif // BUGSENTRYMGR_H

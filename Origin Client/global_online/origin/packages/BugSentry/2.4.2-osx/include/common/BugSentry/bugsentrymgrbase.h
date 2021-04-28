/******************************************************************************/
/*!
    BugSentryMgrBase

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRYMGR_BASE_H
#define BUGSENTRYMGR_BASE_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif
#include "BugSentry/reporter.h"
#include "BugSentry/imageinfo.h"
#include "BugSentry/bugdata.h"
#include "BugSentry/bugsentryconfig.h"
#include "BugSentry/bugsentrysessionconfig.h"

/*** Forward Declarations *****************************************************/
namespace EA
{
    namespace Allocator
    {
        class ICoreAllocator;
    }
    
    namespace BugSentry
    {
        class ReportGenerator;
        class DesyncReportUploader;
        class ReportUploader;
    }
}

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        enum BugSentryState
        {
            STATE_INVALID = -1,
            STATE_FIRST = 0,
            STATE_IDLE = STATE_FIRST,
            STATE_REPORTSESSION,
            STATE_REPORTCRASH,
            STATE_REPORTDESYNC,
            STATE_REPORTSERVERERROR,
            STATE_MAX
        };

        enum BugSentrySession
        {
            SESSION_INVALID = -1,
            SESSION_FIRST = 0,
            SESSION_BOOT = SESSION_FIRST,
            SESSION_GAME,
            SESSION_SERVER,
            SESSION_MAX
        };

        enum BugSentryServerErrorType
        {
            SERVER_ERROR_TYPE_INVALID = -1,
            SERVER_ERROR_TYPE_FIRST = 0,
            SERVER_ERROR_TYPE_CONNECTIONFAILURE = SERVER_ERROR_TYPE_FIRST,
            SERVER_ERROR_TYPE_DISCONNECT,
            SERVER_ERROR_TYPE_MAX
        };

        class BugSentryMgrBase
        {
        public:
            BugSentryMgrBase(BugSentryConfig* config);
            virtual ~BugSentryMgrBase();

            // Interface to game
            virtual void ReportSession(BugSentrySession sessionType, BugSentrySessionConfig *sessionConfig);
            virtual void ReportCrash(void* exceptionPointers, unsigned int exceptionCode, const char* contextData, const char* categoryId = NULL) = 0;
            virtual void ReportDesync(const char* categoryId, const char* desyncId, const char* desyncInfo) = 0;
            virtual void ReportServerError(const char* categoryId, const char* serverName, const char* serverType, BugSentryServerErrorType serverError, const char* contextData);
            bool ProcessUpload();
            BugSentryState GetState() const { return mState; }
            const char* GetSessionId() const { return &sSessionId[0]; }

        protected:
            void Report();
            virtual void CreateSessionId(char* sessionId) = 0;
            void SetState(BugSentryState state) { mState = state; }

            static char sSessionId[BugData::SESSION_ID_LENGTH];

            Reporter mReporter;
            BugSentryConfig mConfig;
            BugSentryState mState;

        private:
            void CleanupReportUploader();
        };
    }
}
/******************************************************************************/

#endif // BUGSENTRYMGR_BASE_H

/******************************************************************************/
/*!
    SessionData
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/05/2010 (mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_SESSIONDATA_H
#define BUGSENTRY_SESSIONDATA_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif
#include "BugSentry/bugdata.h"
#include "BugSentry/bugsentrysessionconfig.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class SessionData  : public BugData
        {
        public:
            SessionData();
            SessionData(const char* buildSignature, const char* sku, const char* sessionId);

            int mSessionType;
            BugSentrySessionConfig mSessionConfig;

        private:
            void Init();
        };
    }
}
/******************************************************************************/

#endif // BUGSENTRY_SESSIONDATA_H

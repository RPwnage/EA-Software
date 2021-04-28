/******************************************************************************/
/*!
    BugSentrySessionConfig
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     05/19/2010 (mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_SESSIONCONFIG_H
#define BUGSENTRY_SESSIONCONFIG_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class BugSentrySessionConfig
        {
        public:
            BugSentrySessionConfig()
            {
                mServerSession.mServerType = NULL;
                mServerSession.mServerName = NULL;
            }
            virtual ~BugSentrySessionConfig() {};

            struct ServerSessionConfig
            {
                const char* mServerType;
                const char* mServerName;
            };

            struct BootSessionConfig {};
            struct GameSessionConfig {};

            union
            {
                BootSessionConfig mBootSession;
                GameSessionConfig mGameSession;
                ServerSessionConfig mServerSession;
            };
        };
    }
}
/******************************************************************************/

#endif // BUGSENTRY_SESSIONCONFIG_H

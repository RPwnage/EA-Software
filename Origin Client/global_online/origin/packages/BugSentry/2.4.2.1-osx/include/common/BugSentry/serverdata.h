/******************************************************************************/
/*! 
    ServerData
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/05/2010 (mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_SERVERDATA_H
#define BUGSENTRY_SERVERDATA_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "EABase/eabase.h"
#endif

#include "BugSentry/bugdata.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class ServerData : public BugData
        {
        public:
            ServerData();
            ServerData(const char* buildSignature, const char* sku, const char* sessionId);

            const char* mCategoryId;
            const char* mServerName;
            const char* mServerType;
            int mServerErrorType;
            const char* mContextData;

        private:
            void Init();
        };
    }
}


/******************************************************************************/

#endif  //  BUGSENTRY_SERVERDATA_H


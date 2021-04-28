/******************************************************************************/
/*! 
    BugData
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     07/01/2009 (gsharma)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_BUGDATA_H
#define BUGSENTRY_BUGDATA_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "EABase/eabase.h"
#endif

#include "BugSentry/imageinfo.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class BugData
        {
        public:
            BugData();
            BugData(const char* buildSignature, const char* sku, const char* sessionId);
            virtual ~BugData() {}

            static const int SESSION_ID_LENGTH = 17;

            const char* mBuildSignature;
            const char* mSku;
            char mSessionId[SESSION_ID_LENGTH];
            ImageInfo mImageInfo;
        };
    }
}
/******************************************************************************/

#endif  //  BUGSENTRY_BUGDATA_H


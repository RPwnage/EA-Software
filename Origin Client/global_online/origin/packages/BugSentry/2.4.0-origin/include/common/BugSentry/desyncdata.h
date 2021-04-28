/******************************************************************************/
/*! 
    DesyncData
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     07/01/2009 (gsharma)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_DESYNCDATA_H
#define BUGSENTRY_DESYNCDATA_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif
#include "BugSentry/bugdata.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class DesyncData : public BugData
        {
        public:
            DesyncData();
            DesyncData(const char* buildSignature, const char* sku, const char* sessionId);

            const char* mDesyncInfo;
            const char* mCategoryId;
            const char* mDesyncId;
        };
    }
}


/******************************************************************************/

#endif  //  BUGSENTRY_DESYNCDATA_H


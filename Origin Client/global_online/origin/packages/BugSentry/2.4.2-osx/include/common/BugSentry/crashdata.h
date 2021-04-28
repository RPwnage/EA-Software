/******************************************************************************/
/*!
    CrashData
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     07/17/2009 (gsharma)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_CRASHDATA_H
#define BUGSENTRY_CRASHDATA_H

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
        class CrashData  : public BugData
        {
        public:
            CrashData();
            CrashData(const char* buildSignature, const char* sku, const char* sessionId);
           
            static const int MAX_STACK_ADDRESSES = 50;
            
            void* mExceptionInformation;
            unsigned int mExceptionCode;
#if defined(EA_PLATFORM_REVOLUTION)
            unsigned int mStackAddresses[MAX_STACK_ADDRESSES];
#else
            uintptr_t mStackAddresses[MAX_STACK_ADDRESSES];
#endif
            int mStackAddressesCount;
            const char* mContextData;
            const char* mCategoryId;

        private:
            void Init();
        };
    }
}
/******************************************************************************/

#endif // BUGSENTRY_CRASHDATA_H

/******************************************************************************/
/*!
    Utils

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     02/24/2009 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_UTILS_H
#define BUGSENTRY_UTILS_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
#include "eabase/eabase.h"
#endif

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class Utils
        {
        public:
            Utils() {}
            ~Utils() {}

            static bool GetCallstack(void* exceptionPointers, unsigned int* addresses, int addressesMax, int* addressesFilledCount);

        private:
            static size_t ExtractPs3Callstack(void* exceptionPointers, void* returnAddressArray[], size_t returnAddressArrayCapacity);
        };
    }
}

#endif // BUGSENTRY_UTILS_H

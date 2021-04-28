/******************************************************************************/
/*!
    Utils

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/09/2010 (mbaylis)
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
            Utils();
            ~Utils() {}

            static bool GetCallstack(void* exceptionPointers, uintptr_t* addresses, int addressesMax, int* addressesFilledCount);
            static bool GetCallstackHeuristic(void* exceptionPointers, uintptr_t* addresses, int addressesMax, int* addressesFilledCount);
            static bool GetStackDump(void* exceptionPointers, unsigned char* dump, int maxDumpSize, int* dumpSize);
            static bool GetModuleInfo(void* exceptionPointers, char* buffer, int bufferSize);
        };
    }
}

#endif // BUGSENTRY_UTILS_H

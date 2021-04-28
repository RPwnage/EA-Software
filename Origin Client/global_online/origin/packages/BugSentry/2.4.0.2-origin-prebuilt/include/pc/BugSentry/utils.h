/******************************************************************************/
/*!
    Utils

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/09/2010 (mbaylis)
*/
/******************************************************************************/

#pragma warning(disable: 4263) // warning C4263: 'HRESULT IDWriteTextLayout::GetFontFamilyNameLength(UINT32,UINT32 *,DWRITE_TEXT_RANGE *)' : member function does not override any base class virtual member function
#pragma warning(disable: 4264) // warning C4264: 'UINT32 IDWriteTextFormat::GetFontFamilyNameLength(void)' : no override available for virtual member function from base 'IDWriteTextFormat'; function is hidden

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

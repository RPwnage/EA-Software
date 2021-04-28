/*! ****************************************************************************
    \file Utility.cpp


    \attention
        Copyright (c) Electronic Arts 2011.  ALL RIGHTS RESERVED.

*******************************************************************************/

#include "BlazeSDK/blazesdk.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h> 

#include <EABase/eabase.h>
#include <EAAssert/eaassert.h>
#include <CoreAllocator/icoreallocator.h>

#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtyvers.h"

#ifdef EA_PLATFORM_WINDOWS
#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)
#include <crtdbg.h>
#endif // EA_PLATFORM_WINDOWS

//------------------------------------------------------------------------------
// Format the dirtysock version palatably
const char * GetDirtySockVersionString(void)
{
    static char buffer[512] = "";

    _snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d.%d",
        DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON, DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH);

    return buffer;
}

//------------------------------------------------------------------------------
void Log(const char *format, ...)
{
    va_list args;
    char strText[4096] = "";

    va_start(args, format);
    vsnprintf(strText, sizeof(strText), format, args);
    va_end(args);

    strText[sizeof(strText) - 1] = 0;

    printf("%s", strText); // This syntax handles odd characters like %n

#ifdef EA_PLATFORM_WINDOWS
    OutputDebugString(strText);
#endif // EA_PLATFORM_WINDOWS
}

//-------------------------------------------------------------------------------------------------
void CheckForMemoryLeaks(void)
{
#if defined EA_PLATFORM_WINDOWS && !defined NDEBUG
    if (::_CrtDumpMemoryLeaks() != 0)
    {
        Log("\n*** WARNING: MEMORY LEAKED ***\n\n");
        ::MessageBeep(MB_OK);
        ::Sleep(3000);
    }
#endif // !NDEBUG
}

#ifndef BLAZESDK_DLL  // If this is not built with Dirtysock as a DLL... provide our own default DirtyMemAlloc/Free
//------------------------------------------------------------------------------
// DirtyMemAlloc and DirtyMemFree are required for DirtySock
void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    return malloc(static_cast<size_t>(iSize));
}

//------------------------------------------------------------------------------
void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    free(pMem);
}
#endif

//------------------------------------------------------------------------------
//EASTL New operator
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return malloc(size);
}

//------------------------------------------------------------------------------
//EASTL New operator
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return malloc(size);
}

//------------------------------------------------------------------------------
// A default allocator
class TestAllocator : public EA::Allocator::ICoreAllocator
{
    virtual void *Alloc(size_t size, const char * name, unsigned int flags) { return malloc(size); }
    virtual void *Alloc(size_t size, const char * name, unsigned int flags, unsigned int align, unsigned int alignoffset)  { return malloc(size); }
    virtual void *AllocDebug(size_t size, const DebugParams debugParams, unsigned int flags)  { return malloc(size); }
    virtual void *AllocDebug(size_t size, const DebugParams debugParams, unsigned int flags, unsigned int align, unsigned int alignOffset)  { return malloc(size); }
    virtual void Free(void *ptr, size_t) { free(ptr); }
};

//------------------------------------------------------------------------------
ICOREALLOCATOR_INTERFACE_API EA::Allocator::ICoreAllocator *EA::Allocator::ICoreAllocator::GetDefaultAllocator()
{
    // NOTE: we can't just use another global allocator object, since we'd have an initialization race condition between the two globals.
    //   to avoid that, we use a static function var, which is init'd the first time the function is called.
    static TestAllocator sTestAllocator; // static function members are only constructed on first function call
    return &sTestAllocator;
}

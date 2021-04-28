/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef DYNLOAD_H
#define DYNLOAD_H

#if defined( EA_PLATFORM_LINUX )
#include <dlfcn.h>
#endif

#include "EASTL/hash_map.h"

namespace Blaze
{

// Trying to represent both Windows and Linux flags in one wrapper here.  There aren't
// that many flags to begin with, so we'll do it this way.  The Upper 16-bits are for
// Windows flags.  The lower 16 bits are for Linux flags.  They will be munged back into
// the right format in the private conversion method.
const uint32_t LOAD_FLAGS_NONE = 0;
#if defined( EA_PLATFORM_WINDOWS )
const uint32_t LOAD_FLAGS_LAZY_BINDING = 0;
const uint32_t LOAD_FLAGS_NOW_BINDING = 0;
#elif defined( EA_PLATFORM_LINUX )
const uint32_t LOAD_FLAGS_LAZY_BINDING = RTLD_LAZY;
const uint32_t LOAD_FLAGS_NOW_BINDING = RTLD_NOW;
#endif

/*************************************************************************************************/
/*!
    \class DynLoad
    DynLoad is a wraper class around the Windows LoadLibrary()/GetProcAddress() interface for
    loading resources dynamically and the Linux dlopen()/dlsym() interface to do the same
*/
/*************************************************************************************************/

class DynLoad
{
public:
    DynLoad() : mModule(nullptr) {}
    ~DynLoad() 
    { 
        if (mModule!= nullptr)
        {
            freeLibrary();
            mModule = nullptr;
        }
    }

    bool loadLibrary( const char8_t* fileName, const uint32_t flags = LOAD_FLAGS_NOW_BINDING );
    bool freeLibrary();

    const char8_t* getLastError() const;

    void* getProcAddress( const char8_t* procName );

    bool isLoaded() { return mModule != nullptr; }

private:

#if defined( EA_PLATFORM_WINDOWS )
    HMODULE mModule;

    uint32_t flags2WinFormat( const uint32_t flags ) const;
#elif defined( EA_PLATFORM_LINUX )
    void* mModule;

    int32_t flags2LinuxFormat( const uint32_t flags ) const;
#endif
};

} // Blaze

#endif // DYNLOAD_H

/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

#if defined( EA_PLATFORM_WINDOWS )

#include <comdef.h>     // for _com_error

#elif defined( EA_PLATFORM_LINUX )

#include <dlfcn.h>
#include <unistd.h>
#include <limits.h>

#include <EAIO/PathString.h>

#endif

#include "dynload.h"

namespace Blaze
{

#if defined( EA_PLATFORM_WINDOWS )

bool DynLoad::loadLibrary( const char8_t* fileName, const uint32_t flags /* = LOAD_FLAGS_NOW_BINDING */ )
{
    if (mModule != nullptr || fileName == nullptr || *fileName == '\0')
    {
        return false;
    }

    uint32_t remappedFlags = flags2WinFormat( flags );

    mModule = LoadLibraryEx( fileName, nullptr, remappedFlags );

    return mModule != nullptr;
}

bool DynLoad::freeLibrary()
{
    BOOL rc = TRUE;
    if (isLoaded())
    {
        rc = FreeLibrary(mModule);
        if (rc == TRUE)
        {
            mModule = nullptr;
        }

    }
    
    return rc == TRUE;
}

const char8_t* DynLoad::getLastError() const
{
    // Yes, this returns a string to match up with how dlerror() works on the Linux side.
    // Just going to use the _com_error() function to convert the error code into a 
    // basic text string.

    // _com_error does not actual require any COM interface code to use
    uint32_t lastError = ::GetLastError();
    return (lastError != ERROR_SUCCESS) ? _com_error(lastError).ErrorMessage() : nullptr;
}

void* DynLoad::getProcAddress( const char8_t* procName )
{
    if (mModule == nullptr || procName == nullptr || *procName == '\0')
    {
        return nullptr;
    }

    return GetProcAddress(mModule, procName);
}

uint32_t DynLoad::flags2WinFormat( const uint32_t flags ) const
{
    // Windows gets the upper 16-bits of the wrapped flags uint32.  Just move
    // them to the lower 16-bits and we're done
    return (flags >> 16);
}

#elif defined( EA_PLATFORM_LINUX )

bool DynLoad::loadLibrary( const char8_t* fileName, const uint32_t flags /* = LOAD_FLAGS_NOW_BINDING */ )
{
    if (mModule != nullptr || fileName == nullptr || *fileName == '\0')
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[DynLoad].loadLibrary: " << (mModule != nullptr ? "already loaded" : "no filename"));
        return false;
    }

    int32_t remappedFlags = flags2LinuxFormat(flags);

    mModule = dlopen( fileName, remappedFlags );

    if (mModule == nullptr)
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[DynLoad].loadLibrary: first location failed: " << getLastError());
        //The direct filename didn't work.  Try loading it from the current working directory
        EA::IO::Path::PathString8 libFilename(fileName);
        EA::IO::Path::PathString8 libPathname(".");
        libPathname = EA::IO::Path::Append(libPathname, libFilename); //append instead of join, as we don't want to normalize
        mModule = dlopen(libPathname.c_str(), remappedFlags );

        if (mModule == nullptr)
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[DynLoad].loadLibrary: second location failed: " << getLastError());
            //Still nothing.  Try loading from the location of the executable.  We can get this by
            //doing "readlink" on our exe in the linux proc area
            char8_t buf[PATH_MAX];
            int32_t numRead = 0;
            if ((numRead = readlink("/proc/self/exe", buf, sizeof(buf) - 1)) > 0)
            {
                //we got something, so buf should point to the executable file.  Strip the exe file
                //and then tack on the library
                buf[numRead] = '\0';
                libPathname = buf;
                libPathname = EA::IO::Path::GetDirectoryString(libPathname.begin());
                libPathname = EA::IO::Path::Join(libPathname, libFilename);

                mModule = dlopen( libPathname.c_str(), remappedFlags );

                if (mModule == nullptr)
                {
                    BLAZE_WARN_LOG(Log::SYSTEM, "[DynLoad].loadLibrary: third location failed: " << getLastError());
                }
            }
        }
    }

    if (mModule != nullptr)
    {
        BLAZE_INFO_LOG(Log::SYSTEM, "[DynLoad].loadLibrary: loaded: " << fileName);
    }
    return mModule != nullptr;
}

bool DynLoad::freeLibrary()
{
    bool rc = true;
    if (isLoaded())
    {
        rc = (dlclose(mModule) == 0) ? true : false;
        if (rc == true)
        {
            mModule = nullptr;
        }
    }
    
    return rc;
}

const char8_t* DynLoad::getLastError() const
{
    // Returns a nullptr-terminated string representation of the last error, or nullptr if
    // no last error (or uninitialized).
    return dlerror();
}

void* DynLoad::getProcAddress( const char8_t* procName )
{
    if (mModule == nullptr || procName == nullptr || *procName == '\0')
    {
        return nullptr;
    }

    dlerror();      // Clear any exising error
    return dlsym(mModule, procName);
}


int32_t DynLoad::flags2LinuxFormat( const uint32_t flags ) const
{
    // Linux gets the lower 16-bits of the wrapped flags uint32_t.  Just mask
    // off the upper 16-bits and we're good to go.
    int32_t remappedFlags = flags & 0x0000ffff;

    EA_ASSERT( remappedFlags != 0 );       // On Linux, at least RTLD_LAZY or RTLD_NOW must be specified
    return remappedFlags;
}
#endif

} // Blaze

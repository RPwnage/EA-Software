///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Client.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Client.h>
#include <EAPatchClient/Debug.h>


#if EAPATCHCLIENT_DLL // When building as a DLL, we need to provide our own version of this as opposed a main app providing it.
    ///////////////////////////////////////////////////////////////////////////////
    // Vsnprintf
    // This is required by EASTL. 
    ///////////////////////////////////////////////////////////////////////////////

    #include <EAStdC/EASprintf.h>

    int Vsnprintf8(char8_t* pDestination, size_t n, const char8_t*  pFormat, va_list arguments)
    {
        return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
    }

    int Vsnprintf16(wchar_t* pDestination, size_t n, const wchar_t* pFormat, va_list arguments)
    {
        return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
    }

    #if (EASTDC_VERSION_N >= 10600)
        int Vsnprintf32(char32_t* pDestination, size_t n, const char32_t* pFormat, va_list arguments)
        {
            return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
        }
    #endif

    #if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
        int VsnprintfW(wchar_t* pDestination, size_t n, const wchar_t* pFormat, va_list arguments)
        {
            return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
        }
    #endif



    ///////////////////////////////////////////////////////////////////////////////
    // DirtySDK memory allocation.
    ///////////////////////////////////////////////////////////////////////////////

    extern "C" void* DirtyMemAlloc(int32_t iSize, int32_t /*iMemModule*/, int32_t /*iMemGroup*/)
    {
        EA::Allocator::ICoreAllocator* pAllocator = EA::Patch::GetAllocator();
        EAPATCH_ASSERT(pAllocator);

        return pAllocator->Alloc(static_cast<size_t>(iSize), "DirtySDK", 0);
    }

    extern "C" void DirtyMemFree(void* p, int32_t /*iMemModule*/, int32_t /*iMemGroup*/)
    {
        EA::Allocator::ICoreAllocator* pAllocator = EA::Patch::GetAllocator();
        EAPATCH_ASSERT(pAllocator);

        return pAllocator->Free(p); 
    }

#endif



namespace EA
{
namespace Patch
{

///////////////////////////////////////////////////////////////////////////////
// Client
///////////////////////////////////////////////////////////////////////////////

Client::Client()
  : mServer()
  , mPatchDirectoryRetriever()
  , mClientLimits()
{
    EAPATCH_ASSERT(GetAllocator()); // To do: Move this to the init function instead of this constructor.
}

Client::~Client()
{
}


} // namespace Patch
} // namespace EA








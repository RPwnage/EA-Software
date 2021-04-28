// EAPackageDefines.h
// Copyright 2014 Electronic Arts Inc. All rights reserved.

#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>

///////////////////////////////////////////////////////////////////////////////
// Begin EA core tech support code.

// Required by EASTL.
int Vsnprintf8(char8_t* pDestination, size_t n, const char8_t*  pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}

int Vsnprintf16(char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}

// EASTL requires the following new operators to be defined.
void* operator new(size_t size, const char*, int, unsigned, const char*, int)
{
    return new char[size];
}

void* operator new[](size_t size, const char*, int, unsigned, const char*, int)
{
    return new char[size];
}

void* operator new[](size_t size, size_t, size_t, const char*, int, unsigned, const char*, int)
{
    return new char[size];
}

void* operator new(size_t size, size_t /*alignment*/, size_t /*alignmentOffset*/, const char* /*name*/, 
                    int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    // We will have a problem if alignment is non-default.
    return new char[size];
}

// Required by EAIO
#include <coreallocator/icoreallocator_interface.h>

#if defined(_MSC_VER)
    #define EA_INIT_PRIORITY(x)
    #pragma warning(disable: 4075)  // warning C4075: initializers put in unrecognized initialization area
    #pragma init_seg(".CRT$XCB")    // Cause this module to init early and shutdown late. Due to CoreAllocatorGeneral. 
#elif defined(__GNUC__)
    #define EA_INIT_PRIORITY(x) __attribute__ ((init_priority (x)))
#endif

namespace EA
{
	namespace Allocator
    {
        class CoreAllocatorGeneral_Tel : public ICoreAllocator
        {
        public:
            virtual void* Alloc(size_t size, const char* /*name*/, unsigned int /*flags*/)
            {
                return new char[size];
            }

            virtual void* Alloc(size_t size, const char* /*name*/, unsigned int /*flags*/, unsigned int /*align*/, unsigned int /*alignOffset*/ = 0)
            {
                return new char[size];
            }

            virtual void  Free(void* block, size_t /*size*/ = 0)
            {
                delete[] (char*)block;
            }
        };

        CoreAllocatorGeneral_Tel gCoreAllocatorGeneral_Tel EA_INIT_PRIORITY(500);

        ICoreAllocator* ICoreAllocator::GetDefaultAllocator()
        {
            return &gCoreAllocatorGeneral_Tel;
        }
    }
}


extern "C"
{
    void* DirtyMemAlloc(int32_t iSize, int32_t /*iMemModule*/, int32_t /*iMemGroup*/, void* /*pMemGroupUserData*/)
    {
		if(iSize>0)
			return malloc(iSize);

		return NULL;
    }

    void DirtyMemFree(void* pMem, int32_t /*iMemModule*/, int32_t /*iMemGroup*/, void* /*pMemGroupUserData*/)
    {
		if(pMem)
	        free(pMem);
    }
}


// End EA core tech support code.
///////////////////////////////////////////////////////////////////////////////
//#endif
///////////////////////////////////////////////////////////////////////////////
// EATraceZoneObject.cpp
// Copyright (c) 2007, Electronic Arts. All rights reserved.
//
// This file was copied from MemoryMan/ZoneObject.cpp in order to avoid a 
// dependency on the MemoryMan package.
///////////////////////////////////////////////////////////////////////////////


#include <EATrace/internal/Config.h>
#include <EATrace/EATraceZoneObject.h>
#include <EATrace/Allocator.h>


namespace EA
{
    namespace Trace
    {

        void* ZoneObject::operator new(size_t n)
        {
            EA::Allocator::ICoreAllocator* const pAllocator = EA::Trace::GetAllocator();
            return DoInternalAllocate(n, pAllocator, EATRACE_ALLOC_PREFIX "ZoneObject", 0);
        }


        void* ZoneObject::operator new(size_t n, EA::Allocator::ICoreAllocator* pAllocator)
        {
            if(!pAllocator)
                pAllocator = EA::Trace::GetAllocator();
            return DoInternalAllocate(n, pAllocator, EATRACE_ALLOC_PREFIX "ZoneObject", 0);
        }


        void* ZoneObject::operator new(size_t n, EA::Allocator::ICoreAllocator* pAllocator, const char* pName)
        {
            return DoInternalAllocate(n, pAllocator ? pAllocator : EA::Trace::GetAllocator(), pName, 0);
        }


        void* ZoneObject::operator new(size_t n, const char* pName, const int flags, int /*debugFlags*/, const char* /*pFile*/, int /*pLine*/)
        {
            EA::Allocator::ICoreAllocator* const pCoreAllocator = EA::Trace::GetAllocator();
            return DoInternalAllocate(n, pCoreAllocator, pName, (unsigned int)flags);
        }


        /// \brief Helper function that allocates memory for operator new()
        void* ZoneObject::DoInternalAllocate(size_t n, EA::Allocator::ICoreAllocator* pAllocator, const char* pName, unsigned int flags)
        {
            // We stash a pointer to the allocator before the actual object:
            //
            // +------------------------------------------+
            // |               memory block               |
            // |..........................................|
            // |alloc    . aligned object                 |
            // +------------------------------------------+
            //           ^
            //    aligned as requested
            //
            // This assumes that the object being allocated has no offset
            // requirement, only an alignment requirement. Currently this
            // is true of C/C++ aligned objects and RenderWare objects.

            static const size_t kAlignOfT = EA_ALIGN_OF(ZoneObject);

            void* const p = pAllocator->Alloc(n + kOffset, pName, flags, kAlignOfT, kOffset);

            if(p)
            {
                EA::Allocator::ICoreAllocator** const pp = (EA::Allocator::ICoreAllocator**)p;
                *pp = pAllocator;

                // Return the start of the memory segment which is dedicated to the object itself.
                return (void*)((uintptr_t)pp + kOffset);
            }

            return NULL;
        }





        void ZoneObject::operator delete(void* p)
        {
            DoInternalDeallocate(p);
        }


        void ZoneObject::operator delete(void* p, EA::Allocator::ICoreAllocator* /*pAllocator*/)
        {
            DoInternalDeallocate(p);
        }


        void ZoneObject::operator delete(void* p, EA::Allocator::ICoreAllocator* /*pAllocator*/, const char* /*pName*/)
        {
            DoInternalDeallocate(p);
        }


        void ZoneObject::operator delete(void* p, const char* /*pName*/, const int /*flags*/, int /*debugFlags*/, 
            const char* /*pFile*/, int /*pLine*/)
        {
            DoInternalDeallocate(p);
        }


        /// \brief deletes an object using the allocator pointer stored 'kOffset' bytes before the object
        void ZoneObject::DoInternalDeallocate(void* p)
        {
            if(p) // As per the C++ standard, deletion of NULL results in a no-op.
            {
                // DoInternalAllocaote() stashed a pointer to the allocator before the actual object:
                //
                // +------------------------------------------+
                // |               memory block               |
                // |..........................................|
                // |alloc    . aligned object                 |
                // +------------------------------------------+
                //           ^
                //           p
                //
                // Given 'p', compute true start of memory block
                EA::Allocator::ICoreAllocator** const pAllocatedBlockStart = (EA::Allocator::ICoreAllocator**)((char*)p - kOffset);

                // Start of block contains ICoreAllocator* responsible for that block
                EA::Allocator::ICoreAllocator*  const pStashedAllocator    = (EA::Allocator::ICoreAllocator*)(*pAllocatedBlockStart);

                // TODO: check that the pointer is not garbage
                // Something like this? (http://www.codeproject.com/macro/openvc.asp)
                //#if defined(EA_DEBUG)
                //    // validate address:
                //    if (FALSE == AfxIsValidAddress(pObj, sizeof(CObject), FALSE))
                //        return false;
                //
                //    // check to make sure the VTable pointer is valid
                //    void** vfptr = (void**)*(void**)pObj;
                //    if (!AfxIsValidAddress(vfptr, sizeof(void*), FALSE))
                //        return false;
                //
                //    // validate the first vtable entry
                //    void* pvtf0 = vfptr[0];
                //    if (IsBadCodePtr((FARPROC)pvtf0))
                //        return false;
                //#endif

                // Free the entire block (including space allocated for the ICoreAllocator*)
                pStashedAllocator->Free(pAllocatedBlockStart);
            }
        }

    }   // namespace Trace
}   // namespace EA











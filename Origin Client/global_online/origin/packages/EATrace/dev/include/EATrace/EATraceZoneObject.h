///////////////////////////////////////////////////////////////////////////////
// EATraceZoneObject.h
// Copyright (c) 2007, Electronic Arts. All rights reserved.
//
// This file was copied from MemoryMan/ZoneObject.cpp in order to avoid a 
// dependency on the MemoryMan package.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Implements the ZoneObject base class, which is mechanism whereby the 
// deleter of an object can 'delete' it without knowing the memory allocator 
// needed to free the object. This is particularly useful for reference 
// counting systems and the 'delete this' often seen in the Release or 
// DecrementRef function.
///////////////////////////////////////////////////////////////////////////////


#ifndef EATRACE_EATRACEZONEOBJECT_H
#define EATRACE_EATRACEZONEOBJECT_H


#include <EABase/eabase.h>
#include <EATrace/internal/Config.h>
#include <coreallocator/icoreallocator_interface.h>
#include <new>


///////////////////////////////////////////////////////////////////////////////
// ZONE_OBJECT_MIN_ALIGN
//
#ifndef ZONE_OBJECT_MIN_ALIGN
    #define ZONE_OBJECT_MIN_ALIGN 16 // 16 because that's what allocators commonly use as a min alignment.
#endif


namespace EA
{
    namespace Trace
    {

        ///////////////////////////////////////////////////////////////////////////////
        /// ZoneObject
        ///
        /// A class derived from ZoneObject is able to be allocated from different 
        /// heaps without the owner of the object needing to know or care which heap
        /// the object came from or even what kind of allocator the object is using.
        /// The owner of the object can use 'delete' or the object can call 'delete'
        /// on itself (useful for reference counting systems) and the object will be 
        /// freed via the correct allocator.
        ///
        /// Zone object uses the ICoreAllocator interface to allocate and free.
        ///
        /// Zone objects have a virtual destructor and thus a vtable. However, the user
        /// doesn't need to use any virtual functions in derived classes.
        ///
        /// EAIOZoneObject-deriving classes can be created via EA_NEW, but don't have 
        /// to be created by it. They can be created by regular new as well.
        ///
        /// Example usage:
        ///     class Widget : public ZoneObject
        ///     {
        ///         uint32_t mRefCount;
        ///
        ///         Widget()
        ///            : mRefCount(0) { }
        ///
        ///         void AddRef()
        ///             { ++mRefCount; }
        ///
        ///         void Release()
        ///         {
        ///             if(mRefCount > 1) 
        ///                 --mRefCount;
        ///             else
        ///                 delete this; // Will use ICoreAllocator to free self.
        ///         }
        ///     };
        ///
        ///     Widget* pWidget = new(pCoreAllocator) Widget;
        ///     pWidget->AddRef();
        ///     pWidget->Release();
        ///////////////////////////////////////////////////////////////////////////////
        class ZoneObject
        {
        public:
            // Destructor must be virtual in order for the the correct value 
            // of 'this' to be passed to operator delete.
            virtual ~ZoneObject() {}

            /// This version uses EA::Trace::GetAllocator to allocate the object. 
            /// Example usage:
            ///    Widget* pWidget = new Widget;
            static void* operator new(size_t n);
            static void  operator delete(void* p);

            /// Example usage:
            ///    Widget* pWidget = new(pCoreAllocator) Widget;
            static void* operator new(size_t n, EA::Allocator::ICoreAllocator* pAllocator);
            static void  operator delete(void* p, EA::Allocator::ICoreAllocator* pAllocator);

            /// Example usage:
            ///    Widget* pWidget = new(pCoreAllocator, "Widget") Widget;
            static void* operator new(size_t n, EA::Allocator::ICoreAllocator* pAllocator, const char* pName);
            static void  operator delete(void* p, EA::Allocator::ICoreAllocator* pAllocator, const char* pName);

            /// Overloads the operator new signature used by EA_NEW.
            /// Example usage:
            ///    Widget* pWidget = EA_NEW(pCoreAllocator) Widget;
            static void* operator new(size_t n, const char* pName, const int flags = 0, 
                                      int debugFlags = 0, const char* pFile = NULL, int pLine = 0);
            static void  operator delete(void* p, const char* pName, const int flags, 
                                         int debugFlags, const char* pFile, int pLine);

        protected:
            /// PPMalloc's minimum alignment is 8 bytes, so we must force at least that much.
            /// We assume that ICoreAllocator-implementing allocators allocate memory on at 
            /// least 8 byte boundaries as well.
            static const uint32_t kOffset = ZONE_OBJECT_MIN_ALIGN;

            static void* DoInternalAllocate(size_t n, EA::Allocator::ICoreAllocator* pAllocator, 
                                            const char* pName, unsigned int flags);
            static void  DoInternalDeallocate(void* p);
        };
    
    }   // namespace Trace
}   // namespace EA


#endif  // Header include guard














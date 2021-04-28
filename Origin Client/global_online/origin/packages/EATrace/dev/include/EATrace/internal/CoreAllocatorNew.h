///////////////////////////////////////////////////////////////////////////////
// CoreAllocatorNew.h
//
// Copyright (c) 2006 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EATRACE_INTERNAL_COREALLOCATORNEW_H
#define EATRACE_INTERNAL_COREALLOCATORNEW_H


#include <EABase/eabase.h>
#include <coreallocator/icoreallocator_interface.h>
#include <EATrace/internal/Config.h>


namespace EA
{
    namespace Trace
    {

        #ifndef EA_TRACE_CA_NEW
            #if EA_TRACE_ENABLED
                #define EA_TRACE_CA_NEW(Class, pAllocator, pName) \
                    new ((pAllocator)->Alloc(sizeof(Class), pName, 0, EA_ALIGN_OF(Class), 0)) Class
            #else
                #define EA_TRACE_CA_NEW(Class, pAllocator, pName) \
                    new ((pAllocator)->Alloc(sizeof(Class), (const char*)NULL, 0, EA_ALIGN_OF(Class), 0)) Class
            #endif
        #endif

        #ifndef EA_TRACE_CA_DELETE
            #define EA_TRACE_CA_DELETE(pObject, pAllocator) EA::Trace::delete_object(pObject, pAllocator)
        #endif

        #ifndef EA_TRACE_CA_NEW_ARRAY
            #if EA_TRACE_ENABLED
                #define EA_TRACE_CA_NEW_ARRAY(Class, pAllocator, pName, nCount) EA::Trace::create_array<Class>(pAllocator, nCount, pName)
            #else
                #define EA_TRACE_CA_NEW_ARRAY(Class, pAllocator, pName, nCount) EA::Trace::create_array<Class>(pAllocator, nCount, (const char*)NULL)
            #endif
        #endif

        #ifndef EA_TRACE_CA_DELETE_ARRAY
            #define EA_TRACE_CA_DELETE_ARRAY(pArray, pAllocator)  EA::Trace::delete_array(pArray, pAllocator)
        #endif


        template <typename T>
        inline void delete_object(T* pObject, Allocator::ICoreAllocator* pAllocator)
        {
            if(pObject) // As per the C++ standard, deletion of NULL results in a no-op.
            {
                pObject->~T();
                pAllocator->Free(pObject);
            }
        }


        #ifdef _MSC_VER
            #pragma warning(push)
            #pragma warning(disable: 4127) // conditional expression is constant
        #endif

        template <typename T>
        inline T* create_array(Allocator::ICoreAllocator* pAllocator, size_t count, const char* pName)
        {
            uint32_t*    p;
            const size_t kAlignOfT = EA_ALIGN_OF(T);

            // We put a uint32_t count in front of the array allocation.
            if(kAlignOfT <= sizeof(uint32_t)) // To do: Make this a compile-time template decision.
                p = (uint32_t*)pAllocator->Alloc(sizeof(uint32_t) + (sizeof(T) * count), pName, 0);
            else
                p = (uint32_t*)pAllocator->Alloc(sizeof(uint32_t) + (sizeof(T) * count), pName, 0, kAlignOfT, sizeof(uint32_t));

            if(p)
            {
                *p++ = (uint32_t)count;

                // Placement new for arrays doesn't work in a portable way in C++, so we implement it ourselves.
                // Note that we don't handle exceptions here. An exception-savvy implementation would undo any
                // object constructions and re-throw.
                for(T* pObject = (T*)(void*)p, *pObjectEnd = pObject + count; pObject != pObjectEnd; ++pObject)
                    new(pObject) T;
            }

            return (T*)(void*)p;
        }

        #ifdef _MSC_VER
            #pragma warning(pop)
        #endif


        template <typename T>
        void delete_array(T* pArray, Allocator::ICoreAllocator* pAllocator)
        {
            if(pArray) // As per the C++ standard, deletion of NULL results in a no-op.
            {
                // Destruct objects in reverse order.
                T* pArrayEnd = pArray + ((uint32_t*)pArray)[-1];
                while(pArrayEnd-- != pArray)
                    pArrayEnd->~T();

                pAllocator->Free(static_cast<void*>((uint32_t*)pArray - 1));
            }
        }
    }
}


#endif // Header include guard


















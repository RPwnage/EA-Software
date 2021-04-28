/////////////////////////////////////////////////////////////////////////////
// Allocator.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_ALLOCATOR_H
#define EAPATCHCLIENT_ALLOCATOR_H


#include <EAPatchClient/Config.h>
#include <EASTL/allocator.h>
#include <coreallocator/icoreallocator_interface.h>


namespace EA
{
    namespace Patch
    {
        /// GetAllocator
        /// 
        /// Gets the default EAIO ICoreAllocator set by the SetAllocator function.
        /// If SetAllocator is not called, the ICoreAllocator::GetDefaultAllocator 
        /// allocator is returned.
        ///
        EAPATCHCLIENT_API Allocator::ICoreAllocator* GetAllocator();


        /// SetAllocator
        /// 
        /// This function lets the user specify the default memory allocator this library will use.
        /// For the most part, this library avoids memory allocations. But there are times 
        /// when allocations are required, especially during startup. Currently the Font Fusion
        /// library which sits under EAIO requires the use of a globally set allocator.
        ///
        EAPATCHCLIENT_API void SetAllocator(Allocator::ICoreAllocator* pCoreAllocator);



        /// CoreAllocator
        ///
        /// Implements the EASTL allocator interface.
        /// Allocates memory from an instance of ICoreAllocator.
        ///
        /// Example usage:
        ///     // Use default allocator.
        ///     eastl::vector<Widget, CoreAllocator> widgetArray;
        ///
        ///     // Use custom allocator.
        ///     eastl::list<Widget, CoreAllocator> widgetList("UI/WidgetList", pSomeCoreAllocator);
        ///     widgetList.push_back(Widget());
        ///
        class CoreAllocator
        {
        public:
            typedef Allocator::ICoreAllocator  allocator_type;
            typedef CoreAllocator              this_type;

        public:
            CoreAllocator(const char* pName = EASTL_NAME_VAL(EAPATCHCLIENT_ALLOC_PREFIX "EASTL"));
            CoreAllocator(const char* pName, allocator_type* pAllocator);
            CoreAllocator(const char* pName, allocator_type* pAllocator, int flags);
            CoreAllocator(const CoreAllocator& x);
            CoreAllocator(const CoreAllocator& x, const char* pName);

            CoreAllocator& operator=(const CoreAllocator& x);

            void* allocate(size_t n, int flags = 0);
            void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
            void  deallocate(void* p, size_t n);

            allocator_type* get_allocator() const;
            void            set_allocator(allocator_type* pAllocator);

            int  get_flags() const;
            void set_flags(int flags);

            const char* get_name() const;
            void        set_name(const char* pName);

        public: // Public because otherwise VC++ generates (possibly invalid) warnings about inline friend template specializations.
            allocator_type* mpCoreAllocator;
            int             mnFlags;    // Allocation flags. See ICoreAllocator/AllocFlags.

            #if EASTL_NAME_ENABLED
                const char* mpName; // Debug name, used to track memory.
            #endif
        };

        bool operator==(const CoreAllocator& a, const CoreAllocator& b);
        bool operator!=(const CoreAllocator& a, const CoreAllocator& b);

    }
}






///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

#include <EAPatchClient/Debug.h> // Need to put this #include down here because the code above needs to compile first, else errors.

namespace EA
{
    namespace Patch
    {

        inline CoreAllocator::CoreAllocator(const char* EASTL_NAME(pName))
            : mpCoreAllocator(EA::Patch::GetAllocator()), mnFlags(0)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName ? pName : EAPATCHCLIENT_ALLOC_PREFIX "EASTL";
            #endif
            EAPATCH_ASSERT(mpCoreAllocator != NULL);
        }

        inline CoreAllocator::CoreAllocator(const char* EASTL_NAME(pName), allocator_type* pCoreAllocator)
            : mpCoreAllocator(pCoreAllocator), mnFlags(0)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName ? pName : EAPATCHCLIENT_ALLOC_PREFIX "EASTL";
            #endif
            EAPATCH_ASSERT(mpCoreAllocator != NULL);
        }

        inline CoreAllocator::CoreAllocator(const char* EASTL_NAME(pName), allocator_type* pCoreAllocator, int flags)
            : mpCoreAllocator(pCoreAllocator), mnFlags(flags)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName ? pName : EAPATCHCLIENT_ALLOC_PREFIX "EASTL";
            #endif
            EAPATCH_ASSERT(mpCoreAllocator != NULL);
        }

        inline CoreAllocator::CoreAllocator(const CoreAllocator& x)
            : mpCoreAllocator(x.mpCoreAllocator), mnFlags(x.mnFlags)
        {
            #if EASTL_NAME_ENABLED
                mpName = x.mpName;
            #endif
            EAPATCH_ASSERT(mpCoreAllocator != NULL);
        }

        inline CoreAllocator::CoreAllocator(const CoreAllocator& x, const char* EASTL_NAME(pName))
            : mpCoreAllocator(x.mpCoreAllocator), mnFlags(x.mnFlags)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName ? pName : EAPATCHCLIENT_ALLOC_PREFIX "EASTL";
            #endif
            EAPATCH_ASSERT(mpCoreAllocator != NULL);
        }

        inline CoreAllocator& CoreAllocator::operator=(const CoreAllocator& x)
        {
            // In order to be consistent with EASTL's allocator implementation, 
            // we don't copy the name from the source object.
            mpCoreAllocator = x.mpCoreAllocator;
            mnFlags         = x.mnFlags;
            EAPATCH_ASSERT(mpCoreAllocator != NULL);
            return *this;
        }

        inline void* CoreAllocator::allocate(size_t n, int /*flags*/)
        {
            // It turns out that EASTL itself doesn't use the flags parameter, 
            // whereas the user here might well want to specify a flags 
            // parameter. So we use ours instead of the one passed in.
            return mpCoreAllocator->Alloc(n, EASTL_NAME_VAL(mpName), (unsigned)mnFlags);
        }

        inline void* CoreAllocator::allocate(size_t n, size_t alignment, size_t offset, int /*flags*/)
        {
            // It turns out that EASTL itself doesn't use the flags parameter, 
            // whereas the user here might well want to specify a flags 
            // parameter. So we use ours instead of the one passed in.
            return mpCoreAllocator->Alloc(n, EASTL_NAME_VAL(mpName), (unsigned)mnFlags, (unsigned)alignment, (unsigned)offset);
        }

        inline void CoreAllocator::deallocate(void* p, size_t n)
        {
            return mpCoreAllocator->Free(p, n);
        }

        inline CoreAllocator::allocator_type* CoreAllocator::get_allocator() const
        {
            return mpCoreAllocator;
        }

        inline void CoreAllocator::set_allocator(allocator_type* pAllocator)
        {
            mpCoreAllocator = pAllocator;
        }

        inline int CoreAllocator::get_flags() const
        {
            return mnFlags;
        }

        inline void CoreAllocator::set_flags(int flags)
        {
            mnFlags = flags;
        }

        inline const char* CoreAllocator::get_name() const
        {
            #if EASTL_NAME_ENABLED
                return mpName;
            #else
                return EAPATCHCLIENT_ALLOC_PREFIX "EASTL";
            #endif
        }

        inline void CoreAllocator::set_name(const char* pName)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName;
            #else
                (void)pName;
            #endif
        }



        inline bool operator==(const CoreAllocator& a, const CoreAllocator& b)
        {
            return (a.mpCoreAllocator == b.mpCoreAllocator) &&
                   (a.mnFlags         == b.mnFlags);
        }

        inline bool operator!=(const CoreAllocator& a, const CoreAllocator& b)
        {
            return (a.mpCoreAllocator != b.mpCoreAllocator) ||
                   (a.mnFlags         != b.mnFlags);
        }
    }
}


#endif // Header include guard















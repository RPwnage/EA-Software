// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

#if !EADPSDK_USE_STD_STL
#include <coreallocator/icoreallocatormacros.h>
#endif

#if EADPSDK_USE_DEFAULT_ALLOCATOR
namespace EA
{
namespace Allocator
{

/*!
 * @brief Overrides the default allocator used by the EADP SDK. 
 * @param allocator the allocator to use as the default allocator
 */
void SetDefaultAllocator(ICoreAllocator* allocator);

}
}

#endif 

namespace eadp
{
namespace foundation
{

/*!
 * @brief Allocator for memory management
 *
 * All the heap memory used by eadp-sdk will be allocated through this class, and it can be
 * customized through HubConfig to allow game control how and where eadp-sdk allocates memory.
 */
class EADPSDK_API Allocator
{
public:
    /*!
     * Default destructor
     */
    ~Allocator() = default;

#if EADPSDK_USE_STD_STL
    /*!
     * Constructor
     */
    Allocator() = default;
#else
    /*!
     * Constructor with ICoreAllocator
     *
     * @param coreAllocator The EA core allocator wrapped; if nullptr, then the default core
     * allocator will be used.
     */
    Allocator(EA::Allocator::ICoreAllocator* coreAllocator)
        : m_coreAllocator(coreAllocator ? coreAllocator : EA::Allocator::ICoreAllocator::GetDefaultAllocator())
    {
        m_stlAllocator.set_allocator(m_coreAllocator);
    }

    /*!
     * Get allocator adapter for STL container
     *
     * @return The EA STL ICoreAllocator adapter, which can be used for EA STL container construction.
     */
    const EA::Allocator::EASTLICoreAllocatorAdapter& getSTLAllocator() const { return m_stlAllocator; }
#endif

    /*!
     * Getter for wrapped ICoreAllcator, used for heap memory allocation
     *
     * @return The ICoreAllocator.
     */
    EA::Allocator::ICoreAllocator* getCoreAllocator() const 
    { 
#if EADPSDK_USE_STD_STL
        return EA::Allocator::ICoreAllocator::GetDefaultAllocator(); 
#else
        return m_coreAllocator; 
#endif
    }

    /*!
     * Template function to create unique pointer
     */
    template <typename T, typename... Args>
    UniquePtr<T> makeUnique(Args&&... args) const
    {
#if EADPSDK_USE_STD_STL
        return UniquePtr<T>(new T(std::forward<Args>(args)...));
#else
        return UniquePtr<T>(CORE_NEW(m_coreAllocator, "eadp::foundation::uniqueptr", EA::Allocator::MEM_TEMP)
                            T(eastl::forward<Args>(args)...), Internal::UniquePtrDeleter<T>(m_coreAllocator));
#endif
    }

    /*!
     * Template function to allocate an STL type instance as a unique pointer
     */
    template <typename T, typename... Args>
    UniquePtr<T> makeUniqueSTL(Args&&... args) const
    {
#if EADPSDK_USE_STD_STL
        return UniquePtr<T>(new T(std::forward<Args>(args)...));
#else
        return UniquePtr<T>(CORE_NEW(m_coreAllocator, "eadp::foundation::uniqueptr", EA::Allocator::MEM_TEMP)
            T(eastl::forward<Args>(args)..., m_stlAllocator), Internal::UniquePtrDeleter<T>(m_coreAllocator));
#endif
    }

    /*!
     * Template function to create shared pointer
     */
    template <typename T, typename... Args>
    SharedPtr<T> makeShared(Args&&... args) const
    {
#if EADPSDK_USE_STD_STL
        return std::make_shared<T>(std::forward<Args>(args)...);
#else
        return eastl::allocate_shared<T>(m_stlAllocator, eastl::forward<Args>(args)...);
#endif
    }

    /*!
     * Template function to allocate an STL type instance as a shared pointer
     */
    template <typename T, typename... Args>
    SharedPtr<T> makeSharedSTL(Args&&... args) const
    {
#if EADPSDK_USE_STD_STL
        return std::make_shared<T>(std::forward<Args>(args)...);
#else
        return eastl::allocate_shared<T>(m_stlAllocator, eastl::forward<Args>(args)..., m_stlAllocator);
#endif
    }
    
    /*!
     * Template function to construct an STL type instance
     */
    template <typename T, typename... Args>
    T make(Args&&... args) const
    {
#if EADPSDK_USE_STD_STL
        return T(std::forward<Args>(args)...);
#else
        return T(eastl::forward<Args>(args)..., m_stlAllocator);
#endif
    }

    /*!
     * Template function to allocate an non-STL type instance as a raw pointer
     */
    template <typename T, typename... Args>
    T* makeRaw(Args&&... args) const
    {
#if EADPSDK_USE_STD_STL
        return new T(std::forward<Args>(args)...);
#else
        return CORE_NEW(m_coreAllocator, "eadp.foundation.Allocator", EA::Allocator::MEM_TEMP)T(eastl::forward<Args>(args)...);
#endif
    }

    /*!
     * Template function to allocate an STL type instance as a raw pointer
     */
    template <typename T, typename... Args>
    T* makeRawSTL(Args&&... args) const
    {
#if EADPSDK_USE_STD_STL
        return new T(std::forward<Args>(args)...);
#else
        return CORE_NEW(m_coreAllocator, "eadp.foundation.Allocator", EA::Allocator::MEM_TEMP)T(eastl::forward<Args>(args)..., m_stlAllocator);
#endif
    }

    /*!
     * Template function to release a raw pointer
     */
    template <typename T>
    void deleteRaw(T* rawPtr) const
    {
#if EADPSDK_USE_STD_STL
        delete rawPtr;
#else
        CORE_DELETE(m_coreAllocator, rawPtr);
#endif
    }

    /*!
     * Get an empty string affiliated with this allocator
     */
    String emptyString() const
    {
        return make<String>();
    }

    /*!
     * Call String construction with placement new operator.
     * This is used in oneof String field that's placed under a union, where the construction of the String needs to be manually controlled using placement new. 
     */
    template <typename... Args>
    void placementNewString(void* address, Args&&... args) const
    {
#if EADPSDK_USE_STD_STL
        new(address) String(std::forward<Args>(args)...);
#else
        new(address) String(std::forward<Args>(args)..., m_stlAllocator);
#endif
    }

private:
#if !EADPSDK_USE_STD_STL
    EA::Allocator::ICoreAllocator* m_coreAllocator;
    EA::Allocator::EASTLICoreAllocatorAdapter m_stlAllocator;
#endif
};

}
}


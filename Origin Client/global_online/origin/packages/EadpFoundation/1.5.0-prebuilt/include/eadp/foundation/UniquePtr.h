// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#if !EADPSDK_USE_STD_STL

#include <eadp/foundation/Config.h>
#include <EASTL/unique_ptr.h>

namespace eadp
{
namespace foundation
{

namespace Internal
{
template <typename T>
struct UniquePtrDeleter
{
public:
    UniquePtrDeleter(EA::Allocator::ICoreAllocator* allocator)
        : m_allocator(allocator)
    {
    }

    template <typename U>  // Enable if T* can be constructed with U* (i.e. U* is convertible to T*).
    UniquePtrDeleter(const UniquePtrDeleter<U>& u, typename eastl::enable_if<eastl::is_convertible<U*, T*>::value>::type* = 0)
        : m_allocator(u.m_allocator)
    {
    }

    void operator()(T* p)
    {
        if (p)
        {
            p->~T();
            m_allocator->Free(p);
        }
    };

    EA::Allocator::ICoreAllocator* m_allocator; // keep it public we can support construction from convertible type
};
}

template <typename T>
class UniquePtr : public eastl::unique_ptr<T, ::eadp::foundation::Internal::UniquePtrDeleter<T>>
{
public:
    using ThisType = UniquePtr<T>;
    using DeleterType = ::eadp::foundation::Internal::UniquePtrDeleter<T>;
    using ParentType = eastl::unique_ptr<T, DeleterType>;
    using Pointer = T*;

    // helpful default constructor
    EA_CPP14_CONSTEXPR UniquePtr() EA_NOEXCEPT
        : ParentType(nullptr, DeleterType(nullptr))
    {
    }

    // helpful nullptr constructor
    EA_CPP14_CONSTEXPR UniquePtr(std::nullptr_t) EA_NOEXCEPT
        : ParentType(nullptr, DeleterType(nullptr))
    {
    }

    // Move constructor
    UniquePtr(Pointer value, typename eastl::remove_reference<DeleterType>::type&& deleter) EA_NOEXCEPT
        : ParentType(value, eastl::move(deleter))
    {
    }

    // Move constructor
    UniquePtr(ThisType&& ref) EA_NOEXCEPT
        : ParentType(eastl::move(ref))
    {
    }

    // Move constructor
    template <typename U>
    UniquePtr(UniquePtr<U>&& u, typename eastl::enable_if<!eastl::is_array<U>::value && eastl::is_convertible<typename UniquePtr<U>::Pointer, Pointer>::value>::type* = 0)
        : ParentType(u.release(), eastl::forward<::eadp::foundation::Internal::UniquePtrDeleter<U>>(u.get_deleter()))
    {
    }

    UniquePtr(const ThisType&) = delete;
    void operator=(const ThisType&) = delete;

    ThisType& operator=(ThisType&& ref) EA_NOEXCEPT
    {
        ParentType::operator=(eastl::move(ref));
        return *this;
    }

    ThisType& operator=(std::nullptr_t aNull) EA_NOEXCEPT
    {
        ParentType::operator=(aNull);
        return *this;
    }

    ~UniquePtr() = default;

    // includes other assignment signatures
    using eastl::unique_ptr<T, DeleterType>::operator=;
};

}
}
#endif

/*! *********************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_OBJECT_H
#define EA_TDF_OBJECT_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/************* Include files ***************************************************************************/
#include <EABase/nullptr.h>
#include <EATDF/internal/config.h>
#include <EATDF/typedescription.h>
#include <EASTL/intrusive_ptr.h>

/************ Defines/Macros/Constants/Typedefs *******************************************************/

namespace EA
{

namespace TDF
{


class TdfObject;
class TdfObjectAllocHelper;

template <class T>
class tdf_ptr : public eastl::intrusive_ptr<T>
{
public:
    typedef typename eastl::intrusive_ptr<T> base_type;

    tdf_ptr() {}
    tdf_ptr(const tdf_ptr& other) : base_type(other) {}
    tdf_ptr(T* p, bool bAddRef = true) : base_type(p, bAddRef) {}

// Const safe versions with backwards compatibility #define:
#if EA_TDF_PTR_CONST_SAFETY
    operator const T* () const { return base_type::get(); }
    operator T* () { return base_type::get(); }
#else
    operator T* () const { return base_type::get(); }
#endif
};

typedef tdf_ptr<TdfObject> TdfObjectPtr;

// Note: Using T* instead of const T* allows these operators to be used on
// both const and non-const T*. Using const T* creates ambiguity when 
// the operators are used on a non-const T*, resulting in compiler errors.

template <class T>
inline bool operator==(tdf_ptr<T> const& ptr1, T* p) { return (ptr1.get() == p); }

template <class T>
inline bool operator!=(tdf_ptr<T> const& ptr1, T* p) { return (ptr1.get() != p); }

template <class T>
inline bool operator==(T* p, tdf_ptr<T> const& ptr2) { return (p == ptr2.get()); }

template <class T>
inline bool operator!=(T* p, tdf_ptr<T> const& ptr2) { return (p != ptr2.get()); }

template<class T> 
inline bool operator!=(std::nullptr_t, tdf_ptr<T> const& ptr) { return ptr.get() != nullptr; }

template<class T> 
inline bool operator==(std::nullptr_t, tdf_ptr<T> const& ptr) { return ptr.get() == nullptr; }

template<class T> 
inline bool operator!=(tdf_ptr<T> const& ptr, std::nullptr_t) { return ptr.get() != nullptr; }

template<class T> 
inline bool operator==(tdf_ptr<T> const& ptr, std::nullptr_t) { return ptr.get() == nullptr; }

class EATDF_API TdfObjectAllocHelper
{
public: 

    template<typename T>
    T* operator<<(T* obj) { fixupRefCount(obj); return obj; }

private:
    void fixupRefCount(void* obj) { }; // do nothing
    void fixupRefCount(TdfObject* obj);
};

// Helper functions to create a copy of the given value. 'Primitive' types used as map keys (blob, string) overload this to use the correct allocator. 
template <typename T>
struct TdfPrimitiveAllocHelper
{
    T operator()(const T& value, EA::Allocator::ICoreAllocator& allocator) const
    {
        return value;
    }
    T operator()(EA::Allocator::ICoreAllocator& allocator) const
    {
        return T();
    }
};

/*! ***********************************************************************/
/*! \class TdfObject
    \brief Abstract base class for all Tdf object(Tdf, Union, Collection, Blob) types
***************************************************************************/
class EATDF_API TdfObject
{

friend class TdfObjectAllocHelper;

public:
    TdfObject() : mRefCount(NONHEAP_REF_INIT) {}
    virtual ~TdfObject() {}

    TdfId getTdfId() const { return getTypeDescription().getTdfId(); }
    TdfType getTdfType() const { return getTypeDescription().getTdfType(); }
    virtual const TypeDescription& getTypeDescription() const = 0;
    virtual void copyIntoObject(TdfObject& newObj, const MemberVisitOptions& options = MemberVisitOptions()) const = 0;

    template <class TdfObjectType>
    static TdfObject* createInstance(EA::Allocator::ICoreAllocator& allocator, const char8_t* debugMemName, uint8_t* placementBuf = nullptr) { return (placementBuf == nullptr) ? EA::TDF::TdfObjectAllocHelper() << new (TdfObject::alloc(sizeof(TdfObjectType), allocator, debugMemName, 0)) TdfObjectType(allocator, debugMemName) : new (placementBuf) TdfObjectType(allocator, debugMemName); }
        
    static void* operator new(size_t size, EA::Allocator::detail::opNonArrayNew, EA::Allocator::ICoreAllocator* allocator, const char* name, unsigned int flags) { return alloc(size, *allocator, name, flags); } 
    static void* operator new(size_t size, EA::Allocator::detail::opNonArrayNew, EA::Allocator::ICoreAllocator* allocator, const EA::Allocator::ICoreAllocator::DebugParams debugParams, unsigned int flags) { return alloc(size, *allocator, debugParams, flags); } 
    static void* operator new(size_t size, void* placement) { return placement; }
    static void* operator new(size_t size, EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN) { return alloc(size, allocator, "TDF_DEFAULT_ALLOC", 0); }

    static void operator delete(void* p, EA::Allocator::detail::opNonArrayNew, EA::Allocator::ICoreAllocator* allocator, const char* name, unsigned int flags) { if (p == nullptr) return; EA_ASSERT_MSG(((reinterpret_cast<TdfObject*>(p))->isNotRefCounted() || (reinterpret_cast<TdfObject*>(p))->mRefCount == 0), "Delete invoked on TdfObject tracking mRefCount but mRefCount != 0"); TdfObject::free(p); } 
    static void operator delete(void* p, EA::Allocator::detail::opNonArrayNew, EA::Allocator::ICoreAllocator* allocator, const EA::Allocator::ICoreAllocator::DebugParams debugParams, unsigned int flags) { if (p == nullptr) return; EA_ASSERT_MSG(((reinterpret_cast<TdfObject*>(p))->isNotRefCounted() || (reinterpret_cast<TdfObject*>(p))->mRefCount == 0), "Delete invoked on TdfObject tracking mRefCount but mRefCount != 0"); TdfObject::free(p); } 
    static void operator delete(void* p, void* placement) { /*intentionally empty*/ }
    static void operator delete(void* p, EA::Allocator::ICoreAllocator& allocator) { if (p == nullptr) return; EA_ASSERT_MSG(((reinterpret_cast<TdfObject*>(p))->isNotRefCounted() || (reinterpret_cast<TdfObject*>(p))->mRefCount == 0), "Delete invoked on TdfObject tracking mRefCount but mRefCount != 0"); TdfObject::free(p);  }
    static void operator delete(void* p) { if (p == nullptr) return; EA_ASSERT_MSG(((reinterpret_cast<TdfObject*>(p))->isNotRefCounted() || (reinterpret_cast<TdfObject*>(p))->mRefCount == 0), "Delete invoked on TdfObject tracking mRefCount but mRefCount != 0"); TdfObject::free(p);  }

    // Sets the Ref count to 0 if reference counting was not in use (i.e. the allocation was made without using createInstance)
    // This does not fix the 
    void startRefCounting();
    inline bool isNotRefCounted() const { return mRefCount == NONHEAP_REF_INIT; }

    void AddRef() const
    {
        if (EA_UNLIKELY(isNotRefCounted())) return;

        mRefCount++;
    }

    void Release() const
    {
        if (EA_UNLIKELY(isNotRefCounted())) return; 
        
        if (--mRefCount == 0) 
            delete this;
    }

protected:
    static void* alloc(size_t size, EA::Allocator::ICoreAllocator& allocator, const char* name, unsigned int flags);
    static void* alloc(size_t size, EA::Allocator::ICoreAllocator& allocator, const EA::Allocator::ICoreAllocator::DebugParams debugParams, unsigned int flags);
    static void free(void* p);

private:    
    typedef uint32_t ref_count_type;
    mutable ref_count_type mRefCount;
    static const ref_count_type NONHEAP_REF_INIT = 1U << 31U;
};


#if EA_TDF_INCLUDE_CHANGE_TRACKING
class EATDF_API TdfChangeTracker
{
public:
    TdfChangeTracker()
        :   mIsSet(false)
    { }
    void markSet() { mIsSet = true; }
    void clearIsSet() { mIsSet = false; }
    bool isSet() const { return mIsSet; }
protected:
    bool mIsSet : 1;
};
#endif

} //namespace TDF

} //namespace EA


namespace EA
{
    namespace Allocator
    {
        namespace detail
        {
            template <>
            inline void DeleteObject(Allocator::ICoreAllocator*, ::EA::TDF::TdfObject* object) { delete object; }
        }
    }
}


#endif


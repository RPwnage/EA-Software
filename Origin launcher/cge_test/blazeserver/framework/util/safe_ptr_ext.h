/*************************************************************************************************/
/*!
    \file

    
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef SAFE_PTR_EXT_H
#define SAFE_PTR_EXT_H


#include <EASTL/safe_ptr.h>

/// safe_ptr_ext
///
/// safe_ptr_ext is an extension of the safe_ptr class. The only difference is that 
/// this class calls a helper function (deleteIfUnreferenced) when a reference is removed.
/// The function is passed in true when the last safe_ptr is being removed.
///
/// Additionally ==, and != operator were added for pointers, to allow statements like: if (ptr == nullptr), to work.
///
template<class T>
class safe_ptr_ext : public eastl::safe_ptr<T>
{
public:
    safe_ptr_ext() : eastl::safe_ptr<T>() {};                                    /// nullptr init
    explicit safe_ptr_ext(T* pObject) : eastl::safe_ptr<T>(pObject) {};          /// Construct a safeptr from a naked pointer.
 
    ~safe_ptr_ext()
    {
        // Delete object if this is the last reference:
        if(this->mpObject && this->unique())
            ((T*)this->mpObject)->deleteIfUnreferenced(true);
    }

    bool operator==(const T* ptr) const                /// Returns true if ptr points to the same object as this.
    {
        return ptr == this->mpObject;
    }
    bool operator!=(const T* ptr) const                /// Returns false if ptr points to the same object as this.
    {
        return ptr != this->mpObject;
    }

    typename eastl::safe_ptr<T>::this_type& operator=(const typename eastl::safe_ptr<T>::this_type& safePtr)     /// Assignment operator.
    {
        T* pObj = (T*)this->mpObject;
        if(this != &safePtr)
            this->reset(safePtr.mpObject);
        if(pObj)
            pObj->deleteIfUnreferenced();
        return *this;
    }
    typename eastl::safe_ptr<T>::this_type& operator=(T* const pObject)             /// Assign this to a naked pointer.
    {
        T* pObj = (T*)this->mpObject;
        this->reset(pObject);
        if(pObj)
            pObj->deleteIfUnreferenced();
        return *this;
    }
};


/// safe_ptr_ext_owner
///
/// safe_ptr_ext_owner is a class where the pointer references are stored on the owner class, while the
/// safe_ptr_ext_owner itself automatically casts to the child class. 
///

template<class ChildT, class OwnerT>
class safe_ptr_ext_owner 
{
public:
    typedef ChildT                          value_type;
    typedef OwnerT                          owner_value_type;
    typedef safe_ptr_ext<ChildT>            value_type_ptr;
    typedef safe_ptr_ext<OwnerT>            owner_value_type_ptr;
    typedef safe_ptr_ext_owner<ChildT, OwnerT>  this_type;

public:
    safe_ptr_ext_owner() {}                                                                               /// Default constructor.
    explicit safe_ptr_ext_owner(ChildT* pObject) : mChildPtr(pObject), mOwnerPtr(pObject != nullptr ? pObject->getOwner() : nullptr)  {}            /// Construct a safeptr from a naked pointer.
    safe_ptr_ext_owner(const value_type_ptr& pObject) : mChildPtr(pObject), mOwnerPtr(pObject != nullptr ? pObject->getOwner() : nullptr) {};          /// Construct a safeptr from a child safe ptr
   
    this_type& operator=(ChildT* const pObject)             /// Assign this to a naked pointer.
    {
        mChildPtr = pObject;
        mOwnerPtr = pObject != nullptr ? pObject->getOwner() : nullptr;
       
        return *this;
    }

    bool operator==(const this_type& rhs) const         /// Returns true if safePtr points to the same object as this.
    {
        return (mChildPtr == rhs.mChildPtr);
    }

    bool operator==(const ChildT* ptr) const                /// Returns true if ptr points to the same object as this.
    {
        return mChildPtr == ptr;
    }
    bool operator!=(const ChildT* ptr) const                /// Returns false if ptr points to the same object as this.
    {
        return mChildPtr != ptr;
    }

public:
    ChildT* get() const          { return mChildPtr.get(); }                                     /// Get the naked pointer from this safe ptr.
    operator ChildT*() const     { return mChildPtr.get();}                               /// Implicit safe_ptr<T> -> T* conversion operator.
    ChildT* operator->() const   { return  mChildPtr.get();}                               /// Member operator.
    ChildT& operator*() const    { return *mChildPtr.get();}                               /// Dereference operator.
    bool operator!() const  { return (mChildPtr == nullptr); }                  /// Boolean negation operator.

    typedef ChildT* (this_type::*bool_)() const;             /// Allows for a more portable version of testing an instance of this class as a bool.
    operator bool_() const                              // A bug in the CodeWarrior compiler forces us to implement this inline instead of below.
    {
        if(mChildPtr != nullptr)
            return &this_type::get;
        return nullptr;
    }

    value_type_ptr mChildPtr;
    owner_value_type_ptr mOwnerPtr;
};






///////////////////////////////////////////////////////////////////////////////
// global operators
///////////////////////////////////////////////////////////////////////////////

template<class ChildT, class OwnerT>
inline bool operator==(const safe_ptr_ext_owner<ChildT, OwnerT>& safePtr, const ChildT* pObject)
{
    return (safePtr.get() == pObject);
}


template<class ChildT, class OwnerT>
inline bool operator!=(const safe_ptr_ext_owner<ChildT, OwnerT>& safePtr, const ChildT* pObject)
{
    return (safePtr.get() != pObject);
}


template<class ChildT, class OwnerT>
inline bool operator<(const safe_ptr_ext_owner<ChildT, OwnerT>& safePtrA, const safe_ptr_ext_owner<ChildT, OwnerT>& safePtrB)
{
    return (safePtrA.get() < safePtrB.get());
}

#endif // SAFE_PTR_EXT_H
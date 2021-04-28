/*************************************************************************************************/
/*!
    \file frameworkresource.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FRAMEWORK_RESOURCE_H
#define BLAZE_FRAMEWORK_RESOURCE_H

/*** Include files *******************************************************************************/

#include "EASTL/intrusive_list.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class FrameworkResourcePtrBase;
class Fiber;

//  Represents a resource managed by the server framework as exposed to components.
//  Typically resources are contained within lists managed by the server framework.
//  
//  All resources must support a method to release its allocated resource.
//

class FrameworkResource : public eastl::intrusive_list_node
{
    NON_COPYABLE(FrameworkResource);

    friend class Fiber;                         // Allows Fiber, which mangages these resources to release them on fiber shutdown.
    friend class FrameworkResourcePtrBase;      // FrameworkResourcePtrBase will modify the refcounts and potentially release resources.

    // These are specified so that this derived class can access the clearOwningFiber() call.  
    // Normally this is not desired, but is done to support the deprecated interface where callers 
    // explicitly deleted result objects (that will have been assigned a fiber on creation from a query method.)
    friend class DbResult;                      
    friend class StreamingDbResult;

public:
    int32_t getRefCount() const { return mRefCount; }
    virtual const char8_t* getTypeName() const = 0;

    //  detaches the resource from the fiber it was created on, allowing resources to span calls to multiple fibers.
    void setOwningFiber(Fiber& fiber) { mOwningFiber = &fiber; }
    void clearOwningFiber();

protected:
    FrameworkResource(): mRefCount(0), mOwningFiber(nullptr) {}  
    virtual ~FrameworkResource() {}
    virtual void assignInternal() = 0;      // customized behavior for assign: when the framework assigns a resource to this container.
    virtual void releaseInternal() = 0;     // customized behavior for release: when the framwork releases resources from this container.

protected:
    //  assigns the resource to a fiber and invokes assignInternal - pass nullptr if assigning to no fiber (which will just call assignInternal.)
    void assign();
    // common release utility (invokes releaseInternal)
    void release();

    Fiber *getOwningFiber() const { return mOwningFiber; }

private:
    void incRefCount() 
    { 
        ++mRefCount; 
    }
    //  releases resource when ref count reaches zero
    void decRefCount() 
    {
        EA_ASSERT(mRefCount > 0);
        if (mRefCount > 0)
        {
            --mRefCount;
            if (mRefCount == 0)
            {
                release();
            }
        }
    }

    int32_t mRefCount;   
    Fiber* mOwningFiber;
};


//  FrameworkResourcePtrBase allows direct manipluation of a FrameworkResource object's reference
//  counting (see FrameworkResource friend relationships.)   Having a non-templatized base class
//  prevents FrameworkResource from having to befriend all template versions of FrameworkResourcePtr.
//
class FrameworkResourcePtrBase
{
protected:
    FrameworkResourcePtrBase(FrameworkResource *ptr): mPtr(ptr) {}
    FrameworkResourcePtrBase(const FrameworkResourcePtrBase& handle): mPtr(handle.mPtr) 
    {
        if (mPtr != nullptr)
        {
            mPtr->incRefCount();
        }
    }
    ~FrameworkResourcePtrBase() 
    {
        if (mPtr != nullptr)
        {
            mPtr->decRefCount();
        }
    }

protected:
    FrameworkResource *mPtr;
    void attachInternal(FrameworkResource *ptr, bool allowPrereferencedResource) 
    {
        //  note, FrameworkResource is friends with FrameworkResourcePtrBase, so that it can exposes FrameworkResource::incRefCount and decRefCount methods.
        if (ptr != nullptr)
        {
            ptr->incRefCount();
            if (!allowPrereferencedResource)
            {
                EA_ASSERT(ptr->getRefCount() == 1);
            }
        }
        if (mPtr != nullptr)
        {
            mPtr->decRefCount();
        }
        mPtr = ptr;
    }
};


//  Takes ownership of a FrameworkResource derived class T.  As long as any reference of FrameworkResource<T> 
//  exists the contained object remains valid.
//  
//  class T must implement the following methods.
//      T::release(), releases the resource
// 
template<class T> 
class FrameworkResourcePtr: public FrameworkResourcePtrBase
{
public:
    //  An empty FrameworkResourcePtr<T>.  Use the assignment operator to assign an existing FrameworkResourcePtr.
    FrameworkResourcePtr(): FrameworkResourcePtrBase(nullptr) {}

    //  Assign an existing FrameworkResourcePtr to a new FrameworkResourcePtr.
    FrameworkResourcePtr(const FrameworkResourcePtr& handle): FrameworkResourcePtrBase(handle) {}   

    //  Assign this ObjectPtr to the new value, releasing its current reference.
    FrameworkResourcePtr& operator=(const FrameworkResourcePtr& rhs) 
    {
        attachInternal(rhs.mPtr, true);
        return *this;
    }

    //  Access operators to the containing object.
    T* operator->() {
        return static_cast<T*>(mPtr);
    }
    T& operator*() {
        return *static_cast<T*>(mPtr);
    }

    const T* operator->() const {
        return static_cast<const T*>(mPtr);
    }
    const T& operator*() const {
        return *static_cast<const T*>(mPtr);
    }

    //  comparison operators
    bool operator!=(T *ptr) const {
        return mPtr != ptr;
    }
    bool operator==(T *ptr) const {
        return mPtr == ptr;
    }

public:
    // assignment of a FrameworkResource to a FrameworkResourcePtr.  
    //  This method invokes an internal attach method generalized for FrameworkResource based objects.  The purpose
    //  of this public attach method is to guard against attaching arbitrarty FrameworkResource based objects to the templatized
    //  FrameworkResourcePtr class.
    void attach(T* ptr) {
        attachInternal(ptr, false);
    }
};

// used as an alternative for validating FrameworkResourcePtr based object instances.
template<class T> inline bool operator!(const FrameworkResourcePtr<T>& ptr) 
{ 
    return ptr == nullptr; 
} 

} // Blaze

#endif // BLAZE_FRAMEWORK_RESOURCE_H

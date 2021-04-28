/*************************************************************************************************/
/*!
    \file



    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*************************************************************************************************/
/*!
    \class FiberLocalVarBase
    \class FiberLocalVar
    \class FiberLocalManagedPtr
    \class FiberLocalPtr
    \class FiberLocalWrappedPtr

    --------------------------------------------------------------------------------
        Overview
    --------------------------------------------------------------------------------

    The FiberLocalVar classes provide a method of having local variables tied to a specific fiber,
    such that these variables point to a fiber specific section of memory and will return that fiber's
    copy of the memory when queried during a fiber's execution.

    In many ways this Fiber Local Storage (FLS) is exactly like Thread Local Storage (TLS), except per fiber.  Each fiber has a chunk
    of memory that represents the actual storage for all the fiber local variables in the system.  This memory 
    is allocated when the fiber is created and persists until the fiber is shut down.  The fiber local 
    storage is referenced by a single pointer, and the current pointer to the fiber local storage is held in a thread
    local member of the FiberLocalVarBase class.  Whenever a fiber switches context, it updates the global 
    FLS pointer to be that of the current fiber's storage area.  In this way the context switch between 
    fibers costs a single pointer exchange for fiber local storage.  

    Internally, each fiber local variable is declared as a process wide global.  When execution begins, each
    variable uses static initialization to register itself with the primary fiber local storage list.  In this way 
    by the time "main" is hit and the first fiber is initialized we have a list of all the fiber locals
    that will be created per fiber, as well as a total storage size to allocate per fiber.  Each fiber local global also
    has an offset into the storage so it knows how to get at its data given any block of fiber local storage.

    When a fiber initializes, storage is allocated in the total size of all fiber locals.  Then the initFiberLocals method
    walks the list of register fiber locals and initializes each one to a section of memory.  Initialization usually means 
    setting a default value or doing a placement new.

    Each fiber local is a templated class that has getter methods and casting overrides that access the internals of the 
    variable.  When a "getter" method is called, the offset calculated during global registration is used to return the actual
    memory for the current active fiber local storage.

    --------------------------------------------------------------------------------
    Classes
    --------------------------------------------------------------------------------

    Because Fiber Local Storage is not built into the compiler, we cannot rewrite accessors with instructions to offset a memory access
    by the global thread local pointer.  Instead, we use templates to do a very similar task.  This has the benefit in that
    we aren't restricted to POD types like thread locals, but the drawback that its a bit more complicated to declare a fiber local 
    variable than a thread local.

    There are four types of templated classes for FLS that correspond to four potential data that can be stored per fiber.

    FiberLocalVar - This represents the actual instantiation of a POD variable.  For example, if "FiberLocalVar<int> foo" is declared, 
                    foo can be used any place you would use "int".

    FiberLocalPtr - This is a pointer class which can be used any place you'd use "T*".  This has to be a special class because
                    the class overrides "->" to do the right thing when this variable is accessed.
    
    FiberLocalManagedPtr - This class is a departure from TLS because it allows the creation of complex types.  When this is used
                    the memory for the underlying object is initialized and managed by the FLS system.  When the fiber is destroyed,
                    so is the object.  

    FiberLocalWrappedPtr - This class is used much like FiberLocalPtr, but for cases where the underlying data is _already_ wrapped
                    by a safe_ptr or auto_ptr liked class.  This class takes both the wrapped pointer type (W) and the class type (T)
                    as template arguments, and will ensure that you can still transparently access the underlying data.
    
    --------------------------------------------------------------------------------
    Special Helpers
    --------------------------------------------------------------------------------

    Two special helpers exist to make management of fiber locals easier:

    * resetFiberLocals will clear and reinitialize all local variables attached to this fiber.  This is accomplished
      by essentially repeating the process of initialization without reallocating the memory.  Managed pointers
      will have their placement destructor called, then placement new.

    * copyFromCallingFiber copies the value of this method from the calling fiber.  This allows you to carry forward
      important fiber variables across fiber calls.  Generally done immediately when the fiber is called to allow 
      state to transition properly.

*/
/*************************************************************************************************/


#ifndef BLAZE_FIBER_LOCAL_H
#define BLAZE_FIBER_LOCAL_H

/*** Include files *******************************************************************************/

#include "EASTL/intrusive_list.h"
#include "framework/system/fiber.h"

namespace Blaze
{

class FiberLocalVarBase : public eastl::intrusive_list_node
{
public:
    static uint8_t* initFiberLocals(MemoryGroupId memId);
    static void resetFiberLocals();
    static void destroyFiberLocals(uint8_t* mem);
    static void setFiberLocalStorageBase(uint8_t* base) { msFiberLocalStorageBase = base; }
    static bool isValidStorage() { return msFiberLocalStorageBase != nullptr; }

protected:
    friend class Fiber; // needs access to calcCallingFiberAddr()
    FiberLocalVarBase(size_t varSize);
    virtual ~FiberLocalVarBase();

    virtual void init(uint8_t* mem) {}
    virtual void reset() {}
    virtual void destroy(uint8_t* mem) {}

    uint8_t* calcAddr() const { return msFiberLocalStorageBase + mOffset; }
    uint8_t* calcAddr(const Fiber& fiber) { return fiber.mFiberLocalStorage + mOffset; }
    uint8_t* calcCallingFiberAddr() { return Fiber::msCurrentFiber->getCaller()->mFiberLocalStorage + mOffset; }

private:
    size_t mOffset;

    typedef eastl::intrusive_list<FiberLocalVarBase> FiberLocalList;
    static FiberLocalList& getGlobalFiberLocalList();
    static size_t& getGlobalVarSize();

    static EA_THREAD_LOCAL uint8_t* msFiberLocalStorageBase;
};



template <typename T>
class FiberLocalVar : public FiberLocalVarBase
{
public:
    FiberLocalVar() : FiberLocalVarBase(sizeof(T)) {}
    FiberLocalVar(T initialValue) : FiberLocalVarBase(sizeof(T)), mInitialValue(initialValue) {}

    void init(uint8_t* mem) override 
    {
        new (mem) T(mInitialValue); 
    }
    
    void reset() override
    {
        operator=(mInitialValue);
    }

    operator T() { return *((T*) calcAddr()); }
    operator const T() const  { return *((T*) calcAddr()); }
    T operator=(T val) { *((T*) calcAddr()) = val; return val;}

    T& get(const Fiber& fiber) { return *((T*) calcAddr(fiber)); }

    void copyFromCallingFiber()
    {
        operator=(*((T*) calcCallingFiberAddr()));
    }

private:
    T mInitialValue;
};


template <typename T>
class FiberLocalManagedPtr : public FiberLocalVarBase
{
public:
    FiberLocalManagedPtr() : FiberLocalVarBase(sizeof(T*) + sizeof(T)) {}    

    void init(uint8_t* mem) override 
    {     
       T* obj = new (mem + sizeof(T*)) T;
       *((T**)mem) = obj;
    }

    void reset() override
    {
        ((T*) (calcAddr() + sizeof(T*)))->~T();      
        new (calcAddr() + sizeof(T*)) T;
    }
        
    void destroy(uint8_t* mem) override
    {
        ((T*) (mem + sizeof(T*)))->~T();        
    }

    operator T*() { return *((T**)calcAddr()); }
    T& operator=(const T& val) { T& obj = **((T**)calcAddr()); obj = val; return obj; }
    T* operator->() { return *((T**)calcAddr()); }
    T& operator*() { return **((T**)calcAddr()); }

    T* get(const Fiber& fiber) { return *((T**)calcAddr(fiber)); }

    void copyFromCallingFiber()
    {
        operator=(**((T**)calcCallingFiberAddr()));
    }
};

template <typename T>
class FiberLocalPtr : public FiberLocalVarBase
{
public:
    FiberLocalPtr() : FiberLocalVarBase(sizeof(T**)) {}    

    void init(uint8_t* mem) override 
    { 
        memset(mem, 0, sizeof(T**));
    }

    void reset() override
    {
        operator=(nullptr);
    }

    operator T*() { return *((T**)calcAddr()); }
    T* operator=(T* val) { *((T**)calcAddr()) = val; return val;}
    T* operator->() { return *((T**)calcAddr()); }
    T& operator*() { return **((T**)calcAddr()); }

    T* get(const Fiber& fiber) { return *((T**)calcAddr(fiber)); }

    void copyFromCallingFiber()
    {
        operator=(*((T**)calcCallingFiberAddr()));
    }
};

template <typename W, typename T>
class FiberLocalWrappedPtr : public FiberLocalVarBase
{
public:
    FiberLocalWrappedPtr() : FiberLocalVarBase(sizeof(W)) {}
    FiberLocalWrappedPtr(T* initialValue) : FiberLocalVarBase(sizeof(W)), mInitialValue(initialValue) {}

    void init(uint8_t* mem) override { new (mem) W; }
    void reset() override
    {
        operator=(nullptr);
    }

    operator T*() { return &(*(*((W*)calcAddr()))); }
    T* operator=(T* val) { *((W*)calcAddr()) = val; return val;}
    T* operator->() { return &(*(*((W*)calcAddr()))); }
    T& operator*() { return **((W*)calcAddr()); }
    W& get() { return *((W*) calcAddr()); }

    W& get(const Fiber& fiber) { return *((W*) calcAddr(fiber)); }

    void copyFromCallingFiber()
    {
        *((W*)calcAddr()) = *((W*) calcCallingFiberAddr());
    }
private:
    T* mInitialValue;
};

} // Blaze

#endif // BLAZE_FIBER_LOCAL_H


/*! **********************************************************************************************/
/*!
    \file

    Functor callback for member function callbacks.  For more info, see \ref functorsExplained 
    "Functors Explained".


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

/*! **********************************************************************************************/
/*! \page functorsExplained Functors Explained

    The functor implementation used in BlazeSDK can seem a bit tricky at first, so this guide gives a little
    more detail on the implementation and use of functors.  There are two types of functors, 
    C function pointers and C++ member function pointers (MFPs).  The functor object allows both 
    to be wrapped in a stack allocated object that can be passed around and executed without having 
    to know what type of callback is desired.  

    \section cstyle C Function Pointers

    C-style function pointers are fairly trivial to implement.  The function pointer is passed
    to the functor constructor, and when the () operator is called, the function pointer is 
    dereferenced as normal.  And that's it.  Nothing fancy to see here.

    \par Example:
    \code   
    void someFunction(int x) {...}
    ...
    Functor1<int> testFunc(someFunction);
    testFunc(10); //calls "someFunction" with integer 10.
    \endcode

    \section mfps Member Function Pointers

    Member function pointers, on the other hand, are a little more tricky.  First off, MFPs do not have 
    the standard pointer size of the platform.  They can be anywhere from 8 to 20 bytes depending on 
    the type of class pointed to and the compiler used.  See http://www.codeproject.com/cpp/FastDelegate.asp
    for an excellent overview of pointer sizes.  

    To get around that, we store the MFP as a block of memory inside the FunctorBase class.  This 
    memory is in a union with where we put the C function pointer, so there's no wasted space.  In 
    addition to the member function pointer, we need the object it will point to, and a pointer to
    the "execute" function.  The execute function handles the main problem with MFPs and this implementation:
    how to get the member function pointer out of the void * buffer.  Each class type and function
    signature is unique, so there has to be a unique casting made for each functor type.  The execute function
    handles that by casting the void memory to tbe type of the member function pointer, and then making
    the call from there.  

    \code
    template <class T, class MemFunc>
    static void ExecuteFunction(const Functor *base)
    {
        T *object = (T *) base->getObject();
        MemFunc &memFunc(*(MemFunc*)(base->getMemberFunction()));
        (object->*memFunc)();
    }
    \endcode

    This templated function is instantiated in the constructor of the functor class like so:
    
    \code
    template <class T>
    Functor(T* obj, void (T::*func)())
        : FunctorBase((void *)(ExecuteFunc) (Functor::template ExecuteFunction<T, void (T::*)()>), 
                  (void *) obj, &func, sizeof(func)) {}
    \endcode

    Notice the usage of the "template" keyword to give context to the compiler as to exactly which
    version of the static function "ExecuteFunction" we are refering to.  Also notice that the \e pointer
    to the MFP "func" is passed as opposed to the MFP itself.  By pointing to the memory where the MFP
    lives, we let the FunctorBase class know where to copy the MFP off from.  In addition, the sizeof
    passes the size of the memory pointed to by "&func", which is critical on MSVC platforms where the 
    MFP size can vary.

    Usage of an MFP functor is very similar to the C-style functors, with the exception that both the 
    pointer to the object and pointer to the member function are passed to the functor constructor.

    \par Example:
    \code   
    class someClass
    {
    public:
        void someFunction(int x) {...}
    };
    ...
    someClass myInstance;
    Functor1<int> testFunc(myInstance, &someClass::someFunction);
    testFunc(10); //calls "someClass::someFunction" on "myInstance" with integer 10.
    \endcode

    \note A very easy mistake is to not clean up a functor when an object is removed.  Remember 
    when using functors to always provide a method to clear the functors of objects that are 
    being deleted.

    \section references References
    For further reading about this method of implementing functors, see 
    "Callbacks in C++ Using Template Functors" by Rich Hickey at 
    http://www.tutok.sk/fastgl/callback.html

**************************************************************************************************/


#ifndef BLAZE_CALLBACK_H
#define BLAZE_CALLBACK_H

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/debug.h"
#else
#include "EAAssert/eaassert.h"
#ifdef EA_PLATFORM_WINDOWS
#define BLAZESDK_CC _cdecl
#else
#define BLAZESDK_CC 
#endif
#endif
#include <string.h>

namespace Blaze
{

/*! ************************************************************************/
/*! \class FunctorBase
    \brief The base class for all functor types. 

    For more info, see \ref functorsExplained "Functors Explained".

    \note This base class should only be used to store other functors.  To make
    any actual use of it, it must be casted to the appropriate type.
*****************************************************************************/
class FunctorBase
{
public:
    
    /*! \brief Clears out the functor */
    FunctorBase() { clear(); } 

    /*! \brief Clears out the functor */
    void clear() {mExecuteFunc = nullptr; mObject = nullptr; memset(mMemberFunc, 0, sizeof(mMemberFunc));}

    /*! \brief Returns true if the functor has valid callback information. */
    bool isValid() const {return (mExecuteFunc != 0 || mFunc != 0);}

    /*! \brief Gets the object associated with this functor (if any). 
        
        This function is useful to see if a set of functors compare to a
        particular objec.
    */
    void *getObject() const {return mObject;}
    
    /*! \brief Returns true if the given functor is equal to this one. */
    bool operator==(const FunctorBase &rhs) const
    {
        return (mObject == rhs.mObject && memcmp(mMemberFunc, rhs.mMemberFunc, sizeof(mMemberFunc)) == 0 );
    }

/*! \cond INTERNAL_DOCS */
protected:
    //Only derived classes should construct this.   
    
    FunctorBase(void * function) 
    {
        clear();
        mFunc = function;
    }

    FunctorBase(void * executePtr, void *object, void *memFunc, size_t memFuncSize)
    {
        mExecuteFunc = executePtr;
        mObject = object;
        memset(mMemberFunc, 0, sizeof(mMemberFunc));
        
#ifdef BLAZE_CLIENT_SDK
        BlazeVerify(memFuncSize <= sizeof(mMemberFunc) 
                && "ERROR: MEMBER FUNCTION POINTER SIZE TOO LARGE!!!");
#else
        EA_ASSERT_MSG(memFuncSize <= sizeof(mMemberFunc), "ERROR: MEMBER FUNCTION POINTER SIZE TOO LARGE!!!");
#endif
        memcpy(mMemberFunc, memFunc, memFuncSize);
    }   

    void *getExecuteFunc() const {return mExecuteFunc;}
    void *getFunction() const {return mFunc;}
    const char *getMemberFunction() const {return mMemberFunc;}

private:
    void *mExecuteFunc;
    void *mObject;
    union
    {
        void *mFunc;
        char mMemberFunc[16];
    };
/*! \endcond */
};

/*! ************************************************************************/
/*! \class Functor
    \brief A functor with no parameters.

    For more info, see \ref functorsExplained "Functors Explained".
*****************************************************************************/
class Functor : public FunctorBase
{
public: 
    /*! \brief The C function type for this functor */
    typedef void (BLAZESDK_CC *FuncCallback)();

    /*! \brief Creates an empty, invalid functor. */
    Functor() {}

    /*! \brief Creates a C-style functor. 
        \param function Function to call.
    */
    Functor(FuncCallback function) : FunctorBase((void *) function) {}

    /*! \brief Creates a C++ member pointer functor. 
        \param obj The object the functor should call on.
        \param func Pointer to the member function to be called.
    */
    template <class T>
    Functor(T* obj, void (T::*func)())
        : FunctorBase((void *)(ExecuteFunc) (Functor::template ExecuteFunction<T, void (T::*)()>), 
                  (void *) obj, &func, sizeof(func)) {}

    /*! \brief Executes the functor */
    void operator()() const
    {
        if (getExecuteFunc() != 0) //member function
            (*(ExecuteFunc) getExecuteFunc())(this);
        else if (getFunction() != 0) //regular function
            (*(FuncCallback) getFunction())();
    }

private:
    typedef void (*ExecuteFunc)(const Functor *);

    template <class T, class MemFunc>
    static void ExecuteFunction(const Functor *base)
    {
        T *object = (T *) base->getObject();
        MemFunc &memFunc(*(MemFunc*)(base->getMemberFunction()));
        (object->*memFunc)();
    }
};

/*! ************************************************************************/
/*! \class Functor1
    \brief A functor with one parameters.

    For more info, see \ref functorsExplained "Functors Explained".
*****************************************************************************/
template <typename P1>
class Functor1 : public FunctorBase
{
public: 
    /*! \brief The C function type for this functor */
    typedef void (BLAZESDK_CC *FuncCallback)(P1);
    
    /*! \brief Creates an empty, invalid functor. */
    Functor1() {}

    /*! \brief Creates a C-style functor. 
        \param function Function to call.
    */
    Functor1(FuncCallback function) : FunctorBase((void *) function) {}

    /*! \brief Creates a C++ member pointer functor. 
        \param obj The object the functor should call on.
        \param func Pointer to the member function to be called.
    */
    template <class T>
    Functor1(T* obj, void (T::*func)(P1))
        : FunctorBase((void *)(ExecuteFunc) (Functor1<P1>::template ExecuteFunction<T, void (T::*)(P1)>), 
                  (void *) obj, &func, sizeof(func)) {}

    /*! \brief Executes the functor */
    void operator()(P1 arg1) const
    {
        if (getExecuteFunc() != 0) //member function
            (*(ExecuteFunc) getExecuteFunc())(this, arg1);
        else if (getFunction() != 0) //regular function
            (*(FuncCallback) getFunction())(arg1);
    }

private:
    typedef void (*ExecuteFunc)(const Functor1<P1> *, P1);

    template <class T, class MemFunc>
    static void ExecuteFunction(const Functor1<P1> *base, P1 arg1)
    {
        T *object = (T *) base->getObject();
        MemFunc &memFunc(*(MemFunc*)(base->getMemberFunction()));
        (object->*memFunc)(arg1);
    }
};

/*! ************************************************************************/
/*! \class Functor2
    \brief A functor with two parameters.

    For more info, see \ref functorsExplained "Functors Explained".
*****************************************************************************/
template <typename P1, typename P2>
class Functor2 : public FunctorBase
{
public:
    /*! \brief The C function type for this functor */
    typedef void (BLAZESDK_CC *FuncCallback)(P1, P2);
    
    /*! \brief Creates an empty, invalid functor. */
    Functor2() {}
        
    /*! \brief Creates a C-style functor. 
        \param function Function to call.
    */
    Functor2(FuncCallback function) : FunctorBase((void *) function) {}

    /*! \brief Creates a C++ member pointer functor. 
        \param obj The object the functor should call on.
        \param func Pointer to the member function to be called.
    */
    template <class T>
    Functor2(T* obj, void (T::*func)(P1, P2))
        : FunctorBase((void *) (ExecuteFunc) Functor2<P1, P2>::template ExecuteFunction<T, void (T::*)(P1, P2)>, 
                  (void *) obj, &func, sizeof(func)) {}

    /*! \brief Executes the functor */
    void operator()(P1 arg1, P2 arg2) const
    {
        if (getExecuteFunc() != 0) //member function
            (*(ExecuteFunc) getExecuteFunc())(this, arg1, arg2);
        else if (getFunction() != 0) //regular function
            (*(FuncCallback) getFunction())(arg1, arg2);
    }

private:
    typedef void (*ExecuteFunc)(const Functor2<P1, P2> *, P1, P2);
    template <class T, class MemFunc>
    static void ExecuteFunction(const Functor2<P1, P2> *base, P1 arg1, P2 arg2)
    {
        T *object = (T *) base->getObject();
        MemFunc &memFunc(*(MemFunc*)(void *)(base->getMemberFunction()));
        (object->*memFunc)(arg1, arg2);
    }
};

/*! ************************************************************************/
/*! \class Functor3
    \brief A functor with three parameters.

    For more info, see \ref functorsExplained "Functors Explained".
*****************************************************************************/
template <typename P1, typename P2, typename P3>
class Functor3 : public FunctorBase
{
public:
    /*! \brief The C function type for this functor */
    typedef void (BLAZESDK_CC *FuncCallback)(P1, P2, P3);
    
    /*! \brief Creates an empty, invalid functor. */
    Functor3() {}   
    
    /*! \brief Creates a C-style functor. 
        \param function Function to call.
    */
    Functor3(FuncCallback function) : FunctorBase( (void *) function) {}

    /*! \brief Creates a C++ member pointer functor. 
        \param obj The object the functor should call on.
        \param func Pointer to the member function to be called.
    */
    template <class T>
    Functor3(T* obj, void (T::*func)(P1, P2, P3))
        : FunctorBase((void *) (ExecuteFunc) Functor3<P1, P2, P3>::template ExecuteFunction<T, void (T::*)(P1, P2, P3)>, 
                  (void *) obj, &func, sizeof(func)) {}

    /*! \brief Executes the functor */
    void operator()(P1 arg1, P2 arg2, P3 arg3) const
    {
        if (getExecuteFunc() != 0) //member function
            (*(ExecuteFunc) getExecuteFunc())(this, arg1, arg2, arg3);
        else if (getFunction() != 0) //regular function
            (*(FuncCallback) getFunction())(arg1, arg2, arg3);
    }

private:
    typedef void (*ExecuteFunc)(const Functor3<P1, P2, P3> *, P1, P2, P3);
    template <class T, class MemFunc>
    static void ExecuteFunction(const Functor3<P1, P2, P3> *base, P1 arg1, P2 arg2, P3 arg3)
    {
        T *object = (T *) base->getObject();
        MemFunc &memFunc(*(MemFunc*)(void *)(base->getMemberFunction()));
        (object->*memFunc)(arg1, arg2, arg3);
    }
};

/*! ************************************************************************/
/*! \class Functor4
    \brief A functor with four parameters.

    For more info, see \ref functorsExplained "Functors Explained".
*****************************************************************************/
template <typename P1, typename P2, typename P3, typename P4>
class Functor4 : public FunctorBase
{
public:
    /*! \brief The C function type for this functor */
    typedef void (BLAZESDK_CC *FuncCallback)(P1, P2, P3, P4);

    /*! \brief Creates an empty, invalid functor. */
    Functor4() {}

    
    /*! \brief Creates a C-style functor. 
        \param function Function to call.
    */
    Functor4(FuncCallback function) : FunctorBase((void *) function) {}

    /*! \brief Creates a C++ member pointer functor. 
        \param obj The object the functor should call on.
        \param func Pointer to the member function to be called.
    */
    template <class T>
    Functor4(T* obj, void (T::*func)(P1, P2, P3, P4))
        : FunctorBase((void *) (ExecuteFunc) Functor4<P1, P2, P3, P4>::template ExecuteFunction<T, void (T::*)(P1, P2, P3, P4)>, 
                  (void *) obj, &func, sizeof(func)) {}

    /*! \brief Executes the functor */
    void operator()(P1 arg1, P2 arg2, P3 arg3, P4 arg4) const
    {
        if (getExecuteFunc() != 0) //member function
            (*(ExecuteFunc) getExecuteFunc())(this, arg1, arg2, arg3, arg4);
        else if (getFunction() != 0) //regular function
            (*(FuncCallback) getFunction())(arg1, arg2, arg3, arg4);
    }

private:
    typedef void (*ExecuteFunc)(const Functor4<P1, P2, P3, P4> *, P1, P2, P3, P4);
    template <class T, class MemFunc>
    static void ExecuteFunction(const Functor4<P1, P2, P3, P4> *base, P1 arg1, P2 arg2, P3 arg3, P4 arg4)
    {
        T *object = (T *) base->getObject();
        MemFunc &memFunc(*(MemFunc*)(void *)(base->getMemberFunction()));
        (object->*memFunc)(arg1, arg2, arg3, arg4);
    }
};

/*! ************************************************************************/
/*! \class Functor5
    \brief A functor with five parameters.

    For more info, see \ref functorsExplained "Functors Explained".
*****************************************************************************/
template <typename P1, typename P2, typename P3, typename P4, typename P5>
class Functor5 : public FunctorBase
{
public:
    /*! \brief The C function type for this functor */
    typedef void (BLAZESDK_CC *FuncCallback)(P1, P2, P3, P4, P5);

    /*! \brief Creates an empty, invalid functor. */
    Functor5() {}
    
    /*! \brief Creates a C-style functor. 
        \param function Function to call.
    */
    Functor5(FuncCallback function) : FunctorBase((void *) function) {}

    /*! \brief Creates a C++ member pointer functor. 
        \param obj The object the functor should call on.
        \param func Pointer to the member function to be called.
    */
    template <class T>
    Functor5(T* obj, void (T::*func)(P1, P2, P3, P4, P5))
        : FunctorBase((void *) (ExecuteFunc) Functor5<P1, P2, P3, P4, P5>::template ExecuteFunction<T, void (T::*)(P1, P2, P3, P4, P5)>, 
                  (void *) obj, &func, sizeof(func)) {}

    /*! \brief Executes the functor */
    void operator()(P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5) const
    {
        if (getExecuteFunc() != 0) //member function
            (*(ExecuteFunc) getExecuteFunc())(this, arg1, arg2, arg3, arg4, arg5);
        else if (getFunction() != 0) //regular function
            (*(FuncCallback) getFunction())(arg1, arg2, arg3, arg4, arg5);
    }

private:
    typedef void (*ExecuteFunc)(const Functor5<P1, P2, P3, P4, P5> *, P1, P2, P3, P4, P5);
    template <class T, class MemFunc>
    static void ExecuteFunction(const Functor5<P1, P2, P3, P4, P5> *base, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5)
    {
        T *object = (T *) base->getObject();
        MemFunc &memFunc(*(MemFunc*)(void *)(base->getMemberFunction()));
        (object->*memFunc)(arg1, arg2, arg3, arg4, arg5);
    }
};
}       // namespace Blaze

#endif  // BLAZE_CALLBACK_H

/*! **********************************************************************************************/
/*!
    \file callback.h

    Functor callback for member function callbacks.  For more info, see 
    http://online.ea.com/confluence/display/blaze/How+to+Use+Functors+%28Functors+Explained%29

    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_CALLBACK_H
#define BLAZE_CALLBACK_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/debug.h"
#include <string.h>

namespace Blaze
{
/*! ************************************************************************/
/*! \class CallbackJob
    \brief Provides an execution by object interface for classes.

    Certain systems take CallbackJob objects that implement an execution
    method for a caller-defined object.   This is used to flesh out the limits
    of the Functor model.
*****************************************************************************/
class BLAZESDK_API CallbackJob
{
public:
    virtual ~CallbackJob() {}

    /*! \brief Executes the job.  
        
        Classes inheriting from CallbackJob must implement this method.  The system
        handling the job is responsible for calling this method.
    */
    virtual void execute() = 0;
};


/*! ************************************************************************/
/*! \class FunctorBase
    \brief The base class for all functor types. 

    For more info, see http://online.ea.com/confluence/display/blaze/How+to+Use+Functors+%28Functors+Explained%29

    \note This base class should only be used to store other functors.  To make
    any actual use of it, it must be casted to the appropriate type.
*****************************************************************************/
class BLAZESDK_API FunctorBase
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
        particular object.
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

        BlazeAssertMsg(memFuncSize <= sizeof(mMemberFunc), "ERR: MEMBER FUNCTION POINTER SIZE TOO LARGE!!!");

//GOS-24926: Functors copying is limited to 12 bytes max by the compiler, the rest of the 4 bytes will be padded with junk, 
//hence may affect the later functor comparison (currently investigating with MS on this)
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_WINDOWS)
        memcpy(mMemberFunc, memFunc, memFuncSize <= 12 ? memFuncSize : 12);
#else
        memcpy(mMemberFunc, memFunc, memFuncSize <= sizeof(mMemberFunc) ? memFuncSize : sizeof(mMemberFunc));
#endif
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

    For more info, see http://online.ea.com/confluence/display/blaze/How+to+Use+Functors+%28Functors+Explained%29
*****************************************************************************/
class BLAZESDK_API Functor : public FunctorBase
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

    /*! \brief Creates a C++ const member pointer functor. 
        \param obj The object the functor should call on.
        \param func Pointer to the const member function to be called.
    */
    template <class T>
    Functor(T* obj, void (T::*func)() const)
        : FunctorBase((void *)(ExecuteFunc) (Functor::template ExecuteFunction<T, void (T::*)() const>), 
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

    For more info, see http://online.ea.com/confluence/display/blaze/How+to+Use+Functors+%28Functors+Explained%29
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

    /*! \brief Creates a C++ const member pointer functor. 
        \param obj The object the functor should call on.
        \param func Pointer to the const member function to be called.
    */
    template <class T>
    Functor1(T* obj, void (T::*func)(P1) const)
        : FunctorBase((void *)(ExecuteFunc) (Functor1<P1>::template ExecuteFunction<T, void (T::*)(P1) const>), 
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

template <class T, typename P1>
Functor1<P1> MakeFunctor(T* obj, void (T::*func)(P1))
{
    return Functor1<P1>(obj, func);
}


/*! ************************************************************************/
/*! \class Functor2
    \brief A functor with two parameters.

    For more info, see http://online.ea.com/confluence/display/blaze/How+to+Use+Functors+%28Functors+Explained%29
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

    /*! \brief Creates a C++ const member pointer functor. 
        \param obj The object the functor should call on.
        \param func Pointer to the const member function to be called.
    */
    template <class T>
    Functor2(T* obj, void (T::*func)(P1, P2) const)
        : FunctorBase((void *) (ExecuteFunc) Functor2<P1, P2>::template ExecuteFunction<T, void (T::*)(P1, P2) const>, 
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

template <class T, typename P1, typename P2>
Functor2<P1, P2> MakeFunctor(T* obj, void (T::*func)(P1, P2))
{
    return Functor2<P1, P2>(obj, func);
}


/*! ************************************************************************/
/*! \class Functor3
    \brief A functor with three parameters.

    For more info, see http://online.ea.com/confluence/display/blaze/How+to+Use+Functors+%28Functors+Explained%29.
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

    /*! \brief Creates a C++ const member pointer functor. 
        \param obj The object the functor should call on.
        \param func Pointer to the const member function to be called.
    */
    template <class T>
    Functor3(T* obj, void (T::*func)(P1, P2, P3) const)
        : FunctorBase((void *) (ExecuteFunc) Functor3<P1, P2, P3>::template ExecuteFunction<T, void (T::*)(P1, P2, P3) const>, 
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

template <class T, typename P1, typename P2, typename P3>
Functor3<P1, P2, P3> MakeFunctor(T* obj, void (T::*func)(P1, P2, P3))
{
    return Functor3<P1, P2, P3>(obj, func);
}


/*! ************************************************************************/
/*! \class Functor4
    \brief A functor with four parameters.

    For more info, see http://online.ea.com/confluence/display/blaze/How+to+Use+Functors+%28Functors+Explained%29
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

    /*! \brief Creates a C++ const member pointer functor. 
        \param obj The object the functor should call on.
        \param func Pointer to the const member function to be called.
    */
    template <class T>
    Functor4(T* obj, void (T::*func)(P1, P2, P3, P4) const)
        : FunctorBase((void *) (ExecuteFunc) Functor4<P1, P2, P3, P4>::template ExecuteFunction<T, void (T::*)(P1, P2, P3, P4) const>, 
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

template <class T, typename P1, typename P2, typename P3, typename P4>
Functor4<P1, P2, P3, P4> MakeFunctor(T* obj, void (T::*func)(P1, P2, P3, P4))
{
    return Functor4<P1, P2, P3, P4>(obj, func);
}

/*! ************************************************************************/
/*! \class Functor5
    \brief A functor with five parameters.

    For more info, see http://online.ea.com/confluence/display/blaze/How+to+Use+Functors+%28Functors+Explained%29
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

    /*! \brief Creates a C++ const member pointer functor. 
        \param obj The object the functor should call on.
        \param func Pointer to the const member function to be called.
    */
    template <class T>
    Functor5(T* obj, void (T::*func)(P1, P2, P3, P4, P5) const)
        : FunctorBase((void *) (ExecuteFunc) Functor5<P1, P2, P3, P4, P5>::template ExecuteFunction<T, void (T::*)(P1, P2, P3, P4, P5) const>, 
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

template <class T, typename P1, typename P2, typename P3, typename P4, typename P5>
Functor5<P1, P2, P3, P4, P5> MakeFunctor(T* obj, void (T::*func)(P1, P2, P3, P4, P5))
{
    return Functor5<P1, P2, P3, P4, P5>(obj, func);
}


/*! ************************************************************************/
/*! \class Functor6
    \brief A functor with six parameters.

    For more info, see http://online.ea.com/confluence/display/blaze/How+to+Use+Functors+%28Functors+Explained%29
*****************************************************************************/
template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
class Functor6 : public FunctorBase
{
public:
    /*! \brief The C function type for this functor */
    typedef void (BLAZESDK_CC *FuncCallback)(P1, P2, P3, P4, P5, P6);

    /*! \brief Creates an empty, invalid functor. */
    Functor6() {}

    /*! \brief Creates a C-style functor. 
    \param function Function to call.
    */
    Functor6(FuncCallback function) : FunctorBase((void *) function) {}

    /*! \brief Creates a C++ member pointer functor. 
    \param obj The object the functor should call on.
    \param func Pointer to the member function to be called.
    */
    template <class T>
    Functor6(T* obj, void (T::*func)(P1, P2, P3, P4, P5, P6))
        : FunctorBase((void *) (ExecuteFunc) Functor6<P1, P2, P3, P4, P5, P6>::template ExecuteFunction<T, void (T::*)(P1, P2, P3, P4, P5, P6)>, 
        (void *) obj, &func, sizeof(func)) {}

    /*! \brief Creates a C++ const member pointer functor. 
    \param obj The object the functor should call on.
    \param func Pointer to the const member function to be called.
    */
    template <class T>
    Functor6(T* obj, void (T::*func)(P1, P2, P3, P4, P5, P6) const)
        : FunctorBase((void *) (ExecuteFunc) Functor6<P1, P2, P3, P4, P5, P6>::template ExecuteFunction<T, void (T::*)(P1, P2, P3, P4, P5, P6) const>, 
        (void *) obj, &func, sizeof(func)) {}

    /*! \brief Executes the functor */
    void operator()(P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, P6 arg6) const
    {
        if (getExecuteFunc() != 0) //member function
            (*(ExecuteFunc) getExecuteFunc())(this, arg1, arg2, arg3, arg4, arg5, arg6);
        else if (getFunction() != 0) //regular function
            (*(FuncCallback) getFunction())(arg1, arg2, arg3, arg4, arg5, arg6);
    }

private:
    typedef void (*ExecuteFunc)(const Functor6<P1, P2, P3, P4, P5, P6> *, P1, P2, P3, P4, P5, P6);
    template <class T, class MemFunc>
    static void ExecuteFunction(const Functor6<P1, P2, P3, P4, P5, P6> *base, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, P6 arg6)
    {
        T *object = (T *) base->getObject();
        MemFunc &memFunc(*(MemFunc*)(void *)(base->getMemberFunction()));
        (object->*memFunc)(arg1, arg2, arg3, arg4, arg5, arg6);
    }
};

template <class T, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
Functor6<P1, P2, P3, P4, P5, P6> MakeFunctor(T* obj, void (T::*func)(P1, P2, P3, P4, P5, P6))
{
    return Functor6<P1, P2, P3, P4, P5, P6>(obj, func);
}


/*! ************************************************************************/
/*! \class Functor7
    \brief A functor with seven parameters.

    For more info, see http://online.ea.com/confluence/display/blaze/How+to+Use+Functors+%28Functors+Explained%29
*****************************************************************************/
template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
class Functor7 : public FunctorBase
{
public:
    /*! \brief The C function type for this functor */
    typedef void (BLAZESDK_CC *FuncCallback)(P1, P2, P3, P4, P5, P6, P7);

    /*! \brief Creates an empty, invalid functor. */
    Functor7() {}

    /*! \brief Creates a C-style functor. 
    \param function Function to call.
    */
    Functor7(FuncCallback function) : FunctorBase((void *) function) {}

    /*! \brief Creates a C++ member pointer functor. 
    \param obj The object the functor should call on.
    \param func Pointer to the member function to be called.
    */
    template <class T>
    Functor7(T* obj, void (T::*func)(P1, P2, P3, P4, P5, P6, P7))
        : FunctorBase((void *) (ExecuteFunc) Functor7<P1, P2, P3, P4, P5, P6, P7>::template ExecuteFunction<T, void (T::*)(P1, P2, P3, P4, P5, P6, P7)>, 
        (void *) obj, &func, sizeof(func)) {}

    /*! \brief Creates a C++ const member pointer functor. 
    \param obj The object the functor should call on.
    \param func Pointer to the const member function to be called.
    */
    template <class T>
    Functor7(T* obj, void (T::*func)(P1, P2, P3, P4, P5, P6, P7) const)
        : FunctorBase((void *) (ExecuteFunc) Functor7<P1, P2, P3, P4, P5, P6, P7>::template ExecuteFunction<T, void (T::*)(P1, P2, P3, P4, P5, P6, P7) const>, 
        (void *) obj, &func, sizeof(func)) {}

    /*! \brief Executes the functor */
    void operator()(P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, P6 arg6, P7 arg7) const
    {
        if (getExecuteFunc() != 0) //member function
            (*(ExecuteFunc) getExecuteFunc())(this, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
        else if (getFunction() != 0) //regular function
            (*(FuncCallback) getFunction())(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }

private:
    typedef void (*ExecuteFunc)(const Functor7<P1, P2, P3, P4, P5, P6, P7> *, P1, P2, P3, P4, P5, P6, P7);
    template <class T, class MemFunc>
    static void ExecuteFunction(const Functor7<P1, P2, P3, P4, P5, P6, P7> *base, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, P6 arg6, P7 arg7)
    {
        T *object = (T *) base->getObject();
        MemFunc &memFunc(*(MemFunc*)(void *)(base->getMemberFunction()));
        (object->*memFunc)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }
};

template <class T, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
Functor7<P1, P2, P3, P4, P5, P6, P7> MakeFunctor(T* obj, void (T::*func)(P1, P2, P3, P4, P5, P6, P7))
{
    return Functor7<P1, P2, P3, P4, P5, P6, P7>(obj, func);
}

}       // namespace Blaze

#endif  // BLAZE_CALLBACK_H

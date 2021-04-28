/*! ************************************************************************************************/
/*!
    \file methodjob.h

    This file supplies a set of templated classes useful for scheduling functors and methods.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_METHOD_JOB_H
#define BLAZE_METHOD_JOB_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/job.h"

namespace Blaze
{
    class JobScheduler;

template <class T>
class MethodCallJob : public Job
{
public:
    typedef void (T::*MethodPtr)();
    void execute() {
        (mObj->*mFunc)();
    }

protected:
    friend class Blaze::JobScheduler;
    MethodCallJob(T* obj, MethodPtr func) : mObj(obj), mFunc(func) { mAssociatedTitleCbObject = obj; }

private:
    T* mObj;
    MethodPtr mFunc;   

    NON_COPYABLE(MethodCallJob);
};

template <class T>
class MethodCallJobConst : public Job
{
public:
    typedef void (T::*MethodPtr)() const;
    void execute() {
        (mObj->*mFunc)();
    }

protected:
    friend class Blaze::JobScheduler;
    MethodCallJobConst(const T* obj, MethodPtr func) : mObj(obj), mFunc(func) { mAssociatedTitleCbObject = obj; }

private:
    const T* mObj;
    MethodPtr mFunc;   

    NON_COPYABLE(MethodCallJobConst);
};

template <class T, typename P1>
class MethodCallJob1 : public Job
{
public:
    typedef void (T::*MethodPtr)(P1);
    void execute() {
        (mObj->*mFunc)(mArg1);
    }

protected:
    friend class Blaze::JobScheduler;
    MethodCallJob1(T* obj, MethodPtr func, P1 arg1): mObj(obj), mFunc(func), mArg1(arg1) { mAssociatedTitleCbObject = obj; }

private:
    T* mObj;
    MethodPtr mFunc;
    P1 mArg1;

    NON_COPYABLE(MethodCallJob1);
};

template <class T, typename P1>
class MethodCallJob1Const : public Job
{
public:
    typedef void (T::*MethodPtr)(P1) const;
    void execute() {
        (mObj->*mFunc)(mArg1);
    }

protected:
    friend class Blaze::JobScheduler;
    MethodCallJob1Const(const T* obj, MethodPtr func, P1 arg1): mObj(obj), mFunc(func), mArg1(arg1) { mAssociatedTitleCbObject = obj; }

private:
    const T* mObj;
    MethodPtr mFunc;
    P1 mArg1;

    NON_COPYABLE(MethodCallJob1Const);
};



template <class T, typename P1, typename P2>
class MethodCallJob2 : public Job
{
public:
    typedef void (T::*MethodPtr)(P1, P2);
    void execute() {
        (mObj->*mFunc)(mArg1, mArg2);
    }

protected:
    friend class Blaze::JobScheduler;
    MethodCallJob2(T* obj, MethodPtr func, P1 arg1, P2 arg2) :
    mObj(obj), mFunc(func), mArg1(arg1), mArg2(arg2) { mAssociatedTitleCbObject = obj; }

private:
    T* mObj;
    MethodPtr mFunc;
    P1 mArg1;
    P2 mArg2;
    
    NON_COPYABLE(MethodCallJob2);
};


template <class T, typename P1, typename P2>
class MethodCallJob2Const : public Job
{
public:
    typedef void (T::*MethodPtr)(P1, P2) const;
    void execute() {
        (mObj->*mFunc)(mArg1, mArg2);
    }

protected:
    friend class Blaze::JobScheduler;
    MethodCallJob2Const(const T* obj, MethodPtr func, P1 arg1, P2 arg2) :
    mObj(obj), mFunc(func), mArg1(arg1), mArg2(arg2) { mAssociatedTitleCbObject = obj; }

private:
    const T* mObj;
    MethodPtr mFunc;
    P1 mArg1;
    P2 mArg2;

    NON_COPYABLE(MethodCallJob2Const);
};




template <class T, typename P1, typename P2, typename P3>
class MethodCallJob3 : public Job
{
public:
    typedef void (T::*MethodPtr)(P1, P2, P3);
    void execute() {
        (mObj->*mFunc)(mArg1, mArg2, mArg3);
    }

protected:
    friend class Blaze::JobScheduler;
    MethodCallJob3(T* obj, MethodPtr func, P1 arg1, P2 arg2, P3 arg3) : 
    mObj(obj), mFunc(func), mArg1(arg1), mArg2(arg2), mArg3(arg3) { mAssociatedTitleCbObject = obj; }

private:
    T* mObj;
    MethodPtr mFunc;
    P1 mArg1;
    P2 mArg2;
    P3 mArg3;

    NON_COPYABLE(MethodCallJob3);
};

template <class T, typename P1, typename P2, typename P3>
class MethodCallJob3Const : public Job
{
public:
    typedef void (T::*MethodPtr)(P1, P2, P3) const;
    void execute() {
        (mObj->*mFunc)(mArg1, mArg2, mArg3);
    }

protected:
    friend class Blaze::JobScheduler;
    MethodCallJob3Const(const T* obj, MethodPtr func, P1 arg1, P2 arg2, P3 arg3) : 
    mObj(obj), mFunc(func), mArg1(arg1), mArg2(arg2), mArg3(arg3) { mAssociatedTitleCbObject = obj; }

private:
    const T* mObj;
    MethodPtr mFunc;
    P1 mArg1;
    P2 mArg2;
    P3 mArg3;

    NON_COPYABLE(MethodCallJob3Const);
};


} // namespace Blaze

#endif // BLAZE_METHOD_JOB_H

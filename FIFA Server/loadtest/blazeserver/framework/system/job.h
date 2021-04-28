/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_JOB_H
#define BLAZE_JOB_H

#include "fiber.h"
#include "framework/blazedefines.h"

#if defined(EA_PLATFORM_WIN64)
#pragma warning(disable: 4371) // workaround for known rare/undocumented microsoft warning compiling on 64 bit
#endif

//These macros help us write a compact definition for multiplexing a single type of call across a number of templated "call functions" which turn into 
//method call jobs and are used in a variety of ways in the selector.  Each method takes 0-N args, and will have 0 or more args before and 0 or more args after the calling
//params

#define COMMA_FUNC ,
#define NO_COMMA_FUNC


//NLIST is a recurisive macro that allows you to expand a macro essentially in a for loop, up to N times.  
//We have two methods, one which allows us to run a different macro for the first item in a list. 
//Each function macro takes an integer as its single argument.
#define NLIST_0(_FUNC_FIRST, _FUNC)
#define NLIST_1(_FUNC_FIRST, _FUNC) _FUNC_FIRST(1)
#define NLIST_2(_FUNC_FIRST, _FUNC) NLIST_1(_FUNC_FIRST, _FUNC) _FUNC(2)
#define NLIST_3(_FUNC_FIRST, _FUNC) NLIST_2(_FUNC_FIRST, _FUNC) _FUNC(3)
#define NLIST_4(_FUNC_FIRST, _FUNC) NLIST_3(_FUNC_FIRST, _FUNC) _FUNC(4)
#define NLIST_5(_FUNC_FIRST, _FUNC) NLIST_4(_FUNC_FIRST, _FUNC) _FUNC(5)
#define NLIST_6(_FUNC_FIRST, _FUNC) NLIST_5(_FUNC_FIRST, _FUNC) _FUNC(6)
#define NLIST_FIRST(_N, _FUNC_FIRST, _FUNC) NLIST_##_N(_FUNC_FIRST, _FUNC)
#define NLIST(_N, _FUNC) NLIST_FIRST(_N, _FUNC, _FUNC)

///TYPE PARAMS
//The Type params define a set of parameters for the start of our job creating method. 
//Each param is labeled P1...PN
//
//Ex: template <class Caller, typename P1, typename P2>
#define TYPE_PARAMS(_N) , typename P##_N
#define TYPE_PARAMS_NO_COMMA(_N) typename P##_N

#define METHOD_CALL_TYPE_PARAMS(_N) template <class Caller NLIST_FIRST(_N, TYPE_PARAMS, TYPE_PARAMS)>
#define METHOD_CALL_TYPE_PARAMS_RET(_N) template <class Caller, typename ReturnType NLIST_FIRST(_N, TYPE_PARAMS, TYPE_PARAMS)>
#define FUNC_CALL_TYPE_PARAMS(_N) template <NLIST_FIRST(_N, TYPE_PARAMS_NO_COMMA, TYPE_PARAMS)>
#define FUNC_CALL_TYPE_PARAMS_0

/// METHOD ARGUMENTS
//The Method arguments define the arguments passed into a job caller function.  There are
//actually two lists here - one is the list in the job function that is passed the arguments,
//the second is the job arguments themselves.
//
//
//Caller *caller, void (Caller::*mf)(P1, P2), P1 arg1, P2 arg2

//These methods define the arguments used in the job function.
#define FUNC_TYPE_DECL(_N), P##_N
#define FUNC_TYPE_DECL_NO_COMMA(_N) P##_N
#define FUNC_TYPE_DECL_LIST(_N) NLIST_FIRST(_N, FUNC_TYPE_DECL_NO_COMMA, FUNC_TYPE_DECL)
#define FUNC_TYPE_DECL_LIST_COMMA(_N) NLIST(_N, FUNC_TYPE_DECL)

//This is the inner macro used to call out the actual arguments.
#define FUNC_ARG_DECL(_N), P##_N arg##_N

#define METHOD_CALL_ARG_DECL(_N) Caller* caller, void (Caller::*mf)(FUNC_TYPE_DECL_LIST(_N)) NLIST(_N, FUNC_ARG_DECL)
#define METHOD_CALL_ARG_DECL_RET(_N) Caller* caller, ReturnType (Caller::*mf)(FUNC_TYPE_DECL_LIST(_N)) NLIST(_N, FUNC_ARG_DECL), ReturnType* ret = nullptr
#define CONST_METHOD_CALL_ARG_DECL(_N) Caller* caller, void (Caller::*mf)(FUNC_TYPE_DECL_LIST(_N)) const NLIST(_N, FUNC_ARG_DECL)
#define FUNC_CALL_ARG_DECL(_N) void (*mf)(FUNC_TYPE_DECL_LIST(_N)) NLIST(_N, FUNC_ARG_DECL)


///JOB DECLARATION
// This section defines the creation of the actual job object passed into the base function. The job object
// is templated by the same template arguments as the method in which its called, and its passed all the job
// arguments passed into the method.
//ex: BLAZE_NEW MethodCall2Job<Caller, P1, P2>(caller, mf, arg1, arg2)
#define ARG_PARAM(_N) , arg##_N
#define METHOD_JOB_DECL(_N,_JOB_PARENT) BLAZE_NEW MethodCall##_N##Job<Caller FUNC_TYPE_DECL_LIST_COMMA(_N), _JOB_PARENT >(caller, mf NLIST(_N, ARG_PARAM))
#define METHOD_JOB_DECL_RET(_N,_JOB_PARENT) BLAZE_NEW RetMethodCall##_N##Job<Caller, ReturnType FUNC_TYPE_DECL_LIST_COMMA(_N), _JOB_PARENT >(caller, mf NLIST(_N, ARG_PARAM), ret)
#define CONST_METHOD_JOB_DECL(_N,_JOB_PARENT) BLAZE_NEW ConstMethodCall##_N##Job<Caller FUNC_TYPE_DECL_LIST_COMMA(_N), _JOB_PARENT >(caller, mf NLIST(_N, ARG_PARAM))
#define FUNC_JOB_DECL(_N,_JOB_PARENT) BLAZE_NEW FunctionCall##_N##Job<FUNC_TYPE_DECL_LIST(_N), _JOB_PARENT >(mf NLIST(_N, ARG_PARAM))
#define FUNC_JOB_DECL_0(_JOB_PARENT) BLAZE_NEW FunctionCall0Job<_JOB_PARENT >(mf)

///PUTTING IT ALL TOGETHER
//The above macros are used in a "fill in the blank" method.  We define a function that would 
//take a number of template arguments, would create a job object using passed in methods and arguments
//and do something with the resulting job.  METHOD_JOB_FUNC_DECL is called with the macro that represents the template.
//The template is then called a number of times (up to our max # of arguments for a job) with the corresponding 
//macros that spell out the fill in the blank portions.

//Do "0" as a seperate one because of the way params are structured with regards to needing a comma
#define METHOD_JOB_FUNC_DECL_0(_FUNC_DECL, _JOB_PARENT) \
    _FUNC_DECL(METHOD_CALL_TYPE_PARAMS(0), METHOD_CALL_ARG_DECL(0), METHOD_JOB_DECL(0,_JOB_PARENT), caller) \
    _FUNC_DECL(METHOD_CALL_TYPE_PARAMS(0), CONST_METHOD_CALL_ARG_DECL(0), CONST_METHOD_JOB_DECL(0,_JOB_PARENT), caller) \
    _FUNC_DECL(FUNC_CALL_TYPE_PARAMS_0, FUNC_CALL_ARG_DECL(0), FUNC_JOB_DECL_0(_JOB_PARENT), nullptr)

#define METHOD_JOB_FUNC_DECL_N(_FUNC_DECL, _N, _JOB_PARENT) \
    _FUNC_DECL(METHOD_CALL_TYPE_PARAMS(_N), METHOD_CALL_ARG_DECL(_N), METHOD_JOB_DECL(_N,_JOB_PARENT), caller) \
    _FUNC_DECL(METHOD_CALL_TYPE_PARAMS(_N), CONST_METHOD_CALL_ARG_DECL(_N), CONST_METHOD_JOB_DECL(_N,_JOB_PARENT), caller) \
    _FUNC_DECL(FUNC_CALL_TYPE_PARAMS(_N), FUNC_CALL_ARG_DECL(_N), FUNC_JOB_DECL(_N,_JOB_PARENT), nullptr)

#define METHOD_JOB_FUNC_DECL_JOB_PARENT(_FUNC_DECL, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_0(_FUNC_DECL, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_N(_FUNC_DECL, 1, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_N(_FUNC_DECL, 2, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_N(_FUNC_DECL, 3, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_N(_FUNC_DECL, 4, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_N(_FUNC_DECL, 5, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_N(_FUNC_DECL, 6, _JOB_PARENT) 

#define METHOD_JOB_FUNC_DECL(_FUNC_DECL) METHOD_JOB_FUNC_DECL_JOB_PARENT(_FUNC_DECL, Job)

////Right now the return job variant is seperate because we don't have all the return classes defined, and 
//not every job execution method may be able to deal with return functions.

//Do "0" as a seperate one because of the way params are structured with regards to needing a comma
#define METHOD_JOB_FUNC_DECL_0_RET(_FUNC_DECL, _JOB_PARENT) \
    _FUNC_DECL(METHOD_CALL_TYPE_PARAMS_RET(0), METHOD_CALL_ARG_DECL_RET(0), METHOD_JOB_DECL_RET(0,_JOB_PARENT), caller) 

#define METHOD_JOB_FUNC_DECL_N_RET(_FUNC_DECL, _N, _JOB_PARENT) \
    _FUNC_DECL(METHOD_CALL_TYPE_PARAMS_RET(_N), METHOD_CALL_ARG_DECL_RET(_N), METHOD_JOB_DECL_RET(_N,_JOB_PARENT), caller) 

#define METHOD_JOB_FUNC_DECL_RET_JOB_PARENT(_FUNC_DECL, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_0_RET(_FUNC_DECL, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_N_RET(_FUNC_DECL, 1, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_N_RET(_FUNC_DECL, 2, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_N_RET(_FUNC_DECL, 3, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_N_RET(_FUNC_DECL, 4, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_N_RET(_FUNC_DECL, 5, _JOB_PARENT) \
    METHOD_JOB_FUNC_DECL_N_RET(_FUNC_DECL, 6, _JOB_PARENT) 

#define METHOD_JOB_FUNC_DECL_RET(_FUNC_DECL) METHOD_JOB_FUNC_DECL_RET_JOB_PARENT(_FUNC_DECL, Job)

namespace Blaze
{



class Selector;
class JobQueue;

class Job
{
public:
    Job() : mNext(nullptr), mPriority(Fiber::INVALID_PRIORITY), mNamedContext(nullptr) {}
    virtual ~Job() {};
    virtual void execute() = 0;
    
    Fiber::Priority getPriority() const { return mPriority; }
    void setPriority(Fiber::Priority priority) { mPriority = priority; }
    Job* getNextJob() const { return mNext; }
    const char *getNamedContext() const { return mNamedContext; }
    void setNamedContext(const char *namedContext) { mNamedContext = namedContext; }
    
protected:
    friend class JobQueue;
    friend class Selector;
    Job* mNext;
    Fiber::Priority mPriority;
    const char *mNamedContext;
};

//These classes perform a simple job task of calling a method on an object.  They take as part of the constructor
//an object to call on, a member function pointer (always typedef'd as MemberFunc), and a set of parameters to pass
//to the call.  Since all the parameters are set via copy, the function call's parameters must be copyable.  If they
//aren't, you need to write a custom job as you can't use these. 

/*! \brief A no-arg method calling job. */
template <class Caller, class ParentClass = Job>
class MethodCall0Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)();

    MethodCall0Job(Caller *caller, MemberFunc func) : 
        mCaller(caller), mFunc(func) {}

    void execute() override { (mCaller->*mFunc)(); }
private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(MethodCall0Job);
};

/*! \brief A no-arg method calling job. */
template <class ParentClass = Job>
class FunctionCall0Job : public ParentClass
{
public:
    typedef void (*Func)();

    FunctionCall0Job(Func func) : 
    mFunc(func) {}

    void execute() override { (*mFunc)(); }
private:
    Func mFunc;

    NON_COPYABLE(FunctionCall0Job);
};


//deprecated name
#define MethodCallJob MethodCall0Job

/*! \brief A no-arg method calling const job. */
template <class Caller, class ParentClass = Job>
class ConstMethodCall0Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)() const;

    ConstMethodCall0Job(Caller *caller, MemberFunc func) : 
    mCaller(caller), mFunc(func) {}

    virtual void execute() { (mCaller->*mFunc)(); }
private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(ConstMethodCall0Job);
};

#define ConstMethodCallJob ConstMethodCall0Job

/*! \brief A no-arg method calling job with a return. */
template <class Caller, typename ReturnType, class ParentClass = Job>
class RetMethodCall0Job : public ParentClass
{
public:
    typedef ReturnType (Caller::*MemberFunc)();

    RetMethodCall0Job(Caller *caller, MemberFunc func, ReturnType *ret = nullptr) : 
        mCaller(caller), mFunc(func), mReturnValue(ret) {}

    void execute() override { ReturnType ret = (mCaller->*mFunc)(); if (mReturnValue != nullptr) *mReturnValue = ret;}
private:
    Caller *mCaller;
    MemberFunc mFunc;
    ReturnType *mReturnValue;

    NON_COPYABLE(RetMethodCall0Job);
};

#define RetMethodCallJob RetMethodCall0Job



/*! \brief A one arg method calling job. */
template <class Caller, typename P1, class ParentClass = Job>
class MethodCall1Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)(P1);

    MethodCall1Job(Caller *caller, MemberFunc func, P1 arg1) : 
        mArg1(arg1), mCaller(caller), mFunc(func)  {}

    void execute() override { (mCaller->*mFunc)(mArg1); }

    P1 mArg1;
private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(MethodCall1Job);
};

/*! \brief A one arg method calling const job. */
template <class Caller, typename P1, class ParentClass = Job>
class ConstMethodCall1Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)(P1) const;

    ConstMethodCall1Job(Caller *caller, MemberFunc func, P1 arg1) : 
    mArg1(arg1), mCaller(caller), mFunc(func)  {}

    virtual void execute() { (mCaller->*mFunc)(mArg1); }

    P1 mArg1;
private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(ConstMethodCall1Job);
};

/*! \brief A one arg method calling job with a return. */
template <class Caller, typename ReturnType, typename P1, class ParentClass = Job>
class RetMethodCall1Job : public ParentClass
{
public:
    typedef ReturnType (Caller::*MemberFunc)(P1);

    RetMethodCall1Job(Caller *caller, MemberFunc func, P1 arg1, ReturnType *ret = nullptr) : 
    mArg1(arg1), mCaller(caller), mFunc(func), mReturnValue(ret)  {}

    void execute() override { ReturnType ret = (mCaller->*mFunc)(mArg1); if (mReturnValue != nullptr) *mReturnValue = ret; }

    P1 mArg1;
private:
    Caller *mCaller;
    MemberFunc mFunc;
    ReturnType *mReturnValue;

    NON_COPYABLE(RetMethodCall1Job);
};


/*! \brief A one arg function calling job. */
template <typename P1, class ParentClass = Job>
class FunctionCall1Job : public ParentClass
{
public:
    typedef void (*Func)(P1);

    FunctionCall1Job(Func func, P1 arg1) : 
    mArg1(arg1), mFunc(func)  {}

    void execute() override { (*mFunc)(mArg1); }

    P1 mArg1;
private:
    Func mFunc;

    NON_COPYABLE(FunctionCall1Job);
};

/*! \brief A two arg method calling job. */
template <class Caller, typename P1, typename P2, class ParentClass = Job>
class MethodCall2Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)(P1, P2);

    MethodCall2Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2) : 
        mArg1(arg1), mArg2(arg2), mCaller(caller), mFunc(func) {}

    void execute() override { (mCaller->*mFunc)(mArg1, mArg2); }

    P1 mArg1;
    P2 mArg2;
private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(MethodCall2Job);
};

/*! \brief A two arg method calling const job. */
template <class Caller, typename P1, typename P2, class ParentClass = Job>
class ConstMethodCall2Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)(P1, P2) const;

    ConstMethodCall2Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2) : 
    mArg1(arg1), mArg2(arg2), mCaller(caller), mFunc(func) {}

    virtual void execute() { (mCaller->*mFunc)(mArg1, mArg2); }

    P1 mArg1;
    P2 mArg2;
private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(ConstMethodCall2Job);
};

/*! \brief A two arg method calling job with a return.*/
template <class Caller, typename ReturnType, typename P1, typename P2, class ParentClass = Job>
class RetMethodCall2Job : public ParentClass
{
public:
    typedef ReturnType (Caller::*MemberFunc)(P1, P2);

    RetMethodCall2Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2, ReturnType *ret = nullptr) : 
    mArg1(arg1), mArg2(arg2), mCaller(caller), mFunc(func), mReturnValue(ret) {}

    virtual void execute() { ReturnType ret = (mCaller->*mFunc)(mArg1, mArg2); if (mReturnValue != nullptr) *mReturnValue = ret; }

    P1 mArg1;
    P2 mArg2;
private:
    Caller *mCaller;
    MemberFunc mFunc;
    ReturnType *mReturnValue;

    NON_COPYABLE(RetMethodCall2Job);
};

/*! \brief A two arg function calling job. */
template <typename P1, typename P2, class ParentClass = Job>
class FunctionCall2Job : public ParentClass
{
public:
    typedef void (*Func)(P1, P2);

    FunctionCall2Job(Func func, P1 arg1, P2 arg2) : 
    mArg1(arg1), mArg2(arg2), mFunc(func)  {}

    void execute() override { (*mFunc)(mArg1, mArg2); }

    P1 mArg1;
    P2 mArg2;
private:
    Func mFunc;

    NON_COPYABLE(FunctionCall2Job);
};




/*! \brief A three arg method calling job. */
template <class Caller, typename P1, typename P2, typename P3, class ParentClass = Job>
class MethodCall3Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)(P1, P2, P3);

    MethodCall3Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2, P3 arg3) : 
        mArg1(arg1), mArg2(arg2),
        mArg3(arg3), mCaller(caller), mFunc(func) {}

    void execute() override { (mCaller->*mFunc)(mArg1, mArg2, mArg3); }

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(MethodCall3Job);
};

/*! \brief A three arg method calling const job. */
template <class Caller, typename P1, typename P2, typename P3, class ParentClass = Job>
class ConstMethodCall3Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)(P1, P2, P3) const;

    ConstMethodCall3Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2, P3 arg3) : 
    mArg1(arg1), mArg2(arg2),
        mArg3(arg3), mCaller(caller), mFunc(func) {}

    void execute() override { (mCaller->*mFunc)(mArg1, mArg2, mArg3); }

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(ConstMethodCall3Job);
};

/*! \brief A three arg method calling job with a return. */
template <class Caller, typename ReturnType, typename P1, typename P2, typename P3, class ParentClass = Job>
class RetMethodCall3Job : public ParentClass
{
public:
    typedef ReturnType (Caller::*MemberFunc)(P1, P2, P3);

    RetMethodCall3Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2, P3 arg3, ReturnType *ret = nullptr) : 
    mArg1(arg1), mArg2(arg2),
        mArg3(arg3), mCaller(caller), mFunc(func), mReturnValue(ret) {}

    void execute() override { ReturnType ret = (mCaller->*mFunc)(mArg1, mArg2, mArg3); if (mReturnValue != nullptr) *mReturnValue = ret;}

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
private:
    Caller *mCaller;
    MemberFunc mFunc;
    ReturnType *mReturnValue;

    NON_COPYABLE(RetMethodCall3Job);
};


template <typename P1, typename P2, typename P3, class ParentClass = Job>
class FunctionCall3Job : public ParentClass
{
public:
    typedef void (*Func)(P1, P2, P3);

    FunctionCall3Job(Func func, P1 arg1, P2 arg2, P3 arg3) : 
    mArg1(arg1), mArg2(arg2), mArg3(arg3), mFunc(func)  {}

    virtual void execute() { (*mFunc)(mArg1, mArg2, mArg3); }

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
private:
    Func mFunc;

    NON_COPYABLE(FunctionCall3Job);
};


/*! \brief A four arg method calling job. */
template <class Caller, typename P1, typename P2, typename P3, typename P4, class ParentClass = Job>
class MethodCall4Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)(P1, P2, P3, P4);

    MethodCall4Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2, P3 arg3, P4 arg4) : 
        mArg1(arg1), mArg2(arg2), mArg3(arg3), mArg4(arg4),
        mCaller(caller), mFunc(func) {}

    void execute() override { (mCaller->*mFunc)(mArg1, mArg2, mArg3, mArg4); }

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
    P4 mArg4;

private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(MethodCall4Job);
};

/*! \brief A four arg method calling const job. */
template <class Caller, typename P1, typename P2, typename P3, typename P4, class ParentClass = Job>
class ConstMethodCall4Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)(P1, P2, P3, P4) const;

    ConstMethodCall4Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2, P3 arg3, P4 arg4) : 
    mArg1(arg1), mArg2(arg2), mArg3(arg3), mArg4(arg4),
        mCaller(caller), mFunc(func) {}

    virtual void execute() { (mCaller->*mFunc)(mArg1, mArg2, mArg3, mArg4); }

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
    P4 mArg4;

private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(ConstMethodCall4Job);
};

/*! \brief A four arg method calling job with a return. */
template <class Caller, typename ReturnType, typename P1, typename P2, typename P3, typename P4, class ParentClass = Job>
class RetMethodCall4Job : public ParentClass
{
public:
    typedef ReturnType (Caller::*MemberFunc)(P1, P2, P3, P4);

    RetMethodCall4Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2, P3 arg3, P4 arg4, ReturnType *ret = nullptr) : 
    mArg1(arg1), mArg2(arg2), mArg3(arg3), mArg4(arg4),
        mCaller(caller), mFunc(func), mReturnValue(ret) {}

    virtual void execute() { ReturnType ret = (mCaller->*mFunc)(mArg1, mArg2, mArg3, mArg4); if (mReturnValue != nullptr) *mReturnValue = ret;}

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
    P4 mArg4;

private:
    Caller *mCaller;
    MemberFunc mFunc;
    ReturnType *mReturnValue;

    NON_COPYABLE(RetMethodCall4Job);
};

template <typename P1, typename P2, typename P3, typename P4, class ParentClass = Job>
class FunctionCall4Job : public ParentClass
{
public:
    typedef void (*Func)(P1, P2, P3, P4);

    FunctionCall4Job(Func func, P1 arg1, P2 arg2, P3 arg3, P4 arg4) : 
    mArg1(arg1), mArg2(arg2), mArg3(arg3), mArg4(arg4), mFunc(func)  {}

    virtual void execute() { (*mFunc)(mArg1, mArg2, mArg3, mArg4); }

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
    P4 mArg4;
private:
    Func mFunc;

    NON_COPYABLE(FunctionCall4Job);
};



/*! \brief A five arg method calling job. */
template <class Caller, typename P1, typename P2, typename P3, typename P4, typename P5, class ParentClass = Job>
class MethodCall5Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)(P1, P2, P3, P4, P5);

    MethodCall5Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5) : 
        mArg1(arg1), mArg2(arg2), mArg3(arg3), mArg4(arg4), mArg5(arg5), 
        mCaller(caller), mFunc(func) {}

    void execute() override { (mCaller->*mFunc)(mArg1, mArg2, mArg3, mArg4, mArg5); }

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
    P4 mArg4;
    P5 mArg5;
private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(MethodCall5Job);
};

/*! \brief A five arg method calling const job. */
template <class Caller, typename P1, typename P2, typename P3, typename P4, typename P5, class ParentClass = Job>
class ConstMethodCall5Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)(P1, P2, P3, P4, P5) const;

    ConstMethodCall5Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5) : 
    mArg1(arg1), mArg2(arg2), mArg3(arg3), mArg4(arg4), mArg5(arg5), 
        mCaller(caller), mFunc(func) {}

    virtual void execute() { (mCaller->*mFunc)(mArg1, mArg2, mArg3, mArg4, mArg5); }

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
    P4 mArg4;
    P5 mArg5;
private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(ConstMethodCall5Job);
};

/*! \brief A five arg method calling job with a return.. */
template <class Caller, typename ReturnType, typename P1, typename P2, typename P3, typename P4, typename P5, class ParentClass = Job>
class RetMethodCall5Job : public ParentClass
{
public:
    typedef ReturnType (Caller::*MemberFunc)(P1, P2, P3, P4, P5);

    RetMethodCall5Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, ReturnType *ret = nullptr) : 
    mArg1(arg1), mArg2(arg2), mArg3(arg3), mArg4(arg4), mArg5(arg5), 
        mCaller(caller), mFunc(func), mReturnValue(ret) {}

    virtual void execute() { ReturnType ret = (mCaller->*mFunc)(mArg1, mArg2, mArg3, mArg4, mArg5); if (mReturnValue != nullptr) *mReturnValue = ret;}

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
    P4 mArg4;
    P5 mArg5;
private:
    Caller *mCaller;
    MemberFunc mFunc;
    ReturnType *mReturnValue;

    NON_COPYABLE(RetMethodCall5Job);
};


template <typename P1, typename P2, typename P3, typename P4, typename P5, class ParentClass = Job>
class FunctionCall5Job : public ParentClass
{
public:
    typedef void (*Func)(P1, P2, P3, P4, P5);

    FunctionCall5Job(Func func, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5) : 
    mArg1(arg1), mArg2(arg2), mArg3(arg3), mArg4(arg4), mArg5(arg5), mFunc(func)  {}

    virtual void execute() { (*mFunc)(mArg1, mArg2, mArg3, mArg4, mArg5); }

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
    P4 mArg4;
    P5 mArg5;
private:
    Func mFunc;

    NON_COPYABLE(FunctionCall5Job);
};
    

/*! \brief A six arg method calling job. */
template <class Caller, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, class ParentClass = Job>
class MethodCall6Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)(P1, P2, P3, P4, P5, P6);

    MethodCall6Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, P6 arg6) : 
    mArg1(arg1), mArg2(arg2), mArg3(arg3), mArg4(arg4), mArg5(arg5), mArg6(arg6), mCaller(caller), mFunc(func) {}

    void execute() override { (mCaller->*mFunc)(mArg1, mArg2, mArg3, mArg4, mArg5, mArg6); }

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
    P4 mArg4;
    P5 mArg5;
    P6 mArg6;
private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(MethodCall6Job);
};

/*! \brief A six arg method calling const job. */
template <class Caller, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, class ParentClass = Job>
class ConstMethodCall6Job : public ParentClass
{
public:
    typedef void (Caller::*MemberFunc)(P1, P2, P3, P4, P5, P6) const;

    ConstMethodCall6Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, P6 arg6) : 
    mArg1(arg1), mArg2(arg2), mArg3(arg3), mArg4(arg4), mArg5(arg5), mArg6(arg6), mCaller(caller), mFunc(func) {}

    virtual void execute() { (mCaller->*mFunc)(mArg1, mArg2, mArg3, mArg4, mArg5, mArg6); }

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
    P4 mArg4;
    P5 mArg5;
    P6 mArg6;
private:
    Caller *mCaller;
    MemberFunc mFunc;

    NON_COPYABLE(ConstMethodCall6Job);
};

/*! \brief A six arg method calling job with a return. */
template <class Caller, typename ReturnType, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, class ParentClass = Job>
class RetMethodCall6Job : public ParentClass
{
public:
    typedef ReturnType (Caller::*MemberFunc)(P1, P2, P3, P4, P5, P6);

    RetMethodCall6Job(Caller *caller, MemberFunc func, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, P6 arg6, ReturnType *ret = nullptr) : 
    mArg1(arg1), mArg2(arg2), mArg3(arg3), mArg4(arg4), mArg5(arg5), mArg6(arg6), mCaller(caller), mFunc(func), mReturnValue(ret) {}

    virtual void execute() { ReturnType ret = (mCaller->*mFunc)(mArg1, mArg2, mArg3, mArg4, mArg5, mArg6); if (mReturnValue != nullptr) *mReturnValue = ret;}

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
    P4 mArg4;
    P5 mArg5;
    P6 mArg6;
private:
    Caller *mCaller;
    MemberFunc mFunc;
    ReturnType *mReturnValue;

    NON_COPYABLE(RetMethodCall6Job);
};


template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, class ParentClass = Job>
class FunctionCall6Job : public ParentClass
{
public:
    typedef void (*Func)(P1, P2, P3, P4, P5, P6);

    FunctionCall6Job(Func func, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, P6 arg6) : 
    mArg1(arg1), mArg2(arg2), mArg3(arg3), mArg4(arg4), mArg5(arg5), mArg6(arg6), mFunc(func)  {}

    virtual void execute() { (*mFunc)(mArg1, mArg2, mArg3, mArg4, mArg5, mArg6); }

    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
    P4 mArg4;
    P5 mArg5;
    P6 mArg6;
private:
    Func mFunc;

    NON_COPYABLE(FunctionCall6Job);
};

} // Blaze

#endif // BLAZE_JOB_H


/*! **********************************************************************************************/
/*!
    \file

    Jobs for the RPC system.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_RPC_JOB_H
#define BLAZE_RPC_JOB_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/job.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/allocdefines.h"

namespace EA
{
namespace TDF
{
class Tdf;
} // namespace TDF
} // namespace EA

namespace Blaze
{

class TdfDecoder;
class RawBuffer;
class ComponentManager;

/*! ***************************************************************/
/*! \class RpcJobBase

    \brief The abstract base class for all jobs.
*/
class BLAZESDK_API RpcJobBase : public Job
{
public:

    /*! ***************************************************************************/
    /*!    \brief If we are allowed to execute, we either timed out or were scheduled for error.
    *******************************************************************************/
    virtual void execute();

    /* called when the user cancels */
    virtual void cancel(BlazeError err)
    {
        // If client had provided us with a TDF to fill in, it was saved in
        // mClientProvidedReply. We now need to return it to the client in the cancel notification.
        doCallback(mClientProvidedReply, nullptr, err);
    }

    virtual uint32_t getUpdatedExpiryTime();

    

    /*! ***************************************************************************/
    /*!    \brief Handles any incoming response.

        \param err Any error code associated with the response.
        \param decoder The decoder setup for decoding the response (if any).
        \param buf The rawbuffer used for decoding the response (if any).
    *******************************************************************************/
    void handleReply(BlazeError err, TdfDecoder& decoder, RawBuffer& buf);

    uint16_t getComponentId() const { return mComponentId; }
    uint16_t getCommandId() const { return mCommandId; }

    void setCallbackErr(BlazeError err) { mCallbackErr = err; }
    BlazeError getCallbackErr() const { return mCallbackErr; }

    const EA::TDF::Tdf* getReply() const { return mReply; }
    const EA::TDF::Tdf* getClientProvidedReply() const { return mClientProvidedReply; }

    /*! ***************************************************************************/
    /*!    \brief Constructor

        The callback error is initialized to timeout.  If the job is executed without
        any reply being handled, it will be assumed to be timed out.  Alternately
        another error code might be set in its place.
    *******************************************************************************/
    RpcJobBase(uint16_t componentId, uint16_t commandId, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager);

    virtual ~RpcJobBase();

protected:
    template <typename TdfType>
    static EA::TDF::Tdf *createTdf(MemoryGroupId& memoryGroup) { memoryGroup = MEM_GROUP_FRAMEWORK; return BLAZE_NEW(memoryGroup, "IncomingTDF") TdfType; }

private:
    virtual void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err) = 0;
    virtual EA::TDF::Tdf *createResponseTdf(MemoryGroupId& memoryGroup) = 0;
    virtual EA::TDF::Tdf *createErrorTdf(MemoryGroupId& memoryGroup) = 0;

    uint16_t mComponentId;
    uint16_t mCommandId;
    size_t mDataSize;
    BlazeError mCallbackErr;
    EA::TDF::TdfPtr mReply;
    MemoryGroupId mReplyMemoryGroupId;
    EA::TDF::Tdf *mClientProvidedReply;
    ComponentManager* mComponentManager;

};

template <>
inline EA::TDF::Tdf *RpcJobBase::createTdf<void>(MemoryGroupId& memoryGroup) { return nullptr; }

/*! ***************************************************************/
/*! \class RpcJobBaseTemplate

    \brief An intermediary helper class to simplify the final job templates.
*/
template <class ResponseType, class ErrorType>
class RpcJobBaseTemplate : public RpcJobBase
{
public:
    RpcJobBaseTemplate<ResponseType, ErrorType>(uint16_t componentId, uint16_t commandId, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
      RpcJobBase(componentId, commandId, clientProvidedResponse, manager) {}

private:

    EA::TDF::Tdf *createResponseTdf(MemoryGroupId& memoryGroup) { return createTdf<ResponseType>(memoryGroup); }
    EA::TDF::Tdf *createErrorTdf(MemoryGroupId& memoryGroup) { return createTdf<ErrorType>(memoryGroup); }
};


/*! ***************************************************************/
/*! RPC Job classes - the following is a set of 4 classes for each
    version of the RPC Job class.  For each parameter count, we need
    a version that has both error & response, error or response, or
    neither.
*/
/*! ***************************************************************/

/*! ***************************************************************/
/*! \class RpcJob

    \brief An RPC job with no extra parameters.
*/
template <typename ResponseType, typename ErrorType>
class RpcJob : public RpcJobBaseTemplate<ResponseType, ErrorType>
{
public:
    typedef Functor4<const ResponseType *, const ErrorType *, BlazeError, JobId> Callback;
    RpcJob(uint16_t componentId, uint16_t commandId, Callback cb, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<ResponseType, ErrorType>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb) { this->setAssociatedObject(cb.getObject()); }  // Note: We do not use setAssociatedTitleCb() here because the RPC callback (cb) is owned by Blaze, not the title code. 

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(static_cast<const ResponseType *>(response), static_cast<const ErrorType *>(errorResponse), err, Job::getId());
    }
    Callback mCb;
};


template <typename ResponseType>
class RpcJob<ResponseType, void> : public RpcJobBaseTemplate<ResponseType, void>
{
public:
    typedef Functor3<const ResponseType *, BlazeError, JobId> Callback;
    RpcJob(uint16_t componentId, uint16_t commandId, Callback cb, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<ResponseType, void>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(static_cast<const ResponseType *>(response), err, Job::getId());
    }
    Callback mCb;
};


template <typename ErrorType>
class RpcJob<void, ErrorType> : public RpcJobBaseTemplate<void, ErrorType>
{
public:
    typedef Functor3<const ErrorType *, BlazeError, JobId> Callback;
    RpcJob(uint16_t componentId, uint16_t commandId, Callback cb, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<void, ErrorType>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(static_cast<const ErrorType *>(errorResponse), err, Job::getId());
    }
    Callback mCb;
};

template <>
class RpcJob<void, void> : public RpcJobBaseTemplate<void, void>
{
public:
    typedef Functor2<BlazeError, JobId> Callback;
    RpcJob(uint16_t componentId, uint16_t commandId, Callback cb, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<void, void>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(err, Job::getId());
    }
    Callback mCb;
};

/*! ***************************************************************/
/*! \class RpcJob1

    \brief An RPC job with one pass-through parameter
*/
template <typename ResponseType, typename ErrorType, typename P1>
class RpcJob1 : public RpcJobBaseTemplate<ResponseType, ErrorType>
{
public:
    typedef Functor5<const ResponseType *, const ErrorType *, BlazeError, JobId, P1> Callback;
    RpcJob1(uint16_t componentId, uint16_t commandId, Callback cb, P1 arg1, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<ResponseType, ErrorType>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb), mArg1(arg1) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(static_cast<const ResponseType *>(response), static_cast<const ErrorType *>(errorResponse), err, Job::getId(), mArg1);
    }
    Callback mCb;
    P1 mArg1;
};


template <typename ResponseType, typename P1>
class RpcJob1<ResponseType, void, P1> : public RpcJobBaseTemplate<ResponseType, void>
{
public:
    typedef Functor4<const ResponseType *, BlazeError, JobId, P1> Callback;
    RpcJob1(uint16_t componentId, uint16_t commandId, Callback cb, P1 arg1, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<ResponseType, void>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb), mArg1(arg1) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(static_cast<const ResponseType *>(response), err, Job::getId(), mArg1);
    }
    Callback mCb;
    P1 mArg1;
};


template <typename ErrorType, typename P1>
class RpcJob1<void, ErrorType, P1> : public RpcJobBaseTemplate<void, ErrorType>
{
public:
    typedef Functor4<const ErrorType *, BlazeError, JobId, P1> Callback;
    RpcJob1(uint16_t componentId, uint16_t commandId, Callback cb, P1 arg1, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<void, ErrorType>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb), mArg1(arg1) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(static_cast<const ErrorType *>(errorResponse), err, Job::getId(), mArg1);
    }
    Callback mCb;
    P1 mArg1;
};

template <typename P1>
class RpcJob1<void, void, P1> : public RpcJobBaseTemplate<void, void>
{
public:
    typedef Functor3<BlazeError, JobId, P1> Callback;
    RpcJob1(uint16_t componentId, uint16_t commandId, Callback cb, P1 arg1, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<void, void>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb), mArg1(arg1) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(err, Job::getId(), mArg1);
    }
    Callback mCb;
    P1 mArg1;
};


/*! ***************************************************************/
/*! \class RpcJob2

    \brief An RPC job with two pass-through parameters
*/
template <typename ResponseType, typename ErrorType, typename P1, typename P2>
class RpcJob2 : public RpcJobBaseTemplate<ResponseType, ErrorType>
{
public:
    typedef Functor6<const ResponseType *, const ErrorType *, BlazeError, JobId, P1, P2> Callback;
    RpcJob2(uint16_t componentId, uint16_t commandId, Callback cb, P1 arg1, P2 arg2, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<ResponseType, ErrorType>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb), mArg1(arg1), mArg2(arg2) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(static_cast<const ResponseType *>(response), static_cast<const ErrorType *>(errorResponse), err, Job::getId(), mArg1, mArg2);
    }
    Callback mCb;
    P1 mArg1;
    P2 mArg2;
};


template <typename ResponseType, typename P1, typename P2>
class RpcJob2<ResponseType, void, P1, P2> : public RpcJobBaseTemplate<ResponseType, void>
{
public:
    typedef Functor5<const ResponseType *, BlazeError, JobId, P1, P2> Callback;
    RpcJob2(uint16_t componentId, uint16_t commandId, Callback cb, P1 arg1, P2 arg2, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<ResponseType, void>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb), mArg1(arg1), mArg2(arg2) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(static_cast<const ResponseType *>(response), err, Job::getId(), mArg1, mArg2);
    }
    Callback mCb;
    P1 mArg1;
    P2 mArg2;
};


template <typename ErrorType, typename P1, typename P2>
class RpcJob2<void, ErrorType, P1, P2> : public RpcJobBaseTemplate<void, ErrorType>
{
public:
    typedef Functor5<const ErrorType *, BlazeError, JobId, P1, P2> Callback;
    RpcJob2(uint16_t componentId, uint16_t commandId, Callback cb, P1 arg1, P2 arg2, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<void, ErrorType>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb), mArg1(arg1), mArg2(arg2) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(static_cast<const ErrorType *>(errorResponse), err, Job::getId(), mArg1, mArg2);
    }
    Callback mCb;
    P1 mArg1;
    P2 mArg2;
};

template <typename P1, typename P2>
class RpcJob2<void, void, P1, P2> : public RpcJobBaseTemplate<void, void>
{
public:
    typedef Functor4<BlazeError, JobId, P1, P2> Callback;
    RpcJob2(uint16_t componentId, uint16_t commandId, Callback cb, P1 arg1, P2 arg2, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<void, void>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb), mArg1(arg1), mArg2(arg2) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(err, Job::getId(), mArg1, mArg2);
    }
    Callback mCb;
    P1 mArg1;
    P2 mArg2;
};


/*! ***************************************************************/
/*! \class RpcJob3

    \brief An RPC job with three pass-through parameters
*/
template <typename ResponseType, typename ErrorType, typename P1, typename P2, typename P3>
class RpcJob3 : public RpcJobBaseTemplate<ResponseType, ErrorType>
{
public:
    typedef Functor7<const ResponseType *, const ErrorType *, BlazeError, JobId, P1, P2, P3> Callback;
    RpcJob3(uint16_t componentId, uint16_t commandId, Callback cb, P1 arg1, P2 arg2, P3 arg3, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<ResponseType, ErrorType>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb), mArg1(arg1), mArg2(arg2), mArg3(arg3) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(static_cast<const ResponseType *>(response), static_cast<const ErrorType *>(errorResponse), err,
            Job::getId(), mArg1, mArg2, mArg3);
    }
    Callback mCb;
    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
};


template <typename ResponseType, typename P1, typename P2, typename P3>
class RpcJob3<ResponseType, void, P1, P2, P3> : public RpcJobBaseTemplate<ResponseType, void>
{
public:
    typedef Functor6<const ResponseType *, BlazeError, JobId, P1, P2, P3> Callback;
    RpcJob3(uint16_t componentId, uint16_t commandId, Callback cb, P1 arg1, P2 arg2, P3 arg3, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<ResponseType, void>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb), mArg1(arg1), mArg2(arg2), mArg3(arg3) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(static_cast<const ResponseType *>(response), err, Job::getId(), mArg1, mArg2, mArg3);
    }
    Callback mCb;
    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
};


template <typename ErrorType, typename P1, typename P2, typename P3>
class RpcJob3<void, ErrorType, P1, P2, P3> : public RpcJobBaseTemplate<void, ErrorType>
{
public:
    typedef Functor6<const ErrorType *, BlazeError, JobId, P1, P2, P3> Callback;
    RpcJob3(uint16_t componentId, uint16_t commandId, Callback cb,  P1 arg1, P2 arg2, P3 arg3, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<void, ErrorType>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb), mArg1(arg1), mArg2(arg2), mArg3(arg3) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(static_cast<const ErrorType *>(errorResponse), err, Job::getId(), mArg1, mArg2, mArg3);
    }
    Callback mCb;
    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
};

template <typename P1, typename P2, typename P3>
class RpcJob3<void, void, P1, P2, P3> : public RpcJobBaseTemplate<void, void>
{
public:
    typedef Functor5<BlazeError, JobId, P1, P2, P3> Callback;
    RpcJob3(uint16_t componentId, uint16_t commandId, Callback cb, P1 arg1, P2 arg2, P3 arg3, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) :
        RpcJobBaseTemplate<void, void>(componentId, commandId, clientProvidedResponse, manager),
        mCb(cb), mArg1(arg1), mArg2(arg2), mArg3(arg3) { this->setAssociatedObject(cb.getObject()); }

private:
    void doCallback(const EA::TDF::Tdf *response, const EA::TDF::Tdf *errorResponse, BlazeError err)
    {
        mCb(err, Job::getId(), mArg1, mArg2, mArg3);
    }
    Callback mCb;
    P1 mArg1;
    P2 mArg2;
    P3 mArg3;
};

}        // namespace Blaze

#endif    // BLAZE_RPC_JOB_H

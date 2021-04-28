/*! **********************************************************************************************/
/*!
    \file apijop.cpp

    Jobs wrapper for the API system.  These jobs can be used whenever an API call spans
    multiple RPC's, or there is a delay in making the callback which is dependent
    on an asynchronous message from the server.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/job.h"
#include "BlazeSDK/apijob.h"

namespace Blaze
{

    ApiJobBase::ApiJobBase(API* api, const FunctorBase& cb)
        : mAPI(api), mFunctorCb(cb), mUserIndex(0)
    {
        setAssociatedTitleCb(mFunctorCb);
    }

    ApiJobBase::ApiJobBase(API* api, uint32_t userIndex, const FunctorBase& cb)
        : mAPI(api), mFunctorCb(cb), mUserIndex(userIndex)
    {
        setAssociatedTitleCb(mFunctorCb);
    }


    ApiJobBase::~ApiJobBase()
    {

    }

    void ApiJobBase::clearTitleCb()
    {
        mFunctorCb.clear();
        setAssociatedTitleCb(mFunctorCb);
    }


}        // namespace Blaze

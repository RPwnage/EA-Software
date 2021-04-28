/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/controller/transaction.h"

namespace Blaze
{

TransactionContext::TransactionContext(const char* description) 
    : mDescription(description)
    , mCreationTimestamp(TimeValue::getTimeOfDay())
    , mTimeout(0), mIsActive(true)
{
}

const char8_t *TransactionContext::getTypeName() const 
{ 
    return "TransactionContext"; 
}

BlazeRpcError TransactionContext::commitInternal()
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mIsActive)
    {
        result = commit();
        mIsActive = false;
    }

    return result;
}

BlazeRpcError TransactionContext::rollbackInternal()
{
    BlazeRpcError result = Blaze::ERR_SYSTEM;

    if (mIsActive)
    {
        result = rollback();
        mIsActive = false;
    }

    return result;
}

void TransactionContext::releaseInternal(void)
{ 
    if (mIsActive)
    {
        if (rollback() != ERR_OK)
        {
            BLAZE_TRACE_LOG(Log::SYSTEM, "[TransactionContext].releaseInternal: "
                "Failed to rollback active transaction [id] " << mTransactionContextId);

        }
    }

    delete this; 
}

void TransactionContextHandle::releaseInternal()
{
    if (mAssociated)
    {
        if (mComponent.rollbackTransaction(mId) != ERR_OK)
        {
            BLAZE_TRACE_LOG(Log::SYSTEM, "[TransactionContextHandle].releaseInternal: "
                "Failed to rollback active transaction [id] " << mId);

        }
    }

    delete this;
}


} // namespace Blaze


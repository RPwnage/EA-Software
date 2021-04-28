/*! **********************************************************************************************/
/*!
    \file

    Jobs for the RPC system.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/rpcjob.h"
#include "BlazeSDK/jobscheduler.h"
#include "BlazeSDK/blazesender.h"
#include "framework/protocol/shared/tdfdecoder.h"
#include "BlazeSDK/componentmanager.h"

namespace Blaze
{

RpcJobBase::RpcJobBase(uint16_t componentId, uint16_t commandId, EA::TDF::Tdf *clientProvidedResponse, ComponentManager& manager) : 
    mComponentId(componentId), mCommandId(commandId), mDataSize(0),
    mCallbackErr(SDK_ERR_RPC_TIMEOUT), mReply(nullptr), mReplyMemoryGroupId(MEM_GROUP_FRAMEWORK_TEMP),
    mClientProvidedReply(clientProvidedResponse), mComponentManager(&manager)
{
    // we should only count pending RPCs (this is used to control if a ping is sent) that are going out on the main Blaze connection, and not an override connection.
    if(mComponentManager->getBlazeSender(mComponentId) == nullptr)
        mComponentManager->getDefaultSender().incPendingRpcs();
}

RpcJobBase::~RpcJobBase()
{
    // we should only count pending RPCs (this is used to control if a ping is sent) that are going out on the main Blaze connection, and not an override connection.
    if(mComponentManager->getBlazeSender(mComponentId) == nullptr)
        mComponentManager->getDefaultSender().decPendingRpcs();
}

uint32_t RpcJobBase::getUpdatedExpiryTime()
{
    return mComponentManager->getDefaultSender().getLastReceiveTime() + getDelayMS();
}

void RpcJobBase::execute()
{
    const EA::TDF::Tdf* reply = mClientProvidedReply ? mClientProvidedReply : mReply.get() ;

    BlazeSender::logMessage(mComponentManager, true, false, "resp", reply, getProviderId(), mComponentId, 
        mCommandId, mCallbackErr, mDataSize);

    // Do we own the response TDF?
    if (mClientProvidedReply == nullptr)
    {
        doCallback((mCallbackErr == ERR_OK) ? mReply.get() : nullptr, 
                   (mCallbackErr != ERR_OK) ? mReply.get() : nullptr, 
                    mCallbackErr);
    }
    else
    {
        // We do not own the response TDF, therefore regardless of the
        // error status, we need to return it to the client.
        doCallback( mClientProvidedReply, 
                   (mCallbackErr != ERR_OK) ? mReply.get() : nullptr, 
                    mCallbackErr);    
    }
}

void RpcJobBase::handleReply(BlazeError err, TdfDecoder& decoder, RawBuffer& buf)
{
    mCallbackErr = err;
    mDataSize = buf.datasize();
    
    if (mCallbackErr == ERR_OK)
    {
        // Allocate Response TDF if client did not provide one.
        if (mClientProvidedReply == nullptr)
        {
            mReply = createResponseTdf(mReplyMemoryGroupId);
            if (mReply != nullptr)
            {                 
                // decode the response
                BlazeError decodeError = decoder.decode(buf, *mReply);
                if (decodeError != ERR_OK)
                {
                    BLAZE_SDK_DEBUGF("[RPCJob] RPC for component %u command %u failed to decode reply with error %d\n", mComponentId, mCommandId, decodeError);
                }
            }
        }
        else
        {
            // decode the response
            BlazeError decodeError = decoder.decode(buf, *mClientProvidedReply);

            if (decodeError != ERR_OK)
            {
                BLAZE_SDK_DEBUGF("[RPCJob] RPC for component %u command %u failed to decode reply with error %d\n", mComponentId, mCommandId, decodeError);
            }
        }
    }
    else
    {
        // Allocate Error TDF if needed.
        mReply = createErrorTdf(mReplyMemoryGroupId);
        if (mReply != nullptr)
        {                 
            // decode the response
            decoder.decode(buf, *mReply);
        }
    }

    //Now everything is setup, execute the response.
    execute();
}

}        // namespace Blaze


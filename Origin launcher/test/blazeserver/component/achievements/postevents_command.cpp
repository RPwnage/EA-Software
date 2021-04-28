/*************************************************************************************************/
/*!
    \file   postevents_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class PostEventsCommand

    Submit progression for the new-style dynamic achievements
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/protocol/restprotocolutil.h"
#include "framework/util/hashutil.h"

#include "authentication/rpc/authenticationslave.h"
#include "authentication/tdf/authentication.h"
#include "achievementsslaveimpl.h"
#include "achievements/rpc/achievementsslave/postevents_stub.h"


namespace Blaze
{
namespace Achievements
{

class PostEventsCommand : public PostEventsCommandStub
{
public:

    PostEventsCommand(Message* message, PostEventsRequest* request, AchievementsSlaveImpl* componentImpl);
    ~PostEventsCommand() override {};

private:
    PostEventsCommandStub::Errors execute() override;

    // Not owned memory.
    AchievementsSlaveImpl* mComponent;
    bool mIsTrustedConnection;
};

DEFINE_POSTEVENTS_CREATE()

PostEventsCommand::PostEventsCommand(
    Message* message, PostEventsRequest* request, AchievementsSlaveImpl* componentImpl)
    : PostEventsCommandStub(message, request), mComponent(componentImpl)
{
    mIsTrustedConnection = (message == nullptr) ? true : message->getPeerInfo().isServer();
}

/* Private methods *******************************************************************************/    

PostEventsCommandStub::Errors PostEventsCommand::execute()
{
    AchievementsSlaveImpl::ConnectionManagerType connType = 
        (mIsTrustedConnection) ? AchievementsSlaveImpl::TRUSTED : AchievementsSlaveImpl::UNTRUSTED;
    BlazeRpcError error = ERR_SYSTEM;

    if (mRequest.getProductId() == nullptr || mRequest.getProductId()[0] == '\0')
    {
        // set to default product id if not specified
        mRequest.setProductId(mComponent->getConfig().getProductId());
    }

    // construct the content body by encoding the request tdf or using the input string in JSON format
    RawBuffer payload(1024);   
    if (RestProtocolUtil::encodePayload(mCommandInfo.restResourceInfo, mComponent->getRequestEncoderType(), &mRequest, payload) == false)
    {
        ERR_LOG("[PostEventsCommand].execute(): Failed to encode request payload.");
        return ERR_SYSTEM;
    }
    
    
    // set nucleus auth token if not provided
    if (mRequest.getAuxAuth().getAuthToken()[0] == '\0')
    {
        BlazeRpcError err = mComponent->fetchUserOrServerAuthToken(mRequest.getAuxAuth(), mIsTrustedConnection);
        if (err != ERR_OK)
            return commandErrorFromBlazeError(err);
    }

    if (!mIsTrustedConnection)
    {
        // set timestamp if not provided
        if (mRequest.getAuxAuth().getTimestamp() == 0)
        {
            mRequest.getAuxAuth().setTimestamp(TimeValue::getTimeOfDay().getSec());
        }

        // set cryptographic signature if not provided
        if (mRequest.getAuxAuth().getSignature()[0] == '\0')
        {
            char8_t signature[HashUtil::SHA256_STRING_OUT];
            mComponent->generateSignature(payload.data(), payload.datasize(), mRequest.getAuxAuth().getTimestamp(),
                mRequest.getAuxAuth().getSecretKey(), signature, sizeof(signature));
            mRequest.getAuxAuth().setSignature(signature);
        }
    }

    HttpConnectionManagerPtr connectionManager = mComponent->getConnectionManagerPtr(connType);
    if (connectionManager != nullptr)
    {
        error = RestProtocolUtil::sendHttpRequest(connectionManager, mCommandInfo.restResourceInfo, &mRequest,
            RestProtocolUtil::getContentTypeFromEncoderType(mComponent->getRequestEncoderType()), &mResponse, nullptr, &payload, nullptr, nullptr);
        if (error == Blaze::ERR_OK)
        {
            for (Blaze::Achievements::AchievementList::iterator it = mResponse.getAchievements().begin(), itEnd = mResponse.getAchievements().end(); it != itEnd; ++it)
            {
                mComponent->scaleAchivementDataTimeValue(*(it->second));
            }
        }
    }
    
    return (commandErrorFromBlazeError(error));
}

} // Achievements
} // Blaze

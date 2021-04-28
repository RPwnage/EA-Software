/*************************************************************************************************/
/*!
    \file   grantachievement_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GrantAchievementCommand

    Update progress on an achievement for a user
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
#include "achievements/rpc/achievementsslave/grantachievement_stub.h"


namespace Blaze
{
namespace Achievements
{

class GrantAchievementCommand : public GrantAchievementCommandStub
{
public:

    GrantAchievementCommand(Message* message, GrantAchievementRequest* request, AchievementsSlaveImpl* componentImpl);
    ~GrantAchievementCommand() override {};

private:
    GrantAchievementCommandStub::Errors execute() override;

    // Not owned memory.
    AchievementsSlaveImpl* mComponent;
    bool mIsTrustedConnection;
};

DEFINE_GRANTACHIEVEMENT_CREATE()

GrantAchievementCommand::GrantAchievementCommand(
    Message* message, GrantAchievementRequest* request, AchievementsSlaveImpl* componentImpl)
    : GrantAchievementCommandStub(message, request), mComponent(componentImpl)
{
    mIsTrustedConnection = (message == nullptr) ? true : message->getPeerInfo().isServer();
}

/* Private methods *******************************************************************************/    

GrantAchievementCommandStub::Errors GrantAchievementCommand::execute()
{
    AchievementsSlaveImpl::ConnectionManagerType connType = 
        (mIsTrustedConnection) ? AchievementsSlaveImpl::TRUSTED : AchievementsSlaveImpl::UNTRUSTED;

    if (mRequest.getProductId() == nullptr || mRequest.getProductId()[0] == '\0')
    {
        // set to default product id if not specified
        mRequest.setProductId(mComponent->getConfig().getProductId());
    }

    // construct the content body by encoding the request tdf or using the input string in JSON format
    RawBuffer payload(1024);
    if (RestProtocolUtil::encodePayload(mCommandInfo.restResourceInfo, mComponent->getRequestEncoderType(), &mRequest, payload) == false)
    {
        ERR_LOG("[GrantAchievementCommand].execute(): Failed to encode request payload.");
        return ERR_SYSTEM;
    }
    
    BlazeRpcError error = ERR_SYSTEM;
    // set nucleus auth token if not provided
    if (mRequest.getAuxAuth().getAuthToken()[0] == '\0')
    {
        error = mComponent->fetchUserOrServerAuthToken(mRequest.getAuxAuth(), mIsTrustedConnection);
        if (error != ERR_OK)
            return commandErrorFromBlazeError(error);
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
            mComponent->scaleAchivementDataTimeValue(mResponse);
        }
    }

    return (commandErrorFromBlazeError(error));
}

} // Achievements
} // Blaze

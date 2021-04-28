/*************************************************************************************************/
/*!
    \file   getachievements_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class getachievements_command

    Return the list of achievements (awarded and/or unawarded) to the user
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/protocol/restprotocolutil.h"

#include "achievementsslaveimpl.h"
#include "achievements/rpc/achievementsslave/getachievements_stub.h"


namespace Blaze
{
namespace Achievements
{

class GetAchievementsCommand : public GetAchievementsCommandStub
{
public:

    GetAchievementsCommand(Message* message, GetAchievementsRequest* request, AchievementsSlaveImpl* componentImpl);
    ~GetAchievementsCommand() override {};

private:
    GetAchievementsCommandStub::Errors execute() override;

    // Not owned memory.
    AchievementsSlaveImpl* mComponent;
    bool mIsTrustedConnection;
};

DEFINE_GETACHIEVEMENTS_CREATE()

GetAchievementsCommand::GetAchievementsCommand(
    Message* message, GetAchievementsRequest* request, AchievementsSlaveImpl* componentImpl)
    : GetAchievementsCommandStub(message, request), mComponent(componentImpl)
{
    mIsTrustedConnection = (message == nullptr) ? true : message->getPeerInfo().isServer();
}

/* Private methods *******************************************************************************/    

GetAchievementsCommandStub::Errors GetAchievementsCommand::execute()
{
    AchievementsSlaveImpl::ConnectionManagerType connType = 
        (mIsTrustedConnection) ? AchievementsSlaveImpl::TRUSTED : AchievementsSlaveImpl::UNTRUSTED;

    if (mRequest.getProductId() == nullptr || mRequest.getProductId()[0] == '\0')
    {
        // set to default product id if not specified
        mRequest.setProductId(mComponent->getConfig().getProductId());
    }

    BlazeRpcError error = mComponent->fetchUserOrServerAuthToken(mRequest.getAuxAuth(), mIsTrustedConnection);
    if (error != ERR_OK)
        return commandErrorFromBlazeError(error);

    HttpConnectionManagerPtr connectionManager = mComponent->getConnectionManagerPtr(connType);
    if (connectionManager != nullptr)
    {
        error = RestProtocolUtil::sendHttpRequest(connectionManager, mCommandInfo.restResourceInfo, 
            &mRequest, RestProtocolUtil::getContentTypeFromEncoderType(mComponent->getRequestEncoderType()), &mResponse);

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

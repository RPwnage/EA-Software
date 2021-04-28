/*************************************************************************************************/
/*!
    \file
    \attention

    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/leaveallexternalsessionsforuser_stub.h"
#include "arsonslaveimpl.h"
#include "component/gamemanager/externalsessions/externalsessionutilxboxone.h"

namespace Blaze
{
namespace Arson
{
    class LeaveAllExternalSessionsForUserCommand : public LeaveAllExternalSessionsForUserCommandStub
    {
    public:
        LeaveAllExternalSessionsForUserCommand(Message* message, LeaveAllExternalSessionsForUserRequest* request, ArsonSlaveImpl* componentImpl)
            : LeaveAllExternalSessionsForUserCommandStub(message, request), mComponent(componentImpl)
        {
        }
        ~LeaveAllExternalSessionsForUserCommand() override {}

    private:

        LeaveAllExternalSessionsForUserCommandStub::Errors execute() override
        {
            UserIdentification& ident = mRequest.getUserIdent();
            const Blaze::GameManager::ExternalSessionUtilManager& extSessUtil = mComponent->getExternalSessionServices();
            if (extSessUtil.getExternalSessionUtilMap().empty())
            {
                return ARSON_ERR_EXTERNAL_SESSION_FAILED;//logged
            }

            ClientPlatformType platform = ident.getPlatformInfo().getClientPlatform();
            switch (platform)
            {
            case ClientPlatformType::xone:
                return commandErrorFromBlazeError(mComponent->leaveAllMpsesForUser(ident));
                break;
            case ClientPlatformType::ps4:
            {
                auto* util = extSessUtil.getUtil(platform);
                if (util == nullptr)
                {
                    ERR_LOG("[LeaveAllExternalSessionsForUserCommand].execute: missing ExternalSessionUtil for platform(" << ClientPlatformTypeToString(platform) << "). Cannot process leave!");
                    return ARSON_ERR_EXTERNAL_SESSION_FAILED;
                }
                if (!util->getConfig().getCrossgen())
                {
                    return commandErrorFromBlazeError(mComponent->leaveAllNpsessionsForUser(ident));
                    break;
                }
                //else fall through to use PS5 impln
            }
            case ClientPlatformType::ps5:
            {
                auto err = mComponent->leaveAllPsnMatchesForUser(ident);
                auto err2 = mComponent->leaveAllPsnPlayerSessionsForUser(ident);
                return commandErrorFromBlazeError(err == Blaze::ERR_OK ? err2 : err);
            }
            default:
                ERR_LOG("[LeaveAllExternalSessionsForUserCommand].execute: unhandled platform(" << ClientPlatformTypeToString(platform) << "). No op");
                return commandErrorFromBlazeError(ARSON_ERR_ERR_NOT_SUPPORTED);
            };
        }

    private:
        ArsonSlaveImpl* mComponent;
    };

    DEFINE_LEAVEALLEXTERNALSESSIONSFORUSER_CREATE()

} // Arson
} // Blaze

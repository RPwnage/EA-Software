/*! ************************************************************************************************/
/*!
    \file setdedicatedserverattributes_slavecommand.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/setdedicatedserverattributes_stub.h"
#include "gamemanagerslaveimpl.h"
#include "component/gamemanager/externalsessions/externalsessionutil.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for oldToNewExternalSessionIdentificationParams() in execute()
#include "gamemanager/tdf/externalsessiontypes_server.h" // for SetDedicatedServerAttributes

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief This command is called from client when its local users join or remove from games, to
        update user's game presence.
    ***************************************************************************************************/
    class SetDedicatedServerAttributesCommand : public SetDedicatedServerAttributesCommandStub
    {
    public:

        SetDedicatedServerAttributesCommand(Message* message, SetDedicatedServerAttributesRequest *request, GameManagerSlaveImpl* componentImpl)
            : SetDedicatedServerAttributesCommandStub(message, request), mComponent(*componentImpl)
        {
        }
        ~SetDedicatedServerAttributesCommand() override
        {
        }

    private:

        SetDedicatedServerAttributesCommandStub::Errors execute() override
        {
            // By default, only dedicated servers can make this request, and only for their current Game. 
            if (mRequest.getGameIdList().size() > 1)
            {
                if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_BATCH_UPDATE_DEDICATED_SERVER_ATTRIBUTES))
                {
                    TRACE_LOG("[GameManagerMaster] non-admin user trying to set attributes for multiple games.");
                    return SetDedicatedServerAttributesError::GAMEMANAGER_ERR_PERMISSION_DENIED;
                }
            }

            // Send the request off to all of the GameManagerMasters: 
            Component::InstanceIdList mmInstances;
            mComponent.getMaster()->getComponentInstances(mmInstances, false);
            Component::MultiRequestResponseHelper<void, void> responses(mmInstances);

            mComponent.getMaster()->sendMultiRequest(Blaze::GameManager::GameManagerMaster::CMD_INFO_SETDEDICATEDSERVERATTRIBUTESMASTER, &mRequest, responses);
            for (auto& curResponse : responses)
            {
                if (curResponse->error != ERR_OK)
                {
                    return (SetDedicatedServerAttributesError::Error) curResponse->error;
                }
            }
            return (SetDedicatedServerAttributesError::Error) ERR_OK;
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    DEFINE_SETDEDICATEDSERVERATTRIBUTES_CREATE()

} // namespace GameManager
} // namespace Blaze

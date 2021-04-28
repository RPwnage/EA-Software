/*! ************************************************************************************************/
/*!
    \file resetdedicatedserver_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/connectutil.h"
#include "gamemanager/rpc/gamemanagerslave/resetdedicatedserver_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamemanagervalidationutils.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/tdf/gamebrowser_server.h"
#include "gamemanager/gamemanagerhelperutils.h"
#include "framework/util/random.h"

namespace Blaze
{
namespace GameManager
{

    class ResetDedicatedServerCommand : public ResetDedicatedServerCommandStub
    {
    public:

        ResetDedicatedServerCommand(Message* message, CreateGameRequest *request, GameManagerSlaveImpl* componentImpl)
            : ResetDedicatedServerCommandStub(message, request), mComponent(*componentImpl)
        {
            for (auto& iter : request->getGameCreationData().getSlotCapacitiesMap())
            {
                request->getSlotCapacitiesMap().insert(iter);
            }
            request->getGameCreationData().getSlotCapacitiesMap().clear();

            for (uint32_t i = 0; i < (uint32_t)MAX_SLOT_TYPE; ++i)
            {
                SlotCapacitiesMap::const_iterator iter = request->getSlotCapacitiesMap().find((SlotType)i);
                if (iter != request->getSlotCapacitiesMap().end())
                {
                    request->getSlotCapacities()[iter->first] = iter->second;
                }
            }
            request->getSlotCapacitiesMap().clear();
        }

        ~ResetDedicatedServerCommand() override {}

    private:

        ResetDedicatedServerCommandStub::Errors execute() override
        {
            // Convert the Player Join Data (for back compat)
            for (auto playerJoinData : mRequest.getPlayerJoinData().getPlayerDataList())
                convertToPlatformInfo(playerJoinData->getUser());

            // Convert old Xbox session params to new params (for back compat).
            oldToNewExternalSessionIdentificationParams(mRequest.getGameCreationData().getExternalSessionTemplateName(), nullptr, nullptr, nullptr, mRequest.getGameCreationData().getExternalSessionIdentSetup());

            return commandErrorFromBlazeError(mComponent.resetDedicatedServerInternal(this, nullptr, mRequest, mResponse));
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //static creation factory method of command's stub class
    DEFINE_RESETDEDICATEDSERVER_CREATE()

} // namespace GameManager
} // namespace Blaze

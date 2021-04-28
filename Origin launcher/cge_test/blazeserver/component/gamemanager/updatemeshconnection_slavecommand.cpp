/*! ************************************************************************************************/
/*!
    \file updatemeshconnection_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/updatemeshconnection_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief This RPC is for backwards compatibility with SDK versions X.X.X.X.X and older, and simply redirects requests to the new RPCs in meshendpointsconnectionstatus_slavecommands.cpp
    ***************************************************************************************************/
    class UpdateMeshConnectionCommand : public UpdateMeshConnectionCommandStub
    {
    public:

        UpdateMeshConnectionCommand(Message* message, UpdateMeshConnectionRequest *request, GameManagerSlaveImpl* componentImpl)
            : UpdateMeshConnectionCommandStub(message, request),
              mComponent(*componentImpl)
        {
        }

        ~UpdateMeshConnectionCommand() override
        {
        }

    private:

        UpdateMeshConnectionCommandStub::Errors execute() override
        {
            // Set info for our request to the master
            UpdateMeshConnectionMasterRequest masterRequest;
            // This info is part of the gCurrentUserSession but we pass it explicitly to relieve the master 
            // of the burden of permanently repl-subscribing to the whole usersession object. See: GOS-28952.
            masterRequest.setConnectionAddr(gCurrentLocalUserSession->getConnectionAddr());
            masterRequest.setDeviceInfo(gCurrentLocalUserSession->getClientMetrics().getDeviceInfo());
            masterRequest.setNatType(gCurrentLocalUserSession->getExtendedData().getQosData().getNatType());
            masterRequest.setSourceConnectionGroupId(mRequest.getSourceGroupId().id);
            masterRequest.setTargetConnectionGroupId(mRequest.getTargetGroupId().id);
            masterRequest.setGameId(mRequest.getGameId());
            masterRequest.setPlayerNetConnectionStatus(mRequest.getPlayerNetConnectionStatus());
            masterRequest.getPlayerNetConnectionFlags().setBits(mRequest.getPlayerNetConnectionFlags().getBits());
            mRequest.getQosInfo().copyInto(masterRequest.getQosInfo());
            
            BlazeRpcError err = Blaze::ERR_OK;
            if (mRequest.getPlayerNetConnectionStatus() == CONNECTED)
            {
                err = mComponent.getMaster()->meshEndpointsConnectedMaster(masterRequest);
            }
            else
            {
                err = mComponent.getMaster()->meshEndpointsDisconnectedMaster(masterRequest);
            }

            return commandErrorFromBlazeError(err);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };


    //static creation factory method of command's stub class
    DEFINE_UPDATEMESHCONNECTION_CREATE()

} // namespace GameManager
} // namespace Blaze

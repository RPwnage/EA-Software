/*! ************************************************************************************************/
/*!
    \file meshendpointsconnectionstatus_slavecommands.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/meshendpointsconnected_stub.h"
#include "gamemanager/rpc/gamemanagerslave/meshendpointsdisconnected_stub.h"
#include "gamemanager/rpc/gamemanagerslave/meshendpointsconnectionlost_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace GameManager
{

/*! ************************************************************************************************/
/*! \brief 
***************************************************************************************************/
class MeshEndpointsConnectedCommand : public MeshEndpointsConnectedCommandStub
{
public:

    MeshEndpointsConnectedCommand(Message* message, MeshEndpointsConnectedRequest *request, GameManagerSlaveImpl* componentImpl)
        : MeshEndpointsConnectedCommandStub(message, request),
            mComponent(*componentImpl)
    {
    }

    ~MeshEndpointsConnectedCommand() override
    {
    }

private:

    MeshEndpointsConnectedCommandStub::Errors execute() override
    {
        UpdateMeshConnectionMasterRequest masterRequest;
        
        masterRequest.setSourceConnectionGroupId(gCurrentLocalUserSession->getConnectionGroupId());
        masterRequest.setTargetConnectionGroupId(mRequest.getTargetGroupId().id);
        masterRequest.setGameId(mRequest.getGameId());
        masterRequest.setPlayerNetConnectionStatus(Blaze::GameManager::CONNECTED);
        masterRequest.getPlayerNetConnectionFlags().setBits(mRequest.getPlayerNetConnectionFlags().getBits());
        mRequest.getQosInfo().copyInto(masterRequest.getQosInfo());
        
        BlazeRpcError err = mComponent.getMaster()->meshEndpointsConnectedMaster(masterRequest);
        return commandErrorFromBlazeError(err);
    }

private:
    GameManagerSlaveImpl &mComponent;
};


//static creation factory method of command's stub class
DEFINE_MESHENDPOINTSCONNECTED_CREATE()

/*! ************************************************************************************************/
/*! \brief 
***************************************************************************************************/
class MeshEndpointsDisconnectedCommand : public MeshEndpointsDisconnectedCommandStub
{
public:

    MeshEndpointsDisconnectedCommand(Message* message, MeshEndpointsDisconnectedRequest *request, GameManagerSlaveImpl* componentImpl)
        : MeshEndpointsDisconnectedCommandStub(message, request),
        mComponent(*componentImpl)
    {
    }

    ~MeshEndpointsDisconnectedCommand() override
    {
    }

private:

    MeshEndpointsDisconnectedCommandStub::Errors execute() override
    {
        UpdateMeshConnectionMasterRequest masterRequest;
        
        masterRequest.setSourceConnectionGroupId(gCurrentLocalUserSession->getConnectionGroupId());
        masterRequest.setTargetConnectionGroupId(mRequest.getTargetGroupId().id);
        masterRequest.setGameId(mRequest.getGameId());
        masterRequest.setPlayerNetConnectionStatus(mRequest.getPlayerNetConnectionStatus());
        masterRequest.getPlayerNetConnectionFlags().setBits(mRequest.getPlayerNetConnectionFlags().getBits());
        
        BlazeRpcError err = mComponent.getMaster()->meshEndpointsDisconnectedMaster(masterRequest);
        return commandErrorFromBlazeError(err);
    }

private:
    GameManagerSlaveImpl &mComponent;
};


//static creation factory method of command's stub class
DEFINE_MESHENDPOINTSDISCONNECTED_CREATE()

/*! ************************************************************************************************/
/*! \brief 
***************************************************************************************************/
class MeshEndpointsConnectionLostCommand : public MeshEndpointsConnectionLostCommandStub
{
public:

    MeshEndpointsConnectionLostCommand(Message* message, MeshEndpointsConnectionLostRequest *request, GameManagerSlaveImpl* componentImpl)
        : MeshEndpointsConnectionLostCommandStub(message, request),
        mComponent(*componentImpl)
    {
    }

    ~MeshEndpointsConnectionLostCommand() override
    {
    }

private:

    MeshEndpointsConnectionLostCommandStub::Errors execute() override
    {
        UpdateMeshConnectionMasterRequest masterRequest;
        
        masterRequest.setSourceConnectionGroupId(gCurrentLocalUserSession->getConnectionGroupId());
        masterRequest.setTargetConnectionGroupId(mRequest.getTargetGroupId().id);
        masterRequest.setGameId(mRequest.getGameId());
        masterRequest.setPlayerNetConnectionStatus(Blaze::GameManager::DISCONNECTED);
        masterRequest.getPlayerNetConnectionFlags().setBits(mRequest.getPlayerNetConnectionFlags().getBits());
        
        BlazeRpcError err = mComponent.getMaster()->meshEndpointsConnectionLostMaster(masterRequest);
        return commandErrorFromBlazeError(err);
    }

private:
    GameManagerSlaveImpl &mComponent;
};


//static creation factory method of command's stub class
DEFINE_MESHENDPOINTSCONNECTIONLOST_CREATE()

} // namespace GameManager
} // namespace Blaze

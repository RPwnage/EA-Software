/*! ************************************************************************************************/
/*!
    \file connectivityviaccs_slavecommand.cpp 


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanagerslaveimpl.h"
#include "gamemanager/rpc/gamemanagerslave/requestconnectivityviaccs_stub.h"
#include "gamemanager/rpc/gamemanagerslave/freeconnectivityviaccs_stub.h"
#include "gamemanager/rpc/gamemanagerslave/requestleaseextensionviaccs_stub.h"

namespace Blaze
{
namespace GameManager
{
/*! *****************************************************************************************************************/
/*! \brief This command schedules a fiber to call the CCS service to allocate the connectivity between console pairs 
            specified in the request. The fiber is scheduled in order to avoid blocking the master.
*********************************************************************************************************************/
class RequestConnectivityViaCCSCommand : public RequestConnectivityViaCCSCommandStub
{
public:

    RequestConnectivityViaCCSCommand(Message* message, CCSAllocateRequest *request, GameManagerSlaveImpl* componentImpl)
        : RequestConnectivityViaCCSCommandStub(message, request),
        mComponent(*componentImpl)
    {
    }

    ~RequestConnectivityViaCCSCommand() override
    {
    }

private:

    RequestConnectivityViaCCSCommandStub::Errors execute() override
    {
        CCSAllocateRequestPtr ccsReq = BLAZE_NEW CCSAllocateRequest(mRequest);    
        
        mComponent.getCcsJobQueue().queueFiberJob(&mComponent, &GameManagerSlaveImpl::callCCSAllocate, ccsReq, "GameManagerSlaveImpl::callCCSAllocate");

        return commandErrorFromBlazeError(Blaze::ERR_OK);
    }

private:
    GameManagerSlaveImpl &mComponent;
};


//static creation factory method of command's stub class
DEFINE_REQUESTCONNECTIVITYVIACCS_CREATE()

/*! *****************************************************************************************************************/
/*! \brief This command schedules a fiber to call the CCS service to free the connectivity between console pairs 
            specified in the request. The fiber is scheduled in order to avoid blocking the master.
*********************************************************************************************************************/
class FreeConnectivityViaCCSCommand : public FreeConnectivityViaCCSCommandStub
{
public:

    FreeConnectivityViaCCSCommand(Message* message, CCSFreeRequest *request, GameManagerSlaveImpl* componentImpl)
        : FreeConnectivityViaCCSCommandStub(message, request),
        mComponent(*componentImpl)
    {
    }

    ~FreeConnectivityViaCCSCommand() override
    {
    }

private:

    FreeConnectivityViaCCSCommandStub::Errors execute() override
    {
        CCSFreeRequestPtr ccsReq = BLAZE_NEW CCSFreeRequest(mRequest);  

        mComponent.getCcsJobQueue().queueFiberJob(&mComponent, &GameManagerSlaveImpl::callCCSFree, ccsReq, "GameManagerSlaveImpl::callCCSFree");

        return commandErrorFromBlazeError(Blaze::ERR_OK);
    }

private:
    GameManagerSlaveImpl &mComponent;
};


//static creation factory method of command's stub class
DEFINE_FREECONNECTIVITYVIACCS_CREATE()

/*! *****************************************************************************************************************/
/*! \brief This command schedules a fiber to call the CCS service to extend the lease of a connection set  
    specified in the request. The fiber is scheduled in order to avoid blocking the master.
*********************************************************************************************************************/
class RequestLeaseExtensionViaCCSCommand : public RequestLeaseExtensionViaCCSCommandStub
{
public:

    RequestLeaseExtensionViaCCSCommand(Message* message, CCSLeaseExtensionRequest *request, GameManagerSlaveImpl* componentImpl)
        : RequestLeaseExtensionViaCCSCommandStub(message, request),
        mComponent(*componentImpl)
    {
    }

    ~RequestLeaseExtensionViaCCSCommand() override
    {
    }

private:

    RequestLeaseExtensionViaCCSCommandStub::Errors execute() override
    {
        CCSLeaseExtensionRequestPtr ccsReq = BLAZE_NEW CCSLeaseExtensionRequest(mRequest);    
        
        mComponent.getCcsJobQueue().queueFiberJob(&mComponent, &GameManagerSlaveImpl::callCCSExtendLease, ccsReq, "GameManagerSlaveImpl::callCCSExtendLease");

        return commandErrorFromBlazeError(Blaze::ERR_OK);
    }

private:
    GameManagerSlaveImpl &mComponent;
};


//static creation factory method of command's stub class
DEFINE_REQUESTLEASEEXTENSIONVIACCS_CREATE()

} // namespace GameManager
} // namespace Blaze

/*************************************************************************************************/
/*!
    \file   xblpostresetreputation_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/xblpostresetreputation_stub.h"
#include "framework/connection/outboundhttpservice.h"

#include "xblreputationservice/tdf/xblreputationservice.h"
#include "xblreputationservice/rpc/xblreputationserviceslave.h"

namespace Blaze
{
namespace Arson
{
class XblPostResetReputationCommand : public XblPostResetReputationCommandStub
{
public:
    XblPostResetReputationCommand(
        Message* message, Blaze::Arson::PostResetReputationRequest* request, ArsonSlaveImpl* componentImpl)
        : XblPostResetReputationCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~XblPostResetReputationCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    XblPostResetReputationCommandStub::Errors execute() override
    {

        BlazeRpcError err = ERR_SYSTEM;
        Blaze::XBLReputationService::PostResetReputationRequest req;
        

        Blaze::XBLReputationService::XBLReputationServiceSlave * xblReputationServiceSlave = (Blaze::XBLReputationService::XBLReputationServiceSlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLReputationService::XBLReputationServiceSlave::COMPONENT_INFO.name);

        if (xblReputationServiceSlave == nullptr)
        {
            ERR_LOG("[ArsonComponent] Failed to instantiate XBLReputationServiceSlave object.");
            return ERR_SYSTEM;
        }

        Blaze::ExternalXblAccountId extId = mRequest.getXuid();

        req.setXuid(extId);

        //Blaze::Authentication2::Authentication2Slave* authComponent = static_cast<Blaze::Authentication2::Authentication2Slave*>(gController->getComponent(Blaze::Authentication2::Authentication2Slave::COMPONENT_ID, false, true));
        //if(authComponent == nullptr)
        //{
        //    ERR_LOG("[ArsonComponent] Failed to instantiate authentication object.");
        //    return ERR_SYSTEM;            
        //}

        //// in case we're fetching someone else's token, nucleus requires super user privileges
        //UserSession::SuperUserPermissionAutoPtr autoPtr(true);
        //tokenReq.getRetrieveTokenUsing().setExternalId(extId);
        //err = authComponent->getUserXblToken(tokenReq, tokenRsp);

        //if (err != ERR_OK)
        //{
        //    ERR_LOG("[ArsonComponent] Fail to retrieve xbl auth token for extId: " << extId <<".");
        //    return ERR_SYSTEM;
        //}


        //getRepReq.setXuid(extId);
        //getRepReq.setScid("7492baca-c1b4-440d-a391-b7ef364a8d40");
        //getRepReq.setReputationIdentifier("OverallReputationIsBad");
        //
        //getRepReq.getGetReputationRequestHeader().setContractVersion("1");
        //getRepReq.getGetReputationRequestHeader().setAuthToken(buf.c_str());


        req.getPostResetReputationRequestHeader().setContractVersion("100");
      //  req.getPostResetReputationRequestHeader().setAuthToken(tokenRsp.getXBLToken());
        
        req.getPostResetReputationRequestBody().setFairplayReputation(mRequest.getPostResetReputationRequestBody().getFairplayReputation());
        req.getPostResetReputationRequestBody().setCommsReputation(mRequest.getPostResetReputationRequestBody().getFairplayReputation());
        req.getPostResetReputationRequestBody().setUserContentReputation(mRequest.getPostResetReputationRequestBody().getUserContentReputation());

        err = xblReputationServiceSlave->postResetReputation(req);

        if(err == Blaze::ERR_OK)
        {
            return ERR_OK;
        }
        else
        {
            return ERR_SYSTEM;
        }
    }

};

DEFINE_XBLPOSTRESETREPUTATION_CREATE()

} //Arson
} //Blaze

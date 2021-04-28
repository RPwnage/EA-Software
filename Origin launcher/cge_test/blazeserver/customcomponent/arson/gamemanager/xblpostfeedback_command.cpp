/*************************************************************************************************/
/*!
    \file   xblpostfeedback_command.cpp


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
#include "arson/rpc/arsonslave/xblpostfeedback_stub.h"
#include "framework/connection/outboundhttpservice.h"

#include "xblreputationservice/tdf/xblreputationservice.h"
#include "xblreputationservice/rpc/xblreputationserviceslave.h"

// to get 1st party auth token
#include "framework/rpc/oauthslave.h"
#include "framework/tdf/oauth.h"

namespace Blaze
{
namespace Arson
{
class XblPostFeedbackCommand : public XblPostFeedbackCommandStub
{
public:
    XblPostFeedbackCommand(
        Message* message, Blaze::Arson::PostFeedbackRequest* request, ArsonSlaveImpl* componentImpl)
        : XblPostFeedbackCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~XblPostFeedbackCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    XblPostFeedbackCommandStub::Errors execute() override
    {

        BlazeRpcError err = ERR_SYSTEM;

        Blaze::XBLReputationService::XBLReputationServiceSlave * xblReputationServiceSlave = (Blaze::XBLReputationService::XBLReputationServiceSlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLReputationService::XBLReputationServiceSlave::COMPONENT_INFO.name);

        if (xblReputationServiceSlave == nullptr)
        {
            ERR_LOG("[ArsonComponent] Failed to instantiate XBLReputationConfigsSlave object.");
            return ERR_SYSTEM;
        }

        Blaze::XBLReputationService::PostFeedbackRequest req;
        Blaze::XBLReputationService::ReputationRequestsHeader& header = req.getPostFeedbackRequestHeader();
        Blaze::XBLReputationService::PostFeedbackRequestBody& payload = req.getPostFeedbackRequestBody();
        Blaze::ExternalXblAccountId extId = mRequest.getXuid();

        req.setXuid(extId);

        Blaze::OAuth::GetUserXblTokenRequest tokenReq;
        Blaze::OAuth::GetUserXblTokenResponse tokenRsp;
        tokenReq.getRetrieveUsing().setExternalId(extId);
        Blaze::OAuth::OAuthSlave *oAuthComponent = (Blaze::OAuth::OAuthSlave*) gController->getComponent(
            Blaze::OAuth::OAuthSlave::COMPONENT_ID, false, true);
        if ( oAuthComponent == nullptr)
        {
            ERR_LOG("[XblPostFeedbackCommand].execute: AuthenticationSlave was unavailable, reputation lookup failed.");
            return ERR_SYSTEM;
        }

        // in case we're fetching someone else's token, nucleus requires super user privileges
        UserSession::SuperUserPermissionAutoPtr autoPtr(true);
        err = oAuthComponent->getUserXblToken(tokenReq, tokenRsp);
        if (err != ERR_OK)
        {
            ERR_LOG("[XblPostFeedbackCommand].execute: Failed to retrieve xbl auth token for external id: " << extId << ", with error '" << ErrorHelp::getErrorName(err) <<"'.");
            return commandErrorFromBlazeError(err);
        }

        header.setContractVersion("100");
        header.setAuthToken(tokenRsp.getXblToken());

        payload.setSessionName(mRequest.getPostFeedbackRequestBody().getSessionName());
        payload.setFeedbackType(mRequest.getPostFeedbackRequestBody().getFeedbackType());
        payload.setTextReason(mRequest.getPostFeedbackRequestBody().getTextReason());
        payload.setVoiceReasonId(mRequest.getPostFeedbackRequestBody().getVoiceReasonId());
        payload.setEvidenceId(mRequest.getPostFeedbackRequestBody().getEvidenceId());

        err = xblReputationServiceSlave->postFeedback(req);

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

DEFINE_XBLPOSTFEEDBACK_CREATE()

} //Arson
} //Blaze

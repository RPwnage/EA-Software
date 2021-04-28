/*************************************************************************************************/
/*!
    \file   XblGetReputation_command.cpp


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
#include "arson/rpc/arsonslave/xblgetreputation_stub.h"

#include "externalreputationservicefactorytesthook.h"
#include "xblreputationconfigs/tdf/xblreputationconfigs.h"
#include "xblreputationconfigs/rpc/xblreputationconfigsslave.h"

// to get 1st party auth token
#include "framework/rpc/oauthslave.h"
#include "framework/tdf/oauth.h"

#include "framework/connection/outboundhttpservice.h"

namespace Blaze
{
namespace Arson
{
class XblGetReputationCommand : public XblGetReputationCommandStub, public Blaze::ReputationService::TestHook
{
public:
    XblGetReputationCommand(
        Message* message, Blaze::Arson::GetReputationRequest* request, ArsonSlaveImpl* componentImpl)
        : XblGetReputationCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~XblGetReputationCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    XblGetReputationCommandStub::Errors execute() override
    {
        Blaze::XBLReputation::GetReputationRequest req;
        Blaze::XBLReputation::GetReputationRequestHeader& header = req.getGetReputationRequestHeader();
        Blaze::XBLReputation::GetReputationResponse response;
        BlazeRpcError err = ERR_SYSTEM;

        Blaze::XBLReputation::XBLReputationConfigsSlave * xblReputationConfigsSlave = (Blaze::XBLReputation::XBLReputationConfigsSlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLReputation::XBLReputationConfigsSlave::COMPONENT_INFO.name);

        if (xblReputationConfigsSlave == nullptr)
        {
            ERR_LOG("[ArsonComponent] Failed to instantiate XBLReputationConfigsSlave object.");
            return ERR_SYSTEM;
        }

        Blaze::ExternalXblAccountId extId = mRequest.getXuid();
        const char8_t* reputationIdentifier = gUserSessionManager->getConfig().getReputation().getExternalReputationIdentifier();

        req.setXuid(extId);
        req.setScid(mRequest.getScid());
        req.setReputationIdentifier(reputationIdentifier);

        if(mRequest.getUseValidAuthToken())
        {
            Blaze::OAuth::GetUserXblTokenRequest tokenReq;
            Blaze::OAuth::GetUserXblTokenResponse tokenRsp;
            tokenReq.getRetrieveUsing().setExternalId(extId);
            Blaze::OAuth::OAuthSlave *oAuthComponent = (Blaze::OAuth::OAuthSlave*) gController->getComponent(
                Blaze::OAuth::OAuthSlave::COMPONENT_ID, false, true);
            if ( oAuthComponent == nullptr)
            {
                ERR_LOG("[XblGetReputationCommandStub].execute: AuthenticationSlave was unavailable, reputation lookup failed.");
                return ERR_SYSTEM;
            }
            UserSession::SuperUserPermissionAutoPtr autoPtr(true);
            err = oAuthComponent->getUserXblToken(tokenReq, tokenRsp);
            if (err != ERR_OK)
            {
                ERR_LOG("[XblGetReputationCommandStub].execute: Failed to retrieve xbl auth token for external id: " << extId << ", with error '" << ErrorHelp::getErrorName(err) <<"'.");
                return commandErrorFromBlazeError(err);
            }

            header.setAuthToken(tokenRsp.getXblToken());
        }
        else
        {
            header.setAuthToken("invalidAuthToken");
        }

        err = xblReputationConfigsSlave->getReputation(req, response);

        Blaze::Arson::User &user = mResponse.getUser();
        Blaze::ExternalXblAccountId extIdResp;
        blaze_str2int(response.getXuid(), &extIdResp);
        
        user.setXuid(extIdResp);
        //user.setGamertag(response.getUser().getGamertag());
        
        Blaze::Arson::StatsList &statslist = user.getStats();
        Blaze::XBLReputation::Stats &stats = response.getStats();
        Blaze::XBLReputation::Stats::iterator it = stats.begin();
        for(; it != stats.end(); ++it)
        {
             Blaze::Arson::Stat* stat = statslist.pull_back();
             stat->setStatname((**it).getStatname());
             stat->setType((**it).getType());
             stat->setValue((const char8_t *)(**it).getValue());
        }

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

DEFINE_XBLGETREPUTATION_CREATE()

} //Arson
} //Blaze

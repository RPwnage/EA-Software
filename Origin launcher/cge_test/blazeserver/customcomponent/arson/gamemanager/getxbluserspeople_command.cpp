/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/getxbluserspeople_stub.h"
#include "arsonslaveimpl.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"

namespace Blaze
{
namespace Arson
{

//      (side: we have this in ARSON, because blazeserver xblservicesconfigs.rpc doesn't actually have a 'get' external session method/rpc)
class GetXblUsersPeopleCommand : public GetXblUsersPeopleCommandStub
{
public:
    GetXblUsersPeopleCommand(Message* message, ArsonGetXblUsersPeopleRequest* request, ArsonSlaveImpl* componentImpl)
        : GetXblUsersPeopleCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~GetXblUsersPeopleCommand() override {}

private:

    GetXblUsersPeopleCommandStub::Errors execute() override
    {
        eastl::string extToken;
        UserIdentification ident;
        convertToPlatformInfo(ident.getPlatformInfo(), mRequest.getExternalId(), nullptr, INVALID_ACCOUNT_ID, xone);
        ident.setExternalId(mRequest.getExternalId());
        ident.setBlazeId(mRequest.getExternalId());

        Blaze::BlazeRpcError error = mComponent->getExternalSessionServiceXbox(ident.getPlatformInfo().getClientPlatform())->getAuthToken(ident, gController->getDefaultServiceName(), extToken);
        if (error != Blaze::ERR_OK)
        {
            WARN_LOG("[ArsonSlaveImpl].GetXblUsersPeopleCommand: Could not retrieve xbl auth token. error " << ErrorHelp::getErrorName(error));
            return commandErrorFromBlazeError(error);
        }

        Blaze::GameManager::GameManagerServerConfig gameManagerServerConfig;
        BlazeRpcError rc = gController->getConfigTdf("gamemanager", false, gameManagerServerConfig);
        if (rc != ERR_OK)
        {
            ERR_LOG("[ArsonSlaveImpl].GetXblUsersPeopleCommand: Failure fetching config for game manager, error=" << ErrorHelp::getErrorName(rc));
            return ERR_SYSTEM;
        }

        Arson::GetRestXblUsersPeopleRequest req;
        Arson::GetRestXblUsersPeopleResponse rsp;
        req.setExternalId(mRequest.getExternalId());
        req.getHeader().setAuthToken(extToken.c_str());
        req.getHeader().setContractVersion(mComponent->getExternalSessionServiceXbox(ident.getPlatformInfo().getClientPlatform())->getConfig().getContractVersion());
        BlazeRpcError restErr = mComponent->getRestXblUsersPeople(req, rsp);
        if (restErr != ERR_OK)
        {
            ERR_LOG("[GetXblUsersPeopleCommandStub].execute: failed XBL REST call on error " << ErrorHelp::getErrorName(restErr));
        }

        for (RestXblUsersPeople::const_iterator itr = rsp.getPeople().begin(), end = rsp.getPeople().end();
            itr != end; ++itr)
        {
            Blaze::Arson::ArsonXblUserPerson* person = mResponse.getPeople().pull_back();
            person->setXuid((*itr)->getXuid());
            person->setIsFollowedByCaller((*itr)->getIsFollowedByCaller());
            person->setIsFollowingCaller((*itr)->getIsFollowingCaller());
        }

        return commandErrorFromBlazeError(restErr);
    }

private:
    ArsonSlaveImpl* mComponent;
};

DEFINE_GETXBLUSERSPEOPLE_CREATE()

} // Arson
} // Blaze

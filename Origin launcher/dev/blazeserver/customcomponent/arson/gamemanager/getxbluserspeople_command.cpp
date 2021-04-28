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
        ClientPlatformType platform = (mRequest.getForUser().getPlatformInfo().getClientPlatform() == INVALID ? xone : mRequest.getForUser().getPlatformInfo().getClientPlatform());
        if ((mRequest.getForUser().getPlatformInfo().getExternalIds().getXblAccountId() == INVALID_XBL_ACCOUNT_ID)) //for back compat, with older arson clients that didn't enable this yet for XBSX:
        {
            convertToPlatformInfo(mRequest.getForUser().getPlatformInfo(), mRequest.getExternalId(), nullptr, INVALID_ACCOUNT_ID, platform);
        }
        UserIdentification& ident = mRequest.getForUser();

        GameManager::ExternalSessionUtilXboxOne* extSessUtil = (mComponent ? mComponent->getExternalSessionServiceXbox(ident.getPlatformInfo().getClientPlatform()) : nullptr);
        if (extSessUtil == nullptr)
        {
            eastl::string logBuf;
            ERR_LOG(logPrefix() << ".execute: ExternalSession Util null for input platform(" << ClientPlatformTypeToString(ident.getPlatformInfo().getClientPlatform()) << "), check test setup code. " << (mComponent ? mComponent->toCurrPlatformsHavingExtSessConfigLogStr(logBuf, nullptr) : "<ArsonComponent null>"));
            return ERR_SYSTEM;
        }
        Blaze::BlazeRpcError error = extSessUtil->getAuthToken(ident, gController->getDefaultServiceName(), extToken);
        if (error != Blaze::ERR_OK)
        {
            WARN_LOG(logPrefix() << ".execute: Could not retrieve xbl auth token. error " << ErrorHelp::getErrorName(error));
            return commandErrorFromBlazeError(error);
        }

        Arson::GetRestXblUsersPeopleRequest req;
        Arson::GetRestXblUsersPeopleResponse rsp;
        req.setExternalId(ident.getPlatformInfo().getExternalIds().getXblAccountId());
        req.getHeader().setAuthToken(extToken.c_str());
        req.getHeader().setContractVersion(extSessUtil->getConfig().getContractVersion());
        BlazeRpcError restErr = mComponent->getRestXblUsersPeople(req, rsp);
        if (restErr != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".execute: failed XBL REST call on error " << ErrorHelp::getErrorName(restErr));
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

    const char8_t* logPrefix() const { return (mLogPrefix.empty() ? mLogPrefix.sprintf("[GetXblUsersPeopleCommand]").c_str() : mLogPrefix.c_str()); }
    mutable eastl::string mLogPrefix;
};

DEFINE_GETXBLUSERSPEOPLE_CREATE()

} // Arson
} // Blaze

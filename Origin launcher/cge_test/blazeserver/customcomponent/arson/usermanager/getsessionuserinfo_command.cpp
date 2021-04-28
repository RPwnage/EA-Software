
/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getsessionuserinfo_stub.h"

namespace Blaze
{
namespace Arson
{
class GetSessionUserInfoCommand : public GetSessionUserInfoCommandStub
{
public:
    GetSessionUserInfoCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : GetSessionUserInfoCommandStub(message),
        mComponent(componentImpl)
    {
    }
    ~GetSessionUserInfoCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetSessionUserInfoCommandStub::Errors execute() override
    {
        if(gCurrentLocalUserSession == nullptr)
        {
            ERR_LOG("[GetSessionUserInfoCommandStub].execute: No current usersession.");
            return ERR_SYSTEM;
        }

        ExternalId extId = getExternalIdFromPlatformInfo(gCurrentLocalUserSession->getUserInfo().getPlatformInfo());

        mResponse.setId(gCurrentLocalUserSession->getUserInfo().getId());
        // gCurrentLocalUserSession->getUserInfo().getPlatformInfo().copyInto(mResponse.getUserLoginInfo().getPlatformInfo()); // CROSSPLAY_TODO Reviewed by Arpit. Not sure about it. No integrator impact
        mResponse.setAccountId(gCurrentLocalUserSession->getUserInfo().getPlatformInfo().getEaIds().getNucleusAccountId());
        mResponse.setPersonaName(gCurrentLocalUserSession->getUserInfo().getPersonaName());
        mResponse.setPersonaNamespace(gCurrentLocalUserSession->getUserInfo().getPersonaNamespace());
        mResponse.setExternalId(extId);
        mResponse.setClientPlatform(gCurrentLocalUserSession->getUserInfo().getPlatformInfo().getClientPlatform());
        mResponse.setAccountLocale(gCurrentLocalUserSession->getUserInfo().getAccountLocale());
        mResponse.setAccountCountry(gCurrentLocalUserSession->getUserInfo().getAccountCountry());
        mResponse.setUserInfoAttribute(gCurrentLocalUserSession->getUserInfo().getUserInfoAttribute());
        mResponse.setStatus(gCurrentLocalUserSession->getUserInfo().getStatus());
        mResponse.setFirstLoginDateTime(gCurrentLocalUserSession->getUserInfo().getFirstLoginDateTime());
        mResponse.setLastLoginDateTime(gCurrentLocalUserSession->getUserInfo().getLastLoginDateTime());
        mResponse.setPreviousLoginDateTime(gCurrentLocalUserSession->getUserInfo().getPreviousLoginDateTime());
        mResponse.setLastLogoutDateTime(gCurrentLocalUserSession->getUserInfo().getLastLogoutDateTime());
        mResponse.setLastLocaleUsed(gCurrentLocalUserSession->getUserInfo().getLastLocaleUsed());
        mResponse.setLastAuthErrorCode(gCurrentLocalUserSession->getUserInfo().getLastAuthErrorCode());
        mResponse.setIsChildAccount(gCurrentLocalUserSession->getUserInfo().getIsChildAccount());
        mResponse.setOriginPersonaId(gCurrentLocalUserSession->getUserInfo().getOriginPersonaId());
        mResponse.setPidId(gCurrentLocalUserSession->getUserInfo().getPidId());

        return ERR_OK;
    }
};

DEFINE_GETSESSIONUSERINFO_CREATE()

} //Arson
} //Blaze

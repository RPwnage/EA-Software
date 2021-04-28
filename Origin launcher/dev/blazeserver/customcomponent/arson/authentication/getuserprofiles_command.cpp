/*************************************************************************************************/
/*!
    \file   getuserprofiles_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/connection/outboundhttpservice.h"


#include "framework/oauth/accesstokenutil.h"
#include "authentication/util/authenticationutil.h"

#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"
#include "proxycomponent/nucleusidentity/tdf/nucleusidentity.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getuserprofiles_stub.h"

namespace Blaze
{
namespace Arson
{

class GetUserProfilesCommand : public GetUserProfilesCommandStub
{

public:
    GetUserProfilesCommand(Message* message, Blaze::Arson::GetUserProfilesRequest* request, ArsonSlaveImpl* componentImpl)
        : GetUserProfilesCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetUserProfilesCommand() override { }

private:
    // Not owned memory.
    ArsonSlaveImpl *mComponent;

/* Private methods *******************************************************************************/
    GetUserProfilesCommandStub::Errors execute() override
    {
        NucleusIdentity::NucleusIdentitySlave* ident = (NucleusIdentity::NucleusIdentitySlave*)(gOutboundHttpService->getService(
            NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name));
        if (ident == nullptr)
        {
            WARN_LOG("[GetUserProfilesCommand].execute() - NucleusIdentity component is not available");
            return ERR_SYSTEM;
        }

        OAuth::AccessTokenUtil serverTokenUtil;
        BlazeRpcError rc = serverTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
        if (rc != ERR_OK)
        {
            ERR_LOG("[GetUserProfilesCommand].execute: Failed to retrieve server access token with error(" << ErrorHelp::getErrorName(rc) << ")");
            return commandErrorFromBlazeError(rc);
        }

        OAuth::AccessTokenUtil userTokenUtil;
        rc = userTokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_NONE);
        if (rc != ERR_OK)
        {
            ERR_LOG("[GetUserProfilesCommand].execute: Failed to retrieve user access token with error(" << ErrorHelp::getErrorName(rc) << ") for user " << gCurrentUserSession->getBlazeId());
            return commandErrorFromBlazeError(rc);
        }

        NucleusIdentity::GetProfileInfoRequest req;
        NucleusIdentity::GetProfileInfoResponse resp;
        NucleusIdentity::IdentityError error;
        req.getAuthCredentials().setAccessToken(serverTokenUtil.getAccessToken());
        req.getAuthCredentials().setUserAccessToken(userTokenUtil.getAccessToken());
        req.getAuthCredentials().setClientId(userTokenUtil.getClientId());
        req.setPid(mRequest.getAccountId());
        req.setProfileInfoCategory(mRequest.getCategory());
        req.setExpandResults("true");

        req.getAuthCredentials().setIpAddress(Blaze::Authentication::AuthenticationUtil::getRealEndUserAddr(this).getIpAsString());

        rc = ident->getProfileInfo(req, resp, error);
        if (rc != Blaze::ERR_OK)
        {
            rc = Authentication::AuthenticationUtil::parseIdentityError(error, rc);
            ERR_LOG("[GetUserProfilesCommand].execute: getProfileInfo failed with error(" << ErrorHelp::getErrorName(rc) << ")");
            return commandErrorFromBlazeError(rc);
        }

        mResponse.setAccountId(mRequest.getAccountId());
        const NucleusIdentity::ProfileEntryWrapper& profileEntry = resp.getPidProfiles();
        NucleusIdentity::ProfileEntryList::const_iterator begin = profileEntry.getPidProfileEntry().begin();
        NucleusIdentity::ProfileEntryList::const_iterator end = profileEntry.getPidProfileEntry().end();
        for (; begin != end; ++begin)
        {
            Arson::ProfileInfoElements* info = mResponse.getProfileInfoElementsByCategory().allocate_element();
            mResponse.getProfileInfoElementsByCategory()[(*begin)->getEntryCategory()] = info;
            NucleusIdentity::EntryElementList::const_iterator listIter = (*begin)->getEntryElement().begin();
            NucleusIdentity::EntryElementList::const_iterator listEnd = (*begin)->getEntryElement().end();
            for (; listIter != listEnd; ++listIter)
            {
                (*info)[(*listIter)->getProfileType()] = (*listIter)->getValue();
            }
        }

        return ERR_OK;
    }
};

DEFINE_GETUSERPROFILES_CREATE()

} // Arson
} // Blaze

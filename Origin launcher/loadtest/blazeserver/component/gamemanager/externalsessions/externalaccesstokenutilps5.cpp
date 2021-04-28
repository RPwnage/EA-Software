/*! ************************************************************************************************/
/*!
    \file externalaccesstokenutilps5.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "component/gamemanager/externalsessions/externalaccesstokenutilps5.h"
#include "framework/logger.h"
#include "framework/controller/controller.h" // for gController in isMockPlatformEnabled
#include "framework/rpc/oauthslave.h"//for OAuthSlave in getUserOnlineAuthToken()
#include "framework/usersessions/usersessionmanager.h" //for UserSessionManager::isStatelessUser getUserOnlineAuthToken()
#include "component/gamemanager/externalsessions/externaluserscommoninfo.h" //for toExtUserLogStr()

namespace Blaze
{

namespace GameManager
{
    ExternalAccessTokenUtilPs5::ExternalAccessTokenUtilPs5(bool isMockEnabled) : mIsMockEnabled(isMockEnabled)
    {
    }

    ExternalAccessTokenUtilPs5::~ExternalAccessTokenUtilPs5()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief retrieve PSN token for the external session updates. Pre: authentication component available
        \param[out] buf - holds the token on success. If this is for an 'external user', this may end up empty.
            Callers are expected to check for empty token as needed before attempting PSN calls.
    ***************************************************************************************************/
    BlazeRpcError ExternalAccessTokenUtilPs5::getUserOnlineAuthToken(eastl::string& buf, const UserIdentification& user, const char8_t* serviceName, bool forceRefresh, bool crossgen) const
    {
        if (user.getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID)
        {
            // non PSN clients may be missing the external id
            TRACE_LOG(logPrefix() << ".getUserOnlineAuthToken: not retrieving token, invalid identification for Caller(" << GameManager::toExtUserLogStr(user) << ").");
            return ERR_OK;
        }
        else if ((user.getPlatformInfo().getClientPlatform() != ps5) &&
            !(crossgen && user.getPlatformInfo().getClientPlatform() == ps4))
        {
            TRACE_LOG(logPrefix() << ".getUserOnlineAuthToken: not retrieving token, not a PS5 nor cross gen PS4 identification for Caller(" << GameManager::toExtUserLogStr(user) << ").");
            return ERR_OK;
        }

        BlazeRpcError err = ERR_OK;
        auto* oAuthSlave = static_cast<OAuth::OAuthSlave*>(gController->getComponent(OAuth::OAuthSlave::COMPONENT_ID, false, true, &err));
        if (oAuthSlave == nullptr)
        {
            ERR_LOG(logPrefix() << ".getUserOnlineAuthToken: Unable to resolve auth component, error: " << ErrorHelp::getErrorDescription(err));
            return err;
        }

        OAuth::GetUserPsnTokenRequest req;
        OAuth::GetUserPsnTokenResponse rsp;
        req.setForceRefresh(forceRefresh);
        req.setPlatform(user.getPlatformInfo().getClientPlatform());
        req.setCrossgen(crossgen);
        if (EA_LIKELY(((user.getBlazeId() != INVALID_BLAZE_ID) && !UserSessionManager::isStatelessUser(user.getBlazeId()))))
        {
            req.getRetrieveUsing().setPersonaId(user.getBlazeId());
        }
        else
        {
            WARN_LOG(logPrefix() << ".getUserOnlineAuthToken: user info missing valid persona id, attempting to fetch token by accountId(" << user.getPlatformInfo().getExternalIds().getPsnAccountId() <<
                ") instead. Note: this may be an unneeded call as PSN 1st party sessions doesn't support reservations for users.");
            req.getRetrieveUsing().setExternalId(user.getPlatformInfo().getExternalIds().getPsnAccountId());
        }

        // in case we're fetching someone else's token, nucleus requires super user privileges
        UserSession::SuperUserPermissionAutoPtr permissionPtr(true);

        err = oAuthSlave->getUserPsnToken(req, rsp);
        if (err == ERR_OK)
        {
            buf.sprintf("Bearer %s", rsp.getPsnToken()).c_str();
            TRACE_LOG(logPrefix() << ".getUserOnlineAuthToken: Got accountId(" << user.getPlatformInfo().getExternalIds().getPsnAccountId() << ") auth token(" << rsp.getPsnToken() << "), expires in " <<
                (rsp.getExpiresAt().getSec() - TimeValue::getTimeOfDay().getSec()) << "s.");
        }
        else
        {
            // side: unlike Xbox, no need to see if its an 'external user' here, as 1st party sessions don't support reservations
            ERR_LOG(logPrefix() << ".getUserOnlineAuthToken: Unable to get auth token. Caller(" << GameManager::toExtUserLogStr(user) << "). err(" << ErrorHelp::getErrorName(err) << ", '" << ErrorHelp::getErrorDescription(err) << "').");
        }

        if (isMockPlatformEnabled())
        {
            // Override token to be user's id, to allow mock service to identify the caller by token. To avoid blocking tests, ignore real auth/nucleus errors (already logged)
            buf.sprintf("%" PRIu64"", user.getPlatformInfo().getExternalIds().getPsnAccountId());
            err = ERR_OK;
        }
        return err;
    }



    bool ExternalAccessTokenUtilPs5::isMockPlatformEnabled() const
    {
        return (mIsMockEnabled && gController->getUseMockConsoleServices());
    }

}//ns
}//Blaze

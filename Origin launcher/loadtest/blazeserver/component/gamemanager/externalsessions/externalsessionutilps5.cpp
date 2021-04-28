/*! ************************************************************************************************/
/*!
    \file externalsessionutilps5.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "component/gamemanager/externalsessions/externalsessionutilps5.h"
#include "component/gamemanager/gamemanagermasterimpl.h"//for gGameManagerMaster in isCrossgenConfiguredForPlatform
#include "component/gamemanager/gamemanagerslaveimpl.h"//for gameManagerSlave in isCrossgenConfiguredForPlatform
#include "framework/protocol/restprotocolutil.h"//for RestProtocolUtil::sendHttpRequest in callPSNInternal

namespace Blaze
{
using namespace PSNServices;
namespace GameManager
{
    ExternalSessionUtilPs5::ExternalSessionUtilPs5(ClientPlatformType platform, const ExternalSessionServerConfig& config, const TimeValue& reservationTimeout, GameManagerSlaveImpl* gameManagerSlave)
        : ExternalSessionUtil(config, reservationTimeout), mMatches(config, gameManagerSlave), mPlayerSessions(platform, config, gameManagerSlave),
        mPlatform(platform)
    {
    }
    ExternalSessionUtilPs5::~ExternalSessionUtilPs5()
    {
    }

    bool ExternalSessionUtilPs5::verifyParams(const ExternalSessionServerConfig& config, const TeamNameByTeamIdMap* teamNameByTeamIdMap, const GameManagerSlaveImpl* gameManagerSlave)
    {
        if (isCrossgenConfiguredForPlatform(ps4, gameManagerSlave) != isCrossgenConfiguredForPlatform(ps5, gameManagerSlave))
        {
            ERR_LOG("[ExternalSessionUtilPs5PlayerSessions].verifyParams: Configured crossgen for only one of ps4(" << (isCrossgenConfiguredForPlatform(ps4, gameManagerSlave) ? "yes" : "no") << ") and ps5(" << (isCrossgenConfiguredForPlatform(ps5, gameManagerSlave) ? "yes" : "no") << "). PS crossgen requires both these to be hosted and configured for crossgen together.");
            return false;
        }

        if (!ExternalSessionUtilPs5Matches::verifyParams(config, teamNameByTeamIdMap))
            return false;

        if (!ExternalSessionUtilPs5PlayerSessions::verifyParams(config))
            return false;

        return true;
    }

    bool ExternalSessionUtilPs5::isCrossgenConfiguredForPlatform(ClientPlatformType platform, const GameManagerSlaveImpl* gameManagerSlave)
    {
        const GameSessionServerConfig* config = nullptr;
        if (gameManagerSlave != nullptr)
        {
            config = &gameManagerSlave->getConfig().getGameSession();
        }
        else if (gGameManagerMaster != nullptr)
        {
            config = &gGameManagerMaster->getConfig().getGameSession();
        }
        if (!config || !gController || !gController->isPlatformHosted(platform))
        {
            return false;
        }
        auto it = config->getExternalSessions().find(platform);
        if (it == config->getExternalSessions().end())
        {
            return false;
        }
        return (!ExternalSessionUtilPs5PlayerSessions::isPlayerSessionsDisabled(*it->second) && it->second->getCrossgen()); //Matches also needs PlayerSessions/SessionManager
    }
    
    /*!************************************************************************************************/
    /*! \brief updates PS5 Match presence for the user
    ***************************************************************************************************/
    Blaze::BlazeRpcError ExternalSessionUtilPs5::updateNonPrimaryPresence(const UpdateExternalSessionPresenceForUserParameters& params,
        UpdateExternalSessionPresenceForUserResponse& result,
        UpdateExternalSessionPresenceForUserErrorInfo& errorResult)
    {
        return mMatches.updatePresenceForUser(params, result, errorResult);
    }


    /*!************************************************************************************************/
    /*! \brief Gets a PS5 user token to buffer, for making PSN S2S requests.
        \note Returns *user* tokens only. Match's client credentials tokens generated for the system, aren't handled here.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5::getAuthToken(const UserIdentification& user, const char8_t* serviceName, eastl::string& buf)
    {
        ExternalUserAuthInfo authInfo;
        auto err = mPlayerSessions.getAuthToken(authInfo, user, serviceName, false);
        if (err == ERR_OK)
            buf = authInfo.getCachedExternalSessionToken();
        return err;
    }

    /*!************************************************************************************************/
    /*! \brief updates PS5 PlayerSession presence for the user
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5::setPrimary(const SetPrimaryExternalSessionParameters& params,
        SetPrimaryExternalSessionResult& result,
        SetPrimaryExternalSessionErrorResult* errorResult)
    {
        return mPlayerSessions.setPrimary(params, result, errorResult);
    }

    /*!************************************************************************************************/
    /*! \brief clears PS5 PlayerSession presence for the user
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5::clearPrimary(const ClearPrimaryExternalSessionParameters& params, ClearPrimaryExternalSessionErrorResult* errorResult)
    {
        return mPlayerSessions.clearPrimary(params, errorResult);
    }

    bool ExternalSessionUtilPs5::hasUpdatePrimaryExternalSessionPermission(const UserSessionExistenceData& userSession)
    {
        return mPlayerSessions.hasUpdatePrimaryExternalSessionPermission(userSession);
    }


    /*! ************************************************************************************************/
    /*! \brief Checks whether game change requires potential updates to its Match/PlayerSession properties.
    ***************************************************************************************************/
    bool ExternalSessionUtilPs5::isUpdateRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context)
    {
        return (mMatches.isUpdateRequired(origValues, newValues, context)
            || mPlayerSessions.isUpdateRequired(origValues, newValues, context));
    }
    /*!************************************************************************************************/
    /*! \brief updates game's Match/PlayerSession properties.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5::update(const UpdateExternalSessionPropertiesParameters& params)
    {
        BlazeRpcError retErr = ERR_OK;

        auto err = mMatches.update(params);
        if (err != ERR_OK)
            retErr = err;

        err = mPlayerSessions.update(params);
        if (err != ERR_OK)
            retErr = err;

        return retErr;
    }
    /*!************************************************************************************************/
    /*! \brief Matches don't need any specific updater, but PlayerSessions do, so get one for PlayerSession updates.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5::getUpdater(const ExternalMemberInfoListByActivityTypeInfo& possibleUpdaters, ExternalMemberInfo& updater, BlazeIdSet& avoid, const UpdateExternalSessionPropertiesParameters* updatePropertiesParams /*= nullptr*/)
    {
        if (updatePropertiesParams && !mPlayerSessions.isUpdateRequired(updatePropertiesParams->getExternalSessionOrigInfo(), updatePropertiesParams->getExternalSessionUpdateInfo(), updatePropertiesParams->getContext()))
        {
            // Matches alone should just NOOP without any error code
            if (mMatches.isUpdateRequired(updatePropertiesParams->getExternalSessionOrigInfo(), updatePropertiesParams->getExternalSessionUpdateInfo(), updatePropertiesParams->getContext()))
                return ERR_OK;
            return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }
        return mPlayerSessions.getUpdater(possibleUpdaters, updater, avoid, updatePropertiesParams);
    }

    void ExternalSessionUtilPs5::prepForReplay(GameSessionMaster& gameSession)
    {
        mMatches.prepForReplay(gameSession);
    }

    Blaze::BlazeRpcError ExternalSessionUtilPs5::prepForCreateOrJoin(CreateGameRequest& request)
    {
        return mPlayerSessions.prepForCreateOrJoin(request);
    }

    bool ExternalSessionUtilPs5::onPrepareForReconfigure(const ExternalSessionServerConfig& config)
    {
        return (mMatches.onPrepareForReconfigure(config) && mPlayerSessions.onPrepareForReconfigure(config));
    }

    const char8_t* ExternalSessionUtilPs5::logPrefix() const 
    {
        return (mLogPrefix.empty() ? mLogPrefix.sprintf("[ExternalSessionUtilPs5]").c_str() : mLogPrefix.c_str());
    }

    /*! ************************************************************************************************/
    /*! \brief log helper
    ***************************************************************************************************/
    const FixedString256 ExternalSessionUtilPs5::toLogStr(BlazeRpcError err, const PSNServices::PsnErrorResponse *psnRsp /*= nullptr*/)
    {
        FixedString256 buf(FixedString256::CtorSprintf(), "err(%s, '%s')", ErrorHelp::getErrorName(err), ErrorHelp::getErrorDescription(err));
        if (psnRsp != nullptr)
        {
            buf.append_sprintf(", serviceError(%i, '%s'), refId(%s)", psnRsp->getError().getCode(), psnRsp->getError().getMessage(), psnRsp->getError().getReferenceId()).c_str();
        }
        return buf;
    }
    /*! ************************************************************************************************/
    /*! \brief helper to populate Blaze error info from the PSN info. For granular error handling and logging
    ***************************************************************************************************/
    void ExternalSessionUtilPs5::toExternalSessionErrorInfo(Blaze::ExternalSessionErrorInfo *errorInfo, const Blaze::PSNServices::PsnErrorResponse& psnRsp)
    {
        if (errorInfo != nullptr)
        {
            errorInfo->setCode(psnRsp.getError().getCode());
            errorInfo->setMessage(psnRsp.getError().getMessage());
        }
    }




    ////////////////////////////// PSN Connection //////////////////////////////////////////

    /*! ************************************************************************************************/
    /*! \brief call PSN
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs5::callPSNInternal(HttpConnectionManagerPtr connMgr, const CommandInfo& cmdInfo,
        const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, PSNServices::PsnErrorResponse* rspErr, RawBuffer* rspRaw /*= nullptr*/)
    {
        if ((connMgr == nullptr) || EA_UNLIKELY(cmdInfo.componentInfo == nullptr) || EA_UNLIKELY(cmdInfo.restResourceInfo == nullptr))
        {
            ERR_LOG("[ExternalSessionUtilPs5].callPSNInternal: internal error: null connManagerPtr or invalid input command info. No op.");
            return ERR_SYSTEM;
        }
        const char8_t* contentType = (cmdInfo.restResourceInfo->contentType ? cmdInfo.restResourceInfo->contentType : connMgr->getDefaultContentType());
        const bool addEncodedPayload = cmdInfo.restResourceInfo->addEncodedPayload;

        HttpStatusCode statusCode = 0;
        BlazeRpcError err = RestProtocolUtil::sendHttpRequest(connMgr, cmdInfo.componentInfo->id, cmdInfo.commandId, &req, contentType, addEncodedPayload,
            rsp, rspErr, nullptr, nullptr, &statusCode, cmdInfo.componentInfo->baseInfo.index, RpcCallOptions(), rspRaw);
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief set up S2S connection to the PSN service.
        \param[in,out] connMgr the connection manager to init or refresh
        \param[in] httpServiceName Name of the http service config to use to setup the connection
    ***************************************************************************************************/
    void ExternalSessionUtilPs5::getNewWebApiConnMgr(HttpConnectionManagerPtr& connMgr, HttpServiceName httpServiceName)
    {
        if (gController == nullptr)
        {
            ERR_LOG("[ExternalSessionUtilPs5].getNewWebApiConnMgr: Internal error: Cannot init service(" << httpServiceName << "), gController null!");
            return;
        }
        HttpServiceConnectionMap::const_iterator itr = gController->getFrameworkConfigTdf().getHttpServiceConfig().getHttpServices().find(httpServiceName.c_str());
        const HttpServiceConnection* config = ((itr != gController->getFrameworkConfigTdf().getHttpServiceConfig().getHttpServices().end()) ? itr->second : nullptr);
        if (config == nullptr)
        {
            WARN_LOG("[ExternalSessionUtilPs5].getNewWebApiConnMgr: Cannot init service(" << httpServiceName << "), http service configuration missing.");
            return;
        }
        // init or refresh the cached conn mgr as needed
        bool isConfigChanged = ((connMgr != nullptr) && (config->getNumPooledConnections() != connMgr->maxConnectionsCount()));
        if ((connMgr == nullptr) || isConfigChanged)
        {
            const char8_t* hostname = nullptr;
            bool secure = false;
            HttpProtocolUtil::getHostnameFromConfig(config->getUrl(), hostname, secure);
            auto* newConnMgr = BLAZE_NEW OutboundHttpConnectionManager(hostname);
            newConnMgr->initialize(InetAddress(hostname), config->getNumPooledConnections(), secure, true, OutboundHttpConnectionManager::SSLVERSION_DEFAULT, config->getEncodingType());
            if (connMgr != nullptr)
            {
                connMgr.reset();
            }
            connMgr = HttpConnectionManagerPtr(newConnMgr);
            if (connMgr == nullptr)
            {
                ERR_LOG("[ExternalSessionUtilPs5].getNewWebApiConnMgr: Failed to get connection manager for component(" << httpServiceName << ").");
            }
        }
    }


}//GameManager
}//Blaze

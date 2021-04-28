/*! ************************************************************************************************/
/*!
    \file externalsessionutilxboxone.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "component/gamemanager/externalsessions/externalsessionutilxboxone.h"

#include "framework/logger.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/connection/outboundhttpservice.h"
#include "xblserviceconfigs/tdf/xblserviceconfigs.h"
#include "xblserviceconfigs/rpc/xblserviceconfigsslave.h"
#include "xblclientsessiondirectory/tdf/xblclientsessiondirectory.h"
#include "xblclientsessiondirectory/rpc/xblclientsessiondirectoryslave.h"
#include "xblprofile/tdf/xblprofile.h"
#include "xblprofile/rpc/xblprofileslave.h"
#include "framework/controller/controller.h" // for gController in getAuthToken
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/oauthslave.h"
#include "framework/tdf/oauth_server.h"
#include "component/gamemanager/rpc/gamemanager_defines.h"
#include "component/gamemanager/gamemanagerslaveimpl.h"
#include "component/gamemanager/externalsessions/externalsessionscommoninfo.h" // for gController->isMockPlatformEnabled in getAuthTokenInternal
#include "component/gamemanager/externalsessions/externaluserscommoninfo.h" //for toExtUserLogStr()
#include "xbltournamentshub/rpc/xbltournamentshubslave.h"
#include "xbltournamentshub/tdf/xbltournamentshub.h"

namespace Blaze
{
    namespace GameManager
    {
        bool ExternalSessionUtilXboxOne::verifyParams(const ExternalSessionServerConfig& config, bool& titleIdIsHex,
            uint16_t maxMemberCount, bool validateSessionNames)
        {
            // set it to false, then update later as needed.
            titleIdIsHex = false;

            const char8_t* titleId = config.getExternalSessionTitle();
            if (titleId == nullptr || titleId[0] == '\0')
            {
                ERR_LOG( "[ExternalSessionUtilXboxOne::verifyParams] Missing string for titleId. The ExternalSessionService cannot be created." );
                return false;
            }
            else
            {
                // Verify that the title id is is either decimal or hexidecimal:
                uint32_t i = 0;
                if (titleId[0] == '0' && (titleId[1] == 'X' || titleId[1] == 'x'))
                {
                    i = 2;
                    titleIdIsHex = true;
                }

                for (; titleId[i] != '\0' ; ++i)
                {
                    char8_t c = titleId[i];
                    if (c < '0')
                    {
                        ERR_LOG( "[ExternalSessionUtilXboxOne::verifyParams] Invalid string for titleId ("<< titleId <<"). Title Ids must be decimal or hexadecimal values" );
                        return false;
                    }
                    else if (c > '9')
                    {
                        if ((c < 'A' || c > 'F') && (c < 'a' || c > 'f'))
                        {
                            ERR_LOG( "[ExternalSessionUtilXboxOne::verifyParams] Invalid string for titleId ("<< titleId <<"). Title Ids must be decimal or hexadecimal values" );
                            return false;
                        }
                        else
                            titleIdIsHex = true;
                    }
                }
            }

            if (config.getScid() == nullptr || config.getScid()[0] == '\0')
            {
                ERR_LOG( "[ExternalSessionUtilXboxOne::verifyParams] Missing string for scid. The ExternalSessionService cannot be created." );
                return false;
            }
            const XblSessionTemplateNameList& sessionTemplateNameList = config.getSessionTemplateNames();
            if (sessionTemplateNameList.empty())
            {
                ERR_LOG( "[ExternalSessionUtilXboxOne::verifyParams] The sessionTemplateNames list is empty. The ExternalSessionService cannot be created." );
                return false;
            }

            XblSessionTemplateNameList::const_iterator iter, end;
            iter = sessionTemplateNameList.begin();
            end = sessionTemplateNameList.end();
            for (; iter != end; ++iter)
            {
                const XblSessionTemplateName& sessionTemplateName = *iter;
                if ((sessionTemplateName.c_str())[0] == '\0')
                {
                    ERR_LOG( "[ExternalSessionUtilXboxOne::verifyParams] Invalid string in sessionTemplateNames list. The ExternalSessionService cannot be created." );
                    return false;
                }
            }

            // (Don't verify disabled contract version.)
            int64_t contractVersionInt = 0;
            if (!isDisabled(config.getContractVersion()) && ((blaze_str2int(config.getContractVersion(), &contractVersionInt) == config.getContractVersion()) ||
                (contractVersionInt != 107)))
            {
                ERR_LOG("[ExternalSessionUtilXboxOne::verifyParams] MPSD contract version '" << config.getContractVersion() << "' specified, invalid/unsupported.");
                return false;
            }

            // handles contract version must be at least 105 (note: may differ from external sessions contractVersion).
            const char8_t* handlesContractVersion = config.getExternalSessionHandlesContractVersion();
            int64_t handlesContractVersionInt = 0;
            if (!isHandlesDisabled(handlesContractVersion) && ((blaze_str2int(handlesContractVersion, &handlesContractVersionInt) == handlesContractVersion) ||
                (handlesContractVersionInt < 105)))
            {
                ERR_LOG("[ExternalSessionUtilXboxOne::verifyParams] handles contract version '" << handlesContractVersion << "' specified, invalid/unsupported.");
                return false;
            }
            // primary external session types is expected enabled if handles enabled, in order for Blaze to manage presence.
            if (!isHandlesDisabled(handlesContractVersion) && config.getPrimaryExternalSessionTypeForUx().empty())
            {
                WARN_LOG("[ExternalSessionUtilXboxOne::verifyParams] handles enabled with contract version '" << handlesContractVersion <<
                    "' specified, however primary external session types was disabled. The external session service will not update primary sessions.");
            }
            const char8_t* inviteProtocol = config.getExternalSessionInviteProtocol();
            if ((inviteProtocol != nullptr) && (inviteProtocol[0] != '\0') && (blaze_strcmp("game", inviteProtocol) != 0))
            {
                ERR_LOG("[ExternalSessionUtilXboxOne::verifyParams] invalid inviteProtocol '" << inviteProtocol <<
                    "' specified. Valid values are 'game', or empty string. The external session service cannot be created." );
                return false;
            }

            // validate params vs title's MPSD service configuration
            if (!verifyXdpConfiguration(config.getScid(), sessionTemplateNameList, config.getContractVersion(), inviteProtocol, maxMemberCount, config.getCallOptions()))
                return false;//logged
 
            // ensure MPS names within XBL's max len
            if (validateSessionNames)
            {
                // This is dumb.   Just check the size of the strings: 
                uint32_t serviceNameLen = (gController != nullptr) ? strlen(gController->getDefaultServiceName()) : 0;

                // "%s%s_%u_%u_%020"
                uint32_t gameSessionMaxNameSize = strlen(config.getExternalSessionNamePrefix()) + serviceNameLen + 1 + 10 + 1 + 10 + 1 + 20 + 1;
                // "%s%s_%u_%020"
                uint32_t mmSessionMaxNameSize = strlen(config.getExternalSessionNamePrefix()) + serviceNameLen + 1 + 10 + 1 + 20 + 1;

                if (gameSessionMaxNameSize > config.getExternalSessionNameMaxLength())
                {
                    ERR_LOG("[ExternalSessionUtilXboxOne].verifyParams: game MPS names, have length(" <<
                        gameSessionMaxNameSize << "), max allowed(" << config.getExternalSessionNameMaxLength() << "). Configured externalSessionNamePrefix(" <<
                        config.getExternalSessionNamePrefix() << ").");
                    return false;
                }

                if (mmSessionMaxNameSize > config.getExternalSessionNameMaxLength())
                {
                    ERR_LOG("[ExternalSessionUtilXboxOne].verifyParams: MM MPS names, have length(" <<
                        mmSessionMaxNameSize << "), max allowed(" << config.getExternalSessionNameMaxLength() << "). Configured externalSessionNamePrefix(" <<
                        config.getExternalSessionNamePrefix() << ").");
                    return false;
                }
            }

            return true;
        }

        /*! ************************************************************************************************/
        /*! \brief retrieve xbl delegate token for the xblId's external session updates.
            Pre: authentication component available
            \param[out] buf - holds the token on success. If xblId is for an 'external user', this will be empty.
                Callers are expected to check for empty token as needed before attempting xbl calls.
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::getAuthToken(const UserIdentification& ident, const char8_t* serviceName, eastl::string& buf)
        {
            return getAuthTokenInternal(ident.getPlatformInfo().getExternalIds().getXblAccountId(), serviceName, buf, false);
        }

        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::getAuthTokenInternal(ExternalXblAccountId xblId, const char8_t* serviceName, eastl::string& buf, bool forceRefresh)
        {
            if (isDisabled() || (xblId == INVALID_EXTERNAL_ID))
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].getAuthToken not retrieving token for user " << xblId << (isDisabled()? ". External sessions disabled." : ""));
                return ERR_OK;
            }
            BlazeRpcError err = Blaze::ERR_OK;

            // we send off a request to nucleus via auth component for delegate token
            Blaze::OAuth::OAuthSlave* oAuthSlave = static_cast<Blaze::OAuth::OAuthSlave*>(gController->getComponent(
                Blaze::OAuth::OAuthSlave::COMPONENT_ID, false, true, &err));
            if (oAuthSlave == nullptr)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].getAuthToken - Unable to resolve auth component, error " << ErrorHelp::getErrorName(err));
                return err;
            }

            Blaze::OAuth::GetUserXblTokenRequest req;
            Blaze::OAuth::GetUserXblTokenResponse rsp;
            req.getRetrieveUsing().setExternalId(xblId);
            req.setForceRefresh(forceRefresh);

            // in case we're fetching someone else's token, nucleus requires super user privileges
            UserSession::SuperUserPermissionAutoPtr permissionPtr(true);

            err = oAuthSlave->getUserXblToken(req, rsp);
            if (err == ERR_OK)
            {
                buf.append(rsp.getXblToken());
                TRACE_LOG("[ExternalSessionUtilXboxOne].getAuthToken: Got first party auth token for player(xblId:" << xblId << ", serviceName:" << ((serviceName != nullptr)? serviceName : "<nullptr>") << "): '" << rsp.getXblToken() << "'");
            }
            else if (err == OAUTH_ERR_NO_XBLTOKEN)
            {
                // On Xbox 'external users', those without an EA account (and thus no XBL token info available) may exist. We may only
                // know they're an external user however after the above nucleus call returns an error, and can only assume such failure
                // means its an external user. To avoid spam don't log error. Callers are expected to check for empty token before attempting xbl calls.
                TRACE_LOG("[ExternalSessionUtilXboxOne].getAuthToken: Failed to get first party auth token for player(xblId:" << xblId << ", serviceName:" << ((serviceName != nullptr)? serviceName : "<nullptr>") << "), reason " << ErrorHelp::getErrorName(err) << ". Assuming this is due to user being an external non-EA user.");
                err = ERR_OK;
            }

            if (isMockPlatformEnabled())
            {
                // Set xblId as the token to allow mock service to identify the caller by token. To avoid blocking tests, we'll also ignore any nucleus errors (already logged)
                buf.sprintf("%" PRIu64, xblId);
                err = ERR_OK;
            }
            return err;
        }

        /*! ************************************************************************************************/
        /*! \brief create the external session in MPSD.
            \param[in] commit if we only want to fetch the correlation id, for the would-be MPS, set this
                false to do a 'trial swing' call that returns the MPS result but does not actually commit MPS to MPSD.
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::create(const CreateExternalSessionParameters& params, Blaze::ExternalSessionResult* result, bool commit)
        {
            const char8_t* sessionTemplateName = params.getXblSessionTemplateName();
            const char8_t* sessionName = params.getXblSessionName();

            // (note on Xbox will always also join the creator)
            return createInternal(sessionTemplateName, sessionName, params.getExternalSessionCreationInfo(), params.getExternalUserJoinInfo(), nullptr, result, commit);
        }

        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::createInternal(const char8_t* sessionTemplateName,
            const char8_t* sessionName, const ExternalSessionCreationInfo& params, const ExternalUserJoinInfo& userJoinInfo,
            const XBLServices::ArenaTeamParamsList* teamParams, Blaze::ExternalSessionResult* result, bool commit)
        {
            uint64_t externalSessionId = params.getExternalSessionId();
            ExternalSessionType externalSessionType = params.getExternalSessionType();
            eastl::string custData((char8_t*)params.getExternalSessionCustomData().getData(), params.getExternalSessionCustomData().getCount());
            ExternalXblAccountId xblId = userJoinInfo.getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId();
            const char8_t* authToken = userJoinInfo.getAuthInfo().getCachedExternalSessionToken();

            if (isDisabled() || (xblId == INVALID_XBL_ACCOUNT_ID) || (authToken == nullptr) || (authToken[0] == '\0'))
            {
                // non-Xbox platform users for instance commanders in commander mode games, won't update external sessions.
                TRACE_LOG("[ExternalSessionUtilXboxOne].createInternal: " << (isDisabled() ? "External sessions disabled" : eastl::string().sprintf("No caller who can create. Input xuid(%" PRIu64 ")%s.", xblId, ((authToken == nullptr || authToken[0] == '\0') ? " missing first party auth token, possibly due to no external session join permisson or error in connection to external service" : "")).c_str()));
                return ERR_OK;
            }

            if (!validateSessionTemplateName(sessionTemplateName))
            {
                return XBLSERVICECONFIGS_SESSION_TEMPLATE_NOT_SUPPORTED;
            }
            if (commit)
                validateGameSettings(params.getUpdateInfo().getSettings());

            bool isArenaGame = (teamParams != nullptr) && !teamParams->empty();

            Blaze::XBLServices::PutMultiplayerSessionCreateRequest putRequest;
            putRequest.setScid(mConfig.getScid());
            putRequest.setSessionTemplateName(sessionTemplateName);
            putRequest.setSessionName(sessionName);
            putRequest.setNoCommit(commit? "false" : "true");
            putRequest.getHeader().setAuthToken(authToken);
            putRequest.getHeader().setContractVersion(mConfig.getContractVersion());
            putRequest.getHeader().setOnBehalfOfTitle(mTitleId.c_str());
            if (isMockPlatformEnabled() && externalSessionType != EXTERNAL_SESSION_GAME)
                putRequest.getHeader().setSimOutage("false");

            putRequest.getBody().getConstants().getCustom().setExternalSessionId(eastl::string().sprintf("%" PRIu64, externalSessionId).c_str());
            putRequest.getBody().getConstants().getCustom().setExternalSessionType(externalSessionType);
            putRequest.getBody().getConstants().getCustom().setExternalSessionCustomDataString(custData.c_str());
            putRequest.getBody().getConstants().getSystem().setReservedRemovalTimeout(mReservationTimeout.getMillis());
            putRequest.getBody().getConstants().getSystem().setInactiveRemovalTimeout(mConfig.getExternalSessionInactivityTimeout().getMillis());
            putRequest.getBody().getConstants().getSystem().setReadyRemovalTimeout(mConfig.getExternalSessionReadyTimeout().getMillis());
            putRequest.getBody().getConstants().getSystem().setSessionEmptyTimeout( mConfig.getExternalSessionEmptyTimeout().getMillis());
            if (!isArenaGame && !mInviteProtocol.empty())
            {
                //Blaze can now simply omit sending inviteProtocol (trackChanges true). Equivalent to setting to null, by MS spec
                putRequest.getBody().getConstants().getSystem().setInviteProtocol(mInviteProtocol.c_str());
            }
            putRequest.getBody().getConstants().getSystem().setVisibility(toVisibility(params.getUpdateInfo()));
            putRequest.getBody().getConstants().getSystem().getCapabilities().setGameplay(externalSessionType == EXTERNAL_SESSION_GAME);
            if (toCrossPlay(params.getUpdateInfo()))
                putRequest.getBody().getConstants().getSystem().getCapabilities().setCrossPlay(true);
            if (toXblLarge(params.getUpdateInfo()))
            {
                // Large MPS: To avoid poss full errs, explicitly specify maxMembersCount to be safely above Games' max poss cap, o.w.
                // it'd default to only 100 (again maxMembersCount won't show in UX and Blaze just uses 'closed' flag to disable joins).
                // Specifying it here not in MS's session-template keeps things in one place and avoids errors.
                putRequest.getBody().getConstants().getSystem().setMaxMembersCount(mConfig.getXblMaxLargeMembers());
                putRequest.getBody().getConstants().getSystem().getCapabilities().setLarge(true);
            }
            putRequest.getBody().getProperties().getSystem().setClosed(toClosed(params.getUpdateInfo()));
            putRequest.getBody().getProperties().getSystem().setJoinRestriction(toJoinRestriction(params.getUpdateInfo()));
            // Side: No need to handle poss 'reserve_' members here as Blaze/MS requires creators be non-reserved in the MPS, see join().
            putRequest.getBody().getMembers()["me"] = putRequest.getBody().getMembers().allocate_element();
            putRequest.getBody().getMembers()["me"]->getConstants().getSystem().setInitialize(true); // always use managed initialization
            putRequest.getBody().getMembers()["me"]->getProperties().getSystem().setActive(true); // always explicitly transition 'me' (non-reserved) to active

            if (isArenaGame)
            {
                BlazeRpcError arenaErr = prepareArenaParams(putRequest, params, teamParams, userJoinInfo);
                if (arenaErr != ERR_OK)
                {
                    return arenaErr;//logged
                }
            }
            if (isRecentPlayersBlazeManaged(params.getUpdateInfo()))
            {
                eastl::string groupidBuf;
                putRequest.getBody().getMembers()["me"]->getProperties().getSystem().getGroups().push_back().set(toXblRecentPlayerGroupId(groupidBuf, params.getUpdateInfo(), userJoinInfo.getTeamIndex()).c_str());
            }
            TRACE_LOG("[ExternalSessionUtilXboxOne].create: " << (commit? "" : "simulate ") << "creating new MPS(" << ExternalSessionTypeToString(externalSessionType) << ", id=" << externalSessionId << ", sessionName: " << sessionName <<
                ", Visibility=" << toVisibility(params.getUpdateInfo()) << ", Closed=" << toClosed(params.getUpdateInfo()) << ", JoinRestriction=" << toJoinRestriction(params.getUpdateInfo()) << ", customData=" << custData.c_str() << ") with xuid " << xblId << ". Scid: " << mConfig.getScid() << ", SessionTemplateName: " << sessionTemplateName << ".");

            Blaze::XBLServices::MultiplayerSessionResponse putResponse;

            Blaze::XBLServices::XBLServiceConfigsSlave * xblServiceConfigsSlave = (Blaze::XBLServices::XBLServiceConfigsSlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLServices::XBLServiceConfigsSlave::COMPONENT_INFO.name);
            if (xblServiceConfigsSlave == nullptr)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].create: Failed to instantiate XBLServiceConfigsSlave object. sessionName: " << sessionName  << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid());
                return ERR_SYSTEM;
            }

            Blaze::XBLServices::MultiplayerSessionErrorResponse errorTdf;
            Blaze::RawBufferPtr rspRaw = BLAZE_NEW RawBuffer(0);
            Blaze::BlazeRpcError err = xblServiceConfigsSlave->putMultiplayerSessionCreate(putRequest, putResponse, errorTdf, RpcCallOptions(), rspRaw.get());
            if (err == XBLSERVICECONFIGS_AUTHENTICATION_REQUIRED)
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].create: Authentication with Microsoft failed, token may have expired. Forcing token refresh and retrying.");
                eastl::string buf;
                err = getAuthTokenInternal(xblId, userJoinInfo.getAuthInfo().getServiceName(), buf, true);
                if (err == ERR_OK)//side: assume no need guarding vs empty token here as already know we're not external user (checked atop)
                {
                    putRequest.getHeader().setAuthToken(buf.c_str());
                    rspRaw->reset();
                    err = xblServiceConfigsSlave->putMultiplayerSessionCreate(putRequest, putResponse, errorTdf, RpcCallOptions(), rspRaw.get());
                }
            }
            for (size_t i = 0; (isRetryError(err) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
            {
                if (ERR_OK != (err = waitSeconds(errorTdf.getRetryAfter(), "[ExternalSessionUtilXboxOne].create", i + 1)))
                    return err;
                rspRaw->reset();
                err = xblServiceConfigsSlave->putMultiplayerSessionCreate(putRequest, putResponse, errorTdf, RpcCallOptions(), rspRaw.get());
            }
            if (isMpsAlreadyExistsError(err))
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].create: Failed to create external session as it may already exist. sessionName: " << sessionName << ", titleId: " << mTitleId.c_str() << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid() << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ")");
            }
            else if (err != ERR_OK)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].create: Failed to create external session. sessionName: " << sessionName << ", titleId: " << mTitleId.c_str() << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid()
                    << ", externalSessionId: " << externalSessionId << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err)
                    << "). Raw response: " << eastl::string(reinterpret_cast<const char8_t*>(rspRaw->data()), rspRaw->datasize()).c_str());
            }
            else if (result != nullptr)
            {
                // save result
                result->getSessionIdentification().getXone().setCorrelationId(putResponse.getCorrelationId());
                result->getSessionIdentification().getXone().setTemplateName(sessionTemplateName);
                result->getSessionIdentification().getXone().setSessionName(sessionName);

                TRACE_LOG("[ExternalSessionUtilXboxOne].create: " << (commit ? "" : "simulate ") << "created new MPS(" << ExternalSessionTypeToString(externalSessionType) << ", id=" << externalSessionId <<
                    ", sessionName:" << sessionName << ", correlationId: " << putResponse.getCorrelationId() << ".");

            }
            return err;
        }

        /*! ************************************************************************************************/
        /*! \brief create the external session in MPSD.
            \param[in] externalUserJoinInfos - users to join to the external session. Pre: For Xbox each active user uses its own cached delegate token.
            \param[in] groupAuthToken - if non nullptr, uses this cached delegate token for joining reserved members.
            \param[in] commit false does a 'trial swing' call that returns the join attempt result but does not actually commit to MPSD. To support
                 Blaze spec, true will bypass MPS's JoinRestrictions.
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::join(const JoinExternalSessionParameters& params, Blaze::ExternalSessionResult* result, bool commit)
        {
            const char8_t* sessionTemplateName = params.getSessionIdentification().getXone().getTemplateName();
            const char8_t* sessionName = params.getSessionIdentification().getXone().getSessionName();
            uint64_t externalSessionId = params.getExternalSessionCreationInfo().getExternalSessionId();
            ExternalSessionType externalSessionType = params.getExternalSessionCreationInfo().getExternalSessionType();
            const char8_t* groupAuthToken = params.getGroupExternalSessionToken();
            const ExternalUserJoinInfoList& externalUserJoinInfos = params.getExternalUserJoinInfos();

            bool foundUser = false;
            for (auto& userJoinInfo : externalUserJoinInfos)
            {
                // Only attempt to join with XBL users.  Other users, or players without valid XBL ids are ignored
                if (userJoinInfo->getUserIdentification().getPlatformInfo().getClientPlatform() == mPlatform &&
                    userJoinInfo->getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
                    foundUser = true;
            }
            if (!foundUser)
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].join: None of the "<< externalUserJoinInfos.size() << " users in the join request had a valid XBL account id. No MPS will be joined for externalSessionId: '" << externalSessionId << "', externalSessionType: " << ExternalSessionTypeToString(externalSessionType) << ", sessionName: '" << sessionName << "'");
                return ERR_OK;
            }


            if (isDisabled() || externalUserJoinInfos.empty())
            {
                if (isDisabled())
                {
                    TRACE_LOG("[ExternalSessionUtilXboxOne].join: External sessions explicitly disabled. No MPS will be joined for externalSessionId: '" << externalSessionId << "', externalSessionType: " << ExternalSessionTypeToString(externalSessionType) << ", sessionName: '" << sessionName << "'");
                }
                return ERR_OK;
            }

            Blaze::XBLServices::XBLServiceConfigsSlave * xblServiceConfigsSlave = (Blaze::XBLServices::XBLServiceConfigsSlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLServices::XBLServiceConfigsSlave::COMPONENT_INFO.name);
            if (xblServiceConfigsSlave == nullptr)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].join: Failed to instantiate XBLServiceConfigsSlave object. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid());
                return ERR_SYSTEM;
            }

            if (!validateSessionTemplateName(sessionTemplateName))
            {
                return XBLSERVICECONFIGS_SESSION_TEMPLATE_NOT_SUPPORTED;
            }
            if (sessionName[0] == '\0')
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].join: Invalid empty session name specified. Cannot join or create external session. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid());
                return ERR_SYSTEM;
            }

            // check if MPS already exists, use the existing constants if so.
            XBLServices::MultiplayerSessionResponse mps;
            BlazeRpcError geterr = getExternalSession(sessionTemplateName, sessionName, mps);
            if ((geterr != ERR_OK) && (geterr != XBLCLIENTSESSIONDIRECTORY_NO_CONTENT))
            {
                // error is already logged in getExternalSession()
                return geterr;
            }
            bool mpsExists = (geterr == ERR_OK);

            // check if Large-MPS game. If MPS exists, just check the MPS, else check Blaze's create params
            bool isLarge = (mpsExists ? mps.getConstants().getSystem().getCapabilities().getLarge() : toXblLarge(params.getExternalSessionCreationInfo().getUpdateInfo()));

            // check if tournament game. If MPS exists, just check the MPS, else check for tournament id in create req
            bool isArenaGame = (mpsExists ? (mps.getConstants().getCustom().getTournamentId()[0] != '\0') :
                (params.getExternalSessionCreationInfo().getTournamentIdentification().getTournamentId()[0] != '\0'));

            // check user's teams for the tournament, as needed
            XBLServices::ArenaTeamParamsList foundTeams;
            if (isArenaGame && !validateTeamsRegistered(foundTeams, externalUserJoinInfos, params.getExternalSessionCreationInfo().getTournamentIdentification()))
            {
                WARN_LOG("[ExternalSessionUtilXboxOne].join: Failed to find Arena teams for users for tournament(" << params.getExternalSessionCreationInfo().getTournamentIdentification().getTournamentId() << "), while joining game MPS(" << sessionName << ", template:" << sessionTemplateName << ", gameId:" << externalSessionId << "). Game may not be properly recorded in first party as Arena game");
                isArenaGame = false;
                foundTeams.clear();
            }

            // common settings for both active and reserved players requests
            Blaze::XBLServices::MultiplayerSessionResponse putResponse;
            Blaze::XBLServices::PutMultiplayerSessionJoinRequest putRequest;
            putRequest.setScid(mConfig.getScid());
            putRequest.setSessionTemplateName(sessionTemplateName);
            putRequest.setSessionName(sessionName);
            putRequest.setNoCommit(commit? "false" : "true");
            putRequest.getHeader().setContractVersion(mConfig.getContractVersion());
            putRequest.getHeader().setOnBehalfOfTitle(mTitleId.c_str());
            putRequest.getHeader().setDenyScope(commit? "" : "Multiplayer.Manage"); // to check JoinRestrictions opt out Multiplayer.Manage
            putRequest.getHeader().setAcceptLog("session-write/*"); // for mpsJoinErrorToSpecificError use
            if (isMockPlatformEnabled() && externalSessionType != EXTERNAL_SESSION_GAME)
                putRequest.getHeader().setSimOutage("false");

            TRACE_LOG("[ExternalSessionUtilXboxOne].join: " << (commit? "" : "simulate ") << (mpsExists? "" : "creating and ") << "joining to MPS(" << ExternalSessionTypeToString(externalSessionType) << ", id=" << externalSessionId << ") for " << externalUserJoinInfos.size() << " users " << (externalUserJoinInfos.empty()? "" : eastl::string().sprintf("%" PRIu64 ".", externalUserJoinInfos.front()->getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId()).c_str()) << (!mpsExists? " Non-existent MPS, requires creation" : "") << ". sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", :Scid " << mConfig.getScid() << ".");
            
            Blaze::XBLServices::PutMultiplayerSessionJoinReserveRequest reserveRequest;

            eastl::string indexBuf;
            eastl::string xuidBuf;
            eastl::string groupidBuf;

            // Loop over input list, joining the active users, while setting up a list of reserved users for a single reservations call below.
            uint32_t reservedUsersCount = 0;
            BlazeRpcError err = ERR_OK;
            ExternalUserJoinInfoList cleanupList;
            putRequest.getBody().getMembers()["me"] = putRequest.getBody().getMembers().allocate_element();
            putRequest.getBody().getMembers()["me"]->getConstants().getSystem().setInitialize(true); // always use managed initialization
            putRequest.getBody().getMembers()["me"]->getProperties().getSystem().setActive(true); // always explicitly transition 'me' (non-reserved) to active
            for (ExternalUserJoinInfoList::const_iterator itr = externalUserJoinInfos.begin(); itr != externalUserJoinInfos.end(); ++itr)
            {
                if (EA_UNLIKELY((*itr)->getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId() == INVALID_XBL_ACCOUNT_ID))
                    continue;
                if (isLarge && (*itr)->getReserved()) //Large MPSs by MS spec must omit reserved players
                    continue;

                if (!mpsExists)
                {
                    // note by MPSD spec the first user in who creates and sets initial properties must be active here. Don't switch it to reserved. It should claim the reservation immediately anyhow (and if not, its timeout removal from game will clean it from MPS too).
                    err = createInternal(sessionTemplateName, sessionName, params.getExternalSessionCreationInfo(), *(*itr), &foundTeams, result, commit);
                    if ((err != ERR_OK) && !isMpsAlreadyExistsError(err))
                        break;//logged
                    mpsExists = true;

                    if (!isMpsAlreadyExistsError(err))// Edge case: Two 'create-or-join' game callers may try creating MPS simultaneously here. Only one can succeed, the other should fall through and join MPS below.
                        continue;
                }

                // Do reserved player. append for group join request below. By MS spec, for Arena, joiners get specified like *non-reserved*
                if ((*itr)->getReserved() && !isArenaGame)
                {
                    const char8_t *memberIndexStr = indexBuf.sprintf("reserve_%u", reservedUsersCount).c_str();
                    reserveRequest.getBody().getMembers()[memberIndexStr] = reserveRequest.getBody().getMembers().allocate_element();
                    reserveRequest.getBody().getMembers()[memberIndexStr]->getConstants().getSystem().setXuid(xuidBuf.sprintf("%" PRIu64, (*itr)->getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId()).c_str());
                    reserveRequest.getBody().getMembers()[memberIndexStr]->getConstants().getSystem().setInitialize(true); // always use managed initialization
                    reservedUsersCount++;
                    continue;
                }

                // Do non-reserved player. Join it to external session

                // Each active join must use its own delegate token here (cannot join others as active)
                putRequest.getHeader().setAuthToken((*itr)->getAuthInfo().getCachedExternalSessionToken());

                // add Arena team
                if (isArenaGame)
                {
                    const XBLServices::ArenaTeamParams* team = findUserTeamInList((*itr)->getUserIdentification(), foundTeams);
                    if (team != nullptr)
                    {
                        putRequest.getBody().getMembers()["me"]->getConstants().getSystem().setTeam(team->getUniqueName());
                    }
                }
                // add Large MPS recent player group
                if (isRecentPlayersBlazeManaged(params.getExternalSessionCreationInfo().getUpdateInfo()))
                {
                    putRequest.getBody().getMembers()["me"]->getProperties().getSystem().getGroups().clear();
                    putRequest.getBody().getMembers()["me"]->getProperties().getSystem().getGroups().push_back().set(toXblRecentPlayerGroupId(groupidBuf, params.getExternalSessionCreationInfo().getUpdateInfo(), (*itr)->getTeamIndex()).c_str());
                }

                Blaze::XBLServices::MultiplayerSessionErrorResponse errorTdf;
                Blaze::RawBufferPtr rspRaw = BLAZE_NEW RawBuffer(0);
                err = xblServiceConfigsSlave->putMultiplayerSessionJoin(putRequest, putResponse, errorTdf, RpcCallOptions(), rspRaw.get());
                if (err == XBLSERVICECONFIGS_AUTHENTICATION_REQUIRED)
                {
                    TRACE_LOG("[ExternalSessionUtilXboxOne].join: Authentication with Microsoft failed, token may have expired. Forcing token refresh and retrying.");
                    eastl::string buf;
                    err = getAuthTokenInternal((*itr)->getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId(), (*itr)->getAuthInfo().getServiceName(), buf, true);
                    if (err == ERR_OK)
                    {
                        putRequest.getHeader().setAuthToken(buf.c_str());
                        rspRaw->reset();
                        err = xblServiceConfigsSlave->putMultiplayerSessionJoin(putRequest, putResponse, errorTdf, RpcCallOptions(), rspRaw.get());
                    }
                }
                for (size_t i = 0; (isRetryError(err) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
                {
                    if (ERR_OK != (err = waitSeconds(errorTdf.getRetryAfter(), "[ExternalSessionUtilXboxOne].join", i + 1)))
                        return err;
                    rspRaw->reset();
                    err = xblServiceConfigsSlave->putMultiplayerSessionJoin(putRequest, putResponse, errorTdf, RpcCallOptions(), rspRaw.get());
                }
                if (err != ERR_OK)
                {
                    err = mpsJoinErrorToSpecificError(err, errorTdf, mps);
                    if (commit)
                    {
                        ERR_LOG("[ExternalSessionUtilXboxOne].join: Failed to join active player " << (*itr)->getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId() << " external session. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName
                            << ", titleId: " << mTitleId.c_str() << ", Scid: " << mConfig.getScid() << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err)
                            << "). Raw response: " << eastl::string(reinterpret_cast<const char8_t*>(rspRaw->data()), rspRaw->datasize()).c_str());
                        for (ExternalUserJoinInfoList::const_iterator itr2 = externalUserJoinInfos.begin(); itr2 != itr; ++itr2)
                            (*itr2)->copyInto(*cleanupList.pull_back());
                    }
                    break;
                }

                // save off session template name and session's correlation id if haven't yet
                if (result != nullptr)
                {
                    result->getSessionIdentification().getXone().setTemplateName(sessionTemplateName);
                    result->getSessionIdentification().getXone().setSessionName(sessionName);
                    if (result->getSessionIdentification().getXone().getCorrelationId()[0] == '\0')
                        result->getSessionIdentification().getXone().setCorrelationId(putResponse.getCorrelationId());
                }
            }

            if (!cleanupList.empty())
                scheduleCleanupAfterFailedJoin(cleanupList, sessionName, sessionTemplateName);

            // Do reserved players' group join.
            // Upsert: If was already-exists, join. Note: by MS spec requesting member state be reserved here, is safe from overwriting an existing active state (retains active).
            if ((err == Blaze::ERR_OK || isMpsAlreadyExistsError(err)) && (reservedUsersCount > 0))
            {
                if (EA_UNLIKELY((groupAuthToken == nullptr) || (groupAuthToken[0] == '\0') || !mpsExists))
                {
                    ERR_LOG("[ExternalSessionUtilXboxOne].join: Cannot do external session reservations without an " << (mpsExists? "external token" : "active player creating the MPS") << ". sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid());
                    return ERR_SYSTEM;
                }

                // MPSD can use a single token for adding all reserved users together
                reserveRequest.getHeader().setAuthToken(groupAuthToken);
                reserveRequest.getHeader().setContractVersion(mConfig.getContractVersion());
                reserveRequest.getHeader().setOnBehalfOfTitle(mTitleId.c_str());
                reserveRequest.getHeader().setDenyScope(commit ? "" : "Multiplayer.Manage"); // to check JoinRestrictions opt out Multiplayer.Manage
                reserveRequest.getHeader().setAcceptLog("session-write/*"); // for mpsJoinErrorToSpecificError use
                reserveRequest.setScid(mConfig.getScid());
                reserveRequest.setSessionTemplateName(sessionTemplateName);
                reserveRequest.setSessionName(sessionName);
                reserveRequest.setNoCommit(commit? "false" : "true");

                Blaze::XBLServices::MultiplayerSessionResponse reserveResponse;
                Blaze::XBLServices::MultiplayerSessionErrorResponse errorTdf;
                
                err = xblServiceConfigsSlave->putMultiplayerSessionJoinReserve(reserveRequest, reserveResponse, errorTdf);
                if (err == XBLSERVICECONFIGS_AUTHENTICATION_REQUIRED)
                {
                    TRACE_LOG("[ExternalSessionUtilXboxOne].join: Authentication with Microsoft failed, token may have expired. Forcing token refresh and retrying.");
                    // Its rare we're here, for simplicity just try arbitrary any one's token
                    const ExternalUserJoinInfo* groupTokenRetrier = externalUserJoinInfos.front();
                    ExternalXblAccountId xblId = groupTokenRetrier->getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId();
                    const char8_t* servName = groupTokenRetrier->getAuthInfo().getServiceName();
                    if (gUserSessionManager->getSessionExists(UserSession::getCurrentUserSessionId()))
                    {
                        // However, if we have a calling session, use that instead.
                        // This allows the calling user joinGameByUserList to get their token for adding a list
                        // of players not in nucleus.
                        ServiceName currentServiceName;
                        gUserSessionManager->getServiceName(UserSession::getCurrentUserSessionId(), currentServiceName);
                        xblId = gUserSessionManager->getPlatformInfo(UserSession::getCurrentUserSessionId()).getExternalIds().getXblAccountId();
                        servName = currentServiceName.c_str();
                    }
                    eastl::string buf;
                    err = getAuthTokenInternal(xblId, servName, buf, true);
                    if (err == ERR_OK)
                    {
                        reserveRequest.getHeader().setAuthToken(buf.c_str());
                        err = xblServiceConfigsSlave->putMultiplayerSessionJoinReserve(reserveRequest, reserveResponse, errorTdf);
                    }
                }
                for (size_t i = 0; (isRetryError(err) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
                {
                    if (ERR_OK != (err = waitSeconds(errorTdf.getRetryAfter(), "[ExternalSessionUtilXboxOne].join: for reserved players", i + 1)))
                        return err;
                    err = xblServiceConfigsSlave->putMultiplayerSessionJoinReserve(reserveRequest, reserveResponse, errorTdf);
                }

                // save off session's correlation id if haven't yet
                if (result != nullptr)
                {
                    result->getSessionIdentification().getXone().setTemplateName(sessionTemplateName);
                    result->getSessionIdentification().getXone().setSessionName(sessionName);
                    if (result->getSessionIdentification().getXone().getCorrelationId()[0] == '\0')
                        result->getSessionIdentification().getXone().setCorrelationId(reserveResponse.getCorrelationId());
                }
                err = mpsJoinErrorToSpecificError(err, errorTdf, mps);
            }
            if ((err != ERR_OK) && commit)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].join: Failed to join external session. sessionName: " << sessionName << ", titleId: " << mTitleId.c_str() << ", SessionTemplateName: " << sessionTemplateName << ", :Scid " << mConfig.getScid() << ", externalSessionId: " << externalSessionId << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ")");
            }
            return err;
        }

        /*! ************************************************************************************************/
        /*! \brief translate MPSD http errors to granular Blaze errors based on the response.
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::mpsJoinErrorToSpecificError(Blaze::BlazeRpcError origErr,
            const XBLServices::MultiplayerSessionErrorResponse& errRsp, const XBLServices::MultiplayerSessionResponse& mps)
        {
            Blaze::BlazeRpcError err = origErr;
            if (origErr == XBLSERVICECONFIGS_ACCESS_FORBIDDEN)
            {
                if (errRsp.getXblLog()[0] != '\0')
                {
                    // Use response's Xbl-log MS provided EA (in GOS-27862) to determine reason for http 403. Pre: req header's AcceptLog was specified.
                    if (blaze_stristr(errRsp.getXblLog(), "reason=not-open;") != nullptr)
                        err = XBLSERVICECONFIGS_EXTERNALSESSION_VISIBILITY_VISIBLE;
                    else if (blaze_stristr(errRsp.getXblLog(), "reason=not-followed;") != nullptr)
                        err = XBLSERVICECONFIGS_EXTERNALSESSION_JOINRESTRICTION_FOLLOWED;
                    else if (blaze_stristr(errRsp.getXblLog(), "reason=not-local;") != nullptr)
                        err = XBLSERVICECONFIGS_EXTERNALSESSION_JOINRESTRICTION_LOCAL;
                    else if (blaze_stristr(errRsp.getXblLog(), "reason=closed;") != nullptr)
                        err = XBLSERVICECONFIGS_EXTERNALSESSION_CLOSED;
                }
                else if (blaze_stricmp(mps.getConstants().getSystem().getVisibility(), "private") == 0)
                {
                    // a join fail due to Visibility private won't have Xbl-log. If got mps w/Visibility private here, can assume its due to that.
                    err = XBLSERVICECONFIGS_EXTERNALSESSION_VISIBILITY_PRIVATE;
                }
            }
            if (err != origErr)
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].mpsJoinErrorToSpecificError: MPSD original error " << Blaze::ErrorHelp::getErrorName(origErr) << "(http code " << Blaze::ErrorHelp::getHttpStatusCode(origErr) << ") to specific error " <<
                    Blaze::ErrorHelp::getErrorName(err) << ". XBL-log response details: " << (errRsp.getXblLog()[0] != '\0'? errRsp.getXblLog() : "(not available)") << ".");
            }
            return err;
        }

        /*! ************************************************************************************************/
        /*! \brief leave one or more users from the external session.
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::leave(const LeaveGroupExternalSessionParameters& params)
        {
            BlazeRpcError err = ERR_OK;
            const char8_t* sessionTemplateName = params.getSessionIdentification().getXone().getTemplateName();
            const char8_t* sessionName = params.getSessionIdentification().getXone().getSessionName();

            const Blaze::ExternalUserLeaveInfoList& externalUserLeaveInfoList = params.getExternalUserLeaveInfos();

            // Check to see if actually using MPS.
            if (isDisabled() || externalUserLeaveInfoList.empty())
            {
                return ERR_OK;
            }

            Blaze::XBLServices::XBLClientSessionDirectorySlave * xblClientSessionDirectorySlave = (Blaze::XBLServices::XBLClientSessionDirectorySlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLServices::XBLClientSessionDirectorySlave::COMPONENT_INFO.name);
            if (xblClientSessionDirectorySlave == nullptr)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].leave: Failed to instantiate XBLClientSessionDirectorySlave object.  sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid());
                return ERR_SYSTEM;
            }

            validateSessionTemplateName(sessionTemplateName);
            if (sessionTemplateName[0] == '\0')
            {
                return XBLSERVICECONFIGS_SESSION_TEMPLATE_NOT_SUPPORTED;
            }

            // *Large* sessions: by spec must leave 1 by 1:
            if (params.getXblLarge())
            {
                for (auto iter : externalUserLeaveInfoList)
                {
                    BlazeRpcError curErr = leaveUserFromLargeMps(iter->getUserIdentification(), params);
                    if (curErr != ERR_OK)
                    {
                        err = curErr; //logged. Poss eff enhancement could early out. Could also GET to check if already left and NOOP
                    }
                }
                return err;
            }
            // Else Non-Large: can bulk-leave:


            // First obtain the first party indexes
            XBLServices::MultiplayerSessionResponse getResponse;
            XBLServices::GetMultiplayerSessionRequest getRequest;
            getRequest.setScid(mConfig.getScid());
            getRequest.setSessionTemplateName(sessionTemplateName);
            getRequest.setSessionName(sessionName);
            getRequest.getHeader().setContractVersion(mConfig.getContractVersion());

            Blaze::XBLServices::MultiplayerSessionErrorResponse errorTdf;
            RawBufferPtr rspRaw = BLAZE_NEW RawBuffer(0);
            err = xblClientSessionDirectorySlave->getMultiplayerSession(getRequest, getResponse, errorTdf, RpcCallOptions(), rspRaw.get());
            for (size_t i = 0; (isRetryError(err) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
            {
                if (ERR_OK != (err = waitSeconds(errorTdf.getRetryAfter(), "[ExternalSessionUtilXboxOne].leave: at get MPS call", i + 1)))
                    return err;
                rspRaw->reset();
                err = xblClientSessionDirectorySlave->getMultiplayerSession(getRequest, getResponse, errorTdf, RpcCallOptions(), rspRaw.get());
            }

            if (err != ERR_OK)
            {
                if (err == XBLCLIENTSESSIONDIRECTORY_NO_CONTENT)
                {
                    // depending on calling flow, members might be already cleaned up.
                    TRACE_LOG("[ExternalSessionUtilXboxOne].leave: Failed to get session, " << externalUserLeaveInfoList.size() << " user(s) " << (externalUserLeaveInfoList.empty()? "" : eastl::string().sprintf("%" PRIu64 "", (*externalUserLeaveInfoList.begin())->getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId()).c_str()) << ". were already removed. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid() << ", return code: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ").");
                    return ERR_OK;
                }

                ERR_LOG("[ExternalSessionUtilXboxOne].leave: Failed to get session. Not removing " << externalUserLeaveInfoList.size() << " users " << (externalUserLeaveInfoList.empty()? "" : eastl::string().sprintf("%" PRIu64 "", (*externalUserLeaveInfoList.begin())->getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId()).c_str())
                    << ". sessionName: " << sessionName  << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid()  << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err)
                    << "). Raw response: " << eastl::string(reinterpret_cast<const char8_t*>(rspRaw->data()), rspRaw->datasize()).c_str());
                return err;
            }

            // cache off users into a map by their xuid for lookup of index when adding to the remove request.
            eastl::hash_map<const char8_t*, const char8_t*, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> xblXuidMap;

            XBLServices::Members::const_iterator memberIter = getResponse.getMembers().begin();
            XBLServices::Members::const_iterator memberEnd = getResponse.getMembers().end();
            for (; memberIter != memberEnd; ++memberIter)
            {
                const char8_t* index = (memberIter->first).c_str();
                const XBLServices::Member* member = memberIter->second;
                const char8_t* xuid = member->getConstants().getSystem().getXuid();
                xblXuidMap[xuid] = index;
            }

            // Now remove all players by their index
            XBLServices::MultiplayerSessionResponse putResponse;
            XBLServices::PutMultiplayerSessionLeaveGroupRequest putRequest;
            putRequest.setScid(mConfig.getScid());
            putRequest.setSessionTemplateName(sessionTemplateName);
            putRequest.setSessionName(sessionName);
            putRequest.getHeader().setContractVersion(mConfig.getContractVersion());

            eastl::string externalBuf;

            ExternalUserLeaveInfoList::const_iterator iter = externalUserLeaveInfoList.begin();
            ExternalUserLeaveInfoList::const_iterator end = externalUserLeaveInfoList.end();
            for (; iter != end; ++iter)
            {
                const ExternalUserLeaveInfo* leaveInfo = *iter;
                const char8_t *externalStr = externalBuf.sprintf("%" PRIu64, leaveInfo->getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId()).c_str();
                eastl::hash_map<const char8_t*, const char8_t*, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo>::const_iterator xuidIter = xblXuidMap.find(externalStr);

                if (xuidIter == xblXuidMap.end())
                {
                    // skip unfound members
                    TRACE_LOG("[ExternalSessionUtilXboxOne].leave: Not leaving member " << externalStr << ", as it was already removed from the MPS. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid());
                    continue;
                }
                putRequest.getBody().getMembers()[xuidIter->second] = '\0';
            }
            if (putRequest.getBody().getMembers().empty())
            {
                return ERR_OK;
            }

            TRACE_LOG("[ExternalSessionUtilXboxOne].leave: leaving " << putRequest.getBody().getMembers().size() << " users " << (xblXuidMap.empty() ? "" : xblXuidMap.begin()->first) << ". from session. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid() << ".");

            Blaze::XBLServices::MultiplayerSessionErrorResponse putErrorTdf;

            err = xblClientSessionDirectorySlave->putMultiplayerSessionLeaveGroup(putRequest, putResponse, putErrorTdf);
            
            for (size_t i = 0; (isRetryError(err) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
            {
                if (ERR_OK != (err = waitSeconds(errorTdf.getRetryAfter(), "[ExternalSessionUtilXboxOne].leave: at leave MPS call", i + 1)))
                    return err;
                err = xblClientSessionDirectorySlave->putMultiplayerSessionLeaveGroup(putRequest, putResponse, putErrorTdf);
            }

            if (err != ERR_OK)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].leave: Failed to leave group. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid() << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ")");
            }
            return err;
        }

        /*! ************************************************************************************************/
        /*! \brief Leave user from Large MPS
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::leaveUserFromLargeMps(const UserIdentification& user, const LeaveGroupExternalSessionParameters& params)
        {
            auto* xblClientSessionDirectorySlave = (Blaze::XBLServices::XBLClientSessionDirectorySlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLServices::XBLClientSessionDirectorySlave::COMPONENT_INFO.name);
            if (xblClientSessionDirectorySlave == nullptr)
            {
                ERR_LOG(logPrefix() << ".leaveUserFromLargeMps: Failed to instantiate XBLClientSessionDirectorySlave object");
                return ERR_SYSTEM;
            }
            eastl::string onBehalfOfUserBuf;

            XBLServices::MultiplayerSessionResponse putResponse;
            XBLServices::PutMultiplayerSessionLeaveGroupRequest putRequest;
            putRequest.setScid(mConfig.getScid());
            putRequest.setSessionTemplateName(params.getSessionIdentification().getXone().getTemplateName());
            putRequest.setSessionName(params.getSessionIdentification().getXone().getSessionName());
            putRequest.getHeader().setContractVersion(mConfig.getContractVersion());
            putRequest.getHeader().setOnBehalfOfUsers(toXblOnBehalfOfUsersHeader(onBehalfOfUserBuf, user).c_str());
            putRequest.getBody().getMembers()[eastl::string(eastl::string::CtorSprintf(), "me_%" PRIu64 "", user.getPlatformInfo().getExternalIds().getXblAccountId()).c_str()] = '\0';
            TRACE_LOG(logPrefix() << ".leaveUserFromLargeMps: leaving user(" << toExtUserLogStr(user) << ") from mps(" << toLogStr(params.getSessionIdentification()) << ").");

            XBLServices::MultiplayerSessionErrorResponse errorTdf;
            RawBufferPtr rspRaw = BLAZE_NEW RawBuffer(0);
            BlazeRpcError err = xblClientSessionDirectorySlave->putMultiplayerSessionLeaveGroup(putRequest, putResponse, errorTdf, RpcCallOptions(), rspRaw.get()); //reusing this rpc
            for (size_t i = 0; (isRetryError(err) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
            {
                if (ERR_OK != (err = waitSeconds(errorTdf.getRetryAfter(), "leaveUserFromLargeMps: at leave MPS call", i + 1)))
                    return err;
                err = xblClientSessionDirectorySlave->putMultiplayerSessionLeaveGroup(putRequest, putResponse, errorTdf, RpcCallOptions(), rspRaw.get());
            }
            if (err != ERR_OK)
            {
                ERR_LOG(logPrefix() << ".leaveUserFromLargeMps: Failed to leave user(" << toExtUserLogStr(user) << ") from mps(" << toLogStr(params.getSessionIdentification()) << "), error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err)
                    << "). Raw response: " << eastl::string(reinterpret_cast<const char8_t*>(rspRaw->data()), rspRaw->datasize()).c_str());
            }
            return err;
        }


        /*! ************************************************************************************************/
        /*! \brief internal cleanup helper for a failed join(), removes the already joined users from external session
        ***************************************************************************************************/
        void ExternalSessionUtilXboxOne::scheduleCleanupAfterFailedJoin(ExternalUserJoinInfoList& cleanupList,
            const char8_t* sessionName, const char8_t* sessionTemplate)
        {
            LeaveGroupExternalSessionParametersPtr paramsLocal = BLAZE_NEW LeaveGroupExternalSessionParameters();
            ExternalUserLeaveInfoList& cleanupListLocal = paramsLocal->getExternalUserLeaveInfos();
            for (ExternalUserJoinInfoList::const_iterator itr = cleanupList.begin(); itr != cleanupList.end(); ++itr)
            {
                // we're only removing the Xbox users in this method, so id who's relevant for the call to MS
                if ((*itr)->getUserIdentification().getPlatformInfo().getClientPlatform() == mPlatform)
                {
                    ExternalUserLeaveInfo* leaveInfo = cleanupListLocal.pull_back();
                    (*itr)->getUserIdentification().getPlatformInfo().copyInto(leaveInfo->getUserIdentification().getPlatformInfo());
                }
            }
            paramsLocal->getSessionIdentification().getXone().setSessionName(sessionName);
            paramsLocal->getSessionIdentification().getXone().setTemplateName(sessionTemplate);
            
            //Pre: ok to schedule this cb owned by this util here (fiber would be canceled before util is torn down)
            Fiber::CreateParams params;
            params.namedContext = "ExternalSessionUtilXboxOne::cleanupAfterFailedJoin";
            gFiberManager->scheduleCall<ExternalSessionUtilXboxOne, LeaveGroupExternalSessionParametersPtr>(
                this, &ExternalSessionUtilXboxOne::cleanupAfterFailedJoin, paramsLocal);
        }

        void ExternalSessionUtilXboxOne::cleanupAfterFailedJoin(LeaveGroupExternalSessionParametersPtr paramsLocal)
        {
            if (paramsLocal != nullptr)
                leave(*paramsLocal);
        }

        /*! \brief  helper to attempt to retrieve MPS. returns XBLCLIENTSESSIONDIRECTORY_NO_CONTENT if it does not exist */
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::getExternalSession(const char8_t* sessionTemplateName, const char8_t* sessionName, XBLServices::MultiplayerSessionResponse& result, const UserIdentification* onBehalfOfUser /*= nullptr*/)
        {
            Blaze::XBLServices::XBLClientSessionDirectorySlave * xblClientSessionDirectorySlave = (Blaze::XBLServices::XBLClientSessionDirectorySlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLServices::XBLClientSessionDirectorySlave::COMPONENT_INFO.name);
            if (xblClientSessionDirectorySlave == nullptr)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].getExternalSession: Failed to instantiate XBLClientSessionDirectorySlave object. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid());
                return ERR_SYSTEM;
            }

            if (!validateSessionTemplateName(sessionTemplateName))
            {
                return XBLSERVICECONFIGS_SESSION_TEMPLATE_NOT_SUPPORTED;
            }

            XBLServices::GetMultiplayerSessionRequest getRequest;
            getRequest.setScid(mConfig.getScid());
            getRequest.setSessionTemplateName(sessionTemplateName);
            getRequest.setSessionName(sessionName);
            getRequest.getHeader().setContractVersion(mConfig.getContractVersion());
            if (isMockPlatformEnabled())
                getRequest.getHeader().setSimOutage("false");
            if (onBehalfOfUser != nullptr)
            {
                eastl::string onBehalfOfUserbuf;
                getRequest.getHeader().setOnBehalfOfUsers(toXblOnBehalfOfUsersHeader(onBehalfOfUserbuf, *onBehalfOfUser).c_str());
            }

            Blaze::XBLServices::MultiplayerSessionErrorResponse errorTdf;

            BlazeRpcError err = xblClientSessionDirectorySlave->getMultiplayerSession(getRequest, result, errorTdf);
            for (size_t i = 0; (isRetryError(err) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
            {
                if (ERR_OK != (err = waitSeconds(errorTdf.getRetryAfter(), "[ExternalSessionUtilXboxOne].getExternalSession", i + 1)))
                    return err;
                err = xblClientSessionDirectorySlave->getMultiplayerSession(getRequest, result, errorTdf);
            }

            if (err == XBLCLIENTSESSIONDIRECTORY_NO_CONTENT)
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].getExternalSession: session did not exist. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid());
            }
            else if (err != ERR_OK)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].getExternalSession: Failed to get session. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid() << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ")");
            }
            return err;
        }

        /*! ************************************************************************************************/
        /*! \brief retrieve list of members
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::getMembers(const ExternalSessionIdentification& sessionIdentification, ExternalIdList& memberExternalIds)
        {
            const char8_t* sessionTemplateName = sessionIdentification.getXone().getTemplateName();
            const char8_t* sessionName = sessionIdentification.getXone().getSessionName();
            XBLServices::MultiplayerSessionResponse getResponse;
            BlazeRpcError err = getExternalSession(sessionTemplateName, sessionName, getResponse);
            if (err != ERR_OK)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].getMembers: Failed to get session. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid() << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ")");
                return err;
            }
            XBLServices::Members::const_iterator memberIter = getResponse.getMembers().begin();
            XBLServices::Members::const_iterator memberEnd = getResponse.getMembers().end();
            for (; memberIter != memberEnd; ++memberIter)
            {
                ExternalXblAccountId xuid = INVALID_EXTERNAL_ID;
                const char8_t* xuidStr = memberIter->second->getConstants().getSystem().getXuid();
                if ((xuidStr == nullptr) || (blaze_str2int(xuidStr, &xuid) == xuidStr))
                {
                    WARN_LOG("[ExternalSessionUtilXboxOne].getMembers: Failed to parse an xblId out of external session member xuid string '" << ((xuidStr != nullptr)? xuidStr : "nullptr") << "'.  sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid() << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ")");
                    continue;
                }
                memberExternalIds.push_back(xuid);
            }
            return err;
        }

        bool ExternalSessionUtilXboxOne::isRetryError(BlazeRpcError mpsdCallErr)
        { 
            return ((mpsdCallErr == ERR_TIMEOUT) ||
                (mpsdCallErr == XBLSERVICECONFIGS_SERVICE_UNAVAILABLE) || (mpsdCallErr == XBLCLIENTSESSIONDIRECTORY_SERVICE_UNAVAILABLE) ||
                (mpsdCallErr == XBLSERVICECONFIGS_SERVICE_INTERNAL_ERROR) || (mpsdCallErr == XBLCLIENTSESSIONDIRECTORY_SERVICE_INTERNAL_ERROR) ||
                (mpsdCallErr == XBLSERVICECONFIGS_BAD_GATEWAY) || (mpsdCallErr == XBLCLIENTSESSIONDIRECTORY_BAD_GATEWAY));
        }

        bool ExternalSessionUtilXboxOne::isMpsAlreadyExistsError(BlazeRpcError mpsdCallErr)
        { 
            //Side: for MPSD contract version 107, a PUT specifying system/constants/gameplay false may get a 'conflicting request' instead of 'already exists' code
            return ((mpsdCallErr == XBLSERVICECONFIGS_EXTERNALSESSION_ALREADY_EXISTS) || (mpsdCallErr == XBLSERVICECONFIGS_CONFLICTING_REQUEST));
        }

        /*! ************************************************************************************************/
        /*! \brief return true if the input session template name is configured on the server. 
                   The default session template name will be written to the input parameter if it is an
                   empty string and if there is only one session templated name configured.
        ***************************************************************************************************/
        bool ExternalSessionUtilXboxOne::validateSessionTemplateName(const char8_t*& sessionTemplateName)
        {
            if ((sessionTemplateName == nullptr || sessionTemplateName[0] == '\0'))
            {
                const XblSessionTemplateName& defaultName = *(mConfig.getSessionTemplateNames().begin());
                sessionTemplateName = defaultName.c_str();
                SPAM_LOG("[ExternalSessionUtilXboxOne].validateSessionTemplateName: Override to use the default session template name: '" << sessionTemplateName << "'");
                return true;
            }

            XblSessionTemplateNameList::const_iterator iter, end;
            for (iter = mConfig.getSessionTemplateNames().begin(), end = mConfig.getSessionTemplateNames().end(); iter != end; ++iter)
            {
                if (blaze_stricmp(iter->c_str(), sessionTemplateName) == 0)
                {
                    return true;
                }
            }

            ERR_LOG("[ExternalSessionUtilXboxOne].validateSessionTemplateName: SessionTemplateName '" << sessionTemplateName << "' is not supported on this server.");
            return false;
        }

        /*! ************************************************************************************************/
        /*! \brief Checks whether game settings/state/fullness changed requiring updating of the MPS's properties.
            \param[in] origValues the game's values before the changes
        ***************************************************************************************************/
        bool ExternalSessionUtilXboxOne::isUpdateRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context)
        {
            // final updates unneeded for MPS
            if (isFinalExtSessUpdate(context))
                return false;

            bool usingCorrelationIds = (getConfig().getScid()[0] != '\0');

            if ((usingCorrelationIds && (newValues.getSessionIdentification().getXone().getCorrelationId()[0] == '\0')) ||
                (newValues.getSessionIdentification().getXone().getSessionName()[0] == '\0'))
            {
                // we don't have a MPSD session right now, no update needed
                // any pending updates will be applied when we next create a MPSD session for this game
                return false;
            }

            if (isUpdateBasicPropertiesRequired(origValues, newValues))
                return true;

            return false;
        }

        /*!************************************************************************************************/
        /*! \brief updates the external session's properties.
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::update(const UpdateExternalSessionPropertiesParameters& params)
        {
            const char8_t* sessionTemplateName = params.getSessionIdentification().getXone().getTemplateName();
            if (isDisabled())
            {
                return ERR_OK;
            }

            if (!validateSessionTemplateName(sessionTemplateName))
            {
                return XBLSERVICECONFIGS_SESSION_TEMPLATE_NOT_SUPPORTED;
            }
            validateGameSettings(params.getExternalSessionUpdateInfo().getSettings());

            BlazeRpcError retErr = ERR_OK;
            if (isUpdateBasicPropertiesRequired(params.getExternalSessionOrigInfo(), params.getExternalSessionUpdateInfo()))
            {
                auto err = updateBasicProperties(params);
                if (err != ERR_OK)
                {
                    retErr = err;
                }
            }

            return retErr;
        }

        bool ExternalSessionUtilXboxOne::isUpdateBasicPropertiesRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues) const
        {
            if (toClosed(origValues) != toClosed(newValues))
                return true;

            if (blaze_strcmp(toJoinRestriction(origValues), toJoinRestriction(newValues)) != 0)
                return true;

            return false;
        }

        /*! ************************************************************************************************/
        /*! \brief updates basic properties for the external session
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::updateBasicProperties(const UpdateExternalSessionPropertiesParameters& params)
        {
            const char8_t* sessionTemplateName = params.getSessionIdentification().getXone().getTemplateName();
            const char8_t* sessionName = params.getSessionIdentification().getXone().getSessionName();

            validateGameSettings(params.getExternalSessionUpdateInfo().getSettings());

            Blaze::XBLServices::XBLClientSessionDirectorySlave * xblClientSessionDirectorySlave = (Blaze::XBLServices::XBLClientSessionDirectorySlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLServices::XBLClientSessionDirectorySlave::COMPONENT_INFO.name);
            if (xblClientSessionDirectorySlave == nullptr)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].update: Failed to instantiate XBLClientSessionDirectorySlave object. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid() << ".");
                return ERR_SYSTEM;
            }

            Blaze::XBLServices::PutMultiplayerSessionUpdateRequest putRequest;
            putRequest.setScid(mConfig.getScid());
            putRequest.setSessionTemplateName(sessionTemplateName);
            putRequest.setSessionName(sessionName);
            putRequest.getHeader().setContractVersion(mConfig.getContractVersion());

            putRequest.getBody().getProperties().getSystem().setClosed(toClosed(params.getExternalSessionUpdateInfo()));
            putRequest.getBody().getProperties().getSystem().setJoinRestriction(toJoinRestriction(params.getExternalSessionUpdateInfo()));
            TRACE_LOG("[ExternalSessionUtilXboxOne].update: Updating MPS (Closed=" << toClosed(params.getExternalSessionUpdateInfo()) << ", JoinRestriction=" << toJoinRestriction(params.getExternalSessionUpdateInfo()) << "). Scid: " << mConfig.getScid() << ", SessionTemplateName: " << sessionTemplateName << ", sessionName: " << sessionName << ".");

            Blaze::XBLServices::MultiplayerSessionResponse putResponse;
            Blaze::XBLServices::MultiplayerSessionErrorResponse putErrorTdf;
            RawBufferPtr rspRaw = BLAZE_NEW RawBuffer(0);
            Blaze::BlazeRpcError err = xblClientSessionDirectorySlave->putMultiplayerSessionUpdate(putRequest, putResponse, putErrorTdf, RpcCallOptions(), rspRaw.get());
            for (size_t i = 0; (isRetryError(err) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
            {
                if (ERR_OK != (err = waitSeconds(putErrorTdf.getRetryAfter(), "[ExternalSessionUtilXboxOne].update", i + 1)))
                    return err;
                rspRaw->reset();
                err = xblClientSessionDirectorySlave->putMultiplayerSessionUpdate(putRequest, putResponse, putErrorTdf, RpcCallOptions(), rspRaw.get());
            }

            if (err != ERR_OK)
            {
                WARN_LOG("[ExternalSessionUtilXboxOne].update: Failed to update MPS. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid() << ", error: " << ErrorHelp::getErrorName(err)
                    << " (" << ErrorHelp::getErrorDescription(err) << "). Raw response: " << eastl::string(reinterpret_cast<const char8_t*>(rspRaw->data()), rspRaw->datasize()).c_str());
            }
            return err;
        }

        /*! ************************************************************************************************/
        /*! \brief checks whether the members in the request pass the MPS's restrictions. MS recommends, and for usability, we check at
             Blaze level, even if UX Join buttons 'should've' auto-disabled, as UX has MS issues being stale or unrefreshed.
             This prevents a friends only game from being joinable via a stale UX.
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::checkRestrictions(const CheckExternalSessionRestrictionsParameters& params)
        {
            TRACE_LOG("[ExternalSessionUtilXboxOne].checkRestrictions: checking join restrictions vs " << params.getExternalUserJoinInfos().size() << " potential joiners. Scid: " << mConfig.getScid() << ", SessionTemplateName: " << params.getSessionIdentification().getXone().getTemplateName() << ", sessionName: " << params.getSessionIdentification().getXone().getSessionName() << ".");

            if (params.getSessionIdentification().getXone().getSessionNameAsTdfString().empty())
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].checkRestrictions:  Check skipped due to session not existing yet.   This can happen when joining the first player joins, before initRequestExternalSessionData is called.");
                return ERR_OK;
            }


            // don't commit external session here, just 'trial swing' to check join-ability.
            bool commit = false;
            ExternalSessionResult result;
            BlazeRpcError err = join(params, &result, commit);

            // We check JoinRestriction and Visibility here but ignore poss fail on Closed, for which GM can return a more appropriate err code (i.e. full or in progress).
            // Note MS explicitly set things up for us so Closed won't mask JoinRestriction fails (err return priority is Visibility, JoinRestriction, Closed. GOS-27862).
            // If not for that ignoring Closed could have let JoinRestriction be bypassed here. We must count *Visibility* fails here, since Visibility *will* mask JoinRestriction.
            if (err == XBLSERVICECONFIGS_EXTERNALSESSION_CLOSED)
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].checkRestrictions: recieved non-JoinRestriction/Visibility return code " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << "). Ignoring, " <<
                    "Blaze will return precise error code if actual join fails. sessionName: " << params.getSessionIdentification().getXone().getSessionName() << ", SessionTemplateName: " << params.getSessionIdentification().getXone().getTemplateName() << ", Scid: " << mConfig.getScid() << ".");
                err = ERR_OK;
            }

            // We actually use 'local' for case game is closed to join by players (see toJoinRestriction()).
            // Ignore join by player fail due to openToJoinByPlayer false so GM can return the same join method not supported error it does on other platforms.
            if (err == XBLSERVICECONFIGS_EXTERNALSESSION_JOINRESTRICTION_LOCAL)
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].checkRestrictions: recieved return code " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << "). Ignoring, " <<
                    "Blaze will return precise error code due to game being closed to join player. sessionName: " << params.getSessionIdentification().getXone().getSessionName() << ", SessionTemplateName: " << params.getSessionIdentification().getXone().getTemplateName() << ", Scid: " << mConfig.getScid() << ".");
                err = ERR_OK;
            }

            if ((err == XBLSERVICECONFIGS_EXTERNALSESSION_VISIBILITY_VISIBLE) || (err == XBLSERVICECONFIGS_EXTERNALSESSION_VISIBILITY_PRIVATE) || (err == XBLSERVICECONFIGS_EXTERNALSESSION_JOINRESTRICTION_FOLLOWED))
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].checkRestrictions: failed check. sessionName: " << params.getSessionIdentification().getXone().getSessionName() << ", SessionTemplateName: " << params.getSessionIdentification().getXone().getTemplateName() << ", Scid: " << mConfig.getScid() << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ").");
            }
            else if (err == XBLSERVICECONFIGS_BAD_REQUEST)
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].checkRestrictions: failed check, possibly due to MPS member count being at its configured max (was SessionTemplate " << params.getSessionIdentification().getXone().getTemplateName() << "'s maxMembersCount configured correctly?). Scid: " << mConfig.getScid() << ", SessionTemplateName: " << params.getSessionIdentification().getXone().getTemplateName() << ", sessionName: " << params.getSessionIdentification().getXone().getSessionName() << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ").");
            }
            else if (err != ERR_OK)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].checkRestrictions: failed check. sessionName: " << params.getSessionIdentification().getXone().getSessionName() << ", SessionTemplateName: " << params.getSessionIdentification().getXone().getTemplateName() << ", Scid: " << mConfig.getScid() << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ").");
            }
            return err;
        }
        
        /*!************************************************************************************************/
        /*! \brief Map the Blaze game's PresenceMode to MPS Visibility setting
        ***************************************************************************************************/
        const char8_t* ExternalSessionUtilXboxOne::toVisibility(const ExternalSessionUpdateInfo& updateInfo) const
        {
            ExternalSessionUpdateInfo::PresenceModeByPlatformMap::const_iterator presenceIt = updateInfo.getPresenceModeByPlatform().find(mPlatform);
            PresenceMode presenceMode = (presenceIt == updateInfo.getPresenceModeByPlatform().end()) ? PRESENCE_MODE_STANDARD : presenceIt->second;
            if (toXblLarge(updateInfo) == true)
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].toVisibility: Visibility for Large MPS by MS spec must be 'open'. Setting to that for (" << PresenceModeToString(presenceMode) << ").");
                return "open";
            }
            switch (presenceMode)
            {
            case PRESENCE_MODE_STANDARD:
                return "open";
            case PRESENCE_MODE_PRIVATE:
                return "visible";
            case PRESENCE_MODE_NONE:
            case PRESENCE_MODE_NONE_2:
                return "private";
            default:
                ERR_LOG("[ExternalSessionUtilXboxOne].toVisibility: Unhandled presence mode '" << PresenceModeToString(presenceMode) << "'. Defaulting to Visibility 'open'.");
                return "open";
            };
        }

        /*!************************************************************************************************/
        /*! \brief Map the Blaze game's settings to MPS JoinRestriction setting
        ***************************************************************************************************/
        const char8_t* ExternalSessionUtilXboxOne::toJoinRestriction(const ExternalSessionUpdateInfo& updateInfo) const
        {
            if ((toXblLarge(updateInfo) == true) || updateInfo.getSettings().getOpenToJoinByPlayer())
                return "none";
            else if (updateInfo.getSettings().getFriendsBypassClosedToJoinByPlayer())
                return "followed";
            else
                return "local";//disables UX 'Join' buttons while allowing invites
        }

        /*!************************************************************************************************/
        /*! \brief The Closed prop of the MPS, if true disables the profile/friend UX join buttons (as of Dec14 XDK won't
            disable invite send button but MS says may fix later). Map the Blaze game's state/settings to MPS Closed prop.
        ***************************************************************************************************/
        bool ExternalSessionUtilXboxOne::toClosed(const ExternalSessionUpdateInfo& updateInfo) const
        {
            // True if game full, in-progress but join-in-progress disabled, or closed invites. Note open to GB, MM by EA spec doesn't affect Closed/UX.
            auto iter = updateInfo.getFullMap().find(mPlatform);
            if (iter == updateInfo.getFullMap().end() || iter->second)
                return true;
 
            if ((updateInfo.getGameType() == GAME_TYPE_GAMESESSION) && 
                (updateInfo.getSimplifiedState() == GAME_PLAY_STATE_IN_GAME) && !updateInfo.getSettings().getJoinInProgressSupported())
                return true;

            // By XR-124 closed to invites *should* also mean its closed to join by player. If title for some reason closed invites but opened join by player,
            // to prevent XR violation (via shell UX at least), still set Closed true to disable invite and the user join buttons in UX. Logged in validateGameSettings.
            if (!updateInfo.getSettings().getOpenToInvites())
                return true;

            return false;
        }

        /*!************************************************************************************************/
        /*! \brief Return whether 'XBL crossPlay' should be set true on the game's MPSD session.
                MS now requires this for invites between Xbox One and XBSX clients to work.
            \note This MS 'crossPlay' only applies for XBL platforms using MPSD (X1, XBSX, PC). Different meaning from 'Blaze crossplay'.
        ***************************************************************************************************/
        bool ExternalSessionUtilXboxOne::toCrossPlay(const ExternalSessionUpdateInfo& updateInfo) const
        {
            // if game has additional XBL supported platforms
            if (mConfig.getCrossgen())
            {
                auto alternatePlatform = ((mPlatform == xone) ? xbsx : xone);
                if (updateInfo.getPresenceModeByPlatform().find(alternatePlatform) != updateInfo.getPresenceModeByPlatform().end())
                    return true;
            }
            return false;
        }

        /*!************************************************************************************************/
        /*! \brief Return whether Large session is enabled for the game's external session
        ***************************************************************************************************/
        bool ExternalSessionUtilXboxOne::toXblLarge(const ExternalSessionUpdateInfo& updateInfo) const
        {
            return updateInfo.getXblLarge();
        }

        void ExternalSessionUtilXboxOne::validateGameSettings(const GameSettings& settings)
        {
            // by XR-124, open to join by player *should* mean open to invites. Warn if title tries violating this.
            if (settings.getOpenToJoinByPlayer() && !settings.getOpenToInvites())
            {
                WARN_LOG("[ExternalSessionUtilXboxOne].validateGameSettings: Warning: session settings are 'open to join by player' while 'closed to invites'. To prevent XR violations setting MPS 'Closed' to true to disallow UX joins by player.");
            }
            // by XR-064, closed to join by player typically expects closed to browsing also, since it will have join restrictions
            if (!settings.getOpenToJoinByPlayer() && settings.getOpenToBrowsing())
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].validateGameSettings: Warning: session settings are 'closed to join by player' while 'opened to game browsing'. This may fail certification depending on browser implementation.");
            }
        }


        /*!************************************************************************************************/
        /*! \brief sets which MPS is the user's 'activity MPS', i.e. the session advertised as joinable in the
             first party UX. See XDK's MultiplayerService.SetActivityAsync for details.
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::setPrimary(const SetPrimaryExternalSessionParameters& params, SetPrimaryExternalSessionResult& result, SetPrimaryExternalSessionErrorResult* errorResult)
        {
            params.getSessionIdentification().copyInto(result);
            const char8_t* sessionTemplateName = params.getSessionIdentification().getXone().getTemplateName();
            const char8_t* sessionName = params.getSessionIdentification().getXone().getSessionName();
            ExternalXblAccountId xblId = params.getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId();
            const char8_t* authToken = params.getAuthInfo().getCachedExternalSessionToken();

            if (isDisabled() || isHandlesDisabled() || (xblId == INVALID_XBL_ACCOUNT_ID) || (authToken == nullptr) || (authToken[0] == '\0'))
            {
                if (isHandlesDisabled())
                {
                    TRACE_LOG("[ExternalSessionUtilXboxOne].setPrimary: handles service explicitly disabled: externalSessionHandlesContractVersion (current configured value:" << mConfig.getExternalSessionHandlesContractVersion() << ") should be non-empty to enable. Not setting primary session for xblId: " << xblId << ", Scid: " << mConfig.getScid() << ", SessionTemplateName: " << sessionTemplateName << ", sessionName: " << sessionName << ".");
                }
                return ERR_OK;
            }
            if (!validateSessionTemplateName(sessionTemplateName))
            {
                return XBLSERVICECONFIGS_SESSION_TEMPLATE_NOT_SUPPORTED;
            }

            Blaze::XBLServices::HandlesGetActivityResult orig;
            Blaze::BlazeRpcError getErr = getPrimaryExternalSession(xblId, orig);
            if (getErr != ERR_OK)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].setPrimary: Failed to check for pre-existing primary external session. sessionName: " << sessionName << ", titleId: " << mTitleId.c_str() << ", SessionTemplateName: " << sessionTemplateName << ", Scid: " << mConfig.getScid() << ", error: " << ErrorHelp::getErrorName(getErr) << " (" << ErrorHelp::getErrorDescription(getErr) << ").");
                return getErr;
            }
            bool hadOrig = (orig.getId()[0] != '\0');
            // early out if was already set as user's primary (avoid MS generating a new handle id)
            if ((blaze_strcmp(orig.getSessionRef().getName(), sessionName) == 0) &&
                (blaze_strcmp(orig.getSessionRef().getTemplateName(), sessionTemplateName) == 0))
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].setPrimary: no update required for xblId " << xblId << ", already set to the correct external session. " << " Pre-existing HandleId: " << orig.getId() << ", Scid: " << orig.getSessionRef().getScid() << ", SessionTemplateName: " << orig.getSessionRef().getTemplateName() << ", sessionName: " << orig.getSessionRef().getName() << ".");
                return ERR_OK;
            }

            Blaze::XBLServices::XBLServiceConfigsSlave * xblServiceConfigsSlave = (Blaze::XBLServices::XBLServiceConfigsSlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLServices::XBLServiceConfigsSlave::COMPONENT_INFO.name);
            if (xblServiceConfigsSlave == nullptr)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].setPrimary: Failed to instantiate XBLServiceConfigsSlave object. sessionName: " << sessionName << ", SessionTemplateName: " << sessionTemplateName << ", :Scid " << mConfig.getScid() << ".");
                return ERR_SYSTEM;
            }

            Blaze::XBLServices::PostHandlesSetActivityRequest postRequest;
            postRequest.getHeader().setAuthToken(authToken);
            postRequest.getHeader().setContractVersion(mConfig.getExternalSessionHandlesContractVersion());
            postRequest.getHeader().setOnBehalfOfTitle(mTitleId.c_str());
            postRequest.getBody().setType("activity");
            postRequest.getBody().getSessionRef().setScid(mConfig.getScid());
            postRequest.getBody().getSessionRef().setTemplateName(sessionTemplateName);
            postRequest.getBody().getSessionRef().setName(sessionName);
            TRACE_LOG("[ExternalSessionUtilXboxOne].setPrimary: setting primary session for xblId " << xblId << " to: SessionTemplateName: " << sessionTemplateName << ", sessionName: " << sessionName << ". " <<
                (hadOrig? eastl::string().sprintf("Pre-existing primary session ActivityHandleId: %s, Scid: %s , SessionTemplateName: %s, sessionName: %s.", orig.getId(), orig.getSessionRef().getScid(), orig.getSessionRef().getTemplateName(), orig.getSessionRef().getName()).c_str() : "No pre-existing primary session."));
            
            Blaze::XBLServices::PostHandlesSetActivityResponse responseTdf;
            Blaze::XBLServices::HandlesErrorResponse errorTdf;
            Blaze::RawBufferPtr rspRaw = BLAZE_NEW RawBuffer(0);
            Blaze::BlazeRpcError err = xblServiceConfigsSlave->postHandlesSetActivity(postRequest, responseTdf, errorTdf, RpcCallOptions(), rspRaw.get());
            if (err == XBLSERVICECONFIGS_AUTHENTICATION_REQUIRED)
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].setPrimary: Authentication with Microsoft failed, token may have expired. Forcing token refresh and retrying.");
                eastl::string buf;
                err = getAuthTokenInternal(xblId, params.getAuthInfo().getServiceName(), buf, true);
                if (err == ERR_OK)//side: no need guarding vs empty token here as already know we're not external user (checked atop)
                {
                    rspRaw->reset();
                    postRequest.getHeader().setAuthToken(buf.c_str());
                    err = xblServiceConfigsSlave->postHandlesSetActivity(postRequest, responseTdf, errorTdf, RpcCallOptions(), rspRaw.get());
                }
            }
            for (size_t i = 0; (isRetryError(err) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
            {
                if (ERR_OK != (err = waitSeconds(errorTdf.getRetryAfter(), "[ExternalSessionUtilXboxOne].setPrimary", i + 1)))
                    return err;
                rspRaw->reset();
                err = xblServiceConfigsSlave->postHandlesSetActivity(postRequest, responseTdf, errorTdf, RpcCallOptions(), rspRaw.get());
            }

            if (err == XBLSERVICECONFIGS_ACCESS_FORBIDDEN)
            {
                // on edge cases/timing like disconnects, user can get removed from MPS before we could set it as primary here. The remove flow will trigger re-choosing primary session as needed. Just no op here.
                TRACE_LOG("[ExternalSessionUtilXboxOne].setPrimary: user " << xblId << " is no longer in external session " << sessionName << " (" << ErrorHelp::getErrorName(err) << "). This can be due to edge cases like disconnects or logouts etc, where user got removed from the external session before we could set it as the primary session. No op.");
                return ERR_OK;
            }
            if (err != ERR_OK)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].setPrimary: Failed to set primary external session. sessionName: " << sessionName << ", titleId: " << mConfig.getExternalSessionTitle() << ", SessionTemplateName: " << sessionTemplateName << ", Scid: "
                    << mConfig.getScid() << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err)
                    << "). Raw response: " << eastl::string(reinterpret_cast<const char8_t*>(rspRaw->data()), rspRaw->datasize()).c_str());
            }
            return err;
        }

        /*!************************************************************************************************/
        /*! \brief
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::clearPrimary(const ClearPrimaryExternalSessionParameters& params, ClearPrimaryExternalSessionErrorResult* errorResult)
        {
            // Note: Microsoft now automatically clears the external session as being the primary for a user,
            // when the user leaves it. No explicit clearing of the primary session required.
            return ERR_OK;
        }
        
        /*!************************************************************************************************/
        /*! \brief gets user's primary MPS information.
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::getPrimaryExternalSession(ExternalXblAccountId xblId, XBLServices::HandlesGetActivityResult& result)
        {
            Blaze::XBLServices::XBLClientSessionDirectorySlave * xblClientSessionDirectorySlave = (Blaze::XBLServices::XBLClientSessionDirectorySlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLServices::XBLClientSessionDirectorySlave::COMPONENT_INFO.name);
            if (xblClientSessionDirectorySlave == nullptr)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].getPrimaryExternalSession: Failed to instantiate XBLClientSessionDirectorySlave object. Scid: " << mConfig.getScid() << ".");
                return ERR_SYSTEM;
            }
            eastl::string xuidBuf;

            Blaze::XBLServices::PostHandlesGetActivityRequest postRequest;
            postRequest.getHeader().setContractVersion(mConfig.getExternalSessionHandlesContractVersion());
            postRequest.getBody().setScid(mConfig.getScid());
            postRequest.getBody().setType("activity");
            postRequest.getBody().getOwners().getXuids().push_back(xuidBuf.sprintf("%" PRIu64, xblId).c_str());

            Blaze::XBLServices::PostHandlesGetActivityResponse responseTdf;
            Blaze::XBLServices::HandlesErrorResponse errorTdf;
            Blaze::RawBufferPtr rspRaw = BLAZE_NEW RawBuffer(0);
            Blaze::BlazeRpcError err = xblClientSessionDirectorySlave->postHandlesGetActivity(postRequest, responseTdf, errorTdf, RpcCallOptions(), rspRaw.get());
            for (size_t i = 0; (isRetryError(err) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
            {
                if (ERR_OK != (err = waitSeconds(errorTdf.getRetryAfter(), "[ExternalSessionUtilXboxOne].getPrimaryExternalSession", i + 1)))
                    return err;
                rspRaw->reset();
                err = xblClientSessionDirectorySlave->postHandlesGetActivity(postRequest, responseTdf, errorTdf, RpcCallOptions(), rspRaw.get());
            }

            if (!responseTdf.getResults().empty())
            {
                if (responseTdf.getResults().size() > 1)
                {
                    WARN_LOG("[ExternalSessionUtilXboxOne].getPrimaryExternalSession: retrieved " << responseTdf.getResults().size() << " primary external session for xblId " << xblId << ", expected one. Using the first. Scid: " << mConfig.getScid() << ".");
                }
                responseTdf.getResults().front()->copyInto(result);
            }

            if (err != ERR_OK)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].getPrimaryExternalSession: Failed to get primary external session for xblId " << xblId << ". Scid: " << mConfig.getScid() << ", titleId: " << mConfig.getExternalSessionTitle()
                    << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err)
                    << "). Raw response: " << eastl::string(reinterpret_cast<const char8_t*>(rspRaw->data()), rspRaw->datasize()).c_str());
            }
            return err;
        }

        /*!************************************************************************************************/
        /*! \brief return whether user session should update the primary external session for user.
        ***************************************************************************************************/
        bool ExternalSessionUtilXboxOne::hasUpdatePrimaryExternalSessionPermission(const UserSessionExistenceData& userSession)
        {
            return (hasJoinExternalSessionPermission(userSession) && 
                gUserSessionManager->getClientTypeDescription(userSession.getClientType()).getPrimarySession() 
                && (userSession.getUserSessionType() != USER_SESSION_GUEST));
        }


        /*! ************************************************************************************************/
        /*! \brief Call this at startup to validate the XDP configured session template values. Logs error on failures.
            \param[in] maxMemberCount - cap on number of members in external sessions to validate. If UINT16_MAX omits validating.
            \return false if able to find startup-blocking issues in the session template, true otherwise.
        ***************************************************************************************************/
        bool ExternalSessionUtilXboxOne::verifyXdpConfiguration(const char8_t* scid, const XblSessionTemplateNameList& sessionTemplateNameList,
            const char8_t* contractVersion, const char8_t* inviteProtocol, uint16_t maxMemberCount, const ExternalServiceCallOptions& callOptions)
        {
            bool allTemplatesPassed = true;
            for (XblSessionTemplateNameList::const_iterator iter = sessionTemplateNameList.begin(),
                end = sessionTemplateNameList.end(); iter != end; ++iter)
            {
                const XblSessionTemplateName& sessionTemplateName = *iter;
                XBLServices::MultiplayerSessionTemplateResponse result;

                BlazeRpcError err = getExternalSessionTemplate(scid, sessionTemplateName, contractVersion, result, callOptions);
                if (err == ERR_COMPONENT_NOT_FOUND)
                {
                    // Don't block startup but just log a warning for Ops to spot, if for some reason controller bootstrap flow changed causing proxy component to be N/A.
                    WARN_LOG("[ExternalSessionUtilXboxOne].verifyXdpConfiguration: Cannot check XDP as external service proxy component was not available. XDK misconfigurations will not be caught up front.");
                    return true;
                }
                else if (err != ERR_OK)
                {
                    ERR_LOG("[ExternalSessionUtilXboxOne].verifyXdpConfiguration: the session template '" << sessionTemplateName <<
                        "' was not configured or unfetchable from Xbox Live, error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ").");
                    allTemplatesPassed = false;
                    continue;
                }

                // got the template. check its contract version is as in the blaze cfg
                int64_t contractVersionInt = 0;
                if ((blaze_str2int(contractVersion, &contractVersionInt) == contractVersion) ||
                    (contractVersionInt != result.getContractVersion()))
                {
                    ERR_LOG("[ExternalSessionUtilXboxOne].verifyXdpConfiguration: the session template '" << sessionTemplateName << "' actual contractVersion '" << result.getContractVersion() <<
                        "' was not equal to the Blaze configuration's contractVersion '" << contractVersion << "'. Scid: " << scid << ", SessionTemplateName: " << sessionTemplateName << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ").");
                    allTemplatesPassed = false;
                    continue;
                }

                // check template's max members if specified. Note: we don't actually use MPS's max members as Blaze fully controls
                // fullness via MPS's Closed. MS now also allows omitting it, which case it just defaults to the non Large MPS max max
                if (result.getFixed().getConstants().getSystem().isMaxMembersCountSet())
                {
                    WARN_LOG("[ExternalSessionUtilXboxOne].verifyXdpConfiguration: the session template '" << sessionTemplateName << "' should omit 'maxMembersCount'. Blaze auto-specifies this at runtime based on game/cfg instead, for Large sessions.");
                    if (maxMemberCount != UINT16_MAX)
                    {
                        if (result.getFixed().getConstants().getSystem().getMaxMembersCount() < maxMemberCount)
                        {
                            ERR_LOG("[ExternalSessionUtilXboxOne].verifyXdpConfiguration: the session template '" << sessionTemplateName << "' actual maxMembersCount '" << result.getFixed().getConstants().getSystem().getMaxMembersCount() <<
                                "' was less than the max capacity for the Blaze sessions '" << maxMemberCount << "'. Scid: " << scid << ", SessionTemplateName: " << sessionTemplateName << ", error: " << ErrorHelp::getErrorName(err) << " (" << ErrorHelp::getErrorDescription(err) << ").");
                            // to prevent blocking game teams from starting blaze til they figure out configs, don't block startup but just log a warning for Ops to spot
                            continue;
                        }
                        // A small buffer room is recommended to allow for out-of-sync MPS's due to exceptional/rare network issues (before MPS's are resync'd GOS-26151).
                        const uint16_t recommendedMaxMembersBufferRoom = 2;
                        if (result.getFixed().getConstants().getSystem().getMaxMembersCount() < (maxMemberCount + recommendedMaxMembersBufferRoom))
                        {
                            WARN_LOG("[ExternalSessionUtilXboxOne].verifyXdpConfiguration: the session template '" << sessionTemplateName << "' actual maxMembersCount '" << result.getFixed().getConstants().getSystem().getMaxMembersCount() <<
                                "' was less than the recommended lower bound '" << (maxMemberCount + recommendedMaxMembersBufferRoom) << "' for the title. (Rare) Xbox Live or network issues may cause MPS's to be out of sync with counterpart Blaze sessions.");
                        }
                    }
                    else
                    {
                        TRACE_LOG("[ExternalSessionUtilXboxOne].verifyXdpConfiguration: the session template '" << sessionTemplateName << "' maxMembersCount validation deliberately bypassed because maxMemberCount is set to maximum possible value(" << maxMemberCount << ")");
                    }
                }
                // MS doesn't support mixing specifying certain MPS constants at both runtime and in session template. Simply do all in Blaze at runtime to avoid conflicts:
                if (result.getFixed().getConstants().getSystem().getCapabilities().isLargeSet())
                {
                    ERR_LOG("[ExternalSessionUtilXboxOne].verifyXdpConfiguration: session template '" << sessionTemplateName << "' should omit 'large'. Blaze auto-specifies this at runtime based on game/cfg instead.");
                    allTemplatesPassed = false;
                }
                if (result.getFixed().getConstants().getSystem().getCapabilities().isCrossPlaySet())
                {
                    ERR_LOG("[ExternalSessionUtilXboxOne].verifyXdpConfiguration: session template '" << sessionTemplateName << "' should omit 'crossPlay'. Blaze auto-specifies this at runtime based on game/cfg instead.");
                    allTemplatesPassed = false;
                }
                if (result.getFixed().getConstants().getSystem().getCapabilities().isGameplaySet())
                {
                    ERR_LOG("[ExternalSessionUtilXboxOne].verifyXdpConfiguration: session template '" << sessionTemplateName << "' should omit 'gamePlay'. Blaze auto-specifies this at runtime based on game/cfg instead.");
                    allTemplatesPassed = false;
                }
            }
            // return false on session template failure only if MPSD enabled
            return (allTemplatesPassed || isDisabled(contractVersion));
        }
        
        /*! ************************************************************************************************/
        /*! \brief fetch the session template configuration from the external sessions service.
            \return error if failed to retrieve. ERR_COMPONENT_NOT_FOUND if MPSD proxy not yet init.
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::getExternalSessionTemplate(const char8_t* scid,
            const char8_t* sessionTemplateName, const char8_t* contractVersion,
            XBLServices::MultiplayerSessionTemplateResponse& result, const ExternalServiceCallOptions& callOptions)
        {
            // Pre: gOutboundHttpService onConfigure/register was done to set up MPSD proxy component before getting here.
            Blaze::XBLServices::XBLClientSessionDirectorySlave * xblClientSessionDirectorySlave = (Blaze::XBLServices::XBLClientSessionDirectorySlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLServices::XBLClientSessionDirectorySlave::COMPONENT_INFO.name);
            if (xblClientSessionDirectorySlave == nullptr)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].getExternalSessionTemplate: Failed to instantiate XBLClientSessionDirectorySlave object.");
                return ERR_COMPONENT_NOT_FOUND;
            }

            XBLServices::GetMultiplayerSessionTemplateRequest getRequest;
            getRequest.setScid(scid);
            getRequest.getHeader().setContractVersion(contractVersion);
            getRequest.setSessionTemplateName(sessionTemplateName);

            XBLServices::MultiplayerSessionErrorResponse errorTdf;
            Blaze::RawBufferPtr rspRaw = BLAZE_NEW RawBuffer(0);
            BlazeRpcError err = xblClientSessionDirectorySlave->getMultiplayerSessionTemplate(getRequest, result, errorTdf, RpcCallOptions(), rspRaw.get());
            for (size_t i = 0; (isRetryError(err) && (i <= callOptions.getServiceUnavailableRetryLimit())); ++i)
            {
                if (ERR_OK != (err = waitSeconds(errorTdf.getRetryAfter(), "[ExternalSessionUtilXboxOne].getExternalSessionTemplate", i + 1)))
                    return err;
                rspRaw->reset();
                err = xblClientSessionDirectorySlave->getMultiplayerSessionTemplate(getRequest, result, errorTdf, RpcCallOptions(), rspRaw.get());
            }
            if (err != ERR_OK)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].getExternalSessionTemplate: failed to retrieve session template. Scid: " << scid << ", SessionTemplateName: " << sessionTemplateName << ", error: " << ErrorHelp::getErrorName(err) << " ("
                    << ErrorHelp::getErrorDescription(err) << "). Raw response: " << eastl::string(reinterpret_cast<const char8_t*>(rspRaw->data()), rspRaw->datasize()).c_str());
            }
            return err;
        }

        /*!************************************************************************************************/
        /*! \brief If creating an Arena match, add Arena specific values to the XBL request.
            \param[in,out] putRequest Request to update
        ***************************************************************************************************/
        BlazeRpcError ExternalSessionUtilXboxOne::prepareArenaParams(XBLServices::PutMultiplayerSessionCreateRequest& putRequest,
            const ExternalSessionCreationInfo& params, const XBLServices::ArenaTeamParamsList* teamParams, const ExternalUserJoinInfo& caller)
        {
            eastl::string startTime;
            BlazeRpcError err = validateArenaParams(params, caller, startTime);
            if (err != ERR_OK)
                return err;

            uint16_t reservedUsersCount = 0;
            eastl::string indexBuf;
            eastl::string callerXuidStr(eastl::string::CtorSprintf(), "%" PRIu64 "", caller.getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId());

            putRequest.getBody().getConstants().getSystem().getCapabilities().setArbitration(true);
            putRequest.getBody().getConstants().getSystem().getArbitration().setArbitrationTimeout(params.getTournamentInfo().getArbitrationTimeout().getMillis());
            putRequest.getBody().getConstants().getSystem().getArbitration().setForfeitTimeout(params.getTournamentInfo().getForfeitTimeout().getMillis());
            // the tournamentRef below is no longer returned in XBL GET responses, so to identify, Blaze also sets it in custom section
            putRequest.getBody().getConstants().getCustom().setTournamentId(params.getTournamentIdentification().getTournamentId());
            // servers section:
            putRequest.getBody().getServers().getTournaments().getConstants().getSystem().getTournamentRef().setTournamentId(params.getTournamentIdentification().getTournamentId());
            putRequest.getBody().getServers().getTournaments().getConstants().getSystem().getTournamentRef().setOrganizer(params.getTournamentIdentification().getTournamentOrganizer());
            putRequest.getBody().getServers().getArbitration().getConstants().getSystem().setStartTime(startTime.c_str());

            // add members' teams
            for (XBLServices::ArenaTeamParamsList::const_iterator itr = teamParams->begin(), end = teamParams->end(); itr != end; ++itr)
            {
                const char8_t* teamName = (*itr)->getUniqueName();

                // Workaround for known filed MS issue: need at least one member per team with a reserved_x entry, for later result reporting to work:
                if (!(*itr)->getMembers().empty())
                {
                    // caller uses 'me' instead of 'reserve_'
                    if (blaze_strcmp(callerXuidStr.c_str(), (*(*itr)->getMembers().begin())->getId()) == 0)
                    {
                        XBLServices::Members::iterator meItr = putRequest.getBody().getMembers().find("me");
                        ASSERT_COND_LOG(meItr != putRequest.getBody().getMembers().end(), "[ExternalSessionUtilXboxOne].prepareArenaParams: internal error, 'me' element not found for user(" << callerXuidStr.c_str() << "). Calling code expected to have allocated 'me'.");
                        if (meItr != putRequest.getBody().getMembers().end())
                            meItr->second->getConstants().getSystem().setTeam(teamName);
                        continue;
                    }
                    XBLServices::Member* m = putRequest.getBody().getMembers().allocate_element();
                    m->getConstants().getSystem().setXuid((*(*itr)->getMembers().begin())->getId());
                    m->getConstants().getSystem().setInitialize(true);
                    m->getConstants().getSystem().setTeam(teamName);
                    putRequest.getBody().getMembers()[indexBuf.sprintf("reserve_%u", reservedUsersCount++).c_str()] = m;
                }
            }
            TRACE_LOG("[ExternalSessionUtilXboxOne].prepareArenaParams: Arena parameters for game(" << params.getExternalSessionId() << "): tournament id(" <<
                params.getTournamentIdentification().getTournamentId() << "), organizer(" << params.getTournamentIdentification().getTournamentOrganizer() <<
                "), startTime(" << putRequest.getBody().getServers().getArbitration().getConstants().getSystem().getStartTime() << "). Caller(" << caller.getUserIdentification().getBlazeId() << ":xblId " << caller.getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId() << ").");
            return ERR_OK;
        }

        BlazeRpcError ExternalSessionUtilXboxOne::validateArenaParams(const ExternalSessionCreationInfo& params, const ExternalUserJoinInfo& caller, eastl::string& startTimeStr)
        {
            // Blaze allows for UTC format 'YYYY-MM-ddTHH:mmZ' or 'YYYY-MM-ddTHH:mm:ssZ', validate there's 5 or 6 units.
            uint32_t year, month, day, hour, minute, second;
            int32_t numUnitsParsed = TimeValue::parseAccountTime(params.getTournamentInfo().getScheduledStartTime(), year, month, day, hour, minute, second);
            if ((numUnitsParsed != 5) && (numUnitsParsed != 6))
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].validateArenaParams: start time(" << params.getTournamentInfo().getScheduledStartTime() << ") invalid, must be UTC string format (e.g. 'YYYY-MM-ddTHH:mm:ssZ'). Caller(" << caller.getUserIdentification().getBlazeId() << ":xblId " << caller.getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId() << ").");
                return GAMEMANAGER_ERR_EXTERNAL_SESSION_ERROR;
            }
            // XBL requires 'YYYY-MM-ddTHH:mm:ss..Z'. Ensure of orig req omitted seconds, we add them
            char8_t buf[64];
            startTimeStr = TimeValue(year, month, day, hour, minute, second).toAccountString(buf, sizeof(buf), true);

            return ERR_OK;
        }

        /*!************************************************************************************************/
        /*! \brief Before creating an Arena match, fetch and validate all joining users have registered teams for the tournament
            \param[out] foundTeams fetched users' team infos
        ***************************************************************************************************/
        bool ExternalSessionUtilXboxOne::validateTeamsRegistered(XBLServices::ArenaTeamParamsList& foundTeams,
            const ExternalUserJoinInfoList& joiners, const TournamentIdentification& tournament)
        {
            ASSERT_COND_LOG((tournament.getTournamentId()[0] != '\0' && tournament.getTournamentOrganizer()[0] != '\0'), "[ExternalSessionUtilXboxOne].validateTeamsRegistered: internal error: tournament id or organizer missing");
            for (ExternalUserJoinInfoList::const_iterator userItr = joiners.begin(), userEnd = joiners.end(); userItr != userEnd; ++userItr)
            {
                // check if we already cached a team having the user in it, and can skip validating
                if (nullptr != findUserTeamInList((*userItr)->getUserIdentification(), foundTeams))
                    continue;

                XBLServices::ArenaTournamentTeamInfo result;
                BlazeRpcError err = getTeamForJoiningUser((*userItr)->getUserIdentification(), (*userItr)->getAuthInfo(), tournament, result);
                if ((err != ERR_OK) && (err != XBLTOURNAMENTSHUB_RESOURCE_NOT_FOUND))
                {
                    return false;
                }

                if (err == XBLTOURNAMENTSHUB_RESOURCE_NOT_FOUND)
                {
                    // for mock testing, we won't *require* tests setup team MPSes. Just add dummy one if missing
                    if (addUserTeamParamIfMockPlatformEnabled(foundTeams, tournament, (*userItr)->getUserIdentification()))
                    {
                        continue;
                    }

                    ERR_LOG("[ExternalSessionUtilXboxOne].validateTeamsRegistered: could not find a tournament team for user(" << (*userItr)->getUserIdentification().getBlazeId() <<
                        ":" << (*userItr)->getUserIdentification().getPlatformInfo().getExternalIds().getXblAccountId() << "), for tournament(" << tournament.getTournamentId() <<
                        "). Has organizer(" << tournament.getTournamentOrganizer() << ") set up user's team?");
                    return false;
                }

                // cache to found teams as needed
                auto* teamInfo = findTeamInList(result.getId(), foundTeams);
                if (teamInfo == nullptr)
                {
                    teamInfo = foundTeams.pull_back();
                    teamInfo->setUniqueName(result.getId());
                    teamInfo->setDisplayName(result.getName());
                    result.getMembers().copyInto(teamInfo->getMembers());
                }
            }

            TRACE_LOG("[ExternalSessionUtilXboxOne].validateTeamsRegistered: found (" << foundTeams.size() << ") teams for tournament("
                << tournament.getTournamentId() << ")");
            return true;
        }

        const XBLServices::ArenaTeamParams* ExternalSessionUtilXboxOne::findUserTeamInList(const UserIdentification& user, const XBLServices::ArenaTeamParamsList& list)
        {
            eastl::string xuidStr(eastl::string::CtorSprintf(), "%" PRIu64 "", user.getPlatformInfo().getExternalIds().getXblAccountId());
            for (XBLServices::ArenaTeamParamsList::const_iterator itr = list.begin(), end = list.end(); itr != end; ++itr)
            {
                for (const auto& member : (*itr)->getMembers())
                {
                    if (blaze_strcmp(member->getId(), xuidStr.c_str()) == 0)
                        return *itr;
                }
            }
            return nullptr;
        }

        XBLServices::ArenaTeamParams* ExternalSessionUtilXboxOne::findTeamInList(const char8_t* teamUniqueName, XBLServices::ArenaTeamParamsList& list)
        {
            for (XBLServices::ArenaTeamParamsList::iterator itr = list.begin(), end = list.end(); itr != end; ++itr)
            {
                if (blaze_stricmp(teamUniqueName, (*itr)->getUniqueName()) == 0)
                    return (*itr);
            }
            return nullptr;
        }

        /*!************************************************************************************************/
        /*! \brief if mock platform is enabled, add a mock team info to the found teams create/join MPS params, for the user
        ***************************************************************************************************/
        bool ExternalSessionUtilXboxOne::addUserTeamParamIfMockPlatformEnabled(XBLServices::ArenaTeamParamsList& teamParams, const TournamentIdentification& tournament, const UserIdentification& user)
        {
            if (!isMockPlatformEnabled())
                return false;
            XBLServices::ArenaTeamParams* teamInfo = teamParams.pull_back();
            eastl::string xuidStr(eastl::string::CtorSprintf(), "%" PRIu64 "", user.getPlatformInfo().getExternalIds().getXblAccountId());
            teamInfo->setDisplayName(xuidStr.c_str());
            teamInfo->setUniqueName(xuidStr.c_str());
            teamInfo->getMembers().pull_back()->setId(xuidStr.c_str());

            return true;
        }


        /*!************************************************************************************************/
        /*! \brief retrieves the user's XBL team for the tournament. returns XBLTOURNAMENTSHUB_RESOURCE_NOT_FOUND
        if no team exists
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::getTeamForJoiningUser(const UserIdentification& user,
            const ExternalUserAuthInfo& authInfo, const TournamentIdentification& tournament,
            XBLServices::ArenaTournamentTeamInfo& result)
        {
            auto* xblTournamentsHub = (XBLServices::XBLTournamentsHubSlave *)Blaze::gOutboundHttpService->getService(XBLServices::XBLTournamentsHubSlave::COMPONENT_INFO.name);
            if (xblTournamentsHub == nullptr)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].getTeamForJoiningUser: Failed to instantiate XBLTournamentsHubSlave object");
                return ERR_SYSTEM;
            }

            // tournament hub calls omit token's user claim
            eastl::string authToken = authInfo.getCachedExternalSessionToken();
            if (!stripUserClaim(authToken))
            {
                return ERR_SYSTEM;
            }

            XBLServices::TournamentsHubErrorResponse errRsp;
            XBLServices::GetArenaTeamForUserResponse rsp;
            XBLServices::GetArenaTeamForUserRequest req;
            req.getHeader().setAuthToken(authToken.c_str());
            req.getHeader().setOnBehalfOfTitle(mTitleId.c_str());
            req.getHeader().setContractVersion(mConfig.getExternalTournamentsContractVersion());
            req.setOrganizerId(tournament.getTournamentOrganizer());
            req.setTournamentId(tournament.getTournamentId());
            req.setMemberId(eastl::string(eastl::string::CtorSprintf(), "%" PRIu64 "", user.getPlatformInfo().getExternalIds().getXblAccountId()).c_str());

            BlazeRpcError err = ERR_OK;

            if (!isMockPlatformEnabled())
            {
                err = xblTournamentsHub->getTournamentTeamForUser(req, rsp, errRsp);
            }
            else
            {
                // mock testing currently doesn't support Tournament Hub API. Simply mock having team
                XBLServices::ArenaTeamParamsList teamParams;
                addUserTeamParamIfMockPlatformEnabled(teamParams, tournament, user);
                auto& teamRsp = *rsp.getValue().pull_back();
                teamRsp.setId((*teamParams.begin())->getUniqueName());
                teamRsp.setName((*teamParams.begin())->getDisplayName());
                teamRsp.getMembers().pull_back()->setId(req.getMemberId());
            }

            if (err != ERR_OK && err != XBLTOURNAMENTSHUB_RESOURCE_NOT_FOUND)
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].getTeamForJoiningUser: Failed, error: " << ErrorHelp::getErrorName(err));
            }
            else
            {
                TRACE_LOG("[ExternalSessionUtilXboxOne].getTeamForJoiningUser: got(" << (rsp.getValue().empty() ? "<NO TEAM>" : (*rsp.getValue().begin())->getId()) <<
                    ") for user(" << user.getPlatformInfo().getExternalIds().getXblAccountId() << "), tournament(" << tournament.getTournamentId() << ":" << tournament.getTournamentOrganizer() << ")");

                if (!rsp.getValue().empty())
                {
                    (*rsp.getValue().begin())->copyInto(result);
                }
            }

            return err;
        }

        /*!************************************************************************************************/
        /*! \brief For non-user claim calls, replace token's user claim part: "XBL3.0 x=<userClaim>;.." with '-'
        ***************************************************************************************************/
        bool ExternalSessionUtilXboxOne::stripUserClaim(eastl::string& authToken) const
        {
            auto pos1 = authToken.find_first_of('=');
            if (pos1 == eastl::string::npos)
            {
                if (isMockPlatformEnabled())
                {
                    TRACE_LOG("[ExternalSessionUtilXboxOne].stripUserClaim: input user claim token(" << authToken << ") allowed for mock testing.");
                    return true;
                }

                ERR_LOG("[ExternalSessionUtilXboxOne].stripUserClaim: no '=' in input user claim token(" << authToken << "). NOOP.");
                return false;
            }

            auto pos2 = authToken.find_first_of(';', pos1 + 1);
            if (pos2 == eastl::string::npos || EA_UNLIKELY(pos2 <= pos1))
            {
                ERR_LOG("[ExternalSessionUtilXboxOne].stripUserClaim: no ';' in input user claim token(" << authToken << "). NOOP.");
                return false;
            }

            authToken.replace(pos1 + 1, (pos2 - pos1 - 1), "-");
            return true;
        }

        /*!************************************************************************************************/
        /*! \brief Update user's recent player group id as needed. Done here, not at master update(), so this,
            like other XBL calls that write to her MPS 'member' (create/join), are serial on user's cmd thread
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::updateNonPrimaryPresence(const UpdateExternalSessionPresenceForUserParameters& params, UpdateExternalSessionPresenceForUserResponse& result, UpdateExternalSessionPresenceForUserErrorInfo& errorResult)
        {
            GetExternalSessionInfoMasterResponse gameInfoBuf;
            if (isUpdateRecentPlayerGroupIdRequired(gameInfoBuf, params))
            {
                return updateRecentPlayerGroupId(params.getUserIdentification(), gameInfoBuf.getExternalSessionCreationInfo().getUpdateInfo(), params.getChangedGame().getPlayer().getTeamIndex());
            }
            return ERR_OK;
        }

        BlazeRpcError ExternalSessionUtilXboxOne::updateRecentPlayerGroupId(const UserIdentification& forUser, const ExternalSessionUpdateInfo& gameInfo, TeamIndex teamIndex)
        {
            auto* xblProxy = (XBLServices::XBLClientSessionDirectorySlave *)gOutboundHttpService->getService(XBLServices::XBLClientSessionDirectorySlave::COMPONENT_INFO.name);
            if (xblProxy == nullptr)
            {
                ERR_LOG(logPrefix() << ".updateRecentPlayerGroupId: Failed to instantiate proxy slave object. Can't update for user(" << toExtUserLogStr(forUser) << "), for (" << toLogStr(gameInfo) << ").");
                return ERR_SYSTEM;
            }
            eastl::string groupidBuf, onBehalfUserBuf;
            XBLServices::PutMultiplayerSessionUpdateRequest req;
            req.setScid(mConfig.getScid());
            req.setSessionTemplateName(gameInfo.getSessionIdentification().getXone().getTemplateName());
            req.setSessionName(gameInfo.getSessionIdentification().getXone().getSessionName());
            req.getHeader().setContractVersion(mConfig.getContractVersion());
            req.getHeader().setOnBehalfOfUsers(toXblOnBehalfOfUsersHeader(onBehalfUserBuf, forUser).c_str());
            auto* member = req.getBody().getMembers().allocate_element();
            member->getProperties().getSystem().getGroups().push_back().set(toXblRecentPlayerGroupId(groupidBuf, gameInfo, teamIndex).c_str());
            req.getBody().getMembers()["me"] = member;
            TRACE_LOG(logPrefix() << ".updateRecentPlayerGroupId: setting user(" << toExtUserLogStr(forUser) << ")'s recent player group(" << groupidBuf << "), for (" << toLogStr(gameInfo) << ").");

            XBLServices::MultiplayerSessionResponse rsp;
            XBLServices::MultiplayerSessionErrorResponse errTdf;
            RawBufferPtr rspRaw = BLAZE_NEW RawBuffer(0);
            BlazeRpcError err = xblProxy->putMultiplayerSessionUpdate(req, rsp, errTdf, RpcCallOptions(), rspRaw.get());
            for (size_t i = 0; (isRetryError(err) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
            {
                if (ERR_OK != (err = waitSeconds(errTdf.getRetryAfter(), "[ExternalSessionUtilXboxOne].updateRecentPlayerGroupId", i + 1)))
                    return err;
                rspRaw->reset();
                err = xblProxy->putMultiplayerSessionUpdate(req, rsp, errTdf, RpcCallOptions(), rspRaw.get());
            }
            if (err != ERR_OK)
            {
                ERR_LOG(logPrefix() << ".updateRecentPlayerGroupId: failed setting user(" << toExtUserLogStr(forUser) << ") recent player group(" << groupidBuf << "), for (" << toLogStr(gameInfo) << "), error(" << ErrorHelp::getErrorName(err) << ")" << " (" << ErrorHelp::getErrorDescription(err) << "). Raw response: " << eastl::string(reinterpret_cast<const char8_t*>(rspRaw->data()), rspRaw->datasize()).c_str());
            }
            return err;
        }

        /*!************************************************************************************************/
        /*! \brief Return whether to update the user's recent player group id, on the game update
        ***************************************************************************************************/
        bool ExternalSessionUtilXboxOne::isUpdateRecentPlayerGroupIdRequired(GetExternalSessionInfoMasterResponse& gameInfoRsp, const UpdateExternalSessionPresenceForUserParameters& updateReason) const
        {
            // Can skip unless update reason *changes* group ids here. (For efficiency, *init* of each user's group id done w/its create/join() MPS call):
            if (updateReason.getChange() != UPDATE_PRESENCE_REASON_TEAM_CHANGE)
            {
                return false;
            }
            if ((mGameManagerSlave == nullptr) || (mGameManagerSlave->getMaster() == nullptr))
            {
                ERR_LOG(logPrefix() << ".isUpdateRecentPlayerGroupIdRequired: Game Manager " << ((mGameManagerSlave == nullptr) ? "Slave" : "Master") << " missing. Cannot check if updates needed.");
                return false;
            }
            GetExternalSessionInfoMasterRequest gameInfoReq;
            updateReason.getUserIdentification().copyInto(gameInfoReq.getCaller());
            gameInfoReq.setGameId(updateReason.getChangedGame().getGameId());
            BlazeRpcError err = mGameManagerSlave->getMaster()->getExternalSessionInfoMaster(gameInfoReq, gameInfoRsp);
            if (err != ERR_OK || !gameInfoRsp.getIsGameMember())
            {
                ASSERT_COND_LOG((err == GAMEMANAGER_ERR_INVALID_GAME_ID), logPrefix() << ".isUpdateRecentPlayerGroupIdRequired: Failed getting info for game(" << gameInfoReq.getGameId() << "), err: " << ErrorHelp::getErrorName(err) << ").");
                return false; //just skip
            }
            return isRecentPlayersBlazeManaged(gameInfoRsp.getExternalSessionCreationInfo().getUpdateInfo());
            // (possible enhancement if needed, could also call GET to check if user already updated on XBL)
        }


        // ExternalUserProfileUtilXboxOne
        
        /*! ************************************************************************************************/
        /*! \brief Gets a user's profile info from xbox live profile service.
        ***************************************************************************************************/
        Blaze::BlazeRpcError ExternalSessionUtilXboxOne::getProfiles(const ExternalIdList& externalIds, ExternalUserProfiles& buf, const char8_t* groupAuthToken)
        {
            if (mConfig.getExternalUserProfilesContractVersion()[0] == '\0')
            {
                TRACE_LOG("[ExternalUserProfileUtilXboxOne].getProfiles: External user profile service disabled.");
                return ERR_OK;
            }

            if (externalIds.empty() || (groupAuthToken == nullptr) || (groupAuthToken[0] == '\0'))
            {
                return ERR_OK;
            }

            Blaze::XBLServices::XBLProfileSlave* xblProfileSlave = (Blaze::XBLServices::XBLProfileSlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLServices::XBLProfileSlave::COMPONENT_INFO.name);
            if (xblProfileSlave == nullptr)
            {
                ERR_LOG("[ExternalUserProfileUtilXboxOne].getProfiles: Failed to instantiate XBLProfileConfigsSlave object.");
                return ERR_SYSTEM;
            }
            
            // build request
            Blaze::XBLServices::GetExternalUserProfilesRequest request;
            request.getGetExternalUserProfilesRequestHeader().setAuthToken(groupAuthToken);
            request.getGetExternalUserProfilesRequestHeader().setContractVersion(mConfig.getExternalUserProfilesContractVersion());
            request.getGetExternalUserProfilesRequestHeader().setAccept("Application/JSON");
            const char8_t* nameSetting = "Gamertag";
            request.getGetExternalUserProfilesRequestBody().getSettings().push_back(nameSetting);
            eastl::string xuidBuf;
            for (ExternalIdList::const_iterator itr = externalIds.begin(); itr != externalIds.end(); ++itr)
            {
                if (EA_LIKELY(*itr != INVALID_EXTERNAL_ID))
                    request.getGetExternalUserProfilesRequestBody().getUserIds().push_back(xuidBuf.sprintf("%" PRIu64, *itr).c_str());
            }

            Blaze::XBLServices::GetExternalUserProfilesResponse response;
            Blaze::RawBufferPtr rspRaw = BLAZE_NEW RawBuffer(0);
            Blaze::BlazeRpcError err = xblProfileSlave->getExternalUserProfiles(request, response, RpcCallOptions(), rspRaw.get());
            if (err != ERR_OK)
            {
                WARN_LOG("[ExternalUserProfileUtilXboxOne].getProfiles: Failed to get external user profiles, error: " << ErrorHelp::getErrorName(err)
                    << ". Raw response: " << eastl::string(reinterpret_cast<const char8_t*>(rspRaw->data()), rspRaw->datasize()).c_str());
                return err;
            }

            for (Blaze::XBLServices::ProfileUsers::const_iterator itr = response.getProfileUsers().begin(); itr != response.getProfileUsers().end(); ++itr)
            {
                ExternalUserProfile& returnUser = buf.push_back();
                    
                // store xuid to return results
                blaze_str2int((*itr)->getId(), &returnUser.mExternalId);

                // store profile settings to return results
                for (Blaze::XBLServices::ProfileUser::ProfileUserSettingList::const_iterator setItr =
                    (*itr)->getSettings().begin(); setItr != (*itr)->getSettings().end(); ++setItr)
                {
                    if (blaze_strcmp((*setItr)->getId(), nameSetting) == 0)
                        returnUser.mUserName = (*setItr)->getValue();
                }
            }
            return ERR_OK;
        }
    }
}

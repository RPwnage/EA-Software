/*! ************************************************************************************************/
/*!
    \file externalsessionutilps4.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "component/gamemanager/externalsessions/externalsessionutilps4.h"

#include "framework/logger.h"
#include "psnbaseurl/tdf/psnbaseurl.h"
#include "psnbaseurl/rpc/psnbaseurlslave.h"
#include "psnsessioninvitation/tdf/psnsessioninvitation.h"
#include "psnsessioninvitation/rpc/psnsessioninvitationslave.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/controller/controller.h" // for gController in getAuthToken
#include "framework/protocol/restprotocolutil.h" // for RestProtocolUtil in callPSNInternal()
#include "framework/protocol/shared/jsonencoder.h" // for JsonEncoder in createNpSession()
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/oauthslave.h"
#include "framework/tdf/oauth_server.h"
#include "gamemanager/shared/externalsessionbinarydatashared.h" // for ExternalSessionBinaryDataHeader helpers
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for gController->isMockPlatformEnabled in getAuthToken()
#include "gamemanager/externalsessions/externaluserscommoninfo.h"
#include "gamemanager/rpc/gamemanagermaster.h" // for GameManagerMaster in getMaster()
#include "gamemanager/gamemanagerslaveimpl.h"
#include <EAIO/PathString.h> // for EA::IO::Path in loadImageDataFromFile
#include <EAStdC/EATextUtil.h> // for UTF8validate in createNpSession, toLocalizedSessionStatus
#include <EAIO/EAFileBase.h> // for FileStream in loadImageDataFromFile
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileStream.h>

namespace Blaze
{
using namespace PSNServices;
namespace GameManager
{
    ExternalSessionUtilPs4::ExternalSessionUtilPs4(const ExternalSessionServerConfig& config, const TimeValue& reservationTimeout, GameManagerSlaveImpl* gameManagerSlave) :
        ExternalSessionUtil(config, reservationTimeout), mTrackHelper(gameManagerSlave, ps4, EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY)
    {
        // Cache default session image, locales. Note: verifyParams() already validated
        loadDefaultImage(mDefaultImage, config.getExternalSessionImageDefault(), config);
        loadSupportedLocalesSet(mSupportedLocalesSet, config.getExternalSessionStatusValidLocales());
    }

    ExternalSessionUtilPs4::~ExternalSessionUtilPs4()
    {
        if (mCachedSessionInvitationConnManager != nullptr)
            mCachedSessionInvitationConnManager.reset();
    }

    bool ExternalSessionUtilPs4::hasUpdatePrimaryExternalSessionPermission(const UserSessionExistenceData& userSession)
    { 
        return (hasJoinExternalSessionPermission(userSession) && 
            gUserSessionManager->getClientTypeDescription(userSession.getClientType()).getPrimarySession() 
            && (userSession.getUserSessionType() != USER_SESSION_GUEST)); 
    }

    bool ExternalSessionUtilPs4::verifyParams(const ExternalSessionServerConfig& config)
    {
        if (isDisabled(config))
        {
            return true;
        }
        const char8_t* titleId = config.getExternalSessionTitle();
        if ((titleId != nullptr) && (titleId[0] != '\0'))
        {
            INFO_LOG("[ExternalSessionUtilPs4::verifyParams] This parameter has been deprecated and is meant to be supplied by the client." );
        }

        if (config.getExternalSessionServiceLabel() < 0)
        {
            ERR_LOG("[ExternalSessionUtilPs4].verifyParams: Invalid PSN service label(" << config.getExternalSessionServiceLabel() << ").");
            return false;
        }

        Ps4NpSessionImage tmpImg;
        if (!loadDefaultImage(tmpImg, config.getExternalSessionImageDefault(), config))
            return false;

        LocalesSet tmpLoc;
        if (!loadSupportedLocalesSet(tmpLoc, config.getExternalSessionStatusValidLocales()))
            return false;

        return true;
    }

    const size_t IMAGE_FILE_EXTENSIONS_SIZE = 6;
    const char8_t* IMAGE_FILE_EXTENSIONS[IMAGE_FILE_EXTENSIONS_SIZE] = {"jpg", "jpeg", "jpe", "jif", "jfif", "jfi"};
    /*! ************************************************************************************************/
    /*! \brief Verify we can load the server configured default session image (at startup/reconfigure)
        \param[out] imageData Stores the loaded image to this buffer.
    ***************************************************************************************************/
    bool ExternalSessionUtilPs4::loadDefaultImage(Ps4NpSessionImage& imageData, const ExternalSessionImageHandle& fileName,
        const ExternalSessionServerConfig& config)
    {
        if (fileName.empty())
        {
            if (!config.getPrimaryExternalSessionTypeForUx().empty())//if !MM/disabled
            {
                ERR_LOG("[ExternalSessionUtilPs4].loadDefaultImage: gamesession.cfg externalSessionImageDefault must specify a default image file, missing.");
                return false;
            }
            return true;
        }
        if (!isValidImageFileExtension(fileName, IMAGE_FILE_EXTENSIONS, IMAGE_FILE_EXTENSIONS_SIZE))
            return false;

        if (!loadImageDataFromFile(imageData, fileName, config.getExternalSessionImageMaxSize()))
            return false;

        if (!isValidImageData(imageData, config.getExternalSessionImageMaxSize()))
            return false;

        INFO_LOG("[ExternalSessionUtilPs4].loadDefaultImage: Verified image file (" << fileName.c_str() << "). Data size (" << imageData.getCount() << ").");
        return true;
    }

    bool ExternalSessionUtilPs4::loadSupportedLocalesSet(LocalesSet& localesSet, const char8_t* configuredList)
    {
        eastl::string nextLocale;
        const size_t len = ((configuredList == nullptr)? 0 : strlen(configuredList));
        for (size_t i = 0; i < len; ++i)
        {
            if ((i+1) % 3 == 0)
            {
                if ((configuredList[i] != ',') && (configuredList[i] != '-'))
                {
                    ERR_LOG("[ExternalSessionUtilPs4].loadSupportedLocalesSet: invalid character (" << configuredList[i] << ") at index " << i << " in configuration setting string(" << configuredList << "). Expected every such 3rd char to be (') or (-)");
                    localesSet.clear();
                    return false;
                }
            }
            else if ((configuredList[i] < 'A') || ((configuredList[i] > 'Z') && (configuredList[i] < 'a')) || (configuredList[i] > 'z'))
            {
                ERR_LOG("[ExternalSessionUtilPs4].loadSupportedLocalesSet: invalid character (" << configuredList[i] << ") at index " << i << " in configuration setting string(" << configuredList << "). Expected an language identifier alphabet character.");
                localesSet.clear();
                return false;
            }
            // if at end of locale, insert to set
            if (configuredList[i] == ',')
            {
                localesSet.insert(nextLocale);
                nextLocale.clear();
                continue;
            }
            nextLocale.append_sprintf("%c", configuredList[i]);
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief retrieve PSN token for the external session updates. Pre: authentication component available
        \param[out] buf - holds the token on success. If this is for an 'external user', this may end up empty.
            Callers are expected to check for empty token as needed before attempting PSN calls.
        \forceRefresh - whether to force refreshing Blaze auth component's cached token. side: this is currently a safe measure in case something changes: the cache is auto-refreshed based on Nucleus's reported expiry, which is well before the real expiry
    ***************************************************************************************************/
    Blaze::BlazeRpcError ExternalSessionUtilPs4::getAuthToken(const UserIdentification& user, const char8_t* serviceName, eastl::string& buf)
    {
        return getAuthTokenInternal(user, serviceName, buf, false);
    }
    Blaze::BlazeRpcError ExternalSessionUtilPs4::getAuthTokenInternal(const UserIdentification& user, const char8_t* serviceName, eastl::string& buf, bool forceRefresh)
    {
        if (isDisabled() || (user.getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID))
        {
            // non PS4 clients may be missing the external id
            TRACE_LOG("[ExternalSessionUtilPs4].getAuthTokenInternal: not retrieving token for Caller(" << externalUserIdentificationToString(user, mUserIdentLogBuf) << "). "
                << (isDisabled()? "External sessions disabled." : "Invalid identification."));
            return ERR_OK;
        }

        BlazeRpcError err = Blaze::ERR_OK;
        Blaze::OAuth::OAuthSlave* oAuthSlave = static_cast<Blaze::OAuth::OAuthSlave*>(gController->getComponent(Blaze::OAuth::OAuthSlave::COMPONENT_ID, false, true, &err));
        if (oAuthSlave == nullptr)
        {
            ERR_LOG("[ExternalSessionUtilPs4].getAuthTokenInternal: Unable to resolve auth component, error: " << ErrorHelp::getErrorDescription(err));
            return err;
        }

        Blaze::OAuth::GetUserPsnTokenRequest req;
        Blaze::OAuth::GetUserPsnTokenResponse rsp;
        req.setForceRefresh(forceRefresh);
        req.setPlatform(getPlatform());
        if (EA_LIKELY(((user.getBlazeId() != INVALID_BLAZE_ID) && !UserSessionManager::isStatelessUser(user.getBlazeId()))))
        {
            req.getRetrieveUsing().setPersonaId(user.getBlazeId());
        }
        else
        {
            WARN_LOG("[ExternalSessionUtilPs4].getAuthTokenInternal: user info missing valid persona id, attempting to fetch token by accountId(" << user.getPlatformInfo().getExternalIds().getPsnAccountId() << ") instead. Note: this may be an unneeded call as PS4 1st party sessions doesn't support reservations for users.");
            req.getRetrieveUsing().setExternalId(user.getPlatformInfo().getExternalIds().getPsnAccountId());
        }

        // in case we're fetching someone else's token, nucleus requires super user privileges
        UserSession::SuperUserPermissionAutoPtr permissionPtr(true);

        err = oAuthSlave->getUserPsnToken(req, rsp);
        if (err == ERR_OK)
        {
            buf.sprintf("Bearer %s", rsp.getPsnToken()).c_str();
            TRACE_LOG("[ExternalSessionUtilPs4].getAuthTokenInternal: Got first party auth token for player (psnAccountId:" << toAccountIdLogStr(user) << ", service:"
                << serviceName << ")" << "auth token(" << rsp.getPsnToken() << "), expires in " << (rsp.getExpiresAt().getSec() - TimeValue::getTimeOfDay().getSec()) << "s.");
        }
        else
        {
            // side: unlike X1, no need to see if its an 'external user' here, as NP sessions don't support reservations
            ERR_LOG("[ExternalSessionUtilPs4].getAuthTokenInternal: Unable to get auth token. Caller(" << externalUserIdentificationToString(user, mUserIdentLogBuf) << ")." << toLogStr(err, nullptr) << ".");
        }

        if (isMockPlatformEnabled())
        {
            // Override token to be user's id, to allow mock service to identify the caller by token. To avoid blocking tests, ignore real auth/nucleus errors (already logged)
            getPsnAccountId(user, buf);
            err = ERR_OK;
        }
        return err;
    }

    void addBody(RawBuffer& dstBuffer, const void* src, size_t srcLen)
    {
        uint8_t* dst = dstBuffer.acquire(srcLen);
        memcpy(dst, src, srcLen);
        dstBuffer.put(srcLen);
    }
    void addHeader(RawBuffer& dstBuffer, const char8_t* headerPrefix, size_t contentLength)
    {
        // (NOTE: if changing this method, remember to ensure HEADER_END_MAXLEN in createNpSession() is big enough, see below)
        addBody(dstBuffer, headerPrefix, strlen(headerPrefix));
        eastl::string headerSuffix;//content length, newline
        headerSuffix.sprintf("%" PRIu64 "\n\n", contentLength);//intentionally 2 LFs
        addBody(dstBuffer, headerSuffix.c_str(), headerSuffix.length());
    }

    /*! ************************************************************************************************/
    /*! \brief creates the NP session
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::createNpSession(const UserIdentification& caller, const ExternalUserAuthInfo& authInfo,
        const GetExternalSessionInfoMasterResponse& gameInfo, ExternalSessionIdentification& resultSession, ExternalSessionErrorInfo* errorInfo, const char8_t* titleId)
    {
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (caller.getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID))
        {
            WARN_LOG("[ExternalSessionUtilPs4].createNpSession: invalid/missing auth token(" << authInfo.getCachedExternalSessionToken() << ") or psnId(" << caller.getPlatformInfo().getExternalIds().getPsnAccountId() << "). No op. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return ERR_SYSTEM;
        }

        // the multi-part headers
        #define POST_SESSION_BOUNDARY "boundary_parameter"
        static const char8_t POST_SESSION_CONTENT_TYPE[] = "multipart/mixed; boundary=" POST_SESSION_BOUNDARY;
        static const char8_t POST_SESSIONREQ_HEADER_PREFIX[] = "--" POST_SESSION_BOUNDARY
            "\nContent-Type:application/json; charset=utf-8\nContent-Description:session-request\nContent-Length:";
        static const char8_t POST_SESSIONIMAGE_HEADER_PREFIX[] = "\n--" POST_SESSION_BOUNDARY
            "\nContent-Type:image/jpeg\nContent-Disposition:attachment\nContent-Description:session-image\nContent-Length:";
        static const char8_t POST_SESSIONDATA_HEADER_PREFIX[] = "\n--" POST_SESSION_BOUNDARY
            "\nContent-Type:application/octet-stream\nContent-Disposition:attachment\nContent-Description:session-data\nContent-Length:";
        static const char8_t POST_SESSIONCHANGEABLEDATA_HEADER_PREFIX[] = "\n--" POST_SESSION_BOUNDARY
            "\nContent-Type:application/octet-stream\nContent-Disposition:attachment\nContent-Description:changeable-session-data\nContent-Length:";
        static const char8_t POST_SESSION_END_BOUNDARY[] = "\n--" POST_SESSION_BOUNDARY "--\n";

        const ExternalSessionCreationInfo& creationInfo = gameInfo.getExternalSessionCreationInfo();
        ExternalSessionUpdateInfo::PresenceModeByPlatformMap::const_iterator presenceIt = creationInfo.getUpdateInfo().getPresenceModeByPlatform().find(ps4);
        PresenceMode presenceMode = (presenceIt == creationInfo.getUpdateInfo().getPresenceModeByPlatform().end()) ? PRESENCE_MODE_STANDARD : presenceIt->second;

        // setup Blaze session-data head (NP session's binary blob)
        ExternalSessionBinaryDataHead sessDataHead;
        toSessionDataHead(creationInfo, sessDataHead);
        // setup Blaze changeable-session-data head
        ExternalSessionChangeableBinaryDataHead changeableSessDataHead;
        toSessionChangeableDataHead(creationInfo.getUpdateInfo(), changeableSessDataHead);

        // setup the session root Json body first, to get its length for reserving send buf's length below.
        SessionRequestJsonBody sessReqBody;
        sessReqBody.setNpTitleId(titleId);
        sessReqBody.setSessionType("owner-migration"); // side: not owner-bind as that destroys sessions on host migration
        sessReqBody.setSessionPrivacy(toSessionPrivacy(presenceMode, creationInfo.getUpdateInfo().getSettings()));
        sessReqBody.setSessionMaxUser(creationInfo.getUpdateInfo().getMaxUsers());
        sessReqBody.setSessionName(getValidJsonUtf8(creationInfo.getUpdateInfo().getGameName(), true));
        sessReqBody.setSessionStatus(getValidJsonUtf8(creationInfo.getUpdateInfo().getStatus().getUnlocalizedValue(), false));
        toLocalizedSessionStatus(creationInfo.getUpdateInfo(), sessReqBody.getLocalizedSessionStatus());
        sessReqBody.getAvailablePlatforms().push_back("PS4");
        sessReqBody.setSessionLockFlag(toSessionLockFlag(creationInfo.getUpdateInfo()));
        
        RawBuffer sessReqJson(1024);
        Blaze::JsonEncoder encoder;
        if (!encoder.encode(sessReqJson, sessReqBody))
        {
            ERR_LOG("[ExternalSessionUtilPs4].createNpSession: internal error: failed to encode session-request JSON body. Cannot complete request.");
            return ERR_SYSTEM;
        }

        // approx max header len, content length + newlines, based on addHeader() above:
        static const size_t HEADER_END_MAXLEN = 32;

        // reserve size of request to send for efficiency
        const size_t approxMaxReqSize = (
            sizeof(POST_SESSIONREQ_HEADER_PREFIX) + HEADER_END_MAXLEN + sizeof(POST_SESSION_END_BOUNDARY) + sessReqJson.datasize() +
            sizeof(POST_SESSIONDATA_HEADER_PREFIX) + HEADER_END_MAXLEN + sizeof(POST_SESSION_END_BOUNDARY) + sizeof(sessDataHead) +
            sizeof(POST_SESSIONCHANGEABLEDATA_HEADER_PREFIX) + HEADER_END_MAXLEN + sizeof(POST_SESSION_END_BOUNDARY) + creationInfo.getExternalSessionCustomData().getCount() + sizeof(changeableSessDataHead) +
            sizeof(POST_SESSIONIMAGE_HEADER_PREFIX) + HEADER_END_MAXLEN + sizeof(POST_SESSION_END_BOUNDARY) + mDefaultImage.getCount() +
            sizeof(POST_SESSION_END_BOUNDARY) + 4096);//extra tail chars buffer
        RawBuffer sendBuf(approxMaxReqSize);

        //add session root's headers, and the Json body built above
        addHeader(sendBuf, POST_SESSIONREQ_HEADER_PREFIX, sessReqJson.datasize());
        addBody(sendBuf, sessReqJson.data(), sessReqJson.datasize());

        //add session-data's headers, and the session-data (Blaze-specified, title-specified)
        addHeader(sendBuf, POST_SESSIONDATA_HEADER_PREFIX, sizeof(sessDataHead) + creationInfo.getExternalSessionCustomData().getCount());
        addBody(sendBuf, &sessDataHead, sizeof(sessDataHead));
        addBody(sendBuf, creationInfo.getExternalSessionCustomData().getData(), creationInfo.getExternalSessionCustomData().getCount());

        //add changeable-session-data's headers, and the changeable-session-data
        addHeader(sendBuf, POST_SESSIONCHANGEABLEDATA_HEADER_PREFIX, sizeof(changeableSessDataHead));
        addBody(sendBuf, &changeableSessDataHead, sizeof(changeableSessDataHead));

        //add session-image's headers, and the image. NOTE: done last for efficiency, in case send buffer must expand. Mock service may also depend on this being last.
        addHeader(sendBuf, POST_SESSIONIMAGE_HEADER_PREFIX, mDefaultImage.getCount());
        addBody(sendBuf, mDefaultImage.getData(), mDefaultImage.getCount());

        //add end boundary
        static_assert(sizeof(POST_SESSION_END_BOUNDARY) > 0, "session boundary empty");
        addBody(sendBuf, POST_SESSION_END_BOUNDARY, sizeof(POST_SESSION_END_BOUNDARY) - 1);


        Blaze::PSNServices::PostNpSessionResponse rsp;
        Blaze::PSNServices::PostNpSessionRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setServiceLabel(getServiceLabel());
        req.getBody().assignData((uint8_t*)sendBuf.data(), sendBuf.datasize());
        TRACE_LOG("[ExternalSessionUtilPs4].createNpSession: creating NP session for game(" << sessDataHead.iGameId << ") with user accountId(" << caller.getPlatformInfo().getExternalIds().getPsnAccountId() << "): npTitleId(" << sessReqBody.getNpTitleId() << "), serviceLabel(" << getServiceLabel() << "), privacy(" << toSessionPrivacy(presenceMode, creationInfo.getUpdateInfo().getSettings()) << "), maxUser(" << creationInfo.getUpdateInfo().getMaxUsers() << "), name(" << creationInfo.getUpdateInfo().getGameName() << "), lockFlag(" << (toSessionLockFlag(creationInfo.getUpdateInfo())? "true" : "false") << "), image(" << mDefaultImage.getCount() <<
            " bytes), " << toLogStr(sessDataHead, &creationInfo.getExternalSessionCustomData()) << ", changeable-data(strGameMode(" << eastl::string(changeableSessDataHead.strGameMode, sizeof(changeableSessDataHead.strGameMode)) << ")), status(" << toLogStr(creationInfo.getUpdateInfo().getStatus()) << ". Request size(" << sendBuf.datasize() << "), allocated buffer size(" << approxMaxReqSize << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");

        // send request to PSN
        const CommandInfo& cmd = PSNSessionInvitationSlave::CMD_INFO_POSTNPSESSION;
        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, &rsp, nullptr, errorInfo, POST_SESSION_CONTENT_TYPE, true);
        if (err != ERR_OK)
        {
            // this is more likely a sign that NpSession doesn't exist, lowering verbosity
            if ((err == PSNSESSIONINVITATION_BAD_REQUEST) || (errorInfo && isUserSignedOutError(err, errorInfo->getCode())))
            {
                WARN_LOG("[ExternalSessionUtilPs4].createNpSession: accountId(" << toAccountIdLogStr(caller) << ") failed to create NP session on err(" << toLogStr(err, nullptr) << "). " << ((errorInfo->getCode() == 2113922) ? "This may also be due to S2S serviceLabel not correctly configured with Blaze or DevNet for the env: " : "") << "npTitleId(" << sessReqBody.getNpTitleId() << "), serviceLabel(" << req.getServiceLabel() << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            }
            else
            {
                ERR_LOG("[ExternalSessionUtilPs4].createNpSession: accountId(" << toAccountIdLogStr(caller) << ") failed to create NP session on err(" << toLogStr(err, nullptr) << "). npTitleId(" << sessReqBody.getNpTitleId() << "), serviceLabel(" << req.getServiceLabel() << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            }
            return err;
        }
        // created
        resultSession.getPs4().setNpSessionId(rsp.getSessionId());

        // Ensure Blaze Game and its NPS, and member tracking are in sync
        err = syncNpsAndTrackedMembership(caller, authInfo, gameInfo, resultSession, errorInfo, titleId);
        if (err == ERR_OK)
        {
            TRACE_LOG("[ExternalSessionUtilPs4].createNpSession: accountId(" << toAccountIdLogStr(caller) << ") successfully created NP session(" << resultSession.getPs4().getNpSessionId() << "), err(" << toLogStr(err, nullptr) << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief joins the specified NP session. Creates game's NP session if the specified NP session is n/a.
        \param[out] resultSession the joined NP session's info, populated/updated on success
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::joinNpSession(const UserIdentification& caller, const ExternalUserAuthInfo& authInfo,
        const GetExternalSessionInfoMasterResponse& gameInfo, ExternalSessionIdentification& resultSession, ExternalSessionErrorInfo* errorInfo, const char8_t* titleId)
    {
        GameId gameId = gameInfo.getExternalSessionCreationInfo().getExternalSessionId();
        const char8_t* npSessionId = gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs4().getNpSessionId();
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (caller.getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID) || (npSessionId == nullptr) || (npSessionId[0] == '\0'))
        {
            //(note PS4 external sessions do not currently support members without EA accounts)
            WARN_LOG("[ExternalSessionUtilPs4].joinNpSession: invalid/missing auth token(" << authInfo.getCachedExternalSessionToken() << ") or psnId(" << caller.getPlatformInfo().getExternalIds().getPsnAccountId() << "), or input NP session(" << ((npSessionId != nullptr)? npSessionId : "<nullptr>") << "). No op. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return ERR_SYSTEM;
        }

        Blaze::PSNServices::PostNpSessionMemberRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setServiceLabel(getServiceLabel());
        req.setSessionId(npSessionId);
        req.setNpTitleId(titleId);
        TRACE_LOG("[ExternalSessionUtilPs4].joinNpSession: joining to game(" << gameId << ")'s NP session(" << npSessionId << ") user accountId(" << caller.getPlatformInfo().getExternalIds().getPsnAccountId() << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");

        const CommandInfo& cmd = PSNSessionInvitationSlave::CMD_INFO_POSTNPSESSIONMEMBER;
        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, nullptr, nullptr, errorInfo);
        if (err == PSNSESSIONINVITATION_RESOURCE_NOT_FOUND)
        {
            // NP session went away before could join it, Sony recommends simply creating one for game.
            // If Blaze game somehow still has that NP session id (possible Sony/network issues), remove it first:
            untrackNpsIdOnMaster(gameId, npSessionId);
            TRACE_LOG("[ExternalSessionUtilPs4].joinNpSession: NP session(" << req.getSessionId() << ") was removed before user could join it. Creating new session for game(" << gameId << ") instead. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return createNpSession(caller, authInfo, gameInfo, resultSession, errorInfo, titleId);
        }
        if (err != ERR_OK)
        {
            // this is more likely a sign that NpSession doesn't exist, lowering verbosity
            if ((err == PSNSESSIONINVITATION_BAD_REQUEST) || (errorInfo && isUserSignedOutError(err, errorInfo->getCode())))
            {
                WARN_LOG("[ExternalSessionUtilPs4].joinNpSession: failed to join user(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ") to game(" << gameId << ") NP session(" << req.getSessionId() << "), " << ErrorHelp::getErrorName(err) << ".");
            }
            else
            {
                ERR_LOG("[ExternalSessionUtilPs4].joinNpSession: failed to join user(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ") to game(" << gameId << ") NP session(" << req.getSessionId() << "), " << ErrorHelp::getErrorName(err) << ".");
            }
            return err;
        }
        // joined
        resultSession.getPs4().setNpSessionId(npSessionId);

        // Ensure Blaze Game and its NPS, and member tracking in sync
        err = syncNpsAndTrackedMembership(caller, authInfo, gameInfo, resultSession, errorInfo, titleId);
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief Call after adding user to a PSN NP session, to ensure the Blaze game, its NP session id and members in sync.
        \param[in] gameInfo Blaze game's info
        \param[in,out] resultSession user's PSN NPS to track membership for. If the Blaze game managed to have a different NPS
            than input, first re-syncs by moving user to the correct PSN NPS, and sets this to that, before tracking membership
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::syncNpsAndTrackedMembership(const UserIdentification& member,
        const ExternalUserAuthInfo& memberAuth, const GetExternalSessionInfoMasterResponse& gameInfo,
        ExternalSessionIdentification& resultSession, ExternalSessionErrorInfo* errorInfo, const char8_t* titleId)
    {
        GameId gameId = gameInfo.getExternalSessionCreationInfo().getExternalSessionId();
        GetExternalSessionInfoMasterResponse rspGameInfo;
        BlazeRpcError err = trackMembershipOnMaster(member, memberAuth, gameInfo, resultSession, rspGameInfo);
        if (err != ERR_OK)
        {
            if (EA_UNLIKELY(err == GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION))
            {
                // someone changed the game's NP session. Resync by joining that instead, update resultSession
                TRACE_LOG("[ExternalSessionUtilPs4].syncNpsAndTrackedMembership: could not track user(" << member.getBlazeId() << ":" << toAccountIdLogStr(member) << ") with NP session(" << resultSession.getPs4().getNpSessionId() << ") for Blaze game(" << gameId << "), as game has a different NP session(" << rspGameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs4().getNpSessionId() << "). Resyncing, by moving user to game current NP session instead. Caller(" << externalUserIdentificationToString(member, mUserIdentLogBuf) << ").");
                return joinNpSession(member, memberAuth, rspGameInfo, resultSession, errorInfo, titleId);   
            }
            // on failure, leave NPS, to prevent issues
            leaveNpSession(member, memberAuth, resultSession.getPs4().getNpSessionId(), gameId, false, errorInfo);
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief leaves the specified NP session
        \param[in] untrackBlazeMembership If true, track user as not in the external session, on master game
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::leaveNpSession(const UserIdentification& caller, const ExternalUserAuthInfo& authInfo,
        const char8_t* npSessionId, GameId gameId, bool untrackBlazeMembership, ExternalSessionErrorInfo* errorInfo)
    {
        ExternalPsnAccountId psnAccountId = caller.getPlatformInfo().getExternalIds().getPsnAccountId();
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || psnAccountId == INVALID_PSN_ACCOUNT_ID || (npSessionId == nullptr) || (npSessionId[0] == '\0'))
        {
            WARN_LOG("[ExternalSessionUtilPs4].leaveNpSession: invalid/missing auth token(" << authInfo.getCachedExternalSessionToken() << ") or psn id(" << psnAccountId << "), or input NP session(" << ((npSessionId != nullptr)? npSessionId : "<nullptr>") << "). No op. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return ERR_SYSTEM;
        }
        TRACE_LOG("[ExternalSessionUtilPs4].leaveNpSession: leaving user psnId(" << psnAccountId << ") from NP session(" << npSessionId << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");

        // Before leaving, track as not in the NP session centrally on master. (prevent further joins to a dying NP session)
        if (untrackBlazeMembership)
        {
            untrackMembershipOnMaster(caller, authInfo, gameId);
        }

        Blaze::PSNServices::DeleteNpSessionMemberRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setServiceLabel(getServiceLabel());
        req.setSessionId(npSessionId);
        req.setAccountId(psnAccountId);
        const CommandInfo& cmd = PSNSessionInvitationSlave::CMD_INFO_DELETENPSESSIONMEMBER;

        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, nullptr, nullptr, errorInfo);
        if (err == PSNSESSIONINVITATION_RESOURCE_NOT_FOUND || err == PSNSESSIONINVITATION_BAD_REQUEST)
        {
            WARN_LOG("[ExternalSessionUtilPs4].leaveNpSession: failed to leave user(" << caller.getBlazeId() << ", " << req.getAccountId() << ") from NP session(" << req.getSessionId() << "), " << ErrorHelp::getErrorName(err) << ".");
        }
        else if (err != ERR_OK)
        {
            ERR_LOG("[ExternalSessionUtilPs4].leaveNpSession: failed to leave user(" << caller.getBlazeId() << ", " << req.getAccountId() << ") from NP session(" << req.getSessionId() << "), " << ErrorHelp::getErrorName(err) << ".");
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief Checks whether game settings/state/fullness etc changed requiring updating of the NP session.
    ***************************************************************************************************/
    bool ExternalSessionUtilPs4::isUpdateRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context)
    {
        // if game doesn't have an NP session, no updates needed. Any NP session will be initialized up to date at its creation.
        if (newValues.getSessionIdentification().getPs4().getNpSessionId()[0] == '\0')
            return false;

        // final updates unneeded since NpSessions go away
        if (isFinalExtSessUpdate(context))
            return false;

        // check if lock flag, max users, name, status need updating.
        if (isUpdateBasicPropertiesRequired(origValues, newValues))
            return true;
            
        if (isUpdateChangeableDataRequired(origValues, newValues))
            return true;

        return false;
    }

    bool ExternalSessionUtilPs4::isUpdateBasicPropertiesRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues)
    {
        if (toSessionLockFlag(origValues) != toSessionLockFlag(newValues))
            return true;

        if (origValues.getMaxUsers() != newValues.getMaxUsers())
            return true;

        // Changes to the GameSettings affect the public/private visibility setting:
        if (!(origValues.getSettings() == newValues.getSettings()))
            return true;

        if (blaze_strcmp(origValues.getGameName(), newValues.getGameName()) != 0)
            return true;

        if (!origValues.getStatus().equalsValue(newValues.getStatus()))
            return true;

        return false;
    }

    bool ExternalSessionUtilPs4::isUpdateChangeableDataRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues)
    {
        return (blaze_strcmp(origValues.getGameMode(), newValues.getGameMode()) != 0);
    }

    /*!************************************************************************************************/
    /*! \brief updates the NP session's properties
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::update(const UpdateExternalSessionPropertiesParameters& params)
    {
        if (isDisabled())
        {
            return ERR_OK;
        }
        BlazeRpcError retErr = ERR_OK;

        ExternalUserAuthInfo authInfo = params.getCallerInfo().getAuthInfo();
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (params.getCallerInfo().getUserIdentification().getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID) || (params.getSessionIdentification().getPs4().getNpSessionId()[0] == '\0'))
        {
            ERR_LOG("[ExternalSessionUtilPs4].update: invalid/missing auth token(" << authInfo.getCachedExternalSessionToken() << ") or psnId(" << params.getCallerInfo().getUserIdentification().getPlatformInfo().getExternalIds().getPsnAccountId() << "), or input session(" << params.getSessionIdentification().getPs4().getNpSessionId() << ").");
            return ERR_SYSTEM;
        }

        // update lock flag, max users, name, status
        if (isUpdateBasicPropertiesRequired(params.getExternalSessionOrigInfo(), params.getExternalSessionUpdateInfo()))
        {
            BlazeRpcError err = updateBasicProperties(params, authInfo);
            if (err != ERR_OK)
            {
                retErr = err;
            }
        }
        // update game mode
        if (isUpdateChangeableDataRequired(params.getExternalSessionOrigInfo(), params.getExternalSessionUpdateInfo()))
        {
            BlazeRpcError err = updateChangeableData(params, authInfo);
            if (err != ERR_OK)
            {
                retErr = err;
            }
        }

        return retErr;
    }

    /*! ************************************************************************************************/
    /*! \brief update the specified NP session's session lock flag, max users, name, status
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::updateBasicProperties(const UpdateExternalSessionPropertiesParameters& params, ExternalUserAuthInfo& authInfo)
    {
        const UserIdentification& caller = params.getCallerInfo().getUserIdentification();

        ExternalSessionUpdateInfo::PresenceModeByPlatformMap::const_iterator presenceIt = params.getExternalSessionUpdateInfo().getPresenceModeByPlatform().find(ps4);
        PresenceMode presenceMode = (presenceIt == params.getExternalSessionUpdateInfo().getPresenceModeByPlatform().end()) ? PRESENCE_MODE_STANDARD : presenceIt->second;

        PutNpSessionUpdateRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setServiceLabel(getServiceLabel());
        req.setSessionId(params.getSessionIdentification().getPs4().getNpSessionId());
        req.getBody().setSessionPrivacy(toSessionPrivacy(presenceMode, params.getExternalSessionUpdateInfo().getSettings()));
        req.getBody().setSessionLockFlag(toSessionLockFlag(params.getExternalSessionUpdateInfo()));
        req.getBody().setSessionMaxUser(params.getExternalSessionUpdateInfo().getMaxUsers());
        req.getBody().setSessionName(getValidJsonUtf8(params.getExternalSessionUpdateInfo().getGameName(), true));
        req.getBody().setSessionStatus(getValidJsonUtf8(params.getExternalSessionUpdateInfo().getStatus().getUnlocalizedValue(), false));
        toLocalizedSessionStatus(params.getExternalSessionUpdateInfo(), req.getBody().getLocalizedSessionStatus());
        TRACE_LOG("[ExternalSessionUtilPs4].updateBasicProperties: updating NP session(" << req.getSessionId() << "), with lockFlag(" << (req.getBody().getSessionLockFlag()? "true" : "false") << "), maxUser(" << req.getBody().getSessionMaxUser() << "), name(" << req.getBody().getSessionName() << "), status(" << req.getBody().getSessionStatus() << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");

        const CommandInfo& cmd = PSNSessionInvitationSlave::CMD_INFO_PUTNPSESSIONUPDATE;
        const char8_t* contentType = "application/json; charset=utf-8";

        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, nullptr, nullptr, nullptr, contentType, true);
        logUpdateErr(err, "updateBasicProperties", req.getSessionId(), caller);
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief update the specified NP session's changeable data (Game mode, etc)
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::updateChangeableData(const UpdateExternalSessionPropertiesParameters& params, ExternalUserAuthInfo& authInfo)
    {
        const UserIdentification& caller = params.getCallerInfo().getUserIdentification();
        ExternalSessionChangeableBinaryDataHead blazeStruct;
        toSessionChangeableDataHead(params.getExternalSessionUpdateInfo(), blazeStruct);

        PutNpSessionChangeableDataRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setServiceLabel(getServiceLabel());
        req.setSessionId(params.getSessionIdentification().getPs4().getNpSessionId());
        req.getBody().resize(sizeof(blazeStruct));
        req.getBody().setCount(sizeof(blazeStruct));
        if (!blazeDataToExternalSessionRawBinaryData(blazeStruct, nullptr, req.getBody().getData(), req.getBody().getSize()))
        {
            ERR_LOG("[ExternalSessionUtilPs4].updateChangeableData: failed updating NP session(" << req.getSessionId() << "), could not convert Blaze header structure to raw binary data.");
            return ERR_SYSTEM;
        }
        TRACE_LOG("[ExternalSessionUtilPs4].updateChangeableData: updating NP session(" << req.getSessionId() << ") changeable-data(strGameMode(" << eastl::string(blazeStruct.strGameMode, sizeof(blazeStruct.strGameMode)) << ")). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");

        const CommandInfo& cmd = PSNSessionInvitationSlave::CMD_INFO_PUTNPSESSIONCHANGEABLEDATA;
        const char8_t* contentType = "application/octet-stream";

        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, nullptr, nullptr, nullptr, contentType, true);
        logUpdateErr(err, "updateChangeableData", req.getSessionId(), caller);
        return err;
    }
        
    /*! ************************************************************************************************/
    /*! \brief update the specified NP session's image
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::updateImage(const UpdateExternalSessionImageParameters& params, UpdateExternalSessionImageErrorResult& errorResult)
    {
        const UserIdentification& caller = params.getCallerInfo().getUserIdentification();
        const ExternalUserAuthInfo& authInfo = params.getCallerInfo().getAuthInfo();

        if (isDisabled())
        {
            return ERR_OK;
        }
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (caller.getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID) || (params.getSessionIdentification().getPs4().getNpSessionId()[0] == '\0'))
        {
            ERR_LOG("[ExternalSessionUtilPs4].updateImage: invalid/missing auth token(" << authInfo.getCachedExternalSessionToken() << ") or psnId(" << caller.getPlatformInfo().getExternalIds().getPsnAccountId() << "), or input session(" << params.getSessionIdentification().getPs4().getNpSessionId() << "). No op. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return ERR_SYSTEM;
        }

        const Ps4NpSessionImage& image = params.getImage();
        if (!isValidImageData(image, mConfig.getExternalSessionImageMaxSize()))
        {
            return GAMEMANAGER_ERR_EXTERNAL_SESSION_IMAGE_INVALID;
        }
            
        PutNpSessionImageRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setServiceLabel(getServiceLabel());
        req.setSessionId(params.getSessionIdentification().getPs4().getNpSessionId());
        image.copyInto(req.getBody());
        TRACE_LOG("[ExternalSessionUtilPs4].updateImage: updating NP session(" << req.getSessionId() << "), image size(" << image.getCount() << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");

        const CommandInfo& cmd = PSNSessionInvitationSlave::CMD_INFO_PUTNPSESSIONIMAGE;
        const char8_t* contentType = "image/jpeg";

        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, nullptr, nullptr, &errorResult, contentType, true);
        logUpdateErr(err, "updateImage", req.getSessionId(), caller);
        return err;
    }

    void ExternalSessionUtilPs4::logUpdateErr(BlazeRpcError err, const char8_t* context, const char8_t* npSessionId, const UserIdentification& caller)
    {
        ASSERT_COND_LOG((context != nullptr && npSessionId != nullptr), "[ExternalSessionUtilPs4].logUpdateErr: input had nullptr " << (context != nullptr ? "context" : "") << (npSessionId != nullptr ? " npSessionId" : ""));
        if ((err == PSNSESSIONINVITATION_RESOURCE_NOT_FOUND) || (err == PSNSESSIONINVITATION_ACCESS_FORBIDDEN) || (err == PSNSESSIONINVITATION_BAD_REQUEST))
        {
            TRACE_LOG("[ExternalSessionUtilPs4]." << context << ": not updating NP session(" << npSessionId << "), error(" << ErrorHelp::getErrorName(err) << "). Caller(" << toAccountIdLogStr(caller) << ") may no longer be in the session.");
        }
        else if (shouldRetryAfterExternalSessionUpdateError(err))
        {
            WARN_LOG("[ExternalSessionUtilPs4]." << context << ": failed updating NP session(" << npSessionId << "), error(" << ErrorHelp::getErrorName(err) << "), with Caller(" << toAccountIdLogStr(caller) << "). Blaze may retry using another caller.");
        }
        else if (err != ERR_OK)
        {
            ERR_LOG("[ExternalSessionUtilPs4]." << context << ": failed updating NP session(" << npSessionId << "), error(" << ErrorHelp::getErrorName(err) << ").");
        }
    }

    /*! ************************************************************************************************/
    /*! \brief pick the next caller to do the update.
        \param[in,out] avoid tracks the tried/rate-limited users to avoid re-picking them.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::getUpdater(const ExternalMemberInfoListByActivityTypeInfo& possibleUpdaters, ExternalMemberInfo& updater, BlazeIdSet& avoid, const UpdateExternalSessionPropertiesParameters* updatePropertiesParams /*= nullptr*/)
    {
        if (updatePropertiesParams && !isUpdateRequired(updatePropertiesParams->getExternalSessionOrigInfo(), updatePropertiesParams->getExternalSessionUpdateInfo(), updatePropertiesParams->getContext()))
        {
            return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }
        auto possPs4Updaters = getListFromMap(possibleUpdaters, mTrackHelper.getExternalSessionActivityTypeInfo());
        if (!possPs4Updaters)
            return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        BlazeRpcError err = ExternalSessionUtil::getUpdaterWithUserToken(getPlatform(), *possPs4Updaters, updater, avoid);
        if (err != ERR_OK)
        {
            WARN_LOG("[ExternalSessionUtilPs4].getUpdater: found NO user who can make the next update call, out of (" << possPs4Updaters->size() << ") potential callers, avoid/tried list size(" << avoid.size() << ").");
        }
        return err;
    }

    /*!************************************************************************************************/
    /*! \brief Build the localized session status part of a update NP session
    ***************************************************************************************************/
    void ExternalSessionUtilPs4::toLocalizedSessionStatus(const ExternalSessionUpdateInfo& info, Blaze::PSNServices::LocalizedSessionStatusEntityList& statusList) const
    {
        size_t addedLocales = 0;
        for (Ps4NpSessionStatusStringLocaleMap::const_iterator itr = info.getStatus().getLangMap().begin(), end = info.getStatus().getLangMap().end();
            ((itr != end) && (addedLocales < mConfig.getExternalSessionStatusMaxLocales())); ++itr)
        {
            //PSN limits number of locales. Only try one if it will be supported and valid
            if (!isSupportedLocale(itr) || (getValidJsonUtf8(itr->second.c_str(), false)[0] =='\0'))
                continue;
            Blaze::PSNServices::LocalizedSessionStatusEntity* status = statusList.pull_back();
            status->setNpLanguage(itr->first.c_str());
            status->setSessionStatus(getValidJsonUtf8(itr->second.c_str(), false));
            ++addedLocales;
        }
    }

    bool ExternalSessionUtilPs4::isSupportedLocale(Ps4NpSessionStatusStringLocaleMap::const_iterator& itr) const
    {
        if (mSupportedLocalesSet.find(itr->first.c_str()) == mSupportedLocalesSet.end())
        {
            WARN_LOG("[ExternalSessionUtilPs4].isSupportedLocale: session status(" << itr->second.c_str() << ") specified locale(" << itr->first.c_str() << ") will not be used, as it is not in the configured list of supported locales(" << mConfig.getExternalSessionStatusValidLocales() << ").");
            return false;
        }
        return true;
    }

    /*!************************************************************************************************/
    /*! \return The value, or empty string if it cannot be used as a valid UTF-8 JSON string value
    ***************************************************************************************************/
    const char8_t* ExternalSessionUtilPs4::getValidJsonUtf8(const char8_t* value, bool isNameNotStatus) const
    {
        size_t maxLen = (size_t)(isNameNotStatus ? MAX_NPSESSION_NAME_LEN : MAX_PS4NPSESSIONSTATUSSTRING_LEN);
        return ExternalSessionUtil::getValidJsonUtf8(value, maxLen);
    }

    /*!************************************************************************************************/
    /*! \brief Map the Blaze game's PresenceMode to NP session sessionPrivacy. sessionPrivacy 'public' makes the
            session visible to all. 'private' makes it visible only to members and those who received invites.
    ***************************************************************************************************/
    const char8_t* ExternalSessionUtilPs4::toSessionPrivacy(PresenceMode presenceMode, GameSettings settings) const
    {
        switch (presenceMode)
        {
        case PRESENCE_MODE_STANDARD:
            // Public presence changes based on the game settings flags: 
            // If this is an invite-only game, we switch to private: 
            if (!settings.getOpenToBrowsing() && !settings.getOpenToMatchmaking() && !settings.getOpenToJoinByPlayer() && !settings.getFriendsBypassClosedToJoinByPlayer())
                return "private";

            return "public";
        case PRESENCE_MODE_PRIVATE:
            return "private";
        case PRESENCE_MODE_NONE:
        case PRESENCE_MODE_NONE_2:
            ERR_LOG("[ExternalSessionUtilPs4].toSessionPrivacy: unexpected internal state: session was created with invalid presence mode(" << PresenceModeToString(presenceMode) << "). Returning sessionPrivacy 'private'.");
            return "private";
        default:
            ERR_LOG("[ExternalSessionUtilPs4].toSessionPrivacy: Unhandled presence mode(" << PresenceModeToString(presenceMode) << "). Returning sessionPrivacy 'public'.");
            return "public";
        };
    }

    /*!************************************************************************************************/
    /*! \brief Map the Blaze game's state/settings to NP session sessionLockFlag. True disables UX join buttons
    ***************************************************************************************************/
    bool ExternalSessionUtilPs4::toSessionLockFlag(const ExternalSessionUpdateInfo& updateInfo) const
    {
        // True if game full, in-progress but join-in-progress disabled, or closed to UI joins. Note open to GB, MM by EA spec doesn't affect Closed/UX.
        auto iter = updateInfo.getFullMap().find(ps4);
        if (iter == updateInfo.getFullMap().end() || iter->second)
            return true;

        if ((updateInfo.getGameType() == GAME_TYPE_GAMESESSION) &&
            (updateInfo.getSimplifiedState() == GAME_PLAY_STATE_IN_GAME) && !updateInfo.getSettings().getJoinInProgressSupported())
            return true;

        // The sessionLockFlag disables all UX join buttons.  If you want to allow joins only by invite, then set the PresenceMode of the Game to PRESENCE_MODE_PRIVATE
        if (!updateInfo.getSettings().getOpenToInvites() && !updateInfo.getSettings().getOpenToJoinByPlayer())
            return true;

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief Populate the NP session binary data (structured header part), from the Blaze game's info
    ***************************************************************************************************/
    void ExternalSessionUtilPs4::toSessionDataHead(const ExternalSessionCreationInfo& creationInfo, ExternalSessionBinaryDataHead& dataHead) const
    {
        memset(&dataHead, 0, sizeof(dataHead));
        dataHead.iGameId = creationInfo.getExternalSessionId();
        dataHead.iGameType = externalSessionTypeToGameType(creationInfo.getExternalSessionType());
    }

    /*! ************************************************************************************************/
    /*! \brief Populate the NP session binary data, from the Blaze game's info
    ***************************************************************************************************/
    void ExternalSessionUtilPs4::toSessionChangeableDataHead(const ExternalSessionUpdateInfo& updateInfo, ExternalSessionChangeableBinaryDataHead& dataHead) const
    {
        ASSERT_COND_LOG(sizeof(dataHead.strGameMode) >= strlen(updateInfo.getGameMode()), "[ExternalSessionUtilPs4].toSessionChangeableDataHead: NP session changeable data's strGameMode max length(" << sizeof(dataHead.strGameMode) << ") is insufficient for game mode value(" << updateInfo.getGameMode() << "). Unless fixed, the changeable session data will have it truncated.");
        memset(&dataHead, 0, sizeof(dataHead));
        blaze_strnzcpy(dataHead.strGameMode, updateInfo.getGameMode(), sizeof(dataHead.strGameMode));//null terms
    }



    /*! ************************************************************************************************/
    /*! \brief retrieves the user's current primary NP session and the GameId stored in its data
        \param[out] npSessionId set to the primary NP session's id if user had one. Null omits.
        \param[out] gameId game id stored in session-data of user's primary NP session. INVALID_GAME_ID if user didn't have one.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::getPrimaryNpsIdAndGameId(const UserIdentification& caller, const ExternalUserAuthInfo& authInfo,
        NpSessionId* npSessionId, GameId& gameId, ExternalSessionErrorInfo* errorInfo)
    {
        gameId = INVALID_GAME_ID;
        if (npSessionId != nullptr)
            npSessionId->set("");

        // get primary session if user has one
        Blaze::PSNServices::NpSessionIdContainer idRsp;
        BlazeRpcError err = getPrimaryNpsId(caller, authInfo, idRsp, errorInfo);
        if (err != ERR_OK)
        {
            ERR_LOG("[ExternalSessionUtilPs4].getPrimaryNpsIdAndGameId: Failed to get pre-existing primary NP session id, " << toLogStr(err, nullptr) << ". Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return err;
        }
        if (idRsp.getSessionId()[0] == '\0')
        {
            // didn't have one
            return ERR_OK;
        }
        
        ExternalSessionBinaryDataHead dataRsp;
        err = getNpSessionData(caller, authInfo, idRsp.getSessionId(), dataRsp, nullptr, errorInfo);
        if (err == PSNSESSIONINVITATION_RESOURCE_NOT_FOUND)
        {
            // just went away
            return ERR_OK;
        }
        else if (err == PSNSESSIONINVITATION_ACCESS_FORBIDDEN)
        {
            WARN_LOG("[ExternalSessionUtilPs4].getPrimaryNpsIdAndGameId: cannot get NP session(" << idRsp.getSessionId() << ") data. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ") may have just signed out. " << toLogStr(err, nullptr) << ".");
            return err;
        }
        else if (err != ERR_OK)
        {
            ERR_LOG("[ExternalSessionUtilPs4].getPrimaryNpsIdAndGameId: Failed to get pre-existing primary NP session(" << idRsp.getSessionId() << ") data. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << "). " << toLogStr(err, nullptr) << ".");
            return err;
        }

        gameId = dataRsp.iGameId;
        if (npSessionId != nullptr)
        {
            npSessionId->set(idRsp.getSessionId());
        }
        return err;
    }


    /*! ************************************************************************************************/
    /*! \brief retrieves the user's current primary NP session's id.
        \param[out] errorInfo Stores PSN call rate limit info, plus additional error info on error. nullptr omits
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::getPrimaryNpsId(const UserIdentification& caller, const ExternalUserAuthInfo& authInfo,
        Blaze::PSNServices::NpSessionIdContainer& result, ExternalSessionErrorInfo* errorInfo)
    {
        ExternalPsnAccountId psnId = caller.getPlatformInfo().getExternalIds().getPsnAccountId();
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || psnId == INVALID_PSN_ACCOUNT_ID)
        {
            WARN_LOG("[ExternalSessionUtilPs4].getPrimaryNpsId: invalid/missing auth token(" << authInfo.getCachedExternalSessionToken() << ") or psnId(" << psnId << "). No op. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return ERR_SYSTEM;
        }

        Blaze::PSNServices::GetNpSessionIdsResponse rsp;
        Blaze::PSNServices::GetNpSessionIdsRequest req;
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setServiceLabel(getServiceLabel());
        req.setAccountId(psnId);

        const CommandInfo& cmd = PSNSessionInvitationSlave::CMD_INFO_GETNPSESSIONIDS;

        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, &rsp, nullptr, errorInfo);
        if (err != ERR_OK)
        {
            if (err == PSNSESSIONINVITATION_BAD_REQUEST)
            {
                WARN_LOG("[ExternalSessionUtilPs4].getPrimaryNpsId: Failed getting user(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ") primary session, " << ErrorHelp::getErrorName(err) << ".");
            }
            else
            {
                ERR_LOG("[ExternalSessionUtilPs4].getPrimaryNpsId: Failed getting user(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ") primary session, " << ErrorHelp::getErrorName(err) << ".");
            }
            return err;
        }

        // store to result
        if (!rsp.getSessions().empty())
        {
            if (EA_UNLIKELY(rsp.getSessions().size() > 1))
            {
                WARN_LOG("[ExternalSessionUtilPs4].getPrimaryNpsId: user unexpectedly has " << rsp.getSessions().size() << " NP sessions. Having more than one may cause errors, as multiple session membership is not currently supported. Assuming primary is the first. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            }
            rsp.getSessions().front()->copyInto(result);
        }
        // store the rate limit info regardless of pass/fail, for testing and debugging
        if (errorInfo != nullptr)
            toExternalSessionRateInfo(rsp, errorInfo->getRateLimitInfo(), "getPrimaryNpsId");

        TRACE_LOG("[ExternalSessionUtilPs4].getPrimaryNpsId: psnId(" << psnId << ") currently has (" << ((result.getSessionId()[0] == '\0')? "NO session" : result.getSessionId()) << ") as its primary session. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief retrieves the NP session's custom data.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::getNpSessionData(const UserIdentification& caller,
        const ExternalUserAuthInfo& authInfo, const char8_t* npSessionId, ExternalSessionBinaryDataHead& resultHead,
        ExternalSessionCustomData* resultTitle, ExternalSessionErrorInfo* errorInfo)
    {
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (npSessionId == nullptr) || (npSessionId[0] == '\0'))
        {
            ERR_LOG("[ExternalSessionUtilPs4].getNpSessionData: internal error: invalid or empty input session(" << ((npSessionId == nullptr)? "<nullptr>" : npSessionId) << "), or auth token(" << authInfo.getCachedExternalSessionToken() << ") (note PS4 external sessions do not currently support members without EA accounts).");
            return ERR_SYSTEM;
        }

        Blaze::PSNServices::GetNpSessionDataRequest req;
        RawBuffer rsp(sizeof(ExternalSessionBinaryDataHead));
        req.getHeader().setAuthToken(authInfo.getCachedExternalSessionToken());
        req.setServiceLabel(getServiceLabel());
        req.setSessionId(npSessionId);

        const CommandInfo& cmd = PSNSessionInvitationSlave::CMD_INFO_GETNPSESSIONDATA;

        BlazeRpcError err = callPSN(caller, cmd, authInfo, req.getHeader(), req, nullptr, &rsp, errorInfo);
        if (err == PSNSESSIONINVITATION_ACCESS_FORBIDDEN || err == PSNSESSIONINVITATION_BAD_REQUEST)
        {
            WARN_LOG("[ExternalSessionUtilPs4].getNpSessionData: cannot get NP session(" << req.getSessionId() << ") data, " << ErrorHelp::getErrorName(err) << ". Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ") may have just signed out.");
            return err;
        }
        else if (err != ERR_OK)
        {
            ERR_LOG("[ExternalSessionUtilPs4].getNpSessionData: failed to get NP session(" << req.getSessionId() << ") data, " << ErrorHelp::getErrorName(err)
                << ". Raw response: " << eastl::string(reinterpret_cast<const char8_t*>(rsp.data()), rsp.datasize()));
            return err;
        }

        // store to results
        if (!externalSessionRawBinaryDataToBlazeData(rsp.data(), rsp.datasize(), resultHead, resultTitle))
        {
            ERR_LOG("[ExternalSessionUtilPs4].getNpSessionData: failed to extract Blaze header structure from NP session(" << req.getSessionId() << ") data. Size of header structure(" << sizeof(resultHead) << "), size of raw session data(" << rsp.datasize() << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << "). " << toLogStr(err) << ".");
            return ERR_SYSTEM;
        }
        TRACE_LOG("[ExternalSessionUtilPs4].getNpSessionData: retrieved NP session(" << req.getSessionId() << ") " << toLogStr(resultHead, resultTitle) << ". Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
        return err;
    }


    ///////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Primary External Sessions //////////////////////////////

    /*! ************************************************************************************************/
    /*! \brief return whether set primary required, based on Blaze game. For avoiding extra rate limited PSN calls.
        \param[out] existingPrimaryInfo if there's a pre existing primary session for user found, its populated here
    ***************************************************************************************************/
    bool ExternalSessionUtilPs4::isSetPrimaryRequired(const SetPrimaryExternalSessionParameters& params,
        GetExternalSessionInfoMasterResponse& existingPrimaryInfo) const
    {
        return mTrackHelper.isAddUserToExtSessionRequired(existingPrimaryInfo, params.getNewPrimaryGame().getGameId(), params.getUserIdentification());
    }

    /*! ************************************************************************************************/
    /*! \brief set user's NP session
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::setPrimary(const SetPrimaryExternalSessionParameters& params, SetPrimaryExternalSessionResult& result, SetPrimaryExternalSessionErrorResult* errorResult)
    {
        ASSERT_COND_LOG(!isMultipleMembershipSupported(), "[ExternalSessionUtilPs4].setPrimary: this method is not yet implemented to work with multiple NP session membership");
        BlazeRpcError err = ERR_OK;
        const UserIdentification& caller = params.getUserIdentification();
        const ExternalUserAuthInfo& authInfo = params.getAuthInfo();
        if (isDisabled() || mConfig.getPrimaryExternalSessionTypeForUx().empty())
        {
            TRACE_LOG("[ExternalSessionUtilPs4].setPrimary: External sessions explicitly disabled. No primary session will be set for caller " << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ".");
            return ERR_OK;
        }
        if ((authInfo.getCachedExternalSessionToken()[0] == '\0') || (caller.getPlatformInfo().getExternalIds().getPsnAccountId() == INVALID_PSN_ACCOUNT_ID) || (params.getNewPrimaryGame().getGameId() == INVALID_GAME_ID))
        {
            WARN_LOG("[ExternalSessionUtilPs4].setPrimary: invalid/missing auth token(" << authInfo.getCachedExternalSessionToken() << "), psnId(" << caller.getPlatformInfo().getExternalIds().getPsnAccountId() << "), or GameId(" << params.getNewPrimaryGame().getGameId() << "). No op. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return ERR_SYSTEM;
        }

        const char8_t* titleId = updateTitleIdIfMockPlatformEnabled(params.getTitleId());
        if (titleId[0] == '\0')
        {
            WARN_LOG("[ExternalSessionUtilPs4].setPrimary: missing title id. No op. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return ERR_SYSTEM;
        }


        // If already set as primary, we're done
        GetExternalSessionInfoMasterResponse gameInfoBuf;
        if (!isSetPrimaryRequired(params, gameInfoBuf))
        {
            gameInfoBuf.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().copyInto(result);
            return ERR_OK;
        }
        // 1st track as left any old primary session on master (guard vs timing issues)
        GameId oldPrimaryGameId = params.getOldPrimaryGameId();
        if (oldPrimaryGameId != INVALID_GAME_ID)
        {
            err = untrackMembershipOnMaster(caller, authInfo, params.getOldPrimaryGameId());
            if (err != ERR_OK)
                return err;
        }

        // Ensure have primary game's latest params for the PSN call
        err = mTrackHelper.getGameInfoFromMaster(gameInfoBuf, params.getNewPrimaryGame().getGameId(), caller);
        if (err != ERR_OK || !gameInfoBuf.getIsGameMember())
        {
            // ok if target game was left, its remove notifications will trigger re-choosing primary session soon enough
            return ((err == GAMEMANAGER_ERR_INVALID_GAME_ID)? ERR_OK : err);
        }

        // If game already has an NP session, join, otherwise create. store result
        if (gameInfoBuf.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getPs4().getNpSessionId()[0] != '\0')
        {
            err = joinNpSession(caller, authInfo, gameInfoBuf, result, errorResult, titleId);
        }
        else
        {
            err = createNpSession(caller, authInfo, gameInfoBuf, result, errorResult, titleId);
        }

        // ok if target game was left, its remove notifications will trigger re-choosing primary session soon enough
        return ((err == GAMEMANAGER_ERR_INVALID_GAME_ID)? ERR_OK : err);
    }

    /*! ************************************************************************************************/
    /*! \brief clear user's NP session
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::clearPrimary(const ClearPrimaryExternalSessionParameters& params, ClearPrimaryExternalSessionErrorResult* errorResult)
    {
        ASSERT_COND_LOG(!isMultipleMembershipSupported(), "[ExternalSessionUtilPs4].clearPrimary: this method is not yet implemented to work with multiple NP session membership");
        const UserIdentification& caller = params.getUserIdentification();
        if (isDisabled() || mConfig.getPrimaryExternalSessionTypeForUx().empty())
        {
            return ERR_OK;
        }

        NpSessionId origNpSessionId;
        GameId origPrimaryGameId = INVALID_GAME_ID;
        BlazeRpcError getErr = getPrimaryNpsIdAndGameId(caller, params.getAuthInfo(), &origNpSessionId, origPrimaryGameId, errorResult);
        if (getErr == PSNSESSIONINVITATION_ACCESS_FORBIDDEN)
        {
            WARN_LOG("[ExternalSessionUtilPs4].clearPrimary: No op. Could not get primary NP session, PSN may have already signed out / cleaned up for the caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return ERR_OK;
        }
        if (getErr != ERR_OK)
        {
            ERR_LOG("[ExternalSessionUtilPs4].clearPrimary: Failed checking for existing primary session. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << "). " << toLogStr(getErr) << ").");
        }
        if (origNpSessionId.empty())
        {
            TRACE_LOG("[ExternalSessionUtilPs4].clearPrimary: No existing primary session to clear for user accountId(" << toAccountIdLogStr(caller) << "). No op. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return ERR_OK;
        }
        TRACE_LOG("[ExternalSessionUtilPs4].clearPrimary: clearing existing primary NP session(" << origNpSessionId.c_str() << ") for user accountId(" << toAccountIdLogStr(caller) << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");

        BlazeRpcError err = leaveNpSession(params.getUserIdentification(), params.getAuthInfo(), origNpSessionId.c_str(), origPrimaryGameId, true, errorResult);
        if (err == PSNSESSIONINVITATION_RESOURCE_NOT_FOUND)
        {
            // Sony already cleaned
            err = ERR_OK;
        }
        if (err != ERR_OK)
        {
            ERR_LOG("[ExternalSessionUtilPs4].clearPrimary: failed to clear user's primary NP session(" << origNpSessionId.c_str() << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << "). " << toLogStr(err, nullptr) << ".");
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief Call after adding user to a PSN NP session, to ensure the Blaze Game, its NP session id and members in sync.
        (see DevNet ticket 58807 for why a Game can change or remove NP sessions, and why we need such tracking)

        \param[in] gameInfo the Blaze game's info. Note: this may be a snapshot taken before the first party session update
        \param[in] extSessionToAdvertise the game's expected NP session, to track as advertised by user.
        \param[out] result game's NP session info, after the track attempt.
        \return If game's NP session was not the one in request, GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION. Else an appropriate code
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::trackMembershipOnMaster(const UserIdentification& member,
        const ExternalUserAuthInfo& memberAuth, const GetExternalSessionInfoMasterResponse& gameInfo,
        const ExternalSessionIdentification& extSessionToAdvertise, GetExternalSessionInfoMasterResponse& result) const
    {
        ExternalMemberInfo memberInfo;
        member.copyInto(memberInfo.getUserIdentification());
        memberAuth.copyInto(memberInfo.getAuthInfo());
        return mTrackHelper.trackExtSessionMembershipOnMaster(result, gameInfo, memberInfo, extSessionToAdvertise);
    }

    /*! ************************************************************************************************/
    /*! \brief track user as not advertising / member of the NP session
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::untrackMembershipOnMaster(const UserIdentification& member,
        const ExternalUserAuthInfo& memberAuth, GameId gameId) const
    {
        ExternalMemberInfo memberInfo;
        member.copyInto(memberInfo.getUserIdentification());
        return mTrackHelper.untrackExtSessionMembershipOnMaster(nullptr, gameId, &memberInfo, nullptr);
    }

    BlazeRpcError ExternalSessionUtilPs4::untrackNpsIdOnMaster(GameId gameId, const char8_t* npSessionIdToClear) const
    {
        ExternalSessionIdentification extSessToUntrackAll;
        extSessToUntrackAll.getPs4().setNpSessionId(npSessionIdToClear);
        return mTrackHelper.untrackExtSessionMembershipOnMaster(nullptr, gameId, nullptr, &extSessToUntrackAll);
    }


    /*! ************************************************************************************************/
    /*! \brief Return whether error code indicates the cached Base URL/connection possibly expired
    ***************************************************************************************************/
    bool ExternalSessionUtilPs4::isConnectionExpiredError(BlazeRpcError psnErr) const
    { 
        bool result = (((psnErr == ERR_TIMEOUT) || (psnErr == ERR_COULDNT_CONNECT) || (psnErr == ERR_DISCONNECTED) || 
            (psnErr == PSNSESSIONINVITATION_RESOURCE_NOT_FOUND) || (psnErr == PSNSESSIONINVITATION_SERVICE_UNAVAILABLE) || (psnErr == PSNSESSIONINVITATION_SERVICE_INTERNAL_ERROR)));
        if (result)
        {
            TRACE_LOG("[ExternalSessionUtilPs4].isConnectionExpiredError: Sony service Base URL/connection may have expired. PSN error(" << ErrorHelp::getErrorName(psnErr) << "), " << ErrorHelp::getErrorDescription(psnErr) << "). Blaze may force Base URL refresh and retry the request.");
        }
        return result;
    }

    bool ExternalSessionUtilPs4::isRetryError(BlazeRpcError psnErr, const Blaze::PSNServices::PsnErrorResponse& psnRsp) const
    {
        // side: Sony has had issues that intermittently cause PSNSESSIONINVITATION_ACCESS_FORBIDDEN with code 2114564 ("User not online") which warrant retry
        return ((psnErr == ERR_TIMEOUT) || (psnErr == PSNBASEURL_SERVICE_UNAVAILABLE) ||
            (psnErr == PSNBASEURL_SERVICE_INTERNAL_ERROR) || (psnErr == PSNSESSIONINVITATION_ACCESS_FORBIDDEN && psnRsp.getError().getCode() == 2114564) ||
            (psnErr == PSNSESSIONINVITATION_SERVICE_UNAVAILABLE) || (psnErr == PSNSESSIONINVITATION_SERVICE_INTERNAL_ERROR));
    }

    bool ExternalSessionUtilPs4::isTokenExpiredError(BlazeRpcError psnErr, const Blaze::PSNServices::PsnErrorResponse& psnRsp) const
    {
        // side: Sony has issue currently, if a user changed its PSN onlineId, that can invalidate cached tokens and cause PSNSESSIONINVITATION_ACCESS_FORBIDDEN.
        // (Sony may change this case in future to return a granular error code, instead of simply PSNSESSIONINVITATION_ACCESS_FORBIDDEN. See GOS-30799)
        bool isPossibleOnlineIdChanged = (((psnErr == PSNSESSIONINVITATION_ACCESS_FORBIDDEN) && 
                                           (psnRsp.getError().getCode() == 2114049 ||  // Invalid access token
                                            psnRsp.getError().getCode() == 2114050)) ||// Expired access token
                                          isUserSignedOutError(psnErr, psnRsp.getError().getCode())); // User not online  (May be due to user rename)

        bool result (isPossibleOnlineIdChanged || (psnErr == PSNBASEURL_AUTHENTICATION_REQUIRED) || (psnErr == PSNSESSIONINVITATION_AUTHENTICATION_REQUIRED));
        if (result)
        {
            WARN_LOG("[ExternalSessionUtilPs4].isTokenExpiredError: authentication with Sony failed, token may have expired. PSN error(" << 
                ErrorHelp::getErrorName(psnErr) << "), " << ErrorHelp::getErrorDescription(psnErr) << "). (Code: "<< 
                psnRsp.getError().getCode() << " Message: "<< psnRsp.getError().getMessage() << " ) Blaze may forcing token refresh and retry the request.");
        }
        return result;
    }

    bool ExternalSessionUtilPs4::isUserSignedOutError(BlazeRpcError psnErr, int32_t psnErrorInfoCode) const
    {
        return ((psnErr == PSNSESSIONINVITATION_ACCESS_FORBIDDEN) && (psnErrorInfoCode == 2114564)); // User not online  (May be due to user rename)
    }

    /*! ************************************************************************************************/
    /*! \brief helper to populate Blaze error info from the PSN info.
    ***************************************************************************************************/
    void ExternalSessionUtilPs4::toExternalSessionErrorInfo(const Blaze::PSNServices::PsnErrorResponse& psnRsp, Blaze::ExternalSessionErrorInfo &errorInfo, const char8_t* extraContextInfo) const
    {
        errorInfo.getRateLimitInfo().setRateLimitLimit(psnRsp.getRateLimitLimit());
        errorInfo.getRateLimitInfo().setRateLimitNextAvailable(psnRsp.getRateLimitNextAvailable());
        errorInfo.getRateLimitInfo().setRateLimitRemaining(psnRsp.getRateLimitRemaining());
        errorInfo.getRateLimitInfo().setRateLimitTimePeriod(psnRsp.getRateLimitTimePeriod());
        errorInfo.getRateLimitInfo().setContext(extraContextInfo);
        errorInfo.setCode(psnRsp.getError().getCode());
        errorInfo.setMessage(psnRsp.getError().getMessage());
    }

    void ExternalSessionUtilPs4::toExternalSessionRateInfo(const Blaze::PSNServices::GetNpSessionIdsResponse& psnRsp, RateLimitInfo& rateInfo, const char8_t* extraContextInfo) const
    {
        rateInfo.setRateLimitLimit(psnRsp.getRateLimitLimit());
        rateInfo.setRateLimitNextAvailable(psnRsp.getRateLimitNextAvailable());
        rateInfo.setRateLimitRemaining(psnRsp.getRateLimitRemaining());
        rateInfo.setRateLimitTimePeriod(psnRsp.getRateLimitTimePeriod());
        rateInfo.setContext(extraContextInfo);
    }

    /*! ************************************************************************************************/
    /*! \brief helper to load image from file. Post: image.getSize() returns size of image loaded (up to maxImageSize).
    ***************************************************************************************************/
    bool ExternalSessionUtilPs4::loadImageDataFromFile(Ps4NpSessionImage& imageData, const ExternalSessionImageHandle& fileName, size_t maxImageSize)
    {
        // disabled if empty
        if (fileName.length() == 0)
            return true;

        // pre: must be under the working directory (typically etc/)
        eastl::string path;
        path.sprintf("%s%c%s", gController->getCurrentWorkingDirectory(), EA_FILE_PATH_SEPARATOR_8, fileName.c_str());

        size_t imgSize = EA::IO::File::GetSize(path.c_str());
        if (imgSize > maxImageSize)
        {
            ERR_LOG("[ExternalSessionUtilPs4].loadImageDataFromFile: image file at path(" << path.c_str() << ") has size(" << imgSize << ") which is greater than the max allowed size(" << maxImageSize << "). Image cannot be used.");
            return false;
        }
        EA::IO::FileStream fs(path.c_str());
        if (!fs.Open())
        {
            ERR_LOG("[ExternalSessionUtilPs4].loadImageDataFromFile: Unable to open image file at expected path(" << path.c_str() << "). ExternalSessionService will not be created.");
            return false;
        }
        imageData.resize(imgSize);
        imageData.setCount(imgSize);
        size_t readSize = fs.Read(imageData.getData(), imgSize);
        fs.Close();
        if (readSize != imgSize)
        {
            ERR_LOG("[ExternalSessionUtilPs4].loadImageDataFromFile: Internal error, unable to read image file at path(" << path.c_str() << "). ExternalSessionService will not be created.");
            return false;
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief validation helper for the default image file extension.
    ***************************************************************************************************/
    bool ExternalSessionUtilPs4::isValidImageFileExtension(const ExternalSessionImageHandle& fileName,
        const char8_t* validExtensions[], size_t validExtensionsSize)
    {
        const eastl::string& fileNameStr = fileName.c_str();

        size_t pos = fileNameStr.find_last_of(".");
        eastl::string ext = ((pos == eastl::string::npos)? "" : fileNameStr.substr(pos + 1));
        eastl::string errMsg = "Valid extensions:";
        for (size_t i = 0; i < validExtensionsSize; ++i)
        {
            if (ext.comparei(validExtensions[i]) == 0)
                return true;
            errMsg.append_sprintf(" %s", validExtensions[i]);
        }
        ERR_LOG("[ExternalSessionUtilPs4].isValidImageFileExtension: Invalid file extension(" << ext.c_str() << ") for file(" << fileNameStr.c_str() << "). " << errMsg.c_str());
        return false;
    }
        
    /*! ************************************************************************************************/
    /*! \brief validate the image data is JPEG, and size is <= max image size
    ***************************************************************************************************/
    bool ExternalSessionUtilPs4::isValidImageData(const Ps4NpSessionImage& imageData, size_t maxImageSize)
    {
        // Pre: imageData.getSize is correct size of the image.
        const uint8_t* imgBuf = imageData.getData();
        size_t imgSize = imageData.getCount();

        // JPEG specification
        const uint8_t jpegBeg[] = {0xff, 0xd8};
        const uint8_t jpegEnd[] = {0xff, 0xd9};

        const size_t minSize = sizeof(jpegBeg) + sizeof(jpegEnd) + 1;
        if ((imgSize < minSize) || (imgSize > maxImageSize))
        {
            ERR_LOG("[ExternalSessionUtilPs4].isValidImageData: the image size(" << imgSize << ") is too " << ((imgSize < minSize)? "small" : "large") << ". Must be between (" << minSize << ") and (" << maxImageSize << ") bytes.");
            return false;
        }

        if ((jpegBeg[0] != imgBuf[0]) || (jpegBeg[1] != imgBuf[1]) || 
            (jpegEnd[0] != imgBuf[imgSize-2]) || (jpegEnd[1] != imgBuf[imgSize-1]))
        {
            ERR_LOG("[ExternalSessionUtilPs4].isValidImageData: the image cannot be used, due to invalid data signature. JPEG magic numbers were required(" << jpegBeg[0] << "," << jpegBeg[1] << " .. " << jpegEnd[0] << "," << jpegEnd[1] <<
                "). Actual(" << imgBuf[0] << "," << imgBuf[1] << " .. " << imgBuf[imgSize-2] << "," << imgBuf[imgSize-1] << ").");
            return false;
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief if mock platform is enabled, update client-specified title id as needed
    ***************************************************************************************************/
    const char8_t* ExternalSessionUtilPs4::updateTitleIdIfMockPlatformEnabled(const char8_t* origTitleId) const
    {
        if (((origTitleId == nullptr) || (origTitleId[0] == '\0')) && isMockPlatformEnabled())
        {
            return "mockClientTitleId";
        }
        return origTitleId;
    }

    /*! ************************************************************************************************/
    /*! \brief reconfigure the default session image
    ***************************************************************************************************/
    void ExternalSessionUtilPs4::setImageDefault(const ExternalSessionImageHandle& value)
    {
        // cache image from file
        if (!loadDefaultImage(mDefaultImage, value, mConfig))
        {
            ERR_LOG("[ExternalSessionUtilPs4].setImageDefault: Not reconfiguring default image, validation of the new configuration image or its file(" << value.c_str() << ") failed.");
            return;
        }
        ExternalSessionUtil::setImageDefault(value);
    }
    /*! ************************************************************************************************/
    /*! \brief reconfigure the session status valid locales
    ***************************************************************************************************/
    void ExternalSessionUtilPs4::setStatusValidLocales(const char8_t* value)
    {
        if (!loadSupportedLocalesSet(mSupportedLocalesSet, value))
        {
            ERR_LOG("[ExternalSessionUtilPs4].setStatusValidLocales: Not reconfiguring status locales, validation of new configuration status locales(" << ((value != nullptr)? value : "<nullptr>") << ") failed.");
            return;
        }
        ExternalSessionUtil::setStatusValidLocales(value);
    }




    ////////////////////////////// PSN Connection //////////////////////////////////////////

    /*! ************************************************************************************************/
    /*! \brief Call to PSN
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::callPSN(const UserIdentification& caller, const CommandInfo& cmdInfo, const ExternalUserAuthInfo& authInfo,
        PsnWebApiHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, ExternalSessionErrorInfo* errorInfo /*= nullptr*/,
        const char8_t* contentType /*= getContentTypeFromEncoderType(Encoder::JSON)*/, bool addEncodedPayload /*= false*/)
    {
        PSNServices::PsnErrorResponse rspErr;
        eastl::string tokenBuf;

        BlazeRpcError err = callPSNInternal(caller, cmdInfo, req, rsp, rspRaw, &rspErr, contentType, addEncodedPayload);
        if (isConnectionExpiredError(err))
        {
            err = callPSNInternal(caller, cmdInfo, req, rsp, rspRaw, &rspErr, contentType, addEncodedPayload, true);//true refreshes BaseUrl
        }
        if (isTokenExpiredError(err, rspErr))
        {
            BlazeRpcError authTokenErr = getAuthTokenInternal(caller, authInfo.getServiceName(), tokenBuf, true);
            if (OAUTH_ERR_INVALID_REQUEST == authTokenErr)
            {
                //no point to retry when bad request happens as retry will always fail too
                return authTokenErr;
            }
            else if (ERR_OK == authTokenErr)
            {
                reqHeader.setAuthToken(tokenBuf.c_str());
                err = callPSNInternal(caller, cmdInfo, req, rsp, rspRaw, &rspErr, contentType, addEncodedPayload);
            }
        }
        for (size_t i = 0; (isRetryError(err, rspErr) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
        {
            if (ERR_OK != (err = waitTime(mConfig.getCallOptions().getServiceUnavailableBackoff(), cmdInfo.context, i + 1)))
                return err;
            err = callPSNInternal(caller, cmdInfo, req, rsp, rspRaw, &rspErr, contentType, addEncodedPayload);
        }

        // in case expired at a retry
        if (EA_UNLIKELY(isConnectionExpiredError(err)))
        {
            err = callPSNInternal(caller, cmdInfo, req, rsp, rspRaw, &rspErr, contentType, addEncodedPayload, true);//true refreshes BaseUrl
        }
        if (EA_UNLIKELY(isTokenExpiredError(err, rspErr)))
        {
            BlazeRpcError authTokenErr = getAuthTokenInternal(caller, authInfo.getServiceName(), tokenBuf, true);
            if (OAUTH_ERR_INVALID_REQUEST == authTokenErr)
            {
                //no point to retry when bad request happens as retry will always fail too
                return authTokenErr;
            }
            else if (ERR_OK == authTokenErr)
            {
                reqHeader.setAuthToken(tokenBuf.c_str());
                err = callPSNInternal(caller, cmdInfo, req, rsp, rspRaw, &rspErr, contentType, addEncodedPayload);
            }
        }

        if (err != ERR_OK)
        {
            // Error is logged by caller as well, hence lowering verbosity here
            TRACE_LOG("[ExternalSessionUtilPs4].callPSN: " << ((cmdInfo.context != nullptr) ? cmdInfo.context : "PSN") << " call failed. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << "). " << toLogStr(err, &rspErr) << ".");
            
            if (errorInfo != nullptr)
                toExternalSessionErrorInfo(rspErr, *errorInfo, ((cmdInfo.context != nullptr) ? cmdInfo.context : ""));
        }
        return err;
    }

    BlazeRpcError ExternalSessionUtilPs4::callPSNInternal(const UserIdentification& caller, const CommandInfo& cmdInfo,
        const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, RawBuffer* rspRaw, PSNServices::PsnErrorResponse* rspErr,
        const char8_t* contentType, bool addEncodedPayload, bool refreshBaseUrl /*= false*/)
    {
        BlazeRpcError err = ERR_OK;
        HttpConnectionManagerPtr connMgr = getWebApiConnMgr(caller, cmdInfo, refreshBaseUrl, err);
        if ((connMgr == nullptr) || EA_UNLIKELY(cmdInfo.componentInfo == nullptr))
        {
            return err;//logged
        }
        TimeValue startTime = TimeValue::getTimeOfDay();
        HttpStatusCode statusCode = 0;
        err = RestProtocolUtil::sendHttpRequest(connMgr, cmdInfo.componentInfo->id, cmdInfo.commandId, &req, contentType, addEncodedPayload,
            rsp, rspErr, nullptr, nullptr, &statusCode, cmdInfo.componentInfo->baseInfo.index, RpcCallOptions(), rspRaw);
        TimeValue endTime = TimeValue::getTimeOfDay();

        metricCommandStatus(cmdInfo, endTime - startTime, statusCode);
        return err;
    }

    void ExternalSessionUtilPs4::metricCommandStatus(const CommandInfo& cmdInfo, const TimeValue& duration, HttpStatusCode statusCode) const
    {
        if ((gOutboundHttpService != nullptr) && (cmdInfo.componentInfo != nullptr) && (cmdInfo.componentInfo->name != nullptr))
        {
            // track metrics with the configured connection, not the transient ones
            HttpConnectionManagerPtr configConnMgr = gOutboundHttpService->getConnection(cmdInfo.componentInfo->name);
            ASSERT_COND_LOG((configConnMgr != nullptr), "[ExternalSessionUtilPs4].metricCommandStatus: failed to update metric, couldn't find the configured connection manager for the component");
            if (configConnMgr != nullptr)
                configConnMgr->metricProxyComponentRequest(cmdInfo.componentInfo->id, cmdInfo.commandId, duration, statusCode);
            return;
        }
        ASSERT_LOG("[ExternalSessionUtilPs4].metricCommandStatus: " << ((gOutboundHttpService == nullptr) ? "gOutboundHttpService" : "component info") << " missing, can't update metrics.");
    }

    /*! ************************************************************************************************/
    /*! \brief get connection manager for the PSN command
        \param[in] refreshBaseUrl true to force refresh the Web API Base URL and connection manager
    ***************************************************************************************************/
    HttpConnectionManagerPtr ExternalSessionUtilPs4::getWebApiConnMgr(const UserIdentification& caller, const CommandInfo& cmdInfo,
        bool refreshBaseUrl, BlazeRpcError& err)
    {
        if (EA_UNLIKELY((cmdInfo.componentInfo == nullptr) || (cmdInfo.componentInfo->name == nullptr)))
        {
            ASSERT_LOG("[ExternalSessionUtilPs4].getWebApiConnMgr: command's component" << ((cmdInfo.componentInfo == nullptr) ? " info" : "") << ((cmdInfo.componentInfo->name == nullptr) ? "/name" : "") << " was nullptr, cannot complete request.");
            err = ERR_SYSTEM;
            return nullptr;
        }
        // get connection's configured settings to use
        HttpServiceConnectionMap::const_iterator itr = gController->getFrameworkConfigTdf().getHttpServiceConfig().getHttpServices().find(cmdInfo.componentInfo->name);
        const HttpServiceConnection* config = ((itr != gController->getFrameworkConfigTdf().getHttpServiceConfig().getHttpServices().end()) ? itr->second : nullptr);
        if (config == nullptr)
        {
            WARN_LOG("[ExternalSessionUtilPs4].getWebApiConnMgr: Couldn't fetch configuration for service(" << cmdInfo.componentInfo->name << "). Default values may be used. Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
        }

        // currently, this util is only implemented for 'sdk::sessionInvitation' (and using mCachedSessionInvitationConnManager below)
        ASSERT_COND_LOG((cmdInfo.componentInfo->id == PSNServices::PSNSessionInvitationSlave::COMPONENT_INFO.id), "[ExternalSessionUtilPs4].getWebApiConnMgr: unexpected state: command component(" << cmdInfo.componentInfo->name << ") was invalid, only PSN session invitation requests are implemented for this util currently.");

        // init or refresh the cached conn mgr as needed
        bool isConfigChanged = ((config != nullptr) && (mCachedSessionInvitationConnManager != nullptr) && (config->getNumPooledConnections() != mCachedSessionInvitationConnManager->maxConnectionsCount()));
        if ((mCachedSessionInvitationConnManager == nullptr) || refreshBaseUrl || isConfigChanged)
        {
            err = getNewWebApiConnMgr(caller, "sdk:sessionInvitation", config, mCachedSessionInvitationConnManager);
            if ((mCachedSessionInvitationConnManager == nullptr) || (err != ERR_OK))
            {
                ERR_LOG("[ExternalSessionUtilPs4].getWebApiConnMgr: Failed to get connection manager for command(" << (cmdInfo.name != nullptr ? cmdInfo.name : "<name N/A>") << "), component(" << cmdInfo.componentInfo->name << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ")." << toLogStr(err, nullptr) << ".");
                return nullptr;
            }
        }
        return mCachedSessionInvitationConnManager;
    }

    /*! ************************************************************************************************/
    /*! \brief get connection manager for the PSN Web API
        \param[in,out] connMgr the connection manager to init or refresh
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::getNewWebApiConnMgr(const UserIdentification& caller, const char8_t* psnWebApi,
        const HttpServiceConnection* config, HttpConnectionManagerPtr& connMgr)
    {
        TRACE_LOG("[ExternalSessionUtilPs4].getNewWebApiConnMgr: " << ((connMgr != nullptr)? "refreshing" : "initializing") << " server connection for api(" << psnWebApi << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
        eastl::string baseUrl;
        BlazeRpcError err = getBaseUrl(caller, psnWebApi, baseUrl);
        if (ERR_OK != err)
        {
            return err;//logged
        }

        const char8_t* hostname = nullptr;
        bool secure = false;
        HttpProtocolUtil::getHostnameFromConfig(baseUrl.c_str(), hostname, secure);
        OutboundHttpConnectionManager* newConnMgr = BLAZE_NEW OutboundHttpConnectionManager(hostname);
        if (config == nullptr)
            newConnMgr->initialize(InetAddress(hostname), 256, secure);
        else
        {
            const char8_t* proxyHostname = nullptr;
            bool proxySecure = false;
            HttpProtocolUtil::getHostnameFromConfig(config->getProxyUrl(), proxyHostname, proxySecure);
            newConnMgr->initialize(InetAddress(hostname), InetAddress(proxyHostname), config->getNumPooledConnections(), secure);
        }

        if (connMgr != nullptr)
        {
            connMgr.reset();
        }
        connMgr = HttpConnectionManagerPtr(newConnMgr);

        TRACE_LOG("[ExternalSessionUtilPs4].getNewWebApiConnMgr: server connection for api(" << psnWebApi << ") set to address(" << baseUrl.c_str() << ").");
        return ERR_OK;
    }

    ////////////////////////////// Base URL ///////////////////////////////////////////////

    /*! ************************************************************************************************/
    /*! \brief retrieves the PSN Base URL. Called when refreshing the cached Base URL.
        \param[in] apiName the Web API to fetch the Base URL for.
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionUtilPs4::getBaseUrl(const UserIdentification& caller, const char8_t* apiName, eastl::string& baseUrlBuf)
    {
        if (EA_UNLIKELY(isDisabled()))
        {
            TRACE_LOG("[ExternalSessionUtilPs4].getBaseUrl: External Base URL service disabled.");
            return ERR_OK;
        }
        if (EA_UNLIKELY((apiName == nullptr) || (apiName[0] == '\0')))
        {
            WARN_LOG("[ExternalSessionUtilPs4].getBaseUrl: External Base URL service call will not be called for api " <<
                (((apiName == nullptr) || (apiName[0] == '\0'))? "that's missing" : apiName) << "! Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return ERR_OK;
        }
        // side: Base URL refreshes aren't frequent, token fetch
        eastl::string authToken;
        BlazeRpcError err = getAuthTokenInternal(caller, gController->getDefaultServiceName(), authToken, true);
        if (err != ERR_OK)
        {
            ERR_LOG("[ExternalSessionUtilPs4].getBaseUrl: Failed to fetch auth token, cannot fetch Base URL for api(" << apiName << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return err; 
        }

        Blaze::PSNServices::PSNBaseUrlSlave* psnBaseUrlSlave = (Blaze::PSNServices::PSNBaseUrlSlave *)Blaze::gOutboundHttpService->getService(Blaze::PSNServices::PSNBaseUrlSlave::COMPONENT_INFO.name);
        if (psnBaseUrlSlave == nullptr)
        {
            ERR_LOG("[ExternalSessionUtilPs4].getBaseUrl: Failed to instantiate Base URL proxy, cannot fetch Base URL for api(" << apiName << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
            return ERR_SYSTEM;
        }

        Blaze::PSNServices::PsnErrorResponse rspErr;
        Blaze::PSNServices::GetBaseUrlResponse rsp;
        Blaze::PSNServices::GetBaseUrlRequest req;
        req.setApiGroup(apiName);
        req.getHeader().setAuthToken(authToken.c_str());

        err = psnBaseUrlSlave->getBaseUrl(req, rsp, rspErr);
        for (size_t i = 0; (isRetryError(err, rspErr) && (i <= mConfig.getCallOptions().getServiceUnavailableRetryLimit())); ++i)
        {
            if (ERR_OK != (err = waitTime(mConfig.getCallOptions().getServiceUnavailableBackoff(), "[ExternalSessionUtilPs4].getBaseUrl", i + 1)))
                return err;
            err = psnBaseUrlSlave->getBaseUrl(req, rsp, rspErr);
        }

        if (err != ERR_OK)
        {
            ERR_LOG("[ExternalSessionUtilPs4].getBaseUrl: Failed to get Base URL for api(" << apiName << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << "). " << toLogStr(err, &rspErr) << ".");
        }
        else
        {
            // save result
            if (EA_UNLIKELY((rsp.getUrl() == nullptr) || (rsp.getUrl()[0] == '\0')))
            {
                ERR_LOG("[ExternalSessionUtilPs4].getBaseUrl: Got empty Base URL for api(" << apiName << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
                return ERR_SYSTEM;
            }
            baseUrlBuf.sprintf("%s", rsp.getUrl());
            TRACE_LOG("[ExternalSessionUtilPs4].getBaseUrl: Got Base URL(" << baseUrlBuf.c_str() << ") for api(" << apiName << "). Caller(" << externalUserIdentificationToString(caller, mUserIdentLogBuf) << ").");
        }
        return err;
    }



    /*! ************************************************************************************************/
    /*! \brief log helper
    ***************************************************************************************************/
    const char8_t* ExternalSessionUtilPs4::toLogStr(BlazeRpcError err, const Blaze::PSNServices::PsnErrorResponse *psnRsp /*= nullptr*/) const
    {
        mErrorLogBuf.sprintf("BlazeError(%s, '%s')", ErrorHelp::getErrorName(err), ErrorHelp::getErrorDescription(err));
        if (psnRsp != nullptr)
        {
            const int64_t currentTime = TimeValue::getTimeOfDay().getSec();
            const int64_t nextAvailTime = (int64_t)psnRsp->getRateLimitNextAvailable();
            mErrorLogBuf.append_sprintf(", serviceError(%i, '%s'), serviceRateLimitInfo(for time period(%s), callsAllowed(%u), callsRemaining(%u), nextPossCall in(%" PRIi64 "s), next poss call time(%" PRIi64 "s))",
                psnRsp->getError().getCode(), psnRsp->getError().getMessage(), psnRsp->getRateLimitTimePeriod(), psnRsp->getRateLimitLimit(), psnRsp->getRateLimitRemaining(), ((nextAvailTime > currentTime)? nextAvailTime - currentTime : 0), nextAvailTime).c_str();
        }
        return mErrorLogBuf.c_str();
    }
    const char8_t* ExternalSessionUtilPs4::toLogStr(const ExternalSessionBinaryDataHead& dataHead, const ExternalSessionCustomData* titleData) const
    {
        eastl::string dataStrBuf;
        size_t dataSize = sizeof(dataHead) + ((titleData != nullptr) ? titleData->getCount() : 0);
        mBinaryDataLogBuf.sprintf("session-data(%" PRIu64 " bytes: including header(%" PRIu64 " bytes: raw(%s), parsed(gameId(%" PRIu64 "), gameType(%s=%d)))", dataSize, sizeof(dataHead), toLogStr((uint8_t*)&dataHead, sizeof(dataHead), dataStrBuf), (GameId)dataHead.iGameId, GameTypeToString((GameType)dataHead.iGameType), (GameType)dataHead.iGameType);
        if (titleData != nullptr)
        {
            const size_t maxLoggedBytes = 64;
            mBinaryDataLogBuf.append_sprintf(", and title data(%u bytes: raw(%s%s))", titleData->getCount(), toLogStr((uint8_t*)titleData->getData(), eastl::min((EA::TDF::TdfSizeVal)maxLoggedBytes, titleData->getCount()), dataStrBuf), (maxLoggedBytes < titleData->getCount() ? "..." : ""));
        }
        return mBinaryDataLogBuf.append_sprintf(")").c_str();
    }
    const char8_t* ExternalSessionUtilPs4::toLogStr(const ExternalSessionStatus& status) const
    {
        mLocStatusLogBuf.sprintf("unlocalizedDefault(%s), numLocalizedStatuses(%" PRIu64 ")", status.getUnlocalizedValue(), status.getLangMap().size());
        if (!status.getLangMap().empty())
        {
            mLocStatusLogBuf.append_sprintf(",localizedSessionStatuses=[");
            for (Ps4NpSessionStatusStringLocaleMap::const_iterator itr = status.getLangMap().begin(), end = status.getLangMap().end(); itr != end; ++itr)
            {
                //last element doesn't need comma
                bool isLast = (++itr == end);
                --itr;
                mLocStatusLogBuf.append_sprintf("[%s]='%s'%s", (*itr).first.c_str(), (*itr).second.c_str(), (isLast? "]" : ","));
            }
        }
        return mLocStatusLogBuf.c_str();
    }

    const char8_t* ExternalSessionUtilPs4::toLogStr(const uint8_t* bytes, size_t bytesLen, eastl::string& buf) const
    {
        buf.clear();
        for(size_t i = 0; i < bytesLen; ++i)
            buf.append_sprintf(" %02X", bytes[i]);
        return buf.c_str();
    }
}//GameManager
}//Blaze

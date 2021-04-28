/*! ************************************************************************************************/
/*!
    \file externalsessionmemberstrackinghelper.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/externalsessions/externalsessionmemberstrackinghelper.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h"
#include "gamemanager/rpc/gamemanager_defines.h"
#include "framework/logger.h"

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief Cross-play and cross-gen games could have members in different platform-specific 1st party
        presence sessions, and, in cases like PS5's PlayerSessions and Matches, different poss types of
        activities under the platform. Blaze uses a separate tracking list for each platform/type.

        Each instance of this util deals with updating only the tracking list for this method's specified platform and activity type.

        \param[in] activityTypePlatform  Platform 'name space' the tracked activities belong to.
        \param[in] crossgenClientPlatform Add the client platform this util manages, if crossgen enabled for logging etc.
    ***************************************************************************************************/
    ExternalSessionMembersTrackingHelper::ExternalSessionMembersTrackingHelper(GameManagerSlaveImpl* gameManagerSlave,
        ClientPlatformType activityTypePlatform,
        ExternalSessionActivityType activityType,
        ClientPlatformType crossgenClientPlatform /*= INVALID*/)
        : mGameManagerSlave(gameManagerSlave)
    {
        mExternalSessionActivityTypeInfo.setPlatform(activityTypePlatform);
        mExternalSessionActivityTypeInfo.setType(activityType);
        mExternalSessionActivityTypeInfo.setCrossgenClientPlatform(crossgenClientPlatform);
    }

    /*! ************************************************************************************************/
    /*! \brief To ensure its Blaze Game and 1st party session and members in sync, call after adding user to a 1st party session,
        to update the corresponding memberships tracking on the GameManager master.
        In particular on PS, where only one session might exist at a time, and a Game can change or remove its session, this tracking is used to ensure things keep in sync.
        To fetch the tracking, call getGameInfoFromMaster().

        \param[out] result game's 1st party session info, after the track attempt.
        \param[in] gameInfo the Blaze game's info. Note: this may be a snapshot taken before the first party session update
        \param[in] extSessionToAdvertise the game's expected 1st party session, to track as having the user.
        \return If game's 1st party session was not the one in request, GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION. Else an appropriate code
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionMembersTrackingHelper::trackExtSessionMembershipOnMaster(
        GetExternalSessionInfoMasterResponse& result,
        const GetExternalSessionInfoMasterResponse& gameInfo, const ExternalMemberInfo& member,
        const ExternalSessionIdentification& extSessionToAdvertise) const
    {
        if (EA_UNLIKELY(getMaster() == nullptr))
        {
            return ERR_SYSTEM;//logged
        }
        const auto& forType = getExternalSessionActivityTypeInfo();

        GameId gameId = gameInfo.getExternalSessionCreationInfo().getExternalSessionId();
        TRACE_LOG(logPrefix() << ".trackExtSessionMembershipOnMaster: tracking user(" << toExtUserLogStr(member.getUserIdentification()) << ") as member of session(" << toLogStr(extSessionToAdvertise, forType) << "), activity(" << ExternalSessionActivityTypeToString(forType.getType()) << ":namespace(" << ClientPlatformTypeToString(forType.getPlatform()) << ")) for game(" << gameId << ").");

        GetExternalSessionInfoMasterResponse errorRsp;
        TrackExtSessionMembershipRequest req;
        req.setGameId(gameId);
        forType.copyInto(req.getExtSessType());
        member.copyInto(req.getUser());
        extSessionToAdvertise.copyInto(req.getExtSessionToAdvertise());
        gameInfo.getExternalSessionCreationInfo().getUpdateInfo().copyInto(req.getExtSessionOrigUpdateInfo());

        BlazeRpcError err = getMaster()->trackExtSessionMembershipMaster(req, result, errorRsp);

        if (err == GAMEMANAGER_ERR_INVALID_GAME_ID)
        {
            TRACE_LOG(logPrefix() << ".trackExtSessionMembershipOnMaster: game(" << gameId << ") went away before we could track user(" << toExtUserLogStr(member.getUserIdentification()) << ") as being in its session(" << toLogStr(req.getExtSessionToAdvertise(), forType) << "), activity(" << ExternalSessionActivityTypeToString(forType.getType()) << "). No op (user's primary external session will be appropriately updated when Blaze handles the game's removal).");
        }
        else if (err == GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION)
        {
            TRACE_LOG(logPrefix() << ".trackExtSessionMembershipOnMaster: game(" << gameId << ") 1st party session was updated to be(" << toLogStr(req.getExtSessionToAdvertise(), forType) << "), before we could track user(" << toExtUserLogStr(member.getUserIdentification()) << ") as being in old session(" << toLogStr(extSessionToAdvertise, forType) << "), activity(" << ExternalSessionActivityTypeToString(forType.getType()) << "). Joining game's current external session activity.");
            errorRsp.copyInto(result);
        }
        else if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".trackExtSessionMembershipOnMaster: internal error: failed to track game(" << gameId << ") user(" << toExtUserLogStr(member.getUserIdentification()) << ") as being in session(" << toLogStr(req.getExtSessionToAdvertise(), forType) << "), activity(" << ExternalSessionActivityTypeToString(forType.getType()) << "), err " << ErrorHelp::getErrorName(err) << ".");
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief track user as not advertising member of the 1st party session
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionMembersTrackingHelper::untrackExtSessionMembershipOnMaster(
        UntrackExtSessionMembershipResponse* result,
        GameId gameId, const ExternalMemberInfo* member, 
        const ExternalSessionIdentification* extSessToUntrackAll) const
    {
        const auto& forType = getExternalSessionActivityTypeInfo();

        TRACE_LOG(logPrefix() << ".untrackExtSessionMembershipOnMaster: untracking (" << (member ? toExtUserLogStr(member->getUserIdentification()) : "<all>") << ") from game(" << gameId << ") external session activity(" << ExternalSessionActivityTypeToString(forType.getType()) << ").");
        if (EA_UNLIKELY(getMaster() == nullptr))
            return ERR_SYSTEM;//logged

        UntrackExtSessionMembershipRequest req;
        req.setGameId(gameId);
        forType.copyInto(req.getExtSessType());
        if (member)
        {
            member->copyInto(req.getUser());
        }
        if (extSessToUntrackAll)
        {
            extSessToUntrackAll->copyInto(req.getExtSessionToUntrackAll());
        }

        // Super user privilege in case user logged out
        UserSession::SuperUserPermissionAutoPtr permissionPtr(true);
        UntrackExtSessionMembershipResponse tmpResponse;
        BlazeRpcError err = getMaster()->untrackExtSessionMembershipMaster(req, (result ? *result : tmpResponse));
        if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".untrackExtSessionMembershipOnMaster: failed to untrack advertiser/membership in game(" << gameId << ") external session activity(" << ExternalSessionActivityTypeToString(forType.getType()) << "). No op. err " << ErrorHelp::getErrorName(err) << ".");
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief Gets the GameManager master's cached info for the game and its external session
    ***************************************************************************************************/
    BlazeRpcError ExternalSessionMembersTrackingHelper::getGameInfoFromMaster(GetExternalSessionInfoMasterResponse& gameInfo,
        GameId gameId, const UserIdentification& caller) const
    {
        if (EA_UNLIKELY(getMaster() == nullptr))
            return ERR_SYSTEM;//logged

        GetExternalSessionInfoMasterRequest getOrigExtSessionInfoReq;
        caller.copyInto(getOrigExtSessionInfoReq.getCaller());
        getOrigExtSessionInfoReq.setGameId(gameId);

        BlazeRpcError err = getMaster()->getExternalSessionInfoMaster(getOrigExtSessionInfoReq, gameInfo);
        if (err == GAMEMANAGER_ERR_INVALID_GAME_ID)
        {
            TRACE_LOG(logPrefix() << ".getGameInfoFromMaster: game(" << gameId << ") doesn't exist, activity(" << ExternalSessionActivityTypeToString(getExternalSessionActivityTypeInfo().getType()) << ")");
        }
        else if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".getGameInfoFromMaster: Failed to retrieve external session data for game(" << gameId << "), activity(" << ExternalSessionActivityTypeToString(getExternalSessionActivityTypeInfo().getType()) << "). Caller(" << toExtUserLogStr(caller) << "). " << ErrorHelp::getErrorName(err) << ").");
        }
        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief (Optional) For avoiding extra (1P) rate limited calls in join flows, returns whether joining the user
        to game's external session/1st Party activity is actually required, based on game's cached memberships tracking.
        \param[out] gameInfo if there's a pre existing ext session activity for user found, its populated here.
    ***************************************************************************************************/
    bool ExternalSessionMembersTrackingHelper::isAddUserToExtSessionRequired(GetExternalSessionInfoMasterResponse& gameInfo,
        GameId gameId, const UserIdentification& user) const
    {
        const auto& forType = getExternalSessionActivityTypeInfo();//ext session/activity type potentially joining

        BlazeRpcError err = getGameInfoFromMaster(gameInfo, gameId, user);

        // can early out if GM knows the user is already advertising the session
        const auto* trackedMembers = getListFromMap(gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getTrackedExternalSessionMembers(), forType);
        if (trackedMembers == nullptr)
            return true;
        if ((err == ERR_OK) && (nullptr != getUserInExternalMemberInfoList(user.getBlazeId(), *trackedMembers)))
        {
            TRACE_LOG(logPrefix() << ".isAddUserToExtSessionRequired: no add required, already has the correct session for game(" << gameId << "), session(" << toLogStr(gameInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification(), forType) << ") activity(" << ExternalSessionActivityTypeToString(forType.getType()) << "), for caller(" << toExtUserLogStr(user) << ").");
            return false;
        }
        // ok if target game was left, its remove notifications will trigger re-updating latest game presence for user soon enough
        else if (((err == ERR_OK) && !gameInfo.getIsGameMember()) || (err == GAMEMANAGER_ERR_INVALID_GAME_ID))
        {
            err = ERR_OK;
            return false;
        }
        else if (err != ERR_OK)
        {
            return false;
        }
        return true;
    }

    GameManagerMaster* ExternalSessionMembersTrackingHelper::getMaster() const
    {
        if ((mGameManagerSlave == nullptr) || (mGameManagerSlave->getMaster() == nullptr))
        {
            ERR_LOG(logPrefix() << ".getMaster: Game Manager " << ((mGameManagerSlave == nullptr) ? "Slave" : "Master") << " missing. Unable to complete operation.");
            return nullptr;
        }
        return mGameManagerSlave->getMaster();
    }

}//ns GameManager
}//ns Blaze


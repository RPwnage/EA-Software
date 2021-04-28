/*! ************************************************************************************************/
/*!
    \file   matchmakingsession.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "framework/tdf/attributes.h"
#include "framework/util/random.h"
#include "framework/usersessions/usersession.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/matchmaker/matchmaker.h"
#include "gamemanager/matchmakercomponent/matchmakerslaveimpl.h"

#include "gamemanager/matchmaker/matchmakingutil.h" // for LOG_PREFIX
#include "gamemanager/commoninfo.h"
#include "gamemanager/rpc/gamemanagermaster_stub.h"
#include "gamemanager/tdf/gamemanager_server.h"

#include "gamemanager/matchmaker/rules/gameattributerule.h"
#include "gamemanager/matchmaker/rules/playerattributerule.h"
#include "gamemanager/matchmaker/rules/teamcompositionrule.h"
#include "gamemanager/tdf/matchmaker_component_config_server.h" // for MatchmakerConfig in initCreateGameRequest
#include "gamemanager/matchmaker/creategamefinalization/matchedsessionsbucketutil.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationsettings.h"
#include "gamemanager/gamemanagerhelperutils.h"
#include "gamemanager/gamemanagervalidationutils.h" // for validatePlayerRolesForCreateGame in initCreateGameRequest
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"

// event management
#include "gamemanager/tdf/gamemanagerevents_server.h"
#include "framework/event/eventmanager.h"
#include "gamemanager/pinutil/pinutil.h"

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    bool FullSessionThan::operator()(const MatchmakingSession* a, const MatchmakingSession* b) const
    {
        return (a->getMMSessionId() < b->getMMSessionId());
    }

    static const char8_t* FINALIZATION_CONDITIONS_NAME = "Finalization Conditions";

    /*! ************************************************************************************************/
    /*!    \brief Sessions must be initialized after creation; 0 is an invalid session id.

        \param[in]    id - The sessionId to use.  Zero is invalid.
        \param[in]    parentSession - A pointer to parent MM session of this MM session, if it's nullptr, means ths session is a single MM session
        \param[in]    userSession  - user session which is the owner the MM session (start the MM session)
    *************************************************************************************************/
    MatchmakingSession::MatchmakingSession(Blaze::Matchmaker::MatchmakerSlaveImpl &matchmakerSlave, Matchmaker &matchmaker,
                                            MatchmakingSessionId id, MatchmakingScenarioId owningScenarioId)
        : mMatchmakerSlave(matchmakerSlave),
          mMatchmaker(matchmaker),
          mMMSessionId(id),
          mOwningScenarioId(owningScenarioId),
          mPrimaryMemberInfo(nullptr),
          mIndirectOwnerMemberInfo(nullptr),
          mSessionStartTime(0),
          mScenarioStartTime(0),
          mTotalSessionDuration(0),
          mStartingDecayAge(0),
          mSessionAgeSeconds(0),
          mNextElapsedSecondsUpdate(UINT32_MAX),
          mSessionTimeoutSeconds(0),
          mFailedConnectionAttempts(0),
          mQosTier(0),
          mMatchmakingAsyncStatus(),
          mCriteria(matchmaker.getRuleDefinitionCollection(), mMatchmakingAsyncStatus),
          mDebugCheckOnly(false),
          mDebugCheckMatchSelf(false),
          mDebugFreezeDecay(false),
          mMemberInfoCount(0),
          mMembersUserSessionInfo(DEFAULT_BLAZE_ALLOCATOR, "MembersUserSessionInfo"),
          mStatus(SESSION_NEW),
          mMatchmakingSupplementalData(MATCHMAKING_CONTEXT_MATCHMAKER_CREATE_GAME),
          mLastStatusAsyncTimestamp(0),
          mPrivilegedGameIdPreference(Blaze::GameManager::INVALID_GAME_ID),
          mCreateGameFinalizationFitScore(0),
          mFinalizingMatchmakingSession(INVALID_MATCHMAKING_SESSION_ID),
          mRoleEntryCriteriaEvaluators(BlazeStlAllocator("MatchmakingSession::mRoleCriteriaEvaluators", Blaze::Matchmaker::MatchmakerSlave::COMPONENT_MEMORY_GROUP)),
          mIsFinalStatusAsyncDirty(true),
          mMaxFitScore(0),
          mFitScore(0),
          mMatchmakedGameCapacity(0),
          mMatchmakedGamePlayerCount(0),
          mLastReinsertionIdle(0),
          mMarkedforCancel(false),
          mTrackingTag(""),
          mUsedCC(false),
          mAllMembersFullyConnected(true),
          mAllMembersPassedLatencyValidation(true),
          mAllMembersPassedPacketLossValidation(true),
          mCurrentMatchmakingSessionListPlayerCount(0),
          mTotalMatchmakingSessionListPlayerCount(0),
          mExpiryTimerId(INVALID_TIMER_ID),
          mStatusTimerId(INVALID_TIMER_ID)
    {
        mDiagnostics = BLAZE_NEW MatchmakingSubsessionDiagnostics;

        mMultiRoleEntryCriteriaEvaluator.setStaticRoleInfoSource(mGameCreationData.getRoleInformation());

        mMatchmaker.addSessionLock(this);

        mCriteria.setMatchmakingSession(this);
    }


    //! destructor
    MatchmakingSession::~MatchmakingSession()
    {
        // If the session was locked during destruction, we don't clear the lock, because in the case of matchmaking success, the other subsessions stick around
        // until the scenario manager on the core slave issues a cancel.
        // removing the lock in the destructor could lead to a scenario matching a user into multiple games, or at minimum, generating extra mp_match_join pin events
        // to track the cancellation of the last remaining subsession
        mMatchmaker.removeSessionLock(this);    // This destroys the locking structure itself if this was the last subsession to be destroyed.

        if (mExpiryTimerId != INVALID_TIMER_ID)
            gSelector->cancelTimer(mExpiryTimerId);

        if (mStatusTimerId != INVALID_TIMER_ID)
            gSelector->cancelTimer(mStatusTimerId);

        TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << ")] is destructing."); // TODO: change back to SPAM
        mMembersUserSessionInfo.clear();

        //Kill all our member info nodes
        while (!mMemberInfoList.empty())
        {
            MemberInfo &memberInfo = static_cast<MemberInfo &>(mMemberInfoList.front());
            mMemberInfoList.pop_front();
            removeUserSessionToMatchmakingMemberMapEntry(memberInfo);
            delete &memberInfo;
        }

        if (mIndirectOwnerMemberInfo != nullptr)
        {
            removeUserSessionToMatchmakingMemberMapEntry(*mIndirectOwnerMemberInfo);
            delete mIndirectOwnerMemberInfo;
            mIndirectOwnerMemberInfo = nullptr;
        }

        // free any parsed game entry criteria expressions
        clearCriteriaExpressions();
        // now clean up any role entry criteria
        RoleEntryCriteriaEvaluatorMap::iterator roleCriteriaIter = mRoleEntryCriteriaEvaluators.begin();
        RoleEntryCriteriaEvaluatorMap::iterator roleCriteriaEnd = mRoleEntryCriteriaEvaluators.end();
        for(; roleCriteriaIter != roleCriteriaEnd; ++roleCriteriaIter)
        {
            delete roleCriteriaIter->second;
            roleCriteriaIter->second = nullptr;
        }
        mRoleEntryCriteriaEvaluators.clear();

        // generic rule and skill rule objs are owned by each individual rule
        // instead of session. need to clear here to prevent double destruction
        mMatchmakingAsyncStatus.getGameAttributeRuleStatusMap().clear();
        mMatchmakingAsyncStatus.getPlayerAttributeRuleStatusMap().clear();
        mMatchmakingAsyncStatus.getUEDRuleStatusMap().clear();

        sessionCleanup();
        mMatchmaker.removeSession(*this);

        //  MM_TODO: required to explicitly cleanup criteria so Rule objects have access to mMatchmakingAsyncStatus prior to its implicit destruction.
        mCriteria.cleanup();
    }

    bool MatchmakingSession::MemberInfo::isPotentialHost() const 
    { 
        return (!mIsOptionalPlayer && gUserSessionManager->getSessionExists(mUserSessionInfo.getSessionId())); 
    }

    /*! ************************************************************************************************/
    /*!
        \brief Initialize the matchmaking session from a StartMatchmakingSessionRequest.  Returns true on success.

        \param[in] req - the startMatchmaking request (from the slave/client)
        \param[in] matchmakingSupplimentialData - needed to lookup the rule definitions and other data needed for matchmaking
        \param[out] err    We fill out the errMessage field if we encounter an error
        \param[in] dedicated server override map to determine where to matchmake the session for debugging
        \return true on success; false otherwise.
    *************************************************************************************************/
    bool MatchmakingSession::initialize(const StartMatchmakingInternalRequest &req, MatchmakingCriteriaError &err, const DedicatedServerOverrideMap &dedicatedServerOverrideMap)
    {
        mCachedMatchmakingRequestPtr = req.clone(); // use cached copy henceforth

        const StartMatchmakingInternalRequest &request = *mCachedMatchmakingRequestPtr;
        const StartMatchmakingRequest &startRequest = request.getRequest();
        // init & validate sessionMode
        if (!initSessionMode(startRequest, err))
        {
            return false;
        }

        if (req.getRequest().getGameCreationData().getIsExternalOwner())
        {
            StringBuilder buf;//prevent issues
            err.setErrMessage((buf << "Starting matchmaking using an external owner is not supported.").c_str());
            ERR_LOG("[MatchmakingSession].initialize: " << err.getErrMessage());
            return false;
        }

        // set our starting point for MM QoS criteria

        QosCriteriaMap::const_iterator qosCriteriaIter = request.getQosCriteria().find(request.getRequest().getGameCreationData().getNetworkTopology());
        if (qosCriteriaIter != request.getQosCriteria().end())
        {
            const ConnectionCriteria *connectionCriteria = qosCriteriaIter->second;
            mFailedConnectionAttempts = connectionCriteria->getFailedConnectionAttempts();
            mQosTier = connectionCriteria->getTier();

            // populate avoid lists next
            connectionCriteria->getAvoidGameIdList().copyInto(mQosGameIdAvoidList);
            connectionCriteria->getAvoidPlayerIdList().copyInto(mQosPlayerIdAvoidList);
        }

        mMatchmakingSupplementalData.mQosGameIdAvoidList = &mQosGameIdAvoidList;
        mMatchmakingSupplementalData.mQosPlayerIdAvoidList = &mQosPlayerIdAvoidList;

        // we use this list to ensure we don't put duplicate members into the MM session's info list.
        eastl::set<UserSessionId> memberUserSessionIdSet; //dupe online sessions
        eastl::set<ExternalId> offlineExternalIdSet; //dupe offline users

        startRequest.getPlayerJoinData().copyInto(mPlayerJoinData);
        startRequest.getGameCreationData().copyInto(mGameCreationData);
        mPlayerJoinData.copyInto(mMatchmakingSupplementalData.mPlayerJoinData);
        mGameCreationData.getTeamIds().copyInto(mMatchmakingSupplementalData.mTeamIds);

        request.getXblAccountIdBlockList().copyInto(mXblIdBlockList);
        mMatchmakingSupplementalData.mXblIdBlockList = &mXblIdBlockList;

        //Create the member info object for the session owner, if not the host:
        const UserSessionInfo* hostUserSessionInfo = Blaze::GameManager::getHostSessionInfo(request.getUsersInfo());
        if (hostUserSessionInfo == nullptr || hostUserSessionInfo->getSessionId() != request.getOwnerUserSessionInfo().getSessionId())
        {
            // We only need to add it here so that it will properly be deleted when the session ends: 
            mIndirectOwnerMemberInfo = BLAZE_NEW MemberInfo(*this, request.getOwnerUserSessionInfo(), false);
            mMatchmaker.getUserSessionsToMatchmakingMemberInfoMap().insert(*mIndirectOwnerMemberInfo);
        }


        //Create node for every user in the group
        for (UserJoinInfoList::const_iterator itr = request.getUsersInfo().begin(), end = request.getUsersInfo().end(); itr != end; ++itr)
        {
            if ((*itr)->getIsHost())
            {
                mPrimaryMemberInfo = BLAZE_NEW MemberInfo(*this, (*itr)->getUser(), false);
                mMemberInfoList.push_back(*mPrimaryMemberInfo);
                mMembersUserSessionInfo.push_back(&mPrimaryMemberInfo->mUserSessionInfo);
                mMatchmaker.getUserSessionsToMatchmakingMemberInfoMap().insert(*mPrimaryMemberInfo);
                memberUserSessionIdSet.insert(mPrimaryMemberInfo->mUserSessionId);
            }
            else if (!(*itr)->getIsOptional())
            {
                // if the user group member isn't logged in, don't put him in the MM session.
                if (PlayerInfo::getSessionExists((*itr)->getUser()))
                {
                    //Don't create another member info if we already have one in the user session id set.
                    if (memberUserSessionIdSet.insert((*itr)->getUser().getSessionId()).second)
                    {
                        MemberInfo *info = BLAZE_NEW MemberInfo(*this, (*itr)->getUser(), false);
                        mMemberInfoList.push_back(*info);
                        mMembersUserSessionInfo.push_back(&info->mUserSessionInfo);
                        mMatchmaker.getUserSessionsToMatchmakingMemberInfoMap().insert(*info);
                    }
                }
            }
            else
            {
                // if new offline reserved external user, or, new online user session, add it.
                ExternalId extId = getExternalIdFromPlatformInfo((*itr)->getUser().getUserInfo().getPlatformInfo());
                bool isNewOfflineExternalUser = (((*itr)->getUser().getSessionId() == INVALID_USER_SESSION_ID)
                    && (offlineExternalIdSet.insert(extId).second));
                if (isNewOfflineExternalUser || memberUserSessionIdSet.insert((*itr)->getUser().getSessionId()).second)
                {
                    // a reserved user shouldn't go into the UserSessionToMatchmakingMemberInfoMap, because it doesn't matter if he logs out.
                    MemberInfo *info = BLAZE_NEW MemberInfo(*this, (*itr)->getUser(), true);
                    mMemberInfoList.push_back(*info);
                    mMembersUserSessionInfo.push_back(&info->mUserSessionInfo);

                    // this isn't initialized by the node or the member info constructor, set to null.
                    // normally it is set by an insert into the map.
                    static_cast<FullUserSessionsMapNode*>(info)->mpNext = nullptr;
                }
            }
        }

        mMemberInfoCount = static_cast<uint16_t>(mMemberInfoList.size());

        //Sort the list based on "hostability".  The front of this list is now the best host
        //for this group.  If members leave, the list will update automatically and stay in
        //sorted order, so we never need to resort.
        if (isPlayerHostedTopology(mGameCreationData.getNetworkTopology()))
        {
            mMemberInfoList.sort(BestNetworkHostComparitor());
        }

        if (EA_UNLIKELY(!PlayerInfo::getSessionExists(getHostMemberInfo().mUserSessionInfo)))
        {
            ERR_LOG("[MatchmakingSession].initialize internal error: player without a user session selected as best host, for matchmakingSession(" << getMMSessionId() << "/" << getHostMemberInfo().mUserSessionInfo.getUserInfo().getId() << "/" << getHostMemberInfo().mUserSessionInfo.getUserInfo().getPersonaName() << ") of (" << mMemberInfoCount << ") session members");
            return false;
        }

        if (isCreateGameEnabled())
        {
            if (getHostMemberInfo().mUserSessionInfo.getHasHostPermission())
            {
                TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                    << getOwnerPersonaName() << ")] of (" << mMemberInfoCount << ") session members, best host was ("
                    << getHostMemberInfo().mUserSessionInfo.getUserInfo().getId() << "/" << getHostMemberInfo().mUserSessionInfo.getUserInfo().getPersonaName() << ").");
            }
            else
            {
                TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                    << getOwnerPersonaName() << ")] of (" << mMemberInfoCount << ") session members, had no member with host permission, cannot finalize without finding a host.");
            }
        }

        // this should have been checked before the start matchmaking startRequest got to the master, but we repeat it for safety.
        if (((mPlayerJoinData.getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION) && isCreateGameEnabled())
            && ((!isDedicatedHostedTopology(mGameCreationData.getNetworkTopology())
                && ((mGameCreationData.getNetworkTopology() != NETWORK_DISABLED) || mMatchmakerSlave.getConfig().getGameSession().getAssignTopologyHostForNetworklessGameSessions()))))
        {
            char8_t errMsg[512];
            blaze_snzprintf(errMsg, sizeof(errMsg), "Error: game entry type \"%s\"; not compatible with network topology \"%s\".",
                GameEntryTypeToString(mPlayerJoinData.getGameEntryType()), GameNetworkTopologyToString(mGameCreationData.getNetworkTopology()));

            err.setErrMessage(errMsg);
            ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());

            return false;
        }

        mMatchmakerSlave.getExternalSessionTemplateName(mMatchmakingSessionTemplateName, startRequest.getSessionData().getExternalMmSessionTemplateNameAsTdfString(), request.getUsersInfo());

        // copy the player & game attrib maps

        // save off game settings for createGame sessions
        mMatchmakingSupplementalData.mMaxPlayerCapacity = mGameCreationData.getMaxPlayerCapacity();
        buildTeamIdRoleSizeMap(mMatchmakingSupplementalData.mPlayerJoinData, mMatchmakingSupplementalData.mTeamIdRoleSpaceRequirements, mMemberInfoCount, true);
        buildTeamIdPlayerRoleMap(mMatchmakingSupplementalData.mPlayerJoinData, mMatchmakingSupplementalData.mTeamIdPlayerRoleRequirements, mMemberInfoCount, true);

        if (isCreateGameEnabled())
        {
            // can skip setting up entry criteria for FG-only sessions
            if (!setUpMatchmakingSessionEntryCriteria(err, request))
            {
                ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                    << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
                return false;
            }
        }


        // fill in the information for mMatchmakingSupplementalData;
        //build MatchmakingSupplementalData
        mMatchmakingSupplementalData.mEvaluateGameProtocolVersionString = mMatchmakerSlave.getEvaluateGameProtocolVersionString();

        const Util::NetworkQosData &networkQosData = getPrimaryUserSessionInfo()->getQosData();
        networkQosData.copyInto(mMatchmakingSupplementalData.mNetworkQosData);
        mMatchmakingSupplementalData.mHasMultipleStrictNats = false;
        // Need to fill this out from MM startRequest
        mMatchmakingSupplementalData.mAllIpsResolved = request.getUserSessionIpInformation().getAreAllSessionIpsResolved();
        const UserSessionsToExternalIpMap &userSessionsToExternalIpMap = request.getUserSessionIpInformation().getUserSessionsToExternalIpMap();
        if (mMatchmakingSupplementalData.mAllIpsResolved)
        {
            UserSessionsToExternalIpMap::const_iterator ipIter = userSessionsToExternalIpMap.find(getHostUserSessionId());
            if (ipIter != userSessionsToExternalIpMap.end())
            {
                mMatchmakingSupplementalData.mHostSessionExternalIp = ipIter->second;
            }
            else
            {
                mMatchmakingSupplementalData.mAllIpsResolved = false;
                ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                    << getOwnerPersonaName() << ")] - owning user session IP not present in StartMatchmakingInternalRequest.");
            }
        }

        mMatchmakingSupplementalData.mNetworkTopology = mGameCreationData.getNetworkTopology();


        // need to have this info accessible to UED rules
        mMatchmakingSupplementalData.mMembersUserSessionInfo = &getMembersUserSessionInfo();
        mMatchmakingSupplementalData.mJoiningPlayerCount = mMemberInfoList.size();

        //Only set the iterator if we have more than one user
        if (!isSinglePlayerSession())
        {
            uint16_t poorNatCount = (mMatchmakingSupplementalData.mNetworkQosData.getNatType() >= Util::NAT_TYPE_STRICT_SEQUENTIAL) ? 1 : 0;
            // we need to get the worst NAT type from the various sessions for comparison purposes
            for (MatchmakingSession::MemberInfoList::const_iterator itr = getMemberInfoList().begin(),
                end = getMemberInfoList().end(); itr != end; ++itr)
            {
                const MatchmakingSession::MemberInfo& memberInfo = static_cast<const MatchmakingSession::MemberInfo&>(*itr);
                if (memberInfo.mUserSessionId != getHostUserSessionId())
                {
                    Util::NatType userSessionNatType = memberInfo.mUserSessionInfo.getQosData().getNatType();
                    if (userSessionNatType >= Util::NAT_TYPE_STRICT_SEQUENTIAL)
                    {
                        bool behindSameFirewall = false;
                        if (mMatchmakingSupplementalData.mAllIpsResolved)
                        {
                            // see if we're behind the same firewall as the host session
                            UserSessionsToExternalIpMap::const_iterator ipIter = userSessionsToExternalIpMap.find(memberInfo.mUserSessionId);
                            if (ipIter != userSessionsToExternalIpMap.end())
                            {
                                if (mMatchmakingSupplementalData.mHostSessionExternalIp == ipIter->second)
                                {
                                    behindSameFirewall = true;
                                }
                            }
                            else
                            {
                                mMatchmakingSupplementalData.mAllIpsResolved = false;
                                ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                                    << getOwnerPersonaName() << ")] - owning user session IP not present in StartMatchmakingInternalRequest.");
                            }

                        }

                        if (!behindSameFirewall)
                        {
                            ++poorNatCount;
                        }
                    }

                    if (userSessionNatType > mMatchmakingSupplementalData.mNetworkQosData.getNatType())
                    {
                        mMatchmakingSupplementalData.mNetworkQosData.setNatType(userSessionNatType);
                        TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                            << getOwnerPersonaName() << ")] matchmaking criteria session NAT type changed to: "
                            << (Util::NatTypeToString(userSessionNatType)));
                    }
                }
            }

            if (poorNatCount > 1)
            {
                mMatchmakingSupplementalData.mHasMultipleStrictNats = true;
            }
        }

        mMatchmakingSupplementalData.mDuplicateTeamsAllowed = startRequest.getGameCreationData().getGameSettings().getAllowSameTeamId();
        mMatchmakingSupplementalData.mPrimaryUserInfo = getPrimaryUserSessionInfo();

        mMatchmakingSupplementalData.mGameProtocolVersionString = startRequest.getCommonGameData().getGameProtocolVersionString();
        mMatchmakingSupplementalData.mGameTypeList.push_back(startRequest.getCommonGameData().getGameType());
        mMatchmakingSupplementalData.mCachedTeamSelectionCriteria = &mCriteria.getTeamSelectionCriteriaFromRules();
        mMatchmakingSupplementalData.mIsPseudoRequest = startRequest.getSessionData().getPseudoRequest();

        // set privileged game preference into the new session to check when evaluating
        DedicatedServerOverrideMap::const_iterator privilegedIter = dedicatedServerOverrideMap.find(getOwnerBlazeId());
        if (privilegedIter != dedicatedServerOverrideMap.end())
        {
            INFO_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")].initialize - setting mPrivilegedGameIdPreference to ("
                << privilegedIter->second << ").");
            mPrivilegedGameIdPreference = privilegedIter->second;
        }

        // init criteria
        if (!mCriteria.initialize(startRequest.getCriteriaData(), mMatchmakingSupplementalData, err))
        {
            // error: problem with matchmaking criteria
            ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")].initialize matchmaking criteria error: " << err.getErrMessage());
            return false;
        }

        // if creating a standard game session with this subsession we need to assign a mode attribute.
        if (isCreateGameEnabled() && (request.getRequest().getCommonGameData().getGameType() != GAME_TYPE_GROUP))
        {
            // Verify that the game mode is set properly, or a game attribute rule associated with the game mode is set:
            const char8_t* gameModeAttrName = mMatchmakerSlave.getConfig().getGameSession().getGameModeAttributeName();
            if (mGameCreationData.getGameAttribs().find(gameModeAttrName) == mGameCreationData.getGameAttribs().end())
            {
                bool foundModeRule = false;
                const RuleContainer &rules = mCriteria.getRules();
                RuleContainer::const_iterator iter = rules.begin();
                RuleContainer::const_iterator end = rules.end();
                for (; iter != end; iter++)
                {
                    const Rule* rule = *iter;

                    if (!rule->isDisabled() &&
                        rule->isRuleType(GameAttributeRuleDefinition::getConfigKey()))
                    {
                        const GameAttributeRule* gameAttributeRule = static_cast<const GameAttributeRule*>(rule);
                        if (blaze_stricmp(gameAttributeRule->getDefinition().getAttributeName(), gameModeAttrName) == 0)
                        {
                            foundModeRule = true;
                            break;
                        }
                    }
                }

                if (!foundModeRule)
                {
                    char8_t errMsg[512];
                    blaze_snzprintf(errMsg, sizeof(errMsg), "Create Game sessions must provide a game mode (%s) as a game attribute or from a game attribute rule.", gameModeAttrName);
                    err.setErrMessage(errMsg);
                    ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                        << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
                    return false;
                }

            }
        }

        // fetch/init scenario-subsession list
        if (mOwningScenarioId != INVALID_SCENARIO_ID)
        {
            // All of the player have the same Scenario Info set:
            req.getScenarioInfo().copyInto(mScenarioInfo);

            // only set max fit score metric if not previously set/initialized (assuming 0 means uninitialized)
            Metrics::TagValue scenarioVersionStr;
            Metrics::uint32ToString(getScenarioInfo().getScenarioVersion(), scenarioVersionStr);
            if (mMatchmaker.getMatchmakingMetrics().mSubsessionMetrics.mMaxFitScore.get({ { Metrics::Tag::scenario_name, getScenarioInfo().getScenarioName() },{ Metrics::Tag::scenario_variant_name, getScenarioInfo().getScenarioVariant() },{ Metrics::Tag::scenario_version, scenarioVersionStr.c_str() },{ Metrics::Tag::subsession_name, getScenarioInfo().getSubSessionName() } }) == 0)
            {
                mMatchmaker.getMatchmakingMetrics().mSubsessionMetrics.mMaxFitScore.set(mCriteria.getMaxPossibleFitScore(), getScenarioInfo().getScenarioName(), getScenarioInfo().getScenarioVariant(), getScenarioInfo().getScenarioVersion(), getScenarioInfo().getSubSessionName());
            }
        }
        else
        {
            ERR_LOG("[matchmakingsession::initialize] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
        }

        // for createGame to finalize the max size, you must have enabled TeamCompositionRule, or TotalPlayerSlotsRule
        if (isCreateGameEnabled() && !mCriteria.isTotalSlotsRuleEnabled() && !mCriteria.hasTeamCompositionRuleEnabled())
        {
            // Note: if updating this err msg, take care to test the length of it as TDF's have a set max(160) allowed.
            char8_t errMsg[512];
            blaze_snzprintf(errMsg, sizeof(errMsg), "Create game must use either TotalPlayerSlotsRule or TeamCompositionRule.");
            err.setErrMessage(errMsg);
            ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")].initialize " << err.getErrMessage());

            return false;
        }

        // we ask that at least one game size rule enabled for narrowing down total games
        // side: PlayerSlotUtilizationRule is only enabled on find game instance only and always appears disabled in mCriteria here
        bool isPlayerSlotUtilizationEnabled = (isFindGameEnabled() && (startRequest.getCriteriaData().getPlayerSlotUtilizationRuleCriteria().getRangeOffsetListName()[0] != '\0'));

        if (!mCriteria.isPlayerCountRuleEnabled() && 
            !isPlayerSlotUtilizationEnabled && !mCriteria.isTotalSlotsRuleEnabled() && !mCriteria.hasTeamCompositionRuleEnabled())
        {
            // Note: if updating this err msg, take care to test the length of it as TDF's have a set max(160) allowed.
            char8_t errMsg[512];
            blaze_snzprintf(errMsg, sizeof(errMsg), "Must use PlayercountRule, PlayerSlotUtilizationRule, TeamCompositionRule, or TotalPlayerSlotsRule.");
            err.setErrMessage(errMsg);
            ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")].initialize " << err.getErrMessage());

            return false;
        }

        mDebugCheckOnly = startRequest.getSessionData().getPseudoRequest();
        if (mDebugCheckOnly)
        {
            mDebugCheckMatchSelf = startRequest.getSessionData().getMatchSelf();
        }
        mDebugFreezeDecay = startRequest.getSessionData().getDebugFreezeDecay();


        // Delay / Decay example:
        // Delay causes session time to pause at 0 until the delay passes.  Ex. Delay = 5, Duration = 10 - [000000123456789] 
        // Decay causes session time to start at a different value.         Ex. Decay = 5, Duration = 5 - [56789]
        // They can be used together.   Ex. Delay = 5, Decay = 5, Duration = 5 - [0000056789]
        // Start TimeValue == now + delay
        // Total Duration == Delay + Duration
        // Ending Session Timeout 'timestep'  == Decay + Duration
        TimeValue delay = startRequest.getSessionData().getStartDelay();
        TimeValue decay = startRequest.getSessionData().getStartingDecayAge();
        TimeValue duration = startRequest.getSessionData().getSessionDuration();

        // init session start & expiration times
        // note: no validation of duration: TDF value is unsigned, and zero is valid (for findGame snapshots)
        mScenarioStartTime = TimeValue::getTimeOfDay();
        mSessionStartTime = mScenarioStartTime + delay;  // now + delay
        mLastStatusAsyncTimestamp = mSessionStartTime;
        mSessionTimeoutSeconds = (uint32_t)(duration + decay).getSec();
        mTotalSessionDuration = request.getScenarioTimeoutDuration();

        if ((uint32_t)(decay.getSec()) <= mSessionTimeoutSeconds)
        {
            mStartingDecayAge = (uint32_t)(decay.getSec());
        }
        else
        {
            mStartingDecayAge = mSessionTimeoutSeconds;
        }

        mSessionAgeSeconds = mStartingDecayAge;

        // We can't setup MM's default role information until we understand what our team sizes will be post-rule init.
        // If role information was unspecified, we use & validate against the max possible team capacity to make the session more permissive
        uint16_t teamCapacity = (mGameCreationData.getRoleInformation().getRoleCriteriaMap().size() > 0) ? mCriteria.getTeamCapacity() : mCriteria.getMaxPossibleTeamCapacity();

        // check out role capacities for CG sessions, if some was provided
        if (isCreateGameEnabled())
        {
            // sets up the default role if the request was missing role info
            setUpDefaultRoleInformation(mGameCreationData.getRoleInformation(), teamCapacity);

            // Validate all the incoming teams:
            uint16_t totalRoleRequirementSize = 0;
            TeamIdRoleSizeMap::const_iterator teamIter = mMatchmakingSupplementalData.mTeamIdRoleSpaceRequirements.begin();
            TeamIdRoleSizeMap::const_iterator teamEnd = mMatchmakingSupplementalData.mTeamIdRoleSpaceRequirements.end();
            for (; teamIter != teamEnd; ++teamIter)
            {
                // validate the individual role capacities against my session
                RoleCriteriaMap::const_iterator roleCriteriaEnd = mGameCreationData.getRoleInformation().getRoleCriteriaMap().end();
                RoleSizeMap::const_iterator roleSizeIter = teamIter->second.begin();
                RoleSizeMap::const_iterator roleSizeEnd = teamIter->second.end();
                for (; roleSizeIter != roleSizeEnd; ++roleSizeIter)
                {

                    if (blaze_stricmp(roleSizeIter->first, GameManager::PLAYER_ROLE_NAME_ANY) == 0)
                    {
                        totalRoleRequirementSize += roleSizeIter->second;
                    }
                    else
                    {
                        RoleCriteriaMap::const_iterator roleCriteriaIter = mGameCreationData.getRoleInformation().getRoleCriteriaMap().find(roleSizeIter->first);
                        if (roleCriteriaIter == roleCriteriaEnd)
                        {
                            char8_t errMsg[128];
                            blaze_snzprintf(errMsg, sizeof(errMsg), "Error: Requested joining role \"%s\", not present in provided RoleInformation.", roleSizeIter->first.c_str());
                            err.setErrMessage(errMsg);
                            TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                                << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
                            return false;
                        }

                        if (roleSizeIter->second > roleCriteriaIter->second->getRoleCapacity())
                        {
                            char8_t errMsg[128];
                            blaze_snzprintf(errMsg, sizeof(errMsg), "Error: Requested more slots (%u) in joining role \"%s\" than specified Role capacity (%u).", roleSizeIter->second, roleSizeIter->first.c_str(), roleCriteriaIter->second->getRoleCapacity());
                            err.setErrMessage(errMsg);
                            TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                                << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
                            return false;
                        }

                        totalRoleRequirementSize += roleSizeIter->second;
                    }
                }
            }

            if (totalRoleRequirementSize != mMemberInfoCount)
            {
                // someone didn't have a role (or a role was used that does not exist in the role information)
                char8_t errMsg[128];
                blaze_snzprintf(errMsg, sizeof(errMsg), "Error: Requested fewer slots (%u) in joining roles than member count (%u).", totalRoleRequirementSize, mMemberInfoCount);
                err.setErrMessage(errMsg);
                TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                    << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
                return false;
            }


            teamIter = mMatchmakingSupplementalData.mTeamIdRoleSpaceRequirements.begin();
            teamEnd = mMatchmakingSupplementalData.mTeamIdRoleSpaceRequirements.end();
            for (; teamIter != teamEnd; ++teamIter)
            {
                // after checking that our player's role requirements were valid, verify that the role criteria is also valid.
                uint16_t totalRoleCapacity = 0;
                RoleCriteriaMap::const_iterator roleCriteriaIter = mGameCreationData.getRoleInformation().getRoleCriteriaMap().begin();
                RoleCriteriaMap::const_iterator roleCriteriaEnd = mGameCreationData.getRoleInformation().getRoleCriteriaMap().end();
                for (; roleCriteriaIter != roleCriteriaEnd; ++roleCriteriaIter)
                {
                    uint16_t currentRoleCapacity = roleCriteriaIter->second->getRoleCapacity();

                    if (blaze_stricmp(roleCriteriaIter->first, GameManager::PLAYER_ROLE_NAME_ANY) == 0)
                    {
                        // 'PLAYER_ROLE_NAME_ANY' is reserved and not allowed as a joinable role in the game session
                        char8_t errMsg[128];
                        blaze_snzprintf(errMsg, sizeof(errMsg), "Error: Requested more capacity (%u) for role \"%s\", this is an invalid role.",
                            currentRoleCapacity, roleCriteriaIter->first.c_str());
                        err.setErrMessage(errMsg);
                        TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                            << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
                        return false;
                    }

                    if (currentRoleCapacity > teamCapacity)
                    {
                        char8_t errMsg[128];
                        blaze_snzprintf(errMsg, sizeof(errMsg), "Error: Requested more capacity (%u) for role \"%s\" than requested team capacity (%u).",
                            currentRoleCapacity, roleCriteriaIter->first.c_str(), teamCapacity);
                        err.setErrMessage(errMsg);
                        TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                            << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
                        return false;
                    }
                    totalRoleCapacity += currentRoleCapacity;
                }

                if (totalRoleCapacity < teamCapacity)
                {
                    char8_t errMsg[128];
                    blaze_snzprintf(errMsg, sizeof(errMsg), "Error: Total role capacity (%u) less than requested team capacity (%u).",
                        totalRoleCapacity, teamCapacity);
                    err.setErrMessage(errMsg);
                    TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                        << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
                    return false;
                }


                EntryCriteriaName failedCriteriaName;
                // now check the the MultiRole criteria
                if (!(mMultiRoleEntryCriteriaEvaluator.evaluateEntryCriteria(teamIter->second, failedCriteriaName)))
                {
                    char8_t errMsg[512];
                    blaze_snzprintf(errMsg, sizeof(errMsg), "Error: Role roster unable to pass own multiRoleEntryCriteria named \"%s\".", failedCriteriaName.c_str());

                    err.setErrMessage(errMsg);
                    ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                        << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
                    return false;
                }
            }
        }

        request.getGameExternalSessionData().copyInto(mGameExternalSessionData);
        request.getRequest().getGameCreationData().getExternalSessionCustomData().copyInto(mExternalSessionCustomData);
        request.getRequest().getGameCreationData().getExternalSessionStatus().copyInto(mExternalSessionStatus);

        // Every player in the incoming request has the Scenario Info set, so we don't need to grab it from the host.
        Blaze::GameManager::UserJoinInfoPtr hostJoinInfo = request.getUsersInfo().front();

        const Blaze::Matchmaker::TimeToMatchEstimator& ttmEstimator = mMatchmakerSlave.getTimeToMatchEstimator();

        if (hostJoinInfo != nullptr)
        {
            mEstimatedTimeToMatch = ttmEstimator.getTimeToMatchEstimate(getScenarioInfo().getScenarioName(), hostJoinInfo->getUser().getBestPingSiteAlias(), req.getRequest().getCommonGameData().getDelineationGroup(), mMatchmakerSlave.getConfig().getScenariosConfig());
        }
        else
        {
            // this just gets the global ttm.
            mEstimatedTimeToMatch = ttmEstimator.getTimeToMatchEstimate("", "", "", mMatchmakerSlave.getConfig().getScenariosConfig());
        }
      

        mTotalGameplayUsersAtSessionStart = request.getTotalUsersOnline();
        mTotalUsersInGameAtSessionStart = request.getTotalUsersInGame();
        mTotalUsersInMatchmakingAtSessionStart = request.getTotalUsersInMatchmaking();


        if (getIsUsingFilters())
        {
            WARN_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")] Legacy MM sessions cannot use property filters. Filtering logic disabled!");
#if 0
            // PACKER_TODO: Need to re-enable config validation to validate that filters/properties are not supported for non-gamepacker legacy subsessions
            //EA_FAIL_MSG("Property filters have been disabled in legacy MM, in accordance with the doc: https://developer.ea.com/display/TEAMS/Game+and+Matchmaking+Property+Filters#GameandMatchmakingPropertyFilters-FilterInputs");
            return false;
#endif
        }

        return true;
    }

    void MatchmakingSession::activateReinsertedSession()
    {
        clearLockdown();
        TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
            << getOwnerPersonaName() << ")].activateReinsertedSession clearing finalization lockdown.");
    }

    void MatchmakingSession::removeMember(MatchmakingSession::MemberInfo &memberInfo)
    {
        if (&memberInfo == mIndirectOwnerMemberInfo)
        {
            removeUserSessionToMatchmakingMemberMapEntry(*mIndirectOwnerMemberInfo);
            delete mIndirectOwnerMemberInfo;
            mIndirectOwnerMemberInfo = nullptr;
            return;
        }

        // save off values for updating external sessions before deleting
        ExternalUserLeaveInfoList externalUserInfos;
        if (memberInfo.mUserSessionInfo.getHasExternalSessionJoinPermission())
        {
            setExternalUserLeaveInfoFromUserSessionInfo(*externalUserInfos.pull_back(), memberInfo.mUserSessionInfo);
        }

        mMemberInfoList.remove(memberInfo);
        removeMembersUserSessionInfoListEntry(memberInfo);
        removeUserSessionToMatchmakingMemberMapEntry(memberInfo);

        for (MatchedFromList::const_iterator itr = mMatchedFromList.begin(), end = mMatchedFromList.end(); (itr != end); ++itr)
        {
            MatchmakingSessionMatchNode &node = (MatchmakingSessionMatchNode &) *itr;
            MatchmakingSession* fromSession = node.sMatchedFromSession;
            if (fromSession != nullptr)
            {
                // If I'm in some other MM session's pending list, update their tracking of my size (node.sOtherSessionSize)
                // (ensure I'll be added to their correct sessions bucket).
                const uint16_t myOrigSize = node.sOtherSessionSize;
                node.sOtherSessionSize = ((myOrigSize > 0)? myOrigSize - 1: 0);

                // In rare event I'm somehow *already* in the other MM session's finalizing sessions buckets
                // remove myself from them since my size is no longer valid there.
                MatchedSessionsBucket& fromBucket = fromSession->mMatchedToBuckets[myOrigSize];
                bool removedFromOther = fromBucket.removeMatchFromBucket(*this, myOrigSize);

                TRACE_LOG("[MatchmakingSession(ownerblazeid:" << getOwnerBlazeId() << "/" << getOwnerPersonaName() << ")].removeMember removing user(" <<
                    memberInfo.mUserSessionInfo.getSessionId() << ",blazeid: " << memberInfo.mUserSessionInfo.getUserInfo().getId() << ") from this matchmaking session(" <<
                    getMMSessionId() << ", orig/new group size: " << myOrigSize << "/" << node.sOtherSessionSize << ")" << ". This session was previously matched by other " <<
                    "matchmaking session(" << fromSession->getMMSessionId()  << (removedFromOther? " but has now been removed from the other's finalizable sessions buckets." : "."));
            }
        }

        // In the unlikely event that the primary member of an indirect MM attempt leaves, we switch to the current host member:
        if (mPrimaryMemberInfo == &memberInfo)
        {
            if (mMemberInfoCount > 1)
                mPrimaryMemberInfo = &(const_cast<MemberInfo &>(getHostMemberInfo()));
            else
                mPrimaryMemberInfo = nullptr;
        }

        delete &memberInfo;
        --mMemberInfoCount;

        mIsFinalStatusAsyncDirty = true;

        // update external session for this member
        mMatchmaker.leaveMatchmakingExternalSession(getMMSessionId(), mMatchmakingSessionTemplateName, externalUserInfos);
    }

    void MatchmakingSession::removeMembersUserSessionInfoListEntry(const MatchmakingSession::MemberInfo &memberInfo)
    {
        GameManager::UserSessionInfoList::iterator itr = mMembersUserSessionInfo.begin();
        while (itr != mMembersUserSessionInfo.end())
        {
            if (*itr == &memberInfo.mUserSessionInfo)
            {
                mMembersUserSessionInfo.erase(itr);
                break;
            }
            ++itr;
        }
    }


    // remove the userSessionToMatchmakingMemberInfoMap entry for this MM session
    void MatchmakingSession::removeUserSessionToMatchmakingMemberMapEntry(MatchmakingSession::MemberInfo &memberInfo)
    {
        // NOTE: ideally, we'd just construct an iterator from the memberInfo (since it's the node of the intrusive map),
        //   but there seems to be an EASTL bug in the iterator constructor for intrusive multi hash maps (construct iter from node)
        //   the iterator's node is correct, but the bucket is left nullptr.
        //   So the following (commented out) code crashes:
        //              FullUserSessionsMap::iterator mapIter( &memberInfo ); // ok, memberInfo is the node in the intrusive map
        //              mMatchmaker.getUserSessionsToMatchmakingMemberInfoMap().erase(mapIter); // crashes trying to increment nullptr bucket

        // instead, we do an equal_range on the userSession (getting all of that user's sessions), and find/erase the entry for this MM session.
        FullUserSessionMapEqualRangePair iterPair = mMatchmaker.getUserSessionsToMatchmakingMemberInfoMap().equal_range(memberInfo.mUserSessionId);
        for ( ; iterPair.first != iterPair.second; ++iterPair.first )
        {
            const MemberInfo &fullUserSessionMapMemberInfo = static_cast<MemberInfo &>(*iterPair.first);
            if (&fullUserSessionMapMemberInfo.mMMSession == this)
            {
                if(EA_UNLIKELY(&memberInfo != &fullUserSessionMapMemberInfo))
                {
                    ERR_LOG(LOG_PREFIX << " MatchmakingSession::removeUserSessionToMatchmakingMemberMapEntry failed - removing mismatched entry from map (id: "
                        << mMMSessionId << ") for userSessionId: " << memberInfo.mUserSessionInfo.getSessionId() << " (blazeid: " << memberInfo.mUserSessionInfo.getUserInfo().getId() << ")"
                        << "target MemberInfo at address (" << &memberInfo << "), discovered reference at address (" << &fullUserSessionMapMemberInfo << ")");
                }
                else
                {
                    mMatchmaker.getUserSessionsToMatchmakingMemberInfoMap().erase(iterPair.first);
                    return;
                }
            }
        }

        // only unexpected result for non-optional players
        if (!memberInfo.mIsOptionalPlayer)
        {
            ERR_LOG(LOG_PREFIX << " MatchmakingSession::removeUserSessionToMatchmakingMemberMapEntry failed - unable to find entry for this matchmaking session (id: "
                << mMMSessionId << ") for userSessionId: " << memberInfo.mUserSessionInfo.getSessionId() << " (blazeid: " << memberInfo.mUserSessionInfo.getUserInfo().getId() << ") ");
        }
    }


    /*! ************************************************************************************************/
    /*!
        \brief Initialize the findGame/CreateGame session mode flags from the startMatchmakingRequest.
            Returns true on success.

        \param[in] request  The gameMode is determined by the startMatchmaking request
        \param[out] err We set the errMessage field if the mode is invalid
        \return true on success, false if the mode isn't valid.
    *************************************************************************************************/
    bool MatchmakingSession::initSessionMode(const StartMatchmakingRequest &request, MatchmakingCriteriaError &err)
    {
        // set the sessionMode flags
        if (request.getSessionData().getSessionMode().getCreateGame())
        {
            setCreateGameEnabled();
        }

        if (request.getSessionData().getSessionMode().getFindGame())
        {
            setFindGameEnabled();
        }

        // validate: must specify at least 1 mode
        if ( (!isCreateGameEnabled()) && (!isFindGameEnabled()) )
        {
            err.setErrMessage("you must specify at least create session mode.");
            return false;
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*!
        \brief returns true if there are new matches from the pending bucket that now match
    *************************************************************************************************/
    bool MatchmakingSession::updatePendingMatches()
    {
        bool newMatches = false;
        uint32_t sessionAgeSeconds = mSessionAgeSeconds;

        // move any pending matches that match now into the matched list
        while ( !mPendingMatchedToListMap.empty() )
        {
            uint32_t pendingBucketAge = mPendingMatchedToListMap.begin()->first;
            TRACE_LOG(LOG_PREFIX << " session " << mMMSessionId << " at time " << sessionAgeSeconds << " has pending matches at time bucket " << pendingBucketAge << " and status " << mStatus << ".");

            if ( (pendingBucketAge <= mSessionAgeSeconds) || (isExpired() && getMatchmaker().getMatchmakingConfig().getServerConfig().getAllowExpiredSessionFutureMatching()) )
            {
                // If we are expired, we allow sessions to pull in other sessions they would match in the future
                // Consider making expired check based off scenario timeout value, as well as making this logic configurable.
                // move the bucket into the matched game list
                MatchedToList &pendingBucketList = mPendingMatchedToListMap.begin()->second;
                TRACE_LOG(LOG_PREFIX << " Adding " << pendingBucketList.size() << " pending matches for session " << mMMSessionId 
                          << " to the currently matching list (matched at time " << mSessionAgeSeconds << ")");

                if (mCriteria.isAnyTeamRuleEnabled())
                {
                    while( !pendingBucketList.empty() )
                    {
                        MatchmakingSessionMatchNode& matchNode = (MatchmakingSessionMatchNode&)pendingBucketList.front();
                        pendingBucketList.pop_front();
                        mMatchedToList.push_back(matchNode);
                        MatchedSessionsBucket& bucket = mMatchedToBuckets[matchNode.sOtherSessionSize];
                        bucket.addMatchToBucket(&matchNode, matchNode.sOtherSessionSize);
                    }
                }
                else
                {
                    // can just do a splice, since we don't care about team
                    mMatchedToList.splice(mMatchedToList.end(), pendingBucketList);
                }

                newMatches = true;
                mPendingMatchedToListMap.erase(mPendingMatchedToListMap.begin());
            }
            else
            {
                // We have passed our current session age, break out of the loop.
                break;
            }
        }
        return newMatches;
    }

    void MatchmakingSession::evaluateCreateGame(FullMatchmakingSessionSet &fullSessionList, MatchedSessionsList& matchedSessions)
    {
        EA_ASSERT(isCreateGameEnabled());

        // compare this (new) session's sessions against the existing sessions

        TRACE_LOG(LOG_PREFIX << "evaluating new CreateGame Session " << mMMSessionId << " (age: " << mSessionAgeSeconds << " seconds) against "
                  << fullSessionList.size() << " other sessions: ");

        for (auto&& matchmakingSession : fullSessionList)
        {
            mMatchmaker.getMatchmakerMetrics(this).mTotalMatchmakingCreateGameEvaluationsByNewSessions.increment();
            MatchmakingSession &existingSession = *matchmakingSession;

            // Skip sessions that were created by the same scenario
            if (getMMScenarioId() != INVALID_SCENARIO_ID && getMMScenarioId() == existingSession.getMMScenarioId())
            {                    
                continue;
            }

            // Validate we aren't already switched context to the owning session.
            LogContextOverride logAudit(existingSession.getOwnerUserSessionId());

            AggregateSessionMatchInfo aggregateSessionMatchInfo(getDebugCheckOnly() || IS_LOGGING_ENABLED(Logging::TRACE));

            evaluateSessionsForCreateGame(existingSession, aggregateSessionMatchInfo);

            // check existing session vs this session
            if (existingSession.isMatchNow(aggregateSessionMatchInfo.sOtherMatchInfo))
            {
                // Can't actually finalize here as currently only 1 session can finalize at a time.
                // Putting existing sessions on the queue here prefers them to finalize before this session will
                // as this session wont get added until after exiting this function.
                matchedSessions.push_back(existingSession.getMMSessionId());
            }
        }
    }

    // update this session's CG finalization eligibility and attempt to finalize.  Returns true if finalized successfully
    bool MatchmakingSession::attemptGreedyCreateGameFinalization(MatchmakingSessionList &finalizedSessionList)
    {
        CreateGameFinalizationSettings* createGameFinalizationSettings = BLAZE_NEW CreateGameFinalizationSettings(*this);
        updateCreateGameFinalizationEligibility(*createGameFinalizationSettings);
        if (isEligibleToFinalize())
        {
            // attempt to finalize the matchmaking session (vote on results, create game, have members join)
            // we send asyn notification when session got finalized
            sendFinalMatchmakingStatus();

            if (attemptCreateGameFinalization(finalizedSessionList, *createGameFinalizationSettings))
            {
                // Note: at this point, the session owner and all members who successfully joined the game
                // have been sent their async matchmaking result notifications.

                // This session has finalized so no need to evaluate any other sessions.
                // MM is greedy and doesn't always choose the "Best Match" out of everyone
                // but will choose the best match of the sessions that match which it finds first.
                // There may be a better match out there, but we aren't waiting around to find it
                // thus we finalize now.
                return true;
            }
            else
            {
                clearEligibleToFinalize();
            }
        }

        delete createGameFinalizationSettings;
        return false;
    }


    /*! ************************************************************************************************/
    /*! \brief Drop any players that fail the Xbox One block list check
        (Note: This doesn't check vs. the main session owner, because that's already handled by the Rule)

        Open Question: would it be easier to check each player we try to add in buildSessionList versus everyone we've
        already pulled in?
    ***************************************************************************************************/
    void MatchmakingSession::dropBlockListMatches()
    {
        MatchedToList::iterator iter = mMatchedToList.begin();
        MatchedToList::iterator end = mMatchedToList.end();
        while (iter != end)
        {
            MatchedToList::iterator curIter = iter;
            ++iter;

            MatchmakingSessionMatchNode &curNode = (MatchmakingSessionMatchNode&)*curIter;
            const MatchmakingSession* matchingSession = curNode.sMatchedToSession;
            if (matchingSession->getCriteria().isXblBlockPlayersRuleEnabled())
            {
                MatchedToList::iterator iter2 = mMatchedToList.begin();
                MatchedToList::iterator end2 = mMatchedToList.end();
                for (; iter2 != end2; ++iter2)
                {
                    // skip outer session.
                    if (curIter == iter2)
                        continue;

                    const MatchmakingSession* matchingSession2 = ((MatchmakingSessionMatchNode&)*iter2).sMatchedToSession;
                    const XblBlockPlayersRule* blockRule = matchingSession->getCriteria().getXblBlockPlayersRule();
                    if (blockRule != nullptr && blockRule->isSessionBlocked(matchingSession2))
                    {
                        // Drop the person with the block list:
                        MatchedSessionsBucket& bucket = mMatchedToBuckets[curIter->sOtherSessionSize];
                        bucket.removeMatchFromBucket(*matchingSession, curIter->sOtherSessionSize);
                        mMatchedToList.remove(*curIter);
                        MatchedFromList::remove(curNode);
                        mMatchmaker.deallocateMatchNode(curNode);
                        break;
                    }
                }
            }
        }
    }


    /*! ************************************************************************************************/
    /*! \brief determine if this CreateGame matchmaking session is eligible for finalization.  If so, set
            the session's SESSION_ELIGIBLE_TO_FINALIZE status flag.
            Additionally, outputs the team ids that will be used by the game. 

        \param[in/out] createGameFinalizationSettings The create game finalization settings
    ***************************************************************************************************/
    void MatchmakingSession::updateCreateGameFinalizationEligibility(CreateGameFinalizationSettings& createGameFinalizationSettings)
    {
        EA_ASSERT_MSG(this->isCreateGameEnabled(), "Evaluating a non create game matchmaking session for create game finalization eligibility.");

        // We test 2 things:
        // 1) that the session has pooled together enough matching participants to satisfy the gameSizeRule
        // 2) that (at least) one matching participant is a viable host for the game (using the NAT type of the hosts)

        // calculate the size of the participant pool (count of all matching participants, including self & any/all group members)
        // and determine if a participant is a viable game host
        bool hasViableHost = mCriteria.isViableGameHost(*this); // JESSE_TODO: packer sessions shall not rely on evaluating hardcoded rules about host viability. If required it will be implemented as an explicit game quality factor that can be evaluated just like all the others and used to score the game, and need to pass the *game viability* check. We shall remove this code entirely once packer MM sessions are used for create as well as find/create matchmaking.

        if (isDedicatedHostedTopology(mGameCreationData.getNetworkTopology()) 
            || (!isPlayerHostedTopology(mGameCreationData.getNetworkTopology()) 
                    && ((getGameEntryType() != GAME_ENTRY_TYPE_MAKE_RESERVATION) || !mMatchmakerSlave.getConfig().getGameSession().getAssignTopologyHostForNetworklessGameSessions())))
        {
            // override for dedicated servers and networkless games (NETWORK_DISABLED topology) if we're configured to not assign a host
            hasViableHost = true;
        }

        // Checks everyone pooled together against everyone else to see if we've 
        // pulled in people that have blocked each other.
        dropBlockListMatches();

        uint32_t participantCount = getPlayerCount();
        MatchedToList::const_iterator matchedSessionIter = mMatchedToList.begin();
        MatchedToList::const_iterator matchedSessionEnd = mMatchedToList.end();
        for ( ; matchedSessionIter != matchedSessionEnd; ++matchedSessionIter )
        {
            const MatchmakingSession* matchedSession = ((MatchmakingSessionMatchNode&)*matchedSessionIter).sMatchedToSession;
            MatchmakingSession* session = const_cast<MatchmakingSession*>(matchedSession);

            if (matchedSession->isLockedDown())
            {
                continue;
            }
            
            // We shouldn't have matched ourself.
            EA_ASSERT( matchedSession != this );

            // make sure everyone gets decayed.
            session->updateSessionAge(TimeValue::getTimeOfDay());
            if (session->updateCreateGameSession())
            {
                // schedule a dirty evaluation for after this session if needed.
                mMatchmaker.queueSessionForDirtyEvaluation(session->getMMSessionId());
            }

            // Check that each matched session's Team compatibility with us. (if the team size rule is enabled)
            if (mCriteria.isAnyTeamRuleEnabled())
            {
                // Track the matched session team information, if they are not compatible, we skip this session
                createGameFinalizationSettings.trackTeamsWithMatchedSession(*matchedSession);
            }

            participantCount += matchedSession->getPlayerCount();

            // check for a viable host (if we haven't found one already)
            if (!hasViableHost)
            {
                hasViableHost = mCriteria.isViableGameHost(*matchedSession);
            }

            // Calculate the number of players for each of this session's accepted ping sites
            // Both our session and each other session must accept the ping site at decay step that we finalize at
            if (mCriteria.isExpandedPingSiteRuleEnabled())
            {
                createGameFinalizationSettings.updatePingSitePlayerCount(*this, *matchedSession);
            }
        }

        // determine if the participantCount is sufficient
        bool particpantCountSufficient = (participantCount >= mMatchmakerSlave.getMatchmaker()->getGlobalMinPlayerCountInGame());
        if (mCriteria.isPlayerCountRuleEnabled())
        {
            // Note that this now only checks ourself and not everyone else.   We only check the owning session list's
            // counts are sufficient after we decide how we are going to build the game.
            particpantCountSufficient = mCriteria.getPlayerCountRule()->isParticipantCountSufficientToCreateGame(participantCount);
        }

        // determine if the team size is sufficient.
        bool teamSizeSufficient = true;
        uint16_t numTeams = 1;
        if ( mCriteria.isAnyTeamRuleEnabled() && (mCriteria.getTeamIds() != nullptr) )
        {
            numTeams = mCriteria.getTeamIds()->size();
            teamSizeSufficient = createGameFinalizationSettings.areTeamSizesSufficientToAttemptFinalize(*this);
        }

        // determine if enough players to potentially make full teams for team composition rule.
        if (teamSizeSufficient && mCriteria.hasTeamCompositionRuleEnabled())
        {
            // team compositions requires full team capacities, so no point
            // attempting finalization unless we have at least that many matched sessions.
            teamSizeSufficient = (participantCount >= (uint32_t)(mCriteria.getTeamCompositionRule()->getTeamCapacityForRule() * mCriteria.getTeamCountFromRule()));
        }

        // determine if we have any ping sites that have enough players
        bool pingSitePlayerCountSufficient = true;
        if (mCriteria.isExpandedPingSiteRuleEnabled())
        {
            createGameFinalizationSettings.buildSortedPingSitePlayerCountList(mMatchmakerSlave, getCriteria(), pingSitePlayerCountSufficient);
        }

        // prepare createGameStatus information
        if (hasViableHost)
        {
            mMatchmakingAsyncStatus.getCreateGameStatus().getEvaluateStatus().setAcceptableHostFound();
        }
        else
        {
            mMatchmakingAsyncStatus.getCreateGameStatus().getEvaluateStatus().clearAcceptableHostFound();
        }

        if (particpantCountSufficient)
        {
            mMatchmakingAsyncStatus.getCreateGameStatus().getEvaluateStatus().setPlayerCountSufficient();
        }
        else
        {
            mMatchmakingAsyncStatus.getCreateGameStatus().getEvaluateStatus().clearPlayerCountSufficient();
        }

        mMatchmakingAsyncStatus.getCreateGameStatus().setNumOfMatchedPlayers((uint32_t)participantCount - getPlayerCount());

        if (teamSizeSufficient)
        {
            mMatchmakingAsyncStatus.getCreateGameStatus().getEvaluateStatus().setTeamSizesSufficient();
        }
        else
        {
            mMatchmakingAsyncStatus.getCreateGameStatus().getEvaluateStatus().clearTeamSizesSufficient();
        }

        bool matchedSessionCount = ( (!isSingleGroupMatch()) || (mMatchedToList.size() > 0) );
        bool eligible = particpantCountSufficient && teamSizeSufficient && hasViableHost && matchedSessionCount && pingSitePlayerCountSufficient;
        if (eligible)
        {
            setEligibleToFinalize();
        }

        TRACE_LOG(LOG_PREFIX << "CreateGame Session " << mMMSessionId << (eligible ? " " : " NOT ") << "ELIGIBLE for finalization. " 
            << (particpantCountSufficient ? "PlayerCountSufficient" : "PlayerCountInsufficient") << " (" << participantCount << " players for " 
            << (mCriteria.isPlayerCountRuleEnabled() ? mCriteria.getPlayerCountRule()->getMinParticipantCount() : mMatchmakerSlave.getMatchmaker()->getGlobalMinPlayerCountInGame()) << " minimum participants), " 
            << (teamSizeSufficient ? "TeamSizeSufficient" : "TeamSizeInsufficient") << " (" << mCriteria.getAcceptableTeamMinSize() 
            << " minimum needed for " << numTeams << " teams)"
            << (matchedSessionCount ? "": " SessionCountMismatch ") <<  (isSingleGroupMatch() ? " SGM, ":", ")
            << "Matched Session Count: " << mMatchedToList.size() 
            << ", " << (hasViableHost ? "hasViabileHost, " : "noViableHost, ")
            << (pingSitePlayerCountSufficient ? "PingSitePlayerCountSufficient" : "PingSitePlayerCountInsufficient"));
    }


    // eval this vs the supplied session (and vice versa)
    void MatchmakingSession::evaluateSessionsForCreateGame(MatchmakingSession &otherSession, AggregateSessionMatchInfo &aggregateSessionMatchInfo)
    {
        bool shouldEvaluateSessions = true;
        MatchFailureMessage matchFailureMsg(Blaze::Logging::TRACE);

        // bail if we're evaluating against ourself or a sibling
        if (otherSession.getMMSessionId() == mMMSessionId)
        {
            matchFailureMsg.writeMessage("Skipping evaluation against the self.");
            shouldEvaluateSessions = false;
        }

        if (EA_UNLIKELY(getPrimaryUserSessionInfo() == nullptr || otherSession.getPrimaryUserSessionInfo() == nullptr))
        {
            matchFailureMsg.writeMessage("Skipping evaluation as one or both sessions does not have a primary usersession.");
            shouldEvaluateSessions = false;
        }

        // don't evaluate sessions from the same scenario
        if ((mOwningScenarioId != INVALID_SCENARIO_ID) && 
            (otherSession.getMMScenarioId() == mOwningScenarioId))
        {
            matchFailureMsg.writeMessage("Skipping evaluation against session owned by the same scenario.");
            shouldEvaluateSessions = false;
        }

        if ((getUserGroupId() != EA::TDF::OBJECT_ID_INVALID) &&  (otherSession.getUserGroupId() == getUserGroupId()) &&
            ((!otherSession.isDebugCheckOnly() || !mDebugCheckOnly) || (!otherSession.isDebugCheckMatchSelf() || !mDebugCheckMatchSelf)))
        {
            matchFailureMsg.writeMessage("Skipping evaluation against session owned by the same UserGroup.");
            shouldEvaluateSessions = false;
        }

        // sessions owned by the same user cannot evaluate each other, unless they are both pseudo requests and have enabled the matchSelf property
        if ((otherSession.getOwnerBlazeId() == getOwnerBlazeId()) &&
            ((!otherSession.isDebugCheckOnly() || !mDebugCheckOnly) || (!otherSession.isDebugCheckMatchSelf() || !mDebugCheckMatchSelf)))
        {

            matchFailureMsg.writeMessage("Skipping evaluation of concurrent sessions owned by the same user.");
            shouldEvaluateSessions = false;
        }

        // bail if the other session doesn't support creatGame mode
        // sessions without CG enabled shouldn't ever be in the set of evaluated sessions, so this shouldn't be true unless there's a bug
        if (EA_UNLIKELY(shouldEvaluateSessions && !otherSession.isCreateGameEnabled()))
        {
            
            matchFailureMsg.writeMessage("Due to other session not supporting CreateGame mode.");
            shouldEvaluateSessions = false;
            WARN_LOG(LOG_PREFIX << "evaluated CreateGame Session " << getMMSessionId() << " player " << getOwnerBlazeId() 
                << " (" << getOwnerPersonaName() << ") against session " << otherSession.getMMSessionId() 
                << " player " << otherSession.getOwnerBlazeId() << " (" 
                << otherSession.getOwnerPersonaName() << ") - NO_MATCH : " << matchFailureMsg.getMessage() << " - this should not be possible.");
        }

        // bail if the other session doesn't have the same GameNetworkTopology as I do
        if ( shouldEvaluateSessions && (mGameCreationData.getNetworkTopology() != otherSession.getNetworkTopology()) )
        {
            matchFailureMsg.writeMessage("Due to mismatch of the session's desired GameNetworkTopology: I require %s, other session requires: %s",
                GameNetworkTopologyToString(mGameCreationData.getNetworkTopology()), GameNetworkTopologyToString(otherSession.getNetworkTopology()) );
            shouldEvaluateSessions = false;
        }

        // check the entry criteria (in each direction), saving the result in the aggregateSessionMatchInfo
        //   This lets us "flag" neither, one, or both directions for evaluation
        if (shouldEvaluateSessions)
        {
            if ( !(evaluateEntryCriteria(otherSession.getPrimaryUserBlazeId(), otherSession.getPrimaryUserSessionInfo()->getDataMap(), mFailedGameEntryCriteriaName)) )
            {
                aggregateSessionMatchInfo.sMyMatchInfo.setNoMatch();
                matchFailureMsg.writeMessage("Due to other player failing my Game Entry Criteria: '%s'", mFailedGameEntryCriteriaName.c_str());
            }

            if ( !(otherSession.evaluateEntryCriteria(getPrimaryUserBlazeId(), getPrimaryUserSessionInfo()->getDataMap(), mFailedGameEntryCriteriaName)) )
            {
                aggregateSessionMatchInfo.sOtherMatchInfo.setNoMatch();
                matchFailureMsg.writeMessage("Due to other player failing my Game Entry Criteria: '%s'", mFailedGameEntryCriteriaName.c_str());
            }

            if (aggregateSessionMatchInfo.isMisMatch())     // Continue evaluating unless both sessions mismatch
            {
                shouldEvaluateSessions = false;
            }
        }

        if (shouldEvaluateSessions)
        {
            const char8_t* otherLeaderRole = lookupPlayerRoleName(otherSession.mPlayerJoinData, otherSession.getPrimaryUserBlazeId());
            if (EA_LIKELY(otherLeaderRole))
            {
                RoleEntryCriteriaEvaluatorMap::const_iterator roleCriteriaIter = mRoleEntryCriteriaEvaluators.find(otherLeaderRole);
                if (roleCriteriaIter != mRoleEntryCriteriaEvaluators.end())
                {
                    if (!roleCriteriaIter->second->evaluateEntryCriteria(otherSession.getPrimaryUserBlazeId(), getPrimaryUserSessionInfo()->getDataMap(), mFailedGameEntryCriteriaName))
                    {
                        aggregateSessionMatchInfo.sMyMatchInfo.setNoMatch();
                        matchFailureMsg.writeMessage("Due to other player failing my Role Entry Criteria: '%s' for Role '%s'", mFailedGameEntryCriteriaName.c_str(), otherLeaderRole);
                    }
                }
            }

            const char8_t* myLeaderRole = lookupPlayerRoleName(mPlayerJoinData, getPrimaryUserBlazeId());
            if (EA_LIKELY(myLeaderRole))
            {
                RoleEntryCriteriaEvaluatorMap::const_iterator otherRoleCriteriaIter = otherSession.mRoleEntryCriteriaEvaluators.find(myLeaderRole);
                if (otherRoleCriteriaIter != otherSession.mRoleEntryCriteriaEvaluators.end())
                {
                    if (!otherRoleCriteriaIter->second->evaluateEntryCriteria(getPrimaryUserBlazeId(), getPrimaryUserSessionInfo()->getDataMap(), mFailedGameEntryCriteriaName))
                    {
                        aggregateSessionMatchInfo.sOtherMatchInfo.setNoMatch();
                        matchFailureMsg.writeMessage("Due to failing other player's Role Entry Criteria: '%s' for Role '%s'", mFailedGameEntryCriteriaName.c_str(), myLeaderRole);
                    }
                }
            }

            if (aggregateSessionMatchInfo.isMisMatch())
            {
                shouldEvaluateSessions = false;
            }
        }

        // Since we don't know the teams that are going to be used in the end, these checks just use the default team.
        if (shouldEvaluateSessions)
        {
            if (!(mMultiRoleEntryCriteriaEvaluator.evaluateEntryCriteria(otherSession.mMatchmakingSupplementalData.mPlayerJoinData,
                                                                         (uint16_t)otherSession.mMatchmakingSupplementalData.mJoiningPlayerCount, mFailedGameEntryCriteriaName)) )
            {
                aggregateSessionMatchInfo.sMyMatchInfo.setNoMatch();
                matchFailureMsg.writeMessage("Due to other player failing my Multi-Role Entry Criteria: '%s'", mFailedGameEntryCriteriaName.c_str());
            }

            if (!(otherSession.mMultiRoleEntryCriteriaEvaluator.evaluateEntryCriteria(mMatchmakingSupplementalData.mPlayerJoinData,
                                                                                      (uint16_t)mMatchmakingSupplementalData.mJoiningPlayerCount, mFailedGameEntryCriteriaName)) )
            {
                aggregateSessionMatchInfo.sOtherMatchInfo.setNoMatch();
                matchFailureMsg.writeMessage("Due to other player failing my Multi-Role Entry Criteria: '%s'", mFailedGameEntryCriteriaName.c_str());
            }


            if (aggregateSessionMatchInfo.isMisMatch())
            {
                shouldEvaluateSessions = false;
            }
        }

        bool samePriviledgedGameSetting = (getPrivilegedGameIdPreference() == otherSession.getPrivilegedGameIdPreference());
        // make sure that sessions that have a privileged game id preference don't get pulled in by sessions that don't, or that have a different override game
        if (shouldEvaluateSessions && !samePriviledgedGameSetting)
        {

            if ((getPrivilegedGameIdPreference() != INVALID_GAME_ID) && (otherSession.getPrivilegedGameIdPreference() != INVALID_GAME_ID))
            {
                // we have different target games, skip eval of the MM rules, can't allow these sessions to match each other
                shouldEvaluateSessions = false;

                aggregateSessionMatchInfo.sMyMatchInfo.setNoMatch();
                aggregateSessionMatchInfo.sOtherMatchInfo.setNoMatch();
                matchFailureMsg.writeMessage("Due to other player having a privileged game id preference different from my own: %" PRIu64 "", otherSession.getPrivilegedGameIdPreference());
            }
        }

        // evaluate myself vs the otherSession (and vice versa)
        if (shouldEvaluateSessions)
        {
            bool sameDebugness = (getDebugCheckOnly() == otherSession.getDebugCheckOnly());

            // Validate we aren't already switched context to the owning session.
            LogContextOverride logAudit(otherSession.getOwnerUserSessionId());

            mCriteria.evaluateCreateGame(*this, otherSession, aggregateSessionMatchInfo);

            // check me vs other. don't allow debug only sessions to be pulled in by me if we're not both debug only. 
            // Ensure sessions with privileged game id preference don't get pulled in by me if I don't have a matching preferred game.
            FitScore myTotalFitScore = aggregateSessionMatchInfo.sMyMatchInfo.sFitScore;
            if ( isFitScoreMatch( myTotalFitScore ) )
            {
                if ( (sameDebugness || !otherSession.getDebugCheckOnly())
                    && (samePriviledgedGameSetting || (otherSession.getPrivilegedGameIdPreference() == INVALID_GAME_ID)) )
                {
                    addMatchingSession(&otherSession, aggregateSessionMatchInfo.sMyMatchInfo, aggregateSessionMatchInfo.sRuleResultMap);
                }
                // increment diagnostics also
                getDiagnostics().setCreateEvaluationsMatched(getDiagnostics().getCreateEvaluationsMatched() + 1);
            }
            getDiagnostics().setCreateEvaluations(getDiagnostics().getCreateEvaluations() + 1);

            // check other vs me.  Don't allow me to be pulled in by non-debug sessions if I'm debug only.
           // Ensure my session, if I have a privileged game id preference doesn't get pulled in by sessions without.
            FitScore otherTotalFitScore = aggregateSessionMatchInfo.sOtherMatchInfo.sFitScore;
            if ( isFitScoreMatch( otherTotalFitScore ) )
            {
                if ( (sameDebugness || !getDebugCheckOnly())
                    && (samePriviledgedGameSetting || (getPrivilegedGameIdPreference() == INVALID_GAME_ID)) )
                {
                    otherSession.addMatchingSession(this, aggregateSessionMatchInfo.sOtherMatchInfo, aggregateSessionMatchInfo.sRuleResultMap);
                }
                // increment diagnostics also
                otherSession.getDiagnostics().setCreateEvaluationsMatched(otherSession.getDiagnostics().getCreateEvaluationsMatched() + 1);
            }
            otherSession.getDiagnostics().setCreateEvaluations(otherSession.getDiagnostics().getCreateEvaluations() + 1);
        }
        else
        {
            // sessions didn't match
            aggregateSessionMatchInfo.setMisMatch();
        }

        // log (failure) results from both bi-directional perspectives
        if ( !isFitScoreMatch(aggregateSessionMatchInfo.sMyMatchInfo.sFitScore) )
        {
            TRACE_LOG(LOG_PREFIX << "evaluated CreateGame Session " << getMMSessionId() << " player " << getOwnerBlazeId() 
                << " (" << getOwnerPersonaName() << ") against session " << otherSession.getMMSessionId() 
                << " player " << otherSession.getOwnerBlazeId() << " (" 
                << otherSession.getOwnerPersonaName() << ") - NO_MATCH : " << matchFailureMsg.getMessage());
        }

        if ( !isFitScoreMatch(aggregateSessionMatchInfo.sOtherMatchInfo.sFitScore) && shouldEvaluateSessions)
        {
            TRACE_LOG(LOG_PREFIX << "evaluated CreateGame Session " << otherSession.getMMSessionId() << " player " 
                << otherSession.getOwnerBlazeId() << " (" << otherSession.getOwnerPersonaName() 
                << ") against session " << getMMSessionId() << " player " << getOwnerBlazeId() << " (" 
                << getOwnerPersonaName() << ") - NO_MATCH : " << matchFailureMsg.getMessage());
        }

    }


    /*! ************************************************************************************************/
    /*! \brief try to finalize this session as createGame.  Returns true on success; also returns a list of
            additional sessions that were finalized (the sessions that joined this game).

        \param[out] finalizedSessionList - filled out with all the sessions that were finalized (includes this session, and any
                        other sessions that joined the game).
        \param[in] finalized teamIdVector of suggested team ids for the game as determined by the team size rules
        \return true if this session was able to finalize (create/reset a game)
    ***************************************************************************************************/
    bool MatchmakingSession::attemptCreateGameFinalization(MatchmakingSessionList& finalizedSessionList, CreateGameFinalizationSettings& createGameFinalizationSettings)
    {
        mMatchmaker.getMatchmakerMetrics(this).mTotalMatchmakingCreateGameFinalizeAttempts.increment();
        EA_ASSERT(finalizedSessionList.empty());
        EA_ASSERT(isCreateGameEnabled());

        TRACE_LOG(LOG_PREFIX << " Session(" << mMMSessionId << ") - attempting to finalize session as CreateGame.");

        uint16_t playerCapacity = mCriteria.getParticipantCapacity();
        if (playerCapacity == 0)
        {
            ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/" << getOwnerPersonaName() << ")].attemptCreateGameFinalization Team Size, Game Size, Total Player Slots, Team Composition and all other player capacity determining rules are all disabled, cannot determine the player capacity.");
            return false;
        }

        // specific sorter for team size rule which takes the session size into account.  This is
        // to provide a better packing algorithm for filling out teams.  Allowing smaller sessions
        // in first can potentially leave you un full games.
        if (mCriteria.getTeamCountFromRule() > 1)
        {
            if (mGameCreationData.getRoleInformation().getRoleCriteriaMap().size() > 1)
                mMatchedToList.sort( MatchedToListTeamsMultiRolesCompare() );
            else
                mMatchedToList.sort( MatchedToListTeamsCompare() );
        }
        else
        {
            mMatchedToList.sort( MatchedToListCompare() );
        }

        // matched buckets are always sorted at item insert.

        // Build out the game session.
        if (!buildGameSessionList(createGameFinalizationSettings))
        {
            return false;
        }

        // Note: We no longer choose the host from the full list of matched sessions.
        // Due to various rules and a desire to provide the "best" match,
        // we only pick the host from the players that made it through buildGameSessionList
        if (isPlayerHostedTopology(mGameCreationData.getNetworkTopology()))
        {
            chooseGameHost(createGameFinalizationSettings);
        }
        else
        {
            // For dedicated servers, order our preferred data centers if the expanded ping site rule is enabled
            if (getCriteria().isExpandedPingSiteRuleEnabled())
            {
                TRACE_LOG("[MatchmakingSession].attemptCreateGameFinalization: Expanded ping site rule is enabled. Determining preferred ping site.");

                orderPreferredPingSites(createGameFinalizationSettings);
            }
        }

        // final check that the selected team members satisfy the team size requirements
        if (mCriteria.isAnyTeamRuleEnabled())
        {
            // out var, set when we validate team sizes are sufficient.
            uint16_t teamSizeDifference = 0;
            TeamIndex largestTeamIndex = UNSPECIFIED_TEAM_INDEX;
            if (!createGameFinalizationSettings.areTeamSizesSufficientToCreateGame(mCriteria, teamSizeDifference, largestTeamIndex))
            {
                // We fail immediately for any error but the team size difference.  
                if (teamSizeDifference <= mCriteria.getAcceptableTeamBalance())
                {
                    return false;
                }
                // if we failed due to our team size difference, see if we can remove players to accomplish
                // a match.

                const MatchmakingSession& hostOrAutoJoinMatchingSession = createGameFinalizationSettings.mAutoJoinMemberInfo->mMMSession;

                bool ableToFinalize = false;

                // AUDIT: seems like we could do this more efficiently by just keeping track of
                // player sizes for each finalizing session.  The matchedToList can be rather large.

                // For teams, mMatchedToList is sorted by session size, largest to smallest.
                // We loop through the matchedToList only once.  So if a player is skipped
                // we do not loop back through to check them again.
                // We loop until we no longer meet our overall size requirements or we have
                // exhausted the number of players.
                MatchedToList::reverse_iterator rIter = mMatchedToList.rbegin();
                MatchedToList::reverse_iterator rEnd = mMatchedToList.rend();
                for (; rIter != rEnd; ++rIter)
                {
                    const MatchmakingSession* matchingSession = ((MatchmakingSessionMatchNode&)*rIter).sMatchedToSession;

                    // we skip anyone not on the currently largest team.
                    bool onLargestTeam = false;
                    const TeamIdSizeList* teamIdSizeList = matchingSession->getCriteria().getTeamIdSizeListFromRule();
                    if (teamIdSizeList == nullptr)
                    {
                        TRACE_LOG(LOG_PREFIX << " Session(" << mMMSessionId << ") - Missing Team Size information.");
                        return false;
                    }

                    TeamIdSizeList::const_iterator curTeamId = teamIdSizeList->begin();
                    TeamIdSizeList::const_iterator endTeamId = teamIdSizeList->end();
                    for (; curTeamId != endTeamId; ++curTeamId)
                    {
                        TeamId teamId = curTeamId->first;
                        if (createGameFinalizationSettings.getTeamIndexForMMSession(*matchingSession, teamId) == largestTeamIndex)
                        {
                            onLargestTeam = true;
                            break;
                        }
                    }

                    if (!onLargestTeam)
                        continue;

                    // Check to make sure we don't remove auto join session as well as our finalizing session.
                    if ( (matchingSession->getMMSessionId() == hostOrAutoJoinMatchingSession.getMMSessionId()) ||
                         (matchingSession->getMMSessionId() == getMMSessionId()) )
                    {
                        continue;
                    }

                    if (!createGameFinalizationSettings.removeSessionFromTeams(matchingSession))
                    {
                        TRACE_LOG(LOG_PREFIX << " Session(" << mMMSessionId << ") - Failed removing Session(" << matchingSession->getMMSessionId() << " while attempting to achieve team balance.");
                        return false;
                    }

                    TRACE_LOG("[MatchmakingSession] Session(" << getMMSessionId() << ") Removing session(" << matchingSession->getMMSessionId() << ") from finalization to try and meet team size difference criteria(" << mCriteria.getAcceptableTeamBalance() << ").");

                    // Check if now able to finalize.
                    teamSizeDifference = 0;
                    if (createGameFinalizationSettings.areTeamSizesSufficientToCreateGame(mCriteria, teamSizeDifference, largestTeamIndex))
                    {
                        ableToFinalize = true;
                        break;
                    }
                    else if (teamSizeDifference <= mCriteria.getAcceptableTeamBalance())
                    {
                        // We fail immediately for any error but the team size difference.  
                        return false;
                    }
                    // otherwise we've gone too far on this session, well loop back around with the next guy.
                }

                if (!ableToFinalize)
                {
                    TRACE_LOG(LOG_PREFIX << " Session(" << mMMSessionId << ") - Failed to obtain sufficient team balance by removing sessions.");
                    return false;
                }
            }
        }
        // if mAutoJoinMemberInfo's user session doesn't have host permission, cancel finalization, no host was found.
        // Exception: if only making reservations in game, allow finalizing without host permission.
        if ( EA_UNLIKELY(createGameFinalizationSettings.mAutoJoinMemberInfo == nullptr) 
            || (isPlayerHostedTopology(mGameCreationData.getNetworkTopology()) 
                && (!createGameFinalizationSettings.mAutoJoinMemberInfo->mUserSessionInfo.getHasHostPermission() && !createGameFinalizationSettings.areAllSessionsMakingReservations())))
        {
            TRACE_LOG("[MatchmakingSession]  Session(" << getMMSessionId() << ") Could not finalize - Auto joining member can't be host.");
            return false;
        }

        uint16_t numFinalizingPlayers = createGameFinalizationSettings.calcFinalizingPlayerCount();

        // Do the slots rules checks after balance
        if (mCriteria.isPlayerCountRuleEnabled())
        {
            // Note that this code will attempt to remove players if the total number of players doesn't match their requirements.
            // Since this occurs after the teams were setup, it's possible that the resulting game may fail the team rules. 
            // The only way to avoid that is to do more passes over the sessions, and attempt the teams again, but that gets into GamePacker levels of complexity. 
            const MatchmakingSession& hostOrAutoJoinMatchingSession = createGameFinalizationSettings.mAutoJoinMemberInfo->mMMSession;
            const MatchmakingSessionList &owningSessionList = createGameFinalizationSettings.getMatchingSessionList();
            for (MatchmakingSessionList::const_iterator owningIter = owningSessionList.begin(); (owningIter != owningSessionList.end());)
            {
                const MatchmakingSession* matchedSession = *owningIter;
                if (matchedSession == nullptr)
                {
                    ERR_LOG("[MatchmakingSession]  Session(" << getMMSessionId() << ") Could not finalize - Number of finalizing players(" << numFinalizingPlayers
                        << " because of nullptr Session in finalizing list.");
                    return false;
                }
                if (!matchedSession->getCriteria().getPlayerCountRule()->isParticipantCountSufficientToCreateGame(numFinalizingPlayers))
                {
                    // Check to make sure we don't remove auto join session as well as our finalizing session.
                    if ((matchedSession->getMMSessionId() == hostOrAutoJoinMatchingSession.getMMSessionId()) ||
                        (matchedSession->getMMSessionId() == getMMSessionId()))
                    {
                        TRACE_LOG("[MatchmakingSession]  Session(" << getMMSessionId() << ") Could not finalize - Number of finalizing players(" << numFinalizingPlayers
                            << ") less than minimum needed (" << mCriteria.getPlayerCountRule()->calcMinPlayerCountAccepted() << ") by required session (" << matchedSession->getMMSessionId() << ").");
                        return false;
                    }

                    // If we hit this, it means that this session didn't accept the current player count. 
                    // So we see if we can just kick them and continue:
                    if (!createGameFinalizationSettings.removeSessionFromTeams(matchedSession))
                    {
                        TRACE_LOG(LOG_PREFIX << " Session(" << mMMSessionId << ") - Failed removing Session(" << matchedSession->getMMSessionId() << " while due to player count.");
                        return false;
                    }

                    TRACE_LOG("[MatchmakingSession] Session(" << mMMSessionId << ") Removing session(" << matchedSession->getMMSessionId() << ") from finalization due to player count rule.");
                    owningIter = owningSessionList.begin();
                }
                else
                {
                    ++owningIter;
                }
            }

            numFinalizingPlayers = createGameFinalizationSettings.calcFinalizingPlayerCount();
            uint16_t teamSizeDiff;
            UserExtendedDataValue teamUedDiff;
            bool teamsAreOkay = (createGameFinalizationSettings.areTeamsAcceptableToCreateGame(getCriteria(), teamSizeDiff, teamUedDiff));
            bool singleGroupMatchCheckOkay = ((!isSingleGroupMatch()) || (owningSessionList.size() > 1));
            if (!mCriteria.getPlayerCountRule()->isParticipantCountSufficientToCreateGame(numFinalizingPlayers) || !teamsAreOkay || !singleGroupMatchCheckOkay)
            {
                TRACE_LOG("[MatchmakingSession]  Session(" << getMMSessionId() << ") Could not finalize - Number of finalizing players(" << numFinalizingPlayers
                    << ") less than minimum needed (" << mCriteria.getPlayerCountRule()->calcMinPlayerCountAccepted() << ") by owner session." 
                    << (teamsAreOkay ? "" : " Teams were not acceptable.")
                    << (singleGroupMatchCheckOkay ? "" : " Single group match failed."));
                return false;
            }
        }
        else
        {
            // we're not PlayerCountRule. Check the global minimum
            if(numFinalizingPlayers < mMatchmaker.getGlobalMinPlayerCountInGame())
            {
                TRACE_LOG("[MatchmakingSession]  Session(" << getMMSessionId() << ") Could not finalize - Number of finalizing players(" << numFinalizingPlayers << ") less than global minimum needed(" <<  mMatchmaker.getGlobalMinPlayerCountInGame() << ").");
                return false;
            }
        }

        // We send the final async status here for each finalizing session.
        const MatchmakingSessionList &joiningSessionList = createGameFinalizationSettings.getMatchingSessionList();
        MatchmakingSessionList::const_iterator joiningSessionIter = joiningSessionList.begin();
        MatchmakingSessionList::const_iterator joiningSessionEnd = joiningSessionList.end();

        for ( ; joiningSessionIter != joiningSessionEnd; ++joiningSessionIter )
        {
            const MatchmakingSession *joiningSession = *joiningSessionIter;
            // Have to do the ugly cast so we can set the rule mode in the rule
            MatchmakingSession* session = const_cast<MatchmakingSession*>(joiningSession);
            MatchmakingSession &alreadyJoinedSession = createGameFinalizationSettings.mAutoJoinMemberInfo->mMMSession;

            if (joiningSession != &alreadyJoinedSession)
            {
                // for sessions joined new created game, we will send the last notification which shares the same information
                // as "this" session here
                session->updateCreateGameAsyncStatus(this->mMatchmakingAsyncStatus);
                session->sendFinalMatchmakingStatus();
            }
        }

        // we initialize the create request here so we can trigger voting on game attributes
        // don't want to create the job and deal with reinsertion if our vote is not acceptable
        // create request is stored in the settings
        if (!initCreateGameRequest(createGameFinalizationSettings))
        {
            TRACE_LOG("[MatchmakingSession].attemptCreateGameFinalization: Failed to initialize create game request.");
            return false;
        }

        // evaluate CG request to ensure all members accept the results
        // Now fill in setup reasons for every session we matched against and their sub members.
        GameManager::Matchmaker::MatchmakingSessionList::const_iterator sessionReqEvalIter = createGameFinalizationSettings.getMatchingSessionList().begin();
        GameManager::Matchmaker::MatchmakingSessionList::const_iterator sessionReqEvalEnd = createGameFinalizationSettings.getMatchingSessionList().end();
        for (; sessionReqEvalIter != sessionReqEvalEnd; ++sessionReqEvalIter)
        {
            const GameManager::Matchmaker::MatchmakingSession *mmSession = *sessionReqEvalIter;

            LogContextOverride logAudit(mmSession->getOwnerUserSessionId());

            // prevent validation of your own game request.
            if (mmSession->getMMSessionId() != getMMSessionId())
            {
                if(!mmSession->evaluateFinalCreateGameRequest(createGameFinalizationSettings.getCreateGameRequest()))
                {
                    TRACE_LOG("[MatchmakingSession].attemptCreateGameFinalization: matchmaking session " << mmSession->getMMSessionId() << " failed to accept the create game request of finalizing session " << getMMSessionId() << ".");
                    return false;
                }
            }
        }

        // see if there's already a job for this mm session
        if (EA_UNLIKELY(mMatchmakerSlave.getMatchmakingJob(mMMSessionId) != nullptr))
        {
            // return false after deleting the job because we won't re-insert sessions 
            // because we've not already started this new (duplicate) job.
            ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/" << getOwnerPersonaName() 
                << ")].attemptCreateGameFinalization attempted to finalize with an existing job, current lockdown status is '"
                << isLockedDown() << "'.");
            // this will lock down the owning session
            setLockdown();
            return false;
        }

        else
        {
            // Fork a job to do the actually creating/joining.
            Blaze::Matchmaker::FinalizeCreateGameMatchmakingJob *job =
                BLAZE_NEW Blaze::Matchmaker::FinalizeCreateGameMatchmakingJob(mMatchmaker, getOwnerUserSessionId(), mMMSessionId, &createGameFinalizationSettings);
            if (EA_LIKELY(mMatchmakerSlave.addMatchmakingJob(*job)))
            {
                // For simplicity, we just use the max fit score, for final fit score vs myself.
                setCreateGameFinalizationFitScore(mCriteria.calcMaxPossibleFitScore());

                //ASSUMPTION: At this point we'll have a full list of sessions in the finalized session list that
                //are ALL owner sessions.  They should all have valid member info lists.

                // Depending on the gameNetworkTopology, either create a new game, or claim an existing dedicated server
                //   (voting on the game settings as appropriate)

                // we're going to 'remove' all of these sessions to join after we create the game. They will be tracked by the newly created job.
                // copies the vector
                finalizedSessionList = createGameFinalizationSettings.getMatchingSessionList();

                // creatGameFinalizationSettings is cleaned up when the job finished.
                return true;
            }
            else
            {
                // this should never happen, because the get before we create the finalization job returning nullptr should have guaranteed the add will return true.
                // can't delete the job here because both the caller and the job think they own the createGameFinalizationSettings, and both will delete it
                ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/" << getOwnerPersonaName()
                    << ")].attemptCreateGameFinalization attempted to finalize, but couldn't add job, current lockdown status is '"
                    << isLockedDown() << "'.");
                // this will lock down the owning session
                setLockdown();
                // return false after creating a job we can't add because we won't re-insert sessions
                // because we've not already started this new (duplicate) job.
                return false;
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief chooses the 'best' session to host a game (the session with the least restrictive NAT type,
            that's small enough to allow this session to join the game).

        \param[in,out] finalizationSettings requires the playerCapacity be set already, chooses & sets the session to host the game
    ***************************************************************************************************/
    const void MatchmakingSession::chooseGameHost( CreateGameFinalizationSettings &createGameFinalizationSettings ) const
    {
        EA_ASSERT(createGameFinalizationSettings.getPlayerCapacity() > 0);

        // start off with the session owner as host, then try to find someone better (who is small enough
        //    to allow the session owner to join).
        const MemberInfo *result = &getHostMemberInfo();

        BestNetworkHostComparitor comparator;
        uint32_t mySessionSize = getPlayerCount();


        if (mySessionSize <= createGameFinalizationSettings.getPlayerCapacity())
        {
            // find the best session (of an acceptable size, with the most lenient NAT type)
            uint32_t maxHostSessionSize = createGameFinalizationSettings.getPlayerCapacity() - mySessionSize; // ensure there's room for this MM session owner (and group) to join

            MatchmakingSessionList::const_iterator finalizedSessionIter = createGameFinalizationSettings.getMatchingSessionList().begin();
            MatchmakingSessionList::const_iterator finalizedSessionEnd = createGameFinalizationSettings.getMatchingSessionList().end();

            for ( ; finalizedSessionIter != finalizedSessionEnd;  ++finalizedSessionIter)
            {
                const MatchmakingSession *currentMMSession =  *finalizedSessionIter;

                // get the best host session for current session, which could be the current session
                // or one of a member session if the current session is triggered by a user group
                const MemberInfo &testHost = currentMMSession->getHostMemberInfo();

                // this session can't host, skip it
                if ( EA_UNLIKELY(!testHost.mUserSessionInfo.getHasHostPermission()))
                {
                    continue;
                }

                // This session size includes all teams:
                uint16_t currentSessionSize = currentMMSession->getPlayerCount();
                if (currentSessionSize <= maxHostSessionSize)
                {
                    // Team size check logic is done all over the place. No need to do it again here.

                    // See if the test host is better (lower) than the current best host
                    if (comparator(testHost, *result) && result->isPotentialHost())
                    {
                        // found a better host
                        result = &testHost;
                    }
                }
            }
        }
        else
        {
            // my session fills the entire game, so I have to be the host
        }

        // likely because we don't let a MM session without a host enter CG mode.
        if (EA_LIKELY((result != nullptr) && gUserSessionManager->getSessionExists(result->mUserSessionInfo.getSessionId())))
        {
            TRACE_LOG(LOG_PREFIX << " matchmaking session " << mMMSessionId << " - selected session " << result->mMMSession.getMMSessionId()
                  << " to be the game host.  SessionOwner: " << result->mUserSessionInfo.getUserInfo().getPersonaName() << " (playerId "
                  << result->mUserSessionInfo.getUserInfo().getId() << "), SessionSize: " << result->mMMSession.getPlayerCount() << ", NatType: "
                  << NatTypeToString(result->mUserSessionInfo.getQosData().getNatType()) << ".");
        }
        else
        {
            TRACE_LOG(LOG_PREFIX << " matchmaking session " << mMMSessionId << " - could not find a host during finalization and could not supply its own host." 
                << (((result != nullptr) && !gUserSessionManager->getSessionExists(result->mUserSessionInfo.getSessionId()))? " No user session found for selected host MM session." : ""));
        }

        createGameFinalizationSettings.mAutoJoinMemberInfo = result;
    }

    void MatchmakingSession::orderPreferredPingSites(CreateGameFinalizationSettings &createGameFinalizationSettings) const
    {
        const ExpandedPingSiteRule& rule = *getCriteria().getExpandedPingSiteRule();

        LatenciesByAcceptedPingSiteIntersection latenciesByAcceptedPingSiteIntersection;
        for (ExpandedPingSiteRule::LatencyByAcceptedPingSiteMap::const_iterator iter = rule.getLatencyByAcceptedPingSiteMap().begin(),
             end = rule.getLatencyByAcceptedPingSiteMap().end();
             iter != end;
             ++iter)
        {
            latenciesByAcceptedPingSiteIntersection[iter->first].push_back(iter->second);
        }

        OrderedPreferredPingSiteList orderedPreferredPingSites;

        if (latenciesByAcceptedPingSiteIntersection.size() == 1)
        {
            // Short cut here if we only accept a single ping site; this one will have to be our preferred
            orderedPreferredPingSites.push_back(eastl::make_pair(*latenciesByAcceptedPingSiteIntersection.begin()->second.begin(),
                latenciesByAcceptedPingSiteIntersection.begin()->first));
            TRACE_LOG("[MatchmakingSession].orderPreferredPingSites: Only one ping site acceptable. createGameFinalizationSettings.mPreferredPingSite = " <<
                latenciesByAcceptedPingSiteIntersection.begin()->first);
            latenciesByAcceptedPingSiteIntersection.clear(); // Clear out any partial intersection
        }
        else
        {
            for (MatchmakingSessionList::const_iterator iter = createGameFinalizationSettings.getMatchingSessionList().begin(),
                 end = createGameFinalizationSettings.getMatchingSessionList().end();
                 iter != end;
                 ++iter)
            {
                const MatchmakingSession* session = *iter;

                // Skip ourselves, because we already added our values above.
                if (session->getMMSessionId() == getMMSessionId())
                    continue;

                const ExpandedPingSiteRule::LatencyByAcceptedPingSiteMap& otherAcceptedPingSites = session->getCriteria().getExpandedPingSiteRule()->getLatencyByAcceptedPingSiteMap();
                if (otherAcceptedPingSites.size() == 1)
                {
                    // Short cut here if any of our sessions only accept a single ping site; this one will have to be our preferred
                    orderedPreferredPingSites.push_back(eastl::make_pair(otherAcceptedPingSites.begin()->second, otherAcceptedPingSites.begin()->first));
                    TRACE_LOG("[MatchmakingSession].orderPreferredPingSites: Only one ping site acceptable. createGameFinalizationSettings.mPreferredPingSite = " <<
                        otherAcceptedPingSites.begin()->first);
                    latenciesByAcceptedPingSiteIntersection.clear(); // Clear out any partial intersection
                    break;
                }
                else
                {
                    // Build the set intersection of all accepted ping sites for all sessions
                    // We'll choose from this set based on our rule configuration
                    // For each ping site, we need to store this session's latency so that we can compute the preferred ping site (e.g., lowest standard deviation)
                    for (LatenciesByAcceptedPingSiteIntersection::iterator intersectionIter = latenciesByAcceptedPingSiteIntersection.begin(),
                        intersectionEnd = latenciesByAcceptedPingSiteIntersection.end();
                        intersectionIter != intersectionEnd;)
                    {
                        ExpandedPingSiteRule::LatencyByAcceptedPingSiteMap::const_iterator otherIter = otherAcceptedPingSites.find(intersectionIter->first);
                        if (otherIter == otherAcceptedPingSites.end())
                        {
                            TRACE_LOG("[MatchmakingSession].orderPreferredPingSites: Removing ping site " << intersectionIter->first << " from set intersection.");
                            intersectionIter = latenciesByAcceptedPingSiteIntersection.erase(intersectionIter);
                        }
                        else
                        {
                            intersectionIter->second.push_back(otherIter->second);
                            ++intersectionIter;
                        }
                    }
                }
            }
        }

        createGameFinalizationSettings.buildOrderedAcceptedPingSiteList(rule, latenciesByAcceptedPingSiteIntersection, orderedPreferredPingSites);
    }

    const char8_t* MatchmakingSession::getCreateGameTemplateName() const
    {
        const char8_t* createTemplateName = "";
        const auto& scenariosMap = mMatchmakerSlave.getConfig().getScenariosConfig().getScenarios();
        auto scenarioCfgItr = scenariosMap.find(mScenarioInfo.getScenarioName());
        if (scenarioCfgItr != scenariosMap.end())
        {
            auto subsessionCfgItr = scenarioCfgItr->second->getSubSessions().find(mScenarioInfo.getSubSessionName());
            if (subsessionCfgItr != scenarioCfgItr->second->getSubSessions().end())
            {
                createTemplateName = subsessionCfgItr->second->getCreateGameTemplate();
            }
        }
        return createTemplateName;
    }

        /*! ************************************************************************************************/
    /*! \brief  selects and joins a team. Called by owner/host sessions, 
            before rule based criteria joins for remaining sessions.

        \param[in/out] finalizationSettings - our running status of our finalization
        \param[in/out] teamInfo - the selected team information
     ***************************************************************************************************/
    bool MatchmakingSession::selectTeams(CreateGameFinalizationSettings& finalizationSettings, CreateGameFinalizationTeamInfo*& teamInfo) const
    {
        // The composition rule select teams themselves for each player.  Just try and join here as long as the team has been selected.
        // If no team selected, and composition rule in use, we can use the normal(smallest) team selection below.
        const TeamIdSizeList* myTeamIds = getCriteria().getTeamIdSizeListFromRule();
        if (myTeamIds == nullptr)
        {
            TRACE_LOG("[MatchmakingSession].selectTeam - Session(" << getMMSessionId() 
                << ") is missing team ids.");
            return false;
        }

        if (getCriteria().hasTeamCompositionBasedFinalizationRulesEnabled() && teamInfo != nullptr) 
        {
            if (myTeamIds->size() != 1)
            {
                TRACE_LOG("[MatchmakingSession].selectTeam - Session(" << getMMSessionId() 
                    << ") attempted to use TeamCompositionBasedFinalizationRules with multiple team join.");
                return false;
            }

            if ( teamInfo->joinTeam(*this, myTeamIds->begin()->first) )
            {
                return true;
            }

            TRACE_LOG("[MatchmakingSession].selectTeam - Session(" << getMMSessionId() << ") failed to join team(" << teamInfo->mTeamId << ").");
            return false;
        }

        // Non compositional use of teams below

        // First, gather all the joins:
        eastl::set<CreateGameFinalizationTeamInfo*> teamInfoSet;
        typedef eastl::vector<eastl::pair<TeamId, CreateGameFinalizationTeamInfo*> > TeamIdToTeamInfoVector;
        TeamIdToTeamInfoVector teamsAssigned;

        TeamIdSizeList::const_iterator teamIdSize = myTeamIds->begin();
        TeamIdSizeList::const_iterator teamIdSizeEnd = myTeamIds->end();
        for (; teamIdSize != teamIdSizeEnd; ++teamIdSize)
        {
            TeamId sessionTeamId = teamIdSize->first;
            uint16_t sessionSize = teamIdSize->second;

            teamInfo = finalizationSettings.getSmallestAvailableTeam(sessionTeamId, teamInfoSet);
            if (teamInfo == nullptr || !teamInfo->canPotentiallyJoinTeam(*this, true, sessionTeamId))
            {
                if ( (teamInfo != nullptr) && !mCriteria.getDuplicateTeamsAllowed() && (sessionTeamId != ANY_TEAM_ID))
                {
                    TRACE_LOG("[MatchmakingSession].selectTeam - Session(" << getMMSessionId() 
                        << ") of size " << sessionSize << " failed to join team(" << sessionTeamId << ") unable to search for ANY because duplicate teams not allowed.");
                    return false;
                }

                TRACE_LOG("[MatchmakingSession].selectTeam - Session(" << getMMSessionId()
                            << ") of size " << sessionSize << " failed to join team(" << sessionTeamId << ") attempting to find unset team.");

                teamInfo = finalizationSettings.getSmallestAvailableTeam(ANY_TEAM_ID, teamInfoSet);
                if (teamInfo == nullptr || !teamInfo->canPotentiallyJoinTeam(*this, false, sessionTeamId))
                {
                    TRACE_LOG("[MatchmakingSession].selectTeam - Session(" << getMMSessionId()
                                << ") of size " << sessionSize << " failed to find unset team.");
                    return false;
                }
            }

            teamInfoSet.insert(teamInfo);
            teamsAssigned.push_back(eastl::make_pair(sessionTeamId, teamInfo));
        }

        // Now join all the teams that we found:
        TeamIdToTeamInfoVector::const_iterator teamAssigned = teamsAssigned.begin();
        TeamIdToTeamInfoVector::const_iterator teamAssignedEnd = teamsAssigned.end();
        for (; teamAssigned != teamAssignedEnd; ++teamAssigned)
        {
            TeamId sessionTeamId = teamAssigned->first;
            teamInfo = teamAssigned->second;
            if (!teamInfo->joinTeam(*this, sessionTeamId))
            {
                TRACE_LOG("[MatchmakingSession].selectTeam - Session(" << getMMSessionId()
                    << ") failed to find join team that was potentially joinable.");
                return false;
            }
        }
        
        return true;
    }

    void MatchmakingSession::scheduleAsyncStatus()
    {
        if (mStatusTimerId != INVALID_TIMER_ID)
            gSelector->cancelTimer(mStatusTimerId); 
        const auto now = TimeValue::getTimeOfDay();
        const auto nextAsyncNotificationTime = now + mMatchmaker.getMatchmakingConfig().getServerConfig().getStatusNotificationPeriod();
        if (nextAsyncNotificationTime < getSessionExpirationTime())
            mStatusTimerId = gSelector->scheduleTimerCall(nextAsyncNotificationTime, this, &MatchmakingSession::sendAsyncStatusSession, "MatchmakingSession::sendAsyncStatusSession");
    }

    void MatchmakingSession::scheduleExpiry()
    {
        if (mExpiryTimerId != INVALID_TIMER_ID)
            gSelector->cancelTimer(mExpiryTimerId);
        mExpiryTimerId = gSelector->scheduleTimerCall(getSessionExpirationTime(), this, &MatchmakingSession::timeoutSession, "MatchmakingSession::timeoutSession");
    }

    /*! ************************************************************************************************/
    /*! \brief given a playerCapacity & hostPlayer (if peer hosted topology), choose the other matching sessions
            that need to explicitly join the game.

        \param[in,out] finalizationSettings requires the playerCapacity & hostPlayer (if any) to be set
            already, chooses & sets the session to host the game
        \return false if unable to select enough sessions to satisfy criteria (required for group matching where we must select at least 1 other group)
    ***************************************************************************************************/
    bool MatchmakingSession::buildGameSessionList(CreateGameFinalizationSettings &finalizationSettings) const
    {
        EA_ASSERT(finalizationSettings.getMatchingSessionList().empty());

        // 1) put me and my game group on a team
        // 2) Put the good host on a team
        //      - choose good host already ensures that they can get on the team
        //      - there is probably a bug here around capacities of different sizes ie good host takes up 1 slot on team 1 (max size 3), another team 1 (max size 1), my PG can't join size 3.
        // 3) Fill in the rest of the spaces on the teams.

        // Auto join session is the finalizing session.  Host is chosen after this.
        const MatchmakingSession& autoJoinMatchingSession = finalizationSettings.mAutoJoinMemberInfo->mMMSession;

        CreateGameFinalizationTeamInfo* teamInfo = nullptr;

        if (!finalizationSettings.canFinalizeWithSession(this, &autoJoinMatchingSession, teamInfo))
        {
            return false;
        }

        finalizationSettings.addSessionToGameSessionList(autoJoinMatchingSession, teamInfo);


        // continue filling player slots until we're either full, or we've run out of matches
        //  Note: we may skip matches if there's not room for all grouped players (group members)

        TRACE_LOG(LOG_PREFIX << " Session(" << mMMSessionId << ") - choosing sessions to join game; attempting to fill " 
                  << finalizationSettings.getPlayerSlotsRemaining() << " remaining player slots");


        // Add remaining sessions by team UED and team composition related criteria
        // if the rules are enabled.
        if (getCriteria().hasTeamCompositionBasedFinalizationRulesEnabled())
        {
            return addSessionsToFinalizationByTeamCriteria(finalizationSettings);
        }

        // No team composition related rules enabled. Default behavior joins largest sessions into smallest teams below.
        // MM_AUDIT: remove/merge below remaining 'deprecated' code, 
        // once we can get rid of single group join as it looks like that isn't accounted for in the teams finalization yet.
        MatchedToList::const_iterator matchedSessionIter = mMatchedToList.begin();
        MatchedToList::const_iterator matchedSessionEnd = mMatchedToList.end();
        for(; (matchedSessionIter != matchedSessionEnd) && (finalizationSettings.getPlayerSlotsRemaining() > 0); ++matchedSessionIter)
        {
            MatchmakingSessionMatchNode& node = (MatchmakingSessionMatchNode&)*matchedSessionIter;
            const MatchmakingSession* matchingSession = node.sMatchedToSession;

            uint16_t matchedSessionSize = matchingSession->getPlayerCount();

            // We don't count it as explicit joining session if the matched session is the host MM session who has joined session
            if (matchingSession->getMMSessionId() == autoJoinMatchingSession.getMMSessionId())
            {
                // The auto join session is not me, add their full node info here.
                // We could not add it above, because we only had their mmSession
                // and not a match node as we have here.
                if (matchingSession != this)
                {
                    finalizationSettings.addDebugInfoForSessionInGameSessionList(node);
                }
                continue;
            }

            if ( finalizationSettings.getPlayerSlotsRemaining() < matchedSessionSize )
            {
                // skipping session; it's too big to fit in this game.
                TRACE_LOG(LOG_PREFIX << " Session(" << mMMSessionId << ") - skipping over matched session " 
                    << matchingSession->getMMSessionId() << " (Owner: " << matchingSession->getOwnerPersonaName() 
                    << ", playerId: " << matchingSession->getOwnerBlazeId() << ", sessionSize: " 
                    << (int32_t)matchingSession->getPlayerCount() << ").  Session to too big to join game, " << finalizationSettings.getPlayerSlotsRemaining()
                    << " player slots remaining.");
                continue;
            }

            if (!finalizationSettings.canFinalizeWithSession(this, matchingSession, teamInfo))
            {
                continue;
            }

            // note: modifies playerSlotsRemaining
            // teamInfo is nullptr if teams are disabled
            finalizationSettings.addSessionToGameSessionList(node, teamInfo); 


            // only pick first one that fits for playing against only another group, there can't
            // be other players in MM on the same group.
            if (isSingleGroupMatch()) 
            {
                return true;
            }
        }

        // If it is a single group match and we got here then we didn't select another session
        // and should fail to finalize.
        if (isSingleGroupMatch())
        {
            TRACE_LOG(LOG_PREFIX << " buildGameSessionList - matchmaking session " << mMMSessionId << " must have at least 1 other session to finalize for single group match.");
            return false;
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Attempt to fill remaining slots in the finalization based on TeamComposition/TeamUEDBalance rule criteria.

        \param[in] maxGameTeamCompositionsAttemptsRemaining for each successive finalization attempt that fails,
        finalization may retry on a different game team compositions, while this value is greater than 0.

        \return whether successfully can finalize or not.
    ***************************************************************************************************/
    bool MatchmakingSession::addSessionsToFinalizationByTeamCriteria(CreateGameFinalizationSettings &finalizationSettings) const
    {
        uint16_t maxGameTeamCompositionsAttemptsRemaining = getCriteria().getMaxFinalizationGameTeamCompositionsAttempted();
        uint16_t maxFinalizationPickSequenceRetriesPerComposition = getCriteria().getMaxFinalizationPickSequenceRetries();

        mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizeAttempt.increment();

        // call helper to run on the selected team compositions
        const GameTeamCompositionsInfo* gameTeamCompositionsInfo;
        while (selectNextGameTeamCompositionsForFinalization(gameTeamCompositionsInfo, finalizationSettings))
        {
            const bool isCompositionsEnabled = (gameTeamCompositionsInfo != nullptr);
            if (isCompositionsEnabled)
            {
                finalizationSettings.markGameTeamCompositionsIdTried(gameTeamCompositionsInfo->mGameTeamCompositionsId);
                mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizeCompnsTry.increment();

                // reset for next finalization attempt
                const MatchmakingSession& hostOrAutoJoinSession = finalizationSettings.mAutoJoinMemberInfo->mMMSession;
                const uint16_t overallFirstRepickablePickNumber = finalizationSettings.getOverallFirstRepickablePickNumber(*this);
                finalizationSettings.clearAllSessionsTriedAfterIthPick(overallFirstRepickablePickNumber - 1);
                finalizationSettings.removeSessionsFromTeamsFromIthPickOn(overallFirstRepickablePickNumber, *this, hostOrAutoJoinSession);

                // clear old game team compositions finalization info and assign teams to team compositions as needed.
                if (!finalizationSettings.initTeamCompositionsFinalizationInfo(*gameTeamCompositionsInfo, *this))
                {
                    // we can't even attempt this one. try another (don't count towards max attempts)
                    continue;
                }
            }

            // attempt finalization
            if (addSessionsToFinalizationByTeamCriteriaInternal(finalizationSettings, maxFinalizationPickSequenceRetriesPerComposition))
            {
                TRACE_LOG("[MatchmakingSession].addSessionsToFinalizationByTeamCriteria able to successfully finalize on GameTeamComposition " << finalizationSettings.getGameTeamCompositionsInfo().toLogStr());
                mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizeSuccess.increment();
                mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizeSuccessTeamCount.increment((uint64_t) finalizationSettings.mCreateGameFinalizationTeamInfoVector.size());
                mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizeSuccessPlayerCount.increment(finalizationSettings.getPlayerCapacity() - finalizationSettings.getPlayerSlotsRemaining());
                calcFinalizationByTeamCompositionsSuccessMetrics(finalizationSettings, maxGameTeamCompositionsAttemptsRemaining);
                return true;
            }
            // failed this attempt, if can try another game team compositions, try it next loop.
            if (!isCompositionsEnabled || (maxGameTeamCompositionsAttemptsRemaining-- <= 1))//check first to avoid underflow
            {
                break;
            }
            TRACE_LOG("[MatchmakingSession].addSessionsToFinalizationByTeamCriteria: RETRY with a different game team compositions, after failed attempt on game team compositions(" << finalizationSettings.getGameTeamCompositionsInfo().toLogStr() << "). Compositions retries remaining: " << maxGameTeamCompositionsAttemptsRemaining << ".");
        }

        // no more retries remaining.
        TRACE_LOG("[MatchmakingSession].addSessionsToFinalizationByTeamCriteria: NO MATCH. Returning my session(" << getMMSessionId() << ") to matchmaking pool.");
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief Attempt to fill remaining slots in the finalization based on TeamComposition/TeamUEDBalance rule criteria.
        Pre: If TeamCompositions are enabled, above method has been called to set up any game team compositions criteria.

        \param[in] maxFinalizationPickSequenceRetriesRemaining for each successive finalization pick sequence that fails,
        finalization may retry on a different pick sequence, while this value is greater than 0. Fn decrements this counter.

        \return whether successfully can finalize or not.
    ***************************************************************************************************/
    bool MatchmakingSession::addSessionsToFinalizationByTeamCriteriaInternal(CreateGameFinalizationSettings &finalizationSettings,
        uint16_t maxFinalizationPickSequenceRetriesRemaining) const
    {
        const MatchmakingSession& hostOrAutoJoinSession = finalizationSettings.mAutoJoinMemberInfo->mMMSession;

        mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizePickSequenceTry.increment();

        // stash off original values, in case we are going to backtrack
        const uint16_t playerSlotsRemainingOrig = finalizationSettings.getPlayerSlotsRemaining();
        const uint16_t currentPickNumber = finalizationSettings.getTotalSessionsCount();//typically first non host or auto-join (unless there's fewer matched sessions than max retries and we end up inside a longer 'permutation', see below).

        // while have more possible sessions to add.
        bool hadBucketToTry = false;
        CreateGameFinalizationTeamInfo* teamInfo = nullptr;
        const MatchedSessionsBucket* bucket = nullptr;
        while (selectNextBucketAndTeamForFinalization(bucket, teamInfo, finalizationSettings))//Note: sets teamInfo and bucket to found items, or, nullptr if none found.
        {
            hadBucketToTry = true;
            const MatchmakingSessionMatchNode* matchingNode = selectNextSessionForFinalization(*bucket, *teamInfo, finalizationSettings, hostOrAutoJoinSession);

            if (matchingNode == nullptr)
            {
                // We've exhausted all sessions in the current bucket, for this pick number and team (for this finalization pick sequence).
                // Mark the bucket as 'tried', to skip it next iteration.
                TRACE_LOG("[MatchmakingSession].addSessionsForFinalizationByTeamCriteria: exhausted current " << bucket->toLogStr() << " for " << teamInfo->toLogStr() << ", for pick number " << finalizationSettings.getTotalSessionsCount());
                teamInfo->markBucketTriedForCurrentPick(bucket->mKey, finalizationSettings.getTotalSessionsCount());
                continue;
            }

            // session was not null and should be addable. Mark as tried so we don't try it again for this pick in this sequence.
            const MatchmakingSession* matchingSession = matchingNode->sMatchedToSession;
            teamInfo->markSessionTriedForCurrentPick(matchingSession->getMMSessionId(), finalizationSettings.getTotalSessionsCount());

            if (!finalizationSettings.canFinalizeWithSession(this, matchingSession, teamInfo))
            {
                TRACE_LOG("[MatchmakingSession].addSessionsForFinalizationByTeamCriteria: current " << bucket->toLogStr() << " for " << teamInfo->toLogStr() << ", for pick number " 
                    << finalizationSettings.getTotalSessionsCount() << " skipped matchmaking session ");
                continue;
            }

            // add session to finalization
            finalizationSettings.addSessionToGameSessionList(*matchingNode, teamInfo); // note: modifies playerSlotsRemaining
        }

        // Try to finalize
        uint16_t teamSizeDiff;
        UserExtendedDataValue teamUedDiff;
        if (finalizationSettings.areTeamsAcceptableToCreateGame(getCriteria(), teamSizeDiff, teamUedDiff))
        {
            calcFinalizationByTeamUedSuccessMetrics(getCriteria(), finalizationSettings, maxFinalizationPickSequenceRetriesRemaining, teamSizeDiff, teamUedDiff);
            eastl::string logBuf;
            TRACE_LOG("[MatchmakingSession].addSessionsForFinalizationByTeamCriteria: MATCH. On " << finalizationSettings.pickSequenceToLogStr(logBuf) << " no more possible picks found, but able to finalize.");
            return true;
        }
        calcFinalizationByTeamUedFailPickSeqMetrics(teamSizeDiff, teamUedDiff);

        // We failed going down this path of finalization. We might be able to re-pick/retry below.

        // For efficiency early out if can, to avoid retries if can't add any sessions to start with, and/or, we exhausted
        // all possibilities for 1st pick number (which means there's no more poss pick sequences that can work).
        const bool isDoing1stPickNumber = (currentPickNumber == finalizationSettings.getOverallFirstRepickablePickNumber(*this));
        if (!hadBucketToTry && isDoing1stPickNumber)
        {
            TRACE_LOG("[MatchmakingSession].addSessionsForFinalizationByTeamCriteria: No untried/available sessions to pick from, NO MATCH.");
            return false;
        }

        // Try re-picking sessions, from the first repickable one.  Side: the old pre-Team-criteria packing algorithm, on failure here, would pull out sessions to see if it could then finalize. We don't cover that behavior per se here, but retrying pick sequence also seems to work well enough.
        const bool addedRepickablePick = (finalizationSettings.getTotalSessionsCount() > currentPickNumber);//false if none left
        if (maxFinalizationPickSequenceRetriesRemaining > 0)
        {
            // see which pick number/level we can permutate over.
            const uint16_t permutationsLength = calcMaxPermutablyRepickedSequenceLength(
                maxFinalizationPickSequenceRetriesRemaining, getCriteria().getMaxFinalizationPickSequenceRetries());
            if (permutationsLength == 0)
            {
                return false;//logged
            }

            // Start repicking at the max permutation-length, if there's (more) sessions to try at that level. Else, if
            // exhausted current index of the permutations 'firstRepickablePickNumber-1' to try repick at prior index of it.
            uint16_t toRepickNumber = (addedRepickablePick? permutationsLength : (currentPickNumber - 1));
            if (toRepickNumber < finalizationSettings.getOverallFirstRepickablePickNumber(*this))
                toRepickNumber = finalizationSettings.getOverallFirstRepickablePickNumber(*this);//don't roll back auto-join

            --maxFinalizationPickSequenceRetriesRemaining;
            TRACE_LOG("[MatchmakingSession].addSessionsForFinalizationByTeamCriteria: RETRY after the unsuccessful finalization attempt, on a new pick sequence starting from pick number " << toRepickNumber << ". Retries remaining :" << maxFinalizationPickSequenceRetriesRemaining << ".");

            // reset
            finalizationSettings.clearAllSessionsTriedAfterIthPick(toRepickNumber);
            finalizationSettings.removeSessionsFromTeamsFromIthPickOn(toRepickNumber, *this, hostOrAutoJoinSession);
            finalizationSettings.setPlayerSlotsRemaining(playerSlotsRemainingOrig);

            return addSessionsToFinalizationByTeamCriteriaInternal(finalizationSettings, maxFinalizationPickSequenceRetriesRemaining);
        }

        // no more retries remaining.
        finalizationSettings.setPlayerSlotsRemaining(playerSlotsRemainingOrig);
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief select the GameTeamCompositionsInfo to attempt to finalize with.
        \param[out] gameTeamCompositionsInfo holds result to attempt with, if found. nullptr if not found
            or, to treat as ignoring compositions all together (match any composition).
        \return whether to continue trying to finalize after this selection attempt
    ***************************************************************************************************/
    bool MatchmakingSession::selectNextGameTeamCompositionsForFinalization(
        const GameTeamCompositionsInfo*& gameTeamCompositionsInfo,
        const CreateGameFinalizationSettings& finalizationSettings) const
    {
        // Rule based behavior:
        if (getCriteria().hasTeamCompositionRuleEnabled())
        {
            gameTeamCompositionsInfo = getCriteria().getTeamCompositionRule()->selectNextGameTeamCompositionsForFinalization(
                finalizationSettings);
            return (gameTeamCompositionsInfo != nullptr);
        }

        // Default behavior: No op to effectively treat as having a single 'match any' composition to try.
        gameTeamCompositionsInfo = nullptr;
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief searches for a vacant team which could possibly have additional MM sessions added into it, and,
        the next size/bucket of matchmaking sessions we will try to add from.

        \param[out] bucket holds the resulting bucket to get MM sessions from, if found. nullptr otherwise.
        \param[out] teamToFill holds the resulting team, if found. nullptr otherwise.
        \return True if we found a team and sessions bucket. False of there's no more joinable/untried teams or sessions.
    ***************************************************************************************************/
    bool MatchmakingSession::selectNextBucketAndTeamForFinalization(
        const MatchedSessionsBucket*& bucket, CreateGameFinalizationTeamInfo*& teamToFill,
        CreateGameFinalizationSettings& finalizationSettings) const
    {
        // init to nullptr
        teamToFill = nullptr;
        bucket = nullptr;

        uint16_t currentPickNumber = finalizationSettings.getTotalSessionsCount();
        while (finalizationSettings.hasUntriedTeamsForCurrentPick(currentPickNumber))
        {
            teamToFill = selectNextTeamForFinalization(finalizationSettings);
            if (teamToFill == nullptr)
            {
                SPAM_LOG("[MatchmakingSession].selectNextBucketAndTeamForFinalization: exhausted all teams for the current pick number " << currentPickNumber);
                return false;
            }

            bucket = selectNextBucketForFinalization(*teamToFill, finalizationSettings);
            if (bucket == nullptr)
            {
                // this team has no possible MM sessions (from any buckets) that can go into it. Try another.
                finalizationSettings.markTeamTriedForCurrentPick(teamToFill->mTeamIndex, currentPickNumber);
                teamToFill = nullptr;
                continue;
            }

            // found potential new next team and bucket
            break;
        }
        TRACE_LOG("[MatchmakingSession].selectNextBucketAndTeamForFinalization: found " << ((bucket == nullptr)? "no remaining buckets " : "candidate bucket that") << " can fit into " << ((teamToFill != nullptr)? teamToFill->toLogStr() : "any teams") << ".");
        return (teamToFill != nullptr);
    }

    /*! ************************************************************************************************/
    /*! \brief Return the next team to try to fill for finalization. Returns nullptr if no more teams available to try.
    ***************************************************************************************************/
    CreateGameFinalizationTeamInfo* MatchmakingSession::selectNextTeamForFinalization(
        CreateGameFinalizationSettings& finalizationSettings) const
    {
        CreateGameFinalizationTeamInfo* teamToFill = nullptr;


        // Rule based behavior:
        if (getCriteria().hasTeamUEDBalanceRuleEnabled())
        {
            return getCriteria().getTeamUEDBalanceRule()->selectNextTeamForFinalization(finalizationSettings);
        }


        // Default behavior: tries the smallest sized team which is not marked as tried.
        size_t teamCapacitiesVectorSize = finalizationSettings.mCreateGameFinalizationTeamInfoVector.size();
        for (size_t i = 0; i < teamCapacitiesVectorSize; ++i)
        {
            CreateGameFinalizationTeamInfo& nextTeam = finalizationSettings.mCreateGameFinalizationTeamInfoVector[i];
            if ((nextTeam.mTeamSize == nextTeam.mTeamCapacity) ||
                finalizationSettings.wasTeamTriedForCurrentPick(nextTeam.mTeamIndex, finalizationSettings.getTotalSessionsCount()))
            {
                // this team has no possible MM sessions from any buckets that can go into it, try another team.
                continue;
            }
            if ((teamToFill == nullptr) || (nextTeam.mTeamSize < teamToFill->mTeamSize))
            {
                teamToFill = &nextTeam;
            }
        }

        TRACE_LOG("[MatchmakingSession].selectNextTeamForFinalization: found " << ((teamToFill != nullptr)? teamToFill->toLogStr() : "NO vacant/untried teams") << ".");
        return teamToFill;
    }

    /*! ************************************************************************************************/
    /*! \brief Select the next possible size of matchmaking sessions that will go into the team for finalization
    ***************************************************************************************************/
    const MatchedSessionsBucket* MatchmakingSession::selectNextBucketForFinalization(const CreateGameFinalizationTeamInfo& teamToFill,
        CreateGameFinalizationSettings& finalizationSettings) const
    {
        const uint16_t remainingCap = eastl::min((uint16_t)(teamToFill.mTeamCapacity - teamToFill.mTeamSize), (uint16_t)finalizationSettings.getPlayerSlotsRemaining());
        if (remainingCap == 0)
        {
            SPAM_LOG("[MatchmakingSession].selectNextBucketForFinalization: " << ((finalizationSettings.getPlayerSlotsRemaining() == 0)? "game total slots" : teamToFill.toLogStr()) << " full not selecting more sessions to add to this team.");
            return nullptr;
        }

        // Rule based behavior:
        if (getCriteria().hasTeamCompositionRuleEnabled())
        {
            return getCriteria().getTeamCompositionRule()->selectNextBucketForFinalization(teamToFill, finalizationSettings, mMatchedToBuckets);
        }


        // Default behavior: Find the largest bucket that will fit into the team
        const MatchedSessionsBucket* foundBucket = nullptr;
        for (uint16_t nextBucketKey = remainingCap; (nextBucketKey > 0); --nextBucketKey)
        {
            if (teamToFill.wasBucketTriedForCurrentPick(nextBucketKey, finalizationSettings.getTotalSessionsCount()))
                continue;

            foundBucket = getHighestSessionsBucketInMap(nextBucketKey, nextBucketKey, mMatchedToBuckets);
            if (foundBucket != nullptr)
            {
                SPAM_LOG("[MatchmakingSession].selectNextBucketForFinalization: found candidate bucket for " << teamToFill.toLogStr() << ", " << foundBucket->toLogStr());
                return foundBucket;
            }
        }
        TRACE_LOG("[MatchmakingSession].selectNextBucketForFinalization: found candidate no remaining buckets have sessions that can go into " << teamToFill.toLogStr() << ".");
        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief selects next session to go into the finalization, based on the selected team, bucket, and finalization settings
    ***************************************************************************************************/
    const MatchmakingSessionMatchNode* MatchmakingSession::selectNextSessionForFinalization(const MatchedSessionsBucket& bucket,
        CreateGameFinalizationTeamInfo& teamToFill, CreateGameFinalizationSettings& finalizationSettings,
        const MatchmakingSession &autoJoinSession) const
    {
        if ((teamToFill.mTeamSize >= teamToFill.mTeamCapacity) || bucket.mItems.empty())//Just in case check/warn
        {
            WARN_LOG("[MatchmakingSession].selectNextSessionForFinalization: WARN: detected internal algorithm did not skip a full team or empty bucket to try adding sessions with. Team or bucket selection parts of algorithm may need updates/corrections. Selected Team: " << teamToFill.toLogStr() << " Bucket: " << bucket.toLogStr() << ".");
            return nullptr;
        }

        // Rule based behavior:
        if (getCriteria().hasTeamUEDBalanceRuleEnabled())
        {
            return getCriteria().getTeamUEDBalanceRule()->selectNextSessionForFinalizationByTeamUEDBalance(*this,
                bucket, teamToFill, finalizationSettings, autoJoinSession);
        }


        // Default behavior: grab the first available (best fit-scored) session
        MatchedSessionsBucket::BucketItemVector::const_iterator iter = bucket.mItems.begin();
        return (advanceBucketIterToNextJoinableSessionForTeam(iter, bucket, teamToFill, autoJoinSession, *this)?
            iter->getMatchNode() : nullptr);
    }

    /*! ************************************************************************************************/
    /*! \brief update metrics for TeamCompositionRule based finalization success
    ***************************************************************************************************/
    void MatchmakingSession::calcFinalizationByTeamCompositionsSuccessMetrics(
        const CreateGameFinalizationSettings& finalizationSettings, uint16_t maxGameTeamCompositionsAttemptsRemaining) const
    {
        mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizeSuccessCompnsLeft.increment(maxGameTeamCompositionsAttemptsRemaining);

        mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizeSuccessByGroupOfX.increment(1, getPlayerCount());

        if (finalizationSettings.areAllFinalizingTeamCompositionsEqual())
            mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizeSuccessByGroupOfXCompnSame.increment(1, getPlayerCount());

        if (finalizationSettings.getFinalizingTeamCompositionsFitPercent() > 0.5f)
            mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizeSuccessByGroupOfXCompnOver50FitPct.increment(1, getPlayerCount());
    }

    /*! ************************************************************************************************/
    /*! \brief update metrics for TeamUEDBalanceRule based finalization success
    ***************************************************************************************************/
    void MatchmakingSession::calcFinalizationByTeamUedSuccessMetrics(const MatchmakingCriteria& criteria,
        const CreateGameFinalizationSettings& finalizationSettings, uint16_t maxFinalizationPickSequenceRetriesRemaining,
        UserExtendedDataValue teamSizeDiff, UserExtendedDataValue teamUedDiff) const
    {
        UserExtendedDataValue gameMembersUedDelta = 0, teamMembersUedDelta = 0;
        finalizationSettings.getMembersUedMaxDeltas(getCriteria(), gameMembersUedDelta, teamMembersUedDelta);
        mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizeSuccessGameMemberUedDelta.increment((uint64_t) gameMembersUedDelta);
        mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizeSuccessTeamMemberUedDelta.increment((uint64_t) teamMembersUedDelta);
        mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizeSuccessTeamUedDiff.increment((uint64_t) teamUedDiff);
        mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizeSuccessRePicksLeft.increment(maxFinalizationPickSequenceRetriesRemaining);
    }

    /*! ************************************************************************************************/
    /*! \brief update metrics for TeamUEDBalanceRule based finalization fail
    ***************************************************************************************************/
    void MatchmakingSession::calcFinalizationByTeamUedFailPickSeqMetrics(
        UserExtendedDataValue teamSizeDiff, UserExtendedDataValue teamUedDiff) const
    {
        mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizePickSequenceFail.increment();
        mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizePickSequenceFailTeamSizeDiff.increment((uint64_t) teamSizeDiff);
        mMatchmaker.getMatchmakerMetrics(this).mTeamsBasedMetrics.mTotalTeamFinalizePickSequenceFailTeamUedDiff.increment((uint64_t) teamUedDiff);
    }


    /*! ************************************************************************************************/
    /*! \brief Init the supplied createGameRequest for matchmaking createGame finalization.  The game's settings
            are drawn from the finalizing session (this) and the selected gameHost (if any).  Game attributes are
            voted upon.
    ***************************************************************************************************/
    bool MatchmakingSession::initCreateGameRequest(CreateGameFinalizationSettings& finalizationSettings) const
    {
        CreateGameRequest &createGameRequest = finalizationSettings.getCreateGameRequest().getCreateReq().getCreateRequest();
        const PlayerJoinData& hostSessionPlayerJoinData = finalizationSettings.mAutoJoinMemberInfo->mMMSession.getPlayerJoinData();
        const GameManager::UserSessionInfo &hostSessionInfo = finalizationSettings.mAutoJoinMemberInfo->mUserSessionInfo;
        const MatchmakingSessionList &memberSessionList = finalizationSettings.getMatchingSessionList();
        const CreateGameFinalizationTeamInfoVector *finalizedTeamIds = &(finalizationSettings.mCreateGameFinalizationTeamInfoVector);

        // setup the basic game info (from this session's settings)
        mGameCreationData.copyInto(createGameRequest.getGameCreationData());
        createGameRequest.getCommonGameData().setGameProtocolVersionString(getGameProtocolVersionString());

        auto& crossplayCriteria = mCachedMatchmakingRequestPtr->getRequest().getCriteriaData().getPlatformRuleCriteria();
        if (!crossplayCriteria.getClientPlatformListOverride().empty() || crossplayCriteria.getCrossplayMustMatch())
        {
            // This should always be set:
            crossplayCriteria.getClientPlatformListOverride().copyInto(createGameRequest.getClientPlatformListOverride());
        }
        else
        {
            ERR_LOG("[MatchmakingSession].InitCreateGameRequest - Trying to create a Game with no Platform list.  This should never happen, and may fail.")
        }

        // we may be trying to make a GameGroup!
        createGameRequest.getCommonGameData().setGameType(mCachedMatchmakingRequestPtr->getRequest().getCommonGameData().getGameType());

        if (isSingleGroupMatch())
        {
            createGameRequest.getGameCreationData().getGameSettings().setEnforceSingleGroupJoin();
        }

        // init the game's teams (if the team size rule is enabled)
        uint16_t publicParticipantSlotCapacity;
        if (mCriteria.isAnyTeamRuleEnabled())
        {
            createGameRequest.getGameCreationData().getTeamIds().clear();       // We clear the team ids, because will may be refactored below. 

            // note: the team size rule dominates the game size rule
            publicParticipantSlotCapacity = mCriteria.getParticipantCapacity();
            GameManager::TeamIndex defaultTeamIndex = finalizationSettings.getTeamIndexForMMSession(*this, hostSessionPlayerJoinData.getDefaultTeamId());
            createGameRequest.getPlayerJoinData().setDefaultTeamIndex(defaultTeamIndex);
            if (finalizedTeamIds == nullptr)
            {
                if (mCriteria.getTeamIds() != nullptr)
                {
                    mCriteria.getTeamIds()->copyInto(createGameRequest.getGameCreationData().getTeamIds());
                }
            }
            else
            {
                CreateGameFinalizationTeamInfoVector::const_iterator itCur = finalizedTeamIds->begin();
                CreateGameFinalizationTeamInfoVector::const_iterator itEnd = finalizedTeamIds->end();
                for (; itCur != itEnd; ++itCur)
                {
                    createGameRequest.getGameCreationData().getTeamIds().push_back((*itCur).mTeamId);
                }
            }
        }
        else
        {
            // teamSize rule is disabled, use totalPlayerSlots Rule to determine game size

            // need to put in a team, since none have been specified
            createGameRequest.getGameCreationData().getTeamIds().push_back(ANY_TEAM_ID);
            createGameRequest.getPlayerJoinData().setDefaultTeamIndex(0);

            // Max player capacity can be set in the request, the default in the TDF is 0, so if it has been changed
            // we use the value from the request, otherwise we maintain backwards compatibility by using
            // the value from game size rule
            if (mCriteria.isTotalSlotsRuleEnabled())
            {
                publicParticipantSlotCapacity = mCriteria.getTotalPlayerSlotsRule()->calcCreateGameParticipantCapacity();
            }
            else if (mGameCreationData.getMaxPlayerCapacity() != BLAZE_DEFAULT_MAX_PLAYER_CAP)
            {
                publicParticipantSlotCapacity = mGameCreationData.getMaxPlayerCapacity();
            }
            else
            {
                // a size-finalizing rule should always be enabled for create game
                publicParticipantSlotCapacity = (uint16_t)mMatchmaker.getMatchmakingConfig().getServerConfig().getGameSizeRange().getMax();
                ERR_LOG(LOG_PREFIX << " Matchmaking session(" << mMMSessionId << ") request was missing require game size rules for finalizing maximum game size. Falling back on the default global maximum value of " << publicParticipantSlotCapacity);
            }
        }
        uint16_t totalParticipantSlotCapacity = publicParticipantSlotCapacity + createGameRequest.getGameCreationData().getSlotCapacitiesMap()[SLOT_PRIVATE_PARTICIPANT];
        // make sure no roles have more capacity than public slot capacity
        uint16_t teamCount = (uint16_t)createGameRequest.getGameCreationData().getTeamIds().size();
        uint16_t teamCapacity = totalParticipantSlotCapacity / teamCount;
        uint16_t roleCount = createGameRequest.getGameCreationData().getRoleInformation().getRoleCriteriaMap().size();
        RoleCriteriaMap::iterator roleCriteriaIter = createGameRequest.getGameCreationData().getRoleInformation().getRoleCriteriaMap().begin();
        RoleCriteriaMap::iterator roleCriteriaEnd = createGameRequest.getGameCreationData().getRoleInformation().getRoleCriteriaMap().end();
        for (; roleCriteriaIter != roleCriteriaEnd; ++roleCriteriaIter)
        {
            if (roleCriteriaIter->second->getRoleCapacity() > teamCapacity)
            {
                roleCriteriaIter->second->setRoleCapacity(teamCapacity);
            }
            else if (roleCount == 1)
            {
                roleCriteriaIter->second->setRoleCapacity(teamCapacity);
            }
        }

        // If we want to support more than just SPECTATORs and PUBLIC slots in CG Matchmaking, this is where to add the code: 
        uint16_t spectatorSlots = createGameRequest.getGameCreationData().getSlotCapacitiesMap()[SLOT_PUBLIC_SPECTATOR] + createGameRequest.getGameCreationData().getSlotCapacitiesMap()[SLOT_PRIVATE_SPECTATOR];
        uint16_t totalSlots = (((mGameCreationData.getMaxPlayerCapacity() != BLAZE_DEFAULT_MAX_PLAYER_CAP) && mGameCreationData.getMaxPlayerCapacity() > (totalParticipantSlotCapacity +spectatorSlots)) ? mGameCreationData.getMaxPlayerCapacity() : totalParticipantSlotCapacity + spectatorSlots);

        createGameRequest.getGameCreationData().setMaxPlayerCapacity(totalSlots);
        createGameRequest.getSlotCapacities().at(SLOT_PUBLIC_PARTICIPANT) = publicParticipantSlotCapacity;
        createGameRequest.getSlotCapacities().at(SLOT_PRIVATE_PARTICIPANT) = createGameRequest.getGameCreationData().getSlotCapacitiesMap()[SLOT_PRIVATE_PARTICIPANT];
        createGameRequest.getSlotCapacities().at(SLOT_PUBLIC_SPECTATOR) = createGameRequest.getGameCreationData().getSlotCapacitiesMap()[SLOT_PUBLIC_SPECTATOR];
        createGameRequest.getSlotCapacities().at(SLOT_PRIVATE_SPECTATOR) = createGameRequest.getGameCreationData().getSlotCapacitiesMap()[SLOT_PRIVATE_SPECTATOR];

        //Set the group values for the create request.
        createGameRequest.getPlayerJoinData().setGroupId(UserGroupId());  // WARNING: PJD GroupId cannot be used for matchmade games.   The UserJoinInfo must be checked instead. 
        createGameRequest.getPlayerJoinData().setGameEntryType(hostSessionPlayerJoinData.getGameEntryType());

        // Even in the case of indirect matchmaking, we should still have a host session that is used when actually creating the game:
        // (We add a Host session here, since the new request may not already have one.)
        UserJoinInfoPtr hostUserJoinInfo = nullptr;
        addHostSessionInfo(finalizationSettings.getCreateGameRequest().getCreateReq().getUsersInfo(), &hostUserJoinInfo);
        if (hostUserJoinInfo != nullptr)
        {
            // Logic here should match how members are added below:
            getScenarioInfo().copyInto(hostUserJoinInfo->getScenarioInfoPerUser());
            hostSessionInfo.copyInto(hostUserJoinInfo->getUser());
            hostUserJoinInfo->setUserGroupId(finalizationSettings.mAutoJoinMemberInfo->mMMSession.getUserGroupId());
        }

        hostSessionInfo.getNetworkAddress().copyInto(createGameRequest.getCommonGameData().getPlayerNetworkAddress());

        // Hack for ILT:
        if (createGameRequest.getCommonGameData().getPlayerNetworkAddress().getActiveMember() == NetworkAddress::MEMBER_UNSET)
        {
            // Previously, we would always call pullBack() on the list, and then later we would just verify that the list was not empty (always true).
            // Now we need at least a garbage network address here to avoid an error on game create. (We now check that the address is not an unset union)
            NetworkAddress *hostAddress = &createGameRequest.getCommonGameData().getPlayerNetworkAddress();
            hostAddress->switchActiveMember(NetworkAddress::MEMBER_IPPAIRADDRESS);
        }

        // set the role info for host
        createGameRequest.getPlayerJoinData().setDefaultRole(hostSessionPlayerJoinData.getDefaultRole());
        copyPlayerJoinDataEntry(hostSessionInfo.getUserInfo(), hostSessionPlayerJoinData, createGameRequest.getPlayerJoinData());

        if (hostUserJoinInfo != nullptr)
        {
            PerPlayerJoinData* playerData = lookupPerPlayerJoinData(createGameRequest.getPlayerJoinData(), hostUserJoinInfo->getUser().getUserInfo());
            TeamId hostTeamId = (playerData->getTeamId() == INVALID_TEAM_ID) ? createGameRequest.getPlayerJoinData().getDefaultTeamId() : playerData->getTeamId();
            playerData->setTeamIndex(getCriteria().isAnyTeamRuleEnabled() ? finalizationSettings.getTeamIndexForMMSession(finalizationSettings.mAutoJoinMemberInfo->mMMSession, hostTeamId) : 0);
        }
        
        //Vote on the game values, taking each MM session's desires into account
        mCriteria.voteOnGameValues(finalizationSettings.getMatchingSessionList(), createGameRequest);

        if (mMatchmakerSlave.shouldPerformQosCheck(createGameRequest.getGameCreationData().getNetworkTopology()) && (hostSessionPlayerJoinData.getGameEntryType() == GAME_ENTRY_TYPE_DIRECT))
        {
            hostUserJoinInfo->setPerformQosValidation(true);
        }

        // reserve seats in the game for all members of all the other matching sessions (except our host session who was already added above via getHostSessionInfo())
        MatchmakingSessionList::const_iterator mmIter = memberSessionList.begin();
        MatchmakingSessionList::const_iterator mmEnd = memberSessionList.end();
        for ( ; mmIter != mmEnd; ++mmIter )
        {
            const MatchmakingSession *mmSession = *mmIter;

            // reserve a seat for every member of the session(except our host session who is in automatically)
            MemberInfoList::const_iterator groupMemberIter = mmSession->getMemberInfoList().begin();
            MemberInfoList::const_iterator groupMemberEnd = mmSession->getMemberInfoList().end();
            for ( ; groupMemberIter != groupMemberEnd; ++groupMemberIter )
            {
                const MemberInfo& memberInfo = static_cast<const MemberInfo &>(*groupMemberIter);
                BlazeId blazeId = memberInfo.mUserSessionInfo.getUserInfo().getId();
                if (blazeId != hostSessionInfo.getUserInfo().getId())
                {
                    UserJoinInfo* user = finalizationSettings.getCreateGameRequest().getCreateReq().getUsersInfo().pull_back();
                    user->setIsOptional(memberInfo.mIsOptionalPlayer);

                    if (!memberInfo.mIsOptionalPlayer)
                    {
                        // set the role info.
                        copyPlayerJoinDataEntry(memberInfo.mUserSessionInfo.getUserInfo(), mmSession->mPlayerJoinData, createGameRequest.getPlayerJoinData());

                        // add all non-optional players to the Qos validation list
                        if (mMatchmakerSlave.shouldPerformQosCheck(createGameRequest.getGameCreationData().getNetworkTopology()) && (mmSession->getGameEntryType() == GAME_ENTRY_TYPE_DIRECT))
                        {
                            user->setPerformQosValidation(true);
                        }
                    }
                    else
                    {
                        copyOptionalPlayerJoinData(memberInfo, mmSession->mPlayerJoinData, createGameRequest.getPlayerJoinData());
                    }

                    // Logic here should match how the host is added above:
                    mmSession->getScenarioInfo().copyInto(user->getScenarioInfoPerUser());
                    memberInfo.mUserSessionInfo.copyInto(user->getUser());
                    user->setUserGroupId(mmSession->getUserGroupId());

                    // at least for reservedExternalPlayers players (who don't follow-up claim), need to set team index now. May as well set for everyone here.
                    PerPlayerJoinData* playerData = lookupPerPlayerJoinData(createGameRequest.getPlayerJoinData(), memberInfo.mUserSessionInfo.getUserInfo());
                    if (playerData != nullptr)
                    {
                        TeamId playerTeamId = (playerData->getTeamId() == INVALID_TEAM_ID) ? mmSession->mPlayerJoinData.getDefaultTeamId() : playerData->getTeamId();
                        playerData->setTeamIndex(getCriteria().isAnyTeamRuleEnabled() ? finalizationSettings.getTeamIndexForMMSession(*mmSession, playerTeamId) : 0);
                    }
                    else
                        ERR_LOG("[MatchmakingSession].InitCreateGameRequest: req missing an expected player join data for user(" << memberInfo.mUserSessionInfo.getUserInfo().getId() << "), with platform info: " << memberInfo.mUserSessionInfo.getUserInfo().getPlatformInfo());
                }
            }
        }


        UserRoleCounts<PerPlayerJoinDataPtr> perPlayerJoinDataRoleCounts;
        sortPlayersByDesiredRoleCounts(createGameRequest.getPlayerJoinData().getPlayerDataList(), perPlayerJoinDataRoleCounts);
        TeamIndexRoleSizeMap roleCounts; // Temporary map to keep track of the current count for each role

        for (auto& ppJoinDataPair : perPlayerJoinDataRoleCounts)
        {
            RoleName newRole;

            BlazeRpcError err = findOpenRole(finalizationSettings.getCreateGameRequest().getCreateReq(), *ppJoinDataPair.first, newRole, roleCounts);
            if (err != ERR_OK)
            {
                eastl::string rolesBuf = "";
                bool firstRole = true;
                for (auto& desiredRoles : ppJoinDataPair.first->getRoles())
                {
                    if (!firstRole)
                    {
                        rolesBuf += ", ";
                    }

                    rolesBuf += desiredRoles;

                    firstRole = false;
                }

                if (rolesBuf.empty())
                {
                    rolesBuf = ppJoinDataPair.first->getRole();
                }

                WARN_LOG("[MatchmakingSession].initCreateGameRequest: Failed to find open role from set (" << rolesBuf << ") for player " << ppJoinDataPair.first->getUser().getBlazeId());
                return false;
            }
            ppJoinDataPair.first->setRole(newRole);
            ppJoinDataPair.first->getRoles().clear();
        }
        // catch-all check on the final request
        BlazeRpcError valErr = ValidationUtils::validatePlayerRolesForCreateGame(finalizationSettings.getCreateGameRequest().getCreateReq());
        if (ERR_OK != valErr)
        {
            ERR_LOG("[MatchmakingSession].initCreateGameRequest: Failed role criteria validation, err(" << ErrorHelp::getErrorName(valErr) << ").");
            return false;
        }

        createGameRequest.getGameCreationData().setExternalSessionTemplateName(mGameExternalSessionData.getExternalSessionIdentSetupMaster().getXone().getTemplateName());
        mGameExternalSessionData.copyInto(finalizationSettings.getCreateGameRequest().getCreateReq().getExternalSessionData());

        mExternalSessionCustomData.copyInto(createGameRequest.getGameCreationData().getExternalSessionCustomData());
        mExternalSessionStatus.copyInto(createGameRequest.getGameCreationData().getExternalSessionStatus());

        return true;
    }

    // Here we move all of the optional players back into the PlayerDataList
    void MatchmakingSession::copyOptionalPlayerJoinData(const MemberInfo& memberInfo,
        const PlayerJoinData& origData, PlayerJoinData& newJoinData) const
    {
        if (memberInfo.mIsOptionalPlayer)
        {
            const PerPlayerJoinData* origPlayerData = lookupPerPlayerJoinDataConst(origData, memberInfo.mUserSessionInfo.getUserInfo());
            if (origPlayerData)
            {
                PerPlayerJoinData* newPlayerData = lookupPerPlayerJoinData(newJoinData, memberInfo.mUserSessionInfo.getUserInfo());
                if (newPlayerData == nullptr)
                {
                    newPlayerData = newJoinData.getPlayerDataList().pull_back();
                }
                UserInfo::filloutUserIdentification(memberInfo.mUserSessionInfo.getUserInfo(), newPlayerData->getUser());
                origPlayerData->copyInto(*newPlayerData);
                newPlayerData->setIsOptionalPlayer(true);
            }
            else
            {
                // Unlike the copyPlayerJoinDataEntry calls, external/reserved members MUST have a role that can be looked up (if this fails, it means that they weren't part of the original request).
                eastl::string platformInfoStr;
                ERR_LOG("[MatchmakingSession] Failed to find role for player(platformInfo=(" << platformInfoToString(memberInfo.mUserSessionInfo.getUserInfo().getPlatformInfo(), platformInfoStr)
                    << "), blazeId=" << memberInfo.mUserSessionInfo.getUserInfo().getId() << "), not present in role roster.");
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief validate that the to-be-created game meets this session's requirements
    ***************************************************************************************************/
    bool MatchmakingSession::evaluateFinalCreateGameRequest(const Blaze::GameManager::MatchmakingCreateGameRequest &mmCreateGameRequest) const
    {
        DebugRuleResultMap ruleResults;
        bool useDebug = getDebugCheckOnly() || IS_LOGGING_ENABLED(Logging::TRACE);

        FitScore cgFitScore = mCriteria.evaluateCreateGameRequest(mmCreateGameRequest, ruleResults, useDebug);

        if (cgFitScore != FIT_SCORE_NO_MATCH)
        {
            return true;
        }
        else
        {
        // no match, bail on finalization
            TRACE_LOG(LOG_PREFIX << "evaluated CreateGame Request for finalizing session " << getMMSessionId() << " player " << getOwnerBlazeId() 
                << " (" << getOwnerPersonaName() << ") - NO_MATCH.");
            return false;
        }
    }


    void MatchmakingSession::fillUserSessionSetupReason(UserSessionSetupReasonMap &map,
        MatchmakingResult result, FitScore fitScore, FitScore maxFitScore) const
    {
        MemberInfoList::const_iterator itr = mMemberInfoList.begin();
        MemberInfoList::const_iterator end = mMemberInfoList.end();
        for (; itr != end; ++itr)
        {
            GameSetupReasonPtr setupReason = BLAZE_NEW GameSetupReason();
            const MemberInfo &info = static_cast<const MemberInfo &>(*itr);

            if (!PlayerInfo::getSessionExists(info.mUserSessionInfo))
            {
                return;
            }

            if (info.isMMSessionOwner())
            {
                setupReason->switchActiveMember(GameSetupReason::MEMBER_MATCHMAKINGSETUPCONTEXT);
                setupReason->getMatchmakingSetupContext()->setSessionId(getMMSessionId());
                setupReason->getMatchmakingSetupContext()->setScenarioId(getMMScenarioId());
                setupReason->getMatchmakingSetupContext()->setUserSessionId(getOwnerUserSessionId());
                setupReason->getMatchmakingSetupContext()->setMatchmakingResult(result);
                setupReason->getMatchmakingSetupContext()->setFitScore(fitScore);
                setupReason->getMatchmakingSetupContext()->setMaxPossibleFitScore(maxFitScore);
                setupReason->getMatchmakingSetupContext()->setGameEntryType(getGameEntryType());
                setupReason->getMatchmakingSetupContext()->setTimeToMatch(getMatchmakingRuntime());
                setupReason->getMatchmakingSetupContext()->setEstimatedTimeToMatch(getEstimatedTimeToMatch());
                setupReason->getMatchmakingSetupContext()->setMatchmakingTimeoutDuration(mTotalSessionDuration);
                setupReason->getMatchmakingSetupContext()->setTotalUsersOnline(getTotalGameplayUsersAtSessionStart());
                setupReason->getMatchmakingSetupContext()->setTotalUsersInGame(getTotalUsersInGameAtSessionStart());
                setupReason->getMatchmakingSetupContext()->setTotalUsersInMatchmaking(getTotalUsersInMatchmakingAtSessionStart());
                setupReason->getMatchmakingSetupContext()->setInitiatorId(getPrimaryUserBlazeId());
                setupReason->getMatchmakingSetupContext()->setTotalUsersMatched(getCurrentMatchingSessionListPlayerCount());
                setupReason->getMatchmakingSetupContext()->setTotalUsersPotentiallyMatched(getTotalMatchingSessionListPlayerCount());
            }
            else
            {
                setupReason->switchActiveMember(GameSetupReason::MEMBER_INDIRECTMATCHMAKINGSETUPCONTEXT);
                setupReason->getIndirectMatchmakingSetupContext()->setSessionId(getMMSessionId());
                setupReason->getIndirectMatchmakingSetupContext()->setScenarioId(getMMScenarioId());
                setupReason->getIndirectMatchmakingSetupContext()->setUserSessionId(getOwnerUserSessionId());
                setupReason->getIndirectMatchmakingSetupContext()->setMatchmakingResult(result);
                setupReason->getIndirectMatchmakingSetupContext()->setUserGroupId(mPlayerJoinData.getGroupId());
                setupReason->getIndirectMatchmakingSetupContext()->setFitScore(fitScore);
                setupReason->getIndirectMatchmakingSetupContext()->setMaxPossibleFitScore(maxFitScore);
                setupReason->getIndirectMatchmakingSetupContext()->setRequiresClientVersionCheck(mMatchmakerSlave.getEvaluateGameProtocolVersionString());
                setupReason->getIndirectMatchmakingSetupContext()->setGameEntryType(getGameEntryType());
                setupReason->getIndirectMatchmakingSetupContext()->setTimeToMatch(getMatchmakingRuntime());
                setupReason->getIndirectMatchmakingSetupContext()->setEstimatedTimeToMatch(getEstimatedTimeToMatch());
                setupReason->getIndirectMatchmakingSetupContext()->setMatchmakingTimeoutDuration(mTotalSessionDuration);
                setupReason->getIndirectMatchmakingSetupContext()->setTotalUsersOnline(getTotalGameplayUsersAtSessionStart());
                setupReason->getIndirectMatchmakingSetupContext()->setTotalUsersInGame(getTotalUsersInGameAtSessionStart());
                setupReason->getIndirectMatchmakingSetupContext()->setTotalUsersInMatchmaking(getTotalUsersInMatchmakingAtSessionStart());
                setupReason->getIndirectMatchmakingSetupContext()->setTotalUsersMatched(getCurrentMatchingSessionListPlayerCount());
                setupReason->getIndirectMatchmakingSetupContext()->setTotalUsersPotentiallyMatched(getTotalMatchingSessionListPlayerCount());
                setupReason->getIndirectMatchmakingSetupContext()->setInitiatorId(getPrimaryUserBlazeId());
            }
            map[info.mUserSessionInfo.getSessionId()] = setupReason;
        }
    }

    eastl::string MatchmakingSession::getPlatformForMetrics() const
    {
        eastl::string platformStr = "";
        bool firstEntry = true;

        const auto& clientPlatformList = mCachedMatchmakingRequestPtr->getRequest().getCriteriaData().getPlatformRuleCriteria().getClientPlatformListOverride();

        // iterate over possible platforms in enum order to guarantee consistency in the metrics
        for (auto& platformValue : GetClientPlatformTypeEnumMap().getNamesByValue())
        {
            const EA::TDF::TdfEnumInfo& info = (const EA::TDF::TdfEnumInfo&)(platformValue);
            if (clientPlatformList.findAsSet((ClientPlatformType)info.mValue) != clientPlatformList.end())
            {
                if (!firstEntry)
                {
                    platformStr += ",";
                }

                platformStr += info.mName;
                firstEntry = false;
            }
        }

        if (platformStr.empty())
        {
            platformStr += ClientPlatformTypeToString(INVALID);
        }


        return platformStr;
    }

    uint16_t MatchmakingSession::getTeamSize(TeamId teamId) const
    {
        // Do things the slow manual way, just in case something wasn't enabled: 
        const TeamIdSizeList* teamIdSizeList = mCriteria.getTeamIdSizeListFromRule();
        if (teamIdSizeList == nullptr)
        {
            TRACE_LOG(LOG_PREFIX << " Session(" << mMMSessionId << ") - Missing Team Size information.");
            return 0;
        }

        TeamIdSizeList::const_iterator curTeamId = teamIdSizeList->begin();
        TeamIdSizeList::const_iterator endTeamId = teamIdSizeList->end();
        for (; curTeamId != endTeamId; ++curTeamId)
        {
            if (curTeamId->first == teamId)
                return curTeamId->second;
        }

        return 0;
    }

    void MatchmakingSession::prepareForFinalization(MatchmakingSessionId finalizingSessionId)
    {
        // make sure the session doesn't try to finalize again
        // we no longer remove the session from mCreateGameSessionMap
        if (!isDebugCheckOnly() || getMMScenarioId() == INVALID_SCENARIO_ID)
        {
            // Skip the lockdown for scenario subsessions pseudo requests
            setLockdown();
        }
         
        // update the matched session counts for PIN
        updateMatchingSessionListPlayerCounts();

        // we do this for all removed sessions, but it only gets utilized if we're doing CG finalization
        setFinalizingMatchmakingSessionId(finalizingSessionId);
    }


    /*! ************************************************************************************************/
    /*!
        \brief broadcast and log a NotifyMatchmakingFinished msg indicating as SESSION_TIMEOUT.
            Sessions should be deleted after they are finalized.

    *************************************************************************************************/
    void MatchmakingSession::finalizeAsTimeout()
    {
        TRACE_LOG(LOG_PREFIX << " matchmaking session " << getMMSessionId() << " expired; finalizing (SESSION_TIMED_OUT)");
        updateMatchingSessionListPlayerCounts();
        submitMatchmakingFailedEvent(SESSION_TIMED_OUT);
        sendNotifyMatchmakingFailed(SESSION_TIMED_OUT); // fitScore: 0, gameId: 0
    }

    /*! ************************************************************************************************/
    /*!
        \brief broadcast and log a NotifyMatchmakingFinished msg indicating as SESSION_TERMINATED.
            Sessions should be deleted after they are finalized.

    *************************************************************************************************/
    void MatchmakingSession::finalizeAsTerminated()
    {
        TRACE_LOG(LOG_PREFIX << " matchmaking session " << getMMSessionId() << " expired; finalizing (SESSION_TERMINATED)");
        updateMatchingSessionListPlayerCounts();
        submitMatchmakingFailedEvent(SESSION_TERMINATED);
        sendNotifyMatchmakingFailed(SESSION_TERMINATED); // fitScore: 0, gameId: 0
    }

    /*! ************************************************************************************************/
    /*!
        \brief broadcast and log a NotifyMatchmakingFinished msg indicating as SESSION_CANCELED.
            Sessions should be deleted after they are finalized.

    *************************************************************************************************/
    void MatchmakingSession::finalizeAsCanceled()
    {
        TRACE_LOG(LOG_PREFIX << " matchmaking session " << getMMSessionId() << " canceled; finalized (SESSION_CANCELED)");
        updateMatchingSessionListPlayerCounts();
        submitMatchmakingFailedEvent(SESSION_CANCELED);
        sendNotifyMatchmakingFailed(SESSION_CANCELED); // fitScore: 0, gameId: 0
    }


    void MatchmakingSession::finalizeAsGameSetupFailed()
    {
        TRACE_LOG(LOG_PREFIX << " matchmaking session " << getMMSessionId() << " failed to setup game (no dedicaed servers); finalized (SESSION_ERROR_GAME_SETUP_FAILED)");
        // no update of the player counts since we'll have fixed those values as part of finalization
        submitMatchmakingFailedEvent(SESSION_ERROR_GAME_SETUP_FAILED);
        sendNotifyMatchmakingFailed(SESSION_ERROR_GAME_SETUP_FAILED); // fitScore: 0, gameId: 0
    }

    /*! ************************************************************************************************/
    /*!
        \brief broadcast and log a NotifyMatchmakingFinished msg indicating as SESSION_QOS_VALIDATION_FAILED.
            Sessions should be deleted after they are finalized.

    *************************************************************************************************/
    void MatchmakingSession::finalizeAsFailedQosValidation()
    {
        TRACE_LOG(LOG_PREFIX << " matchmaking session " << mMMSessionId << " failed QoS validation; finalized (SESSION_QOS_VALIDATION_FAILED)");
        // no update of the player counts since we'll have fixed those values as part of finalization
        submitMatchmakingFailedEvent(SESSION_QOS_VALIDATION_FAILED);
        sendNotifyMatchmakingFailed(SESSION_QOS_VALIDATION_FAILED); // fitScore: 0, gameId: 0
    }

    /*! ************************************************************************************************/
    /*!
        \brief broadcast and log a NotifyMatchmakingDebugged msg indicating as SESSION_DEBUGGED.
            Sessions should be deleted after they are finalized.
    *************************************************************************************************/
    void MatchmakingSession::finalizeAsDebugged(MatchmakingResult result) const
    {
        TRACE_LOG(LOG_PREFIX << " matchmaking session " << getMMSessionId() << " debugged; finalized " << MatchmakingResultToString(result));
        sendNotifyMatchmakingPseudoSuccess(result);
    }

    void MatchmakingSession::submitMatchmakingFailedEvent(MatchmakingResult matchmakingResult) const
    {
        if (getMMScenarioId() != INVALID_SCENARIO_ID)
        {
            // For scenario matchmaking, only the last sub-session should process the event.
            // if the scenario is locked AND this subsession OWNS the lock when it's the last session, it's ok to proceed with events.
            if (!mMatchmaker.isOnlyScenarioSubsession(this) || (isLockedDown() && !isLockedDown(true)))
                return;
        }
        else
        {
            ERR_LOG("[MatchmakingSession::submitMatchmakingFailedEvent] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
        }

        // submit matchmaking succeeded event, this will catch the game creator and all joining sessions.
        FailedMatchmakingSession failedMatchmakingSession;
        failedMatchmakingSession.setMatchmakingSessionId(getMMSessionId());
        failedMatchmakingSession.setMatchmakingScenarioId(getMMScenarioId());
        failedMatchmakingSession.setUserSessionId(getOwnerUserSessionId());
        failedMatchmakingSession.setMatchmakingResult(MatchmakingResultToString(matchmakingResult));

        gEventManager->submitEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_FAILEDMATCHMAKINGSESSIONEVENT), failedMatchmakingSession);

        MatchmakingSession::MemberInfoList::const_iterator memberIter = mMemberInfoList.begin();
        MatchmakingSession::MemberInfoList::const_iterator memberEnd = mMemberInfoList.end();
        for (; memberIter != memberEnd; ++memberIter)
        {
            PINSubmissionPtr pinSubmission = BLAZE_NEW PINSubmission;
            const MatchmakingSession::MemberInfo &memberInfo =
                static_cast<const MatchmakingSession::MemberInfo &>(*memberIter);
            PINEventHelper::buildMatchJoinForFailedMatchmakingSession(*this, matchmakingResult, memberInfo.mUserSessionInfo, *pinSubmission);
            gUserSessionManager->sendPINEvents(pinSubmission);
        }
    }

    void MatchmakingSession::setLockdown()          { mMatchmaker.setSessionLockdown(this); }
    void MatchmakingSession::clearLockdown()        { mMatchmaker.clearSessionLockdown(this); }
    bool MatchmakingSession::isLockedDown(bool ignoreOtherSessionLock) const   { return mMatchmaker.isSessionLockedDown(this, ignoreOtherSessionLock); }


    /*! ************************************************************************************************/
    /*! \brief send a matchmaking finished notification tdf with the result of this session

        \param[in] matchmakingResult - the matchmaking matchmakingResult.
        \param[in] totalFitScore - the totalFitScore for the matchmakingResult
        \param[in] gameId - the gameId to init the notifyFinished response to (only valid for successful create/join results)
    *************************************************************************************************/
    void MatchmakingSession::sendNotifyMatchmakingFailed(MatchmakingResult matchmakingResult)
    {
        // init the notify matchmaking finished tdf
        NotifyMatchmakingFailed notifyFailed;
        notifyFailed.setUserSessionId(getOwnerUserSessionId());
        notifyFailed.setSessionId(mMMSessionId);
        notifyFailed.setScenarioId(mOwningScenarioId);
        notifyFailed.setMatchmakingResult(matchmakingResult);
        notifyFailed.setMaxPossibleFitScore( mCriteria.calcMaxPossibleFitScore() );
        notifyFailed.getSessionDuration().setSeconds(mSessionAgeSeconds);

        // Update our MM QoS KPI metrics if we failed due to QoS validation failing
        if (matchmakingResult == SESSION_QOS_VALIDATION_FAILED)
        {
            TRACE_LOG("[MatchmakingSession].sendNotifyMatchmakingFailed: Updating MM QoS KPI metrics due to QoS validation failing.");
            updateMMQoSKPIMetrics();
        }

        // send the notification
        mMatchmakerSlave.sendNotifyMatchmakingFailedToSliver(UserSession::makeSliverIdentity(getOwnerUserSessionId()), &notifyFailed);
    }

    void MatchmakingSession::buildNotifyMatchmakingPseudoSuccess(NotifyMatchmakingPseudoSuccess& notification, MatchmakingResult result) const
    {
        notification.setSessionId(getMMSessionId());
        notification.setScenarioId(getMMScenarioId());
        notification.setUserSessionId(getOwnerUserSessionId());
        notification.setMatchmakingResult(result);
        notification.setSubSessionName(getScenarioInfo().getSubSessionName());

        // Match game top results are cached on the mm session itself.
        DebugTopResultMap::const_iterator iter = mDebugTopResultMap.begin();
        DebugTopResultMap::const_iterator end = mDebugTopResultMap.end();
        GameId bestGameId = INVALID_GAME_ID;
        FitScore bestGameFitScore = FIT_SCORE_NO_MATCH;
        FitScore bestGameMaxFitScore = FIT_SCORE_NO_MATCH;
        TimeValue bestTimeToMatch;
        for (; iter != end; ++iter)
        {
            // Best game id is our top MATCH'ing game.
            if ( (bestGameId == INVALID_GAME_ID) && (iter->second.getOverallResult() == MATCH) )
            {
                bestGameId = iter->second.getGameId();
                bestGameFitScore = iter->second.getOverallFitScore();
                bestGameMaxFitScore = iter->second.getMaxFitScore();
                bestTimeToMatch = iter->second.getTimeToMatch();
            }
            iter->second.copyInto(*(notification.getFindGameResults().getTopResultList().pull_back()));
        }

        if(result == SUCCESS_PSEUDO_FIND_GAME)
        {
            // Blocking section.
            // Debug only sessions don't actually join a game, assume we would have joined the best match.
            BlazeRpcError componentResult = Blaze::ERR_OK;
            Component* searchComponent = gController->getComponent(Blaze::Search::SearchSlave::COMPONENT_ID, false, true, &componentResult);
            if ( (componentResult == ERR_OK) && (searchComponent != nullptr) && (bestGameId != INVALID_GAME_ID))
            {
                // find our search slave associated with our best game.
                Search::SearchShardingBroker::SlaveInstanceIdList searchInstances;
                mMatchmaker.getFullCoverageSearchSet(searchInstances);
                InstanceId bestSearchSlaveInstanceId = *searchInstances.begin();

                // Get full game data from that search slave.
                RpcCallOptions opts;
                opts.routeTo.setInstanceId(bestSearchSlaveInstanceId);
                Search::GetGamesRequest request;
                request.getGameIds().push_back(bestGameId);
                request.setResponseType(Search::GET_GAMES_RESPONSE_TYPE_FULL);
                Search::GetGamesResponse response;
                BlazeRpcError gamesErr = searchComponent->sendRequest(Search::SearchSlave::CMD_INFO_GETGAMES, &request, &response, nullptr, opts);
                if (gamesErr == ERR_OK)
                {
                    if(!response.getFullGameData().empty())
                    {
                        const ListGameData& bestGameData = **(response.getFullGameData().begin());
                        bestGameData.copyInto(notification.getFindGameResults().getFoundGame());
                        // add the size of this MM session to the player count
                        notification.getFindGameResults().setJoinedPlayerCount(getPlayerCount());

                        // always clear the persisted game id secret.
                        notification.getFindGameResults().getFoundGame().getGame().getPersistedGameIdSecret().setData(nullptr, 0);
                        notification.getFindGameResults().setFoundGameFitScore(bestGameFitScore);
                        notification.getFindGameResults().setMaxFitScore(bestGameMaxFitScore);
                        notification.getFindGameResults().setFoundGameTimeToMatch(bestTimeToMatch);
                    }
                }
                else
                {
                    WARN_LOG(LOG_PREFIX << " session " << getMMSessionId() << " failed to get full data for game " << bestGameId << " error: " << ErrorHelp::getErrorName(gamesErr));
                }
            }
        }
        if (result == SUCCESS_PSEUDO_CREATE_GAME)
        {
            mDebugCreateGameResults.copyInto(notification.getCreateGameResults());
            // Finalization Rule results can have entries from any rule that participates in finalization, but we add an extra item for sessions that finalize alone.
            if (notification.getCreateGameResults().getSessionResultList().empty())
            {
                DebugRuleResult *finalizationResult = notification.getCreateGameResults().getFinalizationRuleResults().allocate_element();
                
                finalizationResult->setFitScore(0);
                finalizationResult->setMaxFitScore(0);
                finalizationResult->setResult(MATCH);
                finalizationResult->getConditions().push_back("Matchmaking rules decayed to allow session to finalize alone.");
                notification.getCreateGameResults().getFinalizationRuleResults().insert(eastl::make_pair(FINALIZATION_CONDITIONS_NAME, finalizationResult));
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief send debugged notification
            NOTE: function blocks
    *************************************************************************************************/
    void MatchmakingSession::sendNotifyMatchmakingPseudoSuccess(MatchmakingResult result) const
    {
        NotifyMatchmakingPseudoSuccess notification;
        buildNotifyMatchmakingPseudoSuccess(notification, result);

        mMatchmakerSlave.sendNotifyMatchmakingPseudoSuccessToSliver(UserSession::makeSliverIdentity(getOwnerUserSessionId()), &notification);
    }


    void MatchmakingSession::sendNotifyMatchmakingStatus()
    {
        // send the notification
        NotifyMatchmakingAsyncStatus notifyMatchmakingStatus;

        notifyMatchmakingStatus.setInstanceId(gController->getInstanceId());

        uint32_t numOfSessions = mMatchmaker.getNumOfCreateGameMatchmakingSessions();

        if (isCreateGameEnabled())
        {
            mMatchmakingAsyncStatus.getCreateGameStatus().setNumOfMatchmakingSession(numOfSessions);
        }
        if (isFindGameEnabled())
        {
            mMatchmakingAsyncStatus.getFindGameStatus().setNumOfGames(mMatchmakingAsyncStatus.getFindGameStatus().getNumOfGames());
        }
        // AUDIT: since the removal of create game sub sessions, this list will only ever contain 1 value.
        // if someday we should break backward compatibility, lets change this list to a singular value.
        notifyMatchmakingStatus.getMatchmakingAsyncStatusList().push_back(&mMatchmakingAsyncStatus);


        TRACE_LOG(LOG_PREFIX << " sending NotifyMatchmakingAsyncStatus for Session " << mMMSessionId << " at age " << mSessionAgeSeconds);
        notifyMatchmakingStatus.setMatchmakingSessionId(mMMSessionId);
        notifyMatchmakingStatus.setMatchmakingScenarioId(mOwningScenarioId);
        notifyMatchmakingStatus.setUserSessionId(getOwnerUserSessionId());
        notifyMatchmakingStatus.setMatchmakingSessionAge(getSessionAgeSeconds());
        mMatchmakerSlave.sendNotifyMatchmakingAsyncStatusToSliver(UserSession::makeSliverIdentity(getOwnerUserSessionId()), &notifyMatchmakingStatus);

        // Important, since tdf will try to free the memory after it send the notification and we don't
        // want it to do so here because we use a local member, so we have to clear the list so
        // the tdf won't try to free the object
        notifyMatchmakingStatus.getMatchmakingAsyncStatusList().clear();

        mLastStatusAsyncTimestamp = TimeValue::getTimeOfDay();
    }

    // sends the final async status as needed before finalization completes
    void MatchmakingSession::sendFinalMatchmakingStatus()
    {
        if ((!mIsFinalStatusAsyncDirty && !isTimeForAsyncNotifiation(TimeValue::getTimeOfDay(), (uint32_t)mMatchmaker.getMatchmakingConfig().getServerConfig().getStatusNotificationPeriod().getMillis())))
        {
            // to avoid edge case spam on clients on repeated finalization failures we only
            // send async notification when session async time is due or session criteria is changed
            return;
        }

        NotifyMatchmakingAsyncStatus notifyMatchmakingStatus;

        notifyMatchmakingStatus.setInstanceId(gController->getInstanceId());

        uint32_t numOfSessions = mMatchmaker.getNumOfCreateGameMatchmakingSessions();

        if (isCreateGameEnabled())
        {
            mMatchmakingAsyncStatus.getCreateGameStatus().setNumOfMatchmakingSession(numOfSessions);
        }
        notifyMatchmakingStatus.getMatchmakingAsyncStatusList().push_back(&mMatchmakingAsyncStatus);
        notifyMatchmakingStatus.setMatchmakingSessionId(mMMSessionId);
        notifyMatchmakingStatus.setMatchmakingScenarioId(mOwningScenarioId);
        notifyMatchmakingStatus.setUserSessionId(getOwnerUserSessionId());
        TRACE_LOG(LOG_PREFIX << " sending final NotifyMatchmakingAsyncStatus for Session " << mMMSessionId << " at age " << mSessionAgeSeconds);
        mMatchmakerSlave.sendNotifyMatchmakingAsyncStatusToSliver(UserSession::makeSliverIdentity(getOwnerUserSessionId()), &notifyMatchmakingStatus);

        // Important, since tdf will try to free the memory after it send the notification and we don't
        // want it to do so here because we use a local member, so we have to clear the list so
        // the tdf won't try to free the object
        notifyMatchmakingStatus.getMatchmakingAsyncStatusList().clear();

        mIsFinalStatusAsyncDirty = false;
    }

    /*! ************************************************************************************************/
    /*!
        \brief update the session's age given the current time.  NOTE: this will also change the session's
        dirty flag if any of the session's rule thresholds changed due to the new session age.

        \param[in] now the current time
        \return true if the session is dirty, otherwise, false
    ***************************************************************************************************/
    void MatchmakingSession::updateSessionAge(const TimeValue &now)
    {
        // Check if we haven't started yet (due to startDelay)
        if (now < mSessionStartTime)
        {
            return;
        }

        mSessionAgeSeconds = static_cast<uint32_t>((now - mSessionStartTime).getSec()) + mStartingDecayAge;
    }



    bool MatchmakingSession::updateCreateGameSession()
    {

        // always update our pending list.  The relative match time could be after this sessions last decay, meaning
        // it would not be dirty when the time comes to evaluate.  This can happen when this session is older
        // than the matching session.
        bool newMatches = updatePendingMatches();

        if ( (mSessionAgeSeconds < mNextElapsedSecondsUpdate) && !isNewSession() )
        {
            TRACE_LOG(LOG_PREFIX << " updateSessionAge for Session " << mMMSessionId << " at age " << mSessionAgeSeconds << " will not update until age " << mNextElapsedSecondsUpdate << ", is not new, and " << (newMatches?"has":"no") << " new matches, and will timeout at " << mSessionTimeoutSeconds << ".");
            return newMatches;
        }

        UpdateThresholdResult updateResult = NO_RULE_CHANGE;

        if (!mDebugFreezeDecay)
        {
            // Update the cached criteria values that are time sensitive,
            // and see if the time update has made this session dirty.
            // When the session is in new state, we need to make sure it's dirty so that we
            // can make sure the session is added to dirtysessionlist during evaluation.
            updateResult = updateSessionThresholds();

            if (mNextElapsedSecondsUpdate == UINT32_MAX)
            {
                mNextElapsedSecondsUpdate = mSessionTimeoutSeconds;
            }
        }

        if (updateResult != NO_RULE_CHANGE)
        {
            mIsFinalStatusAsyncDirty = true;
        }

        TRACE_LOG(LOG_PREFIX << " updated thresholds for Session " << mMMSessionId << " at age " << mSessionAgeSeconds << ", result was " << updateResult << ", " << (newMatches?"has":"no") << " new matches, and will timeout at " << mSessionTimeoutSeconds << ".");
        if ( (updateResult != NO_RULE_CHANGE) || isNewSession())
        {
            if (isNewSession())
            {
                clearNewSession();
            }

            return true;
        }
        else
        {
            return newMatches;
        }
    }

    //Update the inner criteria and elapsed/next update second members
    UpdateThresholdResult MatchmakingSession::updateSessionThresholds(bool forceUpdate)
    {
        UpdateThresholdStatus result = mCriteria.updateCachedThresholds(mSessionAgeSeconds, forceUpdate);

        mNextElapsedSecondsUpdate = mCriteria.getNextElapsedSecondsUpdate();

        // Keeps old functionality on master of just a single result.
        return ( result.reteRulesResult > result.nonReteRulesResult) ? result.reteRulesResult : result.nonReteRulesResult;
    }


    // add a matching session to this session's match list (either current or pending), also sets up a "back link" in the other session (other's from list)
    void MatchmakingSession::addMatchingSession(MatchmakingSession *otherSession, const MatchInfo &matchInfo, DebugRuleResultMap& debugResults)
    {
        MatchmakingSessionMatchNode *matchNode = mMatchmaker.allocateMatchNode();
        matchNode->sMatchedToSession = otherSession;
        matchNode->sFitScore = matchInfo.sFitScore;
        matchNode->sMatchTimeSeconds = matchInfo.sMatchTimeSeconds;
        debugResults.copyInto(*(matchNode->sRuleResultMap));
        matchNode->sMatchedFromSession = this;
        matchNode->sOtherSessionSize = otherSession->getPlayerCount();
        matchNode->sOtherHasMultiRoleMembers = otherSession->getCriteria().getRolesRule()->hasMultiRoleMembers();

        if (matchInfo.sMatchTimeSeconds <= mSessionAgeSeconds)
        {
            mMatchedToList.push_back(*matchNode);

            MatchedSessionsBucket& bucket = mMatchedToBuckets[matchNode->sOtherSessionSize];
            bucket.addMatchToBucket(matchNode, matchNode->sOtherSessionSize);
        }
        else
        {
            mPendingMatchedToListMap[matchNode->sMatchTimeSeconds].push_back(*matchNode);
        }

        // "back link" from other to us
        //  Note: no explicit pointer back, other session has a reference to the matchNode (it can be removed from both lists)
        otherSession->mMatchedFromList.push_back(*matchNode);

        TRACE_LOG(LOG_PREFIX << " Session " << getMMSessionId() << " player " 
            << getOwnerBlazeId() << " (" << getOwnerPersonaName() 
            << ") adding matching session " << otherSession->getMMSessionId() << " player " << otherSession->getOwnerBlazeId() << " (" 
            << otherSession->getOwnerPersonaName() << ") - MATCH (totalFitScore: " << matchNode->sFitScore << " at time " << matchNode->sMatchTimeSeconds << ")");
    }

    void MatchmakingSession::sessionCleanup()
    {
        TRACE_LOG(LOG_PREFIX << " MatchmakingSession::sessionCleanup() for Session " << mMMSessionId
                  << " to unlink it from any matched/matching sessions.");
        clearMatchingSessionList();
        clearMatchedSessionList();
    }

    void MatchmakingSession::clearMatchingSessionList()
    {
        //Remove nodes from sessions matching me
        while (!mMatchedFromList.empty())
        {
            MatchmakingSessionMatchNode &node = (MatchmakingSessionMatchNode &) mMatchedFromList.front();
            mMatchedFromList.pop_front();

            // if we are a session potentially pulling others in, cache off the fit score, on their sessions
            if (isLockedDown() && 
               (getMMSessionId() == mFinalizingMatchmakingSession))
            {
                // we never need to clear this later on, because it will get overwritten next time we finalize
                node.sMatchedFromSession->setCreateGameFinalizationFitScore(node.sFitScore);

                if (node.sMatchedFromSession->getDebugCheckOnly())
                {
                    // cache off the details of the evaluation for the pulled-in session
                    // again, this won't need to be cleared, because a subsequent finalization attempt will overwrite the data
                    node.sRuleResultMap->copyInto(node.sMatchedFromSession->mFinalizingSessionDebugResult.getRuleResults());
                    node.sMatchedFromSession->mFinalizingSessionDebugResult.setTimeToMatch(TimeValue::getTimeOfDay());
                    node.sMatchedFromSession->mFinalizingSessionDebugResult.setOverallFitScore(node.sFitScore);
                    node.sMatchedFromSession->mFinalizingSessionDebugResult.setMatchedSessionId(node.sMatchedToSession->getMMSessionId());
                    node.sMatchedFromSession->mFinalizingSessionDebugResult.setOwnerPlayerName(node.sMatchedToSession->getOwnerPersonaName());
                    node.sMatchedFromSession->mFinalizingSessionDebugResult.setOwnerBlazeId(node.sMatchedToSession->getOwnerBlazeId());
                    node.sMatchedFromSession->mFinalizingSessionDebugResult.setPlayerCount(node.sMatchedToSession->getPlayerCount());
                }
            }

            MatchmakingSession* fromSession = node.sMatchedFromSession;
            if (fromSession != nullptr)
            {
                MatchedSessionsBucket& bucket = fromSession->mMatchedToBuckets[node.sOtherSessionSize];
                bucket.removeMatchFromBucket(*this, node.sOtherSessionSize);
            }

            MatchedToList::remove(node);
            mMatchmaker.deallocateMatchNode(node);
        }
    }

    void MatchmakingSession::clearMatchedSessionList()
    {
        TRACE_LOG(LOG_PREFIX << " MatchmakingSession::clearMatchedSessionList called for session " << mMMSessionId);

        //Clear buckets for rules
        mMatchedToBuckets.clear();

        //Remove nodes from sessions I'm matching
        while (!mMatchedToList.empty())
        {
            MatchmakingSessionMatchNode &node = (MatchmakingSessionMatchNode &) mMatchedToList.front();
            mMatchedToList.pop_front();

            // if we have the session that pulled us in, cache off the fit score
            if (isLockedDown() && 
                (node.sMatchedToSession->getMMSessionId() == mFinalizingMatchmakingSession))
            {
                // we never need to clear this later on, because it will get overwritten next time we finalize
                setCreateGameFinalizationFitScore(node.sFitScore);

                if (getDebugCheckOnly())
                {
                    // cache off the details of the evaluation for the pulled-in session
                    // again, this won't need to be cleared, because a subsequent finalization attempt will overwrite the data
                    node.sRuleResultMap->copyInto(mFinalizingSessionDebugResult.getRuleResults());
                    mFinalizingSessionDebugResult.setTimeToMatch(TimeValue::getTimeOfDay());
                    mFinalizingSessionDebugResult.setOverallFitScore(node.sFitScore);
                    mFinalizingSessionDebugResult.setMatchedSessionId(node.sMatchedToSession->getMMSessionId());
                    mFinalizingSessionDebugResult.setOwnerPlayerName(node.sMatchedToSession->getOwnerPersonaName());
                    mFinalizingSessionDebugResult.setOwnerBlazeId(node.sMatchedToSession->getOwnerBlazeId());
                    mFinalizingSessionDebugResult.setPlayerCount(node.sMatchedToSession->getPlayerCount());
                }
      
            }

            MatchedFromList::remove(node);
            mMatchmaker.deallocateMatchNode(node);
        }

        // Remove nodes from sessions I will match (in the future)
        PendingMatchListMap::iterator mapIter = mPendingMatchedToListMap.begin();
        PendingMatchListMap::iterator mapEnd = mPendingMatchedToListMap.end();
        for ( ; mapIter != mapEnd; ++mapIter )
        {

            MatchedToList &pendingMatchList = mapIter->second;
            while (!pendingMatchList.empty())
            {
                MatchmakingSessionMatchNode &node = (MatchmakingSessionMatchNode &) pendingMatchList.front();
                pendingMatchList.pop_front();
                MatchedFromList::remove(node);
                mMatchmaker.deallocateMatchNode(node);
            }
        }
    }

    void MatchmakingSession::updateMatchingSessionListPlayerCounts()
    {
        mCurrentMatchmakingSessionListPlayerCount = 0;
        for (const auto& matchingSessionIter : mMatchedToList)
        {
            mCurrentMatchmakingSessionListPlayerCount += matchingSessionIter.sOtherSessionSize;
        }

        mTotalMatchmakingSessionListPlayerCount = 0;
        for (const auto& pendingMatchBucket : mPendingMatchedToListMap)
        {
            for (const auto& matchingSessionIter : pendingMatchBucket.second)
            {
                mTotalMatchmakingSessionListPlayerCount += matchingSessionIter.sOtherSessionSize;
            }
        }

        // the pending buckets don't have the current matching sessions
        mTotalMatchmakingSessionListPlayerCount += mCurrentMatchmakingSessionListPlayerCount;
    }


 

    bool MatchmakingSession::setUpMatchmakingSessionEntryCriteria( MatchmakingCriteriaError &err, const StartMatchmakingInternalRequest &request )
    {
        // validate that game entry criteria are parse-able
        // Note: we'll validate these again when/if we create the game, but it's better to fail the MM session up front
        EntryCriteriaName failedCriteriaName;
        if (!EntryCriteriaEvaluator::validateEntryCriteria(mGameCreationData.getEntryCriteriaMap(), failedCriteriaName))
        {
            char8_t errMsg[512];
            blaze_snzprintf(errMsg, sizeof(errMsg), "Error: unable to parse gameEntryCriteria named \"%s\"; expression=\"%s\".",
                failedCriteriaName.c_str(), mGameCreationData.getEntryCriteriaMap()[failedCriteriaName].c_str());

            err.setErrMessage(errMsg);
            ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());

            return false;
        }


        //build expression map
        int32_t errorCount = createCriteriaExpressions();
        if (errorCount > 0)
        {
            char8_t errMsg[512];
            blaze_snzprintf(errMsg, sizeof(errMsg), "Error: unable to create game entry criteria expressions, createCriteriaExpressions() returned %d errors",
                errorCount);

            err.setErrMessage(errMsg);
            ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
            return false;
        }

        // iterate over the game roles, and validate the expression maps
        RoleCriteriaMap::const_iterator roleCriteriaIter = mGameCreationData.getRoleInformation().getRoleCriteriaMap().begin();
        RoleCriteriaMap::const_iterator roleCriteriaEnd = mGameCreationData.getRoleInformation().getRoleCriteriaMap().end();
        for (; roleCriteriaIter != roleCriteriaEnd; ++roleCriteriaIter)
        {
            const RoleName& roleName = roleCriteriaIter->first;
            RoleEntryCriteriaEvaluator *roleEntryCriteriaEvaluator = BLAZE_NEW RoleEntryCriteriaEvaluator(roleName, roleCriteriaIter->second->getRoleEntryCriteriaMap());
            mRoleEntryCriteriaEvaluators.insert(eastl::make_pair(roleName, roleEntryCriteriaEvaluator));
            if (!EntryCriteriaEvaluator::validateEntryCriteria(mGameCreationData.getEntryCriteriaMap(), failedCriteriaName))
            {
                char8_t errMsg[512];
                blaze_snzprintf(errMsg, sizeof(errMsg), "Error: unable to parse roleEntryCriteria named \"%s\"; expression=\"%s\" for role \"%s\".",
                    failedCriteriaName.c_str(), mGameCreationData.getEntryCriteriaMap()[failedCriteriaName].c_str(), roleName.c_str());

                err.setErrMessage(errMsg);
                ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                    << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());

                return false;
            }
            // create the expressions as we iterate through
            int32_t roleErrorCount = roleEntryCriteriaEvaluator->createCriteriaExpressions();
            if (roleErrorCount > 0)
            {
                char8_t errMsg[512];
                blaze_snzprintf(errMsg, sizeof(errMsg), "Error: Failed to create role-specific entry criteria expressions for role \"%s\".",
                    roleName.c_str());

                err.setErrMessage(errMsg);
                ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                    << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
                return false;
            }


        }

        // Validate the rules before creating them.
        EntryCriteriaName failedCriteriaNameTemp;
        if (!MultiRoleEntryCriteriaEvaluator::validateEntryCriteria(mGameCreationData.getRoleInformation(), failedCriteriaNameTemp, nullptr))
        {
            char8_t errMsg[512];
            blaze_snzprintf(errMsg, sizeof(errMsg), "Error: unable to parse multirole entry criteria  named \"%s\"; expression=\"%s\".",
                failedCriteriaNameTemp.c_str(), mGameCreationData.getRoleInformation().getMultiRoleCriteria()[failedCriteriaNameTemp].c_str());

            err.setErrMessage(errMsg);
            ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
            return false;
        }


        // create the MultiRole expressions
        int32_t roleErrorCount = mMultiRoleEntryCriteriaEvaluator.createCriteriaExpressions();
        if (roleErrorCount > 0)
        {
            char8_t errMsg[512];
            blaze_snzprintf(errMsg, sizeof(errMsg), "Error: Failed to create multirole entry criteria expressions.");

            err.setErrMessage(errMsg);
            ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
            return false;
        }


        // Ensure all users meet the criteria:
        for (UserJoinInfoList::const_iterator sessionInfoItr = request.getUsersInfo().begin(),
            sessionInfoEnd = request.getUsersInfo().end();
            sessionInfoItr != sessionInfoEnd; ++sessionInfoItr)
        {
            if ((*sessionInfoItr)->getIsOptional())
            {
                // Optional users don't check entry criteria:
                continue;
            }

            if (!evaluateEntryCriteria((*sessionInfoItr)->getUser().getUserInfo().getId(), (*sessionInfoItr)->getUser().getDataMap(), failedCriteriaName))
            {
                char8_t errMsg[512];
                blaze_snzprintf(errMsg, sizeof(errMsg), "Error: user unable to pass own gameEntryCriteria named \"%s\"; expression=\"%s\".",
                    failedCriteriaName.c_str(), mGameCreationData.getEntryCriteriaMap()[failedCriteriaName].c_str());

                err.setErrMessage(errMsg);
                ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                    << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
                return false;
            }

            const char8_t* sessionRole = lookupPlayerRoleName(mMatchmakingSupplementalData.mPlayerJoinData, (*sessionInfoItr)->getUser().getUserInfo().getId());
            if (EA_LIKELY(sessionRole))
            {
                RoleEntryCriteriaEvaluatorMap::const_iterator roleCriteriaIt = mRoleEntryCriteriaEvaluators.find(sessionRole);
                if (roleCriteriaIt != mRoleEntryCriteriaEvaluators.end())
                {
                    if (!roleCriteriaIt->second->evaluateEntryCriteria((*sessionInfoItr)->getUser().getUserInfo().getId(), (*sessionInfoItr)->getUser().getDataMap(), failedCriteriaName))
                    {
                        const EntryCriteria* failedCriteria = roleCriteriaIt->second->getEntryCriteria(failedCriteriaName);

                        // block a MM session whose members doesn't meet their own entry criteria
                        char8_t errMsg[512];
                        if (EA_LIKELY(failedCriteria))
                        {
                            blaze_snzprintf(errMsg, sizeof(errMsg), "Error: user unable to pass own roleEntryCriteria named \"%s\"; expression=\"%s\" for role \"%s\".",
                                failedCriteriaName.c_str(), failedCriteria->c_str(), sessionRole);
                        }
                        else
                        {
                            // couldn't retrieve EntryCriteria
                            blaze_snzprintf(errMsg, sizeof(errMsg), "Error: user unable to pass own roleEntryCriteria named \"%s\";  for role \"%s\".",
                                failedCriteriaName.c_str(), sessionRole);
                        }


                        err.setErrMessage(errMsg);
                        ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                            << getOwnerPersonaName() << ")] matchmaking criteria error: " << err.getErrMessage());
                        return false;
                    }
                }
            }
        }

        return true;
    }

    void MatchmakingSession::addFindGameMatchScore(FitScore fitScore, GameId gameId)
    {
        if (mFindGameMatchScoreMap.size() >= mMatchmakerSlave.getConfig().getMatchmakerSettings().getNumDebugFindGameResults())
        {
            return;
        }
        mFindGameMatchScoreMap.insert(eastl::make_pair(fitScore, gameId));
    }

    void MatchmakingSession::outputFindGameMatchScores() const
    {
        uint32_t i = 1;
        FindGameMatchScoreMap::const_iterator iter = mFindGameMatchScoreMap.begin();
        FindGameMatchScoreMap::const_iterator end = mFindGameMatchScoreMap.end();
        for (; iter != end; ++iter)
        {
            TRACE_LOG("[MatchmakingResult Session '" << getMMSessionId() << "'] Match " << i << ": Game '" << iter->second << "' fitScore '" << iter->first
                << "' from Sliver '" << GetSliverIdFromSliverKey(iter->second) << "'.");
            ++i;
        }
    }

    void MatchmakingSession::addDebugTopResult(const MatchedGame& matchedGame)
    {
        if (mDebugTopResultMap.size() < mMatchmakerSlave.getConfig().getMatchmakerSettings().getNumDebugFindGameResults())
        {
            DebugTopResult debugTopResult;
            debugTopResult.setGameId(matchedGame.getGameId());
            debugTopResult.setGameName(matchedGame.getGameName());
            debugTopResult.setOverallFitScore(matchedGame.getFitScore());
            debugTopResult.setMaxFitScore(matchedGame.getMaxFitScore());
            debugTopResult.setTimeToMatch(matchedGame.getTimeToMatch());
            debugTopResult.setOverallResult(matchedGame.getOverallResult());
            matchedGame.getDebugResultMap().copyInto(debugTopResult.getRuleResults());
            mDebugTopResultMap.insert(eastl::make_pair(matchedGame.getFitScore(), debugTopResult));
        }
    }

    void MatchmakingSession::outputFindGameDebugResults() const
    {
        // skip for all debug only's (they shouldn't ever call this) to avoid duplication with notification send
        if (!getDebugCheckOnly())
        {
            NotifyMatchmakingPseudoSuccess notification;
            buildNotifyMatchmakingPseudoSuccess(notification, SUCCESS_PSEUDO_FIND_GAME);

            TRACE_LOG("[MatchmakingResult Session '" << getMMSessionId() << "'] " << notification);
        }
    }

    void MatchmakingSession::setDebugCreateGame(const CreateGameMasterRequest& request, FitScore createdGameFitScore, BlazeId creatorId)
    {
        request.getCreateRequest().copyInto(mDebugCreateGameResults.getCreateGameRequest());
        mDebugCreateGameResults.setTimeToMatch(getMatchmakingRuntime());
        mDebugCreateGameResults.setFitScore(createdGameFitScore);
        mDebugCreateGameResults.setMaxFitScore(mCriteria.getMaxPossibleFitScore());
        mDebugCreateGameResults.setCreatorId(creatorId);
    }

    void MatchmakingSession::addDebugMatchingSession(const DebugSessionResult& debugResults)
    {
        mDebugCreateGameResults.getSessionResultList().push_back(debugResults.clone());
    }

    void MatchmakingSession::addDebugFinalizingSession()
    {
        // there should onle ever be one entry in this map, so clear preemptively
        // though if this was called, it shouldn't be possible for the debug session to be re-entered into the MM pool
        mDebugCreateGameResults.getSessionResultList().clear();
        mDebugCreateGameResults.getSessionResultList().push_back(mFinalizingSessionDebugResult.clone());
    }

    
    void MatchmakingSession::fillNotifyReservedExternalPlayers(GameId joinedGameId, NotifyMatchmakingReservedExternalPlayersMap& notificationMap) const
    {
        for (MatchmakingSession::MemberInfoList::const_iterator itr = getMemberInfoList().begin(); itr != getMemberInfoList().end(); ++itr)
        {
            const MatchmakingSession::MemberInfo& memberInfo = static_cast<const MatchmakingSession::MemberInfo &>(*itr);
            if (memberInfo.mIsOptionalPlayer && memberInfo.mUserSessionInfo.getHasExternalSessionJoinPermission())
            {
                // add member to notification's joined list
                NotifyServerMatchmakingReservedExternalPlayers& notification = notificationMap[getOwnerUserSessionId()];
                if (notification.getClientNotification().getJoinedReservedPlayerIdentifications().empty())
                {
                    notification.setUserSessionId(getOwnerUserSessionId());
                    notification.getClientNotification().setGameId(joinedGameId);
                }
                UserInfo::filloutUserIdentification(memberInfo.mUserSessionInfo.getUserInfo(), *notification.getClientNotification().getJoinedReservedPlayerIdentifications().pull_back());
            }
        }
    }

    bool MatchmakingSession::isSessionJoinableForTeam(const MatchmakingSession& session, const CreateGameFinalizationTeamInfo& teamInfo, const MatchmakingSession &autoJoinSession, TeamId joiningTeamId) const
    {
        return ((session.getMMSessionId() != autoJoinSession.getMMSessionId())//(autoJoined session already joined)
            && !teamInfo.wasSessionTriedForCurrentPick(session.getMMSessionId(), teamInfo.mParent->getTotalSessionsCount())
            && teamInfo.canPotentiallyJoinTeam(session, false, joiningTeamId));
    }

    /*! ************************************************************************************************/
    /*! \brief For team composition/ued balancing finalization's efficiency, the configured max pick sequence
        retries (see cfg) limits the number of possible MM session pick sequences/permutations attempted. To increase
        chance finding a successful finalization sequence with the limited retries, MM starts by varying the
        beginnings of the permutations it chooses as much as possible.  To do this, this fn 'reserves' retries first for
        permutating the first picks in the sequences, with remaining picks done using a less thorough but more
        efficient/good-chance 'best team ued balancing' manner.  It then 'reserves' left over retries for
        permutating longer 2-length beginning subsequences, and so on.

        For simplicity of caller's recursive algorithm, the tries with longer permutated beginnings actually get *executed*
        first, so this fn returns the current *highest* allowed length to permutate based on remaining retries.

        Note: since we prioritize permutating over the shorter start sequences, large scale, often only a length of 1
        would be permutated because there's often more matched sessions to try (at length 1) than configured max retries.
        Longer-length permutating is expected mainly for small scale or edge cases where we need to be more thorough.
    ***************************************************************************************************/
    uint16_t MatchmakingSession::calcMaxPermutablyRepickedSequenceLength(
        uint16_t currentOverallRetriesRemaining, uint16_t origMaxOverallRetries) const
    {
        const size_t totalPickableSessions = mMatchedToList.size();

        // side: quick approx max just to bound below loop's iteration, this might not be super-accurate but good enough as a guard
        uint16_t maxRepickableSeqLength = (getCriteria().getMaxPossibleParticipantCapacity() - 1);// -1 as host never repickable
        maxRepickableSeqLength = eastl::min((uint16_t)totalPickableSessions, maxRepickableSeqLength);

        if ((currentOverallRetriesRemaining == 0) || (maxRepickableSeqLength <= 0) || (totalPickableSessions < 1))
        {
            TRACE_LOG("[MatchmakingSession].calcMaxPermutablyRepickedSequenceLength: NO MATCH. Possible internal error: Couldn't determing repickable permutated pick length, there must be re-picks remaining (currentOverallRetriesRemaining=" << currentOverallRetriesRemaining << "), and sessions should be addable/repickable (maxRepickableSeqLength=" << maxRepickableSeqLength <<", totalPickableSessions=" << totalPickableSessions << ").");
            return 0;
        }

        // start at first pick and work towards tail, 'reserving' the levels' max retries (based on the total session count)
        // to finally arrive at highest pick number allowed that will have retries for.
        uint16_t sumRepicksAtPriorLengths = 0;
        uint16_t returnLen;
        for (returnLen = 1; returnLen < maxRepickableSeqLength; ++returnLen)
        {
            // Key: Calc number of permutations for curr picks seq length. That's how many retries pick seq length 'eats up' ('reserved').
            // Side: By just using the simple permutations calc here, we assume worst case sessions are all size 1 (and 'tryable'). This might not be super-accurate in the calc for each levels max retries, and could cause some longer-length permutations to have lower retry limits than possible by max configured, its likely only noticable small scale and can be worked around by cranking up retry limit.
            const size_t maxRepicksAtCurrLen = calcNumPermutationsOfLengthK(totalPickableSessions, (size_t)returnLen);

            if (currentOverallRetriesRemaining < ((uint16_t)maxRepicksAtCurrLen + sumRepicksAtPriorLengths))//pre: num permutations <= UINT16_MAX
            {
                // Next sequence-length would be overstep max retries. Return this pick number.
                TRACE_LOG("[MatchmakingSession].calcMaxPermutablyRepickedSequenceLength: found max length " << returnLen << ". Overall retries remaining(at any lengths):" << currentOverallRetriesRemaining << ", remaining retries allowed at this length: " << (currentOverallRetriesRemaining - sumRepicksAtPriorLengths) << ". (Calculation based on total pickable sessions " << totalPickableSessions << ", max repickable seq length " << maxRepickableSeqLength << ").");
                return returnLen;
            }
            sumRepicksAtPriorLengths += maxRepicksAtCurrLen;
            SPAM_LOG("[MatchmakingSession].calcMaxPermutablyRepickedSequenceLength: skipping not-highest allowed length: " << returnLen << " (total session permutations possible at this length: " << maxRepicksAtCurrLen << ", total remaining retries that would remain after this level's permutations: " << (currentOverallRetriesRemaining - sumRepicksAtPriorLengths) << ").");
        }
        // if here, result should be maxRepickableSeqLength
        TRACE_LOG("[MatchmakingSession].calcMaxPermutablyRepickedSequenceLength: found max length " << returnLen << " (remaining retries allowed at this length: " << (currentOverallRetriesRemaining - sumRepicksAtPriorLengths) << ". Current overall retries remaining :" << currentOverallRetriesRemaining << ", maxRepickableSeqLength " << maxRepickableSeqLength << ", total pickable sessions overall " << totalPickableSessions << ").");
        return returnLen;
    }

    /*! ************************************************************************************************/
    /*! \brief return numPossibleValues * (numPossibleValues-1) * .. (numPossibleValues-k+1)  =  n!/(n-k)!
    ***************************************************************************************************/
    size_t MatchmakingSession::calcNumPermutationsOfLengthK(size_t numPossibleValues, size_t k) const
    {
        if ((numPossibleValues == 0) || (k == 0) || (k > numPossibleValues))
            return 0;

        size_t n = numPossibleValues;
        size_t result = n;
        while (--n > 1)
        {
            if (n == (numPossibleValues - k))
                break;
            result *= n;
        }
        return result;
    }

    void MatchmakingSession::sendNotifyMatchmakingSessionConnectionValidated( const Blaze::GameManager::GameId gameId, bool dispatchClientListener )
    {
        // build response
        bool qosValidationPerformed = mMatchmakerSlave.shouldPerformQosCheck(mGameCreationData.getNetworkTopology());

        Blaze::GameManager::NotifyMatchmakingSessionConnectionValidated notification;
        notification.setGameId(gameId);
        notification.setDispatchSessionFinished(dispatchClientListener);
        notification.setSessionId(getMMSessionId());
        notification.setScenarioId(getMMScenarioId());
        // We always update the avoid list based on the *requested* network topology, even though find game mode may have matched a different topology.
        // The reason for this is that either a title has create-game mode enabled, *and* the topology being set is correct (create game wouldn't work otherwise)
        // OR, a title is using FG only, and either always uses the same topology or never sets it, meaning that using a single avoid list will get the correct result.
        // We can't divide things up by topology and keep multiple avoid lists, because titles that use multiple topologies across modes would block too aggressively
        // when a user switches modes.
        // I.E.: Failing to connect to someone in a mesh topology shouldn't result in avoiding that player when attempting to play in a mode that uses dedicated servers.
        notification.getConnectionValidatedResults().setNetworkTopology(mGameCreationData.getNetworkTopology());
        notification.getConnectionValidatedResults().setFailCount(mFailedConnectionAttempts);
        notification.getConnectionValidatedResults().setTier(mQosTier);
        notification.setQosValidationPerformed(qosValidationPerformed);

        // avoid lists
        mQosPlayerIdAvoidList.copyInto(notification.getConnectionValidatedResults().getAvoidPlayerIdList());
        mQosGameIdAvoidList.copyInto(notification.getConnectionValidatedResults().getAvoidGameIdList());

        // We've successfully performed QoS validation, meaning we'll attempt to finalize this game
        // Update our MM QoS KPI metrics here. sendNotifyMatchmakingFailed will handle the case where
        // we've failed QoS validation and this MM session has timed out or failed
        if (qosValidationPerformed && dispatchClientListener)
        {
            TRACE_LOG("[MatchmakingSession].sendNotifyMatchmakingSessionConnectionValidated: Updating MM QoS KPI metrics due to QoS validation succeding.");
            updateMMQoSKPIMetrics();
        }

        for (MatchmakingSession::MemberInfoList::const_iterator itr = getMemberInfoList().begin(); itr != getMemberInfoList().end(); ++itr)
        {
            const MatchmakingSession::MemberInfo& memberInfo = static_cast<const MatchmakingSession::MemberInfo &>(*itr);
            if (!memberInfo.mIsOptionalPlayer)
            {
                notification.setUserSessionId(memberInfo.mUserSessionId);
                mMatchmakerSlave.sendNotifyMatchmakingSessionConnectionValidatedToSliver(UserSession::makeSliverIdentity(memberInfo.mUserSessionId), &notification);
            }
        }
    }

    void MatchmakingSession::updateStartMatchmakingInternalRequestForFindGameRestart()
    {
        TimeValue t;
        t.setSeconds(getSessionAgeSeconds());
        mCachedMatchmakingRequestPtr->getRequest().getSessionData().setStartingDecayAge(t);

        QosCriteriaMap::iterator qosCriteriaIter = mCachedMatchmakingRequestPtr->getQosCriteria().find(mCachedMatchmakingRequestPtr->getRequest().getGameCreationData().getNetworkTopology());
        if ( qosCriteriaIter != mCachedMatchmakingRequestPtr->getQosCriteria().end())
        {
            ConnectionCriteria *connectionCriteria = qosCriteriaIter->second;
            connectionCriteria->setFailedConnectionAttempts(mFailedConnectionAttempts);
            connectionCriteria->setTier(mQosTier);

            // populate updated avoid lists
            mQosGameIdAvoidList.copyInto(connectionCriteria->getAvoidGameIdList());
            mQosPlayerIdAvoidList.copyInto(connectionCriteria->getAvoidPlayerIdList());
        }
    }

    bool MatchmakingSession::sessionContainsPlayer(PlayerId playerId) const
    {

        MatchmakingSession::MemberInfoList::const_iterator memberIter = mMemberInfoList.begin();
        MatchmakingSession::MemberInfoList::const_iterator memberEnd = mMemberInfoList.end();
        for(; memberIter != memberEnd; ++memberIter)
        {
            const MatchmakingSession::MemberInfo &memberInfo =
                static_cast<const MatchmakingSession::MemberInfo &>(*memberIter);

            if (playerId == memberInfo.mUserSessionInfo.getUserInfo().getId())
            {
                return true;
            }
        }

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief called after awaiting MM session members' QoS measurements, to complete QoS validation for the potential match.
    ***************************************************************************************************/
    bool MatchmakingSession::validateQosAndUpdateAvoidList(const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response)
    {
        bool allMembersFullyConnected = true;
        bool allMembersPassedLatencyValidation = true;
        bool allMembersPassedPacketLossValidation = true;
        bool usedCC = false;
        QosValidationCriteriaMap::const_iterator qosValidationIter = mMatchmakerSlave.getConfig().getMatchmakerSettings().getRules().getQosValidationRule().getConnectionValidationCriteriaMap().find(request.getNetworkTopology());
        if (qosValidationIter == mMatchmakerSlave.getConfig().getMatchmakerSettings().getRules().getQosValidationRule().getConnectionValidationCriteriaMap().end())
        {
            // if this topology isn't in the config, bail early
            WARN_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: attempting to evaluate QoS for topology ("
                << GameNetworkTopologyToString(request.getNetworkTopology()) << ") not present in configuration. Skipping eval.");
            return true;
        }

        if (qosValidationIter->second->getQosCriteriaList().empty())
        {
            ERR_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: attempting to evaluate QoS for topology ("
                << GameNetworkTopologyToString(request.getNetworkTopology()) << ") but there are no tiers defined/configured for the topology. Skipping eval.");
            return true;
        }
        else if ( mQosTier >= qosValidationIter->second->getQosCriteriaList().size() )
        {
            // fix current tier if outside of range
            WARN_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: attempting to evaluate QoS for topology ("
                << GameNetworkTopologyToString(request.getNetworkTopology()) << ") for tier (" << mQosTier << ") outside defined range.");
            mQosTier = qosValidationIter->second->getQosCriteriaList().size() - 1;
        }

        float worstPacketLoss = 0.0;
        uint32_t worstLatencyMs = 0;
        // each MM session is evaluated as a unit. We add all members to this list, and put it in the response's passed/failed list as a whole
        Blaze::GameManager::PlayerIdList memberIdList;
        // worst (and most likely case) all members are direct joining, no reservations
        memberIdList.reserve(mMatchmakingSupplementalData.mJoiningPlayerCount);

        const QosCriteria* qosCriteria = qosValidationIter->second->getQosCriteriaList().at(mQosTier);

        GameQosDataMap::const_iterator qosResultIter = request.getGameQosDataMap().begin();
        GameQosDataMap::const_iterator qosResultEnd = request.getGameQosDataMap().end();
        for (; qosResultIter != qosResultEnd; ++qosResultIter)
        {
            if (sessionContainsPlayer(qosResultIter->first))
            {
                memberIdList.push_back(qosResultIter->first);
                const PlayerQosData* qosData = qosResultIter->second;

                bool fullyConnected = qosData->getGameConnectionStatus() == CONNECTED;
                // this is a member of my MM session, eval the QoS of the connections
                if (!fullyConnected)
                {
                    // use the overall status, because we may not require connections to other players (partial mesh, for example)
                    TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                        << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: attempting to evaluate QoS for topology (" << GameNetworkTopologyToString(request.getNetworkTopology())
                        << ") member (" << qosResultIter->first << ") was not fully connected to the mesh in game (" << request.getGameId() << ").");
                    allMembersFullyConnected = false;
                }

                // now check the individual connections to validate QoS
                // Note: even if connectivity failed already, while at it, we can possibly still update avoid lists to avoid future bad matches below.
                LinkQosDataMap::const_iterator linkQosIter = qosData->getLinkQosDataMap().begin();
                LinkQosDataMap::const_iterator linkQosEnd = qosData->getLinkQosDataMap().end();
                for (; linkQosIter != linkQosEnd; ++linkQosIter)
                {
                    const LinkQosData* linkQosData = linkQosIter->second;
                    bool addToAvoidList = false;

                    if (linkQosData->getConnectivityHosted())
                    {
                        usedCC = true;
                    }

                    // if we're fully connected, skip over members of the game we're not connected to, they're not needed
                    if (linkQosData->getLinkConnectionStatus() == CONNECTED)
                    {

                        if (linkQosData->getLatencyMs() > qosCriteria->getMaximumLatencyMs())
                        {
                            TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                                << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: attempting to evaluate QoS for topology (" << GameNetworkTopologyToString(request.getNetworkTopology())
                                << ") member (" << qosResultIter->first << ") failed latency evaluation to user (" << linkQosIter->first << ") in game (" << request.getGameId()
                                << ") latency of (" << linkQosData->getLatencyMs() << "ms) exceeded tier (" << mQosTier << ") maximum of (" << qosCriteria->getMaximumLatencyMs() << "ms).");
                            addToAvoidList = true;
                            allMembersPassedLatencyValidation = false;
                        }

                        if (linkQosData->getLatencyMs() > worstLatencyMs)
                        {
                            worstLatencyMs = linkQosData->getLatencyMs();
                        }

                        if (linkQosData->getPacketLoss() > qosCriteria->getMaximumPacketLoss())
                        {
                            TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                                << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: attempting to evaluate QoS for topology (" << GameNetworkTopologyToString(request.getNetworkTopology())
                                << ") member (" << qosResultIter->first << ") failed packet loss evaluation to user (" << linkQosIter->first << ") in game (" << request.getGameId()
                                << ") packet loss of (" << linkQosData->getPacketLoss() << "%) exceeded tier (" << mQosTier << ") maximum of (" << qosCriteria->getMaximumPacketLoss() << "%).");
                            addToAvoidList = true;
                            allMembersPassedPacketLossValidation = false;
                        }

                        if (linkQosData->getPacketLoss() > worstPacketLoss)
                        {
                            worstPacketLoss = linkQosData->getPacketLoss();
                        }
                    }
                    else if (!fullyConnected)
                    {
                        // add this user to the avoid list, this connection was required for the mesh, and failed
                        TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                            << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: attempting to evaluate QoS for topology (" << GameNetworkTopologyToString(request.getNetworkTopology())
                            << ") member (" << qosResultIter->first << ") failed connection evaluation to user (" << linkQosIter->first << ") in game (" << request.getGameId()
                            << ").");
                        addToAvoidList = true;
                    }

                    if (addToAvoidList)
                    {
                        // update avoid list
                        TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                            << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: added player (" << linkQosIter->first
                            << ") to avoid list for topology (" << GameNetworkTopologyToString(request.getNetworkTopology()) << ").");

                        // don't add a dedicated server host to the player avoid list
                        if (request.getNetworkTopology() != CLIENT_SERVER_DEDICATED)
                        {
                            mQosPlayerIdAvoidList.push_back(linkQosIter->first);
                        }
                    }
                }
            }
        }

        // If we've passed qos validation, we need to increment our MM QoS KPI metrics based only on the qos results of this game
        bool passedQosValidation = (allMembersFullyConnected && allMembersPassedLatencyValidation && allMembersPassedPacketLossValidation);
        if (passedQosValidation)
        {
            mUsedCC = usedCC;
            mAllMembersFullyConnected = allMembersFullyConnected;
            mAllMembersPassedLatencyValidation = allMembersPassedLatencyValidation;
            mAllMembersPassedPacketLossValidation = allMembersPassedPacketLossValidation;

            TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: passed QoS validation with values usedCC: " << (mUsedCC ? "true" : "false") << ", allMembersFullyConnected: " << (mAllMembersFullyConnected ? "true" : "false")
                << ", allMembersPassedLatencyValidation: " << (mAllMembersPassedLatencyValidation ? "true" : "false") << ", allMembersPassedPacketLossValidation: " << (mAllMembersPassedPacketLossValidation ? "true" : "false"));
        }
        else // Otherwise, we keep track of this game's qos results along with any other games on which we've performed (failed) validation
        {
            mUsedCC = mUsedCC || usedCC;
            mAllMembersFullyConnected = allMembersFullyConnected && mAllMembersFullyConnected;
            mAllMembersPassedLatencyValidation = allMembersPassedLatencyValidation && mAllMembersPassedLatencyValidation;
            mAllMembersPassedPacketLossValidation = allMembersPassedPacketLossValidation && mAllMembersPassedPacketLossValidation;

            TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: failed QoS validation with values usedCC: " << (mUsedCC ? "true" : "false") << ", allMembersFullyConnected: " << (mAllMembersFullyConnected ? "true" : "false")
                << ", allMembersPassedLatencyValidation: " << (mAllMembersPassedLatencyValidation ? "true" : "false") << ", allMembersPassedPacketLossValidation: " << (mAllMembersPassedPacketLossValidation ? "true" : "false"));
        }


        // failover topology games shouldn't go into the avoid list, just the members of the game that were unconnectable
        if ((request.getNetworkTopology() == CLIENT_SERVER_DEDICATED) && !passedQosValidation)
        {
            // add dedicated server game to avoid list
            TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: added dedicated server game (" << request.getGameId()
                << ") to avoid list.");

            mQosGameIdAvoidList.push_back(request.getGameId());
        }

        if (!passedQosValidation)
        {
            ++mFailedConnectionAttempts;
            if (mFailedConnectionAttempts >= qosCriteria->getAttemptsAtTier())
            {
                if (mQosTier < (qosValidationIter->second->getQosCriteriaList().size() - 1))
                {
                    // failed enough to bump to next tier
                    TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                        << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: failed (" << mFailedConnectionAttempts << ") times at QoS tier (" << mQosTier
                        << ") for topology (" << GameNetworkTopologyToString(request.getNetworkTopology()) << ") worst latency was (" << worstLatencyMs
                        << "ms) and worst packet loss was (" << worstPacketLoss << "%), advancing to next tier, clearing avoid lists.");
                    ++mQosTier;
                    mFailedConnectionAttempts = 0;
                    // clear avoid lists when increasing tier
                    mQosPlayerIdAvoidList.clear();
                    mQosGameIdAvoidList.clear();
                }
            }
            response.getFailedValidationList().insert(response.getFailedValidationList().end(), memberIdList.begin(), memberIdList.end());
            if (isCreateGameEnabled())
            {
                getCriteria().updateQosAvoidPlayersRule(mMatchmakingSupplementalData);
            }
        }
        else
        {
            if (mFailedConnectionAttempts > 0)
            {
                mFailedConnectionAttempts = 0;
            }
            else if (mQosTier > 0) // things are getting better, bump us down a tier.
            {
                // make sure our latest attempt would qualify for the previous tier before dropping
                const QosCriteria* previousQosCriteria = qosValidationIter->second->getQosCriteriaList().at(mQosTier - 1);
                if ((worstLatencyMs <= previousQosCriteria->getMaximumLatencyMs()) && (worstPacketLoss <= previousQosCriteria->getMaximumPacketLoss()))
                {
                    TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                        << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: passed validation at QoS tier (" << mQosTier
                        << ") for topology (" << GameNetworkTopologyToString(request.getNetworkTopology()) << ") worst latency was (" << worstLatencyMs
                        << "ms) and worst packet loss was (" << worstPacketLoss << "%), dropping to previous tier.");
                    --mQosTier;
                }
            }

            response.getPassedValidationList().insert(response.getPassedValidationList().end(), memberIdList.begin(), memberIdList.end());
        }

        // Update our per-game evaluation MM QoS KPI metrics

        uint32_t numUsersIncomplete = 0;
        for (auto& usersItr : mCachedMatchmakingRequestPtr->getUsersInfo())
        {
            BlazeId id = usersItr->getUser().getUserInfo().getId();
            // Case 1: Count anyone else who was in the original request, but since left (or was removed from) the session and had their QoS data removed
            // Case 2: There also might be edge case that user was removed from qos map from coreMaster, but matchmaker hasn't receive user removal notification, memberIdlist still contains the user.
            // We treat the above 2 cases as user incomplete
            if (request.getGameQosDataMap().find(id) == request.getGameQosDataMap().end() || eastl::find(memberIdList.begin(), memberIdList.end(), id) == memberIdList.end())
            {
                ++numUsersIncomplete;
            }
        }

        mMatchmaker.getMatchmakerMetrics(this).mQosValidationMetrics.mTotalUsers.increment((uint64_t)numUsersIncomplete, request.getNetworkTopology(), NOT_FULLY_CONNECTED_INCOMPLETE, usedCC ? CC_HOSTEDONLY_MODE : CC_UNUSED);

        // To simplify, we now count all members of a group Matchmaking Session failing a QoS request as failing for the same single reason.
        // The first reason for failure, by below order determines which reason's metrics gets updated.
        // 1. check connectivity
        // 2. check latency
        // 3. check packet loss
        QosValidationMetricResult qosResult = PASSED_QOS_VALIDATION;
        if (!allMembersFullyConnected)
        {
            qosResult = NOT_FULLY_CONNECTED_FAILED;
        }
        else
        {
            if (!allMembersPassedLatencyValidation)
            {
                qosResult = FAILED_LATENCY_CHECK;
            }
            else
            {
                if (!allMembersPassedPacketLossValidation)
                {
                    qosResult = FAILED_PACKET_LOSS_CHECK;
                }
                // else all passed ... legacy
            }
        }
        mMatchmaker.getMatchmakerMetrics(this).mQosValidationMetrics.mTotalUsers.increment((uint64_t)memberIdList.size(), request.getNetworkTopology(), qosResult, usedCC ? CC_HOSTEDONLY_MODE : CC_UNUSED);

        TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
            << getOwnerPersonaName() << ")].validateQosAndUpdateAvoidList: " << (passedQosValidation ? "passed" : "failed") << " qos evaluation for game ("
            << request.getGameId() << ").");
        return passedQosValidation;
    }

    bool MatchmakingSession::hasMemberWithExternalSessionJoinPermission() const
    {
        for (GameManager::UserSessionInfoList::const_iterator itr = getMembersUserSessionInfo().begin(), end = getMembersUserSessionInfo().end(); itr != end; ++itr)
        {
            if ((*itr)->getHasExternalSessionJoinPermission())
            {
                return true;
            }
        }
        return false;
    }

    // Updates the per-MM Session QoS KPI metrics
    void MatchmakingSession::updateMMQoSKPIMetrics()
    {
        // To simplify, we now count all members of a group Matchmaking Session failing a QoS request as failing for the same single reason.
        // The first reason for failure, by below order determines which reason's metrics gets updated.
        // 1. check connectivity
        // 2. check latency
        // 3. check packet loss
        QosValidationMetricResult qosResult = PASSED_QOS_VALIDATION;
        if (!mAllMembersFullyConnected)
        {
            qosResult = NOT_FULLY_CONNECTED_FAILED;
        }
        else
        {
            if (!mAllMembersPassedLatencyValidation)
            {
                qosResult = FAILED_LATENCY_CHECK;
            }
            else
            {
                if (!mAllMembersPassedPacketLossValidation)
                {
                    qosResult = FAILED_PACKET_LOSS_CHECK;
                }
            }
        }
        mMatchmaker.getMatchmakerMetrics(this).mQosValidationMetrics.mTotalUsersInQosRequests.increment((uint64_t)mMatchmakingSupplementalData.mJoiningPlayerCount, mGameCreationData.getNetworkTopology(), qosResult, mUsedCC ? CC_HOSTEDONLY_MODE : CC_UNUSED);
        mMatchmaker.getMatchmakerMetrics(this).mQosValidationMetrics.mTotalMatchmakingSessions.increment(1, mGameCreationData.getNetworkTopology(), qosResult, mQosTier); // per-MMsession
    }

    /*!************************************************************************************************/
    /*! \brief finds a role that can be assigned as the 'any role' for the player
        \param[in] createGameRequest - Request object for creation of the game
        \param[in] playerData - Join request's per player join data. Used to count those in request that explicitly
            specified roles, to ensure there'd be enough space for them in the roles, before picking as the player's open role
        \param[in/out] foundRoleName - Role name found for the player
        \param[in/out] roleCounts - Tracks how many players have been 'added' to the game and at what team/role
            such that role capacity and criteria validation works
    ***************************************************************************************************/
    BlazeRpcError MatchmakingSession::findOpenRole(const CreateGameMasterRequest& createGameRequest, const PerPlayerJoinData& playerData,
                                                   RoleName &foundRoleName, TeamIndexRoleSizeMap& roleCounts) const
    {
        RoleNameList desiredRoles;
        playerData.getRoles().copyInto(desiredRoles); // Need a copy so we can shuffle it randomly
        const RoleCriteriaMap& roleCriteriaMap = createGameRequest.getCreateRequest().getGameCreationData().getRoleInformation().getRoleCriteriaMap();

        if (!desiredRoles.empty())
        {
            // Randomize our desired roles so that we don't inadvertently prefer some roles over others
            // There's a case to be made here for ordering the roles based on which roles are least desired amongst the group
            // such that we're more likely to choose roles that won't compete with other players
            // This can be looked as a future enhancement should a random selection prove unsatisfactory
            eastl::random_shuffle(desiredRoles.begin(), desiredRoles.end(), Random::getRandomNumber);

            RoleNameList::const_iterator desiredRoleIt = desiredRoles.begin();
            RoleNameList::const_iterator desiredRoleEnd = desiredRoles.end();
            for (; desiredRoleIt != desiredRoleEnd; ++desiredRoleIt)
            {
                RoleCriteriaMap::const_iterator roleCriteriaItr = roleCriteriaMap.find(*desiredRoleIt);
                if (roleCriteriaItr != roleCriteriaMap.end())
                {
                    if (validateOpenRole(createGameRequest, playerData, roleCriteriaItr, foundRoleName, roleCounts))
                        return ERR_OK;
                }
            }
        }
        else
        {
            RoleCriteriaMap::const_iterator roleCriteriaItr = roleCriteriaMap.begin();
            RoleCriteriaMap::const_iterator roleCriteriaEnd = roleCriteriaMap.end();
            for (; roleCriteriaItr != roleCriteriaEnd; ++roleCriteriaItr)
            {
                if (validateOpenRole(createGameRequest, playerData, roleCriteriaItr, foundRoleName, roleCounts))
                    return ERR_OK;
            }
        }

        return GAMEMANAGER_ERR_ROLE_FULL;
    }

    bool MatchmakingSession::validateOpenRole(const CreateGameMasterRequest& createGameRequest, const PerPlayerJoinData& playerData,
                                              const RoleCriteriaMap::const_iterator& roleCriteriaItr, RoleName &foundRoleName, TeamIndexRoleSizeMap& roleCounts) const
    {
        const PlayerId playerId = playerData.getUser().getBlazeId();
        const RoleName &currentRoleName = roleCriteriaItr->first;
        const RoleCriteria *roleCriteria = roleCriteriaItr->second;
        const uint16_t roleCapacity = roleCriteria->getRoleCapacity();
        const TeamIndex joiningTeamIndex = playerData.getTeamIndex();
        const PlayerJoinData& groupJoinData = createGameRequest.getCreateRequest().getPlayerJoinData();
        RoleSizeMap& roleRequirements = roleCounts[joiningTeamIndex];

        auto teamRoleCountsItr = roleCounts.insert(joiningTeamIndex).first;
        auto countItr = teamRoleCountsItr->second.insert(eastl::make_pair(currentRoleName, (uint16_t)0)).first;
        uint16_t currentRoleCount = countJoinersRequiringRole(joiningTeamIndex, currentRoleName, groupJoinData);

        TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
            << getOwnerPersonaName() << ")].validateOpenRole: Role capacity (" << roleCapacity << "), current role count (" << currentRoleCount << ") for role(" << currentRoleName << ") for player(" << playerId << ") on teamIndex ("
            << joiningTeamIndex << ").");

        // ensure enough room for possible remaining joining group members, before picking this role as an any role
        if (roleCapacity > currentRoleCount)
        {
            ++countItr->second;
            // make sure that this role selection doesn't violate multirole criteria
            EntryCriteriaName failedEntryCriteria;
            if (!mMultiRoleEntryCriteriaEvaluator.evaluateEntryCriteria(roleRequirements, failedEntryCriteria))
            {
                SPAM_LOG("[MatchmakingSession(" << getMMSessionId() << " / " << getOwnerBlazeId() << " / "
                    << getOwnerPersonaName() << ")].validateOpenRole: Open role " << currentRoleName <<
                    " failed multirole entry criteria to enter game, failedEntryCriteria(" << failedEntryCriteria << "), trying next role.");
            }
            else
            {
                // role entry criteria need to be tested
                RoleEntryCriteriaEvaluatorMap::const_iterator roleCriteriaIter = mRoleEntryCriteriaEvaluators.find(currentRoleName);
                if (roleCriteriaIter != mRoleEntryCriteriaEvaluators.end())
                {
                    for (const auto& userInfo : createGameRequest.getUsersInfo())
                    {
                        if (userInfo->getUser().getUserInfo().getId() == playerId)
                        {
                            if (!roleCriteriaIter->second->evaluateEntryCriteria(playerId, userInfo->getUser().getDataMap(), failedEntryCriteria))
                            {
                                SPAM_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                                    << getOwnerPersonaName() << ")].validateOpenRole: Player(" << playerId << ") failed game entry criteria for role '"
                                    << currentRoleName << "', failedEntryCriteria(" << failedEntryCriteria << ")");
                            }
                        }
                    }
                }

                foundRoleName = currentRoleName;
                TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
                    << getOwnerPersonaName() << ")].validateOpenRole: Role(" << foundRoleName << ") found for player(" << playerId << " on teamIndex ("
                    << joiningTeamIndex << ").");
                return true;
            }

            if (countItr->second > 0)
                --countItr->second;
        }

        TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
            << getOwnerPersonaName() << ")].validateOpenRole: Role(" << currentRoleName << ") is not available for player(" << playerId << ") on teamIndex ("
            << joiningTeamIndex << ").");
        return false;
    }

    uint16_t MatchmakingSession::countJoinersRequiringRole(TeamIndex teamIndex, const RoleName &roleName, const PlayerJoinData& groupJoinData) const
    {
        uint16_t count = 0;
        for (const auto& itr : groupJoinData.getPlayerDataList())
        {
            // for simplicity, no need to check whether user already joined
            if (isParticipantSlot(getSlotType(groupJoinData, itr->getSlotType())) && (itr->getTeamIndex() == teamIndex))
            {
                auto nextRole = lookupPlayerRoleName(groupJoinData, itr->getUser().getBlazeId());
                if ((nextRole != nullptr) && (blaze_stricmp(nextRole, roleName.c_str()) == 0))
                {
                    ++count;
                }
            }
        }

        TRACE_LOG("[MatchmakingSession(" << getMMSessionId() << "/" << getOwnerBlazeId() << "/"
            << getOwnerPersonaName() << ")].countJoinersRequiringRole: " << count << " players require role(" << roleName << ") on teamIndex ("
            << teamIndex << ").");

        return count;
    }

    void MatchmakingSession::timeoutSession()
    {
        mExpiryTimerId = INVALID_TIMER_ID;

        if (isLockedDown())
            return;

        mMatchmaker.timeoutExpiredSession(getMMSessionId());
    }

    void MatchmakingSession::sendAsyncStatusSession()
    {
        mStatusTimerId = INVALID_TIMER_ID;
        scheduleAsyncStatus(); // we always reschedule the status timer, regardless if the session is locked down or not
        if (!isLockedDown())
        {
            sendNotifyMatchmakingStatus();
        }
    }

    void MatchmakingSession::sortPlayersByDesiredRoleCounts(const GameManager::PerPlayerJoinDataList &perPlayerJoinData, UserRoleCounts<PerPlayerJoinDataPtr> &perPlayerJoinDataRoleCounts) const
    {
        for (auto& ppJoinData : perPlayerJoinData)
        {
            if (blaze_stricmp(ppJoinData->getRole(), PLAYER_ROLE_NAME_ANY) == 0)
            {
                size_t roleCount = ppJoinData->getRoles().size();
                // Special case to handle players desiring ANY role
                if (roleCount == 0)
                    roleCount = SIZE_MAX;
                perPlayerJoinDataRoleCounts.push_back(eastl::make_pair(ppJoinData, roleCount));
            }
        }

        // Sort the players by desired role count in ascending order such that we will select roles for the more restrictive players first
        // (i.e., players that desire fewer roles)
        eastl::sort(perPlayerJoinDataRoleCounts.begin(), perPlayerJoinDataRoleCounts.end(), UserRoleCountComparator<PerPlayerJoinDataPtr>());
    }

    bool MatchmakingSession::getIsUsingFilters() const
    {
        return (mCachedMatchmakingRequestPtr != nullptr) &&
            !mCachedMatchmakingRequestPtr->getMatchmakingFilters().getMatchmakingFilterCriteriaMap().empty() &&
            (isFindGameEnabled() || isCreateGameEnabled());
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

/*! ************************************************************************************************/
/*!
    \file   matchmakingcriteria.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_CRITERIA
#define BLAZE_MATCHMAKING_CRITERIA

#include "gamemanager/matchmaker/ruledefinitioncollection.h"
#include "gamemanager/matchmaker/matchmakingutil.h" // for MatchFailureMessage
#include "gamemanager/rolescommoninfo.h" // for RoleSizeMap

#include "gamemanager/matchmaker/rules/expandedpingsiterule.h"
#include "gamemanager/matchmaker/rules/expandedpingsiteruledefinition.h"
#include "gamemanager/matchmaker/rules/teamchoicerule.h"
#include "gamemanager/matchmaker/rules/teamcountrule.h"
#include "gamemanager/matchmaker/rules/teamminsizerule.h"
#include "gamemanager/matchmaker/rules/teambalancerule.h"
#include "gamemanager/matchmaker/rules/rolesrule.h"
#include "gamemanager/matchmaker/rules/hostviabilityruledefinition.h"
#include "gamemanager/matchmaker/rules/hostviabilityrule.h"
#include "gamemanager/matchmaker/rules/playersrule.h" // for PlayerToGamesMapping in MatchmakingSupplementalData
#include "gamemanager/matchmaker/rules/xblblockplayersrule.h" // for XblAccountIdToGamesMapping in MatchmakingSupplementalData
#include "gamemanager/matchmaker/rules/totalplayerslotsrule.h"
#include "gamemanager/matchmaker/rules/playercountrule.h"
#include "gamemanager/matchmaker/rules/playerslotutilizationrule.h"
#include "gamemanager/matchmaker/rules/qosavoidplayersrule.h"
#include "gamemanager/matchmaker/rules/preferredgamesrule.h"
#include "gamemanager/matchmaker/rules/xblblockplayersrule.h"
#include "gamemanager/matchmaker/rules/teamcompositionrule.h"
#include "gamemanager/matchmaker/rules/teamcompositionruledefinition.h" // for GameTeamCompositionsInfoVector in getAcceptableGameTeamCompositionsInfoVectorFromRule()
#include "gamemanager/tdf/gamemanagerconfig_server.h"
#include "gamemanager/tdf/matchmaker_server.h"//for GroupValueFormula in getTeamUEDBalanceRuleGroupFormula()
#include "gamemanager/tdf/matchmaking_properties_config_server.h"

#include "framework/tdf/attributes.h"
#include "framework/tdf/userdefines.h"
#include "framework/system/iterator.h"

namespace Blaze
{

namespace GameManager
{
    class GameSession;
    class GameSessionMaster;
    
namespace Rete
{
    class ReteRule;
}

namespace Matchmaker
{
    class Matchmaker;
    class MatchmakingSession;
    class Rule;
    class RuleDefinitionCollection;

    class GameSettingsRule;
    class TeamUEDBalanceRule;
    class TeamUEDPositionParityRule;
    class TeamCompositionRule;

    struct MatchmakingContext
    {
        MatchmakingContext(MatchmakingContextEnum context) { mContext = context; }
        const MatchmakingContext& operator=(const MatchmakingContext & rhs) { mContext = rhs.mContext; return *this; }

        // Logical classifications:
        bool isSearchingGames() const
        {
            return (canSearchResettableGames()
                 || canSearchJoinableGames());
        }
        bool isSearchingPlayers() const
        {
            return (mContext == MATCHMAKING_CONTEXT_MATCHMAKER_CREATE_GAME);
        }
        bool canSearchJoinableGames() const
        {
            return (mContext == MATCHMAKING_CONTEXT_MATCHMAKER_FIND_GAME
                 || mContext == MATCHMAKING_CONTEXT_GAMEBROWSER);
        }
        bool canSearchResettableGames() const
        {
            return (mContext == MATCHMAKING_CONTEXT_GAMEBROWSER 
                 || canOnlySearchResettableGames());
        }
        bool canOnlySearchResettableGames() const
        {
            return (mContext == MATCHMAKING_CONTEXT_FIND_DEDICATED_SERVER);
        }
        bool hasPlayerJoinInfo() const
        {
            return (mContext == MATCHMAKING_CONTEXT_MATCHMAKER_CREATE_GAME 
                 || mContext == MATCHMAKING_CONTEXT_MATCHMAKER_FIND_GAME 
                 || mContext == MATCHMAKING_CONTEXT_FIND_DEDICATED_SERVER);
        }
        // Only for use by GameSettingsRule which restricts searching based on type 
        bool isSearchingByMatchmaking() const
        {
            return (mContext == MATCHMAKING_CONTEXT_MATCHMAKER_CREATE_GAME 
                 || mContext == MATCHMAKING_CONTEXT_MATCHMAKER_FIND_GAME);
        }
        bool isSearchingByGameBrowser() const
        {
            return (mContext == MATCHMAKING_CONTEXT_GAMEBROWSER);
        }
    
        MatchmakingContextEnum mContext;
    };


    struct MatchmakingSupplementalData
    {
        MatchmakingSupplementalData(MatchmakingContextEnum matchmakingContext)
            : mAllIpsResolved(false),
              mHostSessionExternalIp(0),
              mHasMultipleStrictNats(false),
              mNetworkTopology(CLIENT_SERVER_PEER_HOSTED),
              mMatchmakingContext(matchmakingContext),
              mEvaluateGameProtocolVersionString(false),
              mJoiningPlayerCount(1),
              mDuplicateTeamsAllowed(false),
              mIgnoreGameSettingsRule(false),
              mIgnoreCallerPlatform(false),
              mPlayerToGamesMapping(nullptr),
              mXblAccountIdToGamesMapping(nullptr),
              mPrimaryUserInfo(nullptr),
              mMembersUserSessionInfo(nullptr),
              mXblIdBlockList(nullptr),
              mCachedTeamSelectionCriteria(nullptr),
              mQosGameIdAvoidList(nullptr),
              mQosPlayerIdAvoidList(nullptr),
              mMaxPlayerCapacity(BLAZE_DEFAULT_MAX_PLAYER_CAP),
              mIsPseudoRequest(false)
        {
        }

        MatchmakingSupplementalData& operator=(const MatchmakingSupplementalData& data)
        {
            if (this != &data)
            {
                data.mNetworkQosData.copyInto(mNetworkQosData);
                mAllIpsResolved = data.mAllIpsResolved;
                mHostSessionExternalIp = data.mHostSessionExternalIp;
                mHasMultipleStrictNats = data.mHasMultipleStrictNats;
                data.mPlayerJoinData.copyInto(mPlayerJoinData);
                mNetworkTopology = data.mNetworkTopology;
                mMatchmakingContext = data.mMatchmakingContext;
                mGameProtocolVersionString = data.mGameProtocolVersionString;
                mEvaluateGameProtocolVersionString = data.mEvaluateGameProtocolVersionString;
                mJoiningPlayerCount = data.mJoiningPlayerCount;
                mDuplicateTeamsAllowed = data.mDuplicateTeamsAllowed;
                data.mGameStateWhitelist.copyInto(mGameStateWhitelist);
                data.mGameTypeList.copyInto(mGameTypeList);
                mIgnoreGameSettingsRule = data.mIgnoreGameSettingsRule;
                mIgnoreCallerPlatform = data.mIgnoreCallerPlatform;
                mPlayerToGamesMapping = data.mPlayerToGamesMapping;
                mXblAccountIdToGamesMapping = data.mXblAccountIdToGamesMapping;
                mTeamIdRoleSpaceRequirements = data.mTeamIdRoleSpaceRequirements;
                mTeamIdPlayerRoleRequirements = data.mTeamIdPlayerRoleRequirements;                
                mPrimaryUserInfo = data.mPrimaryUserInfo;
                mMembersUserSessionInfo = data.mMembersUserSessionInfo;
                mXblIdBlockList = data.mXblIdBlockList;
                mCachedTeamSelectionCriteria = data.mCachedTeamSelectionCriteria;
                mQosGameIdAvoidList = data.mQosGameIdAvoidList;
                mQosPlayerIdAvoidList = data.mQosPlayerIdAvoidList;
                mMaxPlayerCapacity = data.mMaxPlayerCapacity;
                mPreferredPingSite = data.mPreferredPingSite;
                mIsPseudoRequest = data.mIsPseudoRequest;
                data.mTeamIds.copyInto(mTeamIds);
                data.mFilterMap.copyInto(mFilterMap);
            }

            return *this;
        }

        Util::NetworkQosData mNetworkQosData;
        bool mAllIpsResolved;
        uint32_t mHostSessionExternalIp;
        bool mHasMultipleStrictNats;
        PlayerJoinData mPlayerJoinData;
        GameNetworkTopology mNetworkTopology;
        MatchmakingContext mMatchmakingContext;
        GameProtocolVersionString mGameProtocolVersionString;
        bool mEvaluateGameProtocolVersionString;
        uint32_t mJoiningPlayerCount; // TODO: Merge this with user session iterator and user group Id.
        bool mDuplicateTeamsAllowed;
        GameManager::GameStateList mGameStateWhitelist;
        GameManager::GameTypeList mGameTypeList;
        bool mIgnoreGameSettingsRule;
        bool mIgnoreCallerPlatform; // used by the platformRule to enable tools that aren't platform specific to use the game browser and filter by allowed platforms.
        PlayerToGamesMapping* mPlayerToGamesMapping;
        XblAccountIdToGamesMapping* mXblAccountIdToGamesMapping;
        TeamIdRoleSizeMap mTeamIdRoleSpaceRequirements; // store session role space requirements info
        TeamIdPlayerRoleNameMap mTeamIdPlayerRoleRequirements;
        const UserSessionInfo* mPrimaryUserInfo;
        const UserSessionInfoList* mMembersUserSessionInfo;
        const ExternalXblAccountIdList* mXblIdBlockList;  // TODO: Just pass the StartMatchmakingInternalRequest around instead.
        TeamSelectionCriteria* mCachedTeamSelectionCriteria; // global cache not owned by any specific rule, but updated by rules
        const GameIdList* mQosGameIdAvoidList;
        const BlazeIdList* mQosPlayerIdAvoidList;
        uint16_t mMaxPlayerCapacity;
        PingSiteAlias mPreferredPingSite;
        bool mIsPseudoRequest;
        TeamIdVector mTeamIds;      // Teams desired for creation.
        MatchmakingFilterCriteriaMap mFilterMap;

    private:
        // no copy ctor
        MatchmakingSupplementalData(const MatchmakingSupplementalData& otherObj);
    };

    typedef eastl::vector<Rule*> RuleContainer;
    typedef eastl::vector<GameManager::Rete::ReteRule*> ReteRuleContainer;

    /*! ************************************************************************************************/
    /*!
        \brief MatchmakingCriteria holds the collection of rules used by a matchmaking session.  Call initialize
            and updateCachedInfo before evaluating.

        See the Matchmaking TDD for details: http://easites.ea.com/gos/FY09%20Requirements/Blaze/TDD%20Approved/Matchmaker%20TDD.doc
    *************************************************************************************************/
    class MatchmakingCriteria
    {
        NON_COPYABLE(MatchmakingCriteria);
    public:

        //! \brief Note: you must initialize the matchmaking criteria & update the cachedInfo before evaluating it.
        MatchmakingCriteria(RuleDefinitionCollection& ruleDefinitionCollection, MatchmakingAsyncStatus& matchmakingAsyncStatus, RuleDefinitionId useRuleDefinitionId = RuleDefinition::INVALID_DEFINITION_ID);
        MatchmakingCriteria(const MatchmakingCriteria& other, MatchmakingAsyncStatus& matchmakingAsyncStatus);        
        ~MatchmakingCriteria();

        /*! ************************************************************************************************/
        /*!
            \brief initialize a MatchmakingCriteria - get it ready for evaluation (as part of starting a matchmaking session).

            Given a MatchmakingCriteriaData, lookup and initialize each rule from the matchmaker;
            You cannot evaluate a criteria until it's been successfully initialized.

            \param[in]  criteriaData - the set of rules defining the criteria (from StartMatchmakingSession).
            \param[in]  matchmakingSupplementalData - used to lookup the rule definitions
            \param[out] err - errMessage is filled out if initialization fails.
            \param[out] diagnostics - if non nullptr, initializes diagnostic's rule use counts
            \return true on success, false on error (see the errMessage field of err)
        *************************************************************************************************/
        bool initialize(const MatchmakingCriteriaData& criteriaData,
                        const MatchmakingSupplementalData& matchmakingSupplementalData,
                        MatchmakingCriteriaError& err);

        /*! ************************************************************************************************/
        /*!
            \brief explicitly destroys the rule objects and maps created by initialize.

            The destructor implicitly does this as well, but if it's required to explicitly cleanup 
            MatchmakingCritieria, the owning object should invoke this method instead.
        *************************************************************************************************/
        void cleanup();

        /*! ************************************************************************************************/
        /*!
            \brief Set the owning matchmaking session so that the rules & criteria have access to the
                matchmaking session data.

            \param matchmakingSession The owning matchmaking session
        *************************************************************************************************/
        void setMatchmakingSession(const MatchmakingSession* matchmakingSession);

        /*! ************************************************************************************************/
        /*!
            \brief Get the owning matchmaking session so that the rules have access to the
                matchmaking session data.

            \return The owning matchmaking session
        *************************************************************************************************/
        const MatchmakingSession* getMatchmakingSession() const { return mMatchmakingSession; }

        /*! ************************************************************************************************/
        /*!
            \brief Returns true if the matchmaking session is set.
        
            \return true if the matchmaking session is set.
        *************************************************************************************************/
        bool isMatchmakingCriteria() const { return mMatchmakingSession != nullptr; }

        /*! ************************************************************************************************/
        /*!
            \brief update each rule's cached fit/count Thresholds to the given time.

                Instead of scanning through the minFitThresholdList every time we evaluate a rule, we can cache the value
                and do one lookup per session idle (since elapsed seconds doesn't change during a session's idle loop).

            \param[in]  elapsedSeconds - the 'age' of the matchmaking session
                  \param[in]  forceUpdate - if true, we update the async status with the current value
            \return true if this time change caused an update; false otherwise
        *************************************************************************************************/
        UpdateThresholdStatus updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate = false);

        /*! ************************************************************************************************/
        /*! \brief Returns the next threshold for any rule where this criteria should be updated.
        *************************************************************************************************/
        uint32_t getNextElapsedSecondsUpdate() const { return mNextUpdate; }

        /*! ************************************************************************************************/
        /*!
        \brief evaluate the criteria against a Game (for FindGame).  If the game is a 'match',
            we return the total weighted fit score (>= 0); otherwise we return a negative number.

            If this criteria doesn't match the supplied game, we return a log-able error message via
            matchFilaureMsg.  Note: the message is only formatted if logging is enabled in the 
            MatchFailureMessage object.

            \param[in]  game                The game to evaluate the criteria against.
            \param[out] debugResultMap Debugging information about rule evaluations.
            \param[in]  useDebug

            \return the total weighted fitScore for this criteria.  The game is not a match if the fitScore is negative.
        *************************************************************************************************/
        FitScore evaluateGame(const Search::GameSessionSearchSlave &game, DebugRuleResultMap& debugResultMap, bool useDebug = false) const;


        FitScore evaluateGameReteFitScore(const Search::GameSessionSearchSlave &game, DebugRuleResultMap& debugResultMap, bool useDebug = false) const;
        
        // Special version of evaluateGame that includes RETE-only rules.
        FitScore evaluateGameAllRules(const Search::GameSessionSearchSlave &game, DebugRuleResultMap& debugResultMap, bool useDebug = false) const;

        /*! ************************************************************************************************/
        /*!
            \brief evaluate the criteria against another matchmaking session (for CreateGame).  If the game is a 'match',
            we return the total weighted fit score (>= 0); otherwise we return a negative number.

            If this criteria doesn't match the supplied game, we return a log-able error message via
            matchFilaureMsg.  Note: the message is only formatted if logging is enabled in the 
            MatchFailureMessage object.

            \param[in]  session The session to evaluate the criteria against.
            \param[in/out] aggregateRuleMatchInfo aggregateMatchInfo for all of the rules.
        *************************************************************************************************/
        void evaluateCreateGame(MatchmakingSession &mySession, MatchmakingSession &session, AggregateSessionMatchInfo &aggregateRuleMatchInfo) const;
        
        /*! ************************************************************************************************/
        /*!
            \brief evaluate the criteria against a finalized create game request

            If this criteria doesn't match the supplied create game request, we return a log-able error message via
            matchFilaureMsg.  Note: the message is only formatted if logging is enabled in the 
            MatchFailureMessage object.

            \param[in] request - The create requests to evaluate the criteria against.
            \param[out] ruleResults - the debug output from matching
            \param[in] useDebug - tells the criteria if the session requests debug output or not.
            \return the total weighted fitScore for this criteria.  The game is not a match if the fitScore is negative.
        *************************************************************************************************/
        FitScore evaluateCreateGameRequest(const MatchmakingCreateGameRequest request,  DebugRuleResultMap& ruleResults, bool useDebug = false) const;
        
        /*! ************************************************************************************************/
        /*! - DEPRICATED -
            \brief Sums the max possible score for the this session's criteria
            \return the max possible fit score for this session.
        *************************************************************************************************/
        FitScore calcMaxPossibleFitScore() const { return getMaxPossibleFitScore(); };

        /*! ************************************************************************************************/
        /*!
            \brief Sums the max possible score for the this session's criteria
            \return the max possible fit score for this session.
        *************************************************************************************************/
        FitScore getMaxPossibleFitScore() const { return mMaxPossibleFitScore; }

        FitScore getMaxPossiblePostReteFitScore() const { return mMaxPossiblePostReteFitScore; }

        FitScore getMaxPossibleNonReteFitScore() const { return mMaxPossibleNonReteFitScore; }

        const Rule* getRuleByIndex(uint32_t index) const;

        
        /*! ************************************************************************************************/
        /*!
            \brief Returns the Rules for this search.
        
            \return The container of the Rules.
        *************************************************************************************************/
        const RuleContainer& getRules() const { return mRuleContainer; }
        
        /*! ************************************************************************************************/
        /*!
            \brief Returns the Rete Rules for this search.
        
            \return The container of the Rete Rules.
        *************************************************************************************************/
        const ReteRuleContainer& getReteRules() const { return mReteRuleContainer; }

        void initReteRules();

        bool isAnyTeamRuleEnabled() const 
        { 
            return  isRuleEnabled<TeamMinSizeRuleDefinition>() ||
                    isRuleEnabled<TeamBalanceRuleDefinition>() ||
                    isRuleEnabled<TeamChoiceRuleDefinition>() ||
                    isRuleEnabled<TeamCountRuleDefinition>() ||
                    hasTeamUEDBalanceRuleEnabled() || hasTeamCompositionRuleEnabled();
        }

        // Rule getters
        const TeamCountRule* getTeamCountRule() const { return getRule<TeamCountRuleDefinition>(); }
        const TeamChoiceRule* getTeamChoiceRule() const { return getRule<TeamChoiceRuleDefinition>(); }
        const TeamMinSizeRule* getTeamMinSizeRule() const { return getRule<TeamMinSizeRuleDefinition>(); }
        const TeamBalanceRule* getTeamBalanceRule() const { return getRule<TeamBalanceRuleDefinition>(); }
        const TeamCompositionRule* getTeamCompositionRule() const { return getRule<TeamCompositionRuleDefinition>(mTeamCompositionRuleId); }
        const RolesRule* getRolesRule() const { return getRule<RolesRuleDefinition>(); }
        const PreferredGamesRule* getPreferredGamesRule() const { return getRule<PreferredGamesRuleDefinition>(); }
        const PlayerCountRule* getPlayerCountRule() const { return getRule<PlayerCountRuleDefinition>(); }
        const TotalPlayerSlotsRule* getTotalPlayerSlotsRule() const { return getRule<TotalPlayerSlotsRuleDefinition>(); }
        const XblBlockPlayersRule* getXblBlockPlayersRule() const { return getRule<XblBlockPlayersRuleDefinition>(); }

        /*! \param[in] id if specified (not INVALID_DEFINITION_ID) checks whether this *specific* TeamUEDBalanceRule is enabled */
        bool hasTeamUEDBalanceRuleEnabled(RuleDefinitionId id = RuleDefinition::INVALID_DEFINITION_ID) const;
        const TeamUEDBalanceRule* getTeamUEDBalanceRule() const;
        UserExtendedDataKey getTeamUEDKeyFromRule() const;
        GroupValueFormula getTeamUEDFormulaFromRule() const;
        UserExtendedDataValue getTeamUEDMinValueFromRule() const;
        uint16_t getMaxFinalizationPickSequenceRetries() const;

        /*! \param[in] id if specified (not INVALID_DEFINITION_ID) checks whether this *specific* TeamUEDPositionParityRule is enabled */
        bool hasTeamUEDPositionParityRuleEnabled(RuleDefinitionId id = RuleDefinition::INVALID_DEFINITION_ID) const;
        const TeamUEDPositionParityRule* getTeamUEDPositionParityRule() const;
        UserExtendedDataKey getPositionParityUEDKeyFromRule() const;

        const GameTeamCompositionsInfoVector* getAcceptableGameTeamCompositionsInfoVectorFromRule() const;
        uint16_t getMaxFinalizationGameTeamCompositionsAttempted() const;

        // Rule enabled checks
        bool hasTeamCompositionRuleEnabled(RuleDefinitionId id = RuleDefinition::INVALID_DEFINITION_ID) const;
        bool hasTeamCompositionBasedFinalizationRulesEnabled() const { return (hasTeamUEDBalanceRuleEnabled(mTeamUEDBalanceRuleId) || hasTeamCompositionRuleEnabled(mTeamCompositionRuleId) || hasTeamUEDPositionParityRuleEnabled(mTeamUEDPositionParityRuleId)); }
        bool isPlayerCountRuleEnabled() const { return isRuleEnabled<PlayerCountRuleDefinition>(); }
        bool isTotalSlotsRuleEnabled() const { return isRuleEnabled<TotalPlayerSlotsRuleDefinition>(); }
        bool isXblBlockPlayersRuleEnabled() const { return isRuleEnabled<XblBlockPlayersRuleDefinition>(); }
        bool isPlayerSlotUtilizationRuleEnabled() const { return isRuleEnabled<PlayerSlotUtilizationRuleDefinition>(); }

        bool isExpandedPingSiteRuleEnabled() const { return isRuleEnabled<ExpandedPingSiteRuleDefinition>(); }
        const ExpandedPingSiteRule* getExpandedPingSiteRule() const { return getRule<ExpandedPingSiteRuleDefinition>(); }

        // ugly casts required to update rule after QoS validation failure, essentially, it's like restarting MM
        void updateQosAvoidPlayersRule(const MatchmakingSupplementalData &mmSupplementalData) const 
            { const_cast<QosAvoidPlayersRule*>(getRule<QosAvoidPlayersRuleDefinition>())->updateQosAvoidPlayerIdsForCreateMode(mmSupplementalData); }


        // Team value helper functions
        bool getDuplicateTeamsAllowed() const;
        uint16_t getUnspecifiedTeamCount() const;
        const TeamIdVector* getTeamIds() const;     // Teams required to create the game

        const TeamIdSizeList* getTeamIdSizeListFromRule() const;  // Teams (+join sizes) of players making the request
        uint16_t getTeamCountFromRule() const;

        const TeamIdRoleSizeMap* getTeamIdRoleSpaceRequirementsFromRule() const;
        
        // This varies, so we can't just get the value directly, we have to use the interpolated one.
        uint16_t getAcceptableTeamMinSize() const;
        uint16_t getAcceptableTeamBalance() const;
        UserExtendedDataValue getAcceptableTeamUEDImbalance() const;
        UserExtendedDataValue getTeamUEDContributionValue() const;
        
        // Public player/team capacity:
        uint16_t getParticipantCapacity() const;
        uint16_t getTeamCapacity() const;
        uint16_t getMaxPossibleParticipantCapacity() const;
        uint16_t getMaxPossibleTeamCapacity() const;


        /*! ************************************************************************************************/
        /*! \brief return true if the supplied matchmakingSession is a viable game host.  We check the session's
                NAT type against the hostViability rule's minFitThreshold.
        
            NOTE: you might want to customize this method to take the host's upstream bandwidth into account,
                depending on game size and network topology.

            \param[in] matchmakingSession the matchmaking session to check.
            \return true if the session's NAT type matches the hostViabilityRule's current minFitThreshold.
        ***************************************************************************************************/
        bool isViableGameHost(const MatchmakingSession& matchmakingSession) const 
            { return getRule<HostViabilityRuleDefinition>()->isViableGameHost(matchmakingSession); }

        /*! ************************************************************************************************/
        /*! \brief vote on various game values (settings/attributes), and set them in the supplied createGameRequest.
        
            \param[in] votingSessionList list of all the matchmaking sessions that will be voting.
            \param[in,out] createGameRequest the various voted values are set in the createGameRequest object.
        ***************************************************************************************************/
        void voteOnGameValues(const MatchmakingSessionList &votingSessionList, CreateGameRequest &createGameRequest) const;

        /*! ************************************************************************************************/
        /*! \brief set the ruleMode in all rules of matchmaking criteria
        ***************************************************************************************************/
        void setRuleEvaluationMode(RuleEvaluationMode ruleMode);

        const RuleDefinitionCollection& getRuleDefinitionCollection() const { return mRuleDefinitionCollection; }

        uint16_t calcMaxPossPlayerSlotsFromRules(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, uint16_t globalMaxBound) const;
        uint16_t calcMinPossPlayerCountFromRules(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, uint16_t globalMinBound) const;
        uint16_t calcTeamCountFromRules(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, bool* teamCountIsDefault = nullptr) const;

        void cacheTeamSelectionCriteria(uint16_t joiningPlayerCount);
        const GameManager::TeamSelectionCriteria& getTeamSelectionCriteriaFromRules() const { return mCachedTeamSelectionCriteria; }
        GameManager::TeamSelectionCriteria& getTeamSelectionCriteriaFromRules() { return mCachedTeamSelectionCriteria; }
    private:
        bool initRules(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError &err);
        void sortRules();
        bool validateEnabledGameSizeRulesForRequest(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, eastl::string &err);
        bool validateEnabledTeamSizeRulesForRequest(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, eastl::string &err);

        FitScore evaluateGameInternal(const RuleContainer& ruleContainer, const Search::GameSessionSearchSlave &game, DebugRuleResultMap& debugResultMap, bool useDebug /* = false*/) const;

        // Look rules by their rule definition:
        // Requires a rule id if the definition is a MULTI type (UED, Attribute rules, etc.)
        template<typename RuleDef>
        const typename RuleDef::mRuleType* getRule(RuleDefinitionId id = RuleDefinition::INVALID_DEFINITION_ID) const; 
        const Rule* getRuleById(RuleDefinitionId ruleDefinitionId) const;

        template<typename RuleDef>
        bool isRuleEnabled(RuleDefinitionId id = RuleDefinition::INVALID_DEFINITION_ID) const; 

    private:
        const MatchmakingSession* mMatchmakingSession;

        RuleDefinitionCollection& mRuleDefinitionCollection;

        MatchmakingAsyncStatus& mMatchmakingAsyncStatus;

        RuleContainer mRuleContainer;
        ReteRuleContainer mReteRuleContainer;
        RuleContainer mReteNoncapableRuleContainer;
        RuleContainer mPostReteFitScoreRuleContainer;

        uint32_t mNextUpdate;
        FitScore mMaxPossibleFitScore;
        FitScore mMaxPossiblePostReteFitScore;
        FitScore mMaxPossibleNonReteFitScore;

        GameManager::TeamSelectionCriteria mCachedTeamSelectionCriteria;

        // Note: MM allows multiple TeamUEDPositionParityRule/TeamUEDBalanceRule/TeamCompositionRules to be 
        // defined in the configuration, each with a unique RuleDefinitionId (in the respective rule definition's RuleDefinitionIdMap). 
        // To quickly figure out which rule of each type (if any) is enabled for this matchmaking session we cache 
        // their RuleDefinitionIds here.
        RuleDefinitionId mTeamUEDBalanceRuleId;
        RuleDefinitionId mTeamUEDPositionParityRuleId;
        RuleDefinitionId mTeamCompositionRuleId;
        
    };

    template<typename RuleDef>
    const typename RuleDef::mRuleType* MatchmakingCriteria::getRule(RuleDefinitionId id) const 
    {
        if (RuleDef::getRuleDefType() != RULE_DEFINITION_TYPE_MULTI)
        {
            return static_cast<const typename RuleDef::mRuleType*>(getRuleById(RuleDef::getRuleDefinitionId()));
        }
        else
        {
            // Sanity Check to make sure that the types match:
            const RuleDefinition* ruleDef = mRuleDefinitionCollection.getRuleDefinitionById(id);
            if (!ruleDef)
            {
                // This is common (new TeamUEDBalance rules may not be used)
                SPAM_LOG("[MatchmakingCriteria::getRule] Unable to find definition type "<< RuleDef::getConfigKey() <<" for rule lookup. Returning nullptr.");
                return nullptr;
            }
            else if(!ruleDef->isType(RuleDef::getConfigKey()))
            {
                ERR_LOG("[MatchmakingCriteria::getRule] Unable to find definition type "<< RuleDef::getConfigKey() <<" for rule lookup. (Found type " << ruleDef->getType() << ") Returning nullptr.");
                return nullptr;
            }


            const Rule* rule = getRuleById(id);
            if (!rule)
            {
                ERR_LOG("[MatchmakingCriteria::getRule] Rule definition " << ruleDef->getType() << " exists, but the rule does not (def may be disabled). Returning nullptr.");
                return nullptr;
            }
            
            if (!rule->isRuleType(ruleDef->getType()))
            {
                ERR_LOG("[MatchmakingCriteria::getRule] Rule found from lookup does not match expected type ("<< rule->getRuleType() <<" != "<< ruleDef->getType() <<"). Returning nullptr.");
                return nullptr;
            }

            return static_cast<const typename RuleDef::mRuleType*>(rule);
        }
    }

    template<typename RuleDef>
    inline bool MatchmakingCriteria::isRuleEnabled(RuleDefinitionId id) const
    {
        const typename RuleDef::mRuleType *rule = getRule<RuleDef>(id); 
        return mRuleDefinitionCollection.isRuleDefinitionInUse<RuleDef>() && (rule != nullptr) && !rule->isDisabled();
    }



} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_CRITERIA

/*! ************************************************************************************************/
/*!
    \file gamesession.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAME_SESSION_H
#define BLAZE_GAMEMANAGER_GAME_SESSION_H

#include "gamemanager/tdf/gamemanager_server.h" // for tdf types
#include "gamemanager/tdf/gamemanager.h" // for tdf types
#include "util/tdf/utiltypes.h"

#include "framework/util/entrycriteria.h"
#include "gamemanager/playerinfo.h"
#include "gamemanager/playerroster.h"
#include "gamemanager/roleentrycriteriaevaluator.h"


namespace Blaze
{
namespace Matchmaker
{
    class RuleDefinitionCollection;
    class MatchmakingSessionList;
}

namespace GameManager
{

namespace Matchmaker
{
    class GroupUedExpressionList;
}

    typedef eastl::vector_set<UserGroupId> SingleGroupMatchIdSet;
    typedef eastl::vector_map<TeamId, TeamIndex> TeamIdToTeamIndexMap;
    typedef eastl::set<TeamIndex> TeamIndexSet;
    typedef eastl::set<TeamId> TeamIdSet;
    typedef eastl::vector_map<eastl::string, UserExtendedDataValue> TeamUedMap;
    typedef eastl::vector_map<TeamIndex, TeamUedMap> TeamIndexToTeamUedMap;

    /*! ************************************************************************************************/
    /*! \brief structure operator used by list sort function.

    \param[in] n1 first game protocol version to compare
    \param[in] n2 second game protocol version to compare
    \returns true if either game protocol version is GAME_PROTOCOL_MATCH_ANY, the two game protocol versions are the same
    ***************************************************************************************************/
    bool isGameProtocolVersionStringCompatible(bool doEvaluate, const char8_t* gp1, const char8_t* gp2);

    /*! ************************************************************************************************/
    /*! \brief an abstract base interface class for GameSessionMaster and GameSessionSearchSlave instances
            containing common functionality.

        The GameSession contains a reference to the ReplicatedGameDataServer object (containing the replicated game data)
    ***************************************************************************************************/
    class GameSession : public EntryCriteriaEvaluator
    {
        NON_COPYABLE(GameSession);

    public:

        //! \brief returns the game's unique identifier
        GameId getGameId() const { return mGameId; } // intentionally using cached copy to enable usage of GameSession::getGameId() even after ReplicatedGameSession object is yanked by the Replicator
        GameReportingId getGameReportingId() const { return getGameData().getGameReportingId(); }
        GameReportName getGameReportName() const { return getGameData().getGameReportName(); }

        //! \brief returns the game's (non-unique) name.
        const char8_t* getGameName() const { return getGameData().getGameName(); }

        //! \brief returns the game's game group type.
        GameType getGameType() const { return getGameData().getGameType(); }

        //! \brief returns the game's current 1st party presence mode.
        PresenceMode getPresenceMode() const { return getGameData().getPresenceMode(); }
        const ClientPlatformTypeList& getPresenceDisabledList() const { return getGameData().getPresenceDisabledList(); }
        bool isPresenceDisabledForPlatform(ClientPlatformType platform) const;

        //! \brief returns the game's GameNetworkProtocolVersion.  This identifier is used to segment incompatible game clients from each other.
        const char8_t *getGameProtocolVersionString() const { return getGameData().getGameProtocolVersionString(); }
        uint64_t getGameProtocolVersionHash() const { return getGameData().getGameProtocolVersionHash(); }

        //! \brief returns the game's gameNetwork topology enumeration.
        GameNetworkTopology getGameNetworkTopology() const { return getGameData().getNetworkTopology(); }

        //! \brief return the game's voipNetwork topology enumeration.
        VoipTopology getVoipNetwork() const { return getGameData().getVoipNetwork(); }

        //! \brief returns the game's CCS mode
        CCSMode getCCSMode() const { return getGameData().getCCSMode(); }
        const char8_t* getCCSPool() const { return getGameData().getCCSPool(); }

        //! \brief return the game info (playerId and slotId) for the dedicated host.
        const HostInfo& getDedicatedServerHostInfo() const { return getGameData().getDedicatedServerHostInfo(); }

        //! \brief return the game info (playerId and slotId) for the topology host.
        const HostInfo& getTopologyHostInfo() const { return getGameData().getTopologyHostInfo(); }

        //! \brief return the game info (playerId and slotId) for the platform host.
        const HostInfo& getPlatformHostInfo() const { return getGameData().getPlatformHostInfo(); }

        //! \brief return the game dedicated host's UserSessionId
        UserSessionId getDedicatedServerHostSessionId() const { return getGameData().getDedicatedServerHostInfo().getUserSessionId(); }

        //! \brief return the game dedicated host's BlazeId
        BlazeId getDedicatedServerHostBlazeId() const { return getGameData().getDedicatedServerHostInfo().getPlayerId(); }

        //! \brief return the game topology host's UserSessionId
        UserSessionId getTopologyHostSessionId() const { return getGameData().getTopologyHostInfo().getUserSessionId(); }

        //! \brief return the game topology host's BlazeId
        BlazeId getTopologyHostBlazeId() const { return getGameData().getTopologyHostInfo().getPlayerId(); }

        //! \brief returns true if the game has a dedicated server host session that exists
        bool getDedicatedServerHostSessionExists() const;

        //! \brief returns true if the game has a topology host session that exists
        bool getTopologyHostSessionExists() const;

        //! \brief return the game host's user session info
        const UserSessionInfo& getTopologyHostUserInfo() const { return mGameData->getTopologyHostUserInfo(); }

        //! \brief return network address information
        const NetworkAddressList& getDedicatedServerHostNetworkAddressList() const { return getGameData().getDedicatedServerHostNetworkAddressList(); }

        //! \brief return network address information
        const NetworkAddressList& getTopologyHostNetworkAddressList() const { return getGameData().getTopologyHostNetworkAddressList(); }

        GameState getGameState() const { return getGameData().getGameState(); }

        //! \brief get the game's Settings (a collection of boolean settings: game entry modes, ranked mode, etc)
        const GameSettings& getGameSettings() const { return getGameData().getGameSettings(); }

        //! \brief return the value of the supplied game attribute name.  nullptr if no attrib found with that name.
        const char8_t* getGameAttrib(const char8_t* name) const;

        //! \brief return the value of the supplied game attribute name.  nullptr if no attrib found with that name.
        const char8_t* getMeshAttrib(const char8_t* name) const;

        //! \brief return the value of the supplied dedicated server attribute name.  nullptr if no attrib found with that name.
        const char8_t* getDedicatedServerAttrib(const char8_t* name) const;

        //! \brief return the value of the supplied game attribute name.  nullptr if no attrib found with that name.
        // Virtual because we need to access the component configuration
        virtual const char8_t* getGameMode() const = 0;
        virtual const char8_t* getPINGameModeType() const = 0;
        virtual const char8_t* getPINGameType() const = 0;
        virtual const char8_t* getPINGameMap() const = 0;

        //! \brief return the game's mod register
        const GameModRegister getGameModRegister() const { return getGameData().getGameModRegister(); }

        //! \brief returns the game's AttributeMap (A collection of title-defined name value pairs).
        const Collections::AttributeMap& getGameAttribs() const { return getGameData().getGameAttribs(); }

        //! \brief returns the game's mesh AttributeMap (A collection of title-defined name value pairs that don't change on game reset).
        const Collections::AttributeMap& getMeshAttribs() const { return getGameData().getMeshAttribs(); }

        //! \brief returns the dedicated server's AttributeMap (A collection of title-defined name value pairs).
        const Collections::AttributeMap& getDedicatedServerAttribs() const { return getGameData().getDedicatedServerAttribs(); }

        const char8_t* getCreateGameTemplate() const { return getGameData().getCreateGameTemplateName(); }

        //! \brief return the list of playerIds who have administrative rights for this game.
        const PlayerIdList& getAdminPlayerList() const { return getGameData().getAdminPlayerList(); }

        //! \brief return the persisted game id
        const char8_t* getPersistedGameId() const { return getGameData().getPersistedGameId(); }

        bool hasPersistedGameId() const { return getPersistedGameId()[0] != '\0'; }

        bool isServerNotResetable() const { return getGameData().getServerNotResetable(); }

        //! \brief return the game's current total player capacity.  The max number of players that are allowed into the game.
        uint16_t getTotalParticipantCapacity() const;
        uint16_t getTotalSpectatorCapacity() const;
        uint16_t getTotalPlayerCapacity() const;

        //! \brief return the game's current player capacity per slot type.
        const SlotCapacitiesVector& getSlotCapacities() const { return getGameData().getSlotCapacities(); }

        //! \brief return the game's current player capacity for a given slot type.
        uint16_t getSlotTypeCapacity( SlotType slotType ) const
        {
            if(EA_UNLIKELY((slotType >= MAX_SLOT_TYPE) || (slotType < 0)))
            {
                ERR_LOG("[GameSession].getSlotTypeCapacity - Game(" << getGameId() << ") requested slot type " << slotType << " is not defined in gamemanager.tdf.");
                return 0;
            }

            return getGameData().getSlotCapacities().at(slotType);
        }

        bool isSlotTypeFull(SlotType slotType) const
        {
            return (getPlayerRoster()->getPlayerCount(slotType) >= getSlotTypeCapacity(slotType));
        }

        // GM_AUDIT: can we deprecate this?
        bool isPasswordProtected() const { return false; }

        //! \brief return the game's current team list.
        const TeamIdVector& getTeamIds() const { return getGameData().getTeamIds(); }

        //! \brief return the team id for a given team index
        TeamId getTeamIdByIndex(const TeamIndex& teamIndex) const;

        //! \brief return the number of teams for the current game
        uint16_t getTeamCount() const { return (uint16_t)getGameData().getTeamIds().size(); }

        uint16_t getTeamCapacity() const;

        const EntryCriteriaMap& getEntryCriteriaMap() const { return getGameData().getEntryCriteriaMap(); } 
        const RoleInformation& getRoleInformation() const { return getGameData().getRoleInformation(); }

        //! \brief return the role capacity for the current game, 0 if the role isn't allowed in this game session
        uint16_t getRoleCapacity(const RoleName& roleName) const
            {   RoleCriteriaMap::const_iterator roleCritIter = getRoleInformation().getRoleCriteriaMap().find(roleName);
                return (roleCritIter != getRoleInformation().getRoleCriteriaMap().end()) ? roleCritIter->second->getRoleCapacity() : 0; }

        //! \brief return the game's total maximum possible player capacity.  A hard cap on the game's player capacity.
        uint16_t getMaxPlayerCapacity() const {return getGameData().getMaxPlayerCapacity(); }

        //! \brief return the game's total minimum possible player capacity.  A hard cap on the game's player capacity.
        uint16_t getMinPlayerCapacity() const {return getGameData().getMinPlayerCapacity(); }

        //! \breif return the queue capacity set for the game
        uint16_t getQueueCapacity() const { return getGameData().getQueueCapacity(); }

        //! \brief return the GameStatusURL
        const char8_t* getGameStatusUrl() const { return getGameData().getGameStatusUrl(); }
        const char8_t* getGameEventAddress() const { return getGameData().getGameEventAddress(); }
        const char8_t* getGameStartEventUri() const { return getGameData().getGameStartEventUri(); }
        const char8_t* getGameEndEventUri() const { return getGameData().getGameEndEventUri(); }
        const char8_t* getTournamentId() const { return getGameData().getTournamentIdentification().getTournamentId(); }
        const char8_t* getTournamentOrganizer() const { return getGameData().getTournamentIdentification().getTournamentOrganizer(); }

        const char8_t* getNpSessionId() const { return getGameData().getExternalSessionIdentification().getPs4().getNpSessionId(); }

        virtual const PlayerRoster *getPlayerRoster() const = 0;

        
        // The list of *currently* supported client platforms.  Dynamically updated as new players enter.
        const ClientPlatformTypeList& getCurrentlyAcceptedPlatformList() const { return getGameData().getCurrentlyAcceptedPlatformList(); }
        
        // The list of platforms supported by the Game, as set when the Game was created.  May be larger than the current unrestricted platforms/ClientPlatformList.
        // NOTE:  The Dedicated Server holds another platform list, which may be large than this.
        const ClientPlatformTypeList& getBasePlatformList() const { return getGameData().getBasePlatformList();  }
 
        bool isCrossplayEnabled() const { return getGameData().getIsCrossplayEnabled(); }
        bool getPINIsCrossplayGame() const { return getGameData().getPINIsCrossplayGame(); }

        // String generated from the Platforms allowed in a game session.
        eastl::string getPlatformForMetrics() const;
      
        /*! ************************************************************************************************/
        /*! \brief return the data center the game created

            \return the data center of the game
        ***************************************************************************************************/
        const char8_t* getBestPingSiteAlias() const { return getGameData().getPingSiteAlias(); }

        bool isResetable() const { return (getGameData().getGameState() == RESETABLE); }
        bool isInactiveVirtual() const { return (getGameData().getGameState() == INACTIVE_VIRTUAL); }

        bool isPseudoGame() const { return getGameData().getIsPseudoGame(); }

        TimeValue getPlayerReservationTimeout() const { return getGameData().getPlayerReservationTimeout(); }
        TimeValue getDisconnectReservationTimeout() const { return getGameData().getDisconnectReservationTimeout(); }

        /*! ************************************************************************************************/
        /*! \brief return the game's Network Quality of Service Data (firewall type, bandwidth, etc).

            Note: the NAT Type (firewall type) return value depends on the game's network topology.
            For star topologies (client server & p2p partial mesh), we simply return the current host's NAT type.
            For p2p full mesh games, however, we return an aggregate NAT type (the most restrictive NAT type of
                all the players in the game).

            \return the game's network quality of service data.
        ***************************************************************************************************/
        const Util::NetworkQosData& getNetworkQosData() const { return getGameData().getNetworkQosData(); }

        /*! ************************************************************************************************/
        /*! \brief returns true if this game session allows users to ignore the game entry criteria with an invite.
            \return true if this game session allows users to ignore the game entry criteria with an invite.
        *************************************************************************************************/
        bool getIgnoreEntryCriteriaWithInvite() const { return getGameData().getGameSettings().getIgnoreEntryCriteriaWithInvite(); }

        /*! ************************************************************************************************/
        /*! \brief returns true if this game is currently locked for preferred joins.
            \return  true if this game is currently locked for preferred joins.
        *************************************************************************************************/
        virtual bool isLockedForPreferredJoins() const { return mGameData != nullptr ? mGameData->getIsLockedForJoins() : false; }

        /*! ************************************************************************************************/
        /*! Implementing the EntryCriteriaEvaluator interface
            ************************************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief Accessor for the entry criteria for this entry criteria evaluator.  These are string
                representations of the entry criteria defined by the client or configs.

            \return EntryCriteriaMap* reference to the entry criteria for this entry criteria evaluator.
        *************************************************************************************************/
        const EntryCriteriaMap& getEntryCriteria() const override { return getGameData().getEntryCriteriaMap(); }

        /*! ************************************************************************************************/
        /*!
            \brief Accessor for the entry criteria expressions for this entry criteria evaluator.  Expressions
                are used to evaluate the criteria against a given user session.

            \return ExpressionMap* reference to the entry criteria expressions for this entry criteria evaluator.
        *************************************************************************************************/
        ExpressionMap& getEntryCriteriaExpressions() override { return mExpressionMap; }
        const ExpressionMap& getEntryCriteriaExpressions() const override { return mExpressionMap; }

        // return true if this is a single group game AND the supplied groupId is ok for the game
        bool isGroupAllowedToJoinSingleGroupGame(const UserGroupId& groupId) const;

        /*! ************************************************************************************************/
        /*! \brief return if the game's network behaves like a full mesh.

        \return if the game's network behaves like a full mesh.
        ***************************************************************************************************/
        bool isNetworkFullMesh() const { return (getGameNetworkTopology() == PEER_TO_PEER_FULL_MESH); }

        /*! ************************************************************************************************/
        /*! \brief return if the game has a dedicated game server
        
        \return if the game's network behaves like a full mesh.
        ***************************************************************************************************/
        bool hasDedicatedServerHost() const { return (isDedicatedHostedTopology(getGameNetworkTopology())); }

        /*! ************************************************************************************************/
        /*! \brief return the game info for the dedicated owner/host user session. 
        ***************************************************************************************************/
        const HostInfo& getExternalOwnerInfo() const { return getGameData().getExternalOwnerInfo(); }
        UserSessionId getExternalOwnerSessionId() const { return getExternalOwnerInfo().getUserSessionId(); }
        BlazeId getExternalOwnerBlazeId() const { return getExternalOwnerInfo().getPlayerId(); }

        bool hasExternalOwner() const { return (getExternalOwnerSessionId() != INVALID_USER_SESSION_ID); }

        /*! ************************************************************************************************/
        /*! \brief set up role-specific entry criteria
            NOTE: entry criteria setup will early-out if any of the criteria fails to process.

        \return ERR_OK if role-specific entry criteria was successfully parsed.
        ***************************************************************************************************/
        BlazeRpcError updateRoleEntryCriteriaEvaluators();

        void cleanUpRoleEntryCriteriaEvaluators();

        // need this so MM & GB can validate space availability for a set of joining roles
        BlazeRpcError checkJoinability(const TeamIndexRoleSizeMap &teamRoleSpaceRequirements) const;
        BlazeRpcError checkJoinabilityByTeamIndex(const RoleSizeMap &roleSpaceRequirements, const TeamIndex& teamIndex) const;
        // returns true if a team is found with the required slot count
        // discovered teamId & index are returned via foundTeamId & foundTeamIndex
        // if there are multiple open teams, supplies the least populated team in the game with enough room
        BlazeRpcError findOpenTeams(const TeamIdRoleSizeMap& requiredRoleSizes, TeamIdToTeamIndexMap& foundTeamIndexes) const;
        BlazeRpcError findOpenTeam(const TeamSelectionCriteria& teamSelectionCriteria, TeamIdRoleSizeMap::const_iterator requiredTeamIdRoleSizes, 
                                   const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions, TeamIndex& foundTeamIndex, TeamIndexSet* teamsToSkip = nullptr) const;
        

        UserExtendedDataValue calcTeamUEDValue(TeamIndex teamIndex, UserExtendedDataKey uedKey, GroupValueFormula groupFormula, const Matchmaker::GroupUedExpressionList &groupAdjustmentFormulaList, UserExtendedDataValue defaultValue = 0) const;
        bool doesGameViolateGameTeamCompositions(const GroupSizeCountMapByTeamList& gtcGroupSizeCountsByTeamList, uint16_t joiningSize, TeamIndex joiningTeamIndex = UNSPECIFIED_TEAM_INDEX) const;

        BlazeRpcError findOpenRole(const PlayerToRoleNameMap& playerRoles, TeamIndex joingingTeamIndex, TeamIndexRoleSizeMap& roleCounts) const;
        bool validateOpenRole(PlayerId playerId, TeamIndex joiningTeamIndex, const RoleCriteriaMap::const_iterator& roleCriteriaItr, TeamIndexRoleSizeMap& roleCounts) const;

        TeamIndexToTeamUedMap & getTeamUeds()
        {
            return mTeamUeds;
        }

        const TeamIndexToTeamUedMap & getTeamUeds() const
        {
            return mTeamUeds;
        }

    protected:

        GameSession(GameId gameId);
        GameSession(ReplicatedGameDataServer &replicatedGameSession);
        ~GameSession() override;
        ReplicatedGameData& getGameData() { return mGameData->getReplicatedGameData(); }
        const ReplicatedGameData& getGameData() const { return mGameData->getReplicatedGameData(); }
        BlazeRpcError findOpenTeam(const TeamSelectionCriteria& teamSelectionCriteria, const RoleSizeMap& requiredRoleSizes, TeamId requestedTeamId,
                                   const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions, TeamIndex& foundTeamIndex, TeamIndexSet* teamsToSkip = nullptr) const;

    protected:
        GameId mGameId; // intentionally cached separately(never changes), because GameId is needed by GameSessionMaster even after the replicator destroys the ReplicatedGameDataServer and sets it's pointer to nullptr!
        ExpressionMap mExpressionMap;
        ReplicatedGameDataServerPtr mGameData;

        typedef eastl::map<RoleName, RoleEntryCriteriaEvaluator*, CaseInsensitiveStringLessThan> RoleEntryCriteriaEvaluatorMap;
        RoleEntryCriteriaEvaluatorMap mRoleEntryCriteriaEvaluators;
        MultiRoleEntryCriteriaEvaluator mMultiRoleEntryCriteriaEvaluator;
        TeamIndexToTeamUedMap mTeamUeds; // latest calculated Team UEDs for PIN

    private:
        bool chooseBetweenOpenTeams(TeamIndex t1, TeamIndex t2, const TeamSelectionCriteria& teamSelectionCriteria, const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions) const;
        bool chooseBetweenOpenTeamsByNextPriority(TeamIndex t1, TeamIndex t2, const TeamSelectionCriteria& teamSelectionCriteria, const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions, size_t currPrioritiesIndex) const;
        bool chooseBetweenOpenTeamsByBalance(TeamIndex t1, TeamIndex t2, const TeamSelectionCriteria& teamSelectionCriteria, const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions, size_t currPrioritiesIndex) const;
        bool chooseBetweenOpenTeamsByUedBalance(TeamIndex t1, TeamIndex t2, const TeamSelectionCriteria& teamSelectionCriteria, const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions, size_t currPrioritiesIndex) const;
        bool chooseBetweenOpenTeamsByCompositions(TeamIndex t1, TeamIndex t2, const TeamSelectionCriteria& teamSelectionCriteria, const Matchmaker::GroupUedExpressionList* groupUedAdjustmentExpressions, size_t currPriorityQueueIndex) const;
        bool doesTeamViolateTeamComposition(TeamIndex teamIndex, const GroupSizeCountMap& teamCompositionGroupSizeCounts, uint16_t joiningSize) const;
    };

} //GameManager
} // Blaze

#endif // BLAZE_GAMEMANAGER_GAME_SESSION_H

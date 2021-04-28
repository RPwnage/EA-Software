/*! ************************************************************************************************/
/*!
    \file   matchmakinggameinfocache.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_GAME_INFO_CACHE_H
#define BLAZE_MATCHMAKING_GAME_INFO_CACHE_H

#include "gamemanager/playerinfo.h"
#include "gamemanager/matchmaker/rules/geolocationtypes.h"
#include "gamemanager/matchmaker/rules/teamcompositionruledefinition.h"
#include "gamemanager/rolescommoninfo.h" // for RoleSizeMap
#include "gamemanager/matchmakingfiltersutil.h" // for PropertyUEDIdentification in CachedGamePropertyUEDValuesMap

#include "EABase/eabase.h"
#include "EASTL/map.h"
#include "EASTL/vector_set.h"
#include "EASTL/sort.h"

namespace Blaze
{
namespace Search
{
    class GameSessionSearchSlave;
}
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    class MatchmakingSession;
    class TeamUEDBalanceRuleDefinition;
    class TeamUEDPositionParityRuleDefinition;
    class TeamCompositionRuleDefinition;
    class PlayerAttributeRuleDefinition;
    class GameAttributeRuleDefinition;
    class DedicatedServerAttributeRuleDefinition;
    class UEDRuleDefinition;
    class PropertyRuleDefinition;//for updateGamePropertyUEDValues()
    class CalcUEDUtil;
    
    typedef eastl::list<eastl::string> RuleNames;

    typedef Rete::WMEAttribute ArbitraryValue;
    typedef eastl::vector_set<ArbitraryValue> ArbitraryValueSet;
    //! see initDesiredValuesBitset for details.
    typedef eastl::bitset<MAX_EXPLICIT_ATTRIBUTE_RULE_POSSIBLE_VALUE_COUNT> GameAttributeRuleValueBitset;
    // The uint64_t here is used for either the explicit attribute index (64 indexes max), or the Arbitrary Rete::WMEAttribute value (requires uint64_t)
    typedef eastl::map<uint64_t, int> AttributeValuePlayerCountMap;
    typedef eastl::hash_map<eastl::string, uint64_t> TeamChoiceWMEMap;

    /*! ************************************************************************************************/
    /*!
        \brief This class is shared between GameManager::GameSession and the matchmaker/gamebrowser.  GameManager sets the
            dirty flags as a game's values change, and the matchmaker/gamebrowser updates cached values for game/player/DS
            attributes when it's idled.

        From gameManager's perspective, this class is simply a collection of flags representing which
        game values are 'dirty' (ie: settings that have changed).  As game values change, GameManager calls
        the various 'flag<something>Dirty' functions.

        From Matchmaker's/GameBrowser's perspective, this class contains the dirty flags described above as well as
        cached versions of the game's gameAttributes and playerAttributes.  Each idle loop, we
        updates the cached attributeValues for dirty games.
        
        Note: GameInfoCache is a member of GameSession so we can avoid having to keep/lookup a separate map
        of cache objects.

    ***************************************************************************************************/
    class MatchmakingGameInfoCache
    {
    private:
        NON_COPYABLE(MatchmakingGameInfoCache);

        // Flags for showing the cache is Dirty.  Add new values before ALL_DIRTY
        enum MatchmakingDirtyReason
        {
            ROSTER_DIRTY = 0x1,
            GAME_ATTRIBUTE_DIRTY = 0x2,
            PLAYER_ATTRIBUTE_DIRTY = 0x4,
            GEO_LOCATION_DIRTY = 0x8,
            TEAM_INFO_DIRTY = 0x10,
            DEDICATED_SERVER_ATTRIBUTE_DIRTY = 0x20,
            ALL_DIRTY = 0xFFFF
        };

    public:

        static const ArbitraryValue ATTRIBUTE_VALUE_NOT_DEFINED = UINT64_MAX;   //Attribute value not found
        static const int ATTRIBUTE_INDEX_NOT_DEFINED = -1; //!< No attribute exists with the supplied name (ie: attrib key not found)
        static const int ATTRIBUTE_INDEX_IMPOSSIBLE = -2; //!< The attribute index is not in the matchmaking rule defn's possible value list

        struct TeamIdInfo
        {
            TeamId mTeamId;
            TeamIndex mTeamIndex;
        };

        // teams are kept sorted by id in the cache for matchmaking purposes
        // eastl sort struct for TeamCapacitiesVector (sort by teamId)
        struct TeamInfoIdComparitor
        {
            bool operator() (const TeamIdInfo &a, const TeamIdInfo &b) const
            {
                if (a.mTeamId < b.mTeamId)
                {
                    return true;
                }
                else if((a.mTeamId == b.mTeamId) && (a.mTeamIndex < b.mTeamIndex))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        };

        typedef eastl::vector<TeamIdInfo> TeamIdInfoVector;
        typedef eastl::vector<TeamUEDVector> TeamIndividualUEDValues;

    public:
        //! \brief constructed with all dirty flags set (games are created dirty, as far as the matchmaker is concerned)
        MatchmakingGameInfoCache();
        ~MatchmakingGameInfoCache() {};

        //! \brief mark the game's gameAttributes dirty (matchmaker will re-build the cached gameAttrib values next idle)
        void setGameAttributeDirty() { mDirtyFlags |= GAME_ATTRIBUTE_DIRTY; }

        //! \brief mark the dedicated server's gameAttributes dirty (matchmaker will re-build the cached dedicatedServerAttrib values next idle)
        void setDedicatedServerAttributeDirty() { mDirtyFlags |= DEDICATED_SERVER_ATTRIBUTE_DIRTY; }

        /*! ************************************************************************************************/
        /*! \brief mark the game's playerAttributes dirty (matchmaker will re-build the cached playerAttrib values next idle)

            Note: use when only playerAttributes have changed (ie: not when a new player is added, since that affects the roster as well).
        ***************************************************************************************************/
        void setPlayerAttributeDirty() { mDirtyFlags |= PLAYER_ATTRIBUTE_DIRTY; }

        /*! ************************************************************************************************/
        /*! \brief mark the game's roster & playerAttributes as dirty (matchmaker will re-build the cached playerAttrib values next idle)

            Note: use when an active player is added/removed from the game
        ***************************************************************************************************/
        void setRosterDirty() { mDirtyFlags |= (PLAYER_ATTRIBUTE_DIRTY|ROSTER_DIRTY); }

        /*! ************************************************************************************************/
        /*! \brief mark geo location information dirty
        ***************************************************************************************************/
        void setGeoLocationDirty() { mDirtyFlags |= GEO_LOCATION_DIRTY; }

        /*! ************************************************************************************************/
        /*! \brief mark teaminformation dirty
        ***************************************************************************************************/
        void setTeamInfoDirty() { mDirtyFlags |= TEAM_INFO_DIRTY; }

        /*! ************************************************************************************************/
        /*! \brief return true if the GAME_ATTRIBUTE_DIRTY flag is set (ie: if setGameAttributeDirty was called).
            \return true if the GAME_ATTRIBUTE_DIRTY flag is set
        ***************************************************************************************************/
        bool isGameAttributeDirty() const { return (mDirtyFlags & GAME_ATTRIBUTE_DIRTY) != 0; }

        /*! ************************************************************************************************/
        /*! \brief return true if the DEDICATED_SERVER_ATTRIBUTE_DIRTY flag is set (ie: if setDedicatedServerAttributeDirty was called).
        \return true if the DEDICATED_SERVER_ATTRIBUTE_DIRTY flag is set
        ***************************************************************************************************/
        bool isDedicatedServerAttributeDirty() const { return (mDirtyFlags & DEDICATED_SERVER_ATTRIBUTE_DIRTY) != 0; }

        /*! ************************************************************************************************/
        /*! \brief return true if the PLAYER_ATTRIBUTE_DIRTY flag is set (ie: if setPlayerAttributeDirty was called).
            \return true if the PLAYER_ATTRIBUTE_DIRTY flag is set
        ***************************************************************************************************/
        bool isPlayerAttributeDirty() const { return (mDirtyFlags & PLAYER_ATTRIBUTE_DIRTY) != 0; }

        bool isRosterDirty() const { return ( (mDirtyFlags & ROSTER_DIRTY) != 0 ); }

        /*! ************************************************************************************************/
        /*! \brief returns true if the geo location flag has been marked dirty.
            \return true if the geo location flag has been marked dirty.
        ***************************************************************************************************/
        bool isGeoLocationDirty() const { return (mDirtyFlags & GEO_LOCATION_DIRTY) != 0; }

        /*! ************************************************************************************************/
        /*! \brief returns true if the team info flag has been marked dirty.
        \return true if the team info flag has been marked dirty.
        ***************************************************************************************************/
        bool isTeamInfoDirty() const { return (mDirtyFlags & TEAM_INFO_DIRTY) != 0; }

        /*! ************************************************************************************************/
        /*! \brief return true if any dirty flag is set
            \return true if any dirty flag is set
        ***************************************************************************************************/
        bool isDirty() const { return mDirtyFlags != 0; }

        /*! ************************************************************************************************/
        /*! \brief clear ALL dirty flags
        ***************************************************************************************************/
        void clearDirtyFlags() { mDirtyFlags = 0; }

        /*! ************************************************************************************************/
        /*! \brief clears ALL caches, setting ALL dirty flags. Used for invalidating the cache when the 
            game may not have changed, but our rules have(reconfiguration).
        ***************************************************************************************************/
        void clearRuleCaches();

        /*! ************************************************************************************************/
        /*! 
            \brief cache the possibleValueIndex for a particular gameAttribute.

            Looks up the game's value for the rule's attribute, determines the value's possibleValueIndex and
            caches the result.

            \param[in] gameSession the game to examine; note: 'this' is the game's matchmakingGameInfoCache
            \param[in] ruleDefn the matchmaking rule to cache (provides attributeName and possibleValueList)
        ***************************************************************************************************/
        void cacheGameAttributeIndex(const Search::GameSessionSearchSlave& gameSession, const GameAttributeRuleDefinition& ruleDefn);

        /*! ************************************************************************************************/
        /*!
        \brief cache the possibleValueIndex for a particular dedicatedServerAttribute.

        Looks up the dedicated server's value for the rule's attribute, determines the value's possibleValueIndex and
        caches the result.

        \param[in] gameSession the game to examine; note: 'this' is the game's matchmakingGameInfoCache
        \param[in] ruleDefn the matchmaking rule to cache (provides attributeName and possibleValueList)
        ***************************************************************************************************/
        void cacheDedicatedServerAttributeIndex(const Search::GameSessionSearchSlave& gameSession, const DedicatedServerAttributeRuleDefinition& ruleDefn);
        
        /*! ************************************************************************************************/
        /*!
            \brief cache the possibleValueIndex for a particular gameAttribute.

            Looks up the game's value for the rule's attribute, determines the value's possibleValueIndex and
            caches the result.

            \param[in] gameSession the game to examine; note: 'this' is the game's matchmakingGameInfoCache
            \param[in] ruleDefn the matchmaking rule to cache (provides attributeName and possibleValueList)
        ***************************************************************************************************/
        void cacheArbitraryGameAttributeIndex(const Search::GameSessionSearchSlave& gameSession, const GameAttributeRuleDefinition& ruleDefn);

        /*! ************************************************************************************************/
        /*! 
            \brief returns the cached possibleValueIndex for the supplied rule's gameAttribute (or a negative value indicating an error)

            Note: this object is a member of a GameSession instance, and cached values are for that game.

            \param[in] ruleDefn - the rule we're looking up a cached value for
            \return the cached possibleValueIndex for the supplied rule's attribute (< 0 indicates an error)
        ***************************************************************************************************/
        int getCachedGameAttributeIndex(const GameAttributeRuleDefinition& ruleDefn) const;

        /*! ************************************************************************************************/
        /*!
        \brief returns the cached possibleValueIndex for the supplied rule's dedicatedServerAttribute (or a negative value indicating an error)

        Note: this object is a member of a GameSession instance, and cached values are for that game.

        \param[in] ruleDefn - the rule we're looking up a cached value for
        \return the cached possibleValueIndex for the supplied rule's attribute (< 0 indicates an error)
        ***************************************************************************************************/
        int getCachedDedicatedServerAttributeIndex(const DedicatedServerAttributeRuleDefinition& ruleDefn) const;

        /*! ************************************************************************************************/
        /*! 
            \brief returns the cached ArbitraryValue for the supplied rule's gameAttribute.

            Note: this object is a member of a GameSession instance, and cached values are for that game.

            \param[in] ruleDefn - the rule we're looking up a cached value for
            \param[out] defined - true if found.
            \return the cached possibleValueIndex for the supplied rule's attribute (< 0 indicates an error)
        ***************************************************************************************************/
        ArbitraryValue getCachedGameArbitraryValue(const GameAttributeRuleDefinition& ruleDefn,
                bool& defined) const;

        /*! ************************************************************************************************/
        /*! 
            \brief build and cache a bitset representing all the possibleValueIndices for a particular playerAttribute

            For example, if player1 has "myAttrib=foo", and player2 has "myAttrib=baz", the following bitset
            is built (assuming the rule's possibleValues are "foo,bar,baz") : 1,0,1.
              Index 0 is set due to player1's attributeValue ("foo" has possibleValueIndex 0)
              Index 1 is unset, since no players have an attributeValue ("bar" has possibleValueIndex 1)
              Index 2 is set due to player2's attributeValue ("baz" has possibleValueIndex 2)

            \param[in] gameSession the game to examine; note: 'this' is the game's matchmakingGameInfoCache
            \param[in] ruleDefn the matchmaking rule to cache (provides attributeName and possibleValueList)
            \param[in] gameMembers other session memebers in the game (only valid when we would like to cache
                       matchmaking session's player attributes (session is not added to the game yet)
        ***************************************************************************************************/
        void cachePlayerAttributeValues(const Search::GameSessionSearchSlave& gameSession, const PlayerAttributeRuleDefinition& ruleDefn,
            const MatchmakingSessionList* memberSessions = nullptr);

        /*! ************************************************************************************************/
        /*! 
            \brief returns the cached ruleValueBitset for the supplied rule's playerAttribute.  The bitset
            represents the union of 'the playerAttributeValue's indices for the supplied rule'.

            Note: this object is a member of a GameSession instance, and cached values are for that game.

            \param[in] ruleDefn - the rule we're looking up a cached value for
            \return the cached possibleValueIndex for the supplied rule's attribute; return nullptr if no value was cached
        ***************************************************************************************************/
        const AttributeValuePlayerCountMap* getCachedPlayerAttributeValues(const PlayerAttributeRuleDefinition& ruleDefn) const;

        UserExtendedDataValue getCachedGameUEDValue(const UEDRuleDefinition& uedRuleDefinition) const;
        void updateGameUEDValue(const UEDRuleDefinition &uedRuleDefinition, const Search::GameSessionSearchSlave &gameSession);

        /*! ************************************************************************************************/
        /*! \brief cached (for speed) vector of teams UED values
        ***************************************************************************************************/
        const TeamUEDVector* getCachedTeamUEDVector(const TeamUEDBalanceRuleDefinition& uedRuleDefinition) const;
        void cacheTeamUEDValues(const TeamUEDBalanceRuleDefinition &uedRuleDefinition, const Search::GameSessionSearchSlave& gameSession);

        void sortCachedTeamIndivdualUEDVectors(const TeamUEDPositionParityRuleDefinition& uedRuleDefinition);
        const TeamIndividualUEDValues* getCachedTeamIndivdualUEDVectors(const TeamUEDPositionParityRuleDefinition& uedRuleDefinition) const;
        void cacheTeamIndividualUEDValues(const TeamUEDPositionParityRuleDefinition &uedRuleDefinition, const Search::GameSessionSearchSlave& gameSession);

        const TeamIdInfoVector& getCachedTeamIdInfoVector() const { return mSortedTeamIdInfoVector; }
        const TeamIdVector& getCachedTeamIds() const { return mSortedTeamIdVector; }

        void updateCachedTeamIds(const TeamIdVector& teamIdVector)
        {
            teamIdVector.copyInto(mSortedTeamIdVector);
            eastl::sort(mSortedTeamIdVector.begin(), mSortedTeamIdVector.end());

            mSortedTeamIdInfoVector.clear();
            mSortedTeamIdInfoVector.reserve(teamIdVector.size());

            for(TeamIndex i = 0; i < teamIdVector.size(); ++i)
            {
                mSortedTeamIdInfoVector.push_back_uninitialized();
                mSortedTeamIdInfoVector.back().mTeamId = teamIdVector[i];
                mSortedTeamIdInfoVector.back().mTeamIndex = i;
            }

            eastl::sort(mSortedTeamIdInfoVector.begin(), mSortedTeamIdInfoVector.end(), TeamInfoIdComparitor());

        }

        void cacheTopologyHostGeoLocation(const Search::GameSessionSearchSlave& gameSession);

        const GeoLocationCoordinate& getTopologyHostLocation() const { return mTopologyHostLocation; }

        TeamChoiceWMEMap& getCachedTeamChoiceRuleTeams() { return mCachedTeamIdWMEs; }

        RoleSizeMap& getRoleRuleAllowedRolesWMENameMap() { return mRoleRuleAllowedRolesWMENames; }

        RoleSizeMap& getTeamCompositionRuleWMENameList(const TeamCompositionRuleDefinition& ruleDefinition) { return mTeamCompositionRuleWMENamesMap[ruleDefinition.getID()]; }

    private:
        void setDirty() { mDirtyFlags = (ALL_DIRTY); }
        UserExtendedDataValue calcGameUEDValue(const CalcUEDUtil &calcUtil, const Search::GameSessionSearchSlave &gameSession) const;

    private:
        uint32_t mDirtyFlags;

        // we cache game attributes as a map of <RuleDefinitionId, possibleValueIndex>
        //   this caches off the possibleValueIndex (for each generic rule), for this game's attribs.
        typedef eastl::hash_map<RuleDefinitionId, int> ExplicitAttributeIndexMap; //!< map of 'ruleDefn pointers' to 'possibleValueIndex' of attrib value for the rule
        ExplicitAttributeIndexMap mExplicitGameAttribActualIndexMap;

        ExplicitAttributeIndexMap mExplicitDSAttribActualIndexMap;

        /////////////////////////////////////////////////////////////////////////////
        // below declarations mCreateGamePlayerAttributeBitset and mCreateGamePlayerAttributeValueCount are
        // only used for create game match making session
        //
        typedef eastl::hash_map<RuleDefinitionId, AttributeValuePlayerCountMap> PlayerAttributesAndIndexBitsetMap; //!< map of 'ruleDefn pointers' to AttributesAndIndexBitset structure
        PlayerAttributesAndIndexBitsetMap mRulesAndAttributesInfoMap;

        typedef eastl::hash_map<RuleDefinitionId, ArbitraryValue> ArbitraryValueMap;
        ArbitraryValueMap mArbitraryValueMap;

        // game skills are cached for each game
        typedef eastl::hash_map<RuleDefinitionId, UserExtendedDataValue> CachedGameUEDValuesMap;
        CachedGameUEDValuesMap mCachedGameUEDValuesMap; // cached skill values are indexed by the order of the rule's defn in the defnList

        // team skills are cached for each game. note: indices in values' lists are the team indices here.
        typedef eastl::hash_map<RuleDefinitionId, TeamUEDVector> CachedTeamUEDValuesMap;
        CachedTeamUEDValuesMap mCachedTeamUEDValuesMap; // cached skill values are indexed by the order of the rule's defn in the defnList

        // team skills are cached for each game. note: indices in values' lists are the team indices here.
        typedef eastl::hash_map<RuleDefinitionId, TeamIndividualUEDValues> CachedTeamIndividualUEDValuesMap;
        CachedTeamIndividualUEDValuesMap mCachedTeamIndividualUEDValuesMap;
        
        GeoLocationCoordinate mTopologyHostLocation;

        TeamIdInfoVector mSortedTeamIdInfoVector;
        TeamIdVector mSortedTeamIdVector;

        TeamChoiceWMEMap mCachedTeamIdWMEs;
        RoleSizeMap mRoleRuleAllowedRolesWMENames;

        typedef eastl::hash_map<RuleDefinitionId, RoleSizeMap> TeamCompositionRuleWMENamesMap;
        TeamCompositionRuleWMENamesMap mTeamCompositionRuleWMENamesMap;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_GAME_INFO_CACHE_H

/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef GAME_PACKER_GAME_H
#define GAME_PACKER_GAME_H

#include "gamepacker/gametemplate.h"
#include "gamepacker/property.h"
#include <EASTL/set.h>
#include <EASTL/map.h>

namespace Packer
{

struct GameScoreLessThan
{
    bool operator()(struct Game* a, struct Game* b) const;
};

typedef eastl::vector<Score> ScoreList;
typedef eastl::multimap<int64_t, struct Game*> ViableGameQueue;
typedef eastl::multiset<struct Game*, GameScoreLessThan> GameScoreSet;

struct GamePlayer
{
    PlayerId mPlayerId = 0;
    PartyIndex mGamePartyIndex = 0;
};

typedef eastl::vector<GamePlayer> GamePlayerList;

struct GameParty
{
    PartyId mPartyId = 0;
    uint16_t mPlayerCount = 0;
    uint16_t mPlayerOffset = 0;
    bool mImmutable = false; // whether the party can have its game or team assignment modified
};

typedef eastl::vector<GameParty> GamePartyList;

struct GameTeam
{
    uint16_t mPlayerCount = 0;
};

struct GameInternalState
{
    uint16_t mPartyCount = 0; // number of parties assigned to this game
    uint16_t mPlayerCount = 0; // number of players assigned to this game
    GameTeam mGameTeams[MAX_TEAM_COUNT]; // limit: MAX_TEAM_COUNT
    int16_t mTeamIndexByParty[MAX_PARTY_COUNT + 1]; // needs to accomodate one more party than max full game capacity
    PartyBitset mPartyBits; // parties assigned to the game in this state (mostly useful for tracking evicted parties)

    GameInternalState();
    void clear();
    void assignPartyToGame(int32_t partyIndex, int32_t playerCount);
    void unassignPartyToGame(int32_t partyIndex, int32_t playerCount);
    void assignPartyToTeam(int32_t partyIndex, int32_t playerCount, int32_t teamIndex);
    void unassignPartyToTeam(int32_t partyIndex, int32_t playerCount, int32_t teamIndex);
};

struct PropertyValueLookup
{
    const FieldValueColumn& mValueColumn;
    const GamePartyList& mGameParties;
    PropertyValueLookup(const FieldValueColumn& column, const GamePartyList& gameParties);
    bool hasPlayerPropertyValue(PlayerIndex playerIndex) const;
    PropertyValue getPlayerPropertyValue(PlayerIndex playerIndex) const;
    PropertyValue getPartyPropertyValue(PartyIndex partyIndex, PropertyAggregate aggr) const;
};

struct Party;

// PACKER_MAYBE: Add a GameEvalContext passed into activeEval/passiveEval instead of EvalContext + game. GameEvalContext would only have mGame and mEvalContext vars and dispatches all method calls to the underlying game object. This way we can operate on it as a game without having to specify which state context we are operating under, and having to override state in the transforms...

struct Game
{
    GameId mGameId = INVALID_GAME_ID;
    GameInternalState mInternalState[(int32_t)GameState::E_COUNT];
    GamePartyList mGameParties;
    GamePlayerList mGamePlayers;
    ScoreList mFactorScores; // game rank at last accepted improvement
    ScoreList mScoreDeltas; // game score estimated improvement deltas

    // PACKER_TODO: get rid of mFactorFields, we should always be able to reevaluate it again when its time to reap the game rather than paying memory allocation overhead during evaluation for all cases even when the values are not needed.

    FieldDescriptorList mFactorFields; // best Index as found by the quality factors (randomly selected if multiple have the same score)
    // Limits this factor to optimizing a subset of the fields associated with the bound input property. If empty, no constraints apply.

    // PACKER_TODO: Need to implement value participation count derived from FieldValueColumn.mSetBits.count()/mPropertyValues.size() which determines what is the fraction of values that is actually meaningful.

    FieldValueTable mFieldValueTable; // columns of values associated with property/field pairs referenced by all the elements(party/player) in this game

    PackerSilo& mPacker;

    int32_t mEvalRandomSeed = 0; // used for tie breaking scores
    GameIndex mGameWorkingSetIndex = INVALID_GAME_INDEX; // index of the game in the top level working set, used to help shrink the working set of games that have been retired, @MAYBE: move this into a separate inverted lookup index list
    int64_t mCreatedTimestamp = 0; // absolute game creation time, also used to calculate the absolute time deadline wherein the game can still be improved before it is reaped (if viable)
    int64_t mEvictedCount = 0; // total number of times parties have been evicted from this game
    int64_t mImprovedCount = 0; //(write/only) number of times the game was improved since creation
    int64_t mImprovedTimestamp = 0; //(write/only) last absolute time the game was improved since creation
    int64_t mViableTimestamp = 0; //(write/only) first absolute time the game became viable since creation
    int64_t mImprovementExpiryDeadline = 0; // absolute time deadline wherein the game can still be improved before it is reaped (if viable)
    ViableGameQueue::iterator mViableGameQueueItr; // game position in the viability queue ordered by improvement expiry deadline
    GameScoreSet::iterator mGameByScoreItr; // game position by score

    Game(PackerSilo& packer);
    ~Game();
    int32_t getGameCapacity() const;
    int32_t getTeamCapacity() const;
    int32_t getTeamCount() const;
    PartyIndex getIncomingPartyIndex() const;
    int32_t getPartyCount(GameState state = GameState::PRESENT) const;
    int32_t getPlayerCount(GameState state = GameState::PRESENT) const;
    int32_t getTeamPlayerCount(GameState state, TeamIndex teamIndex) const;
    TeamIndex getTeamIndexByPartyIndex(PartyIndex partyIndex, GameState state) const;
    bool isPartyAssignedToGame(PartyIndex partyIndex, GameState state) const;
    void evictPartyFromGame(PartyIndex partyIndex, GameState state);
    void copyState(GameState toState, GameState fromState);
    void copyState(GameState toState, const GameInternalState& fromState);
    void copyState(GameInternalState& toState, GameState fromState);
    const GameInternalState& getStateByContext(const EvalContext& context) const;
    GameInternalState& getFutureState() { return mInternalState[(uint32_t)GameState::FUTURE]; }
    GameInternalState& getPresentState() { return mInternalState[(uint32_t)GameState::PRESENT]; }
    void addPartyFields(const Party& party);
    void commitPartyFields(const Party& party);
    void removePartyFields(const Party& party);
    const PropertyValueIndex& getPlayerPropertyIndex(const EvalContext& context) const;
    const PropertyValueLookup getPropertyValueLookup(const EvalContext& context) const;
    bool isViable() const;
    void rebuildIndexes();
    void updateGameScoreSet();
    void removeFromScoreSet();
    bool hasCompatibleFields(const Party& party) const;
};

typedef eastl::hash_map<GameId, Game> GameMap;

}

#endif
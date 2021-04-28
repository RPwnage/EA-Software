/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef GAME_PACKER_H
#define GAME_PACKER_H

#include <EABase/eabase.h>
#include <EASTL/map.h>
#include "gamepacker/common.h"
#include "gamepacker/property.h"
#include "gamepacker/qualityfactor.h"
#include "gamepacker/gametemplate.h"
#include "gamepacker/player.h"
#include "gamepacker/party.h"
#include "gamepacker/game.h"


namespace Packer
{

typedef eastl::map<int64_t, int64_t> WorkingSetSizeMap;

struct PackerUtilityMethods;

struct PackerSilo
{
    PackerSilo();
    ~PackerSilo();
    PackerSilo(const PackerSilo&) = delete;
    PackerSilo& operator=(const PackerSilo&) = delete;
    void setUtilityMethods(PackerUtilityMethods& methods);
    PackerUtilityMethods* getUtilityMethods();
    void setPackerSiloId(PackerSiloId packerSiloId);
    void setParentObject(void* parentObject);
    void setPackerName(const char8_t* packerName);
    void setPackerDetails(const char8_t* details);
    Time packGames(struct PackGameHandler* handler);
    void reapGames(struct ReapGameHandler* handler);
    const PropertyDescriptor& addProperty(const eastl::string& propertyName);
    const FieldDescriptor& addPropertyField(const eastl::string& propertyName, const eastl::string& fieldName);
    const FieldDescriptor* getPropertyField(const eastl::string& propertyName, const eastl::string& fieldName);
    bool removePropertyField(PropertyFieldId fieldId);
    auto getPropertyFieldCount() const { return mFieldDescriptorById.size(); }
    auto& getPropertyDescriptors() const { return mPropertyDescriptorByName; }
    auto& getPropertyFieldDescriptors() const { return mFieldDescriptorById; }
    bool addFactor(const GameQualityFactorConfig& packerConfig);
    void setEvalSeedMode(GameEvalRandomSeedMode mode);
    void setMaxWorkingSet(uint32_t maxWorkingSet);
    ConfigVarValue getVar(GameTemplateVar var) const;
    void setVar(GameTemplateVar var, ConfigVarValue value);
    bool setVarByName(const char8_t* varName, ConfigVarValue value);
    // PACKER_TODO: Change to be a utility function, because this makes it
    // more easily reconfigurable without having to update all the packer silos,
    // or else we can have this value live in the GameTemplate and have that be 
    // shared by multiple PackerSilos and updated once on reconfigure...
    void setViableGameCooldownThreshold(int64_t threshold);
    float evalVarExpression(const char8_t* expression) const;
    bool addPackerParty(PartyId partyId, Time creationTime = 0, Time expiryTime = 0, bool immutable = false);
    bool eraseParty(PartyId party);
    void unpackGameAndRequeueParties(GameId gameId);
    void updatePackerPartyPriority(Party& party);
    Time getNextReapDeadline() const;
    void* getParentObject() const;
    const eastl::string& getPackerName() const;
    const eastl::string& getPackerDetails() const;
    const Game* getPackerGame(GameId gameId) const;
    Party* getPackerParty(PartyId partyId);
    const Party* getPackerParty(PartyId partyId) const;
    uint64_t getPackerPartiesSince(const Party& party) const;
    PartyPlayerIndex addPackerPlayer(PlayerId playerId, Party& party);
    Player* getPackerPlayer(PlayerId playerId);
    const Player* getPackerPlayer(PlayerId playerId) const;
    bool isIdealGame(const Game& game) const;
    void setDebugFactorMode(uint32_t mode);
    bool isReportEnabled(ReportMode mode) const;
    void setReport(ReportMode mode, bool enable = true);
    void reportLog(ReportMode mode, const char8_t* fmt, ...);
    void traceLog(TraceMode mode, const char8_t* fmt, ...);
    void errorLog(const char8_t* fmt, ...);
    PackerSiloId getPackerSiloId() const { return mPackerSiloId; }
    const auto& getDebugGameWorkingSetFreq() const { return mDebugGameWorkingSetFreq; }
    const auto& getGames() const { return mGames; }
    const auto& getParties() const { return mParties; }
    const auto& getPlayers() const { return mPlayers; }
    const auto& getViableGameQueue() const { return mViableGameQueue; }
    const auto& getPartyExpiryQueue() const { return mPartyExpiryQueue; }
    const auto& getGameByScoreSet() const { return mGameByScoreSet; }
    const auto& getFactors() const { return mGameTemplate.mFactors; }
    auto getEvictedPartiesCount() const { return mEvictedPartiesCount; }
    auto getEvictedPlayersCount() const { return mEvictedPlayersCount; }
    auto getTotalPlayersCount() const { return mNewPlayerCount; }
    auto getTotalPartiesCount() const { return mNewPartyCount; }
    auto getTotalGamesCreatedCount() const { return mNewGameCount; }
    auto getTotalReapedViableCount() const { return mReapedViableGameCount; }
    auto getFieldsCreatedCount() const { return mNewFieldCount; }
    auto getDebugFactorMode() const { return mDebugFactorMode; }
    auto getDebugFoundOptimalMatchCount() const { return mDebugFoundOptimalMatchCount; }
    auto getDebugFoundOptimalMatchDiscardCount() const { return mDebugFoundOptimalMatchDiscardCount; }
    bool isPacking() const { return mBeginPackCount > mEndPackCount; }
    bool isReaping() const { return mBeginReapCount > mEndReapCount; }
    bool hasPendingParties() const { return !mPartyPriorityQueue.empty(); }

private:
    friend struct GameQualityFactor;
    friend struct Game;

    GameId findOptimalGame(const Party& incomingParty, const GameIdList& gameWorkingSet, const GameQualityFactorIndex factorIndex);
    void retireGameFromWorkingSet(Game& retiredGame);
    void assignPartyMembersToGame(const Party& incomingParty, Game& newGame);
    void linkPartyToGame(Party& party, Game& game);
    void unlinkPartyToGame(Party& party);
    int32_t formatGame(eastl::string& buf, const Game& game, const char8_t* operation) const;
    int32_t formatParty(eastl::string& buf, const Game& game, PartyIndex partyIndex, const char8_t* operation, GameState state) const;
    int32_t formatScore(eastl::string& buf, GameQualityFactorIndex factorIndex, Score curScore, Score estimateScore, const char8_t* resultReason) const;

private:
    // [core vars]
    void* mParentObject = nullptr; // optional reference to the owner of the packer that can be set at packer creation time
    eastl::string mPackerName;
    eastl::string mPackerDetails;
    eastl::hash_map<eastl::string, PropertyDescriptor, caseless_hash, caseless_eq> mPropertyDescriptorByName;
    eastl::hash_map<PropertyFieldId, FieldDescriptor*> mFieldDescriptorById;
    PackerSiloId mPackerSiloId = INVALID_PACKER_SILO_ID;
    PackerUtilityMethods* mUtilityMethods = nullptr;
    GameTemplate mGameTemplate;
    GameMap mGames;
    PartyMap mParties;
    PlayerMap mPlayers;
    GameScoreSet mGameByScoreSet;
    GameIdList mGamesWorkingSet;
    ViableGameQueue mViableGameQueue; // Viable games sorted by the expiry time of their last viable update. If a game has not been improved in gameTemplate.maxInactiveThreshold while remaining viable, it is emitted by the packer.
    PartyTimeQueue mPartyExpiryQueue;
    PartyTimeQueue mPartyPriorityQueue;
    GameEvalRandomSeedMode mEvalRandomSeedMode = GameEvalRandomSeedMode::ACTIVE_EVAL;
    uint32_t mReportModeBits = 0;
    uint32_t mTraceModeBits = 0;
    uint32_t mMaxWorkingSet = 0; // 0 == unbounded
    uint64_t mNewGameCount = 0; // total number of games created
    uint64_t mNewPartyCount = 0; // total number of new parties added to this packer instance(handles reinserted parties without double counting)
    uint64_t mNewPlayerCount = 0; // total number of new players added to this packer instance(handles reinserted players without double counting)
    uint64_t mNewFieldCount = 0; // total number of fields created
    uint64_t mBeginPackCount = 0;
    uint64_t mEndPackCount = 0;
    uint64_t mBeginReapCount = 0;
    uint64_t mEndReapCount = 0;
    Time mNewGameTimestamp = 0; // tracks mNewGameCount increment time
    Time mNewPartyTimestamp = 0; // tracks mNewPartyCount increment time
    uint64_t mEvictedPartiesCount = 0;
    uint64_t mEvictedPlayersCount = 0;
    uint64_t mReapedViableGameCount = 0; // PACKER_TODO: Maybe we want to remove this entirely, callbacks should provide this info! initialize and add more metrics, mReapedGameCount, etc.
    uint32_t mDebugFactorMode = 0; // used to select various factor settings (such as different functions)
    uint64_t mDebugFoundOptimalMatchCount = 0;
    uint64_t mDebugFoundOptimalMatchDiscardCount = 0; // tracks how many matches that were found that were subsequently rejected by the validating evaluation
    WorkingSetSizeMap mDebugGameWorkingSetFreq;
};

GameQualityFactorSpecList& getAllFactorSpecs();

}

#endif
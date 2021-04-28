
#include <EASTL/sort.h>
#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/packer.h"
#include "gamepacker/game.h"

namespace Packer
{


Game::Game(PackerSilo& packer)
    : mPacker(packer)
{
    mFieldValueTable.mTableType = FieldValueTable::ALLOW_ADD_REMOVE;
}

Game::~Game()
{
    removeFromScoreSet();
}

int32_t Game::getGameCapacity() const
{
    return mPacker.mGameTemplate.getVar(GameTemplateVar::GAME_CAPACITY);
}

int32_t Game::getTeamCapacity() const
{
    return mPacker.mGameTemplate.getVar(GameTemplateVar::TEAM_CAPACITY);
}

int32_t Game::getTeamCount() const
{
    return mPacker.mGameTemplate.getVar(GameTemplateVar::TEAM_COUNT);
}

PartyIndex Game::getIncomingPartyIndex() const
{
    PartyIndex index = getPartyCount(GameState::PRESENT);
    EA_ASSERT(isPartyAssignedToGame(index, GameState::FUTURE)); // incoming party cannot yet be assigned to present state!
    return index;
}

int32_t Game::getPartyCount(GameState state) const
{
    return (int32_t)mInternalState[(int32_t)state].mPartyCount;
}

int32_t Game::getPlayerCount(GameState state) const
{
    return (int32_t)mInternalState[(int32_t)state].mPlayerCount;
}

int32_t Game::getTeamPlayerCount(GameState state, TeamIndex teamIndex) const
{
    return mInternalState[(int32_t)state].mPlayerCountByTeam[teamIndex];
}

void Game::rebuildIndexes()
{
    mFieldValueTable.rebuildIndexes();
}

void Game::updateGameScoreSet()
{
    if (mGameByScoreItr.mpNode != nullptr)
    {
        mPacker.mGameByScoreSet.erase(mGameByScoreItr);
    }
    mGameByScoreItr = mPacker.mGameByScoreSet.insert(this);
}

void Game::removeFromScoreSet()
{
    if (mGameByScoreItr.mpNode != nullptr)
    {
        mPacker.mGameByScoreSet.erase(mGameByScoreItr);
        mGameByScoreItr = GameScoreSet::iterator();
    }
}

bool Game::hasCompatibleFields(const Party & party) const
{
    return mFieldValueTable.hasCommonFields(party.mFieldValueTable);
}

const GameTemplate& Game::getGameTemplate() const
{
    return mPacker.getGameTemplate();
}

TeamIndex Game::getTeamIndexByPartyIndex(PartyIndex partyIndex, GameState state) const
{
    return mInternalState[(uint32_t)state].mTeamIndexByParty[partyIndex];
}

bool Game::isPartyAssignedToGame(PartyIndex partyIndex, GameState state) const
{
    return mInternalState[(uint32_t)state].mPartyBits.test(partyIndex);
}

void Game::evictPartyFromGame(PartyIndex partyIndex, GameState state)
{
    const GameParty& gameParty = mGameParties[partyIndex];
    EA_ASSERT_MSG(!gameParty.mImmutable, "Immutable parties cannot be evicted!");
    auto& internalState = mInternalState[(uint32_t)state];
    internalState.unassignPartyToTeam(partyIndex, gameParty.mPlayerCount);
    internalState.unassignPartyToGame(partyIndex, gameParty.mPlayerCount);
}

void Game::markPartyFieldsForRemoval(PartyIndex partyIndex)
{
    const GameParty& gameParty = mGameParties[partyIndex];
    auto* party = mPacker.getPackerParty(gameParty.mPartyId);
    EA_ASSERT(party);
    mFieldValueTable.markTableForRemoval(party->mFieldValueTable);
}

void Game::copyState(GameState toState, GameState fromState)
{
    if (toState != fromState)
        memcpy(mInternalState + (uint32_t)toState, mInternalState + (uint32_t)fromState, sizeof(mInternalState[0]));
}

void Game::copyState(GameState toState, const GameInternalState& fromState)
{
    if (mInternalState + (uint32_t) toState != &fromState)
        memcpy(mInternalState + (uint32_t)toState, &fromState, sizeof(mInternalState[0]));
}

void Game::copyState(GameInternalState& toState, GameState fromState)
{
    if (mInternalState + (uint32_t)fromState != &toState)
        memcpy(&toState, mInternalState + (uint32_t)fromState, sizeof(mInternalState[0]));
}

const GameInternalState & Game::getStateByContext(const EvalContext & context) const
{
    return mInternalState[(uint32_t)context.mGameState];
}

void Game::addPartyFields(const Party & party)
{
    bool omitPartyFieldsNotInGame = mImmutable && !party.mImmutable;
    mFieldValueTable.addTable(party.mFieldValueTable, omitPartyFieldsNotInGame);
}

void Game::commitPartyFields(const Party & party)
{
    mFieldValueTable.commitTable(party.mFieldValueTable);
}

void Game::removePartyFields(const Party& party)
{
    for (auto rit = mGameParties.rbegin(), rend = mGameParties.rend(); rit != rend; ++rit)
    {
        if (rit->mPartyId == party.mPartyId)
        {
            mFieldValueTable.removeTable(party.mFieldValueTable, rit->mPlayerOffset);
            return;
        }
    }
}

const PropertyValueIndex& Game::getPlayerPropertyIndex(const EvalContext & context) const
{
    return context.mCurFieldValueColumn->mPropertyValueIndex;
}

const PropertyValueLookup Game::getPropertyValueLookup(const EvalContext & context) const
{
    return PropertyValueLookup(*context.mCurFieldValueColumn, mGameParties);
}

bool Game::isViable() const
{
    return mViableGameQueueItr.mpNode != nullptr;
}

bool GameScoreLessThan::operator()(struct Game* a, struct Game* b) const
{
    // PACKER_TODO: Need to make score comparison handle factors with 
    // A) variable participation scores (once support is added)
    // B) unknown scores (once we support those)
    auto& factors = a->mPacker.getFactors();
    for (size_t i = 0, s = factors.size(); i < s; ++i)
    {
        if (factors[i].mScoreOnlyFactor)
            continue;
        if (a->mFactorScores[i] < b->mFactorScores[i])
            return true;
        else if (a->mFactorScores[i] > b->mFactorScores[i])
            return false;
    }
    return false;
}

GameInternalState::GameInternalState()
{
    memset(mTeamIndexByParty, INVALID_TEAM_INDEX, sizeof(mTeamIndexByParty));
    memset(mPlayerCountByTeam, 0, sizeof(mPlayerCountByTeam));
}

void GameInternalState::clear()
{
    mPartyBits.reset();
    memset(mTeamIndexByParty, INVALID_TEAM_INDEX, sizeof(mTeamIndexByParty));
    memset(mPlayerCountByTeam, 0, sizeof(mPlayerCountByTeam));
    mPartyCount = mPlayerCount = 0;
}

void GameInternalState::assignPartyToGame(int32_t partyIndex, int32_t playerCount)
{
    if (!mPartyBits.test(partyIndex))
    {
        mPartyBits.set(partyIndex, true);
        mPartyCount++;
        mPlayerCount += (uint16_t)playerCount;
    }
}

void GameInternalState::unassignPartyToGame(int32_t partyIndex, int32_t playerCount)
{
    if (mPartyBits.test(partyIndex))
    {
        EA_ASSERT_MSG(mTeamIndexByParty[partyIndex] == INVALID_TEAM_INDEX, "Must unassign from team first!");
        mPartyBits.set(partyIndex, false);
        mPartyCount--;
        mPlayerCount -= (uint16_t)playerCount;
    }
}

void GameInternalState::assignPartyToTeam(int32_t partyIndex, int32_t playerCount, int32_t teamIndex)
{
    EA_ASSERT(mTeamIndexByParty[partyIndex] == INVALID_TEAM_INDEX);
    {
        mTeamIndexByParty[partyIndex] = (int16_t)teamIndex;
        mPlayerCountByTeam[teamIndex] += (uint16_t)playerCount;
    }
}

void GameInternalState::unassignPartyToTeam(int32_t partyIndex, int32_t playerCount)
{
    auto teamIndex = mTeamIndexByParty[partyIndex];
    EA_ASSERT(teamIndex != INVALID_TEAM_INDEX);
    {
        mTeamIndexByParty[partyIndex] = (int16_t)INVALID_TEAM_INDEX;
        mPlayerCountByTeam[teamIndex] -= (uint16_t)playerCount;
    }
}

uint32_t GameInternalState::getPlayersAssignedToTeams(int32_t teamCount)
{
    uint32_t countPlayers = 0;
    for (int32_t i = 0; i < teamCount; ++i)
    {
        countPlayers += mPlayerCountByTeam[i];
    }
    return countPlayers;
}

PropertyValueLookup::PropertyValueLookup(const FieldValueColumn & column, const GamePartyList & gameParties) 
    : mValueColumn(column), mGameParties(gameParties) 
{
}

bool PropertyValueLookup::hasPlayerPropertyValue(PlayerIndex playerIndex) const
{
    return mValueColumn.mSetValueBits.test(playerIndex);
}

PropertyValue PropertyValueLookup::getPlayerPropertyValue(PlayerIndex playerIndex) const
{
    return mValueColumn.mPropertyValues[playerIndex];
}

PropertyValue PropertyValueLookup::getPartyPropertyValue(PartyIndex partyIndex, PropertyAggregate aggr) const
{
    PropertyValue aggregateValue = 0;
    const auto& gameParty = mGameParties.at(partyIndex);
    int32_t playerIndex = gameParty.mPlayerOffset;
    const int32_t endPlayerIndex = gameParty.mPlayerOffset + gameParty.mPlayerCount;
    const auto& fieldValueColumn = mValueColumn;
    EA_ASSERT(endPlayerIndex <= fieldValueColumn.mPropertyValues.size());
    switch (aggr)
    {
    case PropertyAggregate::SUM:
    {
        for (; playerIndex < endPlayerIndex; ++playerIndex)
        {
            aggregateValue += fieldValueColumn.mPropertyValues[playerIndex];
        }
        break;
    }
    case PropertyAggregate::MIN:
        aggregateValue = INT64_MAX;
        for (; playerIndex < endPlayerIndex; ++playerIndex)
        {
            auto propValue = fieldValueColumn.mPropertyValues[playerIndex];
            if (propValue < aggregateValue)
                aggregateValue = propValue;
        }
        break;
    case PropertyAggregate::MAX:
        aggregateValue = INT64_MIN;
        for (; playerIndex < endPlayerIndex; ++playerIndex)
        {
            auto propValue = fieldValueColumn.mPropertyValues[playerIndex];
            if (propValue > aggregateValue)
                aggregateValue = propValue;
        }
        break;

    case PropertyAggregate::SIZE:
        aggregateValue = endPlayerIndex - playerIndex;
        break;
    case PropertyAggregate::AVERAGE:
    {
        aggregateValue = 0;
        PropertyValue entryCount = 0;
        for (; playerIndex < endPlayerIndex; ++playerIndex)
        {
            aggregateValue += fieldValueColumn.mPropertyValues[playerIndex];
            ++entryCount;
        }
        aggregateValue /= entryCount;
        break;
    }
    case PropertyAggregate::STDDEV:
    {
        PropertyValue avgValue = 0;
        PropertyValue entryCount = 0;
        for (; playerIndex < endPlayerIndex; ++playerIndex)
        {
            avgValue += fieldValueColumn.mPropertyValues[playerIndex];
            ++entryCount;
        }
        avgValue /= entryCount;

        aggregateValue = 0;
        for (; playerIndex < endPlayerIndex; ++playerIndex)
        {
            auto propValue = fieldValueColumn.mPropertyValues[playerIndex];
            aggregateValue += (propValue > avgValue) ? (propValue - avgValue) : (avgValue - propValue);
        }
        aggregateValue /= entryCount;
        break;
    }
    case PropertyAggregate::MINMAXRANGE:
    {
        PropertyValue minValue = INT64_MAX;
        PropertyValue maxValue = INT64_MIN;
        for (; playerIndex < endPlayerIndex; ++playerIndex)
        {
            auto propValue = fieldValueColumn.mPropertyValues[playerIndex];
            if (propValue < minValue)
                minValue = propValue;
            if (propValue > maxValue)
                maxValue = propValue;
        }
        aggregateValue = maxValue - minValue;
        break;
    }
    case PropertyAggregate::MINMAXRATIO:
    {
        PropertyValue minValue = INT64_MAX;
        PropertyValue maxValue = INT64_MIN;
        for (; playerIndex < endPlayerIndex; ++playerIndex)
        {
            auto propValue = fieldValueColumn.mPropertyValues[playerIndex];
            if (propValue < minValue)
                minValue = propValue;
            if (propValue > maxValue)
                maxValue = propValue;
        }
        if (maxValue != 0)
            aggregateValue = minValue / maxValue;
        break;
    }
    default:
        EA_FAIL_MSG("Unsupported Property Aggregate Type");
    }
    return aggregateValue;
}

} // GamePacker


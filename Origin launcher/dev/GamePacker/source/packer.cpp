/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include <EAStdC/EACType.h>
#include <EAStdC/EAString.h>
#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/handlers.h"
#include "gamepacker/packer.h"

/**
MAJOR TODOS:
* Add support for Parties (groups of players matched into game together)
    Testing idea: Once party functionality is implemented we test its robustness by 
    1. running the algo on a set of individual players
    2. use the final result to construct some parties
    3. feed the parties + remaining individuals into the new mixed mode algo to see if it can construct the same idealized games 
    (with skill imbalance no worse than before), ditto check aggregate imbalance.

* Add support for roles
* Add support for respecting team and role assignments
* Add support for factor minViableGame conditionals
* Add support for respecting expiry timeout to avoid starvation when replacing players
* Add support for immovable Parties
*/

// code line count: find2 . -name '*.cpp' -exec cat {} \; | wc -l

/**
NOTES:
- Tried to use eastl/chrono.h for high res timer, but it doesnt seem to make use of QueryPerfFrequency which leads it to be incorrect 
(unlike the standard library version)! Therefore we had to import EAThread && EAStdC for EAStopWatch...
Tried both:
eastl::chrono::high_resolution_clock::time_point t1 = eastl::chrono::high_resolution_clock::now();
eastl::chrono::high_resolution_clock::time_point t2 = eastl::chrono::high_resolution_clock::now();
eastl::chrono::nanoseconds time_span = eastl::chrono::duration_cast<eastl::chrono::nanoseconds>(t2 - t1);
time_span.count()*1e-9f
And:
eastl::chrono::high_resolution_clock::time_point t1 = eastl::chrono::high_resolution_clock::now();
eastl::chrono::high_resolution_clock::time_point t2 = eastl::chrono::high_resolution_clock::now();
eastl::chrono::duration<double> time_span = eastl::chrono::duration_cast<eastl::chrono::duration<double>>(t2 - t1);
time_span.count()
Neither works!
Opened a ticket with EATECH: https://jira.frostbite.ea.com/browse/FB-64645
*/

/**
IDEAS:

* Replace gamemap with a map of intrusive lists. Since we end up looking up all the games anyway, we might as well point to them directly, this also removes the overhead
of allocating vectors of gameids for each rank cohort because all the games can be stored in an intrusive list with 0 allocations. The game will need 2 intrusive nodes one for
the top level working set that never changes, and the other node that is used to attach the game into specific cohort indexed list where it belongs for the currently
evaluated factor. Once a list of games is passed into the next factor evaluation, the index node can be reused (*without* even needing to unlink it from the previous list)
in order to attach it to the new list within the new factor, because the original ranking will never again be needed (assuming the next game pointer is already cached in the stack).

* Quality Factor Metrics - Game quality factors are used to progressively sift the set of possible games. Each factor is responsible for rejecting 
a certain portion of the total game candidates. This means that much like the hard filters used in the initial filtering phase of matchmaking, the GQF benefit
from tracking the number of items admitted/rejected for a given filter within its sift order as well as in a standalone context. Besides being useful for
investigating the available game populations these metrics could also in the event when the priorityorder of the factors is not strict, and it is better
to order the factors in sequence from most restrictive in order to reduce the processing costs.

** Rejected game metrics tracking per factor - Need to add tracking of rejected games (accept returns -1)
** Change GamePackerTool to output Game metrics for each factor by using the per/factor provided eval functions.
** Change game::scores to store the span, rather than only the score. This way those values can be reused for calculating the score again.


* Future: Try the idea of using an interval range tree that could be used to speed up selecting Parties that match the incoming skill range. 
    Algo links: 
    http://stackoverflow.com/questions/12720505/data-structure-for-range-query, 
    http://stackoverflow.com/questions/4106541/c-implementation-of-an-interval-tree
    http://www.bowdoin.edu/~ltoma/teaching/cs231/fall07/Lectures/augtrees.pdf // how to augment RBtree to make an interval tree
    https://github.com/ekg/intervaltree
    If this can work it could prove very good since the number of evictable Parties return viable range counts are: 
    1200 4v4 Parties<0.63%, 32000 1v1 Parties<0.026% of total Parties considered.
    range tree usage:
    greater team: party.skill - initialAbsImbalance * 2 <= incomingSkill <= party.skill =>
        greaterintervals.insert(party.skill - initialAbsImbalance * 2, party.skill)
        greaterinvervals.find(incomingSkill)
    lesser team: party.skill <= incomingSkill <= party.skill + initialAbsImbalance * 2 =>
        lesserintervals.insert(party.skill, party.skill + initialAbsImbalance * 2)
        lesserinvervals.find(incomingSkill)

*/

namespace Packer
{

static float clampToNearest(float value, float start, float end)
{
    auto min = start;
    auto max = end;
    if (start > end)
    {
        // range is reversed
        min = end;
        max = start;
    }
    if (value < min)
        return min;
    if (value > max)
        return max;

    return value;
}

typedef eastl::map<Score, GameIdList> GameIdByScoreMap;

static Time MAX_PARTY_PRIORITY = INT64_MIN; // Largest negative number because most negative number has the highest priority
static size_t TRACE_PARTY_OUTPUT_DEFAULT_SIZE = 200;

static_assert(MAX_TEAM_COUNT <= MAX_PLAYER_COUNT, "Team count cannot exceed player count!");

PackerSilo::PackerSilo()
{
}

PackerSilo::~PackerSilo()
{
    EA_ASSERT_MSG(!isReaping(), "Destroying silo during reap phase!");
    EA_ASSERT_MSG(!isPacking(), "Destroying silo during pack phase!");
    // remove games/parties first because their destructors remove entries from seondary indexing data structures
    mParties.clear();
    mGames.clear();
}

void PackerSilo::setUtilityMethods(PackerUtilityMethods& methods)
{
    EA_ASSERT_MSG(mUtilityMethods == nullptr, "Utility methods cannot be changed!");
    mUtilityMethods = &methods;
}

PackerUtilityMethods* PackerSilo::getUtilityMethods()
{
    return mUtilityMethods;
}

void PackerSilo::setPackerSiloId(PackerSiloId packerSiloId)
{
    mPackerSiloId = packerSiloId;
}

void PackerSilo::setParentObject(void* parentObject)
{
    mParentObject = parentObject;
}

void PackerSilo::setPackerName(const char8_t* packerName)
{
    mPackerName = packerName;
}

void PackerSilo::setPackerDetails(const char8_t* details)
{
    mPackerDetails = details;
}

const PropertyDescriptor& PackerSilo::addProperty(const eastl::string& propertyName)
{
    auto ret = mPropertyDescriptorByName.insert(propertyName);
    auto& descriptor = ret.first->second;
    if (ret.second)
    {
        descriptor.mDescriptorIndex = (decltype(descriptor.mDescriptorIndex)) (mPropertyDescriptorByName.size() - 1);
        descriptor.mPropertyName = propertyName;
        descriptor.mPacker = this;
    }
    return descriptor;
}

const FieldDescriptor& PackerSilo::addPropertyField(const eastl::string & propertyName, const eastl::string & fieldName)
{
    auto& propDescriptor = const_cast<PropertyDescriptor&>(addProperty(propertyName));
    auto ret = propDescriptor.mFieldDescriptorByName.insert(fieldName);
    auto& descriptor = ret.first->second;
    if (ret.second)
    {
        descriptor.mFieldId = ++mNewFieldCount;
        descriptor.mFieldName = fieldName;
        descriptor.mPropertyDescriptor = &propDescriptor;
        auto ret2 = mFieldDescriptorById.insert(descriptor.mFieldId);
        EA_ASSERT_MSG(ret2.second, "Property field ids must be unique within the silo!");
        ret2.first->second = &descriptor;
    }
    return descriptor;
}

const FieldDescriptor * PackerSilo::getPropertyField(const eastl::string & propertyName, const eastl::string & fieldName)
{
    auto propItr = mPropertyDescriptorByName.find(propertyName);
    if (propItr != mPropertyDescriptorByName.end())
    {
        auto fieldItr = propItr->second.mFieldDescriptorByName.find(fieldName);
        if (fieldItr != propItr->second.mFieldDescriptorByName.end())
        {
            return &fieldItr->second;
        }
    }
    return nullptr;
}

bool PackerSilo::removePropertyField(PropertyFieldId fieldId)
{
    auto fieldItr = mFieldDescriptorById.find(fieldId);
    if (fieldItr != mFieldDescriptorById.end())
    {
        auto& fieldDesc = *fieldItr->second;
        mFieldDescriptorById.erase(fieldItr);
        auto& propDescriptor = *fieldDesc.mPropertyDescriptor;
        // NOTE: Care is needed because we can't safely erase from eastl::hash_map 
        // by referring to the key *within* the object being deallocated, since the map continues 
        // to try to reference the key after it deallocated the memory of the object in order 
        // to traverse the nodes remaining in the bucket.
        auto fieldName = eastl::move(fieldDesc.mFieldName);
        propDescriptor.mFieldDescriptorByName.erase(fieldName);

        return true;
    }
    return false;
}

bool PackerSilo::addFactor(const GameQualityFactorConfig& packerConfig)
{
    eastl::string factorName = packerConfig.mPropertyName;
    factorName += '.';
    factorName += packerConfig.mTransform;

    auto foundFactorSpec = GameQualityFactorSpec::getFactorSpecFromConfig(packerConfig.mPropertyName, packerConfig.mTransform);
    if (foundFactorSpec == nullptr)
    {
        errorLog("addFactor: Unable to find factor spec for property name(%s).", packerConfig.mPropertyName.c_str());
        return false;
    }

    const GameQualityFactorSpec& spec = *foundFactorSpec;

    // If the Key is specified, we have to do a lookup of the properties to find out where it's located.

    PropertyDescriptor* foundDescriptor = nullptr;
    if (!packerConfig.mPropertyName.empty())
    {
        auto descriptorItr = mPropertyDescriptorByName.find(packerConfig.mPropertyName);
        if (descriptorItr == mPropertyDescriptorByName.end())
        {
            errorLog("addFactor: Required property descriptor: %s not found. Factor(%s %s).", packerConfig.mPropertyName.c_str(), spec.mTransformName.c_str(), packerConfig.mPropertyName.c_str());
            return false;
        }
        foundDescriptor = &descriptorItr->second;
    }

    // Sanity check the values: 
    if (packerConfig.mBestValue == packerConfig.mWorstValue)
    {
        errorLog("addFactor: Required bestValue(%f) != worstValue(%f). Factor(%s).", packerConfig.mBestValue, packerConfig.mWorstValue, factorName.c_str());
        return false;
    }

    if ((packerConfig.mBestValue != 0) && (packerConfig.mWorstValue != 0))
    {
        errorLog("addFactor: One of the best/worst value range endpoints must be 0. Factor(%s %s).", spec.mTransformName.c_str(), packerConfig.mPropertyName.c_str());
        return false;
    }

    auto& factor = mGameTemplate.mFactors.push_back();
    factor.mFactorSpec = spec;   // copy the factor spec
    factor.mFactorName = factorName; 
    factor.mOutputPropertyName = packerConfig.mOutputPropertyName;
    factor.mResultAggregate = packerConfig.mTeamAggregate;

    factor.mBestValue = packerConfig.mBestValue;
    factor.mWorstValue = packerConfig.mWorstValue;
    factor.mGranularity = packerConfig.mGranularity;
    factor.mViableValue = packerConfig.mViableValue;
    factor.mGoodEnoughValue = packerConfig.mGoodEnoughValue;

    if (factor.mViableValue == INVALID_SHAPER_VALUE)
        factor.mViableValue = factor.mBestValue; // strict viability by default
    else
        factor.mViableValue = clampToNearest(factor.mViableValue, factor.mBestValue, factor.mWorstValue);

    if (factor.mGoodEnoughValue == INVALID_SHAPER_VALUE)
        factor.mGoodEnoughValue = factor.mBestValue;  // strict good enough'ness by default
    else
        factor.mGoodEnoughValue = clampToNearest(factor.mGoodEnoughValue, factor.mBestValue, factor.mViableValue);
    
    if (factor.mGranularity > 0.0f)
        factor.mInverseGranularity = 1.0f / factor.mGranularity;

    factor.mInverseResultValueRange = 1.0f / (factor.mBestValue - factor.mWorstValue);

     // NOTE: raw score computations depend on mInverseResultValueRange computed above, do not reorder them ahead of this
    factor.mViableRawScore = factor.computeRawScore(factor.mViableValue);
    factor.mGoodEnoughRawScore = factor.computeRawScore(factor.mGoodEnoughValue);

    factor.mFactorIndex = (GameQualityFactorIndex) mGameTemplate.mFactors.size() - 1; 
    if (foundDescriptor != nullptr)
    {
        factor.mInputProperty = foundDescriptor; // link factor to property descriptor
        foundDescriptor->mReferencingFactors.insert(&factor); // add descriptor to factor
    }
    // PACKER_TODO - Score only factors are not well supported.
    // factor.mScoreOnlyFactor = packerConfig.scoreOnlyFactor;

    factor.mValuesAsString.sprintf("best:%.4f, good:%.4f, vbl:%.4f, worst:%.4f, gran:%.4f",
        factor.mBestValue, factor.mGoodEnoughValue, factor.mViableValue, factor.mWorstValue, factor.mGranularity);

    mGameTemplate.mInterFactorSettings.mTeamFillIncremental = foundFactorSpec->mRequiresIncrementalTeamFill;

    return true;
}

void PackerSilo::setEvalSeedMode(GameEvalRandomSeedMode mode)
{
    mEvalRandomSeedMode = mode;
}

void PackerSilo::setMaxWorkingSet(uint32_t maxWorkingSet)
{
    mMaxWorkingSet = maxWorkingSet;
}

ConfigVarValue PackerSilo::getVar(GameTemplateVar var) const
{
    return mGameTemplate.getVar(var);
}

void PackerSilo::setVar(GameTemplateVar var, ConfigVarValue value)
{
    mGameTemplate.setVar(var, value);
}

bool PackerSilo::addPackerParty(PartyId partyId, Time creationTime, Time expiryTime, bool immutable)
{
    auto ret = mParties.insert(partyId);
    auto& party = ret.first->second;
    const bool inserted = ret.second;
    if (inserted)
    {
        party.mPartyId = partyId;
        party.mImmutable = immutable;
        party.mCreatedTimestamp = creationTime;
        party.mExpiryTime = expiryTime;
        party.mPartyExpiryQueueItr = mPartyExpiryQueue.insert({ expiryTime, &party });
        party.mPartyPriorityQueueItr = mPartyPriorityQueue.insert({ immutable ? MAX_PARTY_PRIORITY : expiryTime, &party });
        if (mNewPartyTimestamp < creationTime)
        {
            ++mNewPartyCount;
            mNewPartyTimestamp = creationTime;
            party.mCreationSequence = mNewPartyCount;
            mUtilityMethods->utilPackerUpdateMetric(*this, PackerMetricType::PARTIES, 1);
        }
        else
            ++party.mReinsertionCount;
    }
    else
    {
        EA_ASSERT_MSG(party.mPartyId == partyId, "Can't change partyId!");
        EA_ASSERT_MSG(party.mImmutable == immutable, "Can't change immutability!");
    }
    return inserted;
}

void PackerSilo::unpackGameAndRequeueParties(GameId gameId)
{
    auto gameItr = mGames.find(gameId);
    if (gameItr != mGames.end())
    {
        auto& game = gameItr->second;
        EA_ASSERT(game.mGameId == gameId);

        const auto viable = game.mViableGameQueueItr.mpNode != nullptr;
        if (viable)
        {
            mViableGameQueue.erase(game.mViableGameQueueItr);
            game.mViableGameQueueItr = ViableGameQueue::iterator();
            mUtilityMethods->utilPackerUpdateMetric(*this, PackerMetricType::VIABLE_GAMES, -1);
        }

        retireGameFromWorkingSet(game); // is idempotent

        // eject any remaining parties from the game and re-add them back into the queue

        // IMPORTANT: During evaluation game.mGameParties can be left with scratch space containing an extra party, (the one that was most recently a candidate 
        // to be packed into the game) this 'extra' party does not need to be removed by trimming the game.mGameParties because the game's present membership is
        // always correctly tracked by the game.mInternalState[GameState::PRESENT]. This lack of cleanup after an evaluation affords increased efficiency since 
        // the evaluation never needs to do a pass and trim back the mGameParties/mGamePlayers arrays for games whose PRESENT state was *not* altered during the
        // search pass. However, we must be careful not to forget that the mGameParties for a game is likely to contain a scratch element for a party that is not
        // a member of the game, and thus we must always be governed by the party count obtained via game.getPartyCount(GameState::PRESENT) in order to iterate
        // only parties that are members of the game.
        auto requeuedParties = 0;
        const auto partyCount = game.getPartyCount(GameState::PRESENT);
        for (auto partyIndex = 0; partyIndex < partyCount; ++partyIndex)
        {
            EA_ASSERT(game.isPartyAssignedToGame(partyIndex, GameState::PRESENT));
            auto partyItr = mParties.find(game.mGameParties[partyIndex].mPartyId);
            // IMPORTANT: The party existence check is *required* because Game and Party are loosely coupled in the packer. For example:
            // It is valid to call eraseParty() on a party assigned to a game, then call unpackGameAndRequeueParties() on the game to destroy the game and re-enqueue all remaining parties.
            // The eraseParty() call is not expected to update the game because it is inefficient to keep the indexing machinery for party and player information within the game up to date
            // with each party removal while in the process of unpacking the entire game.
            // IMPROVE: erasing a party should either: 
            // 1. unpack and requeue remaining parties, or 
            // 2. should be disallowed, requiring a check to remove from game first, or 
            // 3. it can flag teh game.mGameParties[party] = nullptr; thus making this game clearly not have a party and needing to be repacked. (Need flags in game probably to ensure we know that its dirty.)
            if (partyItr != mParties.end())
            {
                auto& party = partyItr->second;
                EA_ASSERT(party.mGameId == gameId);
                unlinkPartyToGame(party);
                EA_ASSERT_MSG(party.mPartyPriorityQueueItr.mpNode == nullptr, "Party assigned to a game cannot be in the pending queue!");
                party.mPartyPriorityQueueItr = mPartyPriorityQueue.insert({ party.mImmutable ? MAX_PARTY_PRIORITY : party.mExpiryTime, &party });
                party.mRequeuedCount++;
                party.mUnpackedCount++;
                requeuedParties++;
            }
        }

        if (viable)
            traceLog(TraceMode::ALGO, "unpackGameAndRequeueParties: Unpacked viable game(%" PRIi64 "), requeued parties(%d).", game.mGameId, requeuedParties);
        else
            traceLog(TraceMode::ALGO, "unpackGameAndRequeueParties: Unpacked non-viable game(%" PRIi64 "), requeued parties(%d).", game.mGameId, requeuedParties);

        mGames.erase(gameItr);
        mUtilityMethods->utilPackerUpdateMetric(*this, PackerMetricType::GAMES, -1);
    }
}

void PackerSilo::updatePackerPartyPriority(Party& party)
{
    if (party.mPartyPriorityQueueItr.mpNode != nullptr)
        mPartyPriorityQueue.erase(party.mPartyPriorityQueueItr);
    party.mPartyPriorityQueueItr = mPartyPriorityQueue.insert({ party.mImmutable ? MAX_PARTY_PRIORITY : party.mExpiryTime, &party });
}

Party* PackerSilo::getPackerParty(PartyId partyId)
{
    Party* party = nullptr;
    auto partyItr = mParties.find(partyId);
    if (partyItr != mParties.end())
    {
        party = &partyItr->second;
        EA_ASSERT(partyId == party->mPartyId);
    }
    return party;
}

const Party* PackerSilo::getPackerParty(PartyId partyId) const
{
    const Party* party = nullptr;
    auto partyItr = mParties.find(partyId);
    if (partyItr != mParties.end())
    {
        party = &partyItr->second;
        EA_ASSERT(partyId == party->mPartyId);
    }
    return party;
}

uint64_t PackerSilo::getPackerPartiesSince(const Party& party) const
{
    return (mNewPartyCount > party.mCreationSequence) ? mNewPartyCount - party.mCreationSequence : 0;
}

/*
 *  Add player for a specfic party, return index position of player in party where it was added
 */
PartyPlayerIndex PackerSilo::addPackerPlayer(PlayerId playerId, Party& party)
{
    auto ret = mPlayers.insert(playerId);
    auto& player = ret.first->second;
    const bool isNew = ret.second;
    PartyPlayerIndex partyMembershipIndex = 0;
    if (isNew)
    {
        ++mNewPlayerCount;
        player.mPlayerId = playerId;
        partyMembershipIndex = (PartyPlayerIndex) party.mPartyPlayers.size();
        mUtilityMethods->utilPackerUpdateMetric(*this, PackerMetricType::PLAYERS, 1);
    }
    else
    {
        // if player is not new, must be a member of a party, maybe even this one
        for (auto& partyPlayer : party.mPartyPlayers)
        {
            if (partyPlayer.mPlayerId == playerId)
                break;
            ++partyMembershipIndex;
        }
    }
    const bool isNewToParty = partyMembershipIndex == party.mPartyPlayers.size();
    if (isNewToParty)
    {
        if (!isNew)
        {
            // incorporate other games this player may also be a member of
            for (auto otherPartyId : player.mPartyIds)
            {
                EA_ASSERT(otherPartyId != party.mPartyId);
                auto otherPartyItr = mParties.find(otherPartyId);
                EA_ASSERT(otherPartyItr != mParties.end());
                auto& otherParty = otherPartyItr->second;
                // NOTE: otherParty may be in queue (and not be assigned to a game yet), in practice
                // this case can happen when we unpack a game that contains a mutable party A, player X
                // and then add new immutable party B, player X to the queue, with the intent to repack 
                // *both* parties in one packing pass.
                if (otherParty.mGameId != INVALID_GAME_ID)
                    party.mOtherGameIdRefs.insert(otherParty.mGameId).first->second++;
            }
        }
        auto partyPlayerIndex = (PartyPlayerIndex) party.mPartyPlayers.size();
        auto& partyPlayer = party.mPartyPlayers.push_back();
        partyPlayer.mPartyPlayerIndex = partyPlayerIndex;
        partyPlayer.mPlayerId = playerId;
        player.mPartyIds.push_back(party.mPartyId);
    }

    return partyMembershipIndex;
}

Player* PackerSilo::getPackerPlayer(PlayerId playerId)
{
    Player* player = nullptr;
    auto playerItr = mPlayers.find(playerId);
    if (playerItr != mPlayers.end())
    {
        player = &playerItr->second;
        EA_ASSERT(playerId == player->mPlayerId);
    }
    return player;
}

const Player* PackerSilo::getPackerPlayer(PlayerId playerId) const
{
    const Player* player = nullptr;
    auto playerItr = mPlayers.find(playerId);
    if (playerItr != mPlayers.end())
    {
        player = &playerItr->second;
        EA_ASSERT(playerId == player->mPlayerId);
    }
    return player;
}

#define TRACE_GAMES_COL_PADDING "  "
#define TRACE_GAMES_OP_FORMAT "%-10s"
#define TRACE_GAMES_COL_WIDTH "%7"
#define TRACE_GAMES_COL_HEADER_FORMAT TRACE_GAMES_COL_WIDTH "s"
#define TRACE_GAMES_COL_FORMAT_ID TRACE_GAMES_COL_WIDTH PRId64
#define TRACE_GAMES_COL_FORMAT_INT TRACE_GAMES_COL_WIDTH "d"
#define TRACE_GAMES_COL_FORMAT_FLOAT TRACE_GAMES_COL_WIDTH ".5f"

Time PackerSilo::packGames(PackGameHandler* handler)
{
    auto beforeTime = mUtilityMethods->utilPackerMeasureTime();
    mBeginPackCount++;

#if 1
    PartyTimeQueue pendingPartyQueue;
#else
    // PACKER_STUDY: We need to investigate more thoroughly the tradeoffs that exist between immediately re-enqueuing evicted parties, or postponing them until later.
    // In limited testing with a single optimizing factor (gpsettings.cfg only) the former is more efficient and produces substantially better results; 
    // however, with multiple optimizing factors (gpsettings.pings.cfg) the first factor seems to benefit while the secondary factors seem to get worse 
    // (this is possibly a natural consequence of over-tightening the first factor), the performance cost is also increased due to increased eviction rate.
    PartyTimeQueue& pendingPartyQueue = mPartyPriorityQueue;
#endif

    uint64_t numNewGames = 0; // write-only counter, useful for debugging
    uint64_t numPackerIterations = 0; // write-only counter, useful for debugging
    uint64_t numPackerIterationsWithoutNewGame = 0;

    const bool traceGames = mUtilityMethods->utilPackerTraceEnabled(TraceMode::GAMES);
    const bool traceParties = mUtilityMethods->utilPackerTraceEnabled(TraceMode::PARTIES);
    const bool traceAlgo = mUtilityMethods->utilPackerTraceEnabled(TraceMode::ALGO);

    while (!mPartyPriorityQueue.empty())
    {
        pendingPartyQueue.swap(mPartyPriorityQueue);
        for (auto& partyItr : pendingPartyQueue)
        {
            EA_ASSERT_MSG(numPackerIterationsWithoutNewGame < mMaxPackerIterations, "Internal error: Number of repeat packing iterations is too high, likely infinite loop!");
            ++numPackerIterations;
            ++numPackerIterationsWithoutNewGame;
            auto& incomingParty = *partyItr.second;
            incomingParty.mPartyPriorityQueueItr = PartyTimeQueue::iterator();

            GameId optimalGameId = INVALID_GAME_ID;
            if (incomingParty.mImmutable)
            {
                EA_ASSERT(incomingParty.mGameId != INVALID_GAME_ID);
                auto gameItr = mGames.find(incomingParty.mGameId);
                if (gameItr != mGames.end())
                    optimalGameId = incomingParty.mGameId;
            }
            else
            {
                EA_ASSERT(incomingParty.mGameId == INVALID_GAME_ID);
                optimalGameId = findOptimalGame(incomingParty, mGamesWorkingSet, 0);
            }

            const bool createNewGame = optimalGameId == INVALID_GAME_ID;
            if (createNewGame)
            {
                ++numNewGames;
                numPackerIterationsWithoutNewGame = 0;

                // adding new game below, try to enforce maxWorkingset size first
                if ((mMaxWorkingSet > 0) && (mMaxWorkingSet < mGamesWorkingSet.size() + 1))
                {
                    // PACKER_TODO: Currently simple priority order of game quality factors is used to determine which game to age out
                    // in the future we'll need to make this much more elaborate since we want to retire games based on a configurable
                    // policy that respects min viable game constraints, prioritizes games whose party expiry time is older, etc.

                    // find candidate game to retire in score order to trim the working set
                    for (auto ritr = mGameByScoreSet.rbegin(), rend = mGameByScoreSet.rend(); ritr != rend; ++ritr)
                    {
                        auto* candidateGame = *ritr;
                        if (candidateGame->mGameWorkingSetIndex != INVALID_GAME_INDEX)
                        {
                            if (traceGames && mUtilityMethods->utilPackerTraceFilterEnabled(TraceMode::GAMES, candidateGame->mGameId))
                            {
                                eastl::string buf;
                                buf += "\n TRIM working set ->";
                                formatGame(buf, *candidateGame, "retire");
                                traceLog(TraceMode::GAMES, "%s", buf.c_str());
                            }

                            retireGameFromWorkingSet(*candidateGame); // NOTE: Games with immutable parties can be retired just like any other games, so make sure that working set is set above minimum number of immutable party games you intend to pack

                            if (handler)
                                handler->handlePackerRetireGame(*candidateGame);
                            break;
                        }
                    }
                }

                EA_ASSERT(incomingParty.getPlayerCount() <= mGameTemplate.getVar(GameTemplateVar::GAME_CAPACITY)); // TODO: need to actually validate this much earlier, cant fail here!
                auto newGameId = incomingParty.mImmutable ? incomingParty.mGameId : mUtilityMethods->utilPackerMakeGameId(*this);
                mNewGameCount++;
                auto& newGame = mGames.insert({ newGameId, Game(*this) }).first->second;
                newGame.mGameId = newGameId;
                newGame.mImmutable = incomingParty.mImmutable;
                if (mEvalRandomSeedMode != GameEvalRandomSeedMode::NONE)
                {
                    newGame.mEvalRandomSeed = mUtilityMethods->utilPackerRand();
                }
                newGame.mCreatedTimestamp = mUtilityMethods->utilPackerMeasureTime();
                newGame.mImprovementExpiryDeadline = newGame.mCreatedTimestamp + mGameTemplate.mViableGameCooldownThreshold;
                if (newGame.mImprovementExpiryDeadline > incomingParty.mExpiryTime)
                    newGame.mImprovementExpiryDeadline = incomingParty.mExpiryTime;
                mNewGameTimestamp = newGame.mCreatedTimestamp;
                incomingParty.mAddedToGameTimestamp = newGame.mCreatedTimestamp;

                assignPartyMembersToGame(incomingParty, newGame);
                linkPartyToGame(incomingParty, newGame);
                newGame.rebuildIndexes();
                
                newGame.mFactorScores.resize(mGameTemplate.mFactors.size(), INVALID_SCORE);
                newGame.mScoreDeltas.resize(mGameTemplate.mFactors.size(), 0);
                newGame.mFactorFields.resize(mGameTemplate.mFactors.size());

                newGame.mGameWorkingSetIndex = (int32_t)mGamesWorkingSet.size();
                mGamesWorkingSet.push_back(newGame.mGameId);
                ++mDebugGameWorkingSetFreq[(int32_t)mGamesWorkingSet.size()];

                // evaluate all the factors in the game and seed the scores
                bool nowViable = true;

                eastl::string partyOutput;
                if (traceParties)
                {
                    partyOutput.reserve(TRACE_PARTY_OUTPUT_DEFAULT_SIZE);
                    partyOutput.append_sprintf("incoming party(%" PRId64 ") -> new game(%" PRId64 "), factors={", incomingParty.mPartyId, newGameId);
                }

                for (auto& factor : mGameTemplate.mFactors)
                {
                    if (factor.mScoreOnlyFactor)
                        continue;
                    EvalContext evalContext(factor, GameState::PRESENT);
                    const auto score = factor.passiveEval(evalContext, newGame);
                    if (traceParties)
                        partyOutput.append_sprintf("%.4f,", score);
                    if (score != INVALID_SCORE)
                    {
                        newGame.mFactorScores[factor.mFactorIndex] = score;
                        newGame.mFactorFields[factor.mFactorIndex] = evalContext.mCurFieldDescriptor;
                        newGame.mScoreDeltas[factor.mFactorIndex] = score;
                        // ignore granularity param, when we're checking viability
                        if (nowViable)
                        {
                            auto rawScore = factor.computeRawScore(evalContext.mEvalResult);
                            nowViable = factor.isViableScore(rawScore);
                            if (traceAlgo)
                            {
                                traceLog(TraceMode::ALGO, "packGames: viability: game(%" PRIi64 ") with (%d)players/(%d)parties (%s), on factor(%d:%s):  score(%.4f,vbl:%.4f), ideal(%s) \n\t ..with teamCount(%d), expiryIn(%" PRIi64 "us), age(%" PRIi64 "us), factorValues(%s, scoreWithParam:%.4f).",
                                    newGame.mGameId, newGame.getPlayerCount(GameState::PRESENT), newGame.getPartyCount(GameState::PRESENT), (nowViable ? "VIABLE" : "NOT VIABLE"),
                                    factor.mFactorIndex, factor.mFactorName.c_str(), score, factor.computeViableScore(), (isIdealGame(newGame) ? "yes" : "no"),
                                    newGame.getTeamCount(), (newGame.mImprovementExpiryDeadline - newGame.mCreatedTimestamp), (newGame.mCreatedTimestamp - newGame.mCreatedTimestamp),
                                    factor.getValuesAsString(), newGame.mFactorScores[factor.mFactorIndex]);
                            }
                        }
                    }
                    else
                    {
                        errorLog("packGames: Passive eval failed on invalid score(%.4f) for factor(%s, %s), game(%" PRIi64 ").",
                            score, factor.mFactorName.c_str(), factor.getValuesAsString(), newGame.mGameId);
                    }
                }

                if (traceParties)
                {
                    partyOutput.pop_back();
                    partyOutput.append_sprintf("}");
                    traceLog(TraceMode::PARTIES, partyOutput.c_str());
                }

                newGame.updateGameScoreSet(); // index game by score

                mUtilityMethods->utilPackerUpdateMetric(*this, PackerMetricType::GAMES, 1);

                if (nowViable)
                {
                    traceLog(TraceMode::ALGO, "packGames: adding new game(%" PRIi64 ") to viable queue.", newGame.mGameId);
                    EA_ASSERT(newGame.mViableGameQueueItr.mpNode == nullptr);
                    newGame.mViableGameQueueItr = mViableGameQueue.insert({ newGame.mImprovementExpiryDeadline, &newGame });
                    newGame.mViableTimestamp = newGame.mCreatedTimestamp;
                    mUtilityMethods->utilPackerUpdateMetric(*this, PackerMetricType::VIABLE_GAMES, 1);
                }

                if (traceGames && mUtilityMethods->utilPackerTraceFilterEnabled(TraceMode::GAMES, incomingParty.mGameId))
                {
                    eastl::string buf;
                    buf.append("\nCREATE game -> ");
                    formatGame(buf, newGame, "create");
                    buf.append("\n-> incoming party -> ");
                    int32_t partyIndex = newGame.getPartyCount(GameState::PRESENT) - 1;
                    formatParty(buf, newGame, partyIndex, "assign", GameState::FUTURE);
                    traceLog(TraceMode::GAMES, buf.c_str());
                }

                if (handler)
                    handler->handlePackerImproveGame(newGame, incomingParty);
            }
            else
            {
                // improve existing game
                auto gameItr = mGames.find(optimalGameId);
                EA_ASSERT(gameItr != mGames.end());
                auto& foundGame = gameItr->second;
                EA_ASSERT(optimalGameId == foundGame.mGameId);
                EA_ASSERT(foundGame.getPartyCount(GameState::PRESENT) > 0); // findOptimal should never return game with no parties assigned!
                ++foundGame.mImprovedCount;
                auto now = mUtilityMethods->utilPackerMeasureTime();
                foundGame.mImprovedTimestamp = now;
                incomingParty.mAddedToGameTimestamp = now;

                eastl::string alteredPartyBuf;
                if (incomingParty.mImmutable)
                {
                    // we are adding immutable party, findOptimalGame was bypassed so we need to reset future state to match present state, and simlpy add a new party
                    foundGame.copyState(GameState::FUTURE, GameState::PRESENT);
                    assignPartyMembersToGame(incomingParty, foundGame);
                }
                else
                {
                    // NOTE: This section does the equivalent of assignPartyMembersToGame(), except that it also handles evictions
                    const auto& presentPartyBits = foundGame.getPresentState().mPartyBits;
                    const auto& futurePartyBits = foundGame.getFutureState().mPartyBits;
                    // this is equivalent to evictedPartyBitset = presentPartyBitset - futurePartyBitset
                    auto evictedPartyBits = presentPartyBits;
                    evictedPartyBits &= ~futurePartyBits;
                    // iterate backwards because its more efficient to compact elements back to front
                    for (auto partyIndex = evictedPartyBits.find_last(), partySize = evictedPartyBits.size(); partyIndex != partySize; partyIndex = evictedPartyBits.find_prev(partyIndex))
                    {
                        EA_ASSERT(!futurePartyBits.test(partyIndex));
                        const auto& evictedGameParty = foundGame.mGameParties[partyIndex];
                        auto evictedPartyItr = mParties.find(evictedGameParty.mPartyId);
                        EA_ASSERT(evictedPartyItr != mParties.end());
                        auto& evictedParty = evictedPartyItr->second;
                        EA_ASSERT_MSG(!evictedParty.mImmutable, "Immutable party cannot be evicted!");
                        if (traceGames && mUtilityMethods->utilPackerTraceFilterEnabled(TraceMode::GAMES, optimalGameId))
                        {
                            alteredPartyBuf.append("-> alter party -> ");
                            formatParty(alteredPartyBuf, foundGame, (int32_t) partyIndex, "evict", GameState::PRESENT);
                            alteredPartyBuf.append("\n");
                        }
                        EA_ASSERT(evictedParty.mGameId == optimalGameId);
                        unlinkPartyToGame(evictedParty);
                        foundGame.removePartyFields(evictedParty);
                        evictedParty.mEvictionCount++;
                        // keep track of the party that last evicted us
                        if (evictedParty.mLastEvictorPartyId == incomingParty.mPartyId)
                            evictedParty.mLastEvictorEvictionCount++;
                        else
                        {
                            evictedParty.mLastEvictorPartyId = incomingParty.mPartyId;
                            evictedParty.mLastEvictorEvictionCount = 1;
                        }

                        foundGame.mEvictedCount++;
                        mEvictedPartiesCount++;
                        mEvictedPlayersCount += evictedParty.getPlayerCount();

                        if (foundGame.mViableGameQueueItr.mpNode != nullptr)
                        {
                            // If foundGame.mViableGameTimestamp > evictedParty.mAddedToGameTimestamp: Game became viable after party was added to the game. Calculate viable time spent in game against when game became viable.
                            // If foundGame.mViableGameTimestamp < evictedParty.mAddedToGameTimestamp: Game was already viable when party was added to the game. Calculate viable time spent in game against when party was added to the game.
                            // If foundGame.mViableGameTimestamp == evictedParty.mAddedToGameTimestamp: Game became viable when party was added to the game. Calculate viable time spent in game against either game or party timestamp.
                            Time timeInViableGame = (foundGame.mViableTimestamp > evictedParty.mAddedToGameTimestamp) ? (foundGame.mImprovedTimestamp - foundGame.mViableTimestamp) : (foundGame.mImprovedTimestamp - evictedParty.mAddedToGameTimestamp);
                            evictedParty.mTotalTimeInViableGames += timeInViableGame;
                        }

                        if (handler)
                            handler->handlePackerEvictParty(foundGame, evictedParty, incomingParty);

                        evictedParty.mPartyPriorityQueueItr = mPartyPriorityQueue.insert({ evictedParty.mExpiryTime, &evictedParty });
                    }

                    GameInternalState gameState;
                    GamePartyList gameParties; // TODO: reserve
                    GamePlayerList gamePlayers; // TODO: reserve
                    auto minPartyExpiryTime = incomingParty.mExpiryTime;
                    for (auto partyIndex = futurePartyBits.find_first(), partySize = futurePartyBits.size(); partyIndex < partySize; partyIndex = futurePartyBits.find_next(partyIndex))
                    {
                        // update party team assignment
                        TeamIndex futureTeamIndex = foundGame.getTeamIndexByPartyIndex((int32_t) partyIndex, GameState::FUTURE);
                        if (futureTeamIndex == INVALID_TEAM_INDEX)
                        {
                            futureTeamIndex = 0; // Handle case when team is not assigned (due to no team specific factor being used), need to assign team 0 by default.
                        }
                       
                        const TeamIndex currentTeamIndex = foundGame.getTeamIndexByPartyIndex((int32_t) partyIndex, GameState::PRESENT);
                        const bool existingPartySwitchedTeam = (currentTeamIndex >= 0) && (currentTeamIndex != futureTeamIndex);
                        const auto& gameParty = foundGame.mGameParties[partyIndex];
                        if (existingPartySwitchedTeam)
                        {
                            EA_ASSERT_MSG(!gameParty.mImmutable, "Immutable party cannot switch teams!");
                            if (traceGames && mUtilityMethods->utilPackerTraceFilterEnabled(TraceMode::GAMES, optimalGameId))
                            {
                                alteredPartyBuf.append("-> alter party -> ");
                                formatParty(alteredPartyBuf, foundGame, (int32_t)partyIndex, "switch", GameState::FUTURE);
                                alteredPartyBuf.append("\n");
                            }
                        }
                        const auto newPartyIndex = gameState.mPartyCount;
                        auto& newGameParty = gameParties.push_back();
                        newGameParty = gameParty;
                        newGameParty.mPlayerOffset = gameState.mPlayerCount; // use new playerOffset
                        // add party players to the game and assign their indexes to point to the party
                        for (uint16_t i = 0; i < gameParty.mPlayerCount; ++i)
                        {
                            const auto oldPlayerIndex = gameParty.mPlayerOffset + i;
                            auto& newGamePlayer = gamePlayers.push_back();
                            newGamePlayer = foundGame.mGamePlayers[oldPlayerIndex];
                            newGamePlayer.mGamePartyIndex = newPartyIndex; // use new partyIndex
                        }
                        // now we can bump the party and player counts
                        gameState.assignPartyToGame(newPartyIndex, gameParty.mPlayerCount);
                        gameState.assignPartyToTeam(newPartyIndex, gameParty.mPlayerCount, futureTeamIndex);

                        auto futurePartyItr = mParties.find(gameParty.mPartyId);
                        EA_ASSERT(futurePartyItr != mParties.end());
                        auto& futureParty = futurePartyItr->second;
                        auto partyExpiryTime = futureParty.mExpiryTime;
                        if (minPartyExpiryTime > partyExpiryTime)
                            minPartyExpiryTime = partyExpiryTime; // update game expiry time to be that of the lowest party
                    }

                    auto newDeadline = foundGame.mImprovedTimestamp + mGameTemplate.mViableGameCooldownThreshold;
                    if (newDeadline > minPartyExpiryTime)
                        newDeadline = minPartyExpiryTime;
                    foundGame.mImprovementExpiryDeadline = newDeadline;

                    foundGame.copyState(GameState::PRESENT, gameState);
                    foundGame.copyState(GameState::FUTURE, gameState); // We may not need this, because we already sync future state when we do a search
                    foundGame.mGameParties = eastl::move(gameParties);
                    foundGame.mGamePlayers = eastl::move(gamePlayers);
                }
                linkPartyToGame(incomingParty, foundGame);
                foundGame.rebuildIndexes();

                // evaluate all the factors in the game and seed the rank vector


                eastl::string partyOutput;
                if (traceParties)
                {
                    partyOutput.reserve(TRACE_PARTY_OUTPUT_DEFAULT_SIZE);
                    partyOutput.append_sprintf("incoming party(%" PRId64 ") -> existing game(%" PRId64 "), factors={", incomingParty.mPartyId, optimalGameId);
                }

                bool nowViable = true;
                for (auto& factor : mGameTemplate.mFactors)
                {
                    if (factor.mScoreOnlyFactor)
                        continue;
                    EvalContext evalContext(factor, GameState::PRESENT);
                    const auto score = factor.passiveEval(evalContext, foundGame);
                    if (traceParties)
                        partyOutput.append_sprintf("%.4f(%+.4f),", score, foundGame.mScoreDeltas[factor.mFactorIndex]);
                    if (score != INVALID_SCORE)
                    {
                        foundGame.mFactorScores[factor.mFactorIndex] = score;
                        foundGame.mFactorFields[factor.mFactorIndex] = evalContext.mCurFieldDescriptor;

                        // ignore granularity param, when we're checking viability
                        if (nowViable)
                        {
                            const auto rawScore = factor.computeRawScore(evalContext.mEvalResult);
                            nowViable = factor.isViableScore(rawScore);
                            if (traceAlgo)
                            {
                                traceLog(TraceMode::ALGO, "packGames: viability: game(%" PRIi64 ") with (%d)players/(%d)parties (%s), on factor(%d:%s):  score(%.4f,vbl:%.4f), ideal(%s) \n\t ..with teamCount(%d), expiryIn(%" PRIi64 "us), age(%" PRIi64 "us), factorValues(%s, scoreWithParam:%.4f).",
                                    foundGame.mGameId, foundGame.getPlayerCount(GameState::PRESENT), foundGame.getPartyCount(GameState::PRESENT), (nowViable ? "VIABLE" : "NOT VIABLE"),
                                    factor.mFactorIndex, factor.mFactorName.c_str(), score, factor.computeViableScore(), (isIdealGame(foundGame) ? "yes" : "no"),
                                    foundGame.getTeamCount(), (foundGame.mImprovementExpiryDeadline - foundGame.mImprovedTimestamp), (foundGame.mImprovedTimestamp - foundGame.mCreatedTimestamp),
                                    factor.getValuesAsString(), foundGame.mFactorScores[factor.mFactorIndex]);
                            }
                        }
                    }
                    else
                        EA_FAIL_MSG("Bad score!");
                }

                if (traceParties)
                {
                    partyOutput.pop_back();
                    partyOutput.append_sprintf("}");
                    traceLog(TraceMode::PARTIES, partyOutput.c_str());
                }

                foundGame.updateGameScoreSet(); // index by score

                const bool idealScore = isIdealGame(foundGame);

                if (traceGames && mUtilityMethods->utilPackerTraceFilterEnabled(TraceMode::GAMES, foundGame.mGameId))
                {
                    eastl::string gameBuf;
                    gameBuf.append("\nASSIGN party -> ");
                    formatParty(gameBuf, foundGame, foundGame.getPartyCount(GameState::PRESENT) - 1, "assign", GameState::FUTURE);
                    gameBuf.append("\n-> change game -> ");
                    formatGame(gameBuf, foundGame, idealScore ? "retire" : "improve");
                    traceLog(TraceMode::GAMES, gameBuf.c_str());
                    if (!alteredPartyBuf.empty())
                    {
                        alteredPartyBuf.pop_back();
                        traceLog(TraceMode::GAMES, alteredPartyBuf.c_str());
                    }
                }

                bool wasViable = (foundGame.mViableGameQueueItr.mpNode != nullptr);
                if (nowViable)
                {
                    if (wasViable)
                        mViableGameQueue.erase(foundGame.mViableGameQueueItr);
                    else
                    {
                        foundGame.mViableTimestamp = foundGame.mImprovedTimestamp;
                        mUtilityMethods->utilPackerUpdateMetric(*this, PackerMetricType::VIABLE_GAMES, 1);
                    }
                    foundGame.mViableGameQueueItr = mViableGameQueue.insert({ foundGame.mImprovementExpiryDeadline, &foundGame });

                    if (handler)
                        handler->handlePackerImproveGame(foundGame, incomingParty);

                    traceLog(TraceMode::ALGO, "packGames: game(%" PRIi64 ") improvable, would be(VIABLE), previous(%s).", foundGame.mGameId, (wasViable ? "VIABLE" : "NOT VIABLE"));

                    if (idealScore)
                    {
                        retireGameFromWorkingSet(foundGame);

                        if (handler)
                            handler->handlePackerRetireGame(foundGame);
                    }
                }
                else
                {
                    if (handler)
                        handler->handlePackerImproveGame(foundGame, incomingParty);

                    // side: its now possible with granularity applied, for shaped score to be ideal but non-viable, as viability checks vs raw score without granularity
                    traceLog(TraceMode::ALGO, "packGames: game(%" PRIi64 ") improvable, would be(NOT VIABLE), previous(%s).", foundGame.mGameId, (wasViable ? "VIABLE" : "NOT VIABLE"));
                }
            }
        }
        pendingPartyQueue.clear();
    }


    mEndPackCount++;

    auto afterTime = mUtilityMethods->utilPackerMeasureTime();
    return afterTime - beforeTime;
}

void PackerSilo::reapGames(ReapGameHandler* handler)
{
    mBeginReapCount++;
    auto currentTime = mUtilityMethods->utilPackerMeasureTime(); // NOTE: deliberately only take a single reading of the timer here because we want to break up processing of large batches of expired events by requiring repeated calls to reapGames() rather than having a single reapGames() call monopolize the time. This comes with a possible additional overhead whereby the packing step below may cause packing of parties that left expired games only to be expired themselves a short time after, but it's a penalty we willingly pay right now to avoid monopolizing the cpu.

    // sweep through the viable game queue and emit/retire all the games
    while (!mViableGameQueue.empty())
    {
        auto viableGameItr = mViableGameQueue.begin();
        auto& game = *viableGameItr->second;
        traceLog(TraceMode::ALGO, "reapGames: viable game(%" PRIi64 ") reap expiryIn(%" PRIi64 "us): (%s) reaping/removing its parties (has %d currently).", game.mGameId, (viableGameItr->first - currentTime), (viableGameItr->first > currentTime ? "NOT YET" : "EXPIRED"), game.getPartyCount(GameState::PRESENT));
        if (viableGameItr->first > currentTime)
            break;

        EA_ASSERT(viableGameItr == game.mViableGameQueueItr);
        mUtilityMethods->utilPackerUpdateMetric(*this, PackerMetricType::VIABLE_GAMES, -1);
        game.mViableGameQueueItr = ViableGameQueue::iterator();
        retireGameFromWorkingSet(game); // is idempotent
        mViableGameQueue.erase(viableGameItr);

        if (handler)
            handler->handlePackerReapGame(game, true); // PACKER_TODO: this should not require games to be erased immediately, we should just mark them for erasure, and add them to sweep set.

        ++mReapedViableGameCount;

        // erase all game's parties, players and finally the game itself from the packer
        auto partyCount = game.getPartyCount(GameState::PRESENT);
        for (auto i = 0; i < partyCount; ++i)
        {
            auto partyId = game.mGameParties[i].mPartyId;
            auto partyItr = mParties.find(partyId);
            EA_ASSERT(partyItr != mParties.end());
            EA_ASSERT(partyItr->second.mGameId == game.mGameId);
            eraseParty(partyId);
        }

        auto gameId = game.mGameId; // needed because hashtable::erase() may make multiple accesses to the key reference
        mGames.erase(gameId);
        mUtilityMethods->utilPackerUpdateMetric(*this, PackerMetricType::GAMES, -1);
    }

    while (!mPartyExpiryQueue.empty())
    {
        auto partyExpiryItr = mPartyExpiryQueue.begin();
        if (partyExpiryItr->first > currentTime)
            break;

        // expireGame(game), at least one of the parties within it has expired

        auto gameId = partyExpiryItr->second->mGameId;
        auto gameItr = mGames.find(gameId);
        EA_ASSERT(gameItr != mGames.end());
        auto& game = gameItr->second;
        EA_ASSERT(game.mGameId == gameId);
        // NOTE: Don't need to test viability here, because viable game.improveExpiryTime <= min(party.expiryTime); therefore, if we get here the game must not be viable at this party's expiration time.
        EA_ASSERT(game.mViableGameQueueItr.mpNode == nullptr);

        if (handler)
            handler->handlePackerReapGame(game, false);

        retireGameFromWorkingSet(game); // is idempotent

        // eject parties from the game and re-add them back into the queue

        // IMPORTANT: During evaluation game.mGameParties can be left with scratch space containing an extra party, (the one that was most recently a candidate 
        // to be packed into the game) this 'extra' party does not need to be removed by trimming the game.mGameParties because the game's present membership is
        // always correctly tracked by the game.mInternalState[GameState::PRESENT]. This lack of cleanup after an evaluation affords increased efficiency since 
        // the evaluation never needs to do a pass and trim back the mGameParties/mGamePlayers arrays for games whose PRESENT state was *not* altered during the
        // search pass. However, we must be careful not to forget that the mGameParties for a game is likely to contain a scratch element for a party that is not
        // a member of the game, and thus we must always be governed by the party count obtained via game.getPartyCount(GameState::PRESENT) in order to iterate
        // only parties that are members of the game.

        const auto partyCount = game.getPartyCount(GameState::PRESENT);
        traceLog(TraceMode::ALGO, "reapGames: non-viable game(%" PRIi64 ") has an expired party(%" PRIi64 "), removing its parties (has %d currently).", game.mGameId, partyExpiryItr->second->mPartyId, partyCount);
        for (auto partyIndex = 0; partyIndex < partyCount; ++partyIndex)
        {
            auto partyId = game.mGameParties[partyIndex].mPartyId;
            auto partyItr = mParties.find(partyId);
            EA_ASSERT(partyItr != mParties.end());
            auto& party = partyItr->second;
            EA_ASSERT(party.mGameId == gameId);
            if (currentTime < party.mExpiryTime)
            {
                unlinkPartyToGame(party);
                EA_ASSERT_MSG(party.mPartyPriorityQueueItr.mpNode == nullptr, "Party assigned to a game cannot be in the pending queue!");
                party.mPartyPriorityQueueItr = mPartyPriorityQueue.insert({ party.mImmutable ? MAX_PARTY_PRIORITY : party.mExpiryTime, &party });

                if (handler)
                    handler->handlePackerReapParty(game, party, false); // Change to handlePackerRecycleParty?
                party.mRequeuedCount++;
            }
            else
            {
                if (handler)
                    handler->handlePackerReapParty(game, party, true);
                // log party as having timed out without matching. Their metrics are to be recorded, an event is to be emitted for them detailing how many times they were evaluated, what condition caused the game(s) they were in to not be viable, we may also track a full log of each time a party is evicted from a game, the factor causing it and the party that evicted them, etc ?
                eraseParty(partyId);
            }
        }

        mUtilityMethods->utilPackerUpdateMetric(*this, PackerMetricType::GAMES, -1);
        mGames.erase(gameItr);
    }

    mEndReapCount++;
}

GameId PackerSilo::findOptimalGame(const Party& incomingParty, const GameIdList& gameWorkingSet, const GameQualityFactorIndex factorIndex)
{
    EA_ASSERT_MSG(!incomingParty.mImmutable, "Immutable parties must be added explicitly!");
    // each cohort is a subset of workingset ranked by Game Quality
    const bool isLastFactor = mGameTemplate.isLastFactorIndex(factorIndex);
    GameIdByScoreMap gamesByScoreMap; // PACKER_TODO: Replace this structure with a heap of game pointers prioritized by score (and possibly gameid) This would eliminate a lot of allocations when games are pushed into a score based map while still allowing an order based visit.
    {
        GameQualityFactor& factor = mGameTemplate.mFactors[factorIndex];
        EA_ASSERT_MSG(!factor.mScoreOnlyFactor, "Score-only factors should not be sent to findOptimal Game!");

        traceLog(TraceMode::ALGO, "findOptimalGame: evaluating party(%" PRIi64 ") vs %" PRIu64 " games using factor[%d](%s)",
            incomingParty.mPartyId, gameWorkingSet.size(), factorIndex, factor.mFactorName.c_str());

        EvalContext evalContext(factor, GameState::FUTURE);
        for (GameId gameId : gameWorkingSet)
        {
            auto findGameItr = mGames.find(gameId);
            if (findGameItr == mGames.end())
            {
                errorLog("findOptimalGame: internal error, game not found for gameId(%d) from input working set, for party(%" PRIi64 ")", gameId, incomingParty.mPartyId);
                continue;
            }
            Game& game = findGameItr->second;

            if (factorIndex == 0)
            {
                // PACKER_MAYBE: Consider an alternative implementation of this test, whereby we only do this validation after having attempted to optimize the game. This prevents us from spending cycles tracking which parties have members that belong to other games, because the test can be performed locally once a specific game has already been chosen as best.
                bool partyMemberAlreadyInGame = incomingParty.mOtherGameIdRefs.find(gameId) != incomingParty.mOtherGameIdRefs.end();
                if (partyMemberAlreadyInGame)
                {
                    // PACKER_TODO: Make this handling more sophisticated rather than just skipping games that already contains members of the incoming session, we may be able to evict assigned parties that contain duplicate members, see: https://developer.ea.com/pages/viewpage.action?spaceKey=GOSInternal&postingDay=2018%2F5%2F3&title=Gamepacker+handling+of+repeated+parties+and+players

                    traceLog(TraceMode::ALGO, "findOptimalGame: member of party(%" PRIi64 ") already in game(%" PRIu64 "), skipping candidate game evaluation",
                        incomingParty.mPartyId, gameId);
                    continue;
                }

                if (factor.mInputProperty != nullptr)
                {
                    if (!game.hasCompatibleFields(incomingParty))
                    {
                        // PACKER_TODO: Add metric
                        traceLog(TraceMode::ALGO, "findOptimalGame: party(%" PRIi64 ") has no fields in common with game(%" PRIu64 "), skipping candidate game evaluation",
                            incomingParty.mPartyId, gameId);
                        continue;
                    }
                }

                auto numParties = game.getPartyCount(GameState::PRESENT);
                EA_ASSERT(numParties < MAX_PARTY_COUNT + 1);
                auto numPlayers = game.getPlayerCount(GameState::PRESENT);

                // reset scratch(FUTURE) values from real(PRESENT)
                game.copyState(GameState::FUTURE, GameState::PRESENT);
                game.mGameParties.resize(numParties);
                game.mGamePlayers.resize(numPlayers);
                game.mFieldValueTable.trimToCommitted();
                if (mEvalRandomSeedMode == GameEvalRandomSeedMode::ACTIVE_EVAL)
                {
                    game.mEvalRandomSeed = mUtilityMethods->utilPackerRand();
                }

                // add party to the game, without assigning it to any teams, or rebuilding property indexes
                auto& futureState = game.getFutureState();
                auto incomingPartyIndex = numParties;
                auto incomingPlayerCount = incomingParty.getPlayerCount();
                auto& gameParty = game.mGameParties.push_back();
                gameParty.mPartyId = incomingParty.mPartyId;
                gameParty.mImmutable = false; // adding immutable parties to a game is done outside findOptimalGame()
                gameParty.mPlayerCount = (uint16_t) incomingPlayerCount;
                gameParty.mPlayerOffset = (uint16_t) numPlayers;
                int32_t playerPropertyIndex = 0;
                for (auto& partyPlayer : incomingParty.mPartyPlayers)
                {
                    auto& gamePlayer = game.mGamePlayers.push_back();
                    gamePlayer.mPlayerId = partyPlayer.mPlayerId;
                    gamePlayer.mGamePartyIndex = numParties;
                    numPlayers++;
                    playerPropertyIndex++;
                }
                futureState.assignPartyToGame(incomingPartyIndex, incomingPlayerCount);
                // Build property set intersection between incoming party and game
                game.addPartyFields(incomingParty);
            }
            
            auto evictedFuturePartyBits = game.getFutureState().mPartyBits; // save future party bits

            const auto activeEvalSeq = ++mActiveEvalCount;

            const auto newScore = factor.activeEval(evalContext, game);

            const auto curScore = game.mFactorScores[factorIndex];

            traceLog(TraceMode::ALGO, "findOptimalGame: activeEval party(%" PRIi64 ") vs game(%" PRIi64 "), factor[%d](%s): score poss(%.4f) vs old(%.4f), reason(%s), evalSeq(%" PRIi64 ")",
                incomingParty.mPartyId, gameId, factorIndex, factor.mFactorName.c_str(), newScore, curScore, evalContext.getResultReason(), activeEvalSeq);

            const char8_t* operation = nullptr;
            const char8_t* subOp = nullptr;
            uint64_t* metric = nullptr;

            if (newScore >= MIN_SCORE)
            {
                // diff *new* future party bits, to determine which parties were evicted in activeEval() above
                evictedFuturePartyBits &= ~game.getFutureState().mPartyBits;
                if (evictedFuturePartyBits.any())
                {
                    // before evaluating further, mark for removal any fields belonging to parties that were evicted as part of active evaluation
                    for (auto partyIndex = evictedFuturePartyBits.find_first(), partySize = evictedFuturePartyBits.size(); partyIndex != partySize; partyIndex = evictedFuturePartyBits.find_next(partyIndex))
                    {
                        game.markPartyFieldsForRemoval((PartyIndex)partyIndex);
                    }
                }

                /*
                // SUMMARY
                if (game.rankimprovecount > 0)
                ; // add game because preceding rank estimated as improved
                else if (newScore < curScore)
                ; // add game because this rank estimated as improved
                else if (newScore == curScore && !lastfactor)
                ; // add game, because this is not the final rank and we may yet improve
                else
                ; // do not add game
                */
                game.mScoreDeltas[factorIndex] = newScore - curScore;

                bool improvedAny = false;
                bool tryAnyway = false;
                if (newScore > curScore)
                {
                    operation = "improve";
                    subOp = "try";
                    metric = &factor.mInterimMetrics.mImproved;
                    improvedAny = true;
                }
                else
                {
                    if (newScore < curScore)
                    {
                        // always output degraded, as it shows the games we skip over
                        operation = "degrade";
                        subOp = "skip";
                        metric = &factor.mInterimMetrics.mDegraded;
                    }
                    else
                    {
                        operation = "nochange";
                        subOp = "skip";
                        metric = &factor.mInterimMetrics.mUnchanged;
                        tryAnyway = true;
                    }
                    for (GameQualityFactorIndex i = 0; i < factorIndex; ++i)
                    {
                        if (game.mScoreDeltas[i] > 0)
                        {
                            subOp = "try";
                            improvedAny = true; // improved preceding factors
                            break;
                        }
                    }
                }

                if (improvedAny || (tryAnyway && !isLastFactor))
                {
                    // ignore granularity param, when we're checking viability
                    const auto rawScore = factor.computeRawScore(evalContext.mEvalResult);
                    const auto isViable = factor.isViableScore(rawScore);
                    const bool becomesNonviable = ((game.mViableGameQueueItr.mpNode != nullptr) && !isViable);
                    // By default don't allow switching to non viable.
                    if (becomesNonviable)
                    {
                        traceLog(TraceMode::ALGO, "findOptimalGame: on factor[%d](%s) game(%" PRIi64 ") would become no longer viable, evalSeq(%" PRIi64 "). Skip.", factor.mFactorIndex, factor.mFactorName.c_str(), game.mGameId, activeEvalSeq);
                        operation = "nonviable";
                        subOp = "skip";
                        metric = &factor.mInterimMetrics.mRejected;
                    }
                    else
                    {
                        auto& gameIdList = gamesByScoreMap[newScore];
                        gameIdList.push_back(game.mGameId);
                    }
                }
            }
            else
            {
                operation = "reject";
                subOp = "skip";
                metric = &factor.mInterimMetrics.mRejected;
            }

            (*metric)++;
            if (mUtilityMethods->utilPackerTraceFilterEnabled(TraceMode::GAMES, game.mGameId))
            {
                eastl::string buf;
                buf.append_sprintf("\nEVAL evalSeq(%" PRIi64 ") factor -> ", activeEvalSeq);
                formatScore(buf, factorIndex, curScore, newScore, evalContext.mResultReason);
                buf.append("\n-> incoming party -> ");
                PartyIndex partyIndex = game.getIncomingPartyIndex();
                formatParty(buf, game, partyIndex, subOp, GameState::FUTURE);
                buf.append("\n-> for game -> ");
                formatGame(buf, game, operation);
                traceLog(TraceMode::GAMES, buf.c_str());
            }
        }
    }

    for (auto it = gamesByScoreMap.rbegin(), end = gamesByScoreMap.rend(); it != end; ++it)
    {
        const GameIdList& gameIdList = it->second;
        // filled working set, process a rank cohort
        if (!isLastFactor)
        {
            // if the working set is not empty, we have a previous cohort to process
            const auto gameId = findOptimalGame(incomingParty, gameIdList, factorIndex + 1);

            if (gameId != INVALID_GAME_ID)
                return gameId;
        }
        else
        {
            // MAYBE: if multiple games are available, we could do a final assessment to select
            // the game whose ejection candidate shall have the least negative impact on himself/system

            // PACKER_TODO: If multiple games are possible, we could do an additional pass to score all the games by an aggregate score (including consideration for evicting parties with little(definition can be dynamic based on how frequently new options occur in the system) remaining time)

            for (auto gameId : gameIdList)
            {
                auto gameItr = mGames.find(gameId);
                EA_ASSERT(gameItr != mGames.end());
                auto& game = gameItr->second;
                EA_ASSERT(game.mGameId == gameId);
                for (auto& improvedFactor : mGameTemplate.mFactors)
                {
                    if (improvedFactor.mScoreOnlyFactor)
                        continue;
                    EvalContext evalContext(improvedFactor, GameState::FUTURE);

                    const auto futureScore = improvedFactor.passiveEval(evalContext, game);
                    const auto curScore = game.mFactorScores[improvedFactor.mFactorIndex];

                    traceLog(TraceMode::ALGO, "findOptimalGame: passiveEval party(%" PRIi64 ") vs game(%" PRIi64 "), factor[%d](%s): score poss(%.4f) vs old(%.4f), reason(%s)",
                        incomingParty.mPartyId, gameId, improvedFactor.mFactorIndex, improvedFactor.mFactorName.c_str(), futureScore, curScore, evalContext.getResultReason());

                    if (futureScore != INVALID_SCORE)
                    {
                        if (futureScore > curScore)
                        {
                            // highest level factor has improved, all other factors don't matter
                            improvedFactor.mFinalMetrics.mImproved++;
                            mDebugFoundOptimalMatchCount++;
                            return gameId;
                        }
                        if (futureScore < curScore)
                        {
                            improvedFactor.mFinalMetrics.mDegraded++;
                            // TODO: by comparing game.scoreDeltas we can determine how much our interim estimates 
                            // (produced by each factor active evaluation) were off by and track these metrics per factor.
                            break;
                        }
                        improvedFactor.mFinalMetrics.mUnchanged++;
                    }
                    else
                    {
                        // NOTE: This can be a normal situation for example in cases where a factor like gameSize can optimistically expect to fit a party in over capacity, but the passiveEval needs to put its foot down
                        // and reject the over-optimistic situation when no evictions followed (for example when only gamesize factor is configured, thus no evictions would currently take place since gamesize factor does not
                        // specify eviction logic since there is no good way to specify the eviction choice from the factor because a subsequent factor may have a much stronger opinion on which party should be evicted from the game...
                        traceLog(TraceMode::ALGO, "findOptimalGame: factor[%d](%s) passiveEval for game(%" PRId64 ") returned invalid score, game rejected.\n", improvedFactor.mFactorIndex, improvedFactor.mFactorName.c_str(), game.mGameId);
                        break;
                    }
                }

                mDebugFoundOptimalMatchDiscardCount++; // track false positive
            }
        }
    }

    return INVALID_GAME_ID;
}

void PackerSilo::assignPartyMembersToGame(const Party& incomingParty, Game& game)
{
    const auto incomingPlayerCount = incomingParty.getPlayerCount();
    auto assignedTeamIndex = incomingParty.mFaction != INVALID_TEAM_INDEX ? incomingParty.mFaction : 0; // for now default team always 0
    const auto gamePlayerCount = game.getPlayerCount(GameState::PRESENT); // this count does not yet include the players belonging to the incoming party
    const PartyIndex newPartyIndex = game.getPartyCount(GameState::PRESENT); // always last party
    EA_ASSERT_MSG(assignedTeamIndex < mGameTemplate.getVar(GameTemplateVar::TEAM_COUNT), "Internal error: Incoming party team index exceeds valid range");
    EA_ASSERT_MSG(gamePlayerCount + incomingPlayerCount <= game.getGameCapacity(), "Internal error: Game player count exceeds max capacity");
    EA_ASSERT_MSG(game.getTeamPlayerCount(GameState::PRESENT, assignedTeamIndex) + incomingPlayerCount <= game.getTeamCapacity(), "Internal error: Assignment attempt exceeded team player capacity");
    EA_ASSERT_MSG(!game.isPartyAssignedToGame(newPartyIndex, GameState::PRESENT), "Assignment attempt to reuse occupied party index");

    game.mGameParties.resize(newPartyIndex + 1);
    game.mGamePlayers.resize(gamePlayerCount + incomingPlayerCount);
    auto& gameParty = game.mGameParties[newPartyIndex];
    gameParty.mPartyId = incomingParty.mPartyId;
    gameParty.mImmutable = incomingParty.mImmutable;
    gameParty.mPlayerCount = (uint16_t) incomingPlayerCount;
    gameParty.mPlayerOffset = (uint16_t) gamePlayerCount;
    for (PlayerIndex incomingPlayerIndex = 0; incomingPlayerIndex < incomingPlayerCount; ++incomingPlayerIndex)
    {
        const auto newPlayerIndex = gamePlayerCount + incomingPlayerIndex;
        GamePlayer& gamePlayer = game.mGamePlayers[newPlayerIndex];
        const auto& partyPlayer = incomingParty.mPartyPlayers[incomingPlayerIndex];
        gamePlayer.mPlayerId = partyPlayer.mPlayerId;
        gamePlayer.mGamePartyIndex = newPartyIndex;
    }
    auto& presentState = game.getPresentState();
    presentState.assignPartyToGame(newPartyIndex, incomingPlayerCount);
    presentState.assignPartyToTeam(newPartyIndex, incomingPlayerCount, assignedTeamIndex);
    game.copyState(GameState::FUTURE, presentState);
    game.addPartyFields(incomingParty);
}

void PackerSilo::retireGameFromWorkingSet(Game& retiredGame)
{
    if (retiredGame.mGameWorkingSetIndex != INVALID_GAME_INDEX && retiredGame.mGameWorkingSetIndex < (int32_t)mGamesWorkingSet.size())
    {
        auto reassignGameId = mGamesWorkingSet.back();
        if (retiredGame.mGameId != reassignGameId)
        {
            // retired game is not the last game
            EA_ASSERT(mGamesWorkingSet[(size_t)retiredGame.mGameWorkingSetIndex] == retiredGame.mGameId);
            auto gameItr = mGames.find(reassignGameId);
            EA_ASSERT(gameItr != mGames.end());
            auto& lastGame = gameItr->second;
            EA_ASSERT(lastGame.mGameId == reassignGameId);
            EA_ASSERT(lastGame.mGameWorkingSetIndex == (int32_t)mGamesWorkingSet.size() - 1);
            mGamesWorkingSet[(size_t)retiredGame.mGameWorkingSetIndex] = reassignGameId;
            lastGame.mGameWorkingSetIndex = retiredGame.mGameWorkingSetIndex;
        }
        retiredGame.mGameWorkingSetIndex = INVALID_GAME_INDEX; // game has been retired topworking index is invalid
        mGamesWorkingSet.pop_back();
    }
    // PACKER_TODO: We may want to more careful track games that were retired due to being 'ideal' or 'forced' retirement due to working set limit pressure.
    if (retiredGame.mViableGameQueueItr.mpNode != nullptr)
    {
        // If the game is still in the viable queue when it is retired this means that retirement is triggered 'early' from within the packGames() as opposed to 'normally' from reapGames(). When a viable game is retired from packGames() that means the game has reached the 'ideal' score threshold(and needs no further improvement), or the .maxWorkingSet constraint is forcing 'early retirement'. In either case we update it's position in the viableGameQueue to bypass any remaining improvement time the game may have had left.
        EA_ASSERT(retiredGame.mImprovementExpiryDeadline == retiredGame.mViableGameQueueItr->first);
        
        Time cooldownDeadline = mUtilityMethods->utilPackerMeasureTime() + mUtilityMethods->utilPackerRetirementCooldown(*this); // Adds a delay (cannot be greater than remaining expiry time) after game is retired from the working set but before it is reaped. This is useful if some games need to be unpacked even though they are technically ready for reaping.
        if (cooldownDeadline < retiredGame.mImprovementExpiryDeadline)
        {
            mViableGameQueue.erase(retiredGame.mViableGameQueueItr);
            retiredGame.mImprovementExpiryDeadline = cooldownDeadline; // cooldown would occur before expiry
            retiredGame.mViableGameQueueItr = mViableGameQueue.insert({ retiredGame.mImprovementExpiryDeadline, &retiredGame });
        }
    }
}

void PackerSilo::linkPartyToGame(Party& party, Game& game)
{
    auto gameId = game.mGameId;
    if (party.mImmutable)
    {
        EA_ASSERT(gameId == party.mGameId);
    }
    else
    {
        EA_ASSERT(gameId != INVALID_GAME_ID && party.mGameId == INVALID_GAME_ID); // validate we are not stomping another assignment set incoming party to point to the game
        party.mGameId = gameId;
    }
    
    // update all other parties to reference this game because of their players
    for (auto& partyPlayer : party.mPartyPlayers)
    {
        auto playerItr = mPlayers.find(partyPlayer.mPlayerId);
        EA_ASSERT(playerItr != mPlayers.end());
        auto& player = playerItr->second;
        for (auto otherPartyId : player.mPartyIds)
        {
            if (otherPartyId == party.mPartyId)
                continue;
            // upate other parties to reference this game on behalf of each player
            auto otherPartyItr = mParties.find(otherPartyId);
            // assert partyitr
            auto& otherParty = otherPartyItr->second;
            otherParty.mOtherGameIdRefs.insert(gameId).first->second++;
        }
    }
    game.commitPartyFields(party);
}

void PackerSilo::unlinkPartyToGame(Party& party)
{
    auto gameId = party.mGameId;
    if (gameId == INVALID_GAME_ID)
        return; // mutable party was already unlinked from the game id, early out

    // NOTE: We deliberately do not update the gameParty information *within*
    // the associated game, because party/game assignment is 'loosely coupled'.
    // The purpose is to avoid unnecessary overhead when multiple parties
    // are assigned/unassigned to a game in rapid succession. Each entity
    // (ie: Game, Party) is responsible for maintaining its own integrity
    // invariants after a set of assignment operations is complete.

    if (!party.mImmutable)
    {
        party.mGameId = INVALID_GAME_ID; // unlink from game
    }

    // update all other parties to unreference this game
    for (auto& partyPlayer : party.mPartyPlayers)
    {
        auto playerItr = mPlayers.find(partyPlayer.mPlayerId);
        EA_ASSERT(playerItr != mPlayers.end());
        auto& player = playerItr->second;
        for (auto otherPartyId : player.mPartyIds)
        {
            if (otherPartyId == party.mPartyId)
                continue;
            // upate other parties to reference this game on behalf of each player
            auto otherPartyItr = mParties.find(otherPartyId);
            // assert partyitr
            auto& otherParty = otherPartyItr->second;
            auto gameIdRefItr = otherParty.mOtherGameIdRefs.find(gameId);
            if (gameIdRefItr != otherParty.mOtherGameIdRefs.end())
            {
                if (gameIdRefItr->second > 1)
                    gameIdRefItr->second--;
                else
                    otherParty.mOtherGameIdRefs.erase(gameIdRefItr);
            }
        }
    }
}

bool PackerSilo::eraseParty(PartyId partyId)
{
    auto partyItr = mParties.find(partyId);
    if (partyItr == mParties.end())
        return false;

    auto& party = partyItr->second;

    // NOTE:  This doesn't actually remove the Party from the Game::mGameParties,
    // or Game.mInternalState[], etc. This is because party erasure always results
    // in the game getting repacked, so it would be wasteful to attempt to update
    // the game's party indexing information prior to the game itself being rebuilt.
    unlinkPartyToGame(party);

    if (party.mPartyExpiryQueueItr.mpNode != nullptr)
    {
        mPartyExpiryQueue.erase(party.mPartyExpiryQueueItr);
        party.mPartyExpiryQueueItr = PartyTimeQueue::iterator();
    }

    if (party.mPartyPriorityQueueItr.mpNode != nullptr)
    {
        mPartyPriorityQueue.erase(party.mPartyPriorityQueueItr);
        party.mPartyPriorityQueueItr = PartyTimeQueue::iterator();
    }

    for (auto& partyPlayer : party.mPartyPlayers)
    {
        auto playerItr = mPlayers.find(partyPlayer.mPlayerId);
        if (playerItr != mPlayers.end())
        {
            auto& player = playerItr->second;
            uint32_t index = 0;
            for (auto playerPartyId : player.mPartyIds)
            {
                if (playerPartyId == party.mPartyId)
                {
                    player.mPartyIds.erase_unsorted(player.mPartyIds.begin() + index);
                    break;
                }
                ++index;
            }

            if (player.mPartyIds.empty()) // no more parties reference this player
            {
                mPlayers.erase(playerItr);
                mUtilityMethods->utilPackerUpdateMetric(*this, PackerMetricType::PLAYERS, -1);
            }
        }
    }

    mUtilityMethods->utilPackerUpdateMetric(*this, PackerMetricType::PARTIES, -1);
    mParties.erase(partyItr);

    return true;
}

bool PackerSilo::isReportEnabled(ReportMode mode) const
{
    return (mReportModeBits & (1 << (uint32_t)mode)) != 0;
}

void PackerSilo::errorLog(const char8_t* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    mUtilityMethods->utilPackerLog(*this, LogLevel::ERR, fmt, args);
    va_end(args);
}

void PackerSilo::reportLog(ReportMode mode, const char8_t* fmt, ...)
{
    if (isReportEnabled(mode))
    {
        va_list args;
        va_start(args, fmt);
        mUtilityMethods->utilPackerLog(*this, LogLevel::REPORT, fmt, args);
        va_end(args);
    }
}

int32_t PackerSilo::formatGame(eastl::string& buf, const Game& game, const char8_t* operation) const
{
    auto initialSize = (int32_t)buf.size();
    buf.append_sprintf("game-op(%s), game(%" PRId64 "), ", operation, (int64_t)game.mGameId);
    for (auto& factor : mGameTemplate.mFactors)
    {
        if (factor.mScoreOnlyFactor)
            continue;
        buf.append_sprintf("%s(%f), ", factor.mFactorName.c_str(), game.mFactorScores[factor.mFactorIndex]);
    }
    buf.resize(buf.size()-2); // trim last 2 characters

    return (int32_t)buf.size() - initialSize;
}

int32_t PackerSilo::formatParty(eastl::string& buf, const Game& game, PartyIndex partyIndex, const char8_t* operation, GameState state) const
{
    auto initialSize = (int32_t)buf.size();
    const auto& party = game.mGameParties[partyIndex];
    buf.append_sprintf("party-op(%s), party(%" PRId64 "), players(%d), team(%d)", operation, (int64_t)party.mPartyId, (int32_t)party.mPlayerCount, (int32_t)game.getTeamIndexByPartyIndex(partyIndex, state));
    return (int32_t)buf.size() - initialSize;
}

int32_t PackerSilo::formatScore(eastl::string& buf, GameQualityFactorIndex factorIndex, Score curScore, Score estimateScore, const char8_t* resultReason) const
{
    auto initialSize = (int32_t)buf.size();
    if (estimateScore == INVALID_SCORE)
    {
        buf.append_sprintf("factor[%d](%s), score(%f), reason(%s)", factorIndex, mGameTemplate.mFactors[factorIndex].mFactorName.c_str(), estimateScore, (resultReason ? resultReason : "none"));
    }
    else
    {
        buf.append_sprintf("factor[%d](%s), score(%f), change(%f)", factorIndex, mGameTemplate.mFactors[factorIndex].mFactorName.c_str(), estimateScore, estimateScore - curScore);
    }
    return (int32_t)buf.size() - initialSize;
}

bool PackerSilo::isIdealGame(const Game& game) const
{
    // MAYBE: Tests show that disabling 'early' retirement of games when isIdealGame()==true led to better overall results across all games, because the the packer was able to take advantage of more options, without significantly impacting performance. One exampe was that the aggregate absolute skill difference went from being 113 to 88 a more than 20% improvement overall. It is potentially useful to turn off the ideal game retirement entirely and instead rely on the viability + reaping (plus hard cap) alone to reduce the working set, instead of attempting to retire ideal games early...
    for (auto& factor : mGameTemplate.mFactors)
    {
        if (factor.mScoreOnlyFactor)
            continue;

        if (game.mFactorScores[factor.mFactorIndex] < MAX_SCORE)
            return false;
    }

    return true;
}

void PackerSilo::setDebugFactorMode(uint32_t mode)
{
    mDebugFactorMode = mode;
}

void PackerSilo::traceLog(TraceMode mode, const char8_t* fmt, ...)
{
    if (mUtilityMethods->utilPackerTraceEnabled(mode))
    {
        va_list args;
        va_start(args, fmt);
        mUtilityMethods->utilPackerLog(*this, LogLevel::TRACE, fmt, args);
        va_end(args);
    }
}

void PackerSilo::setReport(ReportMode mode, bool enable /*= true*/)
{
    if (enable)
        mReportModeBits |= (1 << (uint32_t)mode);
    else
        mReportModeBits &= ~(1 << (uint32_t)mode);
}

Time PackerSilo::getNextReapDeadline() const
{
    Time nextExpiryTime = -1;
    if (!mViableGameQueue.empty())
        nextExpiryTime = mViableGameQueue.begin()->first;
    else if (!mPartyExpiryQueue.empty())
    {
        auto partyExpiryItr = mPartyExpiryQueue.begin();
        auto nextPartyExpiryTime = partyExpiryItr->first;
        if (nextExpiryTime < 0 || nextPartyExpiryTime < nextExpiryTime)
            nextExpiryTime = nextPartyExpiryTime;
    }
    return nextExpiryTime;
}

void* PackerSilo::getParentObject() const
{
    return mParentObject;
}

const eastl::string& PackerSilo::getPackerName() const
{
    return mPackerName;
}

const eastl::string& PackerSilo::getPackerDetails() const
{
    return mPackerDetails;
}

const Game* PackerSilo::getPackerGame(GameId gameId) const
{
    const Game* game = nullptr;
    auto gameItr = mGames.find(gameId);
    if (gameItr != mGames.end())
        game = &gameItr->second;
    return game;
}

bool PackerSilo::setVarByName(const char8_t* varName, ConfigVarValue value)
{
    if (!varName || !mGameTemplate.setVarByName(varName, value))
    {
        errorLog("setVarByName: Failed to set variable(%s) to value(%d).", (varName != nullptr ? varName : "<NULL>"), value);
        return false;
    }
    return true;
}

void PackerSilo::setViableGameCooldownThreshold(int64_t threshold)
{
    mGameTemplate.mViableGameCooldownThreshold = threshold;
}

void PackerSilo::setMaxPackerIterations(uint64_t maxRepeatPackings)
{ 
    mMaxPackerIterations = maxRepeatPackings;
}

float PackerSilo::evalVarExpression(const char8_t* expression) const
{
    return mGameTemplate.evalVarExpression(expression);
}

} // GamePacker

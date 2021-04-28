/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef GAME_PACKER_COMMON_H
#define GAME_PACKER_COMMON_H

#include <EABase/eabase.h>
#include <EAAssert/eaassert.h>
#include <EASTL/bitset.h>
#include <EASTL/string.h>
#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <EASTL/hash_set.h>
#include <EASTL/vector_map.h>
#include <EASTL/fixed_vector.h>

#if !defined(GAME_PACKER_MAX_TEAM_COUNT)
#define GAME_PACKER_MAX_TEAM_COUNT 64 // by default packer currently supports up to 64 teams: "enough for anyone"^TM
#endif

namespace Packer
{

enum class TraceMode
{
    GAMES,
    PARTIES,
    PLAYERS,
    ALGO,
    E_COUNT
};

enum class ReportMode
{
    SUMMARY,
    GAMES,
    PARTYS,
    PLAYERS,
    WORKINGSET,
    E_COUNT
};

enum class LogLevel
{
    TRACE,
    ERR,
    REPORT,
    E_COUNT
};

enum class PropertyAggregate        // Matches with MergeOps in GameManager
{
    NONE,
    SUM,
    MIN,
    MAX,
    SIZE,
    AVERAGE,
    STDDEV,
    MINMAXRANGE,      // Formerly FactorShaperType 'RANGE'
    MINMAXRATIO,      // Formerly FactorShaperType 'RATIO'.  Not very useful, prefer others. 
    E_COUNT
};

enum class FactorGoal
{
    MAXIMIZE,
    MINIMIZE,
    E_COUNT
};

enum class GameTemplateVar // MAYBE: rename to PackerConfigVar?
{
    TEAM_COUNT,
    TEAM_CAPACITY,
    GAME_CAPACITY,
    E_COUNT
};

enum class GameState
{
    PRESENT,
    FUTURE,
    E_COUNT
};

enum class PackerMetricType
{
    GAMES,
    VIABLE_GAMES,
    PARTIES,
    PLAYERS,
    E_COUNT
};

enum class GameEvalRandomSeedMode
{
    NONE,           // always use seed of 0 (useful for tool to maintain 100% reproducibility)
    CREATION,       // seed computed once per game creation
    ACTIVE_EVAL,    // seed recomuputed each time the game is evaluated as part of the packing pass
    E_COUNT
};

typedef float Score;
const Score MIN_SCORE = 0.0f;
const Score MAX_SCORE = 1.0f;
const Score INVALID_SCORE = -1.0f;

const int32_t MAX_PLAYER_COUNT = 255;                               // max number of players in a packed game
const int32_t MAX_PARTY_COUNT = MAX_PLAYER_COUNT;                   // max number of parties in a packed game
const int32_t MAX_PROVISIONAL_PLAYER_COUNT = MAX_PLAYER_COUNT * 2;  // max number of players in a provisional game (includes temporary players added for the purpose of evaluation)
const int32_t MAX_PROVISIONAL_PARTY_COUNT = MAX_PARTY_COUNT + 1;    // max number of parties in a provisional game (includes temporary party added for the purpose of evaluation)
const int32_t MAX_TEAM_COUNT = GAME_PACKER_MAX_TEAM_COUNT;          // max number of teams supported by packer algorithm
const int32_t MAX_ACTIVE_QUALITY_FACTOR_COUNT = 8;                  // max number of game quality factors.  PACKER_TODO:  Use this as part of config validation.
const int32_t MAX_REPEAT_PACKING_ITERATIONS = 1000000;              // max number of consecutive packing iterations allowed without creating a new game

typedef uint64_t PackerSiloId;
const PackerSiloId INVALID_PACKER_SILO_ID = 0;

typedef int64_t PlayerId;
typedef int64_t PartyId;
typedef int64_t GameId;

const PlayerId INVALID_PLAYER_ID = -1;
const PartyId INVALID_PARTY_ID = -1;
const GameId INVALID_GAME_ID = -1;

typedef int64_t Time;
typedef eastl::vector<PlayerId> PlayerIdList;
typedef eastl::vector<PartyId> PartyIdList;
typedef eastl::hash_set<PartyId> PartyIdSet;
typedef eastl::vector<GameId> GameIdList;

typedef uint32_t PartyPlayerIndex;
typedef uint8_t PropertyIndexOffset;
const PropertyIndexOffset INVALID_PROPERTY_INDEX_OFFSET = (PropertyIndexOffset)-1;

typedef int32_t ConfigVarValue;

typedef int32_t PlayerIndex;
typedef int32_t PartyIndex;
typedef int32_t TeamIndex;
typedef int64_t GameIndex;

const PlayerIndex INVALID_GAME_PLAYER_INDEX = -1;
const PartyIndex INVALID_PARTY_INDEX = -1;
const TeamIndex INVALID_TEAM_INDEX = -1;
const GameIndex INVALID_GAME_INDEX = -1;

typedef int32_t PropertyIndex;
typedef int32_t PropertyDescriptorIndex;
typedef int32_t PropertyFieldIndex;
typedef int64_t PropertyFieldId;
typedef int64_t PropertyValue; // FUTURE: change property values to float/double?

const PropertyIndex INVALID_PROPERTY_INDEX = -1;
const PropertyDescriptorIndex INVALID_PROPERTY_DESCRIPTOR_INDEX = -1;
const PropertyFieldIndex INVALID_PROPERTY_FIELD_INDEX = -1;

typedef int32_t GameQualityFactorIndex;

const GameQualityFactorIndex INVALID_GAME_QUALITY_FACTOR_INDEX = -1;

static const uint32_t MULTI_TEAM_MIN_TEAM_COUNT = 2; // minimum team count that makes sense when you use a multi-teams factor
static const uint32_t MULTI_TEAM_MAX_PARTY_SIZE = MAX_PLAYER_COUNT / MULTI_TEAM_MIN_TEAM_COUNT;
static const uint32_t MULTI_TEAM_MAX_PARTY_SIZE_COUNT = MULTI_TEAM_MAX_PARTY_SIZE - 1; // party of 0 not allowed, hence: - 1
typedef eastl::bitset<MULTI_TEAM_MAX_PARTY_SIZE_COUNT> PartySizeOccupancyBitset;

typedef eastl::bitset<MAX_PARTY_COUNT + 1> PartyBitset;
typedef eastl::fixed_vector<Score, 2> EvalResult;

struct GameQualityTransform;
struct EvalContext;
struct EvalSpan;
struct Game;

struct caseless_eq
{
    bool operator()(const eastl::string& a, const eastl::string& b) const
    {
        return a.comparei(b) == 0;
    }
};

struct caseless_hash
{
    size_t operator()(const eastl::string& str) const
    {
        auto* p = str.c_str();
        uint32_t c, result = 2166136261U;    // Intentionally uint32_t instead of size_t, so the behavior is the same regardless of size.
        while ((c = (uint32_t)tolower(*p++)) != 0)    // cast to unsigned 32 bit.
            result = (result * 16777619) ^ c;
        return (size_t)result;
    }
};


}

#endif
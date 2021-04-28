/*************************************************************************************************/
/*!
    \file   lobbyskill.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_LOBBYSKILL
#define BLAZE_GAMEREPORTING_LOBBYSKILL

#include "EATDF/tdfobjectid.h"

namespace Blaze
{
namespace GameReporting
{

/*** Defines *****************************************************************/

#define WLT_WIN          0x0001   // Bit vector: user won the game
#define WLT_LOSS         0x0002   // Bit vector: user lost the game
#define WLT_TIE          0x0004   // bit vector: user tied the game
#define WLT_DISC         0x0008   // bit vector: user disconnected during game
#define WLT_CHEAT        0x0010   // bit vector: user cheated during game
#define WLT_CONCEDE      0x0020   // Bit vector: user has conceded the game
#define WLT_NULLIFY      0x0040   // Bit vector: user nullified game (could be friendly quit)
#define WLT_QUITCNT      0x0080   // Bit vector: quit and count
#define WLT_PRORATE_PTS  0x0100   // Bit vector: prorate ranking points
#define WLT_NO_PTS       0x0200   // Bit vector: don't count ranking points
#define WLT_NO_STATS     0x0400   // Bit vector: don't count stats
#define WLT_DESYNC       0x0800   // Bit vector: (server) desync game
#define WLT_CONTESTED    0x1000   // Bit vector: (server) contested score game
#define WLT_SHORT        0x2000   // Bit vector: (server) int16_t game
#define WLT_OPPOQCTAG    0x4000   // Bit vector: (server) opponent quit and count tag

// New ranking formula tweaking parameters.
// Adjust only with *extreme caution*.
#define NEW_RANK_MAXIMUM        234999
#define NEW_RANK_MINIMUM        1
#define NEW_PROBABILITY_SCALE   4000.0
#define NEW_FADE_POWER          1.75
#define NEW_RANGE_POWER         2.5
#define NEW_POINTS_RANGE        0.4
#define NEW_PROBABILITY_UNFAIR  0.95
#define NEW_CHANGE_MAXIMUM      400
#define NEW_CHANGE_MINIMUM      100
#define NEW_POINTS_GUARANTEE    3000
#define NEW_ADJUSTMENT_SCALE    25
#define NEW_TEAM_SCALE          200
#define NEW_OVERTIME_SCALE      0.5

#define NEW_SKILL_PCT_0         70
#define NEW_SKILL_PCT_1         80
#define NEW_SKILL_PCT_2         90
#define NEW_SKILL_PCT_3         100

#define _CLAMP(iInput, iFloor, iCeiling) \
    ((iInput > iCeiling) ? iCeiling \
     : (iInput < iFloor) ? iFloor \
     : iInput)

// Team/venue overall ratings, extracted from database.
// Note that normally team ratings would be multiplied by
// NEW_TEAM_SCALE, but since the team rating also includes
// the stadium rating, team ratings are pre-multiplied
// (by TEAM_SCALE and STADIUM_SCALE) and
// NEW_TEAM_SCALE is set to 1.

#define TEAM_SCALE     150
#define STADIUM_SCALE  30

typedef struct TeamRankingT
{
    int32_t iTeamID;
    int32_t iTeamOverallRating;
    int32_t iTeamStadiumRating;
    const char *strTeam;
} TeamRankingT;

static const TeamRankingT TeamOverallStats[] =
{
    // {ID, OVR, STA, "Name"}
    /* eg. { 1, 64, 0, "Air Force" },
           { 2, 76, 0, "Akron" }, */
    { 0,  99, 0,  "" }  // end-of-list marker
};

class LobbySkill
{
public:
    LobbySkill(bool evenTeams = true);
    ~LobbySkill();

    struct LobbySkillInfo
    {
        LobbySkillInfo() 
            : mWLT(0), mDampingPercent(0), mTeamId(0), mWeight(0), mHome(false), mSkillLevel(0), mCurrentPoints(0)
        {
        }

        void set(int32_t wlt, int32_t dampingPercent, int32_t teamId, int32_t weight, bool home, int32_t skillLevel, int32_t currentPoints)
        {
            mWLT = wlt;
            mDampingPercent = dampingPercent;
            mTeamId = teamId;
            mWeight = weight;
            mHome = home;
            mSkillLevel = skillLevel;
            mCurrentPoints = currentPoints;
        }

        int32_t getWLT() const { return mWLT; }
        int32_t getDampingPercent() const { return mDampingPercent; }
        int32_t getTeamId() const { return mTeamId; }
        int32_t getWeight() const { return mWeight; }
        bool getHome() const { return mHome; }
        int32_t getSkillLevel() const { return mSkillLevel; }
        int32_t getCurrentPoints() const { return mCurrentPoints; }

    private:
        int32_t mWLT;
        int32_t mDampingPercent;
        int32_t mTeamId;
        int32_t mWeight;
        bool mHome;
        int32_t mSkillLevel;
        int32_t mCurrentPoints;
    };

    const LobbySkillInfo *getLobbySkillInfo(EntityId entityId) const 
    {
        LobbySkillInfoMap::const_iterator itr = mLobbySkillInfoMap.find(entityId);
        return itr != mLobbySkillInfoMap.end() ? &(itr->second) : nullptr; 
    }

    void addLobbySkillInfo(EntityId entityId, int32_t wlt, int32_t dampingPercent, int32_t teamId,
        int32_t weight, bool home, int32_t skillLevel, int32_t currentPoints);

    int32_t getNewLobbySkillPoints(EntityId entityId);

private:
    int32_t calcPointsEarned(int32_t iUserPts, int32_t iOppoPts, int32_t iWlt, int32_t iAdj,
        int32_t iSkill, int32_t iTeamDifference) const;

private:
    bool mEvenTeams;

    typedef eastl::hash_map<EntityId, LobbySkillInfo> LobbySkillInfoMap;
    LobbySkillInfoMap mLobbySkillInfoMap;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_GAMEREPORTING_LOBBYSKILL


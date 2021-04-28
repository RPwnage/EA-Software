/*************************************************************************************************/
/*!
    \file   gamecommonlobbyskill.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_CUSTOM_GAMECOMMON_LOBBYSKILL
#define BLAZE_CUSTOM_GAMECOMMON_LOBBYSKILL

#include "stats/tdf/stats.h"
#include "gamereporting/osdk/reportconfig.h"

namespace Blaze
{
namespace GameReporting
{

/*** Defines *****************************************************************/

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

typedef struct OsdkTeamRankingT
{
    int32_t iTeamID;
    int32_t iTeamOverallRating;
    int32_t iTeamStadiumRating;
    const char *strTeam;
} OsdkTeamRankingT;

static const OsdkTeamRankingT OsdkTeamOverallStats[] =
{
    // {ID, OVR, STA, "Name"}
    /* eg. { 1, 64, 0, "Air Force" },
           { 2, 76, 0, "Akron" }, */
    { 0,  99, 0,  "" }  // end-of-list marker
};

class GameCommonLobbySkill
{
public:
    GameCommonLobbySkill(bool evenTeams = true);
    virtual ~GameCommonLobbySkill();

    static const uint32_t DEFAULT_SKILL_LEVEL = 1;

    struct LobbySkillInfo
    {
        LobbySkillInfo();
        ~LobbySkillInfo() {}

        void setLobbySkillInfo(int32_t wlt, int32_t dampingPercent, int32_t teamId, int32_t weight, bool home, int32_t skill);
        void setCurrentSkillPoints(int32_t currentPoints, Stats::StatPeriodType periodType);
        void setCurrentSkillLevel(int32_t currentLevel, Stats::StatPeriodType periodType);
        void setCurrentOppSkillLevel(int32_t currentOppLevel, Stats::StatPeriodType periodType);

        int32_t getWLT() const { return mWLT; }
        int32_t getDampingPercent() const { return mDampingPercent; }
        int32_t getTeamId() const { return mTeamId; }
        int32_t getWeight() const { return mWeight; }
        bool    getHome() const { return mHome; }
        int32_t getSkill() const { return mSkill; }
        int32_t getCurrentSkillPoints(Stats::StatPeriodType periodType) const;
        int32_t getCurrentSkillLevel(Stats::StatPeriodType periodType) const;
        int32_t getCurrentOppSkillLevel(Stats::StatPeriodType periodType) const;

    private:
        int32_t mWLT;
        int32_t mDampingPercent;
        int32_t mTeamId;
        int32_t mWeight;
        bool    mHome;
        int32_t mSkill;
        int32_t mCurrentSkillPoints[Stats::STAT_NUM_PERIODS];
        int32_t mCurrentSkillLevel[Stats::STAT_NUM_PERIODS];
        int32_t mCurrentOppSkillLevel[Stats::STAT_NUM_PERIODS];
    };

    const LobbySkillInfo *getLobbySkillInfo(EntityId entityId) const;

    void addLobbySkillInfo(EntityId entityId, int32_t wlt, int32_t dampingPercent, int32_t teamId, int32_t weight, bool home, int32_t skill);
    void addCurrentSkillPoints(EntityId entityId, int32_t currentPoints, Stats::StatPeriodType periodType);
    void addCurrentSkillLevel(EntityId entityId, int32_t currentLevel, Stats::StatPeriodType periodType);
    void addCurrentOppSkillLevel(EntityId entityId, int32_t currentOppLevel, Stats::StatPeriodType periodType);

    int32_t getNewLobbySkillPoints(EntityId entityId, Stats::StatPeriodType periodType);
    int32_t getNewLobbySkillLevel(EntityId entityId, ReportConfig* config, int32_t newPoints);
    int32_t getNewLobbyOppSkillLevel(EntityId entityId, Stats::StatPeriodType periodType);

    virtual int32_t calcPointsEarned(int32_t iUserPts, int32_t iOppoPts, int32_t iWlt, int32_t iAdj,
        int32_t iSkill, int32_t iTeamDifference) const;

private:
    bool mEvenTeams;

    typedef eastl::hash_map<EntityId, LobbySkillInfo> LobbySkillInfoMap;
    LobbySkillInfoMap mLobbySkillInfoMap;

};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECOMMON_LOBBYSKILL


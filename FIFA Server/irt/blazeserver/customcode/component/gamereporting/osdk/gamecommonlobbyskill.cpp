/*************************************************************************************************/
/*!
    \file   gamecommonlobbyskill.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "gamecommonlobbyskill.h"

namespace Blaze
{
namespace GameReporting
{

/*************************************************************************************************/
/*! \brief LobbySkillInfo()
    Constructor
*/
/*************************************************************************************************/
GameCommonLobbySkill::LobbySkillInfo::LobbySkillInfo()
    : mWLT(0), mDampingPercent(0), mTeamId(0), mWeight(0), mHome(false), mSkill(0)
{
    for (uint32_t uPeriod = 0; uPeriod < Stats::STAT_NUM_PERIODS; uPeriod++)
    {
        mCurrentSkillPoints[uPeriod] = 0;
        mCurrentSkillLevel[uPeriod] = DEFAULT_SKILL_LEVEL;
        mCurrentOppSkillLevel[uPeriod] = DEFAULT_SKILL_LEVEL;
    }
}

/*************************************************************************************************/
/*! \brief setLobbySkillInfo()
    Set the information in LobbySkillInfo
*/
/*************************************************************************************************/
void GameCommonLobbySkill::LobbySkillInfo::setLobbySkillInfo(int32_t wlt, int32_t dampingPercent, int32_t teamId, int32_t weight, bool home, int32_t skill)
{
    mWLT = wlt;
    mDampingPercent = dampingPercent;
    mTeamId = teamId;
    mWeight = weight;
    mHome = home;
    mSkill = skill;
}

/*************************************************************************************************/
/*! \brief setCurrentSkillPoints()
    Set the current skill points
*/
/*************************************************************************************************/
void GameCommonLobbySkill::LobbySkillInfo::setCurrentSkillPoints(int32_t currentPoints, Stats::StatPeriodType periodType)
{
    mCurrentSkillPoints[periodType] = currentPoints;
}

/*************************************************************************************************/
/*! \brief setCurrentSkillLevel()
    Set the current skill level
*/
/*************************************************************************************************/
void GameCommonLobbySkill::LobbySkillInfo::setCurrentSkillLevel(int32_t currentLevel, Stats::StatPeriodType periodType)
{
    mCurrentSkillLevel[periodType] = currentLevel;
}

/*************************************************************************************************/
/*! \brief setCurrentOppSkillLevel()
    Set the current opponent skill level
*/
/*************************************************************************************************/
void GameCommonLobbySkill::LobbySkillInfo::setCurrentOppSkillLevel(int32_t currentOppLevel, Stats::StatPeriodType periodType)
{
    mCurrentOppSkillLevel[periodType] = currentOppLevel;
}

/*************************************************************************************************/
/*! \brief getCurrentSkillPoints()
    Get the current skill points
*/
/*************************************************************************************************/
int32_t GameCommonLobbySkill::LobbySkillInfo::getCurrentSkillPoints(Stats::StatPeriodType periodType) const
{
    return mCurrentSkillPoints[periodType];
}

/*************************************************************************************************/
/*! \brief getCurrentSkillLevel()
    Get the current skill level
*/
/*************************************************************************************************/
int32_t GameCommonLobbySkill::LobbySkillInfo::getCurrentSkillLevel(Stats::StatPeriodType periodType) const
{
    return mCurrentSkillLevel[periodType];
}

/*************************************************************************************************/
/*! \brief getCurrentOppSkillLevel()
    Get the current opponent skill level
*/
/*************************************************************************************************/
int32_t GameCommonLobbySkill::LobbySkillInfo::getCurrentOppSkillLevel(Stats::StatPeriodType periodType) const
{
    return mCurrentOppSkillLevel[periodType];
}

/*************************************************************************************************/
/*! \brief GameCommonLobbySkill()
    Constructor
*/
/*************************************************************************************************/
GameCommonLobbySkill::GameCommonLobbySkill(bool evenTeams)
    : mEvenTeams(evenTeams)
{
}

/*************************************************************************************************/
/*! \brief ~GameCommonLobbySkill()
    Destructor
*/
/*************************************************************************************************/
GameCommonLobbySkill::~GameCommonLobbySkill()
{
    mLobbySkillInfoMap.clear();
}

/*************************************************************************************************/
/*! \brief getLobbySkillInfo()
    Get the LobbySkillInfo
*/
/*************************************************************************************************/
const GameCommonLobbySkill::LobbySkillInfo* GameCommonLobbySkill::getLobbySkillInfo(EntityId entityId) const 
{
    LobbySkillInfoMap::const_iterator itr = mLobbySkillInfoMap.find(entityId);
    return itr != mLobbySkillInfoMap.end() ? &(itr->second) : NULL;
}

/*************************************************************************************************/
/*! \brief addLobbySkillInfo
    add the information to fill out LobbySkillInfo
*/
/*************************************************************************************************/
void GameCommonLobbySkill::addLobbySkillInfo(EntityId entityId, int32_t wlt, int32_t dampingPercent, int32_t teamId,
                                             int32_t weight, bool home, int32_t skill)
{
    TRACE_LOG("[GameCommonLobbySkill:" << this << "].addLobbySkillInfo() : Entity id=" << entityId << ", wlt=" << wlt << ", dampingPercent=" << dampingPercent <<
              ",teamId=" << teamId << ", weight=" << weight<< ", home=" << (home ? 1 : 0) << ", skill=" << skill);

    mLobbySkillInfoMap[entityId].setLobbySkillInfo(wlt, dampingPercent, teamId, weight, home, skill);
}

/*************************************************************************************************/
/*! \brief addCurrentSkillPoints
    add the information to fill out LobbySkillInfo current skill points
*/
/*************************************************************************************************/
void GameCommonLobbySkill::addCurrentSkillPoints(EntityId entityId, int32_t currentPoints, Stats::StatPeriodType periodType)
{
    TRACE_LOG("[GameCommonLobbySkill:" << this << "].addCurrentSkillPoints() : Entity id=" << entityId << ", currentPoints=" << currentPoints <<
              ", periodType=" << periodType);

    mLobbySkillInfoMap[entityId].setCurrentSkillPoints(currentPoints, periodType);
}

/*************************************************************************************************/
/*! \brief addCurrentSkillLevel
    add the information to fill out LobbySkillInfo current skill level
*/
/*************************************************************************************************/
void GameCommonLobbySkill::addCurrentSkillLevel(EntityId entityId, int32_t currentLevel, Stats::StatPeriodType periodType)
{
    TRACE_LOG("[GameCommonLobbySkill:" << this << "].addCurrentSkillLevel() : Entity id=" << entityId << ", currentLevel=" << currentLevel <<
              ", periodType=" << periodType);

    mLobbySkillInfoMap[entityId].setCurrentSkillLevel(currentLevel, periodType);
}

/*************************************************************************************************/
/*! \brief addCurrentOppSkillLevel
    add the information to fill out LobbySkillInfo current opponent skill level
*/
/*************************************************************************************************/
void GameCommonLobbySkill::addCurrentOppSkillLevel(EntityId entityId, int32_t currentOppLevel, Stats::StatPeriodType periodType)
{
    TRACE_LOG("[GameCommonLobbySkill:" << this << "].addCurrentOppSkillLevel() : Entity id=" << entityId << ", currentOppLevel=" << currentOppLevel <<
              ", periodType=" << periodType);

    mLobbySkillInfoMap[entityId].setCurrentOppSkillLevel(currentOppLevel, periodType);
}

/*************************************************************************************************/
/*! \brief getNewLobbySkillPoints
    calculate lobby skill points
*/
/*************************************************************************************************/
int32_t GameCommonLobbySkill::getNewLobbySkillPoints(EntityId entityId, Stats::StatPeriodType periodType)
{
    int32_t newSkillPoints = 0;

    const LobbySkillInfo *skillInfo = getLobbySkillInfo(entityId);
    if (NULL != skillInfo)
    {
        int32_t delta = 0;
        int32_t iPlayerTeamIndex, iOppoTeamIndex;
        int32_t iTeamSkill = 0, iOppoTeamSkill = 0;
        int32_t currentPoints = skillInfo->getCurrentSkillPoints(periodType);
        int32_t teamId = skillInfo->getTeamId();
        int32_t iOwnWL = (skillInfo->getWLT() & WLT_PRORATE_PTS) ? WLT_WIN : skillInfo->getWLT();

        LobbySkillInfoMap::const_iterator oppoSkillInfoIter = mLobbySkillInfoMap.begin();
        LobbySkillInfoMap::const_iterator oppoSkillInfoEnd = mLobbySkillInfoMap.end();

        for (; oppoSkillInfoIter != oppoSkillInfoEnd; ++oppoSkillInfoIter)
        {
            if (entityId == oppoSkillInfoIter->first)
                continue;

            int32_t oppoTeamId = oppoSkillInfoIter->second.getTeamId();

            // scan for team information
            for (iPlayerTeamIndex = 0; ; iPlayerTeamIndex++)
                if (OsdkTeamOverallStats[iPlayerTeamIndex].iTeamID == teamId || OsdkTeamOverallStats[iPlayerTeamIndex].iTeamID == 0)
                    break;
            for (iOppoTeamIndex = 0; ; iOppoTeamIndex++)
                if (OsdkTeamOverallStats[iOppoTeamIndex].iTeamID == oppoTeamId || OsdkTeamOverallStats[iOppoTeamIndex].iTeamID == 0)
                    break;

            // calculate team weights, do not apply weight for even
            // teams games
            if (false == mEvenTeams)
            {
                iTeamSkill = OsdkTeamOverallStats[iPlayerTeamIndex].iTeamOverallRating * TEAM_SCALE;
                iOppoTeamSkill = OsdkTeamOverallStats[iOppoTeamIndex].iTeamOverallRating * TEAM_SCALE;
            }
            else
            {
                iTeamSkill = 0;
                iOppoTeamSkill = 0;
            }

            // add in stadium weight to home team
            if (true == skillInfo->getHome())
                iTeamSkill += OsdkTeamOverallStats[iPlayerTeamIndex].iTeamStadiumRating * STADIUM_SCALE;
            else
                iOppoTeamSkill += OsdkTeamOverallStats[iOppoTeamIndex].iTeamStadiumRating * STADIUM_SCALE;

            int32_t oppoCurrentPoints = oppoSkillInfoIter->second.getCurrentSkillPoints(periodType);
            int32_t iWeightDifference = skillInfo->getWeight() - oppoSkillInfoIter->second.getWeight();
            int32_t iTeamDifference = iOppoTeamSkill - iTeamSkill;

            delta += calcPointsEarned(currentPoints, oppoCurrentPoints, iOwnWL, iWeightDifference,
                                      skillInfo->getSkill()&3, iTeamDifference);
        }

        newSkillPoints = currentPoints;

        // prorate points, if necessary
        if (skillInfo->getDampingPercent() >= 0)
        {
            newSkillPoints += static_cast<int32_t>(delta * (skillInfo->getDampingPercent() / 100.0));
        }

        TRACE_LOG("[GameCommonLobbySkill:" << this << "].getNewLobbySkillPoints() : Entity id=" << entityId << " PeriodType=" << periodType <<
                  " receives new lobby skill points = " << newSkillPoints);
    }

    return newSkillPoints;
}

/*************************************************************************************************/
/*! \brief getNewLobbySkillLevel
    calculate lobby skill level
*/
/*************************************************************************************************/
int32_t GameCommonLobbySkill::getNewLobbySkillLevel(EntityId entityId, ReportConfig* config, int32_t newPoints)
{
    int32_t statLevel = 1;  // Default skill level

    if (NULL != config)
    {
        // Calculate the Skill level based on the skill points
        statLevel = config->getSkillLevel((uint32_t)(newPoints));
    }

    return statLevel;
}

/*************************************************************************************************/
/*! \brief getNewLobbyOppSkillLevel
    calculate lobby opponent skill level
*/
/*************************************************************************************************/
int32_t GameCommonLobbySkill::getNewLobbyOppSkillLevel(EntityId entityId, Stats::StatPeriodType periodType)
{
    int32_t newOppSkillLevel = 0;

    const LobbySkillInfo *skillInfo = getLobbySkillInfo(entityId);
    if (NULL != skillInfo)
    {
        int32_t totalOppSkillLevel = 0;
        int32_t currentOppSkillLevel = skillInfo->getCurrentOppSkillLevel(periodType);

        LobbySkillInfoMap::const_iterator oppoSkillInfoIter = mLobbySkillInfoMap.begin();
        LobbySkillInfoMap::const_iterator oppoSkillInfoEnd = mLobbySkillInfoMap.end();

        for (; oppoSkillInfoIter != oppoSkillInfoEnd; ++oppoSkillInfoIter)
        {
            if (entityId == oppoSkillInfoIter->first)
                continue;

            totalOppSkillLevel += oppoSkillInfoIter->second.getCurrentOppSkillLevel(periodType);
        }

        newOppSkillLevel = currentOppSkillLevel + totalOppSkillLevel;
    }

    return newOppSkillLevel;
}

/*************************************************************************************************/
/*! calcPointsEarned

    \Description
    Calculate number of ranking points to add/subtract based on game result

    \Input iUserPts        - current ranking points for user
    \Input iOppoPts        - current ranking points for opponent
    \Input iWlt            - WLT_flag indicating users game result
    \Input iAdj            - number of extra points to add/sub to total
    \Input iSkill          - skill level of game (points modifier)
    \Input iTeamDifference - team skill level difference

    \Output int32_t - Number of points to give to user (positive or negative)

    \Version 1.0 11/20/2002 (GWS) First version
*/
/*************************************************************************************************/
int32_t GameCommonLobbySkill::calcPointsEarned(int32_t iUserPts, int32_t iOppoPts, int32_t iWlt, int32_t iAdj,
                                               int32_t iSkill, int32_t iTeamDifference) const
{
    int32_t iPct[4] = { NEW_SKILL_PCT_0, NEW_SKILL_PCT_1,
                        NEW_SKILL_PCT_2, NEW_SKILL_PCT_3 };

    // Clamp points at the ranking maximum to prevent users from gaining
    // too many points in a single year since there is no longer an upper
    // maximum on points earned due to RPG-style leveling.
    iUserPts = _CLAMP(iUserPts, NEW_RANK_MINIMUM, NEW_RANK_MAXIMUM);
    iOppoPts = _CLAMP(iOppoPts, NEW_RANK_MINIMUM, NEW_RANK_MAXIMUM);

    // number of points to add or subtract
    int32_t iPts = 0;
    int32_t iPtsDifference = iOppoPts - iUserPts;
    double fProbability = (1.0 / (1.0 + pow(10.0, (double) (iPtsDifference+iTeamDifference*NEW_TEAM_SCALE+iAdj*NEW_ADJUSTMENT_SCALE) / NEW_PROBABILITY_SCALE)) - 0.5) / (2*NEW_PROBABILITY_UNFAIR-1) + 0.5;
    fProbability = _CLAMP(fProbability, 0.0, 1.0);
    double fFade = pow(1.0 - (double)(iUserPts < iOppoPts ? iUserPts : iOppoPts) / NEW_RANK_MAXIMUM, NEW_FADE_POWER);
    int32_t iPtsRange = (int32_t) (NEW_POINTS_RANGE * abs(iPtsDifference) * pow((double) (abs(iPtsDifference)) / NEW_RANK_MAXIMUM, NEW_RANGE_POWER));
    iPtsRange = _CLAMP(iPtsRange, NEW_CHANGE_MINIMUM, NEW_CHANGE_MAXIMUM);
    iPtsRange = (int32_t) (fFade * iPtsRange);
    iPtsRange = _CLAMP(iPtsRange, 1, NEW_CHANGE_MAXIMUM);

    if (iWlt & (WLT_LOSS|WLT_DISC|WLT_CHEAT|WLT_CONCEDE))
    {
        iPts = (int32_t) (-fProbability * iPtsRange);
        iPts = iPts * iPct[iSkill] / 100;
        if (iPtsDifference < -NEW_POINTS_GUARANTEE)
            iPts = _CLAMP(iPts, -NEW_CHANGE_MAXIMUM, -1);
        if (iPts > 0)
            iPts = 0;
    }
    else if (iWlt & WLT_WIN)
    {
        iPts = (int32_t) ((1.0 - fProbability) * iPtsRange);
        iPts = iPts * iPct[iSkill] / 100;
        if (iPtsDifference > NEW_POINTS_GUARANTEE)
            iPts = _CLAMP(iPts, 1, NEW_CHANGE_MAXIMUM);
        if (iPts < 0)
            iPts = 0;
    }

    // limit the points adjustments to valid range
    // We don't need to clamp the upper maximum points because there
    // is no longer an upper max due to the RPG-style levelling system.
    if (iUserPts + iPts < NEW_RANK_MINIMUM)
        iPts = NEW_RANK_MINIMUM - iUserPts;
    if (iOppoPts - iPts < NEW_RANK_MINIMUM)
        iPts = NEW_RANK_MINIMUM + iOppoPts;

    return iPts;
}

}   // namespace GameReporting

}   // namespace Blaze

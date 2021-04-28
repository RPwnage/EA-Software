/*************************************************************************************************/
/*!
    \file   lobbyskill.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "lobbyskill.h"

namespace Blaze
{
namespace GameReporting
{

LobbySkill::LobbySkill(bool evenTeams)
    : mEvenTeams(evenTeams)
{
}

LobbySkill::~LobbySkill()
{
    mLobbySkillInfoMap.clear();
}

void LobbySkill::addLobbySkillInfo(EntityId entityId, int32_t wlt, int32_t dampingPercent, int32_t teamId,
                                   int32_t weight, bool home, int32_t skillLevel, int32_t currentPoints)
{
    TRACE_LOG("[LobbySkill].addLobbySkillInfo() : Entity id=" << entityId << ", wlt=" << wlt << ", dampingPercent=" << dampingPercent 
              << ", teamId=" << teamId << ", weight=" << weight << ", home=" << (home ? 1 : 0) << ", skillLevel=" << skillLevel << ", currentPoints=" 
              << currentPoints );
    mLobbySkillInfoMap[entityId].set(wlt, dampingPercent, teamId, weight, home, skillLevel, currentPoints);
}

int32_t LobbySkill::getNewLobbySkillPoints(EntityId entityId)
{
    int32_t newSkillPoints = 0;

    const LobbySkillInfo *skillInfo = getLobbySkillInfo(entityId);
    if (skillInfo != nullptr)
    {
        int32_t delta = 0;
        int32_t iPlayerTeamIndex, iOppoTeamIndex;
        int32_t iTeamSkill = 0, iOppoTeamSkill = 0;
        int32_t currentPoints = skillInfo->getCurrentPoints();
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
                if (TeamOverallStats[iPlayerTeamIndex].iTeamID == teamId || TeamOverallStats[iPlayerTeamIndex].iTeamID == 0)
                    break;
            for (iOppoTeamIndex = 0; ; iOppoTeamIndex++)
                if (TeamOverallStats[iOppoTeamIndex].iTeamID == oppoTeamId || TeamOverallStats[iOppoTeamIndex].iTeamID == 0)
                    break;

            // calculate team weights, do not apply weight for even
            // teams games
            if (mEvenTeams)
            {
                iTeamSkill = TeamOverallStats[iPlayerTeamIndex].iTeamOverallRating * TEAM_SCALE;
                iOppoTeamSkill = TeamOverallStats[iOppoTeamIndex].iTeamOverallRating * TEAM_SCALE;
            }

            // add in stadium weight to home team
            if (skillInfo->getHome())
                iTeamSkill += TeamOverallStats[iPlayerTeamIndex].iTeamStadiumRating * STADIUM_SCALE;
            else
                iOppoTeamSkill += TeamOverallStats[iOppoTeamIndex].iTeamStadiumRating * STADIUM_SCALE;

            int32_t oppoCurrentPoints = oppoSkillInfoIter->second.getCurrentPoints();
            int32_t iWeightDifference = skillInfo->getWeight() - oppoSkillInfoIter->second.getWeight();
            int32_t iTeamDifference = iOppoTeamSkill - iTeamSkill;

            delta += calcPointsEarned(currentPoints, oppoCurrentPoints, iOwnWL, iWeightDifference,
                skillInfo->getSkillLevel()&3, iTeamDifference);
        }

        newSkillPoints = currentPoints;

        // prorate points, if necessary
        if (skillInfo->getDampingPercent() >= 0)
        {
            newSkillPoints += static_cast<int32_t>(delta * (skillInfo->getDampingPercent() / 100.0));
        }

        TRACE_LOG("[LobbySkill].getNewLobbySkillPoints() : Entity id=" << entityId << " receives new lobby skill points = " << newSkillPoints);
    }

    return newSkillPoints;
}

/*************************************************************************************************/
/*!
    calcPointsEarned

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
int32_t LobbySkill::calcPointsEarned(int32_t iUserPts, int32_t iOppoPts, int32_t iWlt, int32_t iAdj,
                                     int32_t iSkill, int32_t iTeamDifference) const
{
    int32_t iPct[4] = { NEW_SKILL_PCT_0, NEW_SKILL_PCT_1,
        NEW_SKILL_PCT_2, NEW_SKILL_PCT_3 };

    // Clamp points at the ranking maximum to prevent users from gaining
    // too many points in a single year since there is no longer an upper
    // maximum on points earned due to RPG-style levelling.
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

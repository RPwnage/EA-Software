/*************************************************************************************************/
/*!
    \file   glickoskill.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "glickoskill.h"

namespace Blaze
{
namespace GameReporting
{

GlickoSkill::GlickoSkill(int32_t skillValue)
    : mMaxSkillValue(skillValue)
{
}

GlickoSkill::~GlickoSkill()
{
    mGlickoSkillInfoMap.clear();
}

void GlickoSkill::addGlickoSkillInfo(EntityId entityId, int32_t wlt, int32_t dampingPercent, int32_t teamId,
                                     int32_t periodsPassed, int32_t currentGlickoSkill, int32_t currentGlickoRd)
{
    double onsetRd = calcOnsetRd(currentGlickoRd, periodsPassed);
    TRACE_LOG("[GlickoSkill].addGlickoSkillInfo() : Entity id=" << entityId << ", wlt=" << wlt << ", dampingPercent=" << dampingPercent 
              << ", teamId=" << teamId << ", periodsPassed=" << periodsPassed << ", onsetRd=" << onsetRd << ", currentGlickoSkill=" << currentGlickoSkill 
              << ", currentGlickoRd=" << currentGlickoRd);
    mGlickoSkillInfoMap[entityId].set(wlt, dampingPercent, teamId, onsetRd, currentGlickoSkill, currentGlickoRd);
}

void GlickoSkill::getNewGlickoSkillAndRd(EntityId entityId, int32_t &newGlickoSkill, int32_t &newGlickoRd)
{
    const GlickoSkillInfo *skillInfo = getGlickoSkillInfo(entityId);
    if (skillInfo == nullptr)
    {
        return;
    }

    int32_t pointsDelta = 0;
    int32_t rdDelta = 0;
    calcSkillPointAndRdChange(skillInfo, pointsDelta, rdDelta);

    TRACE_LOG("[GlickoSkill].getNewGlickoSkillAndRd() : Entity id=" << entityId << ", pointsDelta=" << pointsDelta << ", rdDelta=" << rdDelta);

    if (skillInfo->getWLT() & WLT_WIN)
    {
        if (pointsDelta < GLICKO_MINIMUM_POINTS_GAIN)
        {
            pointsDelta = GLICKO_MINIMUM_POINTS_GAIN;
        }
    }

    pointsDelta = static_cast<int32_t>(pointsDelta * (skillInfo->getDampingPercent() / 100.0f));
    newGlickoSkill = skillInfo->getCurrentSkill() + pointsDelta;

    if (mMaxSkillValue > 0 && newGlickoSkill > mMaxSkillValue)
    {
        newGlickoSkill = mMaxSkillValue;
    }

    newGlickoRd = skillInfo->getCurrentRd() + rdDelta;

    TRACE_LOG("[GlickoSkill].getNewGlickoSkillAndRd() : Entity id=" << entityId << " receives new glicko skill points = " 
              << newGlickoSkill << ", and new glicko rd = " << newGlickoRd);
}

/****************************************************************************/
/*!
    \brief calcOnsetRd

    Calculate user's onset RD
    RD = max( min( sqrt(RD^2 + c^2*t), DEVIATION_CEILING) , DEVIATION_FLOOR)

    \param[in] userRd                - current RD points for user
    \param[in] periodsSinceLastRated - number of rating periods since last match

    \return user's onset RD
*/
/****************************************************************************/
double GlickoSkill::calcOnsetRd(int32_t userRd, int32_t periodsSinceLastRated) const
{
    //calculate onset RD for opponents
    // RD = max( min( sqrt(RD^2 + c^2*t), DEVIATION_CEILING) , DEVIATION_FLOOR)
    double onsetRd = static_cast<double>(userRd);
    onsetRd = sqrt( (onsetRd * onsetRd) + GLICKO_C_SQUARED * periodsSinceLastRated);
    onsetRd = eastl::max<double>( eastl::min<double>( onsetRd, GLICKO_DEVIATION_CEILING), GLICKO_DEVIATION_FLOOR);
    return onsetRd;
}

double GlickoSkill::determineGameResult(int32_t wlt, int32_t oppWlt) const
{
    if (wlt == oppWlt)
    {
        return GLICKO_TIE;
    }
    else if (wlt & WLT_WIN)
    {
        return GLICKO_WIN;
    }
    else
    {
        return GLICKO_LOSS;
    }
}

/*************************************************************************************************/
/*!
    \brief calcSkillPointAndRdChange

    Calculate number of ranking points to add/subtract based on game result

    \param[in\out] userPts      - skill value for user's original ranking score and delta
    \param[in\out] userRd       - skill value for user's original rd and delta
    \param[in] userOnsetRd      - user's onset RD
    \param[in] opponentInfoList - list of opponents, their onset RDs, skill ratings and game results

*/
/*************************************************************************************************/
void GlickoSkill::calcSkillPointAndRdChange(const GlickoSkillInfo *skillInfo, int32_t &pointsDelta, int32_t &rdDelta) const
{
    if (skillInfo == nullptr)
    {
        return;
    }

    double dSquared = 0;
    double sum = 0;

    double userOnsetRd = skillInfo->getOnsetRd();
    int32_t currentPoints = skillInfo->getCurrentSkill();
    int32_t currentRd = skillInfo->getCurrentRd();

    GlickoSkillInfoMap::const_iterator oppoSkillInfoIter = mGlickoSkillInfoMap.begin();
    GlickoSkillInfoMap::const_iterator oppoSkillInfoEnd = mGlickoSkillInfoMap.end();
    for (; oppoSkillInfoIter != oppoSkillInfoEnd; ++oppoSkillInfoIter)
    {
        if (skillInfo->getTeamId() == oppoSkillInfoIter->second.getTeamId())
            continue;

        double oppoRd = oppoSkillInfoIter->second.getOnsetRd();
        double oppoPoints = oppoSkillInfoIter->second.getCurrentSkill();

        double gRD = 1 / sqrt(1 + (3 * GLICKO_Q_SQUARED * (oppoRd * oppoRd)) / GLICKO_PI_SQUARED);
        double eResult = 1 / (1 + pow(10, -gRD * (currentPoints - oppoPoints) / 400));

        double oppoResult = determineGameResult(skillInfo->getWLT(), oppoSkillInfoIter->second.getWLT());

        sum += gRD * (oppoResult - eResult);
        dSquared += (gRD * gRD) * eResult * (1 - eResult);
    }

    dSquared = pow(dSquared * GLICKO_Q_SQUARED, -1);

    pointsDelta = (int32_t)ceil((GLICKO_Q / ( (1/(userOnsetRd * userOnsetRd) + 1/dSquared )) * sum));
    //for storage we don't need the fractional part of the RD
    rdDelta = (int32_t)sqrt( pow( 1/(userOnsetRd * userOnsetRd) + 1/dSquared, -1)) - currentRd;
    // limit the points adjustments to valid range
    // We don't need to clamp the upper maximum points because there
    // is no longer an upper max due to the RPG-style levelling system.
    if (currentPoints + pointsDelta < GLICKO_NEW_RATING_MINIMUM)
        pointsDelta = GLICKO_NEW_RATING_MINIMUM - currentPoints;
    // cap the RD change
    if (rdDelta + currentRd < GLICKO_DEVIATION_FLOOR)
        rdDelta = GLICKO_DEVIATION_FLOOR - currentRd;
    if (rdDelta + currentRd > GLICKO_DEVIATION_CEILING)
        rdDelta = GLICKO_DEVIATION_CEILING - currentRd;
}

}   // namespace GameReporting

}   // namespace Blaze

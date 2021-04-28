/*************************************************************************************************/
/*!
    \file   reportconfig.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/util/locales.h"
#include "reportconfig.h"

namespace Blaze
{
namespace GameReporting
{

// ReportConfig does not exist in the OSDKGameReportBase namespace, but uses types from it
using namespace OSDKGameReportBase;

/*! ****************************************************************************/
/*! \brief Creating the ReportConfig.

    \return ReportConfig pointer
********************************************************************************/
ReportConfig *ReportConfig::createReportConfig(const OSDKCustomGlobalConfig* customConfig)
{
    return new ReportConfig(customConfig);
}

/*************************************************************************************************/
/*! \brief ReportConfig
    Constructor
*/
/*************************************************************************************************/
ReportConfig::ReportConfig(const OSDKCustomGlobalConfig* customConfig) :
    mSkillMap()
{
    {
        const OSDKCustomGlobalConfig::Uint32_tList& skillTableList = customConfig->getSkillTables();
        OSDKCustomGlobalConfig::Uint32_tList::const_iterator skill_itr = skillTableList.begin();
        OSDKCustomGlobalConfig::Uint32_tList::const_iterator skill_itrEnd = skillTableList.end();
        uint16_t level = 1;
        for (; skill_itr != skill_itrEnd; ++skill_itr)
        {
            uint32_t points = (*skill_itr);
            mSkillMap.insert(eastl::make_pair(points, level));
            ++level;
        }
    }

    {
        const OSDKCustomGlobalConfig::OSDKCustomEndPhaseConfigList& endPhaseList = customConfig->getEndPhases();
        OSDKCustomGlobalConfig::OSDKCustomEndPhaseConfigList::const_iterator endPhase_itr = endPhaseList.begin();
        OSDKCustomGlobalConfig::OSDKCustomEndPhaseConfigList::const_iterator endPhase_itrEnd = endPhaseList.end();
        for (; endPhase_itr != endPhase_itrEnd; ++endPhase_itr)
        {
            const OSDKCustomEndPhaseConfig* endPhaseConfig = (*endPhase_itr);

            uint16_t phase = endPhaseConfig->getPhase();
            if (phase > 0 && phase < MAX_END_PHASE)
            {
                mMaxTime[phase - 1] = endPhaseConfig->getMaxTime();
            }
        }
    }
}

/*************************************************************************************************/
/*! \brief ReportConfig
    Destructor
*/
/*************************************************************************************************/
ReportConfig::~ReportConfig()
{
}

/*************************************************************************************************/
/*! \brief getSkillLevel

    \param skill points
    \return event skill level
*/
/*************************************************************************************************/
uint16_t ReportConfig::getSkillLevel(uint32_t skillPoints)
{
	SkillMap::iterator levelIter = mSkillMap.upper_bound(skillPoints);
	uint16_t level = (mSkillMap.begin())->second;

	if ((--levelIter) != mSkillMap.end())
    {
		level = levelIter->second;
    }

	return level;
}


uint32_t ReportConfig::getNextSkillLevelPoints(uint32_t skillPoints)
{
	SkillMap::iterator levelIter = mSkillMap.upper_bound(skillPoints);
	uint32_t points = (mSkillMap.end())->first;

	if (levelIter != mSkillMap.end())
		points = levelIter->first;

	return points;
}

uint32_t ReportConfig::getPrevSkillLevelPoints(uint32_t skillPoints)
{
	SkillMap::iterator levelIter = mSkillMap.upper_bound(skillPoints);
	uint32_t points = (mSkillMap.begin())->first;

	if ((--levelIter) != mSkillMap.end())
		points = levelIter->first;

	return points;
}

} //namespace GameReporting
} //namespace Blaze

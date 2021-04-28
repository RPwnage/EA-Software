/*************************************************************************************************/
/*!
    \file   seasonrollover.cpp

    $Header$
    $Change$
    $DateTime$

    \attention
        (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// seasonal play includes
#include "customcomponent/fifacups/fifacupsslaveimpl.h"
//#include "../../../component/gamereportinglegacy/util/updatestatsutil.h"
#include "fifacups/tdf/fifacupstypes_server.h"

#include "logclubnewsutil.h"

// stats includes
#include "component/stats/statsconfig.h"
#include "component/stats/statsslaveimpl.h"



namespace Blaze
{
namespace FifaCups
{

void FifaCupsSlaveImpl::processWinners(eastl::vector<MemberInfo>& wonMembers, const SeasonConfigurationServer *pSeasonConfig)
{
	eastl::vector<MemberInfo>::iterator iter, end;
	iter = wonMembers.begin();
	end = wonMembers.end();

	//GameReportingLegacy::UpdateStatsUtil updateStatsUtil;
	//GameReportingLegacy::UpdateStatsUtil updateStatsUtil;
	//const char8_t *userStatsCategory = "SPOverallPlayerStats";
	char8_t userStatsName[12];

	eastl::map<uint32_t, uint32_t> divMap;
	SeasonConfigurationServer::CupList::const_iterator cupIter = pSeasonConfig->getCups().begin();
	SeasonConfigurationServer::CupList::const_iterator cupEnd = pSeasonConfig->getCups().end();
	for (; cupIter != cupEnd; cupIter++)
	{
		const Cup* cup = *cupIter;
		uint32_t cupId = cup->getCupId();
		for (uint32_t i = cup->getStartDiv(); i <= cup->getEndDiv(); i++)
		{
			divMap.insert(eastl::map<uint32_t, uint32_t>::value_type(i, cupId));
		}
	}

	for (; iter != end; iter++)
	{
		MemberInfo* member = iter;
		if (member->mMemberType == FIFACUPS_MEMBERTYPE_USER)
		{

			switch (divMap[member->mQualifyingDiv])
			{
			case 1:				
				blaze_strnzcpy(userStatsName, "cupsWon1", sizeof(userStatsName));
				break;
			case 2:				
				blaze_strnzcpy(userStatsName, "cupsWon2", sizeof(userStatsName));
				break;
			case 3:
				blaze_strnzcpy(userStatsName, "cupsWon3", sizeof(userStatsName));
				break;
			case 4:
				blaze_strnzcpy(userStatsName, "cupsWon4", sizeof(userStatsName));
				break;
			default:
				blaze_strnzcpy(userStatsName, "cupsWon4", sizeof(userStatsName));
				break;
			}

			//updateStatsUtil.startStatRow(userStatsCategory, member->mMemberId, 0);
			//updateStatsUtil.incrementStat(userStatsName, 1);
			//updateStatsUtil.completeStatRow();
		}
	}

	//updateStatsUtil.updateStats();
}

void FifaCupsSlaveImpl::processLosers(eastl::vector<MemberInfo>& lossMembers, const SeasonConfigurationServer *pSeasonConfig)
{
	eastl::vector<MemberInfo>::iterator iter, end;
	iter = lossMembers.begin();
	end = lossMembers.end();

	//GameReportingLegacy::UpdateStatsUtil updateStatsUtil;
	//const char8_t *userStatsCategory = "SPOverallPlayerStats";
	char8_t userStatsName[12];
	
	eastl::map<uint32_t, uint32_t> divMap;
	SeasonConfigurationServer::CupList::const_iterator cupIter = pSeasonConfig->getCups().begin();
	SeasonConfigurationServer::CupList::const_iterator cupEnd = pSeasonConfig->getCups().end();
	for (; cupIter != cupEnd; cupIter++)
	{
		const Cup* cup = *cupIter;
		uint32_t cupId = cup->getCupId();
		for (uint32_t i = cup->getStartDiv(); i <= cup->getEndDiv(); i++)
		{
			divMap.insert(eastl::map<uint32_t, uint32_t>::value_type(i, cupId));
		}
	}

	for (; iter != end; iter++)
	{
		MemberInfo* member = iter;
		if (member->mMemberType == FIFACUPS_MEMBERTYPE_USER)
		{
			switch (divMap[member->mQualifyingDiv])
			{
			case 1:				
				blaze_strnzcpy(userStatsName, "cupsElim1", sizeof(userStatsName));
				break;
			case 2:				
				blaze_strnzcpy(userStatsName, "cupsElim2", sizeof(userStatsName));
				break;
			case 3:				
				blaze_strnzcpy(userStatsName, "cupsElim3", sizeof(userStatsName));
				break;
			case 4:
				blaze_strnzcpy(userStatsName, "cupsElim4", sizeof(userStatsName));
				break;
			default:				
				blaze_strnzcpy(userStatsName, "cupsElim4", sizeof(userStatsName));
				break;
			}

			//updateStatsUtil.startStatRow(userStatsCategory, member->mMemberId, 0);
			//updateStatsUtil.incrementStat(userStatsName, 1);
			//updateStatsUtil.completeStatRow();
		}
	}

	//updateStatsUtil.updateStats();
}

} // namespace FifaCups
} // namespace Blaze


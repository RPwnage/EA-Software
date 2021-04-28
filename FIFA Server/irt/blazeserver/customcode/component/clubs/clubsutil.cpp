/*************************************************************************************************/
/*!
    \file   clubsutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "stats/updatestatsrequestbuilder.h"

namespace Blaze
{
namespace Clubs
{

void ClubsUtil::SetClubMemberAsFreeAgent(Stats::UpdateStatsRequestBuilder* statsBuilder, EntityId blazeId, bool isFreeAgent)
{
	// Save the current string columns in the aggregate row so we don't lose it when we update free agency	
	Stats::StatNameValueMap prevVproInfo;
	GetClubMemberStringData(blazeId, prevVproInfo);

	Stats::ScopeNameValueMapList keyScopePairList;

	// Populate keyscope list
	for (int32_t i = 1; i <= MAX_POS_KEYSCOPE_VALUE; ++i)
	{
		Stats::ScopeNameValueMap indexMap;
		indexMap.insert(eastl::make_pair(CLUBOTPPLYRSTAT_KEYSCOPE, i));
		keyScopePairList.push_back();
		keyScopePairList.back().assign(indexMap.begin(), indexMap.end());
	}

	// Update free agent columns.
	UpdateFreeAgentsColumn(prevVproInfo, statsBuilder, blazeId, keyScopePairList, isFreeAgent);
}

void ClubsUtil::GetClubMemberStringData(EntityId blazeId, Stats::StatNameValueMap& nameValueMap)
{
	const char8_t *vproInfoStatGroup = "VProInfo";
	
	Stats::StatsSlaveImpl* statsComponent = static_cast<Stats::StatsSlaveImpl*>(gController->getComponent(Stats::StatsSlaveImpl::COMPONENT_ID, false));

	if (statsComponent == nullptr)
	{
		ERR_LOG("[ClubsUtil].GetClubMemberStringData() - statsComponent == NULL");
	}
	else
	{
		Stats::GetStatGroupRequest statGroupRequest;
		statGroupRequest.setName(vproInfoStatGroup);
		
		Stats::StatGroupResponse statGroupResponse;
		BlazeRpcError error = statsComponent->getStatGroup(statGroupRequest, statGroupResponse);
		if(error != Blaze::ERR_OK)
		{
			ERR_LOG("[ClubsUtil].GetClubMemberStringData() - getStatGroup Failed " << error << " " );
			return;
		}
		
		Stats::GetStatsByGroupRequest request;
		request.setGroupName(vproInfoStatGroup);
		request.getEntityIds().push_back(blazeId);
		request.setPeriodId(Stats::STAT_PERIOD_ALL_TIME);

		Stats::GetStatsResponse response;
		error = statsComponent->getStatsByGroup(request, response);

		if(error == Blaze::ERR_OK)
		{
			Stats::KeyScopeStatsValueMap::const_iterator it = response.getKeyScopeStatsValueMap().begin();
			Stats::KeyScopeStatsValueMap::const_iterator itEnd = response.getKeyScopeStatsValueMap().end();
			
			for(; it != itEnd; ++it)
			{
				const Stats::StatValues* statValues = it->second;
				
				if (statValues != NULL)
				{
					const Stats::StatValues::EntityStatsList &entityList = statValues->getEntityStatsList();
					if (entityList.size() > 0)
					{
						Stats::StatValues::EntityStatsList::const_iterator entityItr = entityList.begin();
						Stats::StatValues::EntityStatsList::const_iterator entityItrEnd = entityList.end();
						for (; entityItr != entityItrEnd; ++entityItr)
						{
							if ((*entityItr) != nullptr)
							{
								const Stats::StatGroupResponse::StatDescSummaryList &statDescList = statGroupResponse.getStatDescs();
								const Stats::EntityStats::StringStatValueList &statList = (*entityItr)->getStatValues();

								Stats::StatGroupResponse::StatDescSummaryList::const_iterator statDescItr = statDescList.begin();
								Stats::StatGroupResponse::StatDescSummaryList::const_iterator statDescItrEnd = statDescList.end();

								Stats::EntityStats::StringStatValueList::const_iterator statValuesItr = statList.begin();
								Stats::EntityStats::StringStatValueList::const_iterator statValuesItrEnd = statList.end();

								for(; (statDescItr != statDescItrEnd) && (statValuesItr != statValuesItrEnd); ++statDescItr, ++statValuesItr)
								{
									if (((*statDescItr) != nullptr) &&( (*statValuesItr).length() > 0))
									{
										nameValueMap.insert(eastl::make_pair(Stats::StatName((*statDescItr)->getName()), *statValuesItr));
										TRACE_LOG("[ClubsUtil].GetClubMemberStringData() - Stats Name = " << (*statDescItr)->getName() << ", Stats = " << *statValuesItr);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void ClubsUtil::UpdateFreeAgentsColumn(Stats::StatNameValueMap &nameValueMap, Stats::UpdateStatsRequestBuilder* statsBuilder, EntityId blazeId, Stats::ScopeNameValueMapList &keyScopeList, bool isFreeAgent)
{
	int64_t freeAgentValue = isFreeAgent ? 1 : 0;

	Stats::ScopeNameValueMapList::const_iterator it = keyScopeList.begin();
	Stats::ScopeNameValueMapList::const_iterator itEnd = keyScopeList.end();

	for(; it != itEnd; it++)
	{
		statsBuilder->startStatRow(CLUBOTPPLYRSTAT_CAT, blazeId, &(*it));
		
		// override all rows with latest string data
		if(!nameValueMap.empty())
		{
			statsBuilder->assignStat(STATNAME_PRONAME, nameValueMap[STATNAME_PRONAME].c_str());
			statsBuilder->assignStat(STATNAME_PROPOS, nameValueMap[STATNAME_PROPOS].c_str());
			statsBuilder->assignStat(STATNAME_PROSTYLE, nameValueMap[STATNAME_PROSTYLE].c_str());
			statsBuilder->assignStat(STATNAME_PROHEIGHT, nameValueMap[STATNAME_PROHEIGHT].c_str());
			statsBuilder->assignStat(STATNAME_PRONATION, nameValueMap[STATNAME_PRONATION].c_str());
			statsBuilder->assignStat(STATNAME_PROOVR, nameValueMap[STATNAME_PROOVR].c_str());

			TRACE_LOG("[ClubsUtil] - UpdateFreeAgentsColumn - proName:" << nameValueMap[STATNAME_PRONAME] << 
				" proPos:" << nameValueMap[STATNAME_PROPOS].c_str() <<
				" proStyle:" << nameValueMap[STATNAME_PROSTYLE].c_str() <<
				" proHeight:" << nameValueMap[STATNAME_PROHEIGHT].c_str() <<
				" proNation:" << nameValueMap[STATNAME_PRONATION].c_str() <<
				" proOverall:" << nameValueMap[STATNAME_PROOVR].c_str() << " "
				);
		}
		
		statsBuilder->assignStat(STATNAME_ISFREEAGENT, freeAgentValue);
		statsBuilder->completeStatRow();
	}
}

} // Clubs
} // Blaze


/*************************************************************************************************/
/*!
    \file   fifastatsvalueutil.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFASTATSVALUEUTIL_H
#define BLAZE_CUSTOM_FIFASTATSVALUEUTIL_H

/*** Include files *******************************************************************************/
#include "EABase/eabase.h"
#include "framework/blaze.h"
#include "gamereporting/gamereportingslaveimpl.h"
#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"
//#include "stats/stat"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{

namespace StatValueUtil
{

	struct StatValue
	{
		StatValue()
		{
			for (int32_t i = 0; i < Stats::STAT_NUM_PERIODS; ++i)
			{
				iBefore[i] = 0;
				iAfter[i] = 0;
			}
		}

		int64_t iBefore[Stats::STAT_NUM_PERIODS]; /* \brief Stat value obtained from the database before it is transformed */
		int64_t iAfter[Stats::STAT_NUM_PERIODS];  /* \brief Stat value that will be written out after it is transformed*/
	};

	typedef eastl::map<const char8_t*, StatValue, CaseSensitiveStringLessThan> StatValueMap;

	/* \brief Per/StatRowUpdate transform object that tracks stats affected by the transform */
	struct StatUpdate
	{
		StatValueMap stats;
	};
	typedef eastl::vector_map<Stats::UpdateRowKey, StatUpdate> StatUpdateMap;
	
	inline StatValue* GetStatValue(StatValueMap* statValueMap, const char8_t * statName)
	{
		StatValueMap::iterator svIter = statValueMap->find(statName);
		if (svIter == statValueMap->end()) 
		{ 
			ERR("[StatValueUtil].GetStatValue(): Cannot find statName %s", statName); 
			return NULL; 
		}
		StatValue* svStat = &(svIter->second);
		return svStat;
	}

	inline void insertStat(StatValueUtil::StatUpdate& statUpdate, const char8_t* name, Stats::StatPeriodType periodType, int64_t value)
	{
		StatValue statValue;
		statValue.iBefore[periodType] = value;
		statValue.iAfter[periodType] = statValue.iBefore[periodType];

		statUpdate.stats.insert(eastl::make_pair(name, statValue));

		TRACE_LOG("[FifaSeasonalPlay].insertStat() - Stat Name:" << name <<  
															" Period:" << periodType << 
															" Value:" << value << 
															" ");
	}

	inline int32_t getInt(const Collections::AttributeMap *map, const char *key)
	{
		Collections::AttributeMap::const_iterator iter = map->find(key);
		if(iter != map->end())
		{
			int val;
			if(sscanf(iter->second, "%d", &val))
			{
				return (int32_t)val;
			}
		}
		return 0;
	}

}	// namespace StatsValueUtil
}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFASTATSVALUEUTIL_H


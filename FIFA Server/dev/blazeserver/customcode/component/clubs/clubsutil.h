/*************************************************************************************************/
/*!
    \file   clubsutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_COMPONENT_CLUBS_UTIL_H
#define BLAZE_COMPONENT_CLUBS_UTIL_H

namespace Blaze
{
	namespace Stats
	{
		class UpdateStatsRequestBuilder;
		//typedef eastl::map<StatName, StatValue> StatNameValueMap;
		typedef eastl::list<ScopeNameValueMap> ScopeNameValueMapList;
	}

	namespace Clubs
	{
// VPro stat names
#define CLUBOTPPLYRSTAT_CAT			"ClubOTPPlayerStats"
#define CLUBOTPPLYRSTAT_KEYSCOPE	"pos"

#define STATNAME_PRONAME			"proName"
#define STATNAME_PROPOS				"proPos"
#define STATNAME_PROSTYLE			"proStyle"
#define STATNAME_PROHEIGHT			"proHeight"
#define STATNAME_PRONATION			"proNationality"
#define STATNAME_PROOVR				"proOverallStr"
#define STATNAME_ISFREEAGENT		"isFreeAgent"

		class ClubsUtil
		{
		public:
			ClubsUtil(){}
			virtual ~ClubsUtil(){}

			void SetClubMemberAsFreeAgent(Stats::UpdateStatsRequestBuilder* statsBuilder, EntityId entityId, bool isFreeAgent);

		private:
			enum
			{
				MAX_POS_KEYSCOPE_VALUE = 5
			};

			void GetClubMemberStringData(EntityId entityId, Stats::StatNameValueMap &nameValueMap);
			void UpdateFreeAgentsColumn(Stats::StatNameValueMap &nameValueMap, Stats::UpdateStatsRequestBuilder* statsBuilder, EntityId blazeId, Stats::ScopeNameValueMapList &keyScopeList, bool isFreeAgent);
		};

	} // Clubs
} // Blaze

#endif


/*************************************************************************************************/
/*!
    \file   fifa_elo_rpghybridskillextensions.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_ELO_RPGHYBRIDSKILLEXTENSIONS_H
#define BLAZE_CUSTOM_FIFA_ELO_RPGHYBRIDSKILLEXTENSIONS_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/tdf/gameosdkreport.h"
#include "gamereporting/osdk/tdf/gameosdkclubreport.h"

#include "customcode/component/gamereporting/fifa/tdf/fifacoopreport.h"

#include "component/stats/tdf/stats.h"

#include "util/updateclubsutil.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{

/*!
    class FifaSeasonalPlayExtension
*/
class IFifaEloRpgHybridSkillExtension
{
public:
	IFifaEloRpgHybridSkillExtension() {}
	virtual ~IFifaEloRpgHybridSkillExtension() {}

	virtual void setExtensionData(void* extensionData) = 0;

	struct ScopePair
	{
		ScopePair()
		{
			name = NULL;
			value = 0;
		}

		const char* name;
		int64_t value;
	};
	struct EntityInfo
	{
		EntityInfo()
		{
			entityId = INVALID_BLAZE_ID;
		}

		EntityId entityId;

		typedef eastl::vector<ScopePair> ScopeList;
		ScopeList scopeList;
	};
	struct CategoryInfo
	{
		CategoryInfo()
		{
			category = NULL;
		}

		const char* category;
		EntityInfo entityA;
		EntityInfo entityB;

	};
	typedef eastl::vector<CategoryInfo> CategoryInfoList;
	virtual void getEloStatUpdateInfo(CategoryInfoList& categoryInfoList) = 0;

	virtual void generateAttributeMap(EntityId id, Collections::AttributeMap& map) = 0;
	virtual bool isClubs() = 0;
};

class HeadtoHeadEloExtension : public IFifaEloRpgHybridSkillExtension
{
public:
	struct ExtensionData
	{
		ProcessedGameReport*	mProcessedReport;

		enum CalculatedStat
		{
			DISCONNECT,
			STAT_PERC,
			RESULT,

			STATS_MAX
		};
		struct CalculatedStats
		{
			int64_t stats[STATS_MAX];
		};
		typedef eastl::map<int64_t, CalculatedStats> CalculatedStatsMap;
		CalculatedStatsMap mCalculatedStats;
	};
	HeadtoHeadEloExtension();
	~HeadtoHeadEloExtension();

	virtual void setExtensionData(void* extensionData);

	virtual void getEloStatUpdateInfo(CategoryInfoList& categoryInfoList);

	virtual void generateAttributeMap(EntityId id, Collections::AttributeMap& map);
	virtual bool isClubs();

private:
	OSDKGameReportBase::OSDKPlayerReport* getOsdkPlayerReport(EntityId entityId);
	char* convertToString(int64_t value, eastl::string& stringResult);

	ExtensionData mExtensionData;
};

class ClubEloExtension : public IFifaEloRpgHybridSkillExtension
{
public:
	struct ExtensionData
	{
		ProcessedGameReport*	mProcessedReport;
		UpdateClubsUtil*		mUpdateClubsUtil;

		enum CalculatedStat
		{
			DISCONNECT,
			STAT_PERC,
			RESULT,

			STATS_MAX
		};
		struct CalculatedStats
		{
			int64_t stats[STATS_MAX];
		};
		typedef eastl::map<int64_t, CalculatedStats> CalculatedStatsMap;
		CalculatedStatsMap mCalculatedStats;
	};
	ClubEloExtension();
	~ClubEloExtension();

	virtual void setExtensionData(void* extensionData);

	virtual void getEloStatUpdateInfo(CategoryInfoList& categoryInfoList);

	virtual void generateAttributeMap(EntityId id, Collections::AttributeMap& map);
	virtual bool isClubs();

private:
	OSDKClubGameReportBase::OSDKClubClubReport* getOsdkClubClubReport(EntityId entityId);
	char* convertToString(int64_t value, eastl::string& stringResult);
	int getNumberOfPlayersPlayed(uint32_t teamId, const OSDKGameReportBase::OSDKReport *osdkreport);

	ExtensionData mExtensionData;
};

class CoopEloExtension : public IFifaEloRpgHybridSkillExtension
{
public:
	struct ExtensionData
	{
		ProcessedGameReport*	mProcessedReport;
		UpdateClubsUtil*		mUpdateClubsUtil;

		enum CalculatedStat
		{
			DISCONNECT,
			STAT_PERC,
			RESULT,

			STATS_MAX
		};
		struct CalculatedStats
		{
			int64_t stats[STATS_MAX];
		};
		typedef eastl::map<int64_t, CalculatedStats> CalculatedStatsMap;
		CalculatedStatsMap mCalculatedStats;
	};
	CoopEloExtension();
	~CoopEloExtension();

	virtual void setExtensionData(void* extensionData);

	virtual void getEloStatUpdateInfo(CategoryInfoList& categoryInfoList);

	virtual void generateAttributeMap(EntityId id, Collections::AttributeMap& map);
	virtual bool isClubs();

private:
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* getSquadDetailsReport(EntityId entityId);
	char* convertToString(int64_t value, eastl::string& stringResult);

	ExtensionData mExtensionData;
};

class LiveCompEloExtension : public IFifaEloRpgHybridSkillExtension
{
public:
	struct ExtensionData
	{
		ProcessedGameReport*	mProcessedReport;
		int						mSponsoredEventId;
		bool					mUseLockedTeam;
		Stats::StatPeriodType	mPeriodType;

		enum CalculatedStat
		{
			DISCONNECT,
			STAT_PERC,
			RESULT,

			STATS_MAX
		};
		struct CalculatedStats
		{
			int64_t stats[STATS_MAX];
		};
		typedef eastl::map<int64_t, CalculatedStats> CalculatedStatsMap;
		CalculatedStatsMap mCalculatedStats;

		ExtensionData()
			: mProcessedReport(NULL)
			, mSponsoredEventId(0)
			, mUseLockedTeam(false)
			, mPeriodType(Stats::STAT_PERIOD_ALL_TIME)
		{
		}
	};

	LiveCompEloExtension();
	~LiveCompEloExtension();

	virtual void setExtensionData(void* extensionData);

	virtual void getEloStatUpdateInfo(CategoryInfoList& categoryInfoList);

	virtual void generateAttributeMap(EntityId id, Collections::AttributeMap& map);
	virtual bool isClubs();

private:
	OSDKGameReportBase::OSDKPlayerReport* getOsdkPlayerReport(EntityId entityId);
	char* convertToString(int64_t value, eastl::string& stringResult);

	ExtensionData mExtensionData;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_ELO_RPGHYBRIDSKILLEXTENSIONS_H


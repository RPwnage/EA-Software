/*************************************************************************************************/
/*!
    \file   vprogrowthunlocksslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_VPROGROWTHUNLOCKS_SLAVEIMPL_H
#define BLAZE_VPROGROWTHUNLOCKS_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave_stub.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksmaster.h"
#include "framework/replication/replicationcallback.h"
#include "vprogrowthunlocks/tdf/vprogrowthunlockstypes.h"
#include "framework/usersessions/usersessionmanager.h"

#include "stats/tdf/stats.h"

#include "avatargrowthcommon.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace VProGrowthUnlocks
{

class VProGrowthUnlocksSlaveImpl : 
	public VProGrowthUnlocksSlaveStub,
	private UserSessionSubscriber
{
public:
    VProGrowthUnlocksSlaveImpl();
    ~VProGrowthUnlocksSlaveImpl();

	// Component Interface
	virtual uint16_t getDbSchemaVersion() const { return 8; }

	void fetchPlayerGrowthConfig(Blaze::VProGrowthUnlocks::PlayerGrowthConfig &outPlayerGrowthConfig, BlazeRpcError &outError);
	void fetchSkillTreeConfig(Blaze::VProGrowthUnlocks::SkillTree &outSkillTree, BlazeRpcError &outError);
	void fetchPerksConfig(Blaze::VProGrowthUnlocks::PerkSlotsMap &outPerkSlots, Blaze::VProGrowthUnlocks::PerksMap &outPerks, Blaze::VProGrowthUnlocks::PerkCategoriesMap &outPerkCategories, BlazeRpcError &outError);
	void fetchLoadOuts(Blaze::VProGrowthUnlocks::UserIdList idList, Blaze::VProGrowthUnlocks::VProLoadOutList &outLoadOutList, BlazeRpcError &outError);
	void fetchObjectiveProgress(Blaze::VProGrowthUnlocks::UserIdList idList, Blaze::VProGrowthUnlocks::VProObjectiveProgressGroupList &outObjectiveProgressGroups, BlazeRpcError &outError);
	void fetchObjectiveConfig(Blaze::VProGrowthUnlocks::VProObjectiveCategoryConfigList &outObjectiveCategoryConfigList, Blaze::VProGrowthUnlocks::SkillPointRewardsMap &outProgressionRewardsMap, BlazeRpcError &outError);
	void resetLoadOuts(BlazeId userId, BlazeRpcError &outError);
	void resetSingleLoadOut(BlazeId userId, int32_t loadOutId, BlazeRpcError &outError);
	void updateLoadOutsPeripherals(BlazeId userId, Blaze::VProGrowthUnlocks::VProLoadOutList &loadOutList, BlazeRpcError &outError);
	void updateLoadOutUnlocks(BlazeId userId, Blaze::VProGrowthUnlocks::VProLoadOut &loadOut, BlazeRpcError &outError);

	void updatePlayerGrowth(BlazeId userId, Blaze::VProGrowthUnlocks::MatchEventsMap &matchEventsMap, BlazeRpcError &outError);

	void grantSkillPointsPostMatch(BlazeId userId, bool grantMatchCountSP, bool grantObjectiveSP, BlazeRpcError &outError);
	void grantSkillPoints(BlazeId userId, uint32_t grantSP, BlazeRpcError &outError);
	void calculateXPRewardsFromMatch(BlazeId userId, uint32_t matchRating, Blaze::VProGrowthUnlocks::MatchEventsMap &matchEventsMap,  Blaze::VProGrowthUnlocks::AttributeTypesMap &attributeTypesMap, Blaze::VProGrowthUnlocks::AttributeXPsMap &attributeXPsMap, BlazeRpcError &outError);

	void completeObjectives(BlazeId userId, Blaze::VProGrowthUnlocks::CategoryIdList categoryList, Blaze::VProGrowthUnlocks::ObjectiveIdList objectiveList, BlazeRpcError &outError);
	void grantMatchRatingXP(BlazeId userId, uint32_t matchCount, uint32_t matchRating, Blaze::VProGrowthUnlocks::AttributeTypeList attributeTypeList, BlazeRpcError &outError);
private:
	virtual bool onConfigure();
	virtual bool onReconfigure();
	virtual void onShutdown();

	//! Called to perform tasks common to onConfigure and onReconfigure
	bool configureHelper();
	void populateInternalConfigCache();
	void setStatsPeriodType();
	bool buildStatsDescIndexMap(const char8_t* statGroupName, Blaze::VProGrowthUnlocks::StatsDescIndexMap& storageMap);
	bool retrieveEntityStats(Blaze::VProGrowthUnlocks::UserIdList idList, const char8_t* statGroupName, Stats::StatPeriodType periodType, Stats::StatValues::EntityStatsList& outEntityStatsList);
	bool loadFile(const eastl::string &filePath, eastl::vector<char> &data);

	uint32_t checkObjectiveProgress(UserIdList idList, VProObjectiveProgressGroupList &outObjectiveProgressGroups, BlazeRpcError &outError, bool setProgress = true);	
	uint32_t checkMatchCount(BlazeId userId, BlazeRpcError &outError);

    //VProGrowthUnlocksMasterListener interface
    virtual void onVoidMasterNotification(UserSession *);

	// event handlers for User Session Manager events
	virtual void onUserSessionExistence(const UserSession& userSession);

	// helper to register a user in proClub SP management
	void autoRegistration(BlazeId userId);
    
    static const int TOTAL_PLAYER_ATTRIBUTE = 34;
	static const int TOTAL_PLAYER_ATTRIBUTE_TYPE = 3;
	static const int DEFAULT_LOADOUT_NAME_MAX_LENGTH = 21;

	uint32_t mDbId;
	uint32_t mMaxLoadOutCount;
	uint32_t mDefaultGrantedSP;
	uint32_t mMaxGrantedSP;
	uint32_t mLastMatchCount;

	uint32_t mModifier[TOTAL_PLAYER_ATTRIBUTE_TYPE];

	Blaze::VProGrowthUnlocks::SkillPointRewardsMap mSkillPointRewardsMap;
	Blaze::VProGrowthUnlocks::MatchRatingXPRewardsMap mMatchRatingXPRewardsMap;
	Blaze::VProGrowthUnlocks::StatsDescIndexMap mObjectiveStatsDescIndexMap;
	Blaze::VProGrowthUnlocks::StatsDescIndexMap mMatchCountStatsDescIndexMap;

	Blaze::VProGrowthUnlocks::VProObjectiveCategoryConfigList mObjectiveCategoryConfigs;	
	Blaze::VProGrowthUnlocks::VProLoadOutList mDefaultLoadOuts;

	EA::TDF::TdfString mObjectiveStatsGroup;
	Stats::StatPeriodType mObjectiveStatsPeriod;

	EA::TDF::TdfString mMatchCountStatsGroup;
	Stats::StatPeriodType mMatchCountStatsPeriod;

	enum
	{
		STATS_TYPE_MATCH_COUNT = 0,
		STATS_TYPE_OBJECTIVE,
		STATS_TYPE_COUNT
	};

	AvatarGrowthCommon mAvatarGrowthCommon;
};

} // VProGrowthUnlocks
} // Blaze

#endif // BLAZE_VPROGROWTHUNLOCKS_SLAVEIMPL_H


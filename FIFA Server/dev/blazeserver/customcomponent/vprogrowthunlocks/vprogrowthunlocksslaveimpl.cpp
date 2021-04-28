/*************************************************************************************************/
/*!
    \file   vprogrowthunlocksslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class VProGrowthUnlocksSlaveImpl

    VProGrowthUnlocks Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocksslaveimpl.h"

#include "EAIO/EAFileStream.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

// vprogrowthunlocks includes
#include "vprogrowthunlocks/rpc/vprogrowthunlocksmaster.h"
#include "vprogrowthunlocks/tdf/vprogrowthunlockstypes.h"
#include "vprogrowthunlocks/tdf/vprogrowthunlockstypes_server.h"

#include "component/stats/statsslaveimpl.h"
#include "component/stats/updatestatshelper.h"
#include "component/stats/updatestatsrequestbuilder.h"

namespace Blaze
{
namespace VProGrowthUnlocks
{

#define		KEYSCOPE_NAME_VPRO_ATTRIBUTE_TYPE	"attributetype"
#define		VPRO_ATTRIBUTE_STATS_CATEGORY	"VProAttribute"
#define		VPRO_ATTRIBUTE_XPEARNED_COLUMN	"xpEarned"
#define		VPRO_OBJECTIVE_STATS_CATEGORY	"VProObjectiveStats"

	// static
VProGrowthUnlocksSlave* VProGrowthUnlocksSlave::createImpl()
{
    return BLAZE_NEW_NAMED("VProGrowthUnlocksSlaveImpl") VProGrowthUnlocksSlaveImpl();
}


/*** Public Methods ******************************************************************************/
VProGrowthUnlocksSlaveImpl::VProGrowthUnlocksSlaveImpl():
	 mDbId(DbScheduler::INVALID_DB_ID),
	 mMaxLoadOutCount(0),
	 mDefaultGrantedSP(0),
	 mMaxGrantedSP(0),
	 mLastMatchCount(0),
	 mAvatarGrowthCommon(Allocator::getAllocator())
{
}

VProGrowthUnlocksSlaveImpl::~VProGrowthUnlocksSlaveImpl()
{
}

bool VProGrowthUnlocksSlaveImpl::onConfigure()
{
	bool success = configureHelper();
	TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].onConfigure: configuration " << (success ? "successful." : "failed."));
	return true;
}

bool VProGrowthUnlocksSlaveImpl::onReconfigure()
{
	// stop listening to user session events
	gUserSessionManager->removeSubscriber(*this);

	bool success = configureHelper();
	TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].onReconfigure: configuration " << (success ? "successful." : "failed."));
	return true;
}

void VProGrowthUnlocksSlaveImpl::onShutdown()
{
	TRACE_LOG("[VProGrowthUnlocksSlaveImpl].onShutdown");

	// stop listening to user session events
	gUserSessionManager->removeSubscriber(*this);

}

bool VProGrowthUnlocksSlaveImpl::configureHelper()
{
	TRACE_LOG("[VProGrowthUnlocksSlaveImpl].configureHelper");

	const char8_t* dbName = getConfig().getDbName();
	mDbId = gDbScheduler->getDbId(dbName);
	if(mDbId == DbScheduler::INVALID_DB_ID)
	{
		ERR_LOG("[VProGrowthUnlocksSlaveImpl].configureHelper(): Failed to initialize db");
		return false;
	}
	mMaxLoadOutCount = getConfig().getMaxLoadOutCount();
	mDefaultGrantedSP = getConfig().getDefaultGrantedSP();
	mMaxGrantedSP = mDefaultGrantedSP;

	mModifier[0] = getConfig().getPrimaryModifier();
	mModifier[1] = getConfig().getSecondaryModifier();
	mModifier[2] = getConfig().getTertiaryModifier();

	mSkillPointRewardsMap.clear();
	uint32_t matchCount = 0;
	uint32_t skillPointReward = 0;
	Blaze::VProGrowthUnlocks::VProGrowthUnlocksConfig::SkillPointRewardList sprlist = getConfig().getSkillPointRewards();
	for ( Blaze::VProGrowthUnlocks::VProGrowthUnlocksConfig::SkillPointRewardList::const_iterator iter = sprlist.begin(); iter != sprlist.end(); iter++)
	{
		matchCount = (*iter)->getMatchCount();
		skillPointReward = (*iter)->getSPReward();
		mSkillPointRewardsMap.insert(eastl::make_pair(matchCount, skillPointReward));
	}
	mLastMatchCount = matchCount;
	mMaxGrantedSP += skillPointReward;
	
	mMatchRatingXPRewardsMap.clear();
	Blaze::VProGrowthUnlocks::VProGrowthUnlocksConfig::MatchRatingXPRewardList mrxprlist = getConfig().getMatchRatingXPRewards();
	for ( Blaze::VProGrowthUnlocks::VProGrowthUnlocksConfig::MatchRatingXPRewardList::const_iterator iter = mrxprlist.begin(); iter != mrxprlist.end(); iter++)
	{
		uint32_t matchRating = (*iter)->getRating();
		uint32_t xpReward = (*iter)->getXPReward();
		mMatchRatingXPRewardsMap.insert(eastl::make_pair(matchRating, xpReward));
	}

	TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].configureHelper mDefaultGrantedSP = " << mDefaultGrantedSP <<" ");
	TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].configureHelper mMaxGrantedSP = " << mMaxGrantedSP <<" ");
	TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].configureHelper mPrimaryModifier = " << mModifier[0] <<" ");
	TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].configureHelper mSecondaryModifier = " << mModifier[1] <<" ");
	TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].configureHelper mTertiaryModifier = " << mModifier[2] <<" ");

	AvatarGrowthCommon::Result result;
	eastl::string configDirectory(eastl::string::CtorSprintf(), "%s/customcomponent/vprogrowthunlocks/", gController->getCurrentWorkingDirectory());

	TRACE_LOG("[VProGrowthUnlocksSlaveImpl].configureHelper(): Loading " << configDirectory << "player_growth.json");

	eastl::vector<char> data;

	result = loadFile(configDirectory + "player_growth.json", data) ? AvatarGrowthCommon::Success : AvatarGrowthCommon::CannotOpenFile;
	if (result == AvatarGrowthCommon::Success)
	{
		result = mAvatarGrowthCommon.loadPlayerGrowthConfigFile(data);
	}
	if (result != AvatarGrowthCommon::Success)
	{
		ERR_LOG("[VProGrowthUnlocksSlaveImpl].configureHelper(): Failed to load player_growth.json - result = " << result);
		return false;
	}

	TRACE_LOG("[VProGrowthUnlocksSlaveImpl].configureHelper(): Loading " << configDirectory << "skill_tree.json");

	result = loadFile(configDirectory + "skill_tree.json", data) ? AvatarGrowthCommon::Success : AvatarGrowthCommon::CannotOpenFile;
	if (result == AvatarGrowthCommon::Success)
	{
		result = mAvatarGrowthCommon.loadSkillTreeConfigFile(data);
	}
	if (result != AvatarGrowthCommon::Success)
	{
		ERR_LOG("[VProGrowthUnlocksSlaveImpl].configureHelper(): Failed to load skill_tree.json - result = " << result);
		return false;
	}

	TRACE_LOG("[VProGrowthUnlocksSlaveImpl].configureHelper(): Loading " << configDirectory << "perks.json");

	result = loadFile(configDirectory + "perks.json", data) ? AvatarGrowthCommon::Success : AvatarGrowthCommon::CannotOpenFile;
	if (result == AvatarGrowthCommon::Success)
	{
		result = mAvatarGrowthCommon.loadPerksConfigFile(data);
	}
	if (result != AvatarGrowthCommon::Success)
	{
		ERR_LOG("[VProGrowthUnlocksSlaveImpl].configureHelper(): Failed to load perks.json - result = " << result);
		return false;
	}

	VProGrowthUnlocksDb::initialize(mDbId);
	// listen for user session events
	gUserSessionManager->addSubscriber(*this); 

	populateInternalConfigCache();
	setStatsPeriodType();

	mObjectiveStatsGroup = getConfig().getObjectiveStatsGroup();
	mMatchCountStatsGroup = getConfig().getMatchCountStatsGroup();
	mObjectiveStatsDescIndexMap.clear();
	mMatchCountStatsDescIndexMap.clear();
	return true;
}

void VProGrowthUnlocksSlaveImpl::populateInternalConfigCache()
{
	mObjectiveCategoryConfigs.clear();

	VProGrowthUnlocksConfig::ObjectiveCategoryServerConfigList objectiveCategoryServerConfigList = getConfig().getObjectiveCategoryServerConfigs();
	for (VProGrowthUnlocksConfig::ObjectiveCategoryServerConfigList::const_iterator ocscIter = objectiveCategoryServerConfigList.begin(); ocscIter != objectiveCategoryServerConfigList.end(); ocscIter++)
	{
		VProObjectiveCategoryConfig* objCategoryConfig = BLAZE_NEW VProObjectiveCategoryConfig();
		objCategoryConfig->setObjectiveCategoryId((*ocscIter)->getObjectiveCategoryId());
		objCategoryConfig->setObjectiveCategoryTitle((*ocscIter)->getObjectiveCategoryTitle());
		objCategoryConfig->setObjectiveCategoryDesc((*ocscIter)->getObjectiveCategoryDesc());

		uint32_t totalReward = 0;

		ObjectiveCategoryServerConfig::ObjectiveServerConfigList objectiveServerConfigList = (*ocscIter)->getObjectiveServerConfigs();
		for (ObjectiveCategoryServerConfig::ObjectiveServerConfigList::const_iterator oscIter = objectiveServerConfigList.begin(); oscIter != objectiveServerConfigList.end(); oscIter++)
		{
			VProObjectiveConfig* objConfig = BLAZE_NEW VProObjectiveConfig();
			objConfig->setObjectiveId((*oscIter)->getObjectiveId());
			objConfig->setObjectiveTitle((*oscIter)->getObjectiveTitle());
			objConfig->setObjectiveDesc((*oscIter)->getObjectiveDesc());
			objConfig->setAssetId((*oscIter)->getAssetId());
			objConfig->setGroup((*oscIter)->getGroup());
			objConfig->setRow((*oscIter)->getRow());
			objConfig->setColumn((*oscIter)->getColumn());
			objConfig->setStatReqVal((*oscIter)->getStatReqVal());
			objConfig->setRewardType((*oscIter)->getRewardType());
			objConfig->setRewardVal((*oscIter)->getRewardVal());

			objCategoryConfig->getObjectiveConfigs().push_back(objConfig);
			totalReward += (*oscIter)->getRewardVal();
		}

		objCategoryConfig->setRewardAvailable(totalReward);
		mObjectiveCategoryConfigs.push_back(objCategoryConfig);
	}
}

void VProGrowthUnlocksSlaveImpl::setStatsPeriodType()
{
	const char8_t* statsPeriodString = nullptr;
	Stats::StatPeriodType* periodTypePtr = nullptr;

	for (int i = 0; i < STATS_TYPE_COUNT; i++)
	{
		switch (i)
		{
		case STATS_TYPE_MATCH_COUNT:
		{
			statsPeriodString = getConfig().getMatchCountStatsPeriod();
			periodTypePtr = &mMatchCountStatsPeriod;
		}
		break;

		case STATS_TYPE_OBJECTIVE:
		{
			statsPeriodString = getConfig().getObjectiveStatsPeriod();
			periodTypePtr = &mObjectiveStatsPeriod;
		}
		break;

		default:
			break;
		}

		*periodTypePtr = Stats::STAT_PERIOD_ALL_TIME;

		if (statsPeriodString != nullptr)
		{
			if (strcmp(statsPeriodString, "STAT_PERIOD_MONTHLY") == 0)
			{
				*periodTypePtr = Stats::STAT_PERIOD_MONTHLY;
			}
			else if (strcmp(statsPeriodString, "STAT_PERIOD_WEEKLY") == 0)
			{
				*periodTypePtr = Stats::STAT_PERIOD_WEEKLY;
			}
			else if (strcmp(statsPeriodString, "STAT_PERIOD_DAILY") == 0)
			{
				*periodTypePtr = Stats::STAT_PERIOD_DAILY;
			}
		}
	}
}

bool VProGrowthUnlocksSlaveImpl::buildStatsDescIndexMap(const char8_t* statGroupName, Blaze::VProGrowthUnlocks::StatsDescIndexMap& storageMap)
{
	bool result = false;
	if (storageMap.size() == 0)
	{
		Stats::StatsSlaveImpl* statsComponent = static_cast<Stats::StatsSlaveImpl*>(gController->getComponent(Stats::StatsSlaveImpl::COMPONENT_ID, false));
		if (statsComponent == nullptr || statGroupName == nullptr)
		{
			ERR_LOG("[VProGrowthUnlocksSlaveImpl].buildStatsDescIndexMap(): Failed to get statsComponent or invalid argument");
		}
		else
		{
			// Get Stat Desc List
			Stats::GetStatGroupRequest statGroupRequest;
			statGroupRequest.setName(statGroupName);

			Stats::StatGroupResponse statGroupResponse;

			BlazeRpcError error = statsComponent->getStatGroup(statGroupRequest, statGroupResponse);
			if (error == Blaze::ERR_OK)
			{
				uint32_t index = 0;
				const Stats::StatGroupResponse::StatDescSummaryList& statDescList = statGroupResponse.getStatDescs();
				Stats::StatGroupResponse::StatDescSummaryList::const_iterator statDescItr = statDescList.begin();

				for (; (statDescItr != statDescList.end()); statDescItr++)
				{
					const char8_t* statName = (*statDescItr)->getName();
					storageMap.insert(eastl::make_pair(statName, index));
					index++;
				}
				result = true;
			}
		}
	}
	else
	{
		result = true;
	}		
	return result;
}

bool VProGrowthUnlocksSlaveImpl::retrieveEntityStats(Blaze::VProGrowthUnlocks::UserIdList idList, const char8_t* statGroupName, Stats::StatPeriodType periodType, Stats::StatValues::EntityStatsList& outEntityStatsList)
{
	bool result = false;

	if (idList.size() > 0)
	{
		Stats::StatsSlaveImpl* statsComponent = static_cast<Stats::StatsSlaveImpl*>(gController->getComponent(Stats::StatsSlaveImpl::COMPONENT_ID, false));
		if (statsComponent == nullptr)
		{
			ERR_LOG("[VProGrowthUnlocksSlaveImpl].fetchObjectiveProgress(): Failed to get statsComponent");
		}
		else
		{
			// Get Stat values by group
			Stats::GetStatsByGroupRequest request;
			request.setGroupName(statGroupName);
			request.setPeriodId(periodType);
			for (uint64_t userId : idList)
			{
				request.getEntityIds().push_back(userId);
			}

			Stats::GetStatsResponse response;
			BlazeRpcError error = statsComponent->getStatsByGroup(request, response);
			if (error != Blaze::ERR_OK || response.getKeyScopeStatsValueMap().size() == 0)
			{
				ERR_LOG("[VProGrowthUnlocksSlaveImpl].retrieveEntityStats(): Failed to get statsbyGroup");
			}
			else
			{
				Stats::KeyScopeStatsValueMap::iterator iter = response.getKeyScopeStatsValueMap().begin();
				Stats::StatValues* statValues = iter->second;
				if (statValues != nullptr)
				{
					outEntityStatsList.clear();
					statValues->getEntityStatsList().copyInto(outEntityStatsList);
					result = true;
				}
			}
		}
	}
	return result;
}

bool VProGrowthUnlocksSlaveImpl::loadFile(const eastl::string &filePath, eastl::vector<char> &data)
{
	bool loaded = false;

	EA::IO::FileStream fs(filePath.c_str());
	if (fs.Open())
	{
		eastl_size_t fileSize = static_cast<eastl_size_t>(fs.GetSize());
		data.assign(fileSize, '\0');
		EA::IO::size_type read = fs.Read(data.data(), fileSize);
		fs.Close();
		loaded = read > 0 && read == fileSize;
	}

	return loaded;
}

uint32_t VProGrowthUnlocksSlaveImpl::checkObjectiveProgress(UserIdList idList, VProObjectiveProgressGroupList &outObjectiveProgressGroups, BlazeRpcError &outError, bool setProgress)
{
	uint32_t mSkillPointEarned = 0;

	if (!buildStatsDescIndexMap(mObjectiveStatsGroup.c_str(), mObjectiveStatsDescIndexMap))
	{
		ERR_LOG("[VProGrowthUnlocksSlaveImpl].fetchObjectiveProgress(): Failed to build Stats Desc Index Map");
		outError = Blaze::VPROGROWTHUNLOCKS_ERR_SYSTEM;
		return mSkillPointEarned;
	}
	
	Stats::StatValues::EntityStatsList entityStatsList;
	if (!retrieveEntityStats(idList, mObjectiveStatsGroup.c_str(), mObjectiveStatsPeriod, entityStatsList))
	{
		ERR_LOG("[VProGrowthUnlocksSlaveImpl].fetchObjectiveProgress(): Failed to retrieve enttiy stats");
		outError = Blaze::VPROGROWTHUNLOCKS_ERR_SYSTEM;
		return mSkillPointEarned;
	}

	for (Stats::StatValues::EntityStatsList::const_iterator entityStatItr = entityStatsList.begin(); entityStatItr != entityStatsList.end(); entityStatItr++)
	{
		if ((*entityStatItr) != NULL)
		{
			const Stats::EntityStats::StringStatValueList &statValueList = (*entityStatItr)->getStatValues();

			VProObjectiveProgressGroup* objectiveProgressgGroup = nullptr;
			if (setProgress)
			{
				objectiveProgressgGroup = BLAZE_NEW VProObjectiveProgressGroup();
				uint64_t userid = static_cast<uint64_t> ((*entityStatItr)->getEntityId());
				objectiveProgressgGroup->setUserId(userid);
			}				

			VProGrowthUnlocksConfig::ObjectiveCategoryServerConfigList objectiveCategoryServerConfigList = getConfig().getObjectiveCategoryServerConfigs();
			for (VProGrowthUnlocksConfig::ObjectiveCategoryServerConfigList::const_iterator ocscIter = objectiveCategoryServerConfigList.begin(); ocscIter != objectiveCategoryServerConfigList.end(); ocscIter++)
			{
				ObjectiveCategoryServerConfig::ObjectiveServerConfigList objectiveServerConfigList = (*ocscIter)->getObjectiveServerConfigs();
				for (ObjectiveCategoryServerConfig::ObjectiveServerConfigList::const_iterator oscIter = objectiveServerConfigList.begin(); oscIter != objectiveServerConfigList.end(); oscIter++)
				{
					const char8_t* statName = (*oscIter)->getStatName();
					if (statName == nullptr || strlen(statName) == 0)
					{
						continue;
					}
					Blaze::VProGrowthUnlocks::StatsDescIndexMap::iterator indexIter = mObjectiveStatsDescIndexMap.find(statName);
					if (indexIter == mObjectiveStatsDescIndexMap.end())
					{
						continue;
					}
					uint32_t index = indexIter->second;
					uint32_t currentStatValue = EA::StdC::AtoU32((statValueList)[index].c_str());
					uint32_t requiredStatValue = (*oscIter)->getStatReqVal();
					mSkillPointEarned += (currentStatValue >= requiredStatValue) ? (*oscIter)->getRewardVal() : 0;

					VProObjectiveProgress* objectiveProgress = nullptr;
					if (setProgress)
					{
						objectiveProgress = BLAZE_NEW VProObjectiveProgress();
						objectiveProgress->setObjectiveId((*oscIter)->getObjectiveId());
						objectiveProgress->setObjectiveCategoryId((*ocscIter)->getObjectiveCategoryId());
						objectiveProgress->setStatReqVal(requiredStatValue);
						objectiveProgress->setStatActVal(currentStatValue);
						objectiveProgress->setCompleted(currentStatValue >= requiredStatValue);
						
						uint32_t percentage = (currentStatValue >= requiredStatValue) ? 100 : ((100 * currentStatValue) / requiredStatValue);
						objectiveProgress->setPercentage(percentage);
						
						objectiveProgressgGroup->getObjectiveProgresses().push_back(objectiveProgress);
					}
				}
			}
			if (setProgress)
			{
				outObjectiveProgressGroups.push_back(objectiveProgressgGroup);
			}
		}

	}
	outError = Blaze::ERR_OK;
	return mSkillPointEarned;
}

uint32_t VProGrowthUnlocksSlaveImpl::checkMatchCount(BlazeId userId, BlazeRpcError &outError)
{
	uint32_t mSkillPointEarned = 0;

	if (!buildStatsDescIndexMap(mMatchCountStatsGroup.c_str(), mMatchCountStatsDescIndexMap))
	{
		ERR_LOG("[VProGrowthUnlocksSlaveImpl].fetchObjectiveProgress(): Failed to build Stats Desc Index Map");
		outError = Blaze::VPROGROWTHUNLOCKS_ERR_SYSTEM;
		return mSkillPointEarned;
	}

	UserIdList idList;
	idList.push_back(userId);
	Stats::StatValues::EntityStatsList entityStatsList;
	if (!retrieveEntityStats(idList, mMatchCountStatsGroup.c_str(), mMatchCountStatsPeriod, entityStatsList))
	{
		ERR_LOG("[VProGrowthUnlocksSlaveImpl].fetchObjectiveProgress(): Failed to retrieve enttiy stats");
		outError = Blaze::VPROGROWTHUNLOCKS_ERR_SYSTEM;
		return mSkillPointEarned;
	}

	for (Stats::StatValues::EntityStatsList::const_iterator entityStatItr = entityStatsList.begin(); entityStatItr != entityStatsList.end(); entityStatItr++)
	{
		if ((*entityStatItr) != NULL)
		{
			const Stats::EntityStats::StringStatValueList &statValueList = (*entityStatItr)->getStatValues();

			Blaze::VProGrowthUnlocks::StatsDescIndexMap::iterator indexIter = mMatchCountStatsDescIndexMap.find("gamesPlayed");
			if (indexIter != mMatchCountStatsDescIndexMap.end())
			{
				uint32_t index = indexIter->second;
				uint32_t clubGamesPlayed = EA::StdC::AtoU32((statValueList)[index].c_str());

				Blaze::VProGrowthUnlocks::SkillPointRewardsMap::const_iterator iter = mSkillPointRewardsMap.find(clubGamesPlayed);
				if (iter != mSkillPointRewardsMap.end())
				{
					mSkillPointEarned = iter->second + mDefaultGrantedSP;
				}
				else if (clubGamesPlayed > mLastMatchCount)
				{
					mSkillPointEarned = mMaxGrantedSP;
				}				
			}
		}
	}
	outError = Blaze::ERR_OK;
	return mSkillPointEarned;
}
void VProGrowthUnlocksSlaveImpl::onVoidMasterNotification(UserSession *)
{
    TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].onVoidMasterNotification");
}

void VProGrowthUnlocksSlaveImpl::onUserSessionExistence(const UserSession& userSession)
{
	TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].onUserSessionExistence");

	if ( userSession.isLocal() == true && userSession.getClientType() == CLIENT_TYPE_GAMEPLAY_USER && userSession.getSessionFlags().getFirstConsoleLogin() == true)
	{
		gSelector->scheduleFiberCall<VProGrowthUnlocksSlaveImpl, BlazeId>(
			this, 
			&VProGrowthUnlocksSlaveImpl::autoRegistration,
			userSession.getUserId(),
			"VProGrowthUnlocksSlaveImpl::onUserFirstConsoleLogin");
	}
}

void VProGrowthUnlocksSlaveImpl::autoRegistration(BlazeId userId)
{
	mDefaultLoadOuts.clear();
	VProGrowthUnlocksDb dbHelper = VProGrowthUnlocksDb(this);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
	BlazeRpcError outError = dbHelper.setDbConnection(dbConn);

	for (uint32_t i = 0; i < mMaxLoadOutCount; i++)
	{
		char defaultName[DEFAULT_LOADOUT_NAME_MAX_LENGTH];
		EA::StdC::Sprintf(defaultName, "*DefaultLoadOutName%d", i);
		if (outError == Blaze::ERR_OK)
		{
			outError = dbHelper.insertLoadOut(userId, i, defaultName, 0, mDefaultGrantedSP, 0, 0, 0, 0, 0, 0);
			if (outError == Blaze::ERR_OK)
			{
				Blaze::VProGrowthUnlocks::VProLoadOut* obj = mDefaultLoadOuts.allocate_element();
				obj->setUserId(userId);
				obj->setLoadOutId(i);
				obj->setLoadOutName(defaultName);
				obj->setXPEarned(0);
				obj->setSPGranted(mDefaultGrantedSP);
				obj->setSPConsumed(0);
				obj->setUnlocks1(0);
				obj->setUnlocks2(0);
				obj->setUnlocks3(0);
				obj->setVProHeight(0);
				obj->setVProWeight(0);
				obj->setVProPosition(0);
				obj->setVProFoot(0);
				mDefaultLoadOuts.push_back(obj);
			}
		}
	}
	TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].autoRegistration for userId: " << userId << " Result: " << outError);
}

void VProGrowthUnlocksSlaveImpl::fetchPlayerGrowthConfig(Blaze::VProGrowthUnlocks::PlayerGrowthConfig &outPlayerGrowthConfig, BlazeRpcError &outError)
{
	mAvatarGrowthCommon.getPlayerGrowthConfig().copyInto(outPlayerGrowthConfig);
	outError = Blaze::ERR_OK;
}

void VProGrowthUnlocksSlaveImpl::fetchSkillTreeConfig(Blaze::VProGrowthUnlocks::SkillTree &outSkillTree, BlazeRpcError &outError)
{
	mAvatarGrowthCommon.getSkillTree().copyInto(outSkillTree);
	outError = Blaze::ERR_OK;
}

void VProGrowthUnlocksSlaveImpl::fetchPerksConfig(Blaze::VProGrowthUnlocks::PerkSlotsMap &outPerkSlots, Blaze::VProGrowthUnlocks::PerksMap &outPerks, Blaze::VProGrowthUnlocks::PerkCategoriesMap &outPerkCategories, BlazeRpcError &outError)
{
	mAvatarGrowthCommon.getPerkSlots().copyInto(outPerkSlots);
	mAvatarGrowthCommon.getPerks().copyInto(outPerks);
	mAvatarGrowthCommon.getPerkCategories().copyInto(outPerkCategories);
	outError = Blaze::ERR_OK;
}

void VProGrowthUnlocksSlaveImpl::fetchLoadOuts(Blaze::VProGrowthUnlocks::UserIdList idList, Blaze::VProGrowthUnlocks::VProLoadOutList &outLoadOutList, BlazeRpcError &outError)
{
	VProGrowthUnlocksDb dbHelper = VProGrowthUnlocksDb(this);

	DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
	outError = dbHelper.setDbConnection(dbConn);
	uint32_t count = 0;
	if (outError == Blaze::ERR_OK)
	{
		if (idList.size() == 1)
		{
			uint64_t userId = (*idList.begin());
			outError = dbHelper.fetchLoadOuts(userId, outLoadOutList, count);
			if (count == 0)
			{
				TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].fetchLoadOuts for userId: " << userId << " count is 0 ");
				autoRegistration(userId);
				mDefaultLoadOuts.copyInto(outLoadOutList);
			}
		}
		else
		{
			outError = dbHelper.fetchLoadOuts(idList, outLoadOutList, count);
		}
	}
}

void VProGrowthUnlocksSlaveImpl::fetchObjectiveConfig(Blaze::VProGrowthUnlocks::VProObjectiveCategoryConfigList &outObjectiveCategoryConfigList, Blaze::VProGrowthUnlocks::SkillPointRewardsMap &outProgressionRewardsMap, BlazeRpcError &outError)
{
	mObjectiveCategoryConfigs.copyInto(outObjectiveCategoryConfigList);
	mSkillPointRewardsMap.copyInto(outProgressionRewardsMap);
	
	outError = Blaze::ERR_OK;	
}

void VProGrowthUnlocksSlaveImpl::fetchObjectiveProgress(Blaze::VProGrowthUnlocks::UserIdList idList, Blaze::VProGrowthUnlocks::VProObjectiveProgressGroupList &outObjectiveProgressGroups, BlazeRpcError &outError)
{	
	checkObjectiveProgress(idList, outObjectiveProgressGroups, outError);
}

void VProGrowthUnlocksSlaveImpl::resetLoadOuts(BlazeId userId, BlazeRpcError &outError)
{
	VProGrowthUnlocksDb dbHelper = VProGrowthUnlocksDb(this);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
	outError = dbHelper.setDbConnection(dbConn);

	if (outError == Blaze::ERR_OK)
	{
		outError = dbHelper.resetLoadOuts(userId);
	}
}

void VProGrowthUnlocksSlaveImpl::resetSingleLoadOut(BlazeId userId, int32_t loadOutId, BlazeRpcError &outError)
{
	VProGrowthUnlocksDb dbHelper = VProGrowthUnlocksDb(this);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
	outError = dbHelper.setDbConnection(dbConn);

	if (outError == Blaze::ERR_OK)
	{
		outError = dbHelper.resetSingleLoadOut(userId, loadOutId);
	}
}

void VProGrowthUnlocksSlaveImpl::updateLoadOutsPeripherals(BlazeId userId, Blaze::VProGrowthUnlocks::VProLoadOutList &loadOutList, BlazeRpcError &outError)
{
	VProGrowthUnlocksDb dbHelper = VProGrowthUnlocksDb(this);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
	outError = dbHelper.setDbConnection(dbConn);

	uint32_t count = 0;
	
	for(Blaze::VProGrowthUnlocks::VProLoadOutList::const_iterator iter = loadOutList.begin(); iter != loadOutList.end(); ++iter)
	{
		if (count < mMaxLoadOutCount && outError == Blaze::ERR_OK)
		{
			uint32_t loadOutId = (*iter)->getLoadOutId();
			const char* loadOutName =  (*iter)->getLoadOutName();
			uint32_t height = (*iter)->getVProHeight(); 
			uint32_t weight = (*iter)->getVProWeight(); 
			uint32_t position = (*iter)->getVProPosition(); 
			uint32_t foot =  (*iter)->getVProFoot(); 
			
			if (strlen(loadOutName) > 0)
			{
				outError = dbHelper.updateLoadOutPeripherals(userId, loadOutId, loadOutName, height, weight, position, foot);
			}
			else
			{
				outError = dbHelper.updateLoadOutPeripheralsWithoutName(userId, loadOutId, height, weight, position, foot);
			}
			count++;
		}
	}
}

void VProGrowthUnlocksSlaveImpl::updateLoadOutUnlocks(BlazeId userId, Blaze::VProGrowthUnlocks::VProLoadOut &loadOut, BlazeRpcError &outError)
{
	VProGrowthUnlocksDb dbHelper = VProGrowthUnlocksDb(this);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
	outError = dbHelper.setDbConnection(dbConn);

	if (outError == Blaze::ERR_OK)
	{
		outError = dbHelper.updateLoadOutUnlocks(userId, loadOut);
	}
}

void VProGrowthUnlocksSlaveImpl::grantSkillPoints(BlazeId userId, uint32_t grantSP, BlazeRpcError &outError)
{
	TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].grantSkillPoints for user id: " << userId << " ");
	outError = Blaze::ERR_OK;
	
	VProGrowthUnlocksDb dbHelper = VProGrowthUnlocksDb(this);
	DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
	outError = dbHelper.setDbConnection(dbConn);

	if (outError == Blaze::ERR_OK)
	{
		uint32_t count = 0;
		Blaze::VProGrowthUnlocks::VProLoadOutList loadOutList;
		outError = dbHelper.fetchLoadOuts(userId, loadOutList, count);

		if (loadOutList.size() > 0 && grantSP > loadOutList[0]->getSPGranted() && grantSP <= mMaxGrantedSP)
		{
			outError = dbHelper.updateMatchSPGranted(userId, grantSP);
		}
	}
}

void VProGrowthUnlocksSlaveImpl::updatePlayerGrowth(BlazeId userId, Blaze::VProGrowthUnlocks::MatchEventsMap &matchEventsMap, BlazeRpcError &outError)
{
    outError = Blaze::ERR_OK;

    uint32_t xpReward = mAvatarGrowthCommon.calculateXPRewardsFromMatch(matchEventsMap);
    grantXP(userId, xpReward, outError);

    TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].updatePlayerGrowth: userId: " << userId
        << ", matchEventsMap.size(): " << matchEventsMap.size() << ", xpReward: " << xpReward << ", outError: " << outError);
}

void VProGrowthUnlocksSlaveImpl::grantSkillPointsPostMatch(BlazeId userId, bool grantMatchCountSP, bool grantObjectiveSP, BlazeRpcError &outError)
{
	uint32_t matchCountSPGrant = 0;
	if (grantMatchCountSP)
	{
		matchCountSPGrant = checkMatchCount(userId, outError);
	}
		
	uint32_t objectiveSPGrant = 0;
	if (grantObjectiveSP)
	{
		UserIdList idList;
		idList.push_back(userId);
		VProObjectiveProgressGroupList objectiveProgressGroups;
		objectiveProgressGroups.clear();
		BlazeRpcError error = Blaze::ERR_OK;
		
		objectiveSPGrant = checkObjectiveProgress(idList, objectiveProgressGroups, error, false);
	}

	if (matchCountSPGrant > 0 && objectiveSPGrant > 0)
	{
		VProGrowthUnlocksDb dbHelper = VProGrowthUnlocksDb(this);
		DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
		outError = dbHelper.setDbConnection(dbConn);

		if (outError == Blaze::ERR_OK)
		{
			outError = dbHelper.updateAllSPGranted(userId, matchCountSPGrant, objectiveSPGrant);
		}
	}
	else if (matchCountSPGrant > 0)
	{
		VProGrowthUnlocksDb dbHelper = VProGrowthUnlocksDb(this);
		DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
		outError = dbHelper.setDbConnection(dbConn);

		if (outError == Blaze::ERR_OK)
		{
			outError = dbHelper.updateMatchSPGranted(userId, matchCountSPGrant);
		}
	}
	else if (objectiveSPGrant > 0)
	{
		VProGrowthUnlocksDb dbHelper = VProGrowthUnlocksDb(this);
		DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
		outError = dbHelper.setDbConnection(dbConn);

		if (outError == Blaze::ERR_OK)
		{
			outError = dbHelper.updateObjectiveSPGranted(userId, objectiveSPGrant);
		}
	}
}

void VProGrowthUnlocksSlaveImpl::calculateXPRewardsFromMatch(BlazeId userId, uint32_t matchRating, Blaze::VProGrowthUnlocks::MatchEventsMap &matchEventsMap,  Blaze::VProGrowthUnlocks::AttributeTypesMap &attributeTypesMap, Blaze::VProGrowthUnlocks::AttributeXPsMap &attributeXPsMap, BlazeRpcError &outError)
{
	attributeXPsMap.clear();
	outError = Blaze::ERR_OK;

	uint32_t ratingReward = 0;
	Blaze::VProGrowthUnlocks::MatchRatingXPRewardsMap::const_iterator ratingRewardIter = mMatchRatingXPRewardsMap.find(matchRating);
	if (ratingRewardIter != mMatchRatingXPRewardsMap.end())
	{
		ratingReward = ratingRewardIter->second;
	}

	TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].calculateXPRewardsFromMatch for user: "<< userId << ",  matchRating: " << matchRating <<", rating xp reward: " << ratingReward );
	
	uint32_t tempAttributeXP[TOTAL_PLAYER_ATTRIBUTE];
	for (uint32_t i = 0; i < TOTAL_PLAYER_ATTRIBUTE; i++)
	{
		tempAttributeXP[i] = 0;
		Blaze::VProGrowthUnlocks::AttributeTypesMap::const_iterator iter = attributeTypesMap.find(i);
		if (iter != matchEventsMap.end())
		{
			uint32_t typeIndex = iter->second;
			TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].calculateXPRewardsFromMatch attribute: "<< i << ",  typeIndex: " << typeIndex);
			if (typeIndex <TOTAL_PLAYER_ATTRIBUTE_TYPE)
			{
				tempAttributeXP[i] = ratingReward * mModifier[typeIndex] / 100;
			}
		}		
	}

	Blaze::VProGrowthUnlocks::VProGrowthUnlocksConfig::MatchEventRewardList eventRewardList = getConfig().getMatchEventXPRewards();

	for (Blaze::VProGrowthUnlocks::VProGrowthUnlocksConfig::MatchEventRewardList::const_iterator eventRewardListIter = eventRewardList.begin(); eventRewardListIter != eventRewardList.end(); eventRewardListIter++)
	{
		uint32_t eventId = (*eventRewardListIter)->getEventId();
		TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "]. event with rewards: "<< eventId );

		Blaze::VProGrowthUnlocks::MatchEventsMap::const_iterator iter = matchEventsMap.find(eventId);
		if (iter != matchEventsMap.end())
		{
			uint32_t count = iter->second;
			TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "]. event : "<< eventId <<", Occur: " << count << " times");
			Blaze::VProGrowthUnlocks::MatchEventReward::AttributeXPRewardList attributeXPList = (*eventRewardListIter)->getRewardList();

			for (Blaze::VProGrowthUnlocks::MatchEventReward::AttributeXPRewardList::const_iterator attributeXPIter = attributeXPList.begin(); attributeXPIter != attributeXPList.end(); attributeXPIter++)
			{
				uint32_t attribute = (*attributeXPIter)->getAttribute();
				uint32_t reward = count * (*attributeXPIter)->getXPReward();
				TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "]. attribute : "<< attribute << ",  reward: " << reward );
				tempAttributeXP[attribute] += reward;
				TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "]. xp: " << tempAttributeXP[attribute] );
			}
		}
	}

	for (uint32_t i = 0; i < TOTAL_PLAYER_ATTRIBUTE; i++)
	{
		attributeXPsMap.insert(eastl::make_pair(i, tempAttributeXP[i]));
	}
}

void VProGrowthUnlocksSlaveImpl::completeObjectives(BlazeId userId, Blaze::VProGrowthUnlocks::CategoryIdList categoryList, Blaze::VProGrowthUnlocks::ObjectiveIdList objectiveList, BlazeRpcError &outError)
{
	TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].completeObjectives for user: " << userId );
	if (categoryList.size() > 0 && objectiveList.size() > 0 && categoryList.size() == objectiveList.size())
	{
		Blaze::VProGrowthUnlocks::StatsUpdatesMap pendingUpdateMap;
		pendingUpdateMap.clear();

		for (int i = 0; i < categoryList.size(); i++)
		{
			uint32_t categoryId = categoryList[i];
			uint32_t objectiveId = objectiveList[i];

			VProGrowthUnlocksConfig::ObjectiveCategoryServerConfigList objectiveCategoryServerConfigList = getConfig().getObjectiveCategoryServerConfigs();
			for (VProGrowthUnlocksConfig::ObjectiveCategoryServerConfigList::const_iterator ocscIter = objectiveCategoryServerConfigList.begin(); ocscIter != objectiveCategoryServerConfigList.end(); ocscIter++)
			{
				if (categoryId == (*ocscIter)->getObjectiveCategoryId())
				{
					ObjectiveCategoryServerConfig::ObjectiveServerConfigList objectiveServerConfigList = (*ocscIter)->getObjectiveServerConfigs();
					for (ObjectiveCategoryServerConfig::ObjectiveServerConfigList::const_iterator oscIter = objectiveServerConfigList.begin(); oscIter != objectiveServerConfigList.end(); oscIter++)
					{
						if (objectiveId == (*oscIter)->getObjectiveId())
						{
							const char8_t* statsName = (*oscIter)->getStatName();
							uint32_t newValue = (*oscIter)->getStatReqVal();

							Blaze::VProGrowthUnlocks::StatsUpdatesMap::iterator iter = pendingUpdateMap.find(statsName);
							if (iter != pendingUpdateMap.end() && newValue > iter->second)
							{
								iter->second = newValue;
							}
							else
							{
								pendingUpdateMap.insert(eastl::make_pair(statsName, newValue));
							}
							break;
						}
					}
					break;
				}
			}
		}

		if (pendingUpdateMap.size() > 0)
		{
			Blaze::Stats::UpdateStatsHelper updateStatsHelper;
			Blaze::Stats::UpdateStatsRequestBuilder builder;

			builder.startStatRow(VPRO_OBJECTIVE_STATS_CATEGORY, userId, NULL);

			Blaze::VProGrowthUnlocks::StatsUpdatesMap::iterator iter = pendingUpdateMap.begin();
			while (iter != pendingUpdateMap.end())
			{								
				builder.assignStat(iter->first.c_str(), iter->second);				
				iter++;
			}
			builder.completeStatRow();

			if (!builder.empty())
			{
				outError = updateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)builder, true);
				if (outError == ERR_OK)
				{
					outError = updateStatsHelper.commitStats();
					if (outError == ERR_OK)
					{
						grantSkillPointsPostMatch(userId, false, true, outError);
					}
				}
			}
		}
	}
	else
	{
		TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].completeObjectives ids size mismatch ");
	}

}

void VProGrowthUnlocksSlaveImpl::grantXP(BlazeId userId, uint32_t xp, BlazeRpcError &outError)
{
    outError = Blaze::ERR_OK;
    if (xp > 0)
    {
        VProGrowthUnlocksDb dbHelper = VProGrowthUnlocksDb(this);
        DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
        outError = dbHelper.setDbConnection(dbConn);
 
        if (outError == Blaze::ERR_OK)
        {
            uint32_t xpEarned = 0;
            outError = dbHelper.fetchXPEarned(userId, 0, xpEarned);

            if (outError == Blaze::ERR_OK)
            {
                uint32_t maxXP = mAvatarGrowthCommon.getMaxXP();

                xpEarned += xp;
                if (xpEarned > maxXP)
                {
                    xpEarned = maxXP;
                }

                AvatarGrowthCommon::LevelInfo levelInfo;
                mAvatarGrowthCommon.calculatePlayerLevel(xpEarned, levelInfo);
                uint32_t skillPoints = mAvatarGrowthCommon.calculateSkillPointsRewards(levelInfo.mLevel);

                outError = dbHelper.updatePlayerGrowth(userId, xpEarned, skillPoints);

                TRACE_LOG("[VProGrowthUnlocksSlaveImpl:" << this << "].grantXP: userId: " << userId << ", xpAdded: " << xp << ", xpTotal: " << xpEarned
                    << ", playerLevel: " << levelInfo.mLevel << ", skillPoints: " << skillPoints << ", outError: " << outError);
            }
        }  
    }
}

} // VProGrowthUnlocks
} // Blaze

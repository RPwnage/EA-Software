//  *************************************************************************************************
//
//   File:    reference.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "reference.h"
#include "singleplayer.h"
#include "onlineseasons.h"
#include "friendlies.h"
#include "dedicatedservers.h"
#include "coopseasons.h"
#include "ssf.h"
#include "aruba.h"
#include "ssfseasons.h"

#include "framework/config/config_sequence.h"

namespace Blaze {
namespace Stress {

Player* PlayerFactory::create(PlayerType type, StressPlayerInfo* playerData)
{
    switch (type)
    {
        case SINGLEPLAYER:
            return BLAZE_NEW SinglePlayer(playerData);
		case KICKOFFPLAYER:
			return BLAZE_NEW KickOffPlayer(playerData);
		case ONLINESEASONS:
            return BLAZE_NEW OnlineSeasons(playerData);
		case FRIENDLIES:
			return BLAZE_NEW Friendlies(playerData);
		case LIVECOMPETITIONS:
            return BLAZE_NEW LiveCompetitions(playerData);
		case FUTPLAYER:
            return BLAZE_NEW FutPlayer(playerData);
		case DEDICATEDSERVER:
            return BLAZE_NEW DedicatedServer(playerData);
		case COOPPLAYER:
            return BLAZE_NEW CoopSeasons(playerData);
		case CLUBSPLAYER:
            return BLAZE_NEW ClubsPlayer(playerData);
		case DROPINPLAYER:
			return BLAZE_NEW DropinPlayer(playerData);
		case SSFPLAYER:
			return BLAZE_NEW SSFPlayer(playerData);
		case SSFSEASONS:
			return BLAZE_NEW SsfSeasons(playerData);
		case ESLTOURNAMENTS:
			return BLAZE_NEW ESLTournaments(playerData);
        case INVALID:
        default:
            return NULL;
    }
}

//  List all the scenarios here.
//  Array values should match with enum scenariosType
const char8_t* Scenarios[] =
{
    "onlineseasons",
	"dedicatedserver",
    "singleplayer",
	"kickoffplayer",
    "coopplayer",
	"livecompetitions",
	"friendlies",
	"futplayer",
	"clubsplayer",
	"dropinplayer",
	"ssfplayer",
	"ssfseasons",
	"esltournaments",
	"eslserver"
};

PlayerType PlayerFactory::getPlayerType(const char8_t* name)
{
    if (blaze_stricmp(name, Scenarios[COOPPLAYER]) == 0) {
        return COOPPLAYER;
    }
    if (blaze_stricmp(name, Scenarios[ONLINESEASONS]) == 0) {
        return ONLINESEASONS;
    }
	if (blaze_stricmp(name, Scenarios[FRIENDLIES]) == 0) {
        return FRIENDLIES;
	}
	if (blaze_stricmp(name, Scenarios[LIVECOMPETITIONS]) == 0) {
        return LIVECOMPETITIONS;
    }
    if (blaze_stricmp(name, Scenarios[DEDICATEDSERVER]) == 0) {
        return DEDICATEDSERVER;
    }
    if (blaze_stricmp(name, Scenarios[SINGLEPLAYER]) == 0) {
        return SINGLEPLAYER;
    }
	if (blaze_stricmp(name, Scenarios[KICKOFFPLAYER]) == 0) {
		return KICKOFFPLAYER;
	}
    if (blaze_stricmp(name, Scenarios[FUTPLAYER]) == 0) {
        return FUTPLAYER;
    }
	if (blaze_stricmp(name, Scenarios[CLUBSPLAYER]) == 0) {
        return CLUBSPLAYER;
    }
	if (blaze_stricmp(name, Scenarios[DROPINPLAYER]) == 0) {
		return DROPINPLAYER;
	}
	if (blaze_stricmp(name, Scenarios[SSFPLAYER]) == 0) {
		return SSFPLAYER;
	}
	if (blaze_stricmp(name, Scenarios[SSFSEASONS]) == 0) {
		return SSFSEASONS;
	}
	if (blaze_stricmp(name, Scenarios[ESLTOURNAMENTS]) == 0) {
		return ESLTOURNAMENTS;
	}
	if (blaze_stricmp(name, Scenarios[ESLSERVER]) == 0) {
		return ESLSERVER;
	}
    return INVALID;
}

Player* PlayerFactory::createRandomPlayer(StressPlayerInfo* playerData, PlayerType* playerType)
{
	//Add random ping site
    playerData->setPlayerPingSite("bio-iad");  //   default ping site
    PlayerType type = StressConfigInst.getRandomPlayerType(playerData);
    Player* player = PlayerFactory::create(type, playerData);
    if (NULL == player)
    {
        //   this should not occur ...
        recyclePlayerType(type, playerData->getPlayerPingSite());
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[StressConfig::createRandomPlayer] unable to create player");
    }
    *playerType = type;
    return player;
}

void PlayerFactory::recyclePlayerType(PlayerType type, eastl::string pingSite)
{
    StressConfigInst.recyclePlayerType(type, pingSite);
}

//  ***************************************************************************
//  //  /class StressConfigManager implementation

bool StressConfigManager::parseGameplayUserInfo(const Blaze::ConfigMap& guConfig)
{
    //  TODO move following config items to HAVANA module and make changes in Loginmanager as well
    mEmailFormat = guConfig.getString("email-format", "ws-ps4-%07x@ea.com");
    mPersonaFormat = guConfig.getString("persona-format", "wsps4%07x");
    mPassword = guConfig.getString("password", "loadtest1234");
    mNamespace = guConfig.getString("namespace", "ps3");
    return true;
}

StressConfigManager::StressConfigManager()
{
    mStringToEnumMap["coopplayer"] = (int) COOPPLAYER;
    mStringToEnumMap["onlineseasons"] = (int) ONLINESEASONS;
	mStringToEnumMap["friendlies"] = (int) FRIENDLIES;
	mStringToEnumMap["livecompetitions"] = (int) LIVECOMPETITIONS;
    mStringToEnumMap["dedicatedserver"] = (int) DEDICATEDSERVER;
    mStringToEnumMap["singleplayer"] = (int)SINGLEPLAYER;
	mStringToEnumMap["kickoffplayer"] = (int)KICKOFFPLAYER;
	mStringToEnumMap["dropinplayer"] = (int)DROPINPLAYER;
	mStringToEnumMap["ssfseasons"] = (int)SSFSEASONS;
	mStringToEnumMap["clubsplayer"] = (int)CLUBS;
	mStringToEnumMap["ssfplayer"] = (int)SSFPLAYER;
	mStringToEnumMap["esltournaments"] = (int)ESLTOURNAMENTS;
	mStringToEnumMap["eslserver"] = (int)ESLSERVER;
    mCommonConfigPtr = NULL;
}

StressConfigManager::~StressConfigManager()
{
    if (mCommonConfigPtr != NULL)
    {
        BLAZE_FREE(mCommonConfigPtr);
    }
}

StressConfigManager& StressConfigManager::getInstance()
{
    static StressConfigManager Instance;
    return Instance;
}

bool StressConfigManager::initialize(const ConfigMap& config, const uint32_t totalConnections)
{
    bool result = true;
    result = parsePlatform(config);
    result = result ? parseGameplayUserInfo(config) : result;
    result = result ? parseAuthenticationConfig(config) : result;
    result = result ? parseNucleusLoginConfig(config) : result;
    result = result ? parsePlayerTypeDistribution(config, totalConnections) : result;   
    result = result ? parseFriendlistSizeDistribution(config, totalConnections) : result;
    result = result ? parseScenarioDistributionByPlayerType(config, COOPPLAYER) : result;
	result = result ? parseScenarioDistributionByPlayerType(config, ONLINESEASONS) : result;
	result = result ? parseScenarioDistributionByPlayerType(config, FUTPLAYER) : result;
	result = result ? parseScenarioDistributionByPlayerType(config, FRIENDLIES) : result;
	result = result ? parseScenarioDistributionByPlayerType(config, CLUBSPLAYER) : result;
	result = result ? parseScenarioDistributionByPlayerType(config, SSFPLAYER) : result;
	result = result ? parseScenarioDistributionByPlayerType(config, SSFSEASONS) : result;
	result = result ? parseKickOffScenarioDistribution(config) : result;
	result = result ? parseOfflineScenarioDistribution(config) : result;
	// Getting Aruba Trigger Id's distribution 
	result = result ? parseArubaTriggerIdsDistributionMap(config) : result;

	// Getting Reco Trigger Id's distribution
	result = result ? parseRecoTriggerIdsDistributionMap(config) : result;

    //  Currently, NO Scenario Distribution for SINGLEPLAYER
    result = result ? parseCommonGamePlayConfig(config) : result;
	result = result ? parseShieldConfiguration(config) : result;
    result = result ? parseGamePlayConfigByPlayerType(config, COOPPLAYER) : result;
    result = result ? parseGamePlayConfigByPlayerType(config, ONLINESEASONS) : result;
	result = result ? parseGamePlayConfigByPlayerType(config, FRIENDLIES) : result;
	result = result ? parseGamePlayConfigByPlayerType(config, LIVECOMPETITIONS) : result;
    result = result ? parseGamePlayConfigByPlayerType(config, SINGLEPLAYER) : result;
	result = result ? parseGamePlayConfigByPlayerType(config, KICKOFFPLAYER) : result;
	result = result ? parseGamePlayConfigByPlayerType(config, FUTPLAYER) : result;
	result = result ? parseGamePlayConfigByPlayerType(config, DROPINPLAYER) : result;
	result = result ? parseGamePlayConfigByPlayerType(config, CLUBSPLAYER) : result;
	result = result ? parseGamePlayConfigByPlayerType(config, DEDICATEDSERVER) : result;
	result = result ? parseGamePlayConfigByPlayerType(config, SSFPLAYER) : result;
	result = result ? parseGamePlayConfigByPlayerType(config, SSFSEASONS) : result;
	result = result ? parseArubaConfig(config) : result;
	result = result ? parseUPSConfig(config) : result;
	result = result ? parseRecoConfig(config) : result;
	result = result ? parsePINConfig(config) : result;
	result = result ? parseStatsConfig(config) : result;
	result = result ? parseKickOffConfig(config) : result;
	result = result ? parseGroupsConfig(config) : result;
    result = result ? parseProfanityConfig(config) : result;
    result = result ? parsePingSitesDistribution(config) : result;
	result = result ? parseEventIdDistribution(config) : result;
	result = result ? parsePersonaNames(config) : result;
    if (result)
    {
        mMatchMakingSessionTimeoutMs = config.getUInt32("matchmakingTimeout", 60000);
        mReportTelemetryPeriodMs = config.getUInt32("ReportTelemetryPeriodMs", 300000);
        mGameStateInitializingDurationMs = config.getUInt32("initStateDurationMs ", 1000);
        mGameStatePreGameDurationMs = config.getUInt32("preGameStateDurationMs", 10000);
        mGameStateInGameDurationMs = config.getUInt32("inGameStateDurationMs", 720000);
        mGameStateInGameDeviationMs = config.getUInt32("inGameDeviationMs", 120000);
        mRandomDelayMinMs = config.getUInt32("RandomDelayMinMs", 1000);
        mRandomDelayMaxMs = config.getUInt32("RandomDelayMinMs", 10000);
        mIsIncrementalLogin = config.getBool("EnableIncrementalLogin", false);
        mIncrementalLoginIdDelta = config.getInt64("IncrementalLoginIdDelta", 10000);
        mMaxIncrementalLoginsPerConnection = config.getInt64("MaxIncrementalLoginsPerConnection", 90);
        mBaseAccountId = config.getInt64("BaseAccountId", 200000000001);
		const char8_t* cfgstr = config.getString("gameProtocolVersionString", "Fifa-stress");
        blaze_strnzcpy(mGameProtocolVersionString, cfgstr, sizeof(mGameProtocolVersionString));
		geoipfilepath = config.getString("geoipfilepath","US.csv");
		geolocation_mm = config.getBool("geolocation_mm", false);
		mStatsEnabled  = config.getBool("statsEnabled", false);  
		mIsRPCTuningEnabled = config.getBool("EnableRPCTuning", false);
		mIsSineWaveEnabled = config.getBool("EnableSineWave", false);
	}
    return result;
}

bool StressConfigManager::parsePlatform(const ConfigMap& config)
{
    //  Possible values: pc, xbox, ps3
    const char8_t* platformStr = config.getString("platform", "xbox");
    if (blaze_stricmp(platformStr, PLATFORM_STRINGS[PLATFORM_PC]) == 0)
    {
        mPlatform = PLATFORM_PC;
    }
    else if (blaze_stricmp(platformStr, PLATFORM_STRINGS[PLATFORM_XBOX]) == 0)
    {
        mPlatform = PLATFORM_XBOX;
    }
    else if (blaze_stricmp(platformStr, PLATFORM_STRINGS[PLATFORM_PS3]) == 0)
    {
        mPlatform = PLATFORM_PS3;
    }
    else if (blaze_stricmp(platformStr, PLATFORM_STRINGS[PLATFORM_PS4]) == 0)
    {
        mPlatform = PLATFORM_PS4;
    }
    else if (blaze_stricmp(platformStr, PLATFORM_STRINGS[PLATFORM_XONE]) == 0)
    {
        mPlatform = PLATFORM_XONE;
    }
	else if (blaze_stricmp(platformStr, PLATFORM_STRINGS[PLATFORM_PS5]) == 0)
	{
		mPlatform = PLATFORM_PS5;
	}
	else if (blaze_stricmp(platformStr, PLATFORM_STRINGS[PLATFORM_XBSX]) == 0)
	{
		mPlatform = PLATFORM_XBSX;
	}
    else
    {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Unrecognized platform: '%s'", platformStr);
        return false;
    }
    return true;
}

bool StressConfigManager::parseUserInfoConfig(const ConfigMap& config)
{
    return true;
}


bool StressConfigManager::parseAuthenticationConfig(const ConfigMap& config)
{
    const ConfigMap* authConfigMap = config.getMap("authentication");
    if (!authConfigMap)
    {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing Authentication");
        return false;
    }
    const ConfigSequence *grpNameLst = authConfigMap->getSequence("entGroupNameList");
    if ((NULL != grpNameLst) && (grpNameLst->getSize() > 0))
    {
        for (size_t i = 0; i < grpNameLst->getSize(); ++i)
        {
            mEntGroupNameList.push_back(grpNameLst->at(i));
        }
    }
    //  TODO Need to provide default values to the below config
    mEntGroupName = authConfigMap->getString("entGroupName", "");
    mProjectId = authConfigMap->getString("projectId", "312054");
    mEntitlementTag = authConfigMap->getString("entitlementTag", "");
    mNumEntitlementsToShow = authConfigMap->getUInt16("numEntitlementsToShow", 50);
    return true;
}

bool StressConfigManager::parseNucleusLoginConfig(const ConfigMap& config)
{
    const ConfigMap* nucConfigMap = config.getMap("nucleuslogin");
    if (!nucConfigMap)
    {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing Nucleus Login config");
        return false;
    }
    //  TODO Need to provide default values to the below config
    mNucleusEmailFormat = nucConfigMap->getString("emailFormat", "");
    mNucleusPassword = nucConfigMap->getString("password", "");
    mNucleusGamertagFormat = nucConfigMap->getString("gamerTagFromat", "");
    mNucleusStartXuid = nucConfigMap->getUInt32("startXuid", 0);
    mStartAccountId = nucConfigMap->getUInt64("start-external-id", 0);
    return true;
}

bool StressConfigManager::parsePlayerTypeDistribution(const ConfigMap& config, uint32_t totalConnections)
{
    const ConfigSequence* distributionPlayerType = config.getSequence("playerTypeDistribution");
    if (!distributionPlayerType)
    {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing playerTypeDistribution");
        return false;
    }
    else
    {
        uint8_t sumProb = 0;
        for (size_t i = 0; i < distributionPlayerType->getSize(); ++i)
        {
            const ConfigMap* entry = distributionPlayerType->nextMap();
            PlayerType playerType = (PlayerType)entry->getUInt8("type", 0);
            uint8_t probability = entry->getUInt8("probability", 0);
            sumProb += probability;
            mPlayerTypeDistribution[playerType] = probability;
        }
        if (sumProb != 100)
        {
            BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing playerTypeDistribution distribution/probability's total is not 100percent");
            return false;
        }
        //   convert probability distribution into actual connection numbers here
        uint32_t conCount = 0;
        for (PlayerTypeDistributionMap::iterator it = mPlayerTypeDistribution.begin(); it != mPlayerTypeDistribution.end(); ++it)
        {
            if (it->second == 0)
            {
                continue;
            }
            uint32_t count = (it->second * totalConnections) / 100;
            //   ensure count is at least one for probability > 0
            if (count == 0)
            {
                count = 1;
            }
            conCount += count;
            mPlayerCountDistribution[it->first] = count;
        }
        //   adjust error due to rounding
        if (conCount < totalConnections)
        {
            //   add the number into the first non zero entry
            for (PlayerTypeDistributionMap::iterator it = mPlayerCountDistribution.begin(); it != mPlayerCountDistribution.end(); ++it)
            {
                if (it->second > 0)
                {
                    it->second += (totalConnections - conCount);
                    break;
                }
            }
        }
        //   calculate games distribution
        /*for (PlayerTypeDistributionMap::iterator it = mPlayerCountDistribution.begin(); it != mPlayerCountDistribution.end(); ++it)
        {
            BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressConfig] ParsePlayerTypeDistribution " <<  "  Type = " << it->first << " count = " << it->second);
            if (it->first == ONLINESEASONS || it->first == DEDICATEDSERVER)
            {
                calculatePlayerGamesDistributionCount(it->first, it->second);
            }
        }*/
    }
    return true;
}

bool StressConfigManager::parseFriendlistSizeDistribution(const ConfigMap& config, const uint32_t totalConnections)
{
    const ConfigSequence* distributionConfig = config.getSequence("friendListSizeDistribution");
    if (!distributionConfig)
    {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing friendListSizeDistribution");
        return false;
    }
    else
    {
        uint32_t sumProb = 0;
        uint32_t sumCon = 0;
        for (size_t i = 0; i < distributionConfig->getSize(); ++i)
        {
            const ConfigMap* entry = distributionConfig->nextMap();
            FriendlistSizeDistribution distribution;
            distribution.size = entry->getUInt8("size", 0);
            distribution.probability = entry->getUInt8("probability", 0);
            distribution.count = (distribution.probability * totalConnections) / 100;
            //   at least one connection for probability != 0
            if (distribution.probability != 0 && distribution.count == 0)
            {
                distribution.count = 1;
            }
            sumProb += distribution.probability;
            sumCon += distribution.count;
            mFriendlistDistribution[distribution.size] = distribution;
        }
        if (sumProb != 100)
        {
            BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing friendListSizeDistribution distribution/probability's total is not 100percent");
            return false;
        }
        //   connection count adjustment due to round errors
        if (sumCon < totalConnections)
        {
            FriendlistSizeDistributionMap::iterator itBegin = mFriendlistDistribution.begin();
            (*itBegin).second.count += (totalConnections - sumCon);
        }
    }
    return true;
}

bool StressConfigManager::parseRecoTriggerIdsDistributionMap(const ConfigMap & config)
{
	const ConfigSequence* distributionConfig = config.getSequence("RecoTriggerIdsDistribution");
	if (!distributionConfig)
	{
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing RecoTriggerIdsDistribution");
		return false;
	}
	else
	{
		uint32_t sumProb = 0;
		for (size_t i = 0; i < distributionConfig->getSize(); ++i)
		{
			const ConfigMap* entry = distributionConfig->nextMap();
			RecoTriggerIDs triggerIdType = (RecoTriggerIDs)entry->getInt64("recotype", 0);
			uint8_t probablity = entry->getUInt8("probability", 0);
			sumProb += probablity;
			getRecoTriggerIdsDistributionMap()[triggerIdType] = probablity;
		}
		if (sumProb != 100)
		{
			BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing RecoTriggerIdsScenarioDistribution distribution/probability's total is not 100percent");
			return false;
		}
		return true;
	}
}


bool StressConfigManager::parseArubaTriggerIdsDistributionMap(const ConfigMap & config)
{
	const ConfigSequence* distributionConfig = config.getSequence("ArubaTriggerIdsDistribution");
	if (!distributionConfig)
	{
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing ArubaTriggerIdsDistribution");
		return false;
	}
	else
	{
		uint32_t sumProb = 0;
		for (size_t i = 0; i < distributionConfig->getSize(); ++i)
		{
			const ConfigMap* entry = distributionConfig->nextMap();
			ArubaTriggerIDs triggerIdType = (ArubaTriggerIDs)entry->getUInt8("type", 0);
			uint8_t probablity = entry->getUInt8("probability", 0);
			sumProb += probablity;
			getArubaTriggerIdsDistributionMap()[triggerIdType] = probablity;
		}
		if (sumProb != 100)
		{
			BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing ArubaTriggerIdsScenarioDistribution distribution/probability's total is not 100percent");
			return false;
		}
		return true;
	}
}

bool StressConfigManager::parseKickOffScenarioDistribution(const ConfigMap& config)
{
	const ConfigSequence* distributionConfig = config.getSequence("KickOffPlayerDistribution");
	if (!distributionConfig)
	{
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing offlineScenarioDistribution");
		return false;
	}
	else
	{
		uint32_t sumProb = 0;
		for (size_t i = 0; i < distributionConfig->getSize(); ++i)
		{
			const ConfigMap* entry = distributionConfig->nextMap();
			eastl::string gameType = entry->getString("type", 0);
			uint8_t probablity = entry->getUInt8("probability", 0);
			sumProb += probablity;
			getKickOffScenarioDistribution()[gameType] = probablity;
		}
		if (sumProb != 100)
		{
			BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing KickOff distribution/probability's total is not 100percent");
			return false;
		}
		return true;
	}
}

bool  StressConfigManager::parseOfflineScenarioDistribution(const ConfigMap& config)
{
	const ConfigSequence* distributionConfig = config.getSequence("offlineScenarioDistribution");
    if (!distributionConfig)
    {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing offlineScenarioDistribution");
        return false;
    }
    else
    {
        uint32_t sumProb = 0;
		for (size_t i = 0; i < distributionConfig->getSize(); ++i)
        {
            const ConfigMap* entry = distributionConfig->nextMap();    
            eastl::string gameType = entry->getString("type", 0);
            uint8_t probablity = entry->getUInt8("probability", 0);
            sumProb += probablity;
            getOfflineScenarioDistribution()[gameType] = probablity;
        }
        if (sumProb != 100)
        {
            BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing offlineScenarioDistribution distribution/probability's total is not 100percent");
            return false;
        }
		return true;
	}
}

CoopGamePlayconfig* StressConfigManager::getCoopGamePlayConfig() const
{
    GameplayConfig* config = StressConfigInst.getGamePlayConfig(COOPPLAYER);
    if (config) {
        return static_cast<CoopGamePlayconfig*>(config);
    }
    return NULL;
}

OnlineSeasonsGamePlayConfig* StressConfigManager::getOnlineSeasonsGamePlayConfig() const
{
    GameplayConfig* config = StressConfigInst.getGamePlayConfig(ONLINESEASONS);
    if (config) {
        return static_cast<OnlineSeasonsGamePlayConfig*>(config);
    }
    return NULL;
}

FriendliesGamePlayConfig* StressConfigManager::getFriendliesGamePlayConfig() const
{
    GameplayConfig* config = StressConfigInst.getGamePlayConfig(FRIENDLIES);
    if (config) {
        return static_cast<FriendliesGamePlayConfig*>(config);
    }
    return NULL;
}

ClubsPlayerGamePlayconfig* StressConfigManager::getClubsPlayerGamePlayConfig() const
{
    GameplayConfig* config = StressConfigInst.getGamePlayConfig(CLUBSPLAYER);
    if (config) {
        return static_cast<ClubsPlayerGamePlayconfig*>(config);
    }
    return NULL;
}
DropinPlayerGamePlayconfig* StressConfigManager::getDropinPlayerGamePlayConfig() const
{
	GameplayConfig* config = StressConfigInst.getGamePlayConfig(DROPINPLAYER);
	if (config) {
		return static_cast<DropinPlayerGamePlayconfig*>(config);
	}
	return NULL;
}

SSFPlayerGamePlayConfig* StressConfigManager::getSSFPlayerGamePlayConfig() const
{
	GameplayConfig* config = StressConfigInst.getGamePlayConfig(SSFPLAYER);
	if (config) {
		return static_cast<SSFPlayerGamePlayConfig*>(config);
	}
	return NULL;
}

SsfSeasonsGamePlayConfig* StressConfigManager::getSsfSeasonsGamePlayConfig() const
{
	GameplayConfig* config = StressConfigInst.getGamePlayConfig(SSFSEASONS);
	if (config) {
		return static_cast<SsfSeasonsGamePlayConfig*>(config);
	}
	return NULL;
}

LiveCompetitionsGamePlayConfig* StressConfigManager::getLiveCompetitionsGamePlayConfig() const
{
    GameplayConfig* config = StressConfigInst.getGamePlayConfig(LIVECOMPETITIONS);
    if (config) {
        return static_cast<LiveCompetitionsGamePlayConfig*>(config);
    }
    return NULL;
}

DedicatedServerGamePlayconfig* StressConfigManager::getDedicatedServerGamePlayconfig() const
{
    GameplayConfig* config = StressConfigInst.getGamePlayConfig(DEDICATEDSERVER);
    if (config) {
        return static_cast<DedicatedServerGamePlayconfig*>(config);
    }
    return NULL;
}

bool StressConfigManager::parsePingSitesDistribution(const ConfigMap& config)
{
    const ConfigSequence* distributionConfig = config.getSequence("pingSites");
    if (!distributionConfig)
    {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing pingSites");
        return false;
    }
    else
    {
        uint32_t sumProb = 0;
		uint32_t probability;
		eastl::string pingSite;
        for (size_t i = 0; i < distributionConfig->getSize(); ++i)
        {
            const ConfigMap* entry = distributionConfig->nextMap();
			probability = entry->getUInt32("probability", 0);
			pingSite = entry->getString("name", "bio-dub");
			mPingSiteMap[pingSite] = probability;
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressConfig]parsePingSitesDistribution: pingSite= " << pingSite << " probability= " << probability);
			sumProb += probability;
        }
        if (sumProb != 100)
        {
            BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing pingSites. The probability's total is not 100 percent");
            return false;
        }
    }
    return true;
}

EA::TDF::TdfString StressConfigManager::getRandomPersonaNames()
{
	uint32_t sizeOfItems = mPersonaNames.size();
	if (sizeOfItems > 0)
	{
		int randNum = Random::getRandomNumber(sizeOfItems);
		return mPersonaNames[randNum];
	}
	return "0";
}

bool StressConfigManager::parsePersonaNames(const ConfigMap& config)
{
	const ConfigSequence* personaNames = NULL;
	Platform platType = StressConfigInst.getPlatform();
	if (platType == PLATFORM_PC)
	{
		return true;
	}
	if (platType == PLATFORM_XONE)
	{
		personaNames = config.getSequence("PersonaNamesforPlatformXONE");
	}
	else if (platType == PLATFORM_PS4)
	{
		personaNames = config.getSequence("PersonaNamesforPlatformPS4");
	}
	else {
		personaNames = config.getSequence("PersonaNamesforPlatformPS4");
	}

	if (!personaNames)
	{
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing personaNames");
		return false;
	}
	while (personaNames->hasNext())
	{
		 mPersonaNames.push_back(personaNames->nextString(0));
	}
	return true;
}
bool StressConfigManager::parseEventIdDistribution(const ConfigMap& config)
{
    const ConfigSequence* distributionConfig = config.getSequence("CompetitionsEventIdDistribution");
    if (!distributionConfig)
    {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing CompetitionsEventIdDistribution");
        return false;
    }
    else
    {
        uint32_t sumProb = 0;
		uint32_t probability;
		uint32_t eventId;
        for (size_t i = 0; i < distributionConfig->getSize(); ++i)
        {
            const ConfigMap* entry = distributionConfig->nextMap();
			probability = entry->getUInt32("probability", 0);
			eventId = entry->getUInt32("eventid", 0);
			eventIdMap[eventId] = probability;
			sumProb += probability;
        }
        if (sumProb != 100)
        {
            BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing CompetitionsEventIdDistribution. The probability's total is not 100 percent");
            return false;
        }
    }
    return true;
}

FutPlayerGamePlayConfig* StressConfigManager::getFutPlayerGamePlayConfig() const
{
    GameplayConfig* config = StressConfigInst.getGamePlayConfig(FUTPLAYER);
    if (config) {
        return static_cast<FutPlayerGamePlayConfig*>(config);
    }
    return NULL;
}

SingleplayerGamePlayConfig* StressConfigManager::getSingleplayerGamePlayConfig() const
{
    GameplayConfig* config = StressConfigInst.getGamePlayConfig(SINGLEPLAYER);
    if (config) {
        return static_cast<SingleplayerGamePlayConfig*>(config);
    }
    return NULL;
}


KickoffplayerGamePlayConfig* StressConfigManager::getKickoffplayerGamePlayConfig() const
{
	GameplayConfig* config = StressConfigInst.getGamePlayConfig(KICKOFFPLAYER);
	if (config) {
		return static_cast<KickoffplayerGamePlayConfig*>(config);
	}
	return NULL;
}

uint32_t StressConfigManager::getLogoutProbability(PlayerType type) const
{
    uint32_t logoutProbable = 50;
    switch (type)
    {
        case DEDICATEDSERVER:
			{
                DedicatedServerGamePlayconfig* ptrConfig = StressConfigInst.getDedicatedServerGamePlayconfig();
                logoutProbable = ptrConfig ? (ptrConfig->LogoutProbability) : logoutProbable;
            }
        case ONLINESEASONS :
            {
                OnlineSeasonsGamePlayConfig* ptrConfig = StressConfigInst.getOnlineSeasonsGamePlayConfig();
                logoutProbable = ptrConfig ? (ptrConfig->LogoutProbability) : logoutProbable;
            }
            break;
        case COOPPLAYER :
            {
                CoopGamePlayconfig* ptrConfig = StressConfigInst.getCoopGamePlayConfig();
                logoutProbable = ptrConfig ? (ptrConfig->LogoutProbability) : logoutProbable;
            }
            break;
        case FUTPLAYER :
            {
                FutPlayerGamePlayConfig* ptrConfig = StressConfigInst.getFutPlayerGamePlayConfig();
                logoutProbable = ptrConfig ? (ptrConfig->LogoutProbability) : logoutProbable;
            }
            break;
        case SINGLEPLAYER :
            {
                SingleplayerGamePlayConfig* ptrConfig = StressConfigInst.getSingleplayerGamePlayConfig();
                logoutProbable = ptrConfig ? (ptrConfig->LogoutProbability) : logoutProbable;
            }
			break;
		case KICKOFFPLAYER:
		{
			KickoffplayerGamePlayConfig* ptrConfig = StressConfigInst.getKickoffplayerGamePlayConfig();
			logoutProbable = ptrConfig ? (ptrConfig->LogoutProbability) : logoutProbable;
		}
		break;
		case LIVECOMPETITIONS :
            {
                LiveCompetitionsGamePlayConfig* ptrConfig = StressConfigInst.getLiveCompetitionsGamePlayConfig();
                logoutProbable = ptrConfig ? (ptrConfig->LogoutProbability) : logoutProbable;
            }
            break;
		case CLUBSPLAYER :
            {
                ClubsPlayerGamePlayconfig* ptrConfig = StressConfigInst.getClubsPlayerGamePlayConfig();
                logoutProbable = ptrConfig ? (ptrConfig->LogoutProbability) : logoutProbable;
            }
            break;
		case DROPINPLAYER:
		{
			DropinPlayerGamePlayconfig* ptrConfig = StressConfigInst.getDropinPlayerGamePlayConfig();
			logoutProbable = ptrConfig ? (ptrConfig->LogoutProbability) : logoutProbable;
		}
		break;
		case SSFPLAYER:
		{
			SSFPlayerGamePlayConfig* ptrConfig = StressConfigInst.getSSFPlayerGamePlayConfig();
			logoutProbable = ptrConfig ? (ptrConfig->LogoutProbability) : logoutProbable;
		}
		break;
		case SSFSEASONS:
		{
			SsfSeasonsGamePlayConfig* ptrConfig = StressConfigInst.getSsfSeasonsGamePlayConfig();
			logoutProbable = ptrConfig ? (ptrConfig->LogoutProbability) : logoutProbable;
		}
		break;
        default :
            break;
    }
    return logoutProbable;
}

bool StressConfigManager::parseScenarioDistributionByPlayerType(const ConfigMap& config, PlayerType playerType)
{
    const ConfigSequence* distributionScenarioType = NULL;
	switch (playerType)
	{
	case COOPPLAYER:
		distributionScenarioType = config.getSequence("CoopScenarioDistribution");
		break;
	case ONLINESEASONS:
		distributionScenarioType = config.getSequence("OnlineSeasonsDistribution");
		break;
	case FUTPLAYER:
		distributionScenarioType = config.getSequence("FutPlayerDistribution");
		break;
	case FRIENDLIES:
		distributionScenarioType = config.getSequence("FriendliesDistribution");
		break;
	case CLUBSPLAYER:
	{distributionScenarioType = config.getSequence("ClubsPlayerDistribution");break; }
	case KICKOFFPLAYER:
		{distributionScenarioType = config.getSequence("KickOffPlayerDistribution");break;}
	case SSFPLAYER :
		{distributionScenarioType = config.getSequence("SSFPlayerDistribution"); break;}
	case SSFSEASONS:
		distributionScenarioType = config.getSequence("SSFSeasonsDistribution");
		break;
    //   No scenario distribution for SinglePlayer
        default :
            break;
    }
    if (!distributionScenarioType)
    {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing ScenarioTypeDistribution Type:%d", playerType);
        return false;
    }
    else
    {
        uint8_t sumProb = 0;
        ScenarioTypeDistributionMap ScenarioDistribution;
        for (size_t i = 0; i < distributionScenarioType->getSize(); ++i)
        {
            const ConfigMap* entry = distributionScenarioType->nextMap();
            ScenarioType type = (ScenarioType)entry->getUInt8("type", 0);
            uint8_t probability = entry->getUInt8("probability", 0);
            sumProb += probability;
            ScenarioDistribution[type] = probability;
        }
        if (sumProb == 0) {  //  there is no distribution defined for this player type, just return without adding to the map
			BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] ScenarioTypeDistribution Type:%d distribution/probability's total is 0 percent", playerType);
            return true;
        }
        if (sumProb != 100)
        {
            BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing ScenarioTypeDistribution Type:%d distribution/probability's total is not 100 percent", playerType);
            return false;
        }
        mPlayerScenarioDistribution[playerType] = ScenarioDistribution;
    }
    return true;
}

bool StressConfigManager::parseArubaConfig(const ConfigMap& config)
{
    const ConfigMap* arubaConfigs = config.getMap("arubaConfigs");
    if (arubaConfigs == NULL)
    {
        BLAZE_ERR(BlazeRpcLog::stats, "[StressConfigManager::parseArubaConfig] : arubaConfigs not found.");
        return false;
    }
    // Aruba realated configs
    mArubaServerUri		= arubaConfigs->getString("arubaServerUri", "https://pin-em-lt.data.ea.com");
    mArubaGetMessageUri = arubaConfigs->getString("arubaGetMessageUri", "/em/v4/action");
    mArubaTrackingUri	= arubaConfigs->getString("arubaTrackingUri", "/em/v4/tracking/impressions");
	mArubaEnabled		= arubaConfigs->getBool("arubaEnabled", false);  
    return true;
}

bool StressConfigManager::parseUPSConfig(const ConfigMap& config)
{
	const ConfigMap* upsConfigs = config.getMap("upsConfigs");
	if (upsConfigs == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::stats, "[StressConfigManager::parseUPSConfig] : upsConfigs not found.");
		return false;
	}
	//UPS realated configs
	mUPSServerUri = upsConfigs->getString("upsServerUri", "https://info-load.social.ea.com");
	mUPSGetMessageUri = upsConfigs->getString("upsGetMessageUri", "/em/v4/action");
	mUPSTrackingUri = upsConfigs->getString("upsTrackingUri", "/em/v4/tracking/impressions");
	mUPSEnabled = upsConfigs->getBool("upsEnabled", false);
	mTestClientId = upsConfigs->getString("testClientId", "test_token_issuer");
	mClientSecret = upsConfigs->getString("clientSecret", "test_token_issuer_secret");
	return true;
}

bool StressConfigManager::parseRecoConfig(const ConfigMap& config)
{
	const ConfigMap* recoConfigs = config.getMap("recoConfigs");
	if (recoConfigs == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::stats, "[StressConfigManager::parseRecoConfig] : recoConfigs not found.");
		return false;
	}
	//Reco realated configs
	mRecoServerUri = recoConfigs->getString("recoServerUri", "https://info-load.social.ea.com");
	mRecoGetMessageUri = recoConfigs->getString("recoGetMessageUri", "/em/v4/action");
	mRecoTrackingUri = recoConfigs->getString("recoTrackingUri", "/em/v4/tracking/impressions");
	mRecoEnabled = recoConfigs->getBool("recoEnabled", false);
	return true;
}
bool StressConfigManager::parseStatsConfig(const ConfigMap& config)
{
	const ConfigMap* statsProxyConfigs = config.getMap("statsProxyConfigs");
	if (statsProxyConfigs == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::stats, "[StressConfigManager::parseStatsConfigs] : statsConfigs not found.");
		return false;
	}
	//Stats realated configs
	mStatsProxyServerUri = statsProxyConfigs->getString("statsProxyServerUri", "https://internal.stats.e2etest.gameservices.ea.com:11000");
	mStatsProxyGetMessageUri = statsProxyConfigs->getString("statsGetMessageUri", "/api/1.0/contexts/FIFA21/views/");
	mStatsPlayerHealthUri = statsProxyConfigs->getString("statsPlayerHealthUri", "/api/1.0/contexts/FIFA21_PT/views/AllPlayTimeStatsView/entities/stats");
	mOptInProbability = statsProxyConfigs->getUInt32("optInProbability",30);
	mEadpStatsProbability = statsProxyConfigs->getUInt32("eadpStatsProbability", 30);
	mOfflineStatsProbability = statsProxyConfigs->getUInt32("offlineStatsProbability", 20);
	mStatsProxyEnabled = statsProxyConfigs->getBool("statsProxyEnabled", false);
	mPlayTimeStatsEnabled = statsProxyConfigs->getBool("playTimeStatsEnabled", false);
	mUpdateManualStats = statsProxyConfigs->getBool("updateManualStats", false);
	return true;
}
bool StressConfigManager::parseESLConfig(const ConfigMap& config)
{
	const ConfigMap* eslProxyConfigs = config.getMap("eslProxyConfigs");
	if (eslProxyConfigs == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::stats, "[StressConfigManager::eslProxyConfigs] : eslProxyConfigs not found.");
		return false;
	}
	//Stats realated configs
	mESLProxyServerUri = eslProxyConfigs->getString("eslProxyServerUri", "https://test.wal2.tools.gos.bio-dub.ea.com/wal2");
	mESLProxyGetMatchResultsUri = eslProxyConfigs->getString("eslGetMessageUri", "/gamereporting/getTournamentGameReportView");
	mESLProxyEnabled = eslProxyConfigs->getBool("eslProxyEnabled", false);
	return true;
}

bool StressConfigManager::parsePINConfig(const ConfigMap& config)
{
	const ConfigMap* pinConfigs = config.getMap("pinConfigs");
	if (pinConfigs == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::stats, "[StressConfigManager::parsePINConfigs] : pinConfigs not found.");
		return false;
	}
	//PIN realated configs
	mPINServerUri = pinConfigs->getString("pinServerUri", "https://pin-em-lt.data.ea.com");
	mPINGetMessageUri = pinConfigs->getString("pinGetMessageUri", "/em/v4/action");
	mPINTrackingUri = pinConfigs->getString("pinTrackingUri", "/em/v4/tracking/impressions");
	mPINEnabled = pinConfigs->getBool("pinEnabled", false);
	return true;
}

bool StressConfigManager::parseKickOffConfig(const ConfigMap& config)
{
	const ConfigMap* kickOffConfigs = config.getMap("kickOffConfigs");
	if (kickOffConfigs == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::stats, "[StressConfigManager::parseKickOffConfig] :kickOffConfigs not found.");
		return false;
	}
	//Kick off realated configs
	mKickOffServerUri = kickOffConfigs->getString("kickOffServerUri", "https://info-load.social.ea.com");
	mKickOffGetMessageUri = kickOffConfigs->getString("kickOffGetMessageUri", "/em/v4/action");
	mKickOffTrackingUri = kickOffConfigs->getString("kickOffTrackingUri", "/em/v4/tracking/impressions");
	mKickOffEnabled = kickOffConfigs->getBool("kickOffEnabled", false);
	return true;
}


bool StressConfigManager::parseGroupsConfig(const ConfigMap& config)
{
	const ConfigMap* groupsConfigs = config.getMap("groupsConfigs");
	if (groupsConfigs == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::stats, "[StressConfigManager::parseGroupsConfig] :groupsConfigs not found.");
		return false;
	}
	//Kick off realated configs
	mGroupsServerUri = groupsConfigs->getString("groupsServerUri", "https://groups-e2e-black.tntdev.tnt-ea.com:443");
	mGroupsSearchUri = groupsConfigs->getString("groupsSearchUri", "/group/instance/search");
	mKickOffEnabled = groupsConfigs->getBool("groupsEnabled", false);
	return true;
}

bool StressConfigManager::parseProfanityConfig(const ConfigMap& config)
{
	const ConfigMap* profanityConfigs = config.getMap("profanityConfiguration");
	if (profanityConfigs == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::stats, "[StressConfigManager::parseProfanityConfig] :profanityConfigs not found.");
		return false;
	}

	mProfanityAvatarProbability = profanityConfigs->getUInt32("profanityAvatarProbability", 20);
	mProfanityClubnameProbability = profanityConfigs->getUInt32("profanityClubnameProbability", 10);
	mProfanityMessageProbability = profanityConfigs->getUInt32("profanityMessageProbability", 60);
	
	mFileAvatarNameProbability = profanityConfigs->getUInt32("fileAvatarNameProbability", 20);
	mFileClubNameProbability = profanityConfigs->getUInt32("fileClubNameProbability", 20);
	mFileAIPlayerNamesProbability = profanityConfigs->getUInt32("fileAIPlayerNamesProbability", 20);
	mAcceptPetitionProbability = profanityConfigs->getUInt32("acceptPetitionProbability", 20);

	const ConfigSequence* avatarFirstNames = NULL;
	const ConfigSequence* avatarLastNames = NULL;
	const ConfigSequence* clubNames = NULL;
	const ConfigSequence* inGameMessages = NULL;
	const ConfigSequence* aiPlayerNames = NULL;

	avatarFirstNames = profanityConfigs->getSequence("AvatarFirstNames");
	avatarLastNames = profanityConfigs->getSequence("AvatarLastNames");
	clubNames = profanityConfigs->getSequence("ClubNames");
	inGameMessages = profanityConfigs->getSequence("InGameMessages");
	aiPlayerNames = profanityConfigs->getSequence("AIPlayerNames");

	if (!avatarFirstNames)
	{
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing AvatarFirstNames");
		return false;
	}
	while (avatarFirstNames->hasNext())
	{
		mAvatarFirstNames.push_back(avatarFirstNames->nextString(0));
	}

	if (!avatarFirstNames)
	{
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing AvatarFirstNames");
		return false;
	}
	while (avatarFirstNames->hasNext())
	{
		mAvatarFirstNames.push_back(avatarFirstNames->nextString(0));
	}

	if (!avatarLastNames)
	{
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing AvatarLastNames");
		return false;
	}
	while (avatarLastNames->hasNext())
	{
		mAvatarLastNames.push_back(avatarLastNames->nextString(0));
	}

	if (!clubNames)
	{
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing clubNames");
		return false;
	}
	while (clubNames->hasNext())
	{
		mClubNames.push_back(clubNames->nextString(0));
	}

	if (!inGameMessages)
	{
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing inGameMessages");
		return false;
	}
	while (inGameMessages->hasNext())
	{
		mInGameMessages.push_back(inGameMessages->nextString(0));
	}

	if (!aiPlayerNames)
	{
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing aiPlayerNames");
		return false;
	}
	while (aiPlayerNames->hasNext())
	{
		mAIPlayerNames.push_back(aiPlayerNames->nextString(0));
	}

	return true;
}

eastl::string StressConfigManager::getRandomAvatarFirstName()
{
	uint32_t sizeOfItems = mAvatarFirstNames.size();
	if (sizeOfItems > 0)
	{
		int randNum = Random::getRandomNumber(sizeOfItems);
		return mAvatarFirstNames[randNum];
	}
	return "0";
}

eastl::string StressConfigManager::getRandomAvatarLastName()
{
	uint32_t sizeOfItems = mAvatarLastNames.size();
	if (sizeOfItems > 0)
	{
		int randNum = Random::getRandomNumber(sizeOfItems);
		return mAvatarLastNames[randNum];
	}
	return "0";
}

eastl::string StressConfigManager::getRandomClub()
{
	uint32_t sizeOfItems = mClubNames.size();
	if (sizeOfItems > 0)
	{
		int randNum = Random::getRandomNumber(sizeOfItems);
		return mClubNames[randNum];
	}
	return "0";
}

eastl::string StressConfigManager::getRandomMessage()
{
	uint32_t sizeOfItems = mInGameMessages.size();
	if (sizeOfItems > 0)
	{
		int randNum = Random::getRandomNumber(sizeOfItems);
		return mInGameMessages[randNum];
	}
	return "0";
}

eastl::string StressConfigManager::getRandomAIPlayerName()
{
	uint32_t sizeOfItems = mAIPlayerNames.size();
	if (sizeOfItems > 0)
	{
		int randNum = Random::getRandomNumber(sizeOfItems);
		return mInGameMessages[randNum];
	}
	return "0";
}

PlayerType StressConfigManager::getRandomPlayerType(StressPlayerInfo* playerData)
{
    PlayerType ret = INVALID;
    uint32_t rand = static_cast<uint32_t>(Blaze::Random::getRandomNumber(mPlayerCountDistribution.size()));
    PlayerTypeDistributionMap::iterator itr = mPlayerCountDistribution.begin();
    eastl::advance(itr, rand);
    mMutex.Lock();
    for (uint32_t loopCount = 0; loopCount < mPlayerCountDistribution.size(); loopCount++, ++itr)
    {
        if (mPlayerCountDistribution.end() == itr)
        {
            itr = mPlayerCountDistribution.begin();
        }
        if (itr->second > 0)
        {
            itr->second--;
            ret = itr->first;
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressConfig] getRandomPlayerType " <<  "  Type = " << ret << " count = " << itr->second);
            break;
        }
    }

	//   determine pingsite
	{
		int32_t sumprobability = 0;
		int32_t randomprobability = Random::getRandomNumber(100);
		PinSiteProbabilityMap pingSiteMap = StressConfigInst.getPingSiteProbabilityMap();
		PinSiteProbabilityMap::iterator it = pingSiteMap.begin();
		while(it != pingSiteMap.end())
		{
			sumprobability += it->second;			
			if(randomprobability <= sumprobability)
			{				
				playerData->setPlayerPingSite(it->first.c_str());
				break;
			}
			it++;
		}
		BLAZE_TRACE_LOG(BlazeRpcLog::sponsoredevents, "[StressConfigManager::getRandomPlayerType] " << playerData->getPlayerId() << " PlayerPingSite " << playerData->getPlayerPingSite());
	}
    //   assign friendlist size here if not already done. It is assigned only once and used throughtout the test
    if (playerData->getFriendlistSize() == 0 && mFriendlistDistribution.size() > 0)
    {
        rand = static_cast<uint32_t>(Blaze::Random::getRandomNumber(mFriendlistDistribution.size()));
        FriendlistSizeDistributionMap::iterator itrFriend = mFriendlistDistribution.begin();
        eastl::advance(itrFriend, rand);
        for (uint32_t loopCount = 0; loopCount < mFriendlistDistribution.size(); loopCount++, ++itrFriend)
        {
            if (mFriendlistDistribution.end() == itrFriend)
            {
                itrFriend = mFriendlistDistribution.begin();
            }
            if ((*itrFriend).second.count > 0)
            {
                (*itrFriend).second.count--;
                playerData->setFriendlistSize(itrFriend->first);
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressConfig] getRandomPlayerType " <<  "  friendListSize = " << itrFriend->first);
                break;
            }
        }
    }
    mMutex.Unlock();
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressConfig] getRandomPlayerType " <<  " playertype = " << ret << " pingsite = " << playerData->getPlayerPingSite());
    //ASSERT(ret != INVALID);
    if (ret == INVALID)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[StressConfig] getRandomPlayerType unable to get Player Type");
    }
    return ret;
}

//   We maintain exact count of available players types, gameModes and pingSites as per the distribution.
//   getRandomPlayerType() reduces the slot count and when the player has finished the game play,
//   it calls recyclePlayerType() to increase the count again and thus makes the type in the pool
void StressConfigManager::recyclePlayerType(PlayerType type, eastl::string pingSite)
{
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressConfig] recyclePlayerType " <<  " playertype = " << type << " pingsite = " << pingSite);
    mMutex.Lock();
    for (PlayerTypeDistributionMap::iterator itr = mPlayerCountDistribution.begin(); itr != mPlayerCountDistribution.end(); ++itr)
    {
        if (itr->first == type)
        {
            itr->second++;
            break;
        }
    }
    mMutex.Unlock();
}

ScenarioType StressConfigManager::getRandomScenarioByPlayerType(PlayerType type) const
{
    ScenarioType scenario = NONE;
    PlayerScenarioDistributionMap::const_iterator it = mPlayerScenarioDistribution.find(type);
	BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressConfig] getRandomScenarioByPlayerType() in this function and teh map size id "<< mPlayerScenarioDistribution.size());
    if (it != mPlayerScenarioDistribution.end())
    {
        ScenarioTypeDistributionMap scenarioDistribution = it->second;
        uint32_t rand = static_cast<uint32_t>(Blaze::Random::getRandomNumber(100)) + 1;
        uint8_t sumProbalities = 0;
        for ( ScenarioTypeDistributionMap::const_iterator itr = scenarioDistribution.begin(); itr != scenarioDistribution.end(); ++itr )
        {
			BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressConfig] print scenario distribution : " << "  current sub scenario " << itr->first << "Current probablity = " << itr->second);
            sumProbalities += itr->second;
            if (rand <= sumProbalities)
            {
                scenario = itr->first;
				BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressConfig] This is the scenario that is got finally : " << "  current sub scenario " << itr->first << "Probablity Sum = " << sumProbalities << " Randm roulette" << rand);
                break;
            }
        }
    }
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressConfig] GetRandomScenarioByPlayerType " <<  "  Type = " << type << "Scenario = " << scenario);
    return scenario;
}

GameplayConfig* StressConfigManager::getGamePlayConfig(PlayerType type) const
{
    for ( PlayerGamePlayConfigMap::const_iterator itr = mPlayerGamePlayConfigMap.begin(); itr != mPlayerGamePlayConfigMap.end(); ++itr )
    {
        if (type == itr->first) {
            return itr->second;
        }
    }
    return NULL;
}

void StressConfigManager::UpdateCommonGamePlayConfig(const ConfigMap* ptrConfigMap, GameplayConfig* ptrGamePlayConfig, GameplayConfig* ptrDefaultConfig)
{
    ptrGamePlayConfig->GameStateIterationsMax = ptrConfigMap->getUInt32("GameStateIterationsMax", ((!ptrDefaultConfig) ? 2 : ptrDefaultConfig->GameStateIterationsMax));
    ptrGamePlayConfig->GameStateSleepInterval = ptrConfigMap->getUInt32("GameStateSleepInterval", ((!ptrDefaultConfig) ? 6000 : ptrDefaultConfig->GameStateSleepInterval));
    ptrGamePlayConfig->cancelMatchMakingProbability = ptrConfigMap->getUInt32("cancelMatchMakingProbability", ((!ptrDefaultConfig) ? 5 : ptrDefaultConfig->cancelMatchMakingProbability));
    ptrGamePlayConfig->MatchMakingSessionDuration = ptrConfigMap->getUInt32("MatchMakingSessionDuration", ((!ptrDefaultConfig) ? 15000 : ptrDefaultConfig->MatchMakingSessionDuration));
    ptrGamePlayConfig->LogoutProbability = ptrConfigMap->getUInt32("LogoutProbability", ((!ptrDefaultConfig) ? 10 : ptrDefaultConfig->LogoutProbability));
    ptrGamePlayConfig->reLoginWaitTime = ptrConfigMap->getUInt32("reLoginWaitTime", ((!ptrDefaultConfig) ? 60000 : ptrDefaultConfig->reLoginWaitTime));
	ptrGamePlayConfig->loginRateTime = ptrConfigMap->getUInt32("loginRateTime", ((!ptrDefaultConfig) ? 60000 : ptrDefaultConfig->loginRateTime));
    ptrGamePlayConfig->maxLobbyWaitTime = ptrConfigMap->getUInt32("maxLobbyWaitTime", 30000);
    ptrGamePlayConfig->maxPlayerCount = ptrConfigMap->getUInt16("maxPlayerCount",  ((!ptrDefaultConfig) ? 64 : ptrDefaultConfig->maxPlayerCount));
    ptrGamePlayConfig->friendsListUpdateProbability = ptrConfigMap->getUInt32("friendsListUpdateProbability", ((!ptrDefaultConfig) ? 5 : ptrDefaultConfig->friendsListUpdateProbability));
    ptrGamePlayConfig->maxPlayerStateDurationMs = ptrConfigMap->getUInt32("maxPlayerStateDurationMs", ((!ptrDefaultConfig) ? 120000 : ptrDefaultConfig->maxPlayerStateDurationMs));
    ptrGamePlayConfig->maxGamesPerSession = ptrConfigMap->getUInt32("maxGamesPerSession", ((!ptrDefaultConfig) ? 5 : ptrDefaultConfig->maxGamesPerSession));
    ptrGamePlayConfig->processInvitesFiberSleep = ptrConfigMap->getUInt32("ProcessInvitesFiberSleep", ((!ptrDefaultConfig) ? 5 : ptrDefaultConfig->processInvitesFiberSleep));
    ptrGamePlayConfig->maxLoggedInDuration = ptrConfigMap->getUInt64("maxLoggedInDuration", ((!ptrDefaultConfig) ? 7200 : ptrDefaultConfig->maxLoggedInDuration));
    ptrGamePlayConfig->maxSetUsersToListCount = ptrConfigMap->getUInt32("maxSetUsersToListCount", ((!ptrDefaultConfig) ? 100 : ptrDefaultConfig->maxSetUsersToListCount));
    ptrGamePlayConfig->setUsersToListProbability = ptrConfigMap->getUInt32("setUsersToListProbability", ((!ptrDefaultConfig) ? 20 : ptrDefaultConfig->setUsersToListProbability));
    ptrGamePlayConfig->setUsersToListAddPlayerCount = ptrConfigMap->getUInt32("setUsersToListAddPlayerCount", ((!ptrDefaultConfig) ? 5 : ptrDefaultConfig->setUsersToListAddPlayerCount));
    ptrGamePlayConfig->setUsersToListAddPlayerDeviationCount = ptrConfigMap->getUInt32("setUsersToListAddPlayerDeviationCount",
            ((!ptrDefaultConfig) ? 5 : ptrDefaultConfig->setUsersToListAddPlayerDeviationCount));
    ptrGamePlayConfig->playerJoinTimeoutWait = ptrConfigMap->getUInt32("playerJoinTimeoutWait", ((!ptrDefaultConfig) ? 60000 : ptrDefaultConfig->playerJoinTimeoutWait));	
}

bool StressConfigManager::parseCommonGamePlayConfig(const ConfigMap& config)
{
    const ConfigMap* commonConfigMap = config.getMap("CommonGamePlayConfiguration");
    if (!commonConfigMap)
    {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing CommonGamePlayConfiguration");
        return false;
    }
    mCommonConfigPtr = BLAZE_NEW GameplayConfig;
    UpdateCommonGamePlayConfig(commonConfigMap, mCommonConfigPtr, NULL);
    return true;
}

bool StressConfigManager::parseShieldConfiguration(const ConfigMap& config)
{
	mShieldMap = config.getMap("ShieldConfiguration");
	if (!mShieldMap)
	{
		BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing ShieldConfiguration");
		return false;
	}
	return true;
}

bool StressConfigManager::isOnlineScenario(ScenarioType scenariotype) const
{
	//	isOnlineScenario(Blaze::Stress::ScenarioType::FUTDRAFTMODE);
		mShieldMap->resetIter();
		while (mShieldMap->hasNext())
		{
			bool isDefault = true;
			int scenario = atoi(mShieldMap->nextItem()->getString("0", isDefault));
			if (scenariotype == scenario)
				return true;
		}
	return false;
}

bool StressConfigManager::parseGamePlayConfigByPlayerType(const ConfigMap& config, PlayerType type)
{
    if (!mCommonConfigPtr)
    {
        BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing GamePlay Configuration, Common Configurtion not Loaded");
        return false;
    }
    switch (type)
    {
        case SINGLEPLAYER:
            {
                const ConfigMap* singleConfigMap = config.getMap("SingleGamePlayConfiguration");
                if (!singleConfigMap)
                {
                    BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing SingleGamePlayconfig");
                    return false;
                }
                SingleplayerGamePlayConfig* singleConfigPtr = BLAZE_NEW SingleplayerGamePlayConfig();
                singleConfigPtr->RoundInGameDuration		= singleConfigMap->getUInt32("RoundInGameDuration", 900000);
                singleConfigPtr->InGameDeviation			= singleConfigMap->getUInt32("InGameDeviation", 300000);
				singleConfigPtr->LeaderboardCallProbability = singleConfigMap->getUInt16("LeaderboardCallProbability", 25);
                singleConfigPtr->maxSeasonsToPlay			= singleConfigMap->getUInt16("MaxSeasonsToPlay", 6);
                singleConfigPtr->maxStatsForOfflineGameReport = singleConfigMap->getUInt32("maxStatsForOfflineGameReport", 1000);
                singleConfigPtr->QuitFromGameProbability	= singleConfigMap->getUInt32("QuitFromGameProbability", 5);
                
                //  over writing common values if any
                UpdateCommonGamePlayConfig(singleConfigMap, singleConfigPtr, mCommonConfigPtr);
                mPlayerGamePlayConfigMap[SINGLEPLAYER] = singleConfigPtr;
                break;
			    }
		case KICKOFFPLAYER:
		{
			const ConfigMap* kickoffConfigMap = config.getMap("KickoffGamePlayConfiguration");
			if (!kickoffConfigMap)
			{
				BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing KickoffGamePlayConfiguration");
				return false;
			}
			KickoffplayerGamePlayConfig* kickoffConfigPtr = BLAZE_NEW KickoffplayerGamePlayConfig();
			kickoffConfigPtr->RoundInGameDuration = kickoffConfigMap->getUInt32("RoundInGameDuration", 900000);
			kickoffConfigPtr->InGameDeviation = kickoffConfigMap->getUInt32("InGameDeviation", 300000);
			kickoffConfigPtr->LeaderboardCallProbability = kickoffConfigMap->getUInt16("LeaderboardCallProbability", 25);
			kickoffConfigPtr->maxSeasonsToPlay = kickoffConfigMap->getUInt16("MaxSeasonsToPlay", 6);
			kickoffConfigPtr->maxStatsForOfflineGameReport = kickoffConfigMap->getUInt32("maxStatsForOfflineGameReport", 1000);
			kickoffConfigPtr->QuitFromGameProbability = kickoffConfigMap->getUInt32("QuitFromGameProbability", 5);

			//  over writing common values if any
			UpdateCommonGamePlayConfig(kickoffConfigMap, kickoffConfigPtr, mCommonConfigPtr);
			mPlayerGamePlayConfigMap[KICKOFFPLAYER] = kickoffConfigPtr;
			break;
		}
		case DEDICATEDSERVER:
			{
                const ConfigMap* dedicatedServerPlayerConfigMap = config.getMap("DedicatedServerGamePlayConfiguration");
                if (!dedicatedServerPlayerConfigMap)
                {
                    BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing DedicatedServerGamePlayconfig");
                    return false;
                }
                DedicatedServerGamePlayconfig* DedicatedServerConfigPtr = BLAZE_NEW DedicatedServerGamePlayconfig();
                DedicatedServerConfigPtr->GameStateSleepInterval		= dedicatedServerPlayerConfigMap->getUInt32("GameStateSleepInterval", 30000);
                DedicatedServerConfigPtr->MinPlayerCount			= dedicatedServerPlayerConfigMap->getUInt16("MinPlayerCount", 1);
                DedicatedServerConfigPtr->MaxPlayerCount	= dedicatedServerPlayerConfigMap->getUInt16("MaxPlayerCount", 12);
                DedicatedServerConfigPtr->GameStateIterationsMax	= dedicatedServerPlayerConfigMap->getUInt32("GameStateIterationsMax", 10);
                DedicatedServerConfigPtr->roundInGameDuration		= dedicatedServerPlayerConfigMap->getUInt32("RoundInGameDuration", 900000);
                DedicatedServerConfigPtr->inGameDeviation			= dedicatedServerPlayerConfigMap->getUInt32("InGameDeviation", 300000);

                UpdateCommonGamePlayConfig(dedicatedServerPlayerConfigMap, DedicatedServerConfigPtr, mCommonConfigPtr);
                mPlayerGamePlayConfigMap[DEDICATEDSERVER] = DedicatedServerConfigPtr;
                break;
            }
		case COOPPLAYER:
			{
                const ConfigMap* coopConfigMap = config.getMap("CoopGamePlayConfiguration");
                if (!coopConfigMap)
                {
                    BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing CoopGamePlayconfig");
                    return false;
                }
                CoopGamePlayconfig* CoopConfigPtr = BLAZE_NEW CoopGamePlayconfig();
                CoopConfigPtr->roundInGameDuration				= coopConfigMap->getUInt32("RoundInGameDuration", 540000);
                CoopConfigPtr->inGameDeviation					= coopConfigMap->getUInt32("InGameDeviation", 300000);
                CoopConfigPtr->JoinerLeaveProbability			= coopConfigMap->getUInt32("JoinerLeaveProbability", 10);
                CoopConfigPtr->PostGameMaxTickerCount			= coopConfigMap->getUInt32("PostGameMaxTickerCount", 120);
                CoopConfigPtr->maxStatsForOfflineGameReport		= coopConfigMap->getUInt32("maxStatsForOfflineGameReport", 10);
				CoopConfigPtr->GameStateIterationsMax			= coopConfigMap->getUInt32("GameStateIterationsMax", 3);
				CoopConfigPtr->inGameReportTelemetryProbability = coopConfigMap->getUInt32("inGameReportTelemetryProbability", 5);
                //  over writing common values if any
                UpdateCommonGamePlayConfig(coopConfigMap, CoopConfigPtr, mCommonConfigPtr);
                mPlayerGamePlayConfigMap[COOPPLAYER] = CoopConfigPtr;
                break;
            }
        case ONLINESEASONS :
            {
                const ConfigMap* multiConfigMap = config.getMap("OnlineSeasonsGamePlayConfiguration");
                if (!multiConfigMap)
                {
                    BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing OnlineSeasonsGamePlayConfig");
                    return false;
                }
                OnlineSeasonsGamePlayConfig* OnlineSeasonsConfigPtr = BLAZE_NEW OnlineSeasonsGamePlayConfig();
                OnlineSeasonsConfigPtr->roundInGameDuration		= multiConfigMap->getUInt32("RoundInGameDuration", 540000);
                OnlineSeasonsConfigPtr->inGameDeviation			= multiConfigMap->getUInt32("InGameDeviation", 300000);
                OnlineSeasonsConfigPtr->HostLeaveProbability	= multiConfigMap->getUInt32("HostLeaveProbability", 10);
                OnlineSeasonsConfigPtr->JoinerLeaveProbability	= multiConfigMap->getUInt32("JoinerLeaveProbability", 5);
                OnlineSeasonsConfigPtr->KickPlayerProbability	= multiConfigMap->getUInt32("KickPlayerProbability", 5);
                OnlineSeasonsConfigPtr->inGameReportTelemetryProbability = multiConfigMap->getUInt32("inGameReportTelemetryProbability", 5);
                //  over writing common values if any
                UpdateCommonGamePlayConfig(multiConfigMap, OnlineSeasonsConfigPtr, mCommonConfigPtr);		
                mPlayerGamePlayConfigMap[ONLINESEASONS] = OnlineSeasonsConfigPtr;
                break;
            }
		case FRIENDLIES:
            {
				const ConfigMap* multiConfigMap = config.getMap("FriendliesGamePlayConfiguration");
                if (!multiConfigMap)
                {
                    BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing FriendliesGamePlayConfiguration");
                    return false;
                }
                FriendliesGamePlayConfig* FriendliesConfigPtr = BLAZE_NEW FriendliesGamePlayConfig();
                FriendliesConfigPtr->roundInGameDuration = multiConfigMap->getUInt32("RoundInGameDuration", 540000);
				FriendliesConfigPtr->GameStateIterationsMax = multiConfigMap->getUInt32("GameStateIterationsMax", 3);
                FriendliesConfigPtr->inGameDeviation = multiConfigMap->getUInt32("InGameDeviation", 300000);
				FriendliesConfigPtr->JoinerLeaveProbability = multiConfigMap->getUInt32("JoinerLeaveProbability", 0);
                FriendliesConfigPtr->HostLeaveProbability = multiConfigMap->getUInt32("HostLeaveProbability", 10);
                FriendliesConfigPtr->maxPlayerStateDurationMs = multiConfigMap->getUInt32("maxPlayerStateDurationMs", 1800000);
				FriendliesConfigPtr->GameStateSleepInterval = multiConfigMap->getUInt32("GameStateSleepInterval", 30000);
				FriendliesConfigPtr->inGameReportTelemetryProbability = multiConfigMap->getUInt32("inGameReportTelemetryProbability", 10);
				UpdateCommonGamePlayConfig(multiConfigMap, FriendliesConfigPtr, mCommonConfigPtr);
				mPlayerGamePlayConfigMap[FRIENDLIES] = FriendliesConfigPtr;
                break;
            }
		case LIVECOMPETITIONS:
            {
				const ConfigMap* multiConfigMap = config.getMap("LiveCompetitionGamePlayConfiguration");
                if (!multiConfigMap)
                {
                    BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing LiveCompetitionGamePlayConfiguration");
                    return false;
                }
                LiveCompetitionsGamePlayConfig* LiveCompetitionConfigPtr = BLAZE_NEW LiveCompetitionsGamePlayConfig();
				LiveCompetitionConfigPtr->roundInGameDuration = multiConfigMap->getUInt32("RoundInGameDuration", 540000);
                LiveCompetitionConfigPtr->inGameDeviation = multiConfigMap->getUInt32("InGameDeviation", 300000);
				UpdateCommonGamePlayConfig(multiConfigMap, LiveCompetitionConfigPtr, mCommonConfigPtr);
				mPlayerGamePlayConfigMap[LIVECOMPETITIONS] = LiveCompetitionConfigPtr;
                break;
            }
		case CLUBSPLAYER:
            {
				const ConfigMap* multiConfigMap = config.getMap("ClubsPlayerGamePlayConfiguration");
                if (!multiConfigMap)
                {
                    BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing ClubsPlayerGamePlayConfiguration");
                    return false;
                }
                ClubsPlayerGamePlayconfig* ClubsPlayerConfigPtr		= BLAZE_NEW ClubsPlayerGamePlayconfig();
				ClubsPlayerConfigPtr->roundInGameDuration			= multiConfigMap->getUInt32("RoundInGameDuration", 540000);
                ClubsPlayerConfigPtr->inGameDeviation				= multiConfigMap->getUInt32("InGameDeviation", 300000);
				ClubsPlayerConfigPtr->JoinerLeaveProbability		= multiConfigMap->getUInt32("JoinerLeaveProbability", 5);
                ClubsPlayerConfigPtr->inGameReportTelemetryProbability = multiConfigMap->getUInt32("inGameReportTelemetryProbability", 5);
				ClubsPlayerConfigPtr->cancelMatchMakingProbability	= multiConfigMap->getUInt32("cancelMatchMakingProbability", 5);
				ClubsPlayerConfigPtr->acceptInvitationProbablity	= multiConfigMap->getUInt32("acceptInvitationProbablity", 50);
				ClubsPlayerConfigPtr->GameStateIterationsMax		= multiConfigMap->getUInt32("GameStateIterationsMax", 5);
				ClubsPlayerConfigPtr->LogoutProbability				= multiConfigMap->getUInt32("LogoutProbability", 5);
				ClubsPlayerConfigPtr->ClubSizeMax					= multiConfigMap->getUInt32("ClubSizeMax", 5);
				ClubsPlayerConfigPtr->SeasonLevel					= multiConfigMap->getUInt32("SeasonLevel", 5);
				ClubsPlayerConfigPtr->ClubDomainId					= multiConfigMap->getUInt32("ClubDomainId", 5);
				ClubsPlayerConfigPtr->ClubsNameIndex				= multiConfigMap->getUInt32("ClubsNameIndex", 5);
				ClubsPlayerConfigPtr->PlayerWaitingCounter			= multiConfigMap->getUInt32("PlayerWaitingCounter", 8);
				ClubsPlayerConfigPtr->MinTeamSize					= multiConfigMap->getUInt32("MinTeamSize", 2);
				const ConfigSequence* distributionSeq = config.getSequence("ClubsSizeDistribution");
				if(!distributionSeq)
				{
					BLAZE_ERR_LOG(BlazeRpcLog::messaging, ": ClubsSizeDistribution sequence not defined in config!");
					return false;
				}
				uint8_t sumProb = 0;
				for(size_t i=0; i < distributionSeq->getSize(); ++i)
				{
					const ConfigMap* entry = distributionSeq->nextMap();
					uint8_t clubsize = entry->getUInt8("clubsize", 0);
					uint8_t probability = entry->getUInt8("probability", 0);
					sumProb += probability;

					ClubsPlayerConfigPtr->ClubSizeDistribution[clubsize] = probability;
				}
				if(sumProb != 100)
				{
					BLAZE_ERR_LOG(BlazeRpcLog::clubs, ": clubs size probability's total is not 100%");
					return false;
				}

				UpdateCommonGamePlayConfig(multiConfigMap, ClubsPlayerConfigPtr, mCommonConfigPtr);
				mPlayerGamePlayConfigMap[CLUBSPLAYER] = ClubsPlayerConfigPtr;
                break;
            }
		case DROPINPLAYER:
		{
			const ConfigMap* multiConfigMap = config.getMap("DropinPlayerGamePlayConfiguration");
			if (!multiConfigMap)
			{
				BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing DropinPlayerGamePlayConfiguration");
				return false;
			}
			DropinPlayerGamePlayconfig* DropinPlayerConfigPtr = BLAZE_NEW DropinPlayerGamePlayconfig();
			DropinPlayerConfigPtr->roundInGameDuration = multiConfigMap->getUInt32("RoundInGameDuration", 540000);
			DropinPlayerConfigPtr->inGameDeviation = multiConfigMap->getUInt32("InGameDeviation", 300000);
			DropinPlayerConfigPtr->JoinerLeaveProbability = multiConfigMap->getUInt32("JoinerLeaveProbability", 5);
			DropinPlayerConfigPtr->inGameReportTelemetryProbability = multiConfigMap->getUInt32("inGameReportTelemetryProbability", 5);
			DropinPlayerConfigPtr->cancelMatchMakingProbability = multiConfigMap->getUInt32("cancelMatchMakingProbability", 5);
			DropinPlayerConfigPtr->acceptInvitationProbablity = multiConfigMap->getUInt32("acceptInvitationProbablity", 50);
			DropinPlayerConfigPtr->GameStateIterationsMax = multiConfigMap->getUInt32("GameStateIterationsMax", 5);
			DropinPlayerConfigPtr->LogoutProbability = multiConfigMap->getUInt32("LogoutProbability", 5);
			DropinPlayerConfigPtr->ClubSizeMax = multiConfigMap->getUInt32("ClubSizeMax", 5);
			DropinPlayerConfigPtr->SeasonLevel = multiConfigMap->getUInt32("SeasonLevel", 5);
			DropinPlayerConfigPtr->ClubDomainId = multiConfigMap->getUInt32("ClubDomainId", 5);
			DropinPlayerConfigPtr->ClubsNameIndex = multiConfigMap->getUInt32("ClubsNameIndex", 5);
			DropinPlayerConfigPtr->PlayerWaitingCounter = multiConfigMap->getUInt32("PlayerWaitingCounter", 8);
			DropinPlayerConfigPtr->MinTeamSize = multiConfigMap->getUInt32("MinTeamSize", 2);
			const ConfigSequence* distributionSeq = config.getSequence("ClubsSizeDistribution");
			if (!distributionSeq)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::messaging, ": ClubsSizeDistribution sequence not defined in config!");
				return false;
			}
			uint8_t sumProb = 0;
			for (size_t i = 0; i < distributionSeq->getSize(); ++i)
			{
				const ConfigMap* entry = distributionSeq->nextMap();
				uint8_t clubsize = entry->getUInt8("clubsize", 0);
				uint8_t probability = entry->getUInt8("probability", 0);
				sumProb += probability;

				DropinPlayerConfigPtr->ClubSizeDistribution[clubsize] = probability;
			}
			if (sumProb != 100)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::clubs, ": clubs size probability's total is not 100%");
				return false;
			}

			UpdateCommonGamePlayConfig(multiConfigMap, DropinPlayerConfigPtr, mCommonConfigPtr);
			mPlayerGamePlayConfigMap[DROPINPLAYER] = DropinPlayerConfigPtr;
			break;
		}
        case FUTPLAYER :
            {
                const ConfigMap* multiConfigMap = config.getMap("FutPlayerGamePlayConfiguration");
                if (!multiConfigMap)
                {
                    BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing FutPlayerGamePlayConfiguration");
                    return false;
                }
                FutPlayerGamePlayConfig* FutPlayerConfigPtr = BLAZE_NEW FutPlayerGamePlayConfig();
                FutPlayerConfigPtr->roundInGameDuration		= multiConfigMap->getUInt32("RoundInGameDuration", 540000);
                FutPlayerConfigPtr->inGameDeviation			= multiConfigMap->getUInt32("InGameDeviation", 300000);
                FutPlayerConfigPtr->HostLeaveProbability	= multiConfigMap->getUInt32("HostLeaveProbability", 10);
                FutPlayerConfigPtr->JoinerLeaveProbability	= multiConfigMap->getUInt32("JoinerLeaveProbability", 5);
                FutPlayerConfigPtr->KickPlayerProbability	= multiConfigMap->getUInt32("KickPlayerProbability", 5);
                FutPlayerConfigPtr->inGameReportTelemetryProbability = multiConfigMap->getUInt32("inGameReportTelemetryProbability", 5);
                //  over writing common values if any
                UpdateCommonGamePlayConfig(multiConfigMap, FutPlayerConfigPtr, mCommonConfigPtr);		
                mPlayerGamePlayConfigMap[FUTPLAYER] = FutPlayerConfigPtr;
                break;
            }
		case SSFPLAYER:
		{
			const ConfigMap* multiConfigMap = config.getMap("SSFGamePlayConfiguration");
			if (!multiConfigMap)
			{
				BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing SSFPlayGamePlayConfiguration");
				return false;
			}
			SSFPlayerGamePlayConfig* SSFPlayerConfigPtr = BLAZE_NEW SSFPlayerGamePlayConfig();
			SSFPlayerConfigPtr->roundInGameDuration = multiConfigMap->getUInt32("RoundInGameDuration", 540000);
			SSFPlayerConfigPtr->inGameDeviation = multiConfigMap->getUInt32("InGameDeviation", 300000);
			SSFPlayerConfigPtr->joinerLeaveProbability = multiConfigMap->getUInt32("JoinerLeaveProbability", 5);
			SSFPlayerConfigPtr->quitFromGameProbability = multiConfigMap->getUInt32("QuitFromGameProbability", 5);
			//SSFPlayerConfigPtr->leaderboardCallProbability = multiConfigMap->getUInt32("LeaderboardCallProbability", 5);
			SSFPlayerConfigPtr->acceptInvitationProbablity = multiConfigMap->getUInt32("AcceptInvitationProbablity", 50);
			SSFPlayerConfigPtr->gameStateIterationsMax = multiConfigMap->getUInt32("GameStateIterationsMax", 5);
			SSFPlayerConfigPtr->logoutProbability = multiConfigMap->getUInt32("LogoutProbability", 5);
			SSFPlayerConfigPtr->clubSizeMax = multiConfigMap->getUInt32("ClubSizeMax", 5);
			//SSFPlayerConfigPtr->seasonLevel = multiConfigMap->getUInt32("SeasonLevel", 5);
			SSFPlayerConfigPtr->clubDomainId = multiConfigMap->getUInt32("ClubDomainId", 5);
			SSFPlayerConfigPtr->clubsNameIndex = multiConfigMap->getUInt32("ClubsNameIndex", 5);
			SSFPlayerConfigPtr->playerWaitingCounter = multiConfigMap->getUInt32("PlayerWaitingCounter", 8);
			SSFPlayerConfigPtr->minTeamSize = multiConfigMap->getUInt32("MinTeamSize", 2);
			SSFPlayerConfigPtr->inGameReportTelemetryProbability = multiConfigMap->getUInt32("InGameReportTelemetryProbability", 5);
			SSFPlayerConfigPtr->cancelMatchMakingProbability = multiConfigMap->getUInt32("CancelMatchMakingProbability", 5);
			const ConfigSequence* distributionSeq = config.getSequence("ClubsSizeDistribution");
			if (!distributionSeq)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::messaging, ": ClubsSizeDistribution sequence not defined in config!");
				return false;
			}
			uint8_t sumProb = 0;
			for (size_t i = 0; i < distributionSeq->getSize(); ++i)
			{
				const ConfigMap* entry = distributionSeq->nextMap();
				uint8_t clubsize = entry->getUInt8("clubsize", 0);
				uint8_t probability = entry->getUInt8("probability", 0);
				sumProb += probability;

				SSFPlayerConfigPtr->ClubSizeDistribution[clubsize] = probability;
			}
			if (sumProb != 100)
			{
				BLAZE_ERR_LOG(BlazeRpcLog::clubs, ": clubs size probability's total is not 100%");
				return false;
			}

			UpdateCommonGamePlayConfig(multiConfigMap, SSFPlayerConfigPtr, mCommonConfigPtr);
			mPlayerGamePlayConfigMap[SSFPLAYER] = SSFPlayerConfigPtr;
			break;
		}
		case SSFSEASONS:
		{
			const ConfigMap* multiConfigMap = config.getMap("SsfSeasonsGamePlayConfiguration");
			if (!multiConfigMap)
			{
				BLAZE_ERR(BlazeRpcLog::gamemanager, "[StressConfig] Failed in Parsing SsfSeasonsGamePlayConfig");
				return false;
			}
			SsfSeasonsGamePlayConfig* SsfSeasonsConfigPtr = BLAZE_NEW SsfSeasonsGamePlayConfig();
			SsfSeasonsConfigPtr->roundInGameDuration = multiConfigMap->getUInt32("RoundInGameDuration", 540000);
			SsfSeasonsConfigPtr->inGameDeviation = multiConfigMap->getUInt32("InGameDeviation", 300000);
			SsfSeasonsConfigPtr->HostLeaveProbability = multiConfigMap->getUInt32("HostLeaveProbability", 10);
			SsfSeasonsConfigPtr->JoinerLeaveProbability = multiConfigMap->getUInt32("JoinerLeaveProbability", 5);
			SsfSeasonsConfigPtr->KickPlayerProbability = multiConfigMap->getUInt32("KickPlayerProbability", 5);
			SsfSeasonsConfigPtr->inGameReportTelemetryProbability = multiConfigMap->getUInt32("inGameReportTelemetryProbability", 5);
			SsfSeasonsConfigPtr->clubSizeMax = multiConfigMap->getUInt32("ClubSizeMax", 5);
			//  over writing common values if any
			UpdateCommonGamePlayConfig(multiConfigMap, SsfSeasonsConfigPtr, mCommonConfigPtr);
			mPlayerGamePlayConfigMap[SSFSEASONS] = SsfSeasonsConfigPtr;
			break;
		}
        default :
            return false;
    }
    return true;
}

//  //  //  //  //  //  //  //  //  //  //  //  StressPlayerInfo  Implementation  //  //  //  //  //  //  //  /
StressPlayerInfo::StressPlayerInfo()
{
    playerId = 0;
    componentProxy = NULL;
	mArubaProxy = NULL;
	mUPSProxy = NULL;
	mStatsProxy = NULL;
    stressConnection = NULL;
    connGroupId = BlazeObjectId(0, 0, 0);
    mTotalLogins = 0;
    mViewId = 0;
    mIsFirstLogin = true;
    mAttemptReconnect = false;
    mIsPlayerActive = false;
    mGetStatGroupCalled = false;
    mGamesPerSessionCount = 0;
    mMyDetails = NULL;
    mLogInTime = 0;
    mMyPartyGameId = 0;
    mFriendlistSize = 0;
	mKickOffGuid = "";
}

StressPlayerInfo::~StressPlayerInfo()
{
    if (componentProxy) {
        delete componentProxy;
        componentProxy = NULL;
    }
	if (mArubaProxy) {
		delete mArubaProxy;
		mArubaProxy = NULL;
	}
	if (mUPSProxy) {
		delete mUPSProxy;
		mUPSProxy = NULL;
	}
}

bool StressPlayerInfo::isConnected()
{
    bool isConnected = false;
    if ((stressConnection != NULL) && stressConnection->connected()) {
        isConnected = true;
    }
    return isConnected;
}

bool StressPlayerInfo::queuePlayerToUpdate(BlazeId blazeId)
{
    eastl::set<BlazeId>::insert_return_type insertAddedUser = updatedUsersSet.insert(blazeId);
    return insertAddedUser.second;
}

void StressPlayerInfo::removeFromUpdatedUsersSet(BlazeId blazeId)
{
    updatedUsersSet.erase(blazeId);
}

bool StressPlayerInfo::isLoggedInForTooLong()
{
    if (!mLogInTime)
    {
        return false;
    }
    uint64_t  loggedInDurationMs = TimeValue::getTimeOfDay().getMillis() - mLogInTime;
    return (loggedInDurationMs >= StressConfigInst.getCommonConfig()->maxLoggedInDuration * 1000);  //  maxLoggedInDuration is given in seconds in the config
}

//  //  //  //  //  //  //  //  //  //  //  //   PlayerDetails implementation   //  //  //  //  //  //  //  /
PlayerDetails::PlayerDetails(StressPlayerInfo *playersData) : mIsAlive(true), mMyInfo(playersData), mFriendsInvite(0), mPartyInvite(0), mPartyGameId(0), mPartyFriend(0), mOnlineSeasonsGameId(0)
{
}

void PlayerDetails::setPlayerDead()
{
    mIsAlive = false;
    mFriendsInvite = 0;
    mPartyInvite = 0;
    mPartyGameId = 0;
    mPartyFriend = 0;
}

void PlayerDetails::setPlayerAlive(StressPlayerInfo* playerInfo)
{
    mIsAlive = true;
    mFriendsInvite = 0;
    mPartyInvite = 0;
    mPartyGameId = 0;
    mPartyFriend = 0;
    if (playerInfo != mMyInfo)
    {
        mMyInfo = playerInfo;
    }
}

void PlayerDetails::sendPartyInvite(PlayerDetails *friendsDetails, BlazeId inviteId)
{
    if (friendsDetails && !(friendsDetails->isInAParty()))
    {
        friendsDetails->setPartyInvite(inviteId);
        friendsDetails->setPartyFriend(mMyInfo->getPlayerId());
        mPartyGameId = getPlayerId();
        mPartyFriend = friendsDetails->getPlayerId();
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[PlayerDetails::sendPartyInvite][FAIL] friendsDeatil object is empty or friend is already in a party");
    }
}

//  //  //  //  //  //  //  //  //  //  //  //  StressGameData  Implementation  //  //  //  //  //  //  //  /
StressGameData::StressGameData(): mIsDedicatedServer(false)
{
}

StressGameData::StressGameData(bool isDedicatedServer): mIsDedicatedServer(isDedicatedServer)
{
}

StressGameData::~StressGameData()
{
    //  clear out all Game data
    while (!mGameDataMap.empty())
    {
        GameInfoMap::iterator it = mGameDataMap.begin();
        delete it->second;
        mGameDataMap.erase(it);
    }
}

GameInfoMap::iterator StressGameData::findGameIdInGameDataMap(Blaze::GameManager::GameId gameId)
{
    if (!mIsDedicatedServer)
    {
        mMutex.Lock();
    }
    GameInfoMap::iterator gameDataItr = mGameDataMap.find(gameId);
    if (!mIsDedicatedServer)
    {
        mMutex.Unlock();
    }
    return gameDataItr;
}

bool StressGameData::isGameIdExist(Blaze::GameManager::GameId gameId)
{
    GameInfoMap::iterator gameData = findGameIdInGameDataMap(gameId);
    if (gameData != mGameDataMap.end())
    {
        return true;
    }
    return false;
}
StressGameInfo* StressGameData::addGameData(NotifyGameSetup* gameSetup)
{
    StressGameInfo* gameInfo = BLAZE_NEW StressGameInfo;
    gameInfo->setGameId(gameSetup->getGameData().getGameId());
    gameInfo->setGameReportingId(gameSetup->getGameData().getGameReportingId());
    gameInfo->setTopologyHostPlayerId(gameSetup->getGameData().getTopologyHostInfo().getPlayerId());
    gameInfo->setPlatformHostPlayerId(gameSetup->getGameData().getPlatformHostInfo().getPlayerId());
    gameInfo->setGameState(gameSetup->getGameData().getGameState());
    gameInfo->setTopologyHostConnectionGroupId(gameSetup->getGameData().getTopologyHostInfo().getConnectionGroupId());
    gameInfo->setPlayerCapacity(gameSetup->getGameData().getMaxPlayerCapacity());
    ReplicatedGamePlayerList& playerList = gameSetup->getGameRoster();
    ReplicatedGamePlayerList::iterator start = playerList.begin();
    ReplicatedGamePlayerList::iterator end   = playerList.end();
    for (; start != end; start++)
    {
        UserInfo* info = BLAZE_NEW UserInfo(NULL, (*start)->getExternalId(), (*start)->getConnectionGroupId(), (*start)->getRoleName());
        gameInfo->getPlayerIdListMap().insert(eastl::pair<PlayerId, UserInfo*>((*start)->getPlayerId(), info) );
    }
    if (!mIsDedicatedServer)
    {
        mMutex.Lock();
    }
    mGameDataMap.insert(eastl::pair<GameId, StressGameInfo*>(gameSetup->getGameData().getGameId(), gameInfo));
    if (!mIsDedicatedServer)
    {
        mMutex.Unlock();
    }
    return gameInfo;
}
StressGameInfo* StressGameData::getGameData(GameId gameId)
{
    GameInfoMap::iterator itr = findGameIdInGameDataMap(gameId);
    if (itr != mGameDataMap.end())
    {
        return itr->second;
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressGameData::getGameData][Game Id =" << gameId << "] not present");
        return NULL;
    }
}

bool StressGameData::removeGameData(GameId gameId)
{
    GameInfoMap::iterator itr = findGameIdInGameDataMap(gameId);
    if (itr != mGameDataMap.end())
    {
        if (!mIsDedicatedServer)
        {
            mMutex.Lock();
        }
        PlayerIdListMap& playerMap = itr->second->getPlayerIdListMap();
        PlayerIdListMap::iterator start = playerMap.begin();
        PlayerIdListMap::iterator end   = playerMap.end();
        for (; start != end; start++)
        {
            //  Delete player data
            delete start->second;
            //  playerMap.erase(start);  //  this is a vector basically it may reallocate on erase and position can change at runtime
        }
        playerMap.clear();  //  clear all players data
        //  Delete Game Data
		if (itr->second != NULL)
		{
			delete itr->second;
		}
        mGameDataMap.erase(itr);
        if (!mIsDedicatedServer)
        {
            mMutex.Unlock();
        }
        return true;
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressGameData::removeGameData][Game Id =" << gameId << "] not present");
        return false;
    }
}
bool StressGameData::addPlayerToGameInfo(NotifyPlayerJoining* joiningPlayer)
{
    UserInfo PlayerInfo(NULL, joiningPlayer->getJoiningPlayer().getExternalId(), joiningPlayer->getJoiningPlayer().getConnectionGroupId(), joiningPlayer->getJoiningPlayer().getRoleName(),
                        joiningPlayer->getJoiningPlayer().getSlotType(), joiningPlayer->getJoiningPlayer().getTeamIndex());
    return addPlayerToGameInfo(joiningPlayer->getJoiningPlayer().getPlayerId(), &PlayerInfo, joiningPlayer->getGameId());
}

bool StressGameData::addPlayerToGameInfo(PlayerId playerId, UserInfo* playerInfo, GameId gameId)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        //  remove player if already exist,
        removePlayerFromGameData(playerId, gameId);
        //  Add player to playerlist
        UserInfo* info = BLAZE_NEW UserInfo(NULL, playerInfo->extId, playerInfo->connGroupId, playerInfo->roleName, playerInfo->userRoleType);
        it->second->getPlayerIdListMap().insert(eastl::pair<PlayerId, UserInfo*>(playerId, info) );
        return true;
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressGameData::AddPlayerToGameInfo][Game Id =" << gameId << "] not present");
        return false;
    }
}

bool StressGameData::addQueuedPlayerToGameInfo(NotifyPlayerJoining* joiningPlayer)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(joiningPlayer->getGameId());
    if (it != mGameDataMap.end())
    {
        //  remove player if already exist,
        removeQueuedPlayerFromGameInfo(joiningPlayer->getJoiningPlayer().getPlayerId(), joiningPlayer->getGameId());
        //  Add player to playerlist
        UserInfo* info = BLAZE_NEW UserInfo(NULL, joiningPlayer->getJoiningPlayer().getExternalId(), joiningPlayer->getJoiningPlayer().getConnectionGroupId(), joiningPlayer->getJoiningPlayer().getRoleName());
        it->second->getQueuedPlayerList().insert(eastl::pair<PlayerId, UserInfo*>(joiningPlayer->getJoiningPlayer().getPlayerId(), info) );
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressGameData::addQueuedPlayerToGameInfo][Game Id =" << joiningPlayer->getGameId() << "] playerId" << joiningPlayer->getJoiningPlayer().getPlayerId());
        return true;
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[StressGameData::addQueuedPlayerToGameInfo][Game Id =" << joiningPlayer->getGameId() << "] not present");
        return false;
    }
}

bool StressGameData::addInvitedPlayer(PlayerId playerId, GameId gameId)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {   //  Add this player to invited playerlist
        it->second->getInvitePlayerIdList().push_back(playerId);
        return true;
    }
    return false;
}

bool StressGameData::removeQueuedPlayerFromGameInfo(PlayerId playerId, GameId gameId)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        PlayerIdListMap& playerMap = it->second->getQueuedPlayerList();
        PlayerIdListMap::iterator playerData = playerMap.find(playerId);
        if ( playerData != playerMap.end() )
        {
            //  delete player data
            delete playerData->second;
            //  remove player from the list
            playerMap.erase(playerData);
            return true;
        }
        else
        {
             BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressGameData::removeQueuedPlayerFromGameInfo][PlayerId =" << playerId << "] not present");
        }
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[StressGameData::removeQueuedPlayerFromGameInfo][Game Id =" << gameId << "] not present");
    }
    return false;
}

bool StressGameData::removePlayerFromGameData(PlayerId playerId, GameId gameId)
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressGameData::removePlayerFromGameData][PlayerId =" << playerId << "] Removing player data");
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        PlayerIdListMap& playerMap = it->second->getPlayerIdListMap();
        PlayerIdListMap::iterator playerData = playerMap.find(playerId);
        if ( playerData != playerMap.end() )
        {
            //  delete player data
            delete playerData->second;
            //  remove player from the list
            playerMap.erase(playerData);
            return true;
        }
        else
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressGameData::removePlayerFromGameData][PlayerId =" << playerId << "] not present");
        }
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressGameData::removePlayerFromGameData][Game Id =" << gameId << "] not present");
    }
    return false;
}
bool StressGameData::removeInvitedPlayer(PlayerId playerId, GameId gameId)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        //  remove this player from inviteId list if present
        PlayerIdList::iterator start = it->second->getInvitePlayerIdList().begin();
        PlayerIdList::iterator end   = it->second->getInvitePlayerIdList().end();
        for ( ; start != end; ++start)
        {
            if ( ((PlayerId)*start) == playerId ) {
                break;
            }
        }
        if (start != end)
        {
            it->second->getInvitePlayerIdList().erase(start);
            return true;
        }
        else
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressGameData::RemoveInvitedPlayer][PlayerId =" << playerId << "] not present");
        }
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressGameData::RemoveInvitedPlayer][Game Id =" << gameId << "] not present");
    }
    return false;
}

PlayerIdListMap* StressGameData::getPlayerMap(GameId gameId)
{
    PlayerIdListMap  *playerMap = NULL;
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        playerMap = &(it->second->getPlayerIdListMap());
    }
    return playerMap;
}

bool StressGameData::isPlayerExistInGame(PlayerId playerId, GameId gameId)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        PlayerIdListMap& playerMap = it->second->getPlayerIdListMap();
        PlayerIdListMap::iterator playerData = playerMap.find(playerId);
        if (playerData != playerMap.end()) {
            return true;
        }
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressGameData::isPlayerExistInGame][Game Id =" << gameId << "] not present");
    }
    return false;
}

bool StressGameData::isPublicSpectatorPlayer(PlayerId playerId, GameId gameId)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        PlayerIdListMap& playerMap = it->second->getPlayerIdListMap();
        PlayerIdListMap::iterator playerData = playerMap.find(playerId);
        if (playerData != playerMap.end())
        {
            if (playerData->second)
            {
                if (SLOT_PUBLIC_SPECTATOR == playerData->second->userRoleType) {
                    return true;
                }
            }
        }
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressGameData::isPublicSpectatorPlayer][Game Id =" << gameId << "] not present");
    }
    return false;
}


bool StressGameData::isInvitedPlayerExist(PlayerId playerId, GameId gameId)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        //  remove this player from inviteId list if present
        PlayerIdList::iterator start = it->second->getInvitePlayerIdList().begin();
        PlayerIdList::iterator end   = it->second->getInvitePlayerIdList().end();
        for ( ; start != end; ++start)
        {
            if ( ((PlayerId)*start) == playerId ) {
                return true;
            }
        }
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[StressGameData::IsInvitedPlayerExist][Game Id =" << gameId << "] not present");
    }
    return false;
}

bool StressGameData::isEmpty()
{
    return mGameDataMap.empty();
    //  GameInfoMap::iterator start = mGameDataMap.begin();
    //  GameInfoMap::iterator end   = mGameDataMap.end();
    //  return (start == end);
}

PlayerIdList& StressGameData::getInvitePlayerIdListByGame(GameId gameId)
{
    static PlayerIdList staticPlayerList;  //  this is just dummy static list representing a invalid list having size = 0
    staticPlayerList.clear();
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        return it->second->getInvitePlayerIdList();
    }
    return staticPlayerList;
}

GameId StressGameData::getRandomGameId(uint32_t maxGameSize)
{
    GameId randGameId = 0;
    if (!isEmpty())
    {
        if (maxGameSize)
        {
            int32_t gameIndex = 0;
            if (mGameDataMap.size() >= 2) {
                gameIndex = Random::getRandomNumber(static_cast<int32_t>(mGameDataMap.size()) / 2);    //   to make sure that half of the game list is always available to explore
            }
            GameInfoMap::const_iterator citEnd = mGameDataMap.end();
            GameInfoMap::const_iterator cit = &mGameDataMap.at(gameIndex);
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[getRandomGameId]: Game index chosen: %d" << gameIndex);
            for (; cit != citEnd; ++cit)
            {
                if ((cit->second->getPlayerIdListMap().size() + getInvitePlayerIdListByGame(cit->first).size()) < maxGameSize)
                {
                    //  return it->first; //  return first encountered game with sufficient slots to join
                    randGameId = cit->first;
                    return randGameId;
                }
            }
            if (randGameId == 0)
            {
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[getRandomGameId]: could not find a random game with slots");
                return randGameId;
            }
        }
        else
        {
            //   randomly pick a Game ...
            int32_t r = Random::getRandomNumber(static_cast<int32_t>(mGameDataMap.size()));
            return mGameDataMap.at(r).first;
        }
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressGameData::getRandomGameId] mGameDataMap is empty.");
    }
    return randGameId;
}

ConnectionGroupId StressGameData::getPlayerConnectionGroupId(PlayerId playerId, GameId gameId)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        PlayerIdListMap& playerMap = it->second->getPlayerIdListMap();
        PlayerIdListMap::iterator playerData = playerMap.find(playerId);
        if (playerData != playerMap.end()) {
            return playerData->second->connGroupId;
        }
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressGameData::RemovePlayerConnectionGroupFromGameInfo][Game Id =" << gameId << "] not present");
    }
    return 0;
}

ExternalId StressGameData::getPlayerExternalId(PlayerId playerId, GameId gameId)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        PlayerIdListMap& playerMap = it->second->getPlayerIdListMap();
        PlayerIdListMap::iterator playerData = playerMap.find(playerId);
        if (playerData != playerMap.end()) {
            return playerData->second->extId;
        }
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressGameData::RemovePlayerConnectionGroupFromGameInfo][Game Id =" << gameId << "] not present");
    }
    return 0;
}

eastl::string StressGameData::getPlayerPersonaName(PlayerId playerId, GameId gameId)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        PlayerIdListMap& playerMap = it->second->getPlayerIdListMap();
        PlayerIdListMap::iterator playerData = playerMap.find(playerId);
        if (playerData != playerMap.end()) {
            return playerData->second->personaName;
        }
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressGameData::getPlayerPersonaName][Game Id =" << gameId << "] not present");
    }
    return "";
}

UserInfo* StressGameData::getPlayerData(PlayerId playerId, GameId gameId)
{
	UserInfo* userData = NULL;

	GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
	if(it != mGameDataMap.end())
	{	
		PlayerIdListMap& playerMap = it->second->getPlayerIdListMap();

		PlayerIdListMap::iterator playerData = playerMap.find(playerId);
		if(playerData != playerMap.end()) { userData =  playerData->second; }
	}
	else
	{
		BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressGameData::getPlayerConsumableList][Game Id =" <<gameId<< "] not present");		
	}	
	return userData;
}

GameState StressGameData::getGameState(GameId gameId)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        return it->second->getGameState();
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressGameData::getGameState][Game Id =" << gameId << "] not present");
    }
    return NEW_STATE;
}

void StressGameData::setGameState(GameId gameId, GameState state)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        return it->second->setGameState(state);
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressGameData::setGameState][Game Id =" << gameId << "] not present");
    }
}

uint16_t StressGameData::getPlayerCount(GameId gameId)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        return it->second->getPlayerCount();
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressGameData::getPlayerCount][Game Id =" << gameId << "] not present");
    }
    return 0;
}

void StressGameData::setPlayerCount(GameId gameId, uint16_t count)
{
    GameInfoMap::iterator it = findGameIdInGameDataMap(gameId);
    if (it != mGameDataMap.end())
    {
        return it->second->setPlayerCount(count);
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StressGameData::setPlayerCount][Game Id =" << gameId << "] not present");
    }
}


void StressGameInfo::setGameAttributes(const Collections::AttributeMap& GameAttributes)
{
    Collections::AttributeMap::const_iterator citEnd = GameAttributes.end();
    for (Collections::AttributeMap::const_iterator cit = GameAttributes.begin(); cit != citEnd; ++cit)
    {
        Collections::AttributeMap::iterator gameAttrIt = mGameAttributeMap.find(cit->first);
        if (gameAttrIt != getGameAttributes().end())
        {
            //  udate attribute with the new value
            gameAttrIt->second = cit->second;
        }
        else
        {
            //    add new one if not found
            eastl::pair<Collections::AttributeMap::iterator, bool> inserter = mGameAttributeMap.insert(cit->first);
            inserter.first->second = cit->second;
        }
    }
}

PlayerId StressGameInfo::getRandomPlayer()
{
    PlayerId playerId = 0;
    if (playerListMap.size() > 0)
    {
        playerId = playerListMap.at(Blaze::Random::getRandomNumber(playerListMap.size())).first;
    }
    return playerId;
}

StressGameData& DedicatedGameDataRef(StressPlayerInfo* ptrPlayerInfo)
{
    return ((Blaze::Stress::PlayerModule*)ptrPlayerInfo->getOwner())->mDedicatedGamesData;
}

StressGameData& OnlineSeasonsGameDataRef(StressPlayerInfo* ptrPlayerInfo)
{
    return ((Blaze::Stress::PlayerModule*)ptrPlayerInfo->getOwner())->mOnlineSeasonsGamesData;
}

StressGameData& FriendliesGameDataRef(StressPlayerInfo* ptrPlayerInfo)
{
    return ((Blaze::Stress::PlayerModule*)ptrPlayerInfo->getOwner())->mFriendliesGamesData;
}

StressGameData& LiveCompetitionsGameDataRef(StressPlayerInfo* ptrPlayerInfo)
{
    return ((Blaze::Stress::PlayerModule*)ptrPlayerInfo->getOwner())->mLiveCompetitionsGamesData;
}

StressGameData& DedicatedServerGameDataRef(StressPlayerInfo* ptrPlayerInfo)
{
    return ((Blaze::Stress::PlayerModule*)ptrPlayerInfo->getOwner())->mDedicatedServerGamesData;
}

StressGameData& ESLServerGameDataRef(StressPlayerInfo* ptrPlayerInfo)
{
	return ((Blaze::Stress::PlayerModule*)ptrPlayerInfo->getOwner())->mESLServerGamesData;
}

StressGameData& CoopplayerGameDataRef(StressPlayerInfo* ptrPlayerInfo)
{
    return ((Blaze::Stress::PlayerModule*)ptrPlayerInfo->getOwner())->mCoopPlayerGamesData;
}

StressGameData& FutPlayerGameDataRef(StressPlayerInfo* ptrPlayerInfo)
{
    return ((Blaze::Stress::PlayerModule*)ptrPlayerInfo->getOwner())->mFutPlayerGamesData;
}

StressGameData& ClubsPlayerGameDataRef(StressPlayerInfo* ptrPlayerInfo)
{
    return ((Blaze::Stress::PlayerModule*)ptrPlayerInfo->getOwner())->mClubsPlayerGamesData;
}
StressGameData& DropinPlayerGameDataRef(StressPlayerInfo* ptrPlayerInfo)
{
	return ((Blaze::Stress::PlayerModule*)ptrPlayerInfo->getOwner())->mDropinPlayerGamesData;
}
StressGameData& SsfSeasonsGameDataRef(StressPlayerInfo* ptrPlayerInfo)
{
	return ((Blaze::Stress::PlayerModule*)ptrPlayerInfo->getOwner())->mSsfSeasonsGamesData;
}
void PartyManager::resetOpenParty()
{
    mOpenPartyPersistedId = 0;
    mMaxOpenPartyMembers = 0;
    mOpenPartyMembers.clear();
}

int64_t PartyManager::createOrJoinAParty(BlazeId playerId, uint32_t memberCount)
{
    int64_t persistedId = 0;
    if (!mOpenPartyPersistedId)
    {
        // there are no open parties available. this player has to be host and create one.
        mOpenPartyPersistedId = playerId;
        mMaxOpenPartyMembers = memberCount;
        mOpenPartyMembers.clear();
        mOpenPartyMembers.push_back(playerId);
        persistedId = mOpenPartyPersistedId;
    }
    else
    {
        persistedId = mOpenPartyPersistedId;
        mOpenPartyMembers.push_back(playerId);
        BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[SpartaHttpResult::createOrJoinAParty]: player "<< playerId << " is joiner no. "<< mOpenPartyMembers.size() << " max "<< mMaxOpenPartyMembers << " for party " << mOpenPartyPersistedId);
        if ((mOpenPartyMembers.size() >= mMaxOpenPartyMembers))
        {
            // this party is ready. Move it to readyParties list. reset the open party.
            mReadyParties.insert(eastl::pair<PlayerId, eastl::vector<int64_t>>(mOpenPartyPersistedId, mOpenPartyMembers));
            resetOpenParty();
        }
    }
    return persistedId;
}
bool PartyManager::isPartyReady(BlazeId playerId)
{
    if (playerId == mOpenPartyPersistedId)
    {
        return false;
    }
    else
    {
        auto itr = mReadyParties.find(playerId);
        if (itr != mReadyParties.end())
        {
            return true;
        }
        else
        {
            BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[SpartaHttpResult::isPartyReady]: player "<< playerId << " party is not in ready parties list");
            return false;
        }
    }
}
void PartyManager::partyIsOver(BlazeId playerId)
{
    if (playerId == mOpenPartyPersistedId)
    {
        resetOpenParty();
    }
    else
    {
        auto itr = mReadyParties.find(playerId);
        if (itr != mReadyParties.end())
        {
            itr->second.clear();
            mReadyParties.erase(itr);
        }
        else
        {
            BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[SpartaHttpResult::partyIsOver]: player "<< playerId << " party is not in ready parties list");
        }
    }
}
eastl::vector<int64_t> PartyManager::getPartyMembers(BlazeId playerId)
{
    if (playerId == mOpenPartyPersistedId) 
    {
        return mOpenPartyMembers;
    }
    else
    {
        auto itr = mReadyParties.find(playerId);
        if (itr != mReadyParties.end())
        {
            return itr->second;
        }
        else
        {
            BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[SpartaHttpResult::getPartyMembers]: player "<< playerId << " party is not in ready parties list");
            eastl::vector<int64_t> partymembers;
            partymembers.push_back(playerId);
            return partymembers;
        }

    }
}

bool IsPlayerPresentInList(PlayerModule* module, PlayerId  id, PlayerType type)
{
    BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[IsPlayerPresentInList] for type:" << type << " and player=" << id);
    //  PlayerIdList* playerList = NULL;
    switch (type)
    {
        case ONLINESEASONS:
            {
                PlayerMap *playerMap = &(module->onlineSeasonsMap);
                PlayerMap::iterator start = playerMap->begin();
                PlayerMap::iterator end   = playerMap->end();
                for (; start != end; start++)
                {
                    if (id == (*start).first) {
                        return true;
                    }
                }
            }
            break;
		case COOPPLAYER:
            {
                PlayerIdList *playerList = &(module->coopPlayerList);
                PlayerIdList::const_iterator start = playerList->begin();
                PlayerIdList::const_iterator end   = playerList->end();
                for (; start != end; start++)
                {
                    if (id == (*start)) {
                        return true;
                    }
                }
            }
            break;
		case FRIENDLIES:
            {
                PlayerMap *playerMap = &(module->friendliesMap);
                PlayerMap::iterator start = playerMap->begin();
                PlayerMap::iterator end   = playerMap->end();
                for (; start != end; start++)
                {
                    if (id == (*start).first) {
                        return true;
                    }
                }
            }
            break;
		case LIVECOMPETITIONS:
            {
                PlayerMap *playerMap = &(module->liveCompetitionsMap);
                PlayerMap::iterator start = playerMap->begin();
                PlayerMap::iterator end   = playerMap->end();
                for (; start != end; start++)
                {
                    if (id == (*start).first) {
                        return true;
                    }
                }
            }
            break;
		case FUTPLAYER:
            {
                PlayerMap *playerMap = &(module->futPlayerMap);
                PlayerMap::iterator start = playerMap->begin();
                PlayerMap::iterator end   = playerMap->end();
                for (; start != end; start++)
                {
                    if (id == (*start).first) {
                        return true;
                    }
                }
            }
            break;
		case CLUBSPLAYER:
            {
                PlayerMap *playerMap = &(module->clubsPlayerMap);
                PlayerMap::iterator start = playerMap->begin();
                PlayerMap::iterator end   = playerMap->end();
                for (; start != end; start++)
                {
                    if (id == (*start).first) {
                        return true;
                    }
                }
            }
            break;
		case DROPINPLAYER:
		{
			PlayerMap *playerMap = &(module->dropinPlayerMap);
			PlayerMap::iterator start = playerMap->begin();
			PlayerMap::iterator end = playerMap->end();
			for (; start != end; start++)
			{
				if (id == (*start).first) {
					return true;
				}
			}
		}
		break;
		case SSFSEASONS:
		{
			PlayerMap *playerMap = &(module->ssfSeasonsMap);
			PlayerMap::iterator start = playerMap->begin();
			PlayerMap::iterator end = playerMap->end();
			for (; start != end; start++)
			{
				if (id == (*start).first) {
					return true;
				}
			}
		}
		break;
        default:
            {
                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[IsPlayerPresentInList][Invalid PlayerType:" << type);
                return false;
            }
    }
    return false;
}

eastl::string IteratePlayers(PlayerIdListMap& playerList)
{
    eastl::string strPlayers("");
    strPlayers.append("Players [");
    PlayerIdListMap::iterator itr = playerList.begin();
    while (itr != playerList.end())
    {
        PlayerId player = itr->first;
        strPlayers.append_sprintf("%" PRId64 ", ", player);
        ++itr;
    }
    strPlayers.append("]\n");
    return strPlayers;
}

eastl::string IteratePlayers(PlayerIdList& playerList)
{
    eastl::string strPlayers("");
    strPlayers.append("Players [");
    PlayerIdList::iterator itr = playerList.begin();
    while (itr != playerList.end())
    {
        PlayerId player = *itr;
        strPlayers.append_sprintf("%" PRId64 ", ", player);
        ++itr;
    }
    strPlayers.append("]\n");
    return strPlayers;
}

float getRandomFloatVal(float fMin, float fMax)
{
    return ((fMax - fMin) * ((float)rand() / RAND_MAX)) + fMin;
}

bool RollDice(const int32_t& threshold)
{
    bool yo = false;
    //  Random::initializeSeed();
    //   Roll the dice.
    int32_t roll = Random::getRandomNumber(100);
    if (roll < threshold)
    {
        yo = true;
    }
    return yo;
}

void delay(uint32_t milliseconds)
{
    BlazeRpcError err = gSelector->sleep(milliseconds * 1000);
    if (err != ERR_OK)
    {
        BLAZE_WARN(Log::SYSTEM, "[RPCDelay]: failed to sleep.");
    }
}

void delayRandomTime(uint32_t minMs, uint32_t maxMs)
{
    int32_t diffTimeSec = (maxMs - minMs) / 1000;
    int32_t sleepTime = minMs + Blaze::Random::getRandomNumber(diffTimeSec) * 1000 + 1000;
    delay(sleepTime);
}

template <typename String>size_t tokenize(const String& s, const typename String::value_type* pDelimiters, String* resultArray, size_t resultArraySize)
{
    size_t n = 0;
    typename String::size_type lastPos = s.find_first_not_of(pDelimiters, 0);
    typename String::size_type pos     = s.find_first_of(pDelimiters, lastPos);
    while ((n < resultArraySize) && ((pos != String::npos) || (lastPos != String::npos)))
    {
        resultArray[n++].assign(s, lastPos, pos - lastPos);
        lastPos = s.find_first_not_of(pDelimiters, pos);
        pos     = s.find_first_of(pDelimiters, lastPos);
    }
    return n;
}

uint8_t ClubsPlayerGamePlayconfig::getRandomTeamSize() const
{
	uint32_t rand = static_cast<uint32_t>(Blaze::Random::getRandomNumber(100)) + 1;
	uint8_t sumProbalities = 0;

	for( ClubSizeProbability::const_iterator itr = CLUBS_CONFIG.ClubSizeDistribution.begin(); itr != CLUBS_CONFIG.ClubSizeDistribution.end(); ++itr )
	{
		sumProbalities += itr->second;
		if (rand <= sumProbalities)
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayerGamePlayconfig::getRandomTeamSize " << this << " rand = " << rand << " TeamSize chosen = " << itr->first);
			return itr->first;
		}
	}
	return 0;
}

uint8_t DropinPlayerGamePlayconfig::getRandomTeamSize() const
{
	uint32_t rand = static_cast<uint32_t>(Blaze::Random::getRandomNumber(100)) + 1;
	uint8_t sumProbalities = 0;

	for (ClubSizeProbability::const_iterator itr = DROPIN_CONFIG.ClubSizeDistribution.begin(); itr != DROPIN_CONFIG.ClubSizeDistribution.end(); ++itr)
	{
		sumProbalities += itr->second;
		if (rand <= sumProbalities)
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[ClubsPlayerGamePlayconfig::getRandomTeamSize " << this << " rand = " << rand << " TeamSize chosen = " << itr->first);
			return itr->first;
		}
	}
	return 0;
}
}  // namespace Stress
}  // namespace Blaze


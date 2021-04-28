/*************************************************************************************************/
/*!
    \file Clubsmodule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "clubsmodule.h"
#include "loginmanager.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/connection/selector.h"
#include "framework/config/config_file.h"
#include "framework/config/config_boot_util.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/util/random.h"
#ifdef STATIC_MYSQL_DRIVER
#include "framework/database/mysql/blazemysql.h"
#endif
#include "framework/tdf/usersessiontypes_server.h"
#include "gamemanager/tdf/gamemanager.h"
#include "stats/tdf/stats_server.h"
#include "framework/config/configdecoder.h"

#define WIPEOUT_TABLE(table)  { BLAZE_INFO_LOG(BlazeRpcLog::clubs, "Deleting all entries from " table " and resetting auto increment."); \
    int error = mysql_query(&mysql, "TRUNCATE " table "; ALTER TABLE " table " AUTO_INCREMENT=1;"); \
    if (error) \
    { \
        BLAZE_WARN_LOG(BlazeRpcLog::clubs, "Error: " << mysql_error(&mysql)); \
        mysql_close(&mysql); \
        mysql_library_end(); \
        return false; \
    } \
    resetCommand(&mysql); }

#define GET_NOT_ZERO(paramName, paramIdent) { \
    paramName = config.getUInt32(paramIdent, 0); \
    if (paramName == 0) \
    { \
        BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Missing or illegal " paramIdent " parameter in club module configuration."); \
        return false;  \
    } }

#define MYSQL_EXEC(cmd)  { BLAZE_INFO_LOG(BlazeRpcLog::clubs, cmd); \
    int error = mysql_query(&mysql, cmd); \
    if (error) \
    { \
    BLAZE_WARN_LOG(BlazeRpcLog::clubs, "Error: " << mysql_error(&mysql)); \
    mysql_close(&mysql); \
    mysql_library_end(); \
    return false; \
    } \
    resetCommand(&mysql); }

#define START_BLAZE_SEQUENCE do {
#define END_BLAZE_SEQUENCE } while(true);
#define TEST_BLAZE(exp) if ((exp) != Blaze::ERR_OK) break;

using namespace Blaze::Clubs;
using namespace Blaze::GameManager;

namespace Blaze
{
namespace Stress
{

/*
* List of available actions that can be performed by the module.
*/

static const char8_t* ACTION_STR_GET_CLUBS = "getClubs";
static const char8_t* ACTION_STR_GET_NEWS = "getNews";
static const char8_t* ACTION_STR_GET_AWARDS = "getAwards";
static const char8_t* ACTION_STR_SIMULATE_PROD = "simulateProduction";
static const char8_t* ACTION_STR_GET_CLUBRECORDBOOK = "getClubRecordbook";
static const char8_t* ACTION_STR_CREATE_CLUB = "createClub";
static const char8_t* ACTION_STR_FIND_CLUBS = "findClubs";
static const char8_t* ACTION_STR_FIND_CLUBS_BY_TAGS_ALL = "findClubsByTagsAll";
static const char8_t* ACTION_STR_FIND_CLUBS_BY_TAGS_ANY = "findClubsByTagsAny";
static const char8_t* ACTION_STR_FIND_CLUBS2 = "findClubs2";
static const char8_t* ACTION_STR_FIND_CLUBS2_BY_TAGS_ALL = "findClubs2ByTagsAll";
static const char8_t* ACTION_STR_FIND_CLUBS2_BY_TAGS_ANY = "findClubs2ByTagsAny";
static const char8_t* ACTION_STR_PLAY_CLUB_GAME = "playClubGame";
static const char8_t* ACTION_STR_JOIN_CLUB = "joinClub";
static const char8_t* ACTION_STR_JOIN_OR_PETITION_CLUB = "joinOrPetitionClub";
static const char8_t* ACTION_STR_GET_MEMBERS = "getMembers";
static const char8_t* ACTION_STR_GET_CLUBS_COMPONENT_SETTINGS = "getClubsComponentSettings";
static const char8_t* ACTION_STR_GET_CLUB_MEMBERSHIP_FOR_USERS = "getClubMembershipForUsers";
static const char8_t* ACTION_STR_POST_NEWS = "postNews";
static const char8_t* ACTION_STR_PETITION = "petition";
static const char8_t* ACTION_STR_UPDATE_ONLINE_STATUS = "updateOnlineStatus";
static const char8_t* ACTION_STR_UPDATE_MEMBER_METADATA = "updateMemberMetadata";
static const char8_t* ACTION_STR_SET_METADATA = "setMetadata";
static const char8_t* ACTION_STR_UPDATE_CLUB_SETTINGS = "updateClubSettings";

static const char8_t* CONTEXT_STR_GET_CLUBS = "ClubsModule::getClubs";
static const char8_t* CONTEXT_STR_GET_NEWS = "ClubsModule::getNews";
static const char8_t* CONTEXT_STR_GET_AWARDS = "ClubsModule::getAwards";
static const char8_t* CONTEXT_STR_SIMULATE_PROD = "ClubsModule::simulateProduction";
static const char8_t* CONTEXT_STR_GET_CLUBRECORDBOOK = "ClubsModule::getClubRecordbook";
static const char8_t* CONTEXT_STR_CREATE_CLUB = "ClubsModule::createClub";
static const char8_t* CONTEXT_STR_FIND_CLUBS = "ClubsModule::findClubs";
static const char8_t* CONTEXT_STR_FIND_CLUBS_BY_TAGS_ALL = "ClubsModule::findClubsByTagsAll";
static const char8_t* CONTEXT_STR_FIND_CLUBS_BY_TAGS_ANY = "ClubsModule::findClubsByTagsAny";
static const char8_t* CONTEXT_STR_FIND_CLUBS2 = "ClubsModule::findClubs2";
static const char8_t* CONTEXT_STR_FIND_CLUBS2_BY_TAGS_ALL = "ClubsModule::findClubs2ByTagsAll";
static const char8_t* CONTEXT_STR_FIND_CLUBS2_BY_TAGS_ANY = "ClubsModule::findClubs2ByTagsAny";
static const char8_t* CONTEXT_STR_PLAY_CLUB_GAME = "ClubsModule::playClubGame";
static const char8_t* CONTEXT_STR_JOIN_CLUB = "ClubsModule::joinClub";
static const char8_t* CONTEXT_STR_JOIN_OR_PETITION_CLUB = "ClubsModule::joinOrPetitionClub";
static const char8_t* CONTEXT_STR_GET_MEMBERS = "ClubsModule::getMembers";
static const char8_t* CONTEXT_STR_GET_CLUBS_COMPONENT_SETTINGS = "ClubsModule::getClubsComponentSettings";
static const char8_t* CONTEXT_STR_GET_CLUB_MEMBERSHIP_FOR_USERS = "ClubsModule::getClubMembershipForUsers";
static const char8_t* CONTEXT_STR_POST_NEWS = "ClubsModule::postNews";
static const char8_t* CONTEXT_STR_PETITION = "ClubsModule::petition";
static const char8_t* CONTEXT_STR_UPDATE_ONLINE_STATUS = "ClubsModule::updateOnlineStatus";
static const char8_t* CONTEXT_STR_UPDATE_MEMBER_METADATA = "ClubsModule::updateMemberMetadata";
static const char8_t* CONTEXT_STR_SET_METADATA = "ClubsModule::setMetadata";
static const char8_t* CONTEXT_STR_UPDATE_CLUB_SETTINGS = "ClubsModule::updateClubSettings";


uint32_t ClubsInstance::mInstanceOrdStatic = 0;

/*** Public Methods ******************************************************************************/

// static
StressModule* ClubsModule::create()
{
    return BLAZE_NEW ClubsModule();
}

ClubsModule::~ClubsModule()
{
    if (mStatsConfig != nullptr)
        delete mStatsConfig;

    mClubDomainMap.clear();

    for (DomainMemberCounts::iterator i = mDomainMemberCounts.begin(), e = mDomainMemberCounts.end();
        i != e;
        ++i)
    {
        delete i->second;
    }
    mDomainMemberCounts.clear();
}

bool ClubsModule::parseServerConfig(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    const ConfigMap *statsConfigFile = nullptr;
    const ConfigMap *clubsConfigFile = nullptr;

    if (bootUtil != nullptr)
    {
        statsConfigFile = bootUtil->createFrom("stats", ConfigBootUtil::CONFIG_IS_COMPONENT);
        clubsConfigFile = bootUtil->createFrom("clubs", ConfigBootUtil::CONFIG_IS_COMPONENT);
    }
    else
    {
        const ConfigMap* configFilePathMap = config.getMap("configFilePath");

        const char8_t* statsConfigFilePath = "component/stats/stats.cfg";
        const char8_t* clubsConfigFilePath = "component/clubs/clubs.cfg";

        if (configFilePathMap != nullptr)
        {
            statsConfigFilePath = configFilePathMap->getString("stats", "component/stats/stats.cfg");
            clubsConfigFilePath = configFilePathMap->getString("clubs", "component/clubs/clubs.cfg");
        }

        statsConfigFile = ConfigFile::createFromFile(statsConfigFilePath, true);
        clubsConfigFile = ConfigFile::createFromFile(clubsConfigFilePath, true);
    }


    if (statsConfigFile == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs,"[ClubsModule]: Failed to load stats configuration file");
        return false;
    }
    const ConfigMap *statsConfigMap = statsConfigFile->getMap("stats");
    if (statsConfigMap == nullptr) 
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs,"[ClubsModule] : Failed to parse stats.");
        return false;
    }

    ConfigDecoder configDecoder(*statsConfigMap);
    configDecoder.decode(&mStatsConfigTdf);

    mStatsConfig = BLAZE_NEW Stats::StatsConfigData();
    mStatsConfig->setStatsConfig(mStatsConfigTdf);
    if (!mStatsConfig->parseKeyScopes(mStatsConfigTdf))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs,"[ClubsModule] : Failed to parse stats keyscopes");
        return false;
    }
     
    if (clubsConfigFile == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs,"[ClubsModule]: Failed to load clubs configuration file");
        return false;
    }
    const ConfigMap *clubsConfigMap = clubsConfigFile->getMap("clubs");
    if (clubsConfigMap == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs,"[ClubsModule]: Failed to parse clubs.");
        return false;
    }

    const ConfigSequence *domainSequence = clubsConfigMap->getSequence("Domains");
    if (domainSequence == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs,"[ClubsModule]: Failed to find club domains");
        return false;
    }
     while (domainSequence->hasNext())
     {
         const ConfigMap *domainMap = domainSequence->nextMap();
         ClubDomain *domain = BLAZE_NEW ClubDomain;
         domain->setClubDomainId(domainMap->getUInt32("ClubDomainId", 0));
         domain->setDomainName(domainMap->getString("DomainName", ""));
         domain->setMaxMembersPerClub(domainMap->getUInt32("MaxMembersPerClub", 0));
         domain->setMaxGMsPerClub(domainMap->getUInt16("MaxGMsPerClub", 0));
         domain->setMaxMembershipsPerUser(domainMap->getUInt16("MaxMembershipsPerUser", 0));
         domain->setMaxInvitationsPerUserOrClub(domainMap->getUInt16("MaxInvitationsPerUserOrClub", 0));
         domain->setMaxInactiveDaysPerClub(domainMap->getUInt16("MaxInactiveDaysPerClub", 0));
         domain->setMaxNewsItemsPerClub(domainMap->getUInt16("MaxNewsItemsPerClub", 0));
         domain->setTrackMembershipInUED(domainMap->getBool("TrackMembershipInUED", false));
         mClubDomainMap[domain->getClubDomainId()] = domain;
         mDomainIdVector.push_back(domain->getClubDomainId());
     }
     return true;
}

bool ClubsModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if (!StressModule::initialize(config, bootUtil))
    {
        return false;
    }

    if (!parseServerConfig(config, bootUtil))
        return false;

    // blaze id to start at
    mMinAccId = config.getUInt64("minAccId", 1);

    // pickup database parameters
    mDbHost = config.getString("dbhost", "");
    mDbPort = config.getUInt32("dbport", 0);
    mDatabase = config.getString("database", "");
    mDbUser = config.getString("dbuser", "");
    mDbPass = config.getString("dbpass", "");
    mDbStatsTable = config.getString("dbstatstable", "");
    mDbStatsEntityOffset = config.getUInt32("dbstatsentityoffset", 0);

    // club creation parameters
    mPtrnPersonaName = config.getString("persona-format", "");
    mPtrnEmailFormat = config.getString("email-format", "");
    mRecreateDatabase = config.getBool("recreateDatabase", false);
    mStatementBatchSize = config.getUInt32("statementBatchSize", 50);
    
    
    mPtrnName = config.getString("clubname", "");
    mPtrnNonUniqueName = config.getString("nonuniquename", "");
    mPtrnDesc = config.getString("desc", "");
    mPtrnNewsBody = config.getString("newsBody", "");
    mPassword = config.getString("password", "");
    mPasswordPercent = config.getUInt32("passwordPercent", 0);
    
    mTotalClubs = 0;
    mTotalClubMembers = 0;

    const Blaze::ConfigSequence* domainPercentages = config.getSequence("clubDomainPercentages");
    if (domainPercentages == nullptr || !domainPercentages->hasNext())
    {
        eastl::string msg;
        config.toString(msg);
        BLAZE_ERR_LOG(BlazeRpcLog::clubs, "clubDomainPercentages is required. config: \n" << msg.c_str());
        return false;
    }
    uint32_t percentageSum = 0;
    while (domainPercentages->hasNext())
    {
        const ConfigSequence *pair = domainPercentages->nextSequence();
        Blaze::Clubs::ClubDomainId domainId = static_cast<Blaze::Clubs::ClubDomainId>(pair->nextUInt32(0));
        uint32_t percentage = pair->nextUInt32(0);
        percentageSum += percentage;
        mDomainPercentageMap.insert(eastl::make_pair(domainId, percentage));
        MemberCountMap *memberCountMap = BLAZE_NEW MemberCountMap;
        mDomainMemberCounts[domainId] = memberCountMap;
    }
    if (percentageSum != 100)
    {
        eastl::string msg;
        config.toString(msg);
        BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Please make sure sum of percentages in clubDomainPercentages is 100. config: \n" << msg.c_str());
        return false;
    }

    const ConfigSequence *memberCounts = config.getSequence("clubMemberCounts");
    if (memberCounts == nullptr || !memberCounts->hasNext())
    {
        eastl::string msg;
        config.toString(msg);
        BLAZE_ERR_LOG(BlazeRpcLog::clubs, "clubMemberCounts is required. config: \n" << msg.c_str());
        return false;
    }

    while (memberCounts->hasNext())
    {
        const ConfigSequence *pair = memberCounts->nextSequence();
        if (pair == nullptr)
        {
            eastl::string msg;
            config.toString(msg);
            BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Expected format is clubMemberCounts = [ [domainId1, count1], [domaind2, count2] ...] in config file: \n" << msg.c_str());
            return false;
        }
        uint32_t memberCount = pair->nextUInt32(0);
        uint32_t clubCount = pair->nextUInt32(0);
        uint32_t clubSum = 0, domainCount = 0;
        for (DomainPercentageMap::iterator it = mDomainPercentageMap.begin(); it != mDomainPercentageMap.end(); ++it)
        {
            domainCount++;
            DomainMemberCounts::iterator memberCountIt = mDomainMemberCounts.find(it->first);
            if (memberCountIt != mDomainMemberCounts.end())
            {
                uint32_t actualClubCount = 0;
                if (domainCount == mDomainPercentageMap.size())
                    actualClubCount = clubCount - clubSum;
                else
                {
                    double accurateClubCount = static_cast<double>(clubCount*(it->second)/100);
                    actualClubCount = static_cast<uint32_t>(accurateClubCount);
                    if (static_cast<double>(accurateClubCount - actualClubCount) >= 0.5)
                        actualClubCount++;
                }
                memberCountIt->second->insert(eastl::make_pair(memberCount, actualClubCount));
                clubSum += actualClubCount;
            }
        }
        mTotalClubs += clubCount;
        mTotalClubMembers += memberCount * clubCount;
    }

    mTotalUsers = config.getUInt32("totalUsers", 0);
    if (mTotalUsers < mTotalClubMembers)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Total user count " << mTotalUsers << " must be greater than total member count " << mTotalClubMembers << ".");
        return false;
    }
    
    const ConfigSequence *friendCountPercentages = config.getSequence("friendCountPercentages");
    if (friendCountPercentages != nullptr)
    {
        mFriendCountPercentageVector.assign(friendCountPercentages->getSize(), FriendCountPercentage());
        uint32_t index = 0;
        uint32_t lastEndPercent = 0;
        while (friendCountPercentages->hasNext())
        {
            const ConfigSequence *item = friendCountPercentages->nextSequence();
            uint32_t minCount = item->nextUInt32(0);
            uint32_t maxCount = item->nextUInt32(0);
            uint32_t percentage = item->nextUInt32(0);
            if (minCount >= maxCount || percentage == 0)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::clubs, "The item [" << minCount << ", " << maxCount << ", " <<  percentage << "] in friendCountPercentages in clubs module configuration is not correct.");
                return false;
            }
            mFriendCountPercentageVector[index].initialize(minCount, maxCount, percentage, lastEndPercent);
            index++;
            lastEndPercent += percentage;
        }
        if (lastEndPercent != 100 && lastEndPercent != 0)
        {
            eastl::string msg;
            config.toString(msg);
            BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Please make sure sum of percentages in friendCountPercentages is not correct (0 or 100). config: \n" << msg.c_str());
            return false;
        }
    }

    eastl::string action = config.getString("action", "");
    mAction = strToAction(action.c_str());
    if (mAction >= ACTION_MAX_ENUM)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Missing or illegal action parameter (" << action.c_str() << ") in clubs module configuration.");
        return false;
    }
    
    mNewsPerClub = config.getUInt32("newsPerClub", 0);
    mAwardsPerClub = config.getUInt32("awardsPerClub", 0);
    mRecordsPerClub = config.getUInt32("recordsPerClub", 0);

    mTotalTags = config.getUInt32("totalTags", 0);
    if (mTotalTags == 0)
    {
        uint32_t test=config.getUInt32("totalTags", UINT32_MAX);
        if (test==UINT32_MAX)
        {
            eastl::string msg;
            config.toString(msg);
            BLAZE_ERR(BlazeRpcLog::clubs, "totalTags is required. config: \n%s", msg.c_str());
        }
        else
        {
            BLAZE_ERR(BlazeRpcLog::clubs, "totalTags should be non-zero");
        }
        return false;
    }
    GET_NOT_ZERO(mTotalTags, "totalTags");

    // create the tags that we'll use
    for (uint32_t i = 1; i <= mTotalTags; ++i)
    {
        mTagMap[i].sprintf("Tag%u", i);
    }

    mClubTagPercent = 0;
    const ConfigSequence *tagCounts = config.getSequence("clubTagCounts");
    if (tagCounts != nullptr && !mTagMap.empty())
    {
        while (tagCounts->hasNext())
        {
            const ConfigSequence *pair = tagCounts->nextSequence();
            uint32_t size = pair->nextUInt32(0);
            uint32_t pct = pair->nextUInt32(0);
            if (size == 0 || size > static_cast<uint32_t>(mTagMap.size()))
            {
                // can't add more tags to a club than how many tags we're allowed to use
                // so ignore this pair...
                continue;
            }
            mTagCountMap.insert(eastl::make_pair(size, pct));
            mClubTagPercent += pct;
            if (mClubTagPercent >= 100)
            {
                // reached 100%, so ignore the remaining counts
                mClubTagPercent = 100; // don't exceed 100%
                break;
            }
        }
    }
    // else ok if we don't want to add tags to any clubs when recreating the database

    GET_NOT_ZERO(mClubsToGet, "clubsToGet");
    GET_NOT_ZERO(mClubMembershipsToGet, "clubMembershipsToGet");

    if (mClubsToGet > mTotalClubs)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs, "clubsToGet cannot be greater than total number of clubs created");    
        return false;
    }

    if(mClubMembershipsToGet > mTotalClubMembers)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs, "clubMembershipsToGet cannot be greater than total number of club members");    
        return false;
    }
    
    
    if (mRecreateDatabase && !createDatabase())
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Could not recreate database.");    
        return false;
    }

    // findClubsByTagsAll and findClubsByTagsAny parameters
    mTagsToGet = config.getUInt32("tagsToGet", 0);

    // find clubs params
    mFindClubsByName = config.getUInt32("findClubsByName", 0);
    mFindClubsByLanguage = config.getUInt32("findClubsByLanguage", 0);
    mFindClubsByTeamId = config.getUInt32("findClubsByTeamId", 0);
    mFindClubsByNonUniqueName = config.getUInt32("findClubsByNonUniqueName", 0);
    mFindClubsByAcceptanceFlags = config.getUInt32("findClubsByAcceptanceFlags", 0);
    mFindClubsBySeasonLevel = config.getUInt32("findClubsBySeasonLevel", 0);
    mFindClubsByRegion = config.getUInt32("findClubsByRegion", 0);
    mFindClubsByMinMemberCount = config.getUInt32("findClubsByMinMemberCount", 0);
    mFindClubsByMaxMemberCount = config.getUInt32("findClubsByMaxMemberCount", 0);
    mFindClubsByClubFilterList = config.getUInt32("findClubsByClubFilterList", 0);
    mFindClubsByMemberFilterList = config.getUInt32("findClubsByMemberFilterList", 0);
    mFindClubsSkipMetadata = config.getUInt32("findClubsSkipMetadata", 0);
    mFindClubsByDomain = config.getUInt32("findClubsByDomain", 0);
    mFindClubsByPassword = config.getUInt32("findClubsByPassword", 0);
    mFindClubsByMemberOnlineStatusCounts = config.getUInt32("findClubsByMemberOnlineStatusCounts", 0);
    mFindClubsByMemberOnlineStatusSum = config.getUInt32("findClubsByMemberOnlineStatusSum", 0);
    mFindClubsOrdered = config.getUInt32("findClubsOrdered", 0);
    mFindClubsMaxResultCount = config.getUInt32("findClubsMaxResultCount", 0);

    // get members params
    mGetMembersOrdered = config.getUInt32("getMembersOrdered", 0);
    mGetMembersFiltered = config.getUInt32("getMembersFiltered", 0);

    // Seed the random number generator (required by Blaze::Random)
    mRandSeed = config.getInt32("randSeed", -1);

    (mRandSeed < 0) ? srand((unsigned int)TimeValue::getTimeOfDay().getSec()) : srand(mRandSeed);

    mActionWeightSum = 0;
    memset(&mActionWeightDistribution[0], 0, sizeof(uint32_t) * ACTION_MAX_ENUM);
    const Blaze::ConfigMap* weightMap = config.getMap("actionWeightDistribution");
    while((weightMap != nullptr) && 
        (weightMap->hasNext()))
    {
        const char8_t* actionKeyStr = weightMap->nextKey();
        uint32_t actionKeyVal = (uint32_t)strToAction(actionKeyStr);
        if (actionKeyVal == ACTION_MAX_ENUM)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::clubs, "action " << actionKeyStr << " is not a valid action string");
            continue;
        }
        uint32_t actionVal = weightMap->getUInt32(actionKeyStr, 0);
        mActionWeightDistribution[actionKeyVal] = actionVal;
        mActionWeightSum += actionVal;
    }
    if ((mAction == ACTION_SIMULATE_PRODUCTION) &&
        (mActionWeightSum == 0))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Must specify actionWeightDistribution if using action=simulateProduction");
        return false;
    }
    
    mMaxNoLeaveClubId = config.getUInt32("maxNoLeaveClubId", mTotalClubs);
    if (mMaxNoLeaveClubId == 0)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Must specify maxNoLeaveClubId. In all the tests user will"
            " not leave the club if current membership club id is lower than this number");
        return false;
    }
    
    return true;
}

#ifdef STATIC_MYSQL_DRIVER
static void resetCommand(MYSQL *mysql)
{
    do
    {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (res)
            mysql_free_result(res);
        else
            mysql_affected_rows(mysql);
    } while (mysql_next_result(mysql) == 0);
}
#endif

bool ClubsModule::createDatabase()
{
#ifdef STATIC_MYSQL_DRIVER
    MYSQL mysql;

    mysql_init(&mysql);

    mysql_options(&mysql, MYSQL_OPT_COMPRESS, 0);

    bool bSuccess = mysql_real_connect(
            &mysql,
            mDbHost.c_str(),
            mDbUser.c_str(),
            mDbPass.c_str(),
            mDatabase.c_str(),
            mDbPort,
            nullptr,
            CLIENT_MULTI_STATEMENTS) != nullptr;

    if (!bSuccess)
    {
            BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Could not connect to db: " << mysql_error(&mysql));
            mysql_close(&mysql);
            mysql_library_end();
            return false;
    }
    
    BLAZE_INFO_LOG(BlazeRpcLog::clubs, "Connected to db.");

    // read-in entities from stats table to use as blaze ids for club members
    BlazeIdVector blazeIds;
    if (!mDbStatsTable.empty() && mTotalClubMembers > 0)
    {
        eastl::string statement;

        statement.sprintf("SELECT DISTINCT(`entity_id`) FROM %s LIMIT %d,%d;", mDbStatsTable.c_str(), mDbStatsEntityOffset, mTotalClubMembers);

        int error = mysql_query(&mysql, statement.c_str());
        if (error)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Error("<<mysql_errno(&mysql)<<"): ("<<mysql_sqlstate(&mysql)<<")" << mysql_error(&mysql));
            BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Last statement: " << statement.c_str());
            mysql_close(&mysql);
            mysql_library_end();
            return false;
        }

        MYSQL_RES *res = mysql_store_result(&mysql);
        if (res)
        {
            // build the list of blaze ids
            MYSQL_ROW row = mysql_fetch_row(res);
            for(; row != nullptr; row = mysql_fetch_row(res))
            {
                uint64_t entityid = 0;
                blaze_str2int(row[0], &entityid);
                if (entityid != 0)
                {
                    blazeIds.push_back(entityid);
                }
            }

            mysql_free_result(res);
        }
        else
        {
            mysql_affected_rows(&mysql);
        }

        if (mysql_next_result(&mysql) == 0)
        {
            resetCommand(&mysql);
        }

        uint32_t numFetched = blazeIds.size();
        if (numFetched < mTotalClubMembers)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Total ids fetched " << numFetched << " must be at least total member count " << mTotalClubMembers << ".");
            mysql_close(&mysql);
            mysql_library_end();
            return false;
        }
    }

    MYSQL_EXEC("SET autocommit = 0");
    MYSQL_EXEC("SET unique_checks = 0");
    MYSQL_EXEC("SET foreign_key_checks = 0");

    WIPEOUT_TABLE("user_association_list_communicationblocklist");
    WIPEOUT_TABLE("user_association_list_friendlist");
    WIPEOUT_TABLE("user_association_list_persistentmutelist");
    WIPEOUT_TABLE("user_association_list_recentplayerlist");
    WIPEOUT_TABLE("user_association_list_size");
    WIPEOUT_TABLE("clubs_awards");
    WIPEOUT_TABLE("clubs_bans");
    WIPEOUT_TABLE("clubs_club2tag");
    WIPEOUT_TABLE("clubs_clubs");
    WIPEOUT_TABLE("clubs_lastleavetime");
    WIPEOUT_TABLE("clubs_members");
    WIPEOUT_TABLE("clubs_messages");
    WIPEOUT_TABLE("clubs_news");
    WIPEOUT_TABLE("clubs_recordbooks");
    WIPEOUT_TABLE("clubs_rivals");
    WIPEOUT_TABLE("clubs_seasons");
    WIPEOUT_TABLE("clubs_tags");
    WIPEOUT_TABLE("clubs_tickermsgs");

    // fill out clubs_clubs table
    const uint32_t statementsToGroup = mStatementBatchSize;
    uint32_t userCount = 0;
    uint32_t clubCount = 0;
    DomainMemberCounts::const_iterator dmit = mDomainMemberCounts.begin();
    MemberCountMap::const_iterator mmit = dmit->second->begin();
    uint32_t clubsPerMemberCount = mmit->second;
    uint32_t memberCount = mmit->first;
    
    eastl::string statement;
    uint32_t statementCount  = 0;
    
    while(clubCount < mTotalClubs)
    {
        if (clubsPerMemberCount == 0)
        {
            if (++mmit == dmit->second->end())
            {
                if (++dmit == mDomainMemberCounts.end())
                    break;
                mmit = dmit->second->begin();
            }
            clubsPerMemberCount = mmit->second;
            memberCount = mmit->first;
        }    

        statement = "INSERT INTO `clubs_clubs` (name, domainId, memberCount, gmCount,"
            " lastActiveTime, lastResult, lastGameTime, teamId, language, nonuniquename, description,"
            " acceptanceFlags, artPackType, password,"
            " metaData, metaDataType, metaData2, metaDataType2,"
            " joinAcceptance, skipPasswordCheckAcceptance, petitionAcceptance,"
            " custOpt1, custOpt2, custOpt3, custOpt4, custOpt5,"
            " region, logoid, bannerid, seasonLevel) VALUES ";
            
        for (uint32_t j=0; j < statementsToGroup && clubsPerMemberCount > 0; j++, clubsPerMemberCount--)
        {
            eastl::string currentStat;
            
            eastl::string clubName;
            eastl::string clubAbb;
            eastl::string clubDesc;
            eastl::string clubPassword;
            
            clubName.sprintf(mPtrnName.c_str(), clubCount);
            clubAbb.sprintf(mPtrnNonUniqueName.c_str(), clubCount);
            clubDesc.sprintf(mPtrnDesc.c_str(), clubCount);
            if (clubCount%100 < mPasswordPercent)
                clubPassword.sprintf(mPassword.c_str());
            else
                clubPassword.sprintf("");
            uint32_t acceptanceFlags = 7;
            uint32_t joinAcceptance = 0, skipPasswordCheckAcceptance = 0, petitionAcceptance = 0;
          //  if (clubCount % 2 == 0)
            {
                acceptanceFlags = 2;  // Deprecate bits of joins and petitions in acceptanceFlags, use new 3 acceptances below.
                joinAcceptance = Blaze::Clubs::CLUB_ACCEPT_ALL;
                skipPasswordCheckAcceptance = Random::getRandomNumber(3);
                petitionAcceptance = Blaze::Clubs::CLUB_ACCEPT_ALL;
            }
            
            statement.append_sprintf(
                    "('%s', %u, %d, 1, NOW(), '', 0, %d, '%s', '%s', '%s', %d, 0, '%s', 'x', 0, '', 0, %d, %d, %d, 0, 0, 0, 0, 0, %d, 0, 0, %d),", 
                    clubName.c_str(), dmit->first, memberCount, Random::getRandomNumber(mMaxTeams), getRandomLang(), clubAbb.c_str(), clubDesc.c_str(), 
                    acceptanceFlags, clubPassword.c_str(), joinAcceptance, skipPasswordCheckAcceptance, petitionAcceptance,
                    Random::getRandomNumber(mMaxRegions) + 1,
                    Random::getRandomNumber(mMaxSeasonLevels) + 1);

            clubCount++;
        }

        // get rid of last comma
        statement.pop_back();

        if (clubCount % 100 == 0)
            BLAZE_INFO_LOG(BlazeRpcLog::clubs, "Creating club " << clubCount);

        int error = mysql_query(&mysql, statement.c_str());
        if (error)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Error("<<mysql_errno(&mysql)<<"): ("<<mysql_sqlstate(&mysql)<<")" << mysql_error(&mysql));
            BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Last statement: " << statement.c_str());
            mysql_close(&mysql);
            mysql_library_end();
            return false;
        }
            
        resetCommand(&mysql);
    }


    // fill out clubs_members table
    userCount = 0;
    uint64_t blazeid = mMinAccId;
    uint32_t clubid = 1;
    dmit = mDomainMemberCounts.begin();
    mmit = dmit->second->begin();
    clubsPerMemberCount = mmit->second;
    memberCount = mmit->first;
    
    bool bIsOwner = true;

    BlazeIdVector::const_iterator idItr = blazeIds.begin();
    BlazeIdVector::const_iterator idEnd = blazeIds.end();
    if (idItr != idEnd)
    {
        blazeid = *idItr++; // use ids fetched from a stats table
    }

    statement.clear();
    statementCount = 0;
    
    while(true)
    {
        if (memberCount == 0)
        {
            --clubsPerMemberCount;
            if (clubsPerMemberCount == 0)
            {
                if (++mmit == dmit->second->end())
                {
                    if (++dmit == mDomainMemberCounts.end())
                        break;

                    mmit = dmit->second->begin();
                }
                clubsPerMemberCount = mmit->second;
            }
            clubid++;
            memberCount = mmit->first;
            bIsOwner = true;
            
        }
        
        if (statement.empty())
            statement = "INSERT INTO `clubs_members` (blazeid, clubid, membershipStatus, memberSince, metadata) VALUES ";
            
        for (; statementCount < statementsToGroup && memberCount > 0; memberCount--, userCount++, statementCount++)
        {
            mMembershipMap[clubid].push_back(blazeid);
            statement.append_sprintf(
                    "(%" PRIu64 ", %d, %d, NOW(), 'x'),", blazeid++, clubid, bIsOwner ? 2 : 0 );

            if (idItr != idEnd)
            {
                blazeid = *idItr++; // use ids fetched from a stats table
            }
            
            bIsOwner = false;
        }

        if (userCount % 100 == 0)
            BLAZE_INFO_LOG(BlazeRpcLog::clubs, "Creating member " << userCount);
            

        if (statementCount >= statementsToGroup)
        {
            // remove last comma
            statement.pop_back();
            int error = mysql_query(&mysql, statement.c_str());
            if (error)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Error("<<mysql_errno(&mysql)<<"): ("<<mysql_sqlstate(&mysql)<<")" << mysql_error(&mysql));
                BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Last statement: " << statement.c_str());
                mysql_close(&mysql);
                mysql_library_end();
                return false;
            }
            resetCommand(&mysql);
            statement.clear();
            statementCount = 0;
        }
    }
    
    // fill clubs_news table
    clubCount = 0;

    uint32_t newsCount = 0;
    uint32_t newsType = CLUBS_SERVER_GENERATED_NEWS;
    uint64_t gmid = mMinAccId;
    dmit = mDomainMemberCounts.begin();
    mmit = dmit->second->begin();
    clubsPerMemberCount = mmit->second;
    memberCount = mmit->first;
    
    statement.clear();
    statementCount = 0;

    if (mNewsPerClub > 0)
    {
        while(clubCount < mTotalClubs)
        {
            
            if (statement.empty())    
                statement = "INSERT INTO `clubs_news` (clubid, associatedclubid, newstype, "
                    "contentCreator, text, stringid, paramList, timestamp) VALUES ";
                
            clubid = clubCount + 1;

            for (uint32_t j=0; j < mNewsPerClub; j++, newsCount++, statementCount++)
            {
                eastl::string newsString;
                newsString.sprintf(mPtrnNewsBody.c_str(), newsCount);
                
                if (newsType == CLUBS_SERVER_GENERATED_NEWS)
                    statement.append_sprintf(
                            "( %d, %d, %d, %d, '%s', '%s', '%s', NOW() ),", 
                            clubid, clubid, CLUBS_SERVER_GENERATED_NEWS, 0, "", "OSDK_CLUBS_EVENT_FIRST", "fake1"  );
                else if (newsType == CLUBS_MEMBER_POSTED_NEWS)
                    statement.append_sprintf(
                            "( %d, %d, %d, %" PRIu64 ", '%s', '%s', '%s', NOW() ),", 
                            clubid, clubid, CLUBS_MEMBER_POSTED_NEWS, gmid, newsString.c_str(), "", ""  );
                else if (newsType == CLUBS_ASSOCIATE_NEWS)
                    statement.append_sprintf(
                            "( %d, %d, %d, %d, '%s', '%s', '%s', NOW() ),", 
                            clubid, mTotalClubs - clubid / 2, CLUBS_ASSOCIATE_NEWS, 0, "", "OSDK_CLUBS_EASW_RIVAL", "fake1,fake2"  );
                                       

                newsType++;
                newsType %= 3;
            }

            if (newsCount % 100 == 0)
                BLAZE_INFO_LOG(BlazeRpcLog::clubs, "Creating news " << (newsCount + 1));

            if (statementCount >= statementsToGroup)
            {
                // remove last comma
                statement.pop_back();
                int error = mysql_query(&mysql, statement.c_str());
                if (error)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Error("<<mysql_errno(&mysql)<<"): ("<<mysql_sqlstate(&mysql)<<")" << mysql_error(&mysql));
                    BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Last statement: " << statement.c_str());
                    mysql_close(&mysql);
                    mysql_library_end();
                    return false;
                }
                resetCommand(&mysql);
                statement.clear();
                statementCount = 0;
            }

            clubCount++;

            --clubsPerMemberCount;
            gmid += mmit->first;
            if (clubsPerMemberCount == 0)
            {
                if (++mmit == dmit->second->end())
                {
                    if (++dmit == mDomainMemberCounts.end())
                        break;

                    mmit = dmit->second->begin();
                }
                clubsPerMemberCount = mmit->second;
            }
        }
    }


    statement.clear();
    statementCount = 0;
    
    // fill clubs_awards table
    if (mAwardsPerClub > 0)
    {
        for (clubCount = 0; clubCount < mTotalClubs; clubCount++)
        {
            
            if (clubCount % 100 == 0)
                BLAZE_INFO_LOG(BlazeRpcLog::clubs, "Creating awards for club " << (clubCount + 1));
            
            if (statement.empty()) 
                statement = "INSERT INTO `clubs_awards` (clubId, awardId, awardCount, timestamp) VALUES ";
                
            for (uint32_t j=0; j < mAwardsPerClub; j++, statementCount++)
            {
                statement.append_sprintf(
                        "(%d, %d, 1, NOW() ),", clubCount + 1, j + 1);
            }

            if (statementCount >= statementsToGroup)
            {
                // remove last comma
                statement.pop_back();
                int error = mysql_query(&mysql, statement.c_str());
                if (error)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Error("<<mysql_errno(&mysql)<<"): ("<<mysql_sqlstate(&mysql)<<")" << mysql_error(&mysql));
                    BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Last statement: " << statement.c_str());
                    mysql_close(&mysql);
                    mysql_library_end();
                    return false;
                }
                resetCommand(&mysql);
                statement.clear();
                statementCount = 0;
            }
        }
    }

    // fill clubs_recordbooks table
    clubCount = 0;

    uint32_t recordCount = 0;
    gmid = mMinAccId;
    dmit = mDomainMemberCounts.begin();
    mmit = dmit->second->begin();
    clubsPerMemberCount = mmit->second;
    memberCount = mmit->first;
    
    statement.clear();
    statementCount = 0;
    
    if (mRecordsPerClub > 0)
    {
        while(clubCount < mTotalClubs)
        {
                
            if (clubCount % 100 == 0)
                BLAZE_INFO_LOG(BlazeRpcLog::clubs, "Creating records for club " << (clubCount + 1));

            if (statement.empty())
                statement = "INSERT INTO `clubs_recordbooks` (clubId, recordId, blazeId, "
                    "recordStatInt, recordStatFloat, recordStatType, timestamp) VALUES ";
                
            clubid = clubCount + 1;

            for (uint32_t j=0; j < mRecordsPerClub; j++, recordCount++, statementCount++)
            {
                statement.append_sprintf(
                        "( %d, %d, %" PRIu64 ", %d, %d, %d, NOW() ),", 
                        clubid, (recordCount % mRecordsPerClub) + 1, gmid, recordCount, recordCount, 0 );
            }

            if (statementCount >= statementsToGroup)
            {
                // remove last comma
                statement.pop_back();
                int error = mysql_query(&mysql, statement.c_str());
                if (error)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Error("<<mysql_errno(&mysql)<<"): ("<<mysql_sqlstate(&mysql)<<")" << mysql_error(&mysql));
                    BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Last statement: " << statement.c_str());
                    mysql_close(&mysql);
                    mysql_library_end();
                    return false;
                }
                resetCommand(&mysql);
                statement.clear();
                statementCount = 0;
            }
            
            clubCount++;

            --clubsPerMemberCount;
            gmid += mmit->first;
            if (clubsPerMemberCount == 0)
            {
                if (++mmit == dmit->second->end())
                {
                    if (++dmit == mDomainMemberCounts.end())
                        break;

                    mmit = dmit->second->begin();
                }
                clubsPerMemberCount = mmit->second;
            }
        }
    }

    // fill clubs_tags table
    uint32_t tagCount = 0;
    TagMap::const_iterator tagItr = mTagMap.begin();
    TagMap::const_iterator tagEnd = mTagMap.end();
    for (; tagItr != tagEnd; ++tagItr, ++tagCount)
    {
        eastl::string stmt;
        stmt.sprintf("INSERT INTO `clubs_tags` (`tagText`) VALUES ('%s');", tagItr->second.c_str());

        int error = mysql_query(&mysql, stmt.c_str());
        if (error)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Error("<<mysql_errno(&mysql)<<"): ("<<mysql_sqlstate(&mysql)<<")" << mysql_error(&mysql));
            BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Last statement: " << stmt.c_str());
            mysql_close(&mysql);
            mysql_library_end();
            return false;
        }

        resetCommand(&mysql);
    }

    BLAZE_INFO_LOG(BlazeRpcLog::clubs, "Created " << tagCount << " tag" << (tagCount == 1 ? "" : "s"));

    // fill clubs_club2tag table
    if (!mTagCountMap.empty()) // note: if mTagMap is empty, then mTagCountMap would be empty as well
    {
        for (clubCount = 0; clubCount < mTotalClubs; ++clubCount)
        {
            uint32_t clubPercent = clubCount % 100;

            if (clubPercent == 0)
            {
                BLAZE_INFO_LOG(BlazeRpcLog::clubs, "Creating tags for club " << (clubCount + 1));
            }

            // based on percent (0-99), lookup in mTagCountMap how many tags to pick
            uint32_t numTags = 0;
            uint32_t pctGroup = 0;
            TagCountMap::const_iterator countItr = mTagCountMap.begin();
            TagCountMap::const_iterator countEnd = mTagCountMap.end();
            for (; countItr != countEnd; ++countItr)
            {
                pctGroup += countItr->second;
                if (clubPercent < pctGroup)
                {
                    numTags = countItr->first;
                    break;
                }
            }

            TagIdList tagIdList;
            pickRandomTagIds(numTags, tagIdList);

            TagIdList::const_iterator tagIdItr = tagIdList.begin();
            TagIdList::const_iterator tagIdEnd = tagIdList.end();
            for (; tagIdItr != tagIdEnd; ++tagIdItr)
            {
                eastl::string stmt;
                stmt.sprintf("INSERT INTO `clubs_club2tag` (`clubId`, `tagId`) VALUES (%u, %u);", clubCount + 1, *tagIdItr);

                int error = mysql_query(&mysql, stmt.c_str());
                if (error)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Error("<<mysql_errno(&mysql)<<"): ("<<mysql_sqlstate(&mysql)<<")" << mysql_error(&mysql));
                    BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Last statement: " << stmt.c_str());
                    mysql_close(&mysql);
                    mysql_library_end();
                    return false;
                }

                resetCommand(&mysql);
            }
        }
    }

    // Create friend lists for all users
    if (!mFriendCountPercentageVector.empty())
    {
        BlazeId blazeidTemp;
        uint32_t blazeIdCount = mTotalUsers;
        typedef eastl::set<BlazeId> BlazeIdSet;
        BlazeIdSet friendIdSet;
        const uint32_t maxRandomAttempts = 20;
        const uint32_t maxListCountForInsert = statementsToGroup * 4, maxInsertCountForPrint = 50;
        uint32_t listCountForInsert = 0, insertCountForPrint = 0;
        eastl::string listInfoStatement;

        if (blazeIdCount > 0)
            BLAZE_INFO_LOG(BlazeRpcLog::clubs, "Creating friend lists for " << blazeIdCount << " users...");

        for (uint32_t blazeIdIndex = 0; blazeIdIndex < blazeIdCount; ++blazeIdIndex)
        {
            blazeidTemp = mMinAccId + blazeIdIndex;
            for (FriendCountPercentageVector::const_iterator percentageIt = mFriendCountPercentageVector.begin(); percentageIt != mFriendCountPercentageVector.end(); ++percentageIt)
            {
                if (blazeIdIndex % 100 >= percentageIt->mStartPercent && blazeIdIndex % 100 < percentageIt->mEndPercent)
                {
                    friendIdSet.clear();
                    uint32_t friendCount = static_cast<uint32_t>(percentageIt->mMinCount + Random::getRandomNumber(static_cast<int>(percentageIt->mMaxCount - percentageIt->mMinCount)));
                    for (uint32_t count = 0; count < friendCount; ++count)
                    {
                        BlazeId friendId = mMinAccId + static_cast<uint32_t>(Random::getRandomNumber(static_cast<int>(blazeIdCount)));
                        BlazeIdSet::insert_return_type insertFriendIt = friendIdSet.insert(friendId);
                        uint32_t randomCount = 0;
                        while ((!insertFriendIt.second || friendId == blazeidTemp) && randomCount < maxRandomAttempts)
                        {
                            friendId = mMinAccId + static_cast<BlazeId>(Random::getRandomNumber(static_cast<int>(blazeIdCount)));
                            insertFriendIt = friendIdSet.insert(friendId);
                            randomCount++;
                        }
                    }
                    if (!friendIdSet.empty())
                    {
                        uint64_t dateAdded = TimeValue::getTimeOfDay().getMicroSeconds();
                        for (BlazeIdSet::const_iterator friendIdIt = friendIdSet.begin(), friendIdEnd = friendIdSet.end(); friendIdIt != friendIdEnd; ++friendIdIt)
                        {


                            eastl::string currentStat;
                            currentStat.sprintf(" (%" PRIu64 ",%" PRIu64 ",%" PRIu64 "),",
                                blazeidTemp, // ownerBlazeId
                                *friendIdIt, // friend blazeid
                                dateAdded++); 
                            listInfoStatement.append(currentStat);
                        }

                        listCountForInsert++;
                    }
                    break;
                }
            }
            
            if (listCountForInsert == maxListCountForInsert || (blazeIdIndex == blazeIdCount - 1 && listCountForInsert > 0))
            {
                listCountForInsert = 0;
                insertCountForPrint++;

                if (insertCountForPrint % maxInsertCountForPrint == 0)
                    BLAZE_INFO_LOG(BlazeRpcLog::clubs, "Creating friend list for user " << blazeIdIndex << ", friend count " << friendIdSet.size());

                // user_association_list_friendslist
                listInfoStatement.replace(listInfoStatement.size() - 1, 1, ";");  // replace the last ',' with ';'
                eastl::string stmt = "INSERT INTO `user_association_list_friendslist` (`blazeid`, `memberblazeid`,`dateadded`) VALUES ";
                stmt.append(listInfoStatement);
                int error = mysql_query(&mysql, stmt.c_str());
                if (error)
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Error("<<mysql_errno(&mysql)<<"): ("<<mysql_sqlstate(&mysql)<<")" << mysql_error(&mysql));
                    BLAZE_ERR_LOG(BlazeRpcLog::clubs, "Last statement: " << stmt.c_str());
                    mysql_close(&mysql);
                    mysql_library_end();
                    return false;
                } 
                resetCommand(&mysql);

                listInfoStatement.reset_lose_memory();
            }
        }
    }

    MYSQL_EXEC("COMMIT");
    MYSQL_EXEC("SET unique_checks = 1");
    MYSQL_EXEC("SET foreign_key_checks = 1");

    mysql_close(&mysql);
    mysql_library_end();


    return true;
#else
    return false;   //No static MySQL lib
#endif
}

StressInstance* ClubsModule::createInstance(StressConnection* connection, Login* login)
{
    return BLAZE_NEW ClubsInstance(this, connection, login);
}

/*** Private Methods *****************************************************************************/

ClubsModule::ClubsModule()
  : mClubDomainMap(Blaze::BlazeStlAllocator("ClubsModule::mClubDomainMap", ClubsSlave::COMPONENT_MEMORY_GROUP)),
    mDomainMemberCounts(Blaze::BlazeStlAllocator("ClubsModule::mDomainMemberCounts", ClubsSlave::COMPONENT_MEMORY_GROUP)),
    mRandSeed(-1),
    mStatsConfig(nullptr)

{
}

void ClubsModule::pickRandomTagIds(TagIdList &tagIdList)
{
    tagIdList.clear();

    if (mTagMap.empty())
    {
        // no tags to pick
        return;
    }

    // fill a temp tag id list with all the tags
    TagIdList availTagIdList;
    TagMap::const_iterator tagItr = mTagMap.begin();
    TagMap::const_iterator tagEnd = mTagMap.end();
    for (; tagItr != tagEnd; ++tagItr)
    {
        availTagIdList.push_back(tagItr->first);
    }

    int32_t numTags = Random::getRandomNumber(static_cast<int32_t>(mTagMap.size())) + 1;

    for (; numTags > 0; --numTags)
    {
        if (availTagIdList.empty())
        {
            // no more tags to choose
            return;
        }

        // randomly pick a tag from the list...
        int32_t r = Random::getRandomNumber(static_cast<int32_t>(availTagIdList.size()));
        TagIdList::iterator tagIdItr = availTagIdList.begin();
        for (; r > 0; --r)
        {
            ++tagIdItr;
        }

        tagIdList.push_back(*tagIdItr);

        // remove that tag id from the available list
        availTagIdList.erase(tagIdItr);
    }
}

void ClubsModule::pickRandomTagIds(uint32_t numTags, TagIdList &tagIdList)
{
    tagIdList.clear();

    if (mTagMap.empty() || numTags == 0)
    {
        // no tags to pick
        return;
    }

    // fill a temp tag id list with all the tags
    TagIdList availTagIdList;
    TagMap::const_iterator tagItr = mTagMap.begin();
    TagMap::const_iterator tagEnd = mTagMap.end();
    for (; tagItr != tagEnd; ++tagItr)
    {
        availTagIdList.push_back(tagItr->first);
    }

    for (; numTags > 0; --numTags)
    {
        if (availTagIdList.empty())
        {
            // no more tags to choose
            return;
        }

        // randomly pick a tag from the list...
        int32_t r = Random::getRandomNumber(static_cast<int32_t>(availTagIdList.size()));
        TagIdList::iterator tagIdItr = availTagIdList.begin();
        for (; r > 0; --r)
        {
            ++tagIdItr;
        }

        tagIdList.push_back(*tagIdItr);

        // remove that tag id from the available list
        availTagIdList.erase(tagIdItr);
    }
}

void ClubsModule::pickRandomClubAndPlayer(MembershipMap &clubPlayerMap)
{
    // pick random club
    ClubId clubId = 0;
    while ((clubId == 0) || 
           (mMembershipMap[clubId].empty())||
           (!clubPlayerMap[clubId].empty()))
    {
        BLAZE_TRACE_LOG(
            BlazeRpcLog::clubs, 
            "[pickRandomClubAndPlayer] sleeping for " << clubId);
        BlazeRpcError err = gSelector->sleep(clubId);
        if (err != Blaze::ERR_OK)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::clubs, "[pickRandomClubAndPlayer] Got sleep error " << (ErrorHelp::getErrorName(err)));
        }
        int32_t i = Random::getRandomNumber(static_cast<int32_t>(mMembershipMap.size()));
        MembershipMap::const_iterator itr = mMembershipMap.begin();
        for (;i > 0; i--)
        {
            ++itr; 
        }
        clubId = itr->first;    
    }
    const BlazeIdVector &idVector = mMembershipMap[clubId];
    BlazeId playerId = idVector[Random::getRandomNumber(static_cast<int32_t>(idVector.size()))];
    clubPlayerMap[clubId].push_back(playerId);
}

Blaze::Clubs::MemberOnlineStatus ClubsModule::getMemberOnlineStatusFromIndex(int statusIndex)
{
    Blaze::Clubs::MemberOnlineStatus status = Blaze::Clubs::CLUBS_MEMBER_ONLINE_STATUS_COUNT;
    switch (statusIndex)
    {
    case 0:
        status = Blaze::Clubs::CLUBS_MEMBER_OFFLINE;
    case 1:
        status = Blaze::Clubs::CLUBS_MEMBER_ONLINE_INTERACTIVE;
    case 2:
        status = Blaze::Clubs::CLUBS_MEMBER_ONLINE_NON_INTERACTIVE;
    case 3:
        status = Blaze::Clubs::CLUBS_MEMBER_IN_GAMEROOM;
    case 4:
        status = Blaze::Clubs::CLUBS_MEMBER_IN_OPEN_SESSION;
    case 5:
        status = Blaze::Clubs::CLUBS_MEMBER_IN_GAME;
    case 6:
        status = Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_1;
    case 7:
        status = Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_2;
    case 8:
        status = Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_3;
    case 9:
        status = Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_4;
    case 10:
        status = Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_5;
    case 11:
        status = Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_6;
    case 12:
        status = Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_7;
    case 13:
        status = Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_8;
    }
    return status;
}

ClubId ClubsInstance::getRandomClub() const
{
    if (mClubIdSet.size() == 0)
        return 0;

    return mClubIdSet[Random::getRandomNumber(mClubIdSet.size())];
}

ClubsInstance::ClubsInstance(ClubsModule* owner, StressConnection* connection, Login* login)
:   StressInstance(owner, connection, login, BlazeRpcLog::clubs),
    mOwner(owner),
    mTimerId(INVALID_TIMER_ID),
    mClubsProxy(BLAZE_NEW ClubsSlave(*connection)),
    mGameManagerProxy(BLAZE_NEW GameManagerSlave(*connection)),
    mUsersJoinedGame(0), mClubGameOver(false), mRunCount(0), mCreateIt(false),
    mGameId(GameManager::INVALID_GAME_ID), mResultNotificationError(-1), mOutstandingActions(0)
{
    connection->addAsyncHandler(Blaze::UserSessionsSlave::COMPONENT_ID, Blaze::UserSessionsSlave::NOTIFY_USERSESSIONEXTENDEDDATAUPDATE, this);
    connection->addAsyncHandler(Blaze::UserSessionsSlave::COMPONENT_ID, Blaze::UserSessionsSlave::NOTIFY_USERADDED, this);
    connection->addAsyncHandler(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, Blaze::GameManager::GameManagerSlave::NOTIFY_NOTIFYGAMESTATECHANGE, this);
    connection->addAsyncHandler(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, Blaze::GameManager::GameManagerSlave::NOTIFY_NOTIFYPLAYERJOINING, this);
    connection->addAsyncHandler(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, Blaze::GameManager::GameManagerSlave::NOTIFY_NOTIFYGAMEREPORTINGIDCHANGE, this);
    mInstanceOrd = mInstanceOrdStatic++;
    mName = "";
}

void ClubsInstance::onLogin(Blaze::BlazeRpcError result)
{
    if (result == Blaze::ERR_OK)
        mBlazeId = getLogin()->getUserLoginInfo()->getBlazeId();
}

void ClubsInstance::onDisconnected()
{
    BLAZE_INFO_LOG(BlazeRpcLog::clubs, "[onDisconnect:] Disconnected. Run count: " << mRunCount);
}

void ClubsInstance::onUserExtendedData(const Blaze::UserSessionExtendedData& extData)
{
    const ObjectIdList &list = extData.getBlazeObjectIdList();
    ObjectIdList::const_iterator it = list.begin();
    for (; it != list.end(); it++)
    {
        if ((*it).type == ENTITY_TYPE_CLUB)
        {
            ClubId clubId = static_cast<ClubId>((*it).id);
            mClubIdSet.insert(clubId);

            BlazeIdList::const_iterator iter = mOwner->mMembershipMap[clubId].begin();
            while(iter != mOwner->mMembershipMap[clubId].end())
            {
                if (*iter == mBlazeId)
                    break;
                iter++;
            }
            if (iter == mOwner->mMembershipMap[clubId].end())
                mOwner->mMembershipMap[clubId].push_back(mBlazeId);
            if (mTimerId != INVALID_TIMER_ID)
            {
                gSelector->cancelTimer(mTimerId);
                mTimerId = INVALID_TIMER_ID;
            }
            break;                
        }
    }
}

void ClubsInstance::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    if (component == Blaze::UserSessionsSlave::COMPONENT_ID)
    {
        if (type == Blaze::UserSessionsSlave::NOTIFY_USERADDED)
        {
            Blaze::NotifyUserAdded userAdded;
            Heat2Decoder decoder;
            if(decoder.decode(*payload, userAdded) == ERR_OK)
            {
                // Because each user is associate with its own connection, we do not
                // do any validation here to ensure that this notification is "for us"
                onUserExtendedData(userAdded.getExtendedData());
            }
        }
        else if (type == Blaze::UserSessionsSlave::NOTIFY_USERSESSIONEXTENDEDDATAUPDATE)
        {
            Blaze::UserSessionExtendedDataUpdate extData;
            Heat2Decoder decoder;
            if(decoder.decode(*payload, extData) == ERR_OK)
            {
                if (extData.getBlazeId() == mBlazeId)
                {
                    onUserExtendedData(extData.getExtendedData());
                }
            }
        }
    }
}



BlazeRpcError ClubsInstance::execute()
{
    
    BlazeRpcError result = ERR_OK;
    
    ++mRunCount;
    BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[execute:] Start action " << mOwner->mAction << ".");

    ClubsModule::Action action = mOwner->mAction;
    if (action == ClubsModule::ACTION_SIMULATE_PRODUCTION)
    {
        mName = "simulateProduction";
        if (mOutstandingActions == 0)
        {
            // reset our distributions
            for (uint32_t i=0; i<ClubsModule::ACTION_MAX_ENUM; i++)
            {
                mActionWeightDistribution[i] = mOwner->mActionWeightDistribution[i];
            }
            mOutstandingActions = mOwner->mActionWeightSum;
        }

        action = getWeightedRandomAction();
        BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[execute:] Random action " << action << ".");
    }

    switch(action)
    {
    case ClubsModule::ACTION_GET_CLUBS:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_GET_CLUBS);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_GET_CLUBS));
            result = actionGetClubs();
            break;
        }
    case ClubsModule::ACTION_GET_NEWS:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_GET_NEWS);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_GET_NEWS));
            result = actionGetNews();
            break;
        }
    case ClubsModule::ACTION_GET_AWARDS:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_GET_AWARDS);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_GET_AWARDS));
            result = actionGetAwards();
            break;
        }
    case ClubsModule::ACTION_GET_CLUBRECORDBOOK:        
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_GET_CLUBRECORDBOOK);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_GET_CLUBRECORDBOOK));
            result = actionGetClubRecordbook();
            break;
        }   
    case ClubsModule::ACTION_CREATE_CLUB:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_CREATE_CLUB);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_CREATE_CLUB));
            result = actionCreateClub();
            break;
        }
    case ClubsModule::ACTION_FIND_CLUBS:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_FIND_CLUBS);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_FIND_CLUBS));
            result = actionFindClubs();
            break;
        }
    case ClubsModule::ACTION_FIND_CLUBS_BY_TAGS_ALL:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_FIND_CLUBS_BY_TAGS_ALL);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_FIND_CLUBS_BY_TAGS_ALL));
            result = actionFindClubsByTagsAll();
            break;
        }
    case ClubsModule::ACTION_FIND_CLUBS_BY_TAGS_ANY:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_FIND_CLUBS_BY_TAGS_ANY);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_FIND_CLUBS_BY_TAGS_ANY));
            result = actionFindClubsByTagsAny();
            break;
        }
    case ClubsModule::ACTION_FIND_CLUBS2:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_FIND_CLUBS2);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_FIND_CLUBS2));
            result = actionFindClubs2();
            break;
        }
    case ClubsModule::ACTION_FIND_CLUBS2_BY_TAGS_ALL:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_FIND_CLUBS2_BY_TAGS_ALL);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_FIND_CLUBS2_BY_TAGS_ALL));
            result = actionFindClubs2ByTagsAll();
            break;
        }
    case ClubsModule::ACTION_FIND_CLUBS2_BY_TAGS_ANY:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_FIND_CLUBS2_BY_TAGS_ANY);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_FIND_CLUBS2_BY_TAGS_ANY));
            result = actionFindClubs2ByTagsAny();
            break;
        }
    case ClubsModule::ACTION_PLAY_CLUB_GAME:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_PLAY_CLUB_GAME);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_PLAY_CLUB_GAME));
            result = actionPlayClubGame();
            break;
        }
    case ClubsModule::ACTION_JOIN_CLUB:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_JOIN_CLUB);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_JOIN_CLUB));
            result = actionJoinClub();
            break;
        }
    case ClubsModule::ACTION_JOIN_OR_PETITION_CLUB:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_JOIN_OR_PETITION_CLUB);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_JOIN_OR_PETITION_CLUB));
            result = actionJoinOrPetitionClub();
            break;
        }
    case ClubsModule::ACTION_GET_MEMBERS:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_GET_MEMBERS);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_GET_MEMBERS));
            result = actionGetMembers();
            break;
        }
    case ClubsModule::ACTION_GET_CLUBS_COMPONENT_SETTINGS:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_GET_CLUBS_COMPONENT_SETTINGS);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_GET_CLUBS_COMPONENT_SETTINGS));
            result = actionGetClubsComponentSettings();
            break;
        }
    case ClubsModule::ACTION_GET_CLUB_MEMBERSHIP_FOR_USERS:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_GET_CLUB_MEMBERSHIP_FOR_USERS);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_GET_CLUB_MEMBERSHIP_FOR_USERS));
            result = actionGetClubMembershipForUsers();
            break;
        }
    case ClubsModule::ACTION_POST_NEWS:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_POST_NEWS);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_POST_NEWS));
            result = actionPostNews();
            break;
        }
    case ClubsModule::ACTION_PETITION:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_PETITION);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_PETITION));
            result = actionPetition();
            break;
        }
    case ClubsModule::ACTION_UPDATE_ONLINE_STATUS:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_UPDATE_ONLINE_STATUS);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_UPDATE_ONLINE_STATUS));
            result = actionUpdateOnlineStatus();
            break;
        }
    case ClubsModule::ACTION_UPDATE_MEMBER_METADATA:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_UPDATE_MEMBER_METADATA);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_UPDATE_MEMBER_METADATA));
            result = actionUpdateMemberMetadata();
            break;
        }
    case ClubsModule::ACTION_SET_METADATA:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_SET_METADATA);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_SET_METADATA));
            result = actionSetMetadata();
            break;
        }
    case ClubsModule::ACTION_UPDATE_CLUB_SETTINGS:
        {
            mName = mOwner->actionToStr(ClubsModule::ACTION_UPDATE_CLUB_SETTINGS);
            Fiber::setCurrentContext(mOwner->actionToContext(ClubsModule::ACTION_UPDATE_CLUB_SETTINGS));
            result = actionUpdateClubSettings();
            break;
        }
    default:
        {
            BLAZE_ERR_LOG(BlazeRpcLog::clubs, "[ClubsInstance::execute] Unknown action(" << mOwner->mAction << ")");
            result = ERR_SYSTEM;
        }
    }
    return result;
}

bool ClubsInstance::tossTheDice(uint32_t val)
{
    if (static_cast<uint32_t>(Random::getRandomNumber(100)) < val)
        return true;
        
    return false;
}

ClubsModule::Action ClubsInstance::getWeightedRandomAction()
{
    /* pick a number between 0 and mOutstandingActions.  If the random number falls 
     * within the range of bin N, then return action N
     */
    int32_t rand = Random::getRandomNumber(mOutstandingActions);
    uint32_t i = 1;
    for(; i<ClubsModule::ACTION_MAX_ENUM; i++)
    {
        rand -= mActionWeightDistribution[i];
        if (rand < 0)
            break;
    }
    EA_ASSERT(i < ClubsModule::ACTION_MAX_ENUM);
    mActionWeightDistribution[i]--;
    mOutstandingActions--;
        
    return (ClubsModule::Action)(i);
}

BlazeRpcError ClubsInstance::actionGetClubs()
{
    GetClubsRequest request;
    GetClubsResponse response;
    ClubIdList &idList = request.getClubIdList();
    for (uint32_t i = 0; i < mOwner->mClubsToGet; i++)
    {
        while (true)
        {
            uint32_t id = Random::getRandomNumber(mOwner->mTotalClubs) + 1 ;
            if (eastl::find(idList.begin(), idList.end(), id) == idList.end())
            {
                idList.push_back(id);
                break;
            }
        }
    }
            
    BlazeRpcError error = mClubsProxy->getClubs(request, response);

    if (error == Blaze::ERR_OK && response.getClubList().size() != mOwner->mClubsToGet)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionGetClubs] I got " << response.getClubList().size() 
                        << " clubs back, but expected: " << mOwner->mClubsToGet);
    }
    
    return error;
}

BlazeRpcError ClubsInstance::actionGetNews()
{
    GetNewsRequest request;
    GetNewsResponse response;
    
    request.setClubId(Random::getRandomNumber(mOwner->mTotalClubs) + 1);
    request.setMaxResultCount(mOwner->mNewsPerClub);
    request.setOffSet(0);
    request.setSortType(CLUBS_SORT_TIME_ASC);
    uint32_t numNewsFlags = Random::getRandomNumber(3) + 1;
    NewsTypeList &newsList = request.getTypeFilters();
    for (uint32_t i = 0; i < numNewsFlags; i++)
    {
        uint32_t flag = Random::getRandomNumber(4);
        newsList.push_back(static_cast<NewsType>(flag));
    }
    
    BlazeRpcError error = mClubsProxy->getNews(request, response);
    
    if (error == ERR_OK && response.getLocalizedNewsList().size() == 0)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionGetNews] I got 0 news back, but expected more.");
    }
    
    return error;
}

BlazeRpcError ClubsInstance::actionGetAwards()
{
    GetClubAwardsRequest request;
    GetClubAwardsResponse response;
    
    request.setClubId(Random::getRandomNumber(mOwner->mTotalClubs) + 1);
    
    BlazeRpcError error = mClubsProxy->getClubAwards(request, response);
    
    if (error == ERR_OK && response.getClubAwardList().size() != mOwner->mAwardsPerClub)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[actionGetAwards] I got " << response.getClubAwardList().size() << " awards back, but expected " 
                       << mOwner->mAwardsPerClub << " for clubId(" << request.getClubId() << ")");
    }
    
    return error;
}

BlazeRpcError ClubsInstance::actionGetClubRecordbook()
{
    GetClubRecordbookRequest request;
    GetClubRecordbookResponse response;

    request.setClubId(Random::getRandomNumber(mOwner->mTotalClubs) + 1); // club Ids start at 1
            
    BlazeRpcError error = mClubsProxy->getClubRecordbook(request, response);
    if (error != Blaze::ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs, "[actionGetClubRecordbook] Error while fetching clubs: " << (Blaze::ErrorHelp::getErrorName(error)));
    }

    if (mOwner->mRecordsPerClub != (uint32_t)response.getClubRecordList().size())
    {
        BLAZE_ERR_LOG(BlazeRpcLog::clubs, "[actionGetClubRecordbook] Record returned (" << response.getClubRecordList().size() 
                      << "), expected (" << mOwner->mRecordsPerClub << ")");
    }

    return error;
}

BlazeRpcError ClubsInstance::actionCreateClub()
{
    CreateClubRequest request;
    CreateClubResponse response;
    
    eastl::string name;
    uint32_t rndName = Random::getRandomNumber(mOwner->mTotalClubs) + 1 + mOwner->mTotalClubs;
    name.sprintf(mOwner->mPtrnName.c_str(), rndName);
    request.setName(name.c_str());
    
    ClubSettings &clubSettings = request.getClubSettings();

    eastl::string nonUniqueName;
    nonUniqueName.sprintf(mOwner->mPtrnNonUniqueName.c_str(), rndName);          
    clubSettings.setNonUniqueName(nonUniqueName.c_str());
    clubSettings.setJoinAcceptance(Blaze::Clubs::CLUB_ACCEPT_ALL);

    eastl::string desc;
    desc.sprintf(mOwner->mPtrnDesc.c_str(), rndName);
    clubSettings.setDescription(desc.c_str());
    
    eastl::string lang;
    lang.sprintf("%c%c", 
        Random::getRandomNumber(20) + 'A',            
        Random::getRandomNumber(20) + 'A');
    clubSettings.setLanguage(lang.c_str());
    
    clubSettings.setRegion(Random::getRandomNumber(20));
    clubSettings.setSeasonLevel(Random::getRandomNumber(20));
    request.setClubDomainId(mOwner->mDomainIdVector[Random::getRandomNumber(mOwner->mDomainIdVector.size())]);

    /// @todo option to add tags

    BlazeRpcError error = mClubsProxy->createClub(request, response);
    
    if (error == Blaze::CLUBS_ERR_ALREADY_MEMBER || error == Blaze::CLUBS_ERR_MAX_CLUBS)
    {
        ClubId clubId = getRandomClub();
        if (clubId > mOwner->mMaxNoLeaveClubId)
        {    
            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionCreateClub] User is already member. Leaving club.");
            error = actionRemoveMember(clubId);
            
            if (error == Blaze::ERR_OK)
            {
                error = mClubsProxy->createClub(request, response);
                if (error == Blaze::ERR_OK)
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionCreateClub] Successfully created club id is " << response.getClubId());
                    mClubIdSet.insert(response.getClubId());
                }
                else
                {
                    BLAZE_ERR_LOG(BlazeRpcLog::clubs, "[actionCreateClub] Failed to create club with error " 
                                  << (ErrorHelp::getErrorName(error)));
                }
            }
            else
            {
                BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[actionCreateClub] Failed to leave previous club, did not attempt to create club.");
            }
        }
        else
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionCreateClub] My clubid is below MaxNoLeaveClubId. I cannot leave. I am " 
                            << mBlazeId << ". My club is " << clubId << ".");
            error = Blaze::ERR_OK;
                
        }
    }
    
    if (error == Blaze::CLUBS_ERR_CLUB_NAME_IN_USE)
    {
        error = Blaze::ERR_OK;
        BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionCreateClub] Club name is in use.");
    }
    
    if (error == Blaze::CLUBS_ERR_PROFANITY_FILTER)
    {
        error = Blaze::ERR_OK;
        BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionCreateClub] Club name contains profanity");
    }

    return error;    


}

BlazeRpcError ClubsInstance::actionRemoveMember(ClubId clubId)
{
    RemoveMemberRequest request;
    request.setClubId(clubId);
    request.setBlazeId(mBlazeId);
    BlazeRpcError err = mClubsProxy->removeMember(request);

    if (err == Blaze::ERR_OK)
    {
        mClubIdSet.erase(clubId);

        BlazeIdList::iterator iter = mOwner->mMembershipMap[clubId].begin();
        while(iter != mOwner->mMembershipMap[clubId].end())
        {
            if (*iter == mBlazeId)
                break;
            iter++;
        }
        if (iter != mOwner->mMembershipMap[clubId].end())
            mOwner->mMembershipMap[clubId].erase(iter);
        if (mOwner->mMembershipMap[clubId].empty())
            mOwner->mMembershipMap.erase(clubId);
    }
    
    return err;
}

template<class T, class C>
void ClubsInstance::copyList(T *destination, const T *source)
{

    destroyList(destination);
    
    for (typename T::const_iterator it = source->begin();
         it != source->end();
         it++)
    {
        C *item = BLAZE_NEW C();
        (*it)->copyInto(*item);
        destination->push_back(item);
    }
}

template<class T>
void ClubsInstance::destroyList(T *list)
{
    for (typename T::const_iterator it = list->begin();
         it != list->end();
         it++)
    {
        delete (*it);
    }
    
    list->clear();
}

const char8_t* ClubsModule::getRandomLang()
{
    static char8_t language[3];
    language[0] = 'A' + static_cast<char8_t>(Random::getRandomNumber('Z' - 'A' + 1));
    language[1] = 'A' + static_cast<char8_t>(Random::getRandomNumber('Z' - 'A' + 1));
    language[2] = '\0';
    return language;
}

ClubsModule::Action ClubsModule::strToAction(const char8_t* actionStr) const
{
    if (blaze_stricmp(actionStr, ACTION_STR_SIMULATE_PROD) == 0)
        return ACTION_SIMULATE_PRODUCTION;
    else if (blaze_stricmp(actionStr, ACTION_STR_GET_CLUBS) == 0)
        return ACTION_GET_CLUBS;
    else if (blaze_stricmp(actionStr, ACTION_STR_GET_NEWS) == 0)
        return ACTION_GET_NEWS;
    else if (blaze_stricmp(actionStr, ACTION_STR_GET_AWARDS) == 0)
        return ACTION_GET_AWARDS;
    else if (blaze_stricmp(actionStr, ACTION_STR_GET_CLUBRECORDBOOK) == 0)
        return ACTION_GET_CLUBRECORDBOOK;
    else if (blaze_stricmp(actionStr, ACTION_STR_CREATE_CLUB) == 0)
        return ACTION_CREATE_CLUB;
    else if (blaze_stricmp(actionStr, ACTION_STR_FIND_CLUBS) == 0)
        return ACTION_FIND_CLUBS;
    else if (blaze_stricmp(actionStr, ACTION_STR_FIND_CLUBS_BY_TAGS_ALL) == 0)
        return ACTION_FIND_CLUBS_BY_TAGS_ALL;
    else if (blaze_stricmp(actionStr, ACTION_STR_FIND_CLUBS_BY_TAGS_ANY) == 0)
        return ACTION_FIND_CLUBS_BY_TAGS_ANY;
    else if (blaze_stricmp(actionStr, ACTION_STR_FIND_CLUBS2) == 0)
        return ACTION_FIND_CLUBS2;
    else if (blaze_stricmp(actionStr, ACTION_STR_FIND_CLUBS2_BY_TAGS_ALL) == 0)
        return ACTION_FIND_CLUBS2_BY_TAGS_ALL;
    else if (blaze_stricmp(actionStr, ACTION_STR_FIND_CLUBS2_BY_TAGS_ANY) == 0)
        return ACTION_FIND_CLUBS2_BY_TAGS_ANY;
    else if (blaze_stricmp(actionStr, ACTION_STR_PLAY_CLUB_GAME) == 0)
        return ACTION_PLAY_CLUB_GAME;
    else if (blaze_stricmp(actionStr, ACTION_STR_JOIN_CLUB) == 0)
        return ACTION_JOIN_CLUB;
    else if (blaze_stricmp(actionStr, ACTION_STR_JOIN_OR_PETITION_CLUB) == 0)
        return ACTION_JOIN_OR_PETITION_CLUB;
    else if (blaze_stricmp(actionStr, ACTION_STR_GET_MEMBERS) == 0)
        return ACTION_GET_MEMBERS;
    else if (blaze_stricmp(actionStr, ACTION_STR_GET_CLUBS_COMPONENT_SETTINGS) == 0)
        return ACTION_GET_CLUBS_COMPONENT_SETTINGS;
    else if (blaze_stricmp(actionStr, ACTION_STR_GET_CLUB_MEMBERSHIP_FOR_USERS) == 0)
        return ACTION_GET_CLUB_MEMBERSHIP_FOR_USERS;
    else if (blaze_stricmp(actionStr, ACTION_STR_POST_NEWS) == 0)
        return ACTION_POST_NEWS;
    else if (blaze_stricmp(actionStr, ACTION_STR_PETITION) == 0)
        return ACTION_PETITION;
    else if (blaze_stricmp(actionStr, ACTION_STR_UPDATE_ONLINE_STATUS) == 0)
        return ACTION_UPDATE_ONLINE_STATUS;
    else if (blaze_stricmp(actionStr, ACTION_STR_UPDATE_MEMBER_METADATA) == 0)
        return ACTION_UPDATE_MEMBER_METADATA;
    else if (blaze_stricmp(actionStr, ACTION_STR_SET_METADATA) == 0)
        return ACTION_SET_METADATA;
    else if (blaze_stricmp(actionStr, ACTION_STR_UPDATE_CLUB_SETTINGS) == 0)
        return ACTION_UPDATE_CLUB_SETTINGS;
    else
        return ACTION_MAX_ENUM;
}

const char8_t* ClubsModule::actionToStr(Action action) const
{
    switch(action)
    {
        case ACTION_SIMULATE_PRODUCTION:
            return ACTION_STR_SIMULATE_PROD;
        case ACTION_GET_CLUBS:
            return ACTION_STR_GET_CLUBS;
        case ACTION_GET_NEWS:
            return ACTION_STR_GET_NEWS;
        case ACTION_GET_AWARDS:
            return ACTION_STR_GET_AWARDS;
        case ACTION_GET_CLUBRECORDBOOK:
            return ACTION_STR_GET_CLUBRECORDBOOK;
        case ACTION_CREATE_CLUB:
            return ACTION_STR_CREATE_CLUB;
        case ACTION_FIND_CLUBS:
            return ACTION_STR_FIND_CLUBS;
        case ACTION_FIND_CLUBS_BY_TAGS_ALL:
            return ACTION_STR_FIND_CLUBS_BY_TAGS_ALL;
        case ACTION_FIND_CLUBS_BY_TAGS_ANY:
            return ACTION_STR_FIND_CLUBS_BY_TAGS_ANY;
        case ACTION_FIND_CLUBS2:
            return ACTION_STR_FIND_CLUBS2;
        case ACTION_FIND_CLUBS2_BY_TAGS_ALL:
            return ACTION_STR_FIND_CLUBS2_BY_TAGS_ALL;
        case ACTION_FIND_CLUBS2_BY_TAGS_ANY:
            return ACTION_STR_FIND_CLUBS2_BY_TAGS_ANY;
        case ACTION_PLAY_CLUB_GAME:
            return ACTION_STR_PLAY_CLUB_GAME;
        case ACTION_JOIN_CLUB:
            return ACTION_STR_JOIN_CLUB;
        case ACTION_JOIN_OR_PETITION_CLUB:
            return ACTION_STR_JOIN_OR_PETITION_CLUB;
        case ACTION_GET_MEMBERS:
            return ACTION_STR_GET_MEMBERS;
        case ACTION_GET_CLUBS_COMPONENT_SETTINGS:
            return ACTION_STR_GET_CLUBS_COMPONENT_SETTINGS;
        case ACTION_GET_CLUB_MEMBERSHIP_FOR_USERS:
            return ACTION_STR_GET_CLUB_MEMBERSHIP_FOR_USERS;
        case ACTION_POST_NEWS:
            return ACTION_STR_POST_NEWS;
        case ACTION_PETITION:
            return ACTION_STR_PETITION;
        case ACTION_UPDATE_ONLINE_STATUS:
            return ACTION_STR_UPDATE_ONLINE_STATUS;
        case ACTION_UPDATE_MEMBER_METADATA:
            return ACTION_STR_UPDATE_MEMBER_METADATA;
        case ACTION_SET_METADATA:
            return ACTION_STR_SET_METADATA;
        case ACTION_UPDATE_CLUB_SETTINGS:
            return ACTION_STR_UPDATE_CLUB_SETTINGS;
        default:
            return nullptr;
    }
 }

const char8_t* ClubsModule::actionToContext(Action action) const
{
    switch(action)
    {
        case ACTION_SIMULATE_PRODUCTION:
            return CONTEXT_STR_SIMULATE_PROD;
        case ACTION_GET_CLUBS:
            return CONTEXT_STR_GET_CLUBS;
        case ACTION_GET_NEWS:
            return CONTEXT_STR_GET_NEWS;
        case ACTION_GET_AWARDS:
            return CONTEXT_STR_GET_AWARDS;
        case ACTION_GET_CLUBRECORDBOOK:
            return CONTEXT_STR_GET_CLUBRECORDBOOK;
        case ACTION_CREATE_CLUB:
            return CONTEXT_STR_CREATE_CLUB;
        case ACTION_FIND_CLUBS:
            return CONTEXT_STR_FIND_CLUBS;
        case ACTION_FIND_CLUBS_BY_TAGS_ALL:
            return CONTEXT_STR_FIND_CLUBS_BY_TAGS_ALL;
        case ACTION_FIND_CLUBS_BY_TAGS_ANY:
            return CONTEXT_STR_FIND_CLUBS_BY_TAGS_ANY;
        case ACTION_FIND_CLUBS2:
            return CONTEXT_STR_FIND_CLUBS2;
        case ACTION_FIND_CLUBS2_BY_TAGS_ALL:
            return CONTEXT_STR_FIND_CLUBS2_BY_TAGS_ALL;
        case ACTION_FIND_CLUBS2_BY_TAGS_ANY:
            return CONTEXT_STR_FIND_CLUBS2_BY_TAGS_ANY;
        case ACTION_PLAY_CLUB_GAME:
            return CONTEXT_STR_PLAY_CLUB_GAME;
        case ACTION_JOIN_CLUB:
            return CONTEXT_STR_JOIN_CLUB;
        case ACTION_JOIN_OR_PETITION_CLUB:
            return CONTEXT_STR_JOIN_OR_PETITION_CLUB;
        case ACTION_GET_MEMBERS:
            return CONTEXT_STR_GET_MEMBERS;
        case ACTION_GET_CLUBS_COMPONENT_SETTINGS:
            return CONTEXT_STR_GET_CLUBS_COMPONENT_SETTINGS;
        case ACTION_GET_CLUB_MEMBERSHIP_FOR_USERS:
            return CONTEXT_STR_GET_CLUB_MEMBERSHIP_FOR_USERS;
        case ACTION_POST_NEWS:
            return CONTEXT_STR_POST_NEWS;
        case ACTION_PETITION:
            return CONTEXT_STR_PETITION;
        case ACTION_UPDATE_ONLINE_STATUS:
            return CONTEXT_STR_UPDATE_ONLINE_STATUS;
        case ACTION_UPDATE_MEMBER_METADATA:
            return CONTEXT_STR_UPDATE_MEMBER_METADATA;
        case ACTION_SET_METADATA:
            return CONTEXT_STR_SET_METADATA;
        case ACTION_UPDATE_CLUB_SETTINGS:
            return CONTEXT_STR_UPDATE_CLUB_SETTINGS;
        default:
            return nullptr;
    }
 }

BlazeRpcError ClubsInstance::setValuesForFindClubsRequest(FindClubsRequest& request)
{
    request.setMaxResultCount(mOwner->mFindClubsMaxResultCount);
    bool moreSelectiveFilteringCriteriaUsed = false;
    
    if (tossTheDice(mOwner->mFindClubsByName))
    {
        eastl::string clubName;        
        clubName.sprintf(mOwner->mPtrnName.c_str(), Random::getRandomNumber(mOwner->mTotalClubs));
        // the way data is populated in database, the query becomes expensive if we use filtering string which is more than 3 caracters shorter than the
        // club name. All names are generated from club%05x' or similar. In production names will be more dispersed so this should be enough to test it
        clubName = clubName.substr(0, static_cast<uint32_t>(clubName.length()) - Random::getRandomNumber(eastl::min(static_cast<int32_t>(clubName.length()), 3)));
        request.setName(clubName.c_str());
        moreSelectiveFilteringCriteriaUsed = true;
    }

    if (tossTheDice(mOwner->mFindClubsByNonUniqueName))
    {
        eastl::string clubNonUniqueName;        
        clubNonUniqueName.sprintf(mOwner->mPtrnNonUniqueName.c_str(), Random::getRandomNumber(mOwner->mTotalClubs));
        // see comment above for filtering by name
        clubNonUniqueName = clubNonUniqueName.substr(0, static_cast<uint32_t>(clubNonUniqueName.length()) - Random::getRandomNumber(eastl::min(static_cast<int32_t>(clubNonUniqueName.length()), 3)));
        request.setNonUniqueName(clubNonUniqueName.c_str());
        moreSelectiveFilteringCriteriaUsed = true;
    }

    if (tossTheDice(mOwner->mFindClubsByLanguage))
    {
        request.setLanguage(mOwner->getRandomLang());
    }

    if (tossTheDice(mOwner->mFindClubsByTeamId))
    {
        request.setTeamId(Random::getRandomNumber(mOwner->mMaxTeams));
        request.setAnyTeamId(false);
    }

    if (tossTheDice(mOwner->mFindClubsByAcceptanceFlags))
    {
        request.getAcceptanceFlags().setBits(Random::getRandomNumber(8));
        request.getAcceptanceMask().setBits(Random::getRandomNumber(8));
    }    
    
    if (tossTheDice(mOwner->mFindClubsBySeasonLevel))
    {
        request.setSeasonLevel(Random::getRandomNumber(mOwner->mMaxSeasonLevels));
    }
    
    if (tossTheDice(mOwner->mFindClubsByRegion))
    {
        request.setSeasonLevel(Random::getRandomNumber(mOwner->mMaxRegions));
    }

    if (tossTheDice(mOwner->mFindClubsByMinMemberCount))
    {
        request.setMinMemberCount(Random::getRandomNumber(mOwner->mMaxUsersPerClub));
    }

    if (tossTheDice(mOwner->mFindClubsByMaxMemberCount))
    {
        request.setMaxMemberCount(Random::getRandomNumber(mOwner->mMaxUsersPerClub));
    }

    if (tossTheDice(mOwner->mFindClubsByClubFilterList))
    {
        uint32_t clubs = Random::getRandomNumber(20);
        for (uint32_t i = 0; i < clubs; i++)
        {
            request.getClubFilterList().push_back(Random::getRandomNumber(mOwner->mTotalClubs));
        }
        moreSelectiveFilteringCriteriaUsed = true;
    }

    if (tossTheDice(mOwner->mFindClubsByMemberFilterList))
    {
        uint32_t users = Random::getRandomNumber(20);
        for (uint32_t i = 0; i < users; i++)
        {
            request.getMemberFilterList().push_back(Random::getRandomNumber(mOwner->mTotalUsers));
        }
        moreSelectiveFilteringCriteriaUsed = true;
    }

    if (tossTheDice(mOwner->mFindClubsSkipMetadata))
    {
        request.setSkipMetadata(1);
    }
   
    if (tossTheDice(mOwner->mFindClubsByPassword))
    {
        uint32_t passwordOpt = Random::getRandomNumber(3);
        if (passwordOpt == 0)
            request.setPasswordOption(CLUB_PASSWORD_IGNORE);
        else if (passwordOpt == 1)
            request.setPasswordOption(CLUB_PASSWORD_NOT_PROTECTED);
        else
            request.setPasswordOption(CLUB_PASSWORD_PROTECTED);
    }
   
    if (tossTheDice(mOwner->mFindClubsByDomain))
    { 
        request.setAnyDomain(false);
        request.setClubDomainId(mOwner->mDomainIdVector[Random::getRandomNumber(mOwner->mDomainIdVector.size())]);
    }
    else
    {
        request.setAnyDomain(true);
    }

    // include sorting only if more selective criteria is used - otherwise it's going to be expensive search
    if (moreSelectiveFilteringCriteriaUsed && tossTheDice(mOwner->mFindClubsOrdered))
    {
        if (Random::getRandomNumber(2) == 0)
            request.setClubsOrder(CLUBS_ORDER_BY_NAME);
        else
            request.setClubsOrder(CLUBS_ORDER_BY_CREATIONTIME);

        if (Random::getRandomNumber(2) == 0)
            request.setOrderMode(ASC_ORDER);
        else
            request.setOrderMode(DESC_ORDER);
    }
    else
    {
        request.setClubsOrder(CLUBS_NO_ORDER);
    }

    return Blaze::ERR_OK; 
}

BlazeRpcError ClubsInstance::setRequestForFindClubs(FindClubsRequest& request)
{
    BlazeRpcError error = setValuesForFindClubsRequest(request);
    if (error != Blaze::ERR_OK)
        return error;

    if (tossTheDice(mOwner->mFindClubsByMemberOnlineStatusCounts))
    {
        int statusNum = Random::getRandomNumber(CLUBS_MEMBER_ONLINE_STATUS_COUNT);
        if (statusNum == 0)
            return Blaze::ERR_SYSTEM;
        bool validStatusCount = false;
        for (int i = 0; i < statusNum; i++)
        {
            Blaze::Clubs::MemberOnlineStatus status = ClubsModule::getMemberOnlineStatusFromIndex(Random::getRandomNumber(CLUBS_MEMBER_ONLINE_STATUS_COUNT));
            uint16_t memberCount = static_cast<uint16_t>(Random::getRandomNumber(mOwner->mMaxUsersPerClub));
            if (memberCount > 0)
            {
                validStatusCount = true;
                request.getMinMemberOnlineStatusCounts().insert(eastl::make_pair(status, memberCount));
            }
        }
        if (!validStatusCount)
            return Blaze::ERR_SYSTEM;
    }
    
    return Blaze::ERR_OK;
}

BlazeRpcError ClubsInstance::setRequestForFindClubs2(Blaze::Clubs::FindClubs2Request& request)
{
    BlazeRpcError error = setValuesForFindClubsRequest(request.getParams());
    if (error != Blaze::ERR_OK)
        return error;
    
    if (tossTheDice(mOwner->mFindClubsByMemberOnlineStatusSum))
    {
        int statusNum = Random::getRandomNumber(CLUBS_MEMBER_ONLINE_STATUS_COUNT);
        if (statusNum == 0)
            return Blaze::ERR_SYSTEM;
        uint32_t sum = 0;
        for (int i = 0; i < statusNum; i++)
        {
            Blaze::Clubs::MemberOnlineStatus status = ClubsModule::getMemberOnlineStatusFromIndex(Random::getRandomNumber(CLUBS_MEMBER_ONLINE_STATUS_COUNT));
            request.getMemberOnlineStatusFilter().push_back(status);
            sum += static_cast<uint32_t>(Random::getRandomNumber(mOwner->mMaxUsersPerClub));
        }
        if (sum == 0)
            return Blaze::ERR_SYSTEM;
        request.setMemberOnlineStatusSum(sum);
    }

    return Blaze::ERR_OK;
}

BlazeRpcError ClubsInstance::actionFindClubs()
{
    FindClubsRequest request;
    if (setRequestForFindClubs(request) != Blaze::ERR_OK)
        return Blaze::ERR_OK;

    FindClubsResponse response;
    BlazeRpcError result = mClubsProxy->findClubs(request, response);
    
    if (result == Blaze::ERR_OK)
        response.getClubList().copyInto(mClubList);// efficient copy, only 2 allocations max
    
    return result;
}

BlazeRpcError ClubsInstance::actionFindClubs2()
{
    FindClubs2Request request;
    if (setRequestForFindClubs2(request) != Blaze::ERR_OK)
        return Blaze::ERR_OK;

    FindClubsResponse response;
    BlazeRpcError result = mClubsProxy->findClubs2(request, response);
    
    if (result == Blaze::ERR_OK)
        response.getClubList().copyInto(mClubList);// efficient copy, only 2 allocations max
    
    return result;
}

BlazeRpcError ClubsInstance::setRequestForFindClubsByTagsAll(FindClubsRequest& request)
{
    if (mOwner->mTagMap.empty())
    {
        // nothing to do
        return Blaze::ERR_SYSTEM;
    }

    // get some random tag IDs
    ClubsModule::TagIdList tagIdList;
    if (mOwner->mTagsToGet == 0)
    {
        mOwner->pickRandomTagIds(tagIdList);
    }
    else
    {
        mOwner->pickRandomTagIds(mOwner->mTagsToGet, tagIdList);
    }

    if (tagIdList.empty())
    {
        // nothing to do
        BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[ClubsInstance].setRequestForFindClubsByTagsAll: Failed to obtain any tags");
        return Blaze::ERR_SYSTEM;
    }

    request.setMaxResultCount(mOwner->mFindClubsMaxResultCount);

    request.setTagSearchOperation(CLUB_TAG_SEARCH_MATCH_ALL);

    ClubsModule::TagIdList::const_iterator tagIdItr = tagIdList.begin();
    ClubsModule::TagIdList::const_iterator tagIdEnd = tagIdList.end();
    for (; tagIdItr != tagIdEnd; ++tagIdItr)
    {
        request.getTagList().push_back(mOwner->mTagMap[*tagIdItr].c_str());
    }

    return Blaze::ERR_OK;
}

BlazeRpcError ClubsInstance::actionFindClubsByTagsAll()
{
    FindClubsRequest request;
    if (setRequestForFindClubsByTagsAll(request) != Blaze::ERR_OK)
        return Blaze::ERR_OK;

    FindClubsResponse response;
    BlazeRpcError result = mClubsProxy->findClubs(request, response);

    if (result == Blaze::ERR_OK)
        response.getClubList().copyInto(mClubList);// efficient copy, only 2 allocations max

    return result;
}

BlazeRpcError ClubsInstance::actionFindClubs2ByTagsAll()
{
    FindClubs2Request request;
    if (setRequestForFindClubsByTagsAll(request.getParams()) != Blaze::ERR_OK)
        return Blaze::ERR_OK;

    FindClubsResponse response;
    BlazeRpcError result = mClubsProxy->findClubs2(request, response);

    if (result == Blaze::ERR_OK)
        response.getClubList().copyInto(mClubList);// efficient copy, only 2 allocations max

    return result;
}

BlazeRpcError ClubsInstance::setRequestForFindClubsByTagsAny(FindClubsRequest& request)
{
    if (mOwner->mTagMap.empty())
    {
        // nothing to do
        return Blaze::ERR_SYSTEM;
    }

    // get some random tag IDs
    ClubsModule::TagIdList tagIdList;
    if (mOwner->mTagsToGet == 0)
    {
        mOwner->pickRandomTagIds(tagIdList);
    }
    else
    {
        mOwner->pickRandomTagIds(mOwner->mTagsToGet, tagIdList);
    }

    if (tagIdList.empty())
    {
        // nothing to do
        BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[ClubsInstance].setRequestForFindClubsByTagsAny: Failed to obtain any tags");
        return Blaze::ERR_SYSTEM;
    }

    request.setMaxResultCount(mOwner->mFindClubsMaxResultCount);

    request.setTagSearchOperation(CLUB_TAG_SEARCH_MATCH_ANY);

    ClubsModule::TagIdList::const_iterator tagIdItr = tagIdList.begin();
    ClubsModule::TagIdList::const_iterator tagIdEnd = tagIdList.end();
    for (; tagIdItr != tagIdEnd; ++tagIdItr)
    {
        request.getTagList().push_back(mOwner->mTagMap[*tagIdItr].c_str());
    }

    return Blaze::ERR_OK;
}

BlazeRpcError ClubsInstance::actionFindClubsByTagsAny()
{
    FindClubsRequest request;
    if (setRequestForFindClubsByTagsAny(request) != Blaze::ERR_OK)
        return Blaze::ERR_OK;

    FindClubsResponse response;
    BlazeRpcError result = mClubsProxy->findClubs(request, response);

    if (result == Blaze::ERR_OK)
        response.getClubList().copyInto(mClubList);// efficient copy, only 2 allocations max

    return result;
}

BlazeRpcError ClubsInstance::actionFindClubs2ByTagsAny()
{
    FindClubs2Request request;
    if (setRequestForFindClubsByTagsAny(request.getParams()) != Blaze::ERR_OK)
        return Blaze::ERR_OK;

    FindClubsResponse response;
    BlazeRpcError result = mClubsProxy->findClubs2(request, response);

    if (result == Blaze::ERR_OK)
        response.getClubList().copyInto(mClubList);// efficient copy, only 2 allocations max

    return result;
}

BlazeRpcError ClubsInstance::actionPlayClubGame()
{  
    BlazeRpcError result = Blaze::ERR_OK;
    ClubId clubId = getRandomClub();
    if (clubId == 0)
    {
        if ((result = actionCreateClub()) != Blaze::ERR_OK)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[ClubsInstance].actionPlayClubGame: Failed to create club.");
            return result;
        }
        clubId = getRandomClub();    
        if (clubId == 0)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[ClubsInstance].actionPlayClubGame: Club ID did not arrive in time.");
            return result;
        }
    }

    return Blaze::ERR_OK;
}

BlazeRpcError ClubsInstance::actionJoinClub()
{
    JoinClubRequest request;
    BlazeRpcError error;
    
    if (mClubList.size() > 0)
    {
        request.setClubId((*mClubList.begin())->getClubId());
        request.setPassword(mOwner->mPassword.c_str());
        error = mClubsProxy->joinClub(request);
    }
    else
    {
        int joinClubCount = 0;
        do
        {
            request.setClubId(Random::getRandomNumber(mOwner->mTotalClubs) + 1); 
            request.setPassword(mOwner->mPassword.c_str());   
            error = mClubsProxy->joinClub(request);
            ++joinClubCount;
            //give up joining if we try 5 times
        } while (error == Blaze::CLUBS_ERR_INVALID_CLUB_ID && joinClubCount < 6);
    }
    
    
    if (error == Blaze::CLUBS_ERR_ALREADY_MEMBER || error == Blaze::CLUBS_ERR_MAX_CLUBS)
    {
        ClubId clubId = request.getClubId();
        if (error != Blaze::CLUBS_ERR_ALREADY_MEMBER)
            clubId = getRandomClub();
        if (clubId > mOwner->mMaxNoLeaveClubId)
        {    
            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionJoinClub] User is already member. Leaving club.");
            error = actionRemoveMember(clubId);
            if (error == Blaze::ERR_OK)
            {
                error = mClubsProxy->joinClub(request);
            }
            else
            {
                BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[actionJoinClub] Failed to leave previous club, did not attempt to join club.");
            }
        }
        else
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionJoinClub] My clubid is below MaxNoLeaveClubId. I cannot leave. I am " 
                            << mBlazeId << ". My club is " << clubId << ".");
            error = Blaze::ERR_OK;
                
        }
    }
    
    if (error == Blaze::CLUBS_ERR_LAST_GM_CANNOT_LEAVE)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionJoinClub] Last GM cannot leave.");
        error = Blaze::ERR_OK;
    }    
    
    return error;
}

BlazeRpcError ClubsInstance::actionJoinOrPetitionClub()
{
    JoinOrPetitionClubRequest request;
    JoinOrPetitionClubResponse response;
    BlazeRpcError error;
    
    if (mClubList.size() > 0)
    {
        request.setClubId((*mClubList.begin())->getClubId());
        request.setPassword(mOwner->mPassword.c_str());
        if (request.getClubId() % 2 == 0)
            request.setPetitionIfJoinFails(true);
        else
            request.setPetitionIfJoinFails(false);
        error = mClubsProxy->joinOrPetitionClub(request, response);
    }
    else
    {
        int joinClubCount = 0;
        do
        {
            request.setClubId(Random::getRandomNumber(mOwner->mTotalClubs) + 1); 
            request.setPassword(mOwner->mPassword.c_str()); 
            if (request.getClubId() % 2 == 0)
                request.setPetitionIfJoinFails(true);
            else
                request.setPetitionIfJoinFails(false);
            error = mClubsProxy->joinOrPetitionClub(request, response);
            ++joinClubCount;
            //give up joining if we try 5 times
        } while (error == Blaze::CLUBS_ERR_INVALID_CLUB_ID && joinClubCount < 6);
    }

    if (error == Blaze::CLUBS_ERR_ALREADY_MEMBER || error == Blaze::CLUBS_ERR_MAX_CLUBS)
    {
        ClubId clubId = request.getClubId();
        if (error != Blaze::CLUBS_ERR_ALREADY_MEMBER)
            clubId = getRandomClub();
        if (clubId > mOwner->mMaxNoLeaveClubId)
        {    
            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionJoinOrPetitionClub] User is already member. Leaving club.");
            error = actionRemoveMember(clubId);
            if (error == Blaze::ERR_OK)
            {
                error = mClubsProxy->joinOrPetitionClub(request, response);
            }
            else
            {
                BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[actionJoinOrPetitionClub] Failed to leave previous club, did not attempt to join club.");
            }
        }
        else
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionJoinOrPetitionClub] My clubid is below MaxNoLeaveClubId. I cannot leave. I am " 
                            << mBlazeId << ". My club is " << clubId << ".");
            error = Blaze::ERR_OK;
                
        }
    }
    else if (error == Blaze::CLUBS_ERR_PETITION_ALREADY_SENT)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionJoinOrPetitionClub] I have already sent petition to club " << request.getClubId() << " "
                  "I am " << mBlazeId << ".");
        error = Blaze::ERR_OK;
    }
    else if (error == Blaze::CLUBS_ERR_MAX_PETITIONS_RECEIVED)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionJoinOrPetitionClub] The number of petitions in the club " << request.getClubId() << " is maxed out. "
                  "I am " << mBlazeId << ".");
        error = Blaze::ERR_OK;
    }
    else if (error == Blaze::CLUBS_ERR_MAX_PETITIONS_SENT)
    {
        // I maxed out my petitions. Revoke them all!
        GetPetitionsRequest req;
        GetPetitionsResponse resp;
        
        req.setClubId(request.getClubId());
        req.setPetitionsType(CLUBS_PETITION_SENT_BY_ME);
        error = mClubsProxy->getPetitions(req, resp);
        mOwner->addTransactionTime(0);
        
        if (error == Blaze::ERR_OK)
        {
            ClubMessageList &msgList = resp.getClubPetitionsList();
            for (
                ClubMessageList::const_iterator itml = msgList.begin(); 
                itml != msgList.end() && error == Blaze::ERR_OK; 
                itml++)
            {
                ProcessPetitionRequest preq;
                preq.setPetitionId((*itml)->getMessageId());
                error = mClubsProxy->revokePetition(preq);
                mOwner->addTransactionTime(0);
                if (error == Blaze::CLUBS_ERR_INVALID_ARGUMENT)
                {
                    BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[actionJoinOrPetitionClub] I could not revoke petition " << preq.getPetitionId() << " "
                              "I am " << mBlazeId << ".");
                    error = ERR_OK;
                }
            }
        }
    }
    
    if (error == Blaze::CLUBS_ERR_LAST_GM_CANNOT_LEAVE)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionJoinOrPetitionClub] Last GM cannot leave.");
        error = Blaze::ERR_OK;
    }    
    
    return error;
}

BlazeRpcError ClubsInstance::actionGetMembers()
{
    GetMembersRequest request;
    GetMembersResponse response;

    // Some calls should always return at least one member, but others may legitimately not,
    // so we will only validate and warn on an empty response in those cases where we should
    // get a member back 100% of the time
    bool expectMembers = true;
    
    request.setClubId(Random::getRandomNumber(mOwner->mTotalClubs) + 1);    
    request.setMaxResultCount(100);
    
    if (tossTheDice(mOwner->mGetMembersOrdered))
    {
        request.setOrderType(MEMBER_ORDER_BY_JOIN_TIME);

        if (Random::getRandomNumber(2) == 0)
            request.setOrderMode(ASC_ORDER);
        else
            request.setOrderMode(DESC_ORDER);
    }
    else
    {
        request.setOrderType(MEMBER_NO_ORDER);
    }

    if (tossTheDice(mOwner->mGetMembersFiltered))
    {
        if (Random::getRandomNumber(2) == 0)
            request.setMemberType(GM_MEMBERS);
        else
        {
            expectMembers = false;
            request.setMemberType(NON_GM_MEMBERS);
        }
    }
    else
    {
        request.setMemberType(ALL_MEMBERS);
    }

    BlazeRpcError error = mClubsProxy->getMembers(request, response);
    if (error == ERR_OK && expectMembers && response.getClubMemberList().size() == 0)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[actionGetMembers] I got 0 members back, but expected more for club " 
                       << request.getClubId() << ".");
    }
    
    return error;
}

BlazeRpcError ClubsInstance::actionGetClubsComponentSettings()
{
    ClubsComponentSettings response;
    BlazeRpcError error = mClubsProxy->getClubsComponentSettings(response);
    
    return error;
}

BlazeRpcError ClubsInstance::actionGetClubMembershipForUsers()
{
    GetClubMembershipForUsersRequest request;
    GetClubMembershipForUsersResponse response;
    BlazeIdList &idList = request.getBlazeIdList();

    uint32_t clubMembersToGet = Random::getRandomNumber(mOwner->mClubMembershipsToGet) + 1;
    uint32_t nonClubMembersToGet = mOwner->mClubMembershipsToGet - clubMembersToGet;

    for (uint32_t i = 0; i < clubMembersToGet; i++)
    {
        while (true)
        {
            uint32_t id = Random::getRandomNumber(mOwner->mTotalClubMembers) + 1 ;
            if (eastl::find(idList.begin(), idList.end(), id) == idList.end())
            {
                idList.push_back(id);
                break;
            }
        }
    }

    for (uint32_t i = 0; i < nonClubMembersToGet; i++)
    {
        while (true)
        {
            uint32_t id = Random::getRandomNumber(mOwner->mTotalUsers - mOwner->mTotalClubMembers) + 1 + mOwner->mTotalClubMembers;
            if (eastl::find(idList.begin(), idList.end(), id) == idList.end())
            {
                idList.push_back(id);
                break;
            }
        }
    }
    
    BlazeRpcError error = mClubsProxy->getClubMembershipForUsers(request, response);
    
    if (error == ERR_OK)
    {
        uint32_t clubMemberResults = 0;
        for (ClubMembershipMap::const_iterator it = response.getMembershipMap().begin(); 
             it != response.getMembershipMap().end();
             it++)
        {
            clubMemberResults++;
        }
        if(clubMemberResults != clubMembersToGet)
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionGetClubMemberhipForUsers] I got " << clubMemberResults 
                            << " results back, but expected " << mOwner->mClubMembershipsToGet << ".");
        }
    }
    
    return error;
}

BlazeRpcError ClubsInstance::actionPostNews()
{
    BlazeRpcError err = Blaze::ERR_OK;
    static unsigned int newsCount = 0;

    ClubId clubId = getRandomClub();
    if (clubId == 0)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[actionPostNews] I am not a club member.");
    }
    else
    {
        PostNewsRequest req;
        req.setClubId(clubId);
        ClubNews &news = req.getClubNews();
        news.getUser().setBlazeId(mBlazeId);

        eastl::string newsString;
        newsString.sprintf(mOwner->mPtrnNewsBody.c_str(), newsCount++);
        eastl::string txt;
        txt.sprintf(" User: %" PRId64 ", Club: %d", mBlazeId, clubId);
        newsString.append(txt);
        news.setText(newsString.c_str());
        
        news.setType(CLUBS_MEMBER_POSTED_NEWS);
        
        err = mClubsProxy->postNews(req);
    }
        
    return err;
}

BlazeRpcError ClubsInstance::actionPetition()
{

    BlazeRpcError err = Blaze::ERR_OK;
    bool bFilePetition = false;

    ClubId clubId = getRandomClub();
    if (clubId == 0)
    {
        // I am not a club member, so file petition
        bFilePetition = true;
    }
    else
    {
        // I am a club member, but am I GM?
        GetClubMembershipForUsersRequest mreq;
        GetClubMembershipForUsersResponse mresp;
        
        mreq.getBlazeIdList().push_back(mBlazeId);
        err = mClubsProxy->getClubMembershipForUsers(mreq, mresp);
        mOwner->addTransactionTime(0);
        
        if (err == Blaze::ERR_OK)
        {
            ClubMembershipMap::const_iterator it = mresp.getMembershipMap().find(mBlazeId);
            if (it == mresp.getMembershipMap().end())
            {
                BLAZE_ERR_LOG(BlazeRpcLog::clubs, "[actionPetition] Could not find myself in membership map! "
                          "I am " << mBlazeId << " and my club is " << clubId << ".");
                return ERR_SYSTEM;
            }

            ClubMembershipList &membershipList = it->second->getClubMembershipList();
            ClubMembershipList::const_iterator cmlIter = membershipList.begin();
            for ( ; cmlIter != membershipList.end(); ++cmlIter)
            {
                if (clubId == (*cmlIter)->getClubId())
                    break;
            }
            if (cmlIter == membershipList.end())
            {
                BLAZE_ERR_LOG(BlazeRpcLog::clubs, "[actionPetition] Could not find myself in membership list! "
                          "I am " << mBlazeId << " and my club id is " << clubId << ".");
                return ERR_SYSTEM;
            }
            MembershipStatus status = (*cmlIter)->getClubMember().getMembershipStatus();
            if (status == CLUBS_OWNER || status == CLUBS_GM)
            {
                // I am GM, so accept all pending petitions   
                // Is my clubId above mMaxNoLeaveClubId?
                
                if (clubId > mOwner->mMaxNoLeaveClubId)
                {
                    // it is, leave the club!
                    err = actionRemoveMember(clubId);
                    mOwner->addTransactionTime(0);
                    
                    if (err == Blaze::CLUBS_ERR_LAST_GM_CANNOT_LEAVE)
                    {
                        BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionPetition] I could not leave club " << clubId << "."
                                  " because I am last GM.  I am " << mBlazeId << ".");
                        err = Blaze::ERR_OK;
                    }

                    return err;
                }
                
                GetPetitionsRequest req;
                GetPetitionsResponse resp;
                
                req.setClubId(clubId);
                req.setPetitionsType(CLUBS_PETITION_SENT_TO_CLUB);
                err = mClubsProxy->getPetitions(req, resp);
                mOwner->addTransactionTime(0);
                
                if (err == Blaze::ERR_OK)
                {
                    ClubMessageList &msgList = resp.getClubPetitionsList();
                    for (
                        ClubMessageList::const_iterator itml = msgList.begin(); 
                        itml != msgList.end() && err == Blaze::ERR_OK; 
                        itml++)
                    {
                        ProcessPetitionRequest preq;
                        preq.setPetitionId((*itml)->getMessageId());
                        err = mClubsProxy->acceptPetition(preq);
                        mOwner->addTransactionTime(0);
                        if (err == Blaze::CLUBS_ERR_INVALID_ARGUMENT)
                        {
                            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionPetition] I could not accept petition " << preq.getPetitionId() << " to club " << clubId 
                                            << ": CLUBS_ERR_INVALID_ARGUMENT, message was probably deleted. I am " << mBlazeId << ".");
                            err = Blaze::ERR_OK;
                        }
                        else if (err == Blaze::CLUBS_ERR_ALREADY_MEMBER || err == Blaze::CLUBS_ERR_MAX_CLUBS)
                        {
                            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionPetition] I could not accept petition " << preq.getPetitionId() << " to club " 
                                           << clubId << " because user became a member of another club.  I am " << mBlazeId << ".");
                            err = Blaze::ERR_OK;
                        }
                    }
                }
            }
            else
            {
                // I am not GM, so leave the club and send petition to another one
                // talking about loyalty...
                err = actionRemoveMember(clubId);
                mOwner->addTransactionTime(0);
                if (err == Blaze::ERR_OK)
                {
                    bFilePetition = true;
                }
                else
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionPetition] Could not leave club! "
                              "I am " << mBlazeId << " and my club is " << clubId << ".");
                }
            }
        }
    }
    
    if (bFilePetition)
    {
        SendPetitionRequest request;
        request.setClubId(Random::getRandomNumber(mOwner->mMaxNoLeaveClubId - 1) + 1);
        
        // this transaction will be counted by framework (no addTransactionTime here)
        err = mClubsProxy->sendPetition(request);
        
        if (err == Blaze::CLUBS_ERR_PETITION_ALREADY_SENT)
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionPetition] I have already filed petition to club " << request.getClubId() << " "
                      "I am " << mBlazeId << ".");
            err = Blaze::ERR_OK;
        }
        else if (err == Blaze::CLUBS_ERR_MAX_PETITIONS_RECEIVED)
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionPetition] The number of petitions in the club " << request.getClubId() << " is maxed out. "
                      "I am " << mBlazeId << ".");
            err = Blaze::ERR_OK;
        }
        else if (err == Blaze::CLUBS_ERR_ALREADY_MEMBER)
        {
            if (clubId != 0)
            {
                BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionPetition] I could not file petition because I became member of " << clubId << ". "
                          "I am " << mBlazeId << ".");
                err = Blaze::ERR_OK;
            }
        }
        else if (err == Blaze::CLUBS_ERR_MAX_PETITIONS_SENT)
        {
            // I maxed out my petitions. Revoke them all!
            GetPetitionsRequest req;
            GetPetitionsResponse resp;
            
            req.setClubId(clubId);
            req.setPetitionsType(CLUBS_PETITION_SENT_BY_ME);
            err = mClubsProxy->getPetitions(req, resp);
            mOwner->addTransactionTime(0);
            
            if (err == Blaze::ERR_OK)
            {
                ClubMessageList &msgList = resp.getClubPetitionsList();
                for (
                    ClubMessageList::const_iterator itml = msgList.begin(); 
                    itml != msgList.end() && err == Blaze::ERR_OK; 
                    itml++)
                {
                    ProcessPetitionRequest preq;
                    preq.setPetitionId((*itml)->getMessageId());
                    err = mClubsProxy->revokePetition(preq);
                    mOwner->addTransactionTime(0);
                    if (err == Blaze::CLUBS_ERR_INVALID_ARGUMENT)
                    {
                        BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[actionPetition] I could not revoke petition " << preq.getPetitionId() << " "
                                  "I am " << mBlazeId << ".");
                        err = ERR_OK;
                    }
                }
            }
        }
    }
        
    return err;    
}


BlazeRpcError ClubsInstance::actionInvitation()
{
    BlazeRpcError err = Blaze::ERR_OK;

    // check for invitaitons sent to me
    GetInvitationsRequest req;
    GetInvitationsResponse resp;
    
    req.setInvitationsType(CLUBS_SENT_TO_ME);
    err = mClubsProxy->getInvitations(req, resp);
    mOwner->addTransactionTime(0);

    if (err != ERR_OK)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionInvitation] I could not get list of invitations");
        return err;
    }

    bool bAccept = false;    

    ClubId clubId = getRandomClub();
    if (clubId != 0)
    {
        // get memebers
        GetMembersRequest reqGetMembs;
        GetMembersResponse respGetMembs;
        
        reqGetMembs.setClubId(clubId);
        err = mClubsProxy->getMembers(reqGetMembs, respGetMembs);
        mOwner->addTransactionTime(0);
        if (err != Blaze::ERR_OK)
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionInvitation] I could not get list of members for club " << clubId);
            return err;
        }
        
        if ((clubId > mOwner->mMaxNoLeaveClubId || respGetMembs.getClubMemberList().size() > 0) 
                && resp.getClubInvList().size() > 0)
        {
            // if I am in ok to leave club, leave it and accept first invitation, decline other
            err = actionRemoveMember(clubId);
            mOwner->addTransactionTime(0);
            if (err == Blaze::CLUBS_ERR_LAST_GM_CANNOT_LEAVE)
            {
                // promote someone else to GM and leave
                ClubMemberList::const_iterator it = respGetMembs.getClubMemberList().begin();

                if (it != respGetMembs.getClubMemberList().end() && (*it)->getUser().getBlazeId() == mBlazeId)
                    it++;
                
                if (it != respGetMembs.getClubMemberList().end())
                {
                    PromoteToGMRequest promoteReq;
                    promoteReq.setClubId(clubId);
                    promoteReq.setBlazeId((*it)->getUser().getBlazeId());
                    err = mClubsProxy->promoteToGM(promoteReq);
                    mOwner->addTransactionTime(0);
                    if (err != Blaze::ERR_OK)
                    {
                        BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionInvitation] I could not promote " << (*it)->getUser().getBlazeId() << " to GM in club " << clubId);
                        return err;
                    }
                }
                
                // now try to leave again
                err = actionRemoveMember(clubId);
                mOwner->addTransactionTime(0);
            }
            if (err != Blaze::ERR_OK) 
                return err;        
        }
    }

    // favor acceptance 3:1 aginst decline
    if (Random::getRandomNumber(3) > 1)
        bAccept = true;
        
    ClubMessageList &msgList = resp.getClubInvList();
    ClubMessageList::const_iterator it = msgList.begin();
    
    for (; it != msgList.end(); it++)
    {
        ProcessInvitationRequest invReq;
        invReq.setInvitationId((*it)->getMessageId());
        if (bAccept && it == msgList.begin())
            err = mClubsProxy->acceptInvitation(invReq);
        else
            err = mClubsProxy->declineInvitation(invReq);
        mOwner->addTransactionTime(0);
        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[actionInvitation] I could not accept/decline invitation " << (*it)->getMessageId());
        }
    }

    if (clubId != 0)
    {
        // I am a club member, but am I GM?
        GetClubMembershipForUsersRequest mreq;
        GetClubMembershipForUsersResponse mresp;
        
        mreq.getBlazeIdList().push_back(mBlazeId);
        err = mClubsProxy->getClubMembershipForUsers(mreq, mresp);
        mOwner->addTransactionTime(0);
        
        if (err == Blaze::ERR_OK)
        {
            ClubMembershipMap::const_iterator itr = mresp.getMembershipMap().find(mBlazeId);
            if (itr == mresp.getMembershipMap().end())
            {
                BLAZE_ERR_LOG(BlazeRpcLog::clubs, "[actionInvitation] Could not find myself in membership map! "
                          "I am " << mBlazeId << " and my club is " << clubId << ".");
                return ERR_SYSTEM;
            }

            ClubMembershipList &membershipList = itr->second->getClubMembershipList();
            ClubMembership *membership = *(membershipList.begin()); // assuming at least one (and the first one)
            MembershipStatus status = membership->getClubMember().getMembershipStatus();
            if (status == CLUBS_OWNER || status == CLUBS_GM)
            {
                // if I am GM in no-leave club, send invitation
                SendInvitationRequest request;
                uint32_t rndCap = mOwner->mTotalConnections * 16;
                BlazeId blazeId = Random::getRandomNumber(rndCap > mOwner->mTotalUsers ? mOwner->mTotalUsers : rndCap) + mOwner->mMinAccId;
                request.setBlazeId(blazeId);
                request.setClubId(clubId);

                // this transaction will be counted by framework (no addTransactionTime here)
                err = mClubsProxy->sendInvitation(request);
                
                if (err == Blaze::CLUBS_ERR_INVITATION_ALREADY_SENT || err == Blaze::CLUBS_ERR_PETITION_ALREADY_SENT)
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionInvitation] I have already sent invitation/petition to user " << request.getClubId() << " "
                              "I am " << mBlazeId << ".");
                    err = Blaze::ERR_OK;
                }
                else if (err == Blaze::CLUBS_ERR_MAX_INVITES_SENT)
                {
                    BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionInvitation] The number of invitations in the club " << request.getClubId() << " is maxed out. "
                              "I am " << mBlazeId << ".");
                    // revoke one invitation
                    GetInvitationsRequest giReq;
                    GetInvitationsResponse giResp;
                    
                    giReq.setClubId(clubId);
                    giReq.setInvitationsType(CLUBS_SENT_BY_CLUB);
                    err = mClubsProxy->getInvitations(giReq, giResp);
                    mOwner->addTransactionTime(0);
                    
                    if (err == Blaze::ERR_OK)
                    {
                        ClubMessageList &msgListTemp = giResp.getClubInvList();
                        if (msgListTemp.size() > 0)
                        {
                            ProcessInvitationRequest piReq;
                            piReq.setInvitationId((*msgListTemp.begin())->getMessageId());
                            err = mClubsProxy->revokeInvitation(piReq);
                            mOwner->addTransactionTime(0);
                        }
                        else
                            err = Blaze::ERR_OK;
                    }
                }
            }
        }
    }
            
    return err;    
}

BlazeRpcError ClubsInstance::actionUpdateOnlineStatus()
{
    BlazeRpcError err = Blaze::ERR_OK;

    ClubId clubId = getRandomClub();
    if (clubId == 0)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[actionUpdateOnlineStatus] I am not a club member.");
    }
    else
    {
        UpdateMemberOnlineStatusRequest req;

        req.setNewStatus(static_cast<MemberOnlineStatus>(Random::getRandomNumber((int)(CLUBS_MEMBER_ONLINE_STATUS_COUNT))));

        err = mClubsProxy->updateMemberOnlineStatus(req);
    }

    return err;
}

BlazeRpcError ClubsInstance::actionUpdateMemberMetadata()
{
    BlazeRpcError err = Blaze::ERR_OK;

    ClubId clubId = getRandomClub();
    if (clubId == 0)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[actionUpdateMemberMetadata] I am not a club member.");
    }
    else
    {
                

        UpdateMemberMetadataRequest req;
        eastl::string value;
        value.sprintf("%d", Random::getRandomNumber(100));
        req.getMetaData()["someParam"] = value.c_str();
        err = mClubsProxy->updateMemberMetadata(req);
    }
        
    return err;
}

BlazeRpcError ClubsInstance::actionSetMetadata()
{
    BlazeRpcError err = Blaze::ERR_OK;

    ClubId clubId = getRandomClub();
    if (clubId == 0)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[actionSetMetadata] I am not a club member.");
    }
    else
    {
        SetMetadataRequest req;
        req.setClubId(clubId);
        eastl::string value;
        value.sprintf("%d", Random::getRandomNumber(100));
        req.getMetaDataUnion().setMetadataString(value.c_str());
        err = mClubsProxy->setMetadata(req);
        if (err == CLUBS_ERR_NO_PRIVILEGE)
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionSetMetadata] I am not a club GM so I have no privilege to update metadata.");
            err = Blaze::ERR_OK;
        }
    }
        
    return err;
}

BlazeRpcError ClubsInstance::actionUpdateClubSettings()
{
    BlazeRpcError err = Blaze::ERR_OK;

    ClubId clubId = getRandomClub();
    if (clubId == 0)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::clubs, "[actionUpdateClubSettings] I am not a club member.");
    }
    else
    {
        GetClubsRequest gcReq;
        GetClubsResponse gcResp;
        gcReq.getClubIdList().push_back(clubId);
        err = mClubsProxy->getClubs(gcReq, gcResp);
        
        if (err != Blaze::ERR_OK || gcResp.getClubList().size() != 1)
            return err;
            
        UpdateClubSettingsRequest req;
        req.setClubId(clubId);
        (*gcResp.getClubList().begin())->getClubSettings().copyInto(req.getClubSettings());
        eastl::string value;
        value.sprintf("%d", Random::getRandomNumber(100));
        req.getClubSettings().setDescription(value.c_str());

        /// @todo option to change tags

        err = mClubsProxy->updateClubSettings(req);
        
        if (err == CLUBS_ERR_NO_PRIVILEGE)
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::clubs, "[actionUpdateClubSettings] I am not a club GM so I have no privilege to update club settings.");
            err = Blaze::ERR_OK;
        }
    }
        
    return err;
}


} // Stress
} // Blaze



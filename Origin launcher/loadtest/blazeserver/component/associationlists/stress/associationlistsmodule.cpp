/*************************************************************************************************/
/*!
    \file associationlistsmodule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "associationlistsmodule.h"
#include "loginmanager.h"
#include "framework/connection/selector.h"
#include "framework/config/config_file.h"
#include "framework/config/config_boot_util.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/random.h"
#ifdef STATIC_MYSQL_DRIVER
#include "framework/database/mysql/blazemysql.h"
#endif
#include "messaging/tdf/messagingtypes.h"

#define WIPEOUT_TABLE(table)  { BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "Deleting all entries from " table "."); \
    int error = mysql_query(&mysql, "TRUNCATE " table ";"); \
    if (error) \
    { \
        BLAZE_WARN_LOG(BlazeRpcLog::associationlists, "Error: " << mysql_error(&mysql)); \
        mysql_close(&mysql); \
        mysql_library_end(); \
        return false; \
    } \
    resetCommand(&mysql); }

#if 1//def EA_PLATFORM_WINDOWS
// helper macro for a logging timer mechanism that indicates some behavior is still going, but not continually spamming
#define CHECK_PERIODIC_LOG(key, nextDelayUS) if (TimeValue::getTimeOfDay() > mNextLogTime[key] && (mNextLogTime[key] = TimeValue::getTimeOfDay() + (nextDelayUS)) > 0) // assuming assignment doesn't happen if left expression fails
#define RESET_PERIODIC_LOG(key, nextDelayUS) { mNextLogTime[key] = TimeValue::getTimeOfDay() + (nextDelayUS); }
#else
// disabled
#define CHECK_PERIODIC_LOG(key, nextDelayUS) if (false)
#define RESET_PERIODIC_LOG(key, nextDelayUS) { }
#endif

using namespace Blaze::Association;

namespace Blaze
{
namespace Stress
{

// this list type isn't a core feature, so define it here
const uint32_t LIST_TYPE_FOLLOW = 5; // i.e. your "favorites"
const uint32_t LIST_TYPE_FOLLOWERS = 6; // i.e. your "fans"

const uint32_t MESSAGE_TYPE_PUBLIC = 1;

/*
* List of available actions that can be performed by the module.
*/

static const char8_t* ACTION_STR_SIMULATE_PROD = "simulateProduction";
static const char8_t* ACTION_STR_GET_LISTS = "getLists";
static const char8_t* ACTION_STR_UPDATE_FRIEND_LIST = "updateFriendList";
static const char8_t* ACTION_STR_TOGGLE_SUBSCRIPTION = "toggleSubscription";
static const char8_t* ACTION_STR_ADD_RECENT_PLAYER = "addRecentPlayer";
static const char8_t* ACTION_STR_FOLLOW_BALANCED = "followBalanced";
static const char8_t* ACTION_STR_FOLLOW_AS_CELEBRITY = "followAsCelebrity";
static const char8_t* ACTION_STR_FOLLOW_AS_FAN = "followAsFan";

static const char8_t* CONTEXT_STR_SIMULATE_PROD = "AssociationListsModule::simulateProduction";
static const char8_t* CONTEXT_STR_GET_LISTS = "AssociationListsModule::getLists";
static const char8_t* CONTEXT_STR_UPDATE_FRIEND_LIST = "AssociationListsModule::updateFriendList";
static const char8_t* CONTEXT_STR_TOGGLE_SUBSCRIPTION = "AssociationListsModule::toggleSubscription";
static const char8_t* CONTEXT_STR_ADD_RECENT_PLAYER = "AssociationListsModule::addRecentPlayer";
static const char8_t* CONTEXT_STR_FOLLOW_BALANCED = "AssociationListsModule::followBalanced";
static const char8_t* CONTEXT_STR_FOLLOW_AS_CELEBRITY = "AssociationListsModule::followAsCelebrity";
static const char8_t* CONTEXT_STR_FOLLOW_AS_FAN = "AssociationListsModule::followAsFan";

uint32_t AssociationListsInstance::mInstanceOrdStatic = 0;

/*** Public Methods ******************************************************************************/

// static
StressModule* AssociationListsModule::create()
{
    return BLAZE_NEW AssociationListsModule();
}

AssociationListsModule::~AssociationListsModule()
{
}

bool AssociationListsModule::parseServerConfig(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    const ConfigMap *associationListsConfigFile = nullptr;

    if (bootUtil != nullptr)
    {
        associationListsConfigFile = bootUtil->createFrom("associationlists", ConfigBootUtil::CONFIG_IS_COMPONENT);
    }
    else
    {
        const ConfigMap* configFilePathMap = config.getMap("configFilePath");

        const char8_t* associationListsConfigFilePath = "component/associationlists.cfg";

        if (configFilePathMap != nullptr)
        {
            associationListsConfigFilePath = configFilePathMap->getString("associationlists", "component/associationlists.cfg");
        }

        associationListsConfigFile = ConfigFile::createFromFile(associationListsConfigFilePath, true);
    }

    if (associationListsConfigFile == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists,"[AssociationListsModule].parseServerConfig: Failed to load association lists configuration file");
        return false;
    }

    const ConfigMap* configMap = associationListsConfigFile->getMap("component.associationlists");
    if (configMap == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists,"[AssociaionListsModule].parseServerConfig: Failed to locate association lists config inside configuration file");
        return false;
    }

    const ConfigSequence* lists = configMap->getSequence("lists");
    if (lists == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists,"[AssociaionListsModule].parseServerConfig: Failed to locate association lists config component config");
        return false;
    }

    while (lists->hasNext())
    {
        const ConfigMap* listDef = lists->nextMap();
        mListMaxSizes[listDef->getUInt32("id", 0)] = listDef->getUInt32("maxSize", 0);
    }

    return true;
}

bool AssociationListsModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if (!StressModule::initialize(config, bootUtil))
    {
        return false;
    }

    if (!parseServerConfig(config, bootUtil))
        return false;

    // blaze id to start at
    mMinAccId = config.getUInt64("minAccId", 1);

    mCelebrityStartAccId = config.getUInt64("celebrityStartAccId", 1);
    mCelebrityAccCount = config.getUInt64("celebrityAccCount", 1);
    mFetchMessageRate = config.getUInt32("fetchMessageRate", 1);
    if (mFetchMessageRate == 0)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::associationlists, "[AssociationListsModule].initialize: Warning, fetch message rate cannot be 0, setting it to 1");
        mFetchMessageRate = 1;
    }
    mTransientMessageRate = config.getUInt32("transientMessageRate", 1);
    if (mTransientMessageRate == 0)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::associationlists, "[AssociationListsModule].initialize: Warning, transient message rate cannot be 0, setting it to 1");
        mTransientMessageRate = 1;
    }
    mPersistentMessageRate = config.getUInt32("persistentMessageRate", 1);
    if (mPersistentMessageRate == 0)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::associationlists, "[AssociationListsModule].initialize: Warning, persistent message rate cannot be 0, setting it to 1");
        mPersistentMessageRate = 1;
    }

    // pickup database parameters
    mDbHost = config.getString("dbhost", "");
    mDbPort = config.getUInt32("dbport", 0);
    mDatabase = config.getString("database", "");
    mDbUser = config.getString("dbuser", "");
    mDbPass = config.getString("dbpass", "");

    mRecreateDatabase = config.getBool("recreateDatabase", false);

    mTotalUsers = config.getUInt32("totalUsers", 0);
    mTargetFriendListSize = config.getUInt32("targetFriendListSize", 10);
    if (mTargetFriendListSize == 0)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::associationlists, "[AssociationListsModule].initialize: Warning, target friend list size cannot be 0, setting it to 1");
        mTargetFriendListSize = 1;
    }

    eastl::string action = config.getString("action", "");
    mAction = strToAction(action.c_str());
    if (mAction >= ACTION_MAX_ENUM)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsModule].initialize: Missing or illegal action parameter (" << action.c_str() << ") in association lists module configuration.");
        return false;
    }
    
    if (mRecreateDatabase && !createDatabase())
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsModule].initialize: Could not recreate database.");    
        return false;
    }

    mRandSeed = config.getInt32("randSeed", -1);

    (mRandSeed < 0) ? srand((unsigned int)TimeValue::getTimeOfDay().getSec()) : srand(mRandSeed);

    mActionWeightSum = 0;
    memset(&mActionWeightDistribution[0], 0, sizeof(uint32_t) * ACTION_MAX_ENUM);
    const Blaze::ConfigMap* weightMap = config.getMap("actionWeightDistribution");
    while((weightMap != nullptr) && (weightMap->hasNext()))
    {
        const char8_t* actionKeyStr = weightMap->nextKey();
        uint32_t actionKeyVal = (uint32_t)strToAction(actionKeyStr);
        if (actionKeyVal == ACTION_MAX_ENUM)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::associationlists, "[AssociationListsModule].initialize: Action " << actionKeyStr << " is not a valid action string");
            continue;
        }
        uint32_t actionVal = weightMap->getUInt32(actionKeyStr, 0);
        mActionWeightDistribution[actionKeyVal] = actionVal;
        mActionWeightSum += actionVal;
    }

    if ((mAction == ACTION_SIMULATE_PRODUCTION) && (mActionWeightSum == 0))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsModule].initialize: Must specify actionWeightDistribution if using action=simulateProduction");
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

bool AssociationListsModule::createDatabase()
{
#ifdef STATIC_MYSQL_DRIVER
    MYSQL mysql;

    mysql_init(&mysql);

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
            BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsModule].createDatabase: Could not connect to db: " << mysql_error(&mysql));
            mysql_close(&mysql);
            mysql_library_end();
            return false;
    }
    
    BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsModule].createDatabase: Connected to db.");

    WIPEOUT_TABLE("user_association_list_size");
    WIPEOUT_TABLE("user_association_list_friendslist");

    mysql_close(&mysql);
    mysql_library_end();

    return true;
#else
    return false;   //No static MySQL lib
#endif
}

StressInstance* AssociationListsModule::createInstance(StressConnection* connection, Login* login)
{
    return BLAZE_NEW AssociationListsInstance(this, connection, login);
}

/*** Private Methods *****************************************************************************/

AssociationListsModule::AssociationListsModule()
    : mRandSeed(-1)
{
}


AssociationListsInstance::AssociationListsInstance(AssociationListsModule* owner, StressConnection* connection, Login* login)
:   StressInstance(owner, connection, login, BlazeRpcLog::associationlists),
    mOwner(owner),
    mTimerId(INVALID_TIMER_ID),
    mAssociationListsProxy(BLAZE_NEW Blaze::Association::AssociationListsSlave(*connection)),
    mMessagingProxy(BLAZE_NEW Blaze::Messaging::MessagingSlave(*connection)),
    mUserSessionsProxy(BLAZE_NEW UserSessionsSlave(*connection)),
    mPersistentMessagesReceived(0),
    mTransientMessagesReceived(0),
    mRunCount(0), mCreateIt(false),
    mResultNotificationError(-1), mOutstandingActions(0)
{
    connection->addAsyncHandler(Blaze::UserSessionsSlave::COMPONENT_ID, Blaze::UserSessionsSlave::NOTIFY_USERSESSIONEXTENDEDDATAUPDATE, this);
    connection->addAsyncHandler(Blaze::UserSessionsSlave::COMPONENT_ID, Blaze::UserSessionsSlave::NOTIFY_USERADDED, this);
    connection->addAsyncHandler(Blaze::Messaging::MessagingSlave::COMPONENT_ID, Blaze::Messaging::MessagingSlave::NOTIFY_NOTIFYMESSAGE, this);

    mInstanceOrd = mInstanceOrdStatic++;
    mName = "";

    uint64_t celebId = mOwner->mCelebrityStartAccId;
    uint64_t celebCount = mOwner->mCelebrityAccCount;
    for (; celebCount > 0; celebCount--, celebId++)
    {
        mCelebritiesToFollow.insert(celebId);
    }
}

void AssociationListsInstance::onLogin(Blaze::BlazeRpcError result)
{
    if (result == Blaze::ERR_OK)
    {
        mBlazeId = getLogin()->getUserLoginInfo()->getBlazeId();
        mOwner->mBlazeIdList.insert(mBlazeId);
    }

    GetListsRequest req;
    // explicitly specify lists we expect to be small!!!
    ListInfo* listInfo = req.getListInfoVector().pull_back();
    listInfo->getId().setListType(LIST_TYPE_FRIEND);
    listInfo = req.getListInfoVector().pull_back();
    listInfo->getId().setListType(LIST_TYPE_RECENTPLAYER);
    listInfo = req.getListInfoVector().pull_back();
    listInfo->getId().setListType(LIST_TYPE_MUTE);
    listInfo = req.getListInfoVector().pull_back();
    listInfo->getId().setListType(LIST_TYPE_BLOCK);
    Lists resp;
    BlazeRpcError err = mAssociationListsProxy->getLists(req, resp);
    if (err != Blaze::ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].onLogin: Could not fetch association lists at login time");
        return;
    }

    const ListMembersVector& memVec = resp.getListMembersVector();
    for (ListMembersVector::const_iterator it = memVec.begin(); it != memVec.end(); ++it)
    {
        const ListMembers* members = *it;
        const ListMemberInfoVector& memberInfoVec = members->getListMemberInfoVector();
        for (ListMemberInfoVector::const_iterator memIt = memberInfoVec.begin(); memIt != memberInfoVec.end(); ++memIt)
        {
            const ListMemberInfo* info = *memIt;
            info->getListMemberId().copyInto(mMemberMap[members->getInfo().getId().getListType()][info->getListMemberId().getUser().getBlazeId()]);
        }
    }
}

void AssociationListsInstance::onDisconnected()
{
    BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].onDisconnected: Disconnected. Run count: " << mRunCount);
}

void AssociationListsInstance::onUserExtendedData(const Blaze::UserSessionExtendedData& extData)
{
    const ObjectIdList &list = extData.getBlazeObjectIdList();
    ObjectIdList::const_iterator it = list.begin();
    for (; it != list.end(); it++)
    {
    }
}

void AssociationListsInstance::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    if (component == Blaze::UserSessionsSlave::COMPONENT_ID)
    {
        if (type == Blaze::UserSessionsSlave::NOTIFY_USERADDED)
        {
            Heat2Decoder decoder;
            Blaze::NotifyUserAdded userAdded;
            if (decoder.decode(*payload, userAdded) == ERR_OK)
            {
                // Because each user is associate with its own connection, we do not
                // do any validation here to ensure that this notification is "for us"
                //onUserExtendedData(userAdded.getExtendedData());

                CHECK_PERIODIC_LOG("onAsync: NOTIFY_USERADDED", 60 * 1000 * 1000)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].onAsync: NOTIFY_USERADDED: " << userAdded.getUserInfo().getBlazeId());
                }
            }
        }
        else if (type == Blaze::UserSessionsSlave::NOTIFY_USERSESSIONEXTENDEDDATAUPDATE)
        {
            Heat2Decoder decoder;
            Blaze::UserSessionExtendedDataUpdate extData;
            if (decoder.decode(*payload, extData) == ERR_OK)
            {
                //if (extData.getBlazeId() == mBlazeId)
                //{
                //    onUserExtendedData(extData.getExtendedData());
                //}

                CHECK_PERIODIC_LOG("onAsync: NOTIFY_USERSESSIONEXTENDEDDATAUPDATE", 60 * 1000 * 1000)
                {
                    if (extData.getBlazeId() == mBlazeId)
                    {
                        BLAZE_TRACE_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].onAsync: NOTIFY_USERSESSIONEXTENDEDDATAUPDATE: " << extData.getBlazeId());
                    }
                    else
                    {
                        // more interested about updates from who I'm following
                        BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].onAsync: NOTIFY_USERSESSIONEXTENDEDDATAUPDATE: " << extData.getBlazeId());
                    }
                }
            }
        }
    }
    else if (component == Blaze::Messaging::MessagingSlave::COMPONENT_ID)
    {
        if (type == Blaze::Messaging::MessagingSlave::NOTIFY_NOTIFYMESSAGE)
        {
            Heat2Decoder decoder;
            Blaze::Messaging::ServerMessage msg;
            BlazeRpcError err = decoder.decode(*payload, msg);
            if (err == Blaze::ERR_OK)
            {
                if (msg.getPayload().getFlags().getPersistent())
                {
                    ++mPersistentMessagesReceived;
                    /// @todo purge persistent messages ???
                }
                else
                {
                    ++mTransientMessagesReceived;
                }
                CHECK_PERIODIC_LOG("onAsync: NOTIFY_NOTIFYMESSAGE", 60 * 1000 * 1000)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].onAsync: NOTIFY_NOTIFYMESSAGE: num persistent: " << mPersistentMessagesReceived
                        << ", num transient: " << mTransientMessagesReceived
                        << " {src:" << msg.getSourceIdent().getName()
                        << ", id:" << msg.getMessageId()
                        << ", type:" << msg.getPayload().getType()
                        << ", tag:" << msg.getPayload().getTag()
                        << ", cflags:" << msg.getPayload().getFlags().getBits()
                        << ", sflags:" << msg.getFlags().getBits()
                        << ", time:" << msg.getTimestamp()
                        << ", attr_cnt:" << msg.getPayload().getAttrMap().size()
                        << "}");
                }

                RESET_PERIODIC_LOG("actionFollowAsFan: waiting", 60 * 1000 * 1000); // rec'd update, so no need to be reminded about 'waiting'
            }
            else
            {
                BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].onAsync: NOTIFY_NOTIFYMESSAGE: error when decoding received message " << Blaze::ErrorHelp::getErrorName(err));
            }
        }
    }
}

BlazeRpcError AssociationListsInstance::execute()
{
    BlazeRpcError result = ERR_OK;
    
    ++mRunCount;
    BLAZE_TRACE_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].execute: Start action " << mOwner->mAction << ".");

    AssociationListsModule::Action action = mOwner->mAction;
    if (action == AssociationListsModule::ACTION_SIMULATE_PRODUCTION)
    {
        mName = "simulateProduction";
        if (mOutstandingActions == 0)
        {
            // reset our distributions
            for (uint32_t i=0; i<AssociationListsModule::ACTION_MAX_ENUM; i++)
            {
                mActionWeightDistribution[i] = mOwner->mActionWeightDistribution[i];
            }
            mOutstandingActions = mOwner->mActionWeightSum;
        }

        action = getWeightedRandomAction();
        BLAZE_TRACE_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].execute: Random action " << action << ".");
    }

    switch(action)
    {
    case AssociationListsModule::ACTION_GET_LISTS:
        {
            mName = mOwner->actionToStr(AssociationListsModule::ACTION_GET_LISTS);
            Fiber::setCurrentContext(mOwner->actionToContext(AssociationListsModule::ACTION_GET_LISTS));
            result = actionGetLists();
            break;
        }
    case AssociationListsModule::ACTION_UPDATE_FRIEND_LIST:
        {
            mName = mOwner->actionToStr(AssociationListsModule::ACTION_UPDATE_FRIEND_LIST);
            Fiber::setCurrentContext(mOwner->actionToContext(AssociationListsModule::ACTION_UPDATE_FRIEND_LIST));
            result = actionUpdateFriendList();
            break;
        }
    case AssociationListsModule::ACTION_TOGGLE_SUBSCRIPTION:
        {
            mName = mOwner->actionToStr(AssociationListsModule::ACTION_TOGGLE_SUBSCRIPTION);
            Fiber::setCurrentContext(mOwner->actionToContext(AssociationListsModule::ACTION_TOGGLE_SUBSCRIPTION));
            result = actionToggleSubscription();
            break;
        }
    case AssociationListsModule::ACTION_ADD_RECENT_PLAYER:
        {
            mName = mOwner->actionToStr(AssociationListsModule::ACTION_ADD_RECENT_PLAYER);
            Fiber::setCurrentContext(mOwner->actionToContext(AssociationListsModule::ACTION_ADD_RECENT_PLAYER));
            result = actionAddRecentPlayer();
            break;
        }
    case AssociationListsModule::ACTION_FOLLOW_BALANCED:
        {
            mName = mOwner->actionToStr(AssociationListsModule::ACTION_FOLLOW_BALANCED);
            Fiber::setCurrentContext(mOwner->actionToContext(AssociationListsModule::ACTION_FOLLOW_BALANCED));
            result = actionFollowBalanced();
            break;
        }
    case AssociationListsModule::ACTION_FOLLOW_AS_CELEBRITY:
        {
            mName = mOwner->actionToStr(AssociationListsModule::ACTION_FOLLOW_AS_CELEBRITY);
            Fiber::setCurrentContext(mOwner->actionToContext(AssociationListsModule::ACTION_FOLLOW_AS_CELEBRITY));
            result = actionFollowAsCelebrity();
            break;
        }
    case AssociationListsModule::ACTION_FOLLOW_AS_FAN:
        {
            mName = mOwner->actionToStr(AssociationListsModule::ACTION_FOLLOW_AS_FAN);
            Fiber::setCurrentContext(mOwner->actionToContext(AssociationListsModule::ACTION_FOLLOW_AS_FAN));
            result = actionFollowAsFan();
            break;
        }
    default:
        {
            BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].execute: Unknown action(" << mOwner->mAction << ")");
            result = ERR_SYSTEM;
        }
    }
    return result;
}

bool AssociationListsInstance::tossTheDice(uint32_t val)
{
    if (static_cast<uint32_t>(Random::getRandomNumber(100)) < val)
        return true;
        
    return false;
}

AssociationListsModule::Action AssociationListsInstance::getWeightedRandomAction()
{
    /* pick a number between 0 and mOutstandingActions.  If the random number falls 
     * within the range of bin N, then return action N
     */
    int32_t rand = Random::getRandomNumber(mOutstandingActions);
    uint32_t i = 1;
    for(; i<AssociationListsModule::ACTION_MAX_ENUM; i++)
    {
        rand -= mActionWeightDistribution[i];
        if (rand < 0)
            break;
    }
    EA_ASSERT(i < AssociationListsModule::ACTION_MAX_ENUM);
    mActionWeightDistribution[i]--;
    mOutstandingActions--;
        
    return (AssociationListsModule::Action)(i);
}

AssociationListsModule::Action AssociationListsModule::strToAction(const char8_t* actionStr) const
{
    if (blaze_stricmp(actionStr, ACTION_STR_SIMULATE_PROD) == 0)
        return ACTION_SIMULATE_PRODUCTION;
    else if (blaze_stricmp(actionStr, ACTION_STR_GET_LISTS) == 0)
        return ACTION_GET_LISTS;
    else if (blaze_stricmp(actionStr, ACTION_STR_UPDATE_FRIEND_LIST) == 0)
        return ACTION_UPDATE_FRIEND_LIST;
    else if (blaze_stricmp(actionStr, ACTION_STR_TOGGLE_SUBSCRIPTION) == 0)
        return ACTION_TOGGLE_SUBSCRIPTION;
    else if (blaze_stricmp(actionStr, ACTION_STR_ADD_RECENT_PLAYER) == 0)
        return ACTION_ADD_RECENT_PLAYER;
    if (blaze_stricmp(actionStr, ACTION_STR_FOLLOW_BALANCED) == 0)
        return ACTION_FOLLOW_BALANCED;
    if (blaze_stricmp(actionStr, ACTION_STR_FOLLOW_AS_CELEBRITY) == 0)
        return ACTION_FOLLOW_AS_CELEBRITY;
    if (blaze_stricmp(actionStr, ACTION_STR_FOLLOW_AS_FAN) == 0)
        return ACTION_FOLLOW_AS_FAN;
    else
        return ACTION_MAX_ENUM;
}

const char8_t* AssociationListsModule::actionToStr(Action action) const
{
    switch(action)
    {
        case ACTION_SIMULATE_PRODUCTION:
            return ACTION_STR_SIMULATE_PROD;
        case ACTION_GET_LISTS:
            return ACTION_STR_GET_LISTS;
        case ACTION_UPDATE_FRIEND_LIST:
            return ACTION_STR_UPDATE_FRIEND_LIST;
        case ACTION_TOGGLE_SUBSCRIPTION:
            return ACTION_STR_TOGGLE_SUBSCRIPTION;
        case ACTION_ADD_RECENT_PLAYER:
            return ACTION_STR_ADD_RECENT_PLAYER;
        case ACTION_FOLLOW_BALANCED:
            return ACTION_STR_FOLLOW_BALANCED;
        case ACTION_FOLLOW_AS_CELEBRITY:
            return ACTION_STR_FOLLOW_AS_CELEBRITY;
        case ACTION_FOLLOW_AS_FAN:
            return ACTION_STR_FOLLOW_AS_FAN;
        default:
            return nullptr;
    }
 }

const char8_t* AssociationListsModule::actionToContext(Action action) const
{
    switch(action)
    {
        case ACTION_SIMULATE_PRODUCTION:
            return CONTEXT_STR_SIMULATE_PROD;
        case ACTION_GET_LISTS:
            return CONTEXT_STR_GET_LISTS;
        case ACTION_UPDATE_FRIEND_LIST:
            return CONTEXT_STR_UPDATE_FRIEND_LIST;
        case ACTION_TOGGLE_SUBSCRIPTION:
            return CONTEXT_STR_TOGGLE_SUBSCRIPTION;
        case ACTION_ADD_RECENT_PLAYER:
            return CONTEXT_STR_ADD_RECENT_PLAYER;
        case ACTION_FOLLOW_BALANCED:
            return CONTEXT_STR_FOLLOW_BALANCED;
        case ACTION_FOLLOW_AS_CELEBRITY:
            return CONTEXT_STR_FOLLOW_AS_CELEBRITY;
        case ACTION_FOLLOW_AS_FAN:
            return CONTEXT_STR_FOLLOW_AS_FAN;
        default:
            return nullptr;
    }
}

BlazeRpcError AssociationListsInstance::actionGetLists()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionGetLists: Fetching all lists for user");

    GetListsRequest req;
    Lists resp;
    BlazeRpcError err = mAssociationListsProxy->getLists(req, resp);
    if (err != Blaze::ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionGetLists: Failed with error " << Blaze::ErrorHelp::getErrorName(err));
    }

    return err;
}

BlazeRpcError AssociationListsInstance::actionUpdateFriendList()
{
    BlazeRpcError err = Blaze::ERR_OK;

    uint32_t maxSize = mOwner->mListMaxSizes[LIST_TYPE_FRIEND];
    uint32_t currSize = mMemberMap[LIST_TYPE_FRIEND].size();

    uint32_t probAdd = 50;
    if (currSize > mOwner->mTargetFriendListSize)
        probAdd = (maxSize - currSize) * 50 / (maxSize - mOwner->mTargetFriendListSize);
    else
        probAdd = 100 - (currSize * 50 / mOwner->mTargetFriendListSize);

    err = (static_cast<uint32_t>(Random::getRandomNumber(100)) < probAdd) ? addFriend() : removeFriend();

    if (err != Blaze::ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionUpdateFriendList: Failed with error " << Blaze::ErrorHelp::getErrorName(err));
    }

    return err;
}

BlazeRpcError AssociationListsInstance::actionToggleSubscription()
{
    BlazeRpcError err = Blaze::ERR_OK;

    BLAZE_TRACE_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionToggleSubscription: Toggling subscription");

    UpdateListsRequest req;
    ListIdentification* listIdent = req.getListIdentificationVector().pull_back();
    listIdent->setListType(LIST_TYPE_FRIEND);
    if (mSubscribed[LIST_TYPE_FRIEND])
    {
        err = mAssociationListsProxy->unsubscribeFromLists(req);
    }
    else
    {
        err = mAssociationListsProxy->subscribeToLists(req);
    }

    if (err == Blaze::ERR_OK)
        mSubscribed[LIST_TYPE_FRIEND] = !mSubscribed[LIST_TYPE_FRIEND];
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionToggleSubscription: Failed with error " << Blaze::ErrorHelp::getErrorName(err));
    }

    return err;
}

BlazeRpcError AssociationListsInstance::actionAddRecentPlayer()
{
    BlazeRpcError err = Blaze::ERR_OK;

    UpdateListMembersRequest req;
    UpdateListMembersResponse resp;
    req.getListIdentification().setListType(LIST_TYPE_RECENTPLAYER);
    ListMemberId* member = req.getListMemberIdVector().pull_back();
    BlazeId blazeId = Random::getRandomNumber(mOwner->mTotalUsers) + mOwner->mMinAccId;

    BLAZE_TRACE_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionAddRecentPlayer: Adding " << blazeId << " as a recent player");

    member->getUser().setBlazeId(blazeId);
    convertToPlatformInfo(member->getUser().getPlatformInfo(), blazeId, nullptr, INVALID_ACCOUNT_ID, xone);
    member->setExternalSystemId(INVALID);
    err = mAssociationListsProxy->addUsersToList(req, resp);
    if (err != Blaze::ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionAddRecentPlayer: Failed with error " << Blaze::ErrorHelp::getErrorName(err));
    }
    return err;
}

BlazeRpcError AssociationListsInstance::addFriend()
{
    BlazeRpcError err = Blaze::ERR_OK;

    UpdateListMembersRequest req;
    UpdateListMembersResponse resp;
    req.getListIdentification().setListType(LIST_TYPE_FRIEND);
    ListMemberId* member = req.getListMemberIdVector().pull_back();
    BlazeId blazeId = Random::getRandomNumber(mOwner->mTotalUsers) + mOwner->mMinAccId;
    ListMemberIdMap& memberIdMap = mMemberMap[LIST_TYPE_FRIEND];

    if (memberIdMap.size() >= mOwner->mTotalUsers)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].addFriend: Error, association list size " << memberIdMap.size()
            << " needs to be less than the totalUsers configuration, currently set to " << mOwner->mTotalUsers);
        return Blaze::ERR_SYSTEM;
    }

    ListMemberIdMap::const_iterator it = memberIdMap.find(blazeId);
    while (it != memberIdMap.end())
    {
        if (++blazeId >= static_cast<BlazeId>(mOwner->mTotalUsers + mOwner->mMinAccId))
            blazeId = static_cast<BlazeId>(mOwner->mMinAccId);
        it = memberIdMap.find(blazeId);
    }

    BLAZE_TRACE_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].addFriend: Adding " << blazeId << " as a friend");

    member->getUser().setBlazeId(blazeId);
    convertToPlatformInfo(member->getUser().getPlatformInfo(), blazeId, nullptr, INVALID_ACCOUNT_ID, xone);
    member->setExternalSystemId(INVALID);
    err = mAssociationListsProxy->addUsersToList(req, resp);
    if (err == Blaze::ERR_OK)
    {
        member->copyInto(memberIdMap[blazeId]);
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].addFriend: Failed with error " << Blaze::ErrorHelp::getErrorName(err));
    }
    return err;
}

BlazeRpcError AssociationListsInstance::removeFriend()
{
    BlazeRpcError err = Blaze::ERR_OK;

    UpdateListMembersRequest req;
    UpdateListMembersResponse resp;
    req.getListIdentification().setListType(LIST_TYPE_FRIEND);
    ListMemberId* member = req.getListMemberIdVector().pull_back();
    ListMemberIdMap& memberIdMap = mMemberMap[LIST_TYPE_FRIEND];
    if (memberIdMap.empty())
    {
        BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].removeFriend: No one to remove");
        return ERR_OK;
    }
    uint32_t index = Random::getRandomNumber(memberIdMap.size());
    ListMemberIdMap::iterator it = memberIdMap.begin();
    while (index > 0)
    {
        --index;
        ++it;
    }
    
    BLAZE_TRACE_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].removeFriend: Removing " << it->second.getUser().getBlazeId() << " as a friend");

    it->second.copyInto(*member);
    err = mAssociationListsProxy->removeUsersFromList(req, resp);
    if (err == Blaze::ERR_OK)
    {
        memberIdMap.erase(it);
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].removeFriend: Failed with error " << Blaze::ErrorHelp::getErrorName(err));
    }
    return err;
}

BlazeRpcError AssociationListsInstance::actionFollowBalanced()
{
    BlazeRpcError err = Blaze::ERR_OK;
    ListMemberIdMap& memberIdMap = mMemberMap[LIST_TYPE_FOLLOW];

    if (memberIdMap.size() < eastl::min(mOwner->mListMaxSizes[LIST_TYPE_FOLLOW], mOwner->getTotalConnections() - 1))
    {
        // get a random user
        uint32_t index = Random::getRandomNumber(mOwner->mBlazeIdList.size()); // size is at least one (i.e. me) at this point
        AssociationListsModule::BlazeIdList::iterator itr = mOwner->mBlazeIdList.begin();
        for (; index > 0; --index)
        {
            ++itr;
        }
        BlazeId blazeId = *itr;
        if (blazeId != mBlazeId)
        {
            // update (e.g. add to) the follow list if not me
            UpdateListMembersRequest req;
            UpdateListMembersResponse resp;
            req.getListIdentification().setListType(LIST_TYPE_FOLLOW);
            ListMemberId* member = req.getListMemberIdVector().pull_back();
            member->getUser().setBlazeId(blazeId);
            convertToPlatformInfo(member->getUser().getPlatformInfo(), blazeId, nullptr, INVALID_ACCOUNT_ID, xone);
            member->setExternalSystemId(INVALID);
            err = mAssociationListsProxy->addUsersToList(req, resp);
            if (err == Blaze::ERR_OK)
            {
                member->copyInto(memberIdMap[blazeId]);
#if 0
                // remove any rollover users from my list
                ListMemberIdVector::const_iterator memItr = resp.getRemovedListMemberIdVector().begin();
                for (; memItr != resp.getRemovedListMemberIdVector().end(); ++memItr)
                {
                    memberIdMap.erase((*memItr)->getUser().getBlazeId());
                }
#endif

                if (memberIdMap.size() >= eastl::min(mOwner->mListMaxSizes[LIST_TYPE_FOLLOW], mOwner->getTotalConnections() - 1))
                {
                    // newly reached limit (or threshold); make sure we're subscribed
                    mSubscribed[LIST_TYPE_FOLLOW] = false;
#ifdef EA_PLATFORM_WINDOWS
                    BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowBalanced: Finished adding users to follow");
#endif
                }
            }
            else
            {
                BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowBalanced: Failed to add with error " << Blaze::ErrorHelp::getErrorName(err));
            }
        }
        else
        {
            // try again next time
            BLAZE_TRACE_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowBalanced: Ignoring attempt to follow myself");
        }
    }
    else
    {
        // we're done with the phase of adding users we want to follow; now start doing updates for our followers
        /// @todo we should start our 'doing updates' phase after we've reached our limit (or threshold) of followers
        /// @todo make the module the phase governor

        if (!mSubscribed[LIST_TYPE_FOLLOW])
        {
            UpdateListsRequest req;
            ListIdentification* listIdent = req.getListIdentificationVector().pull_back();
            listIdent->setListType(LIST_TYPE_FOLLOW);
            BlazeRpcError subErr = mAssociationListsProxy->subscribeToLists(req);
            if (subErr == Blaze::ERR_OK)
            {
                mSubscribed[LIST_TYPE_FOLLOW] = true;
            }
            else
            {
                BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowBalanced: Failed to sub/unsub with error " << Blaze::ErrorHelp::getErrorName(subErr));
            }
        }

        UpdateExtendedDataAttributeRequest req;
        req.setRemove(false);
        req.setKey(1); // arbitrary
        req.setComponent(0); // INVALID_COMPONENT_ID
        req.setValue(Random::getRandomNumber(100));
        BlazeRpcError uedErr = mUserSessionsProxy->updateExtendedDataAttribute(req);
        if (uedErr != Blaze::ERR_OK)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowBalanced: Failed updateExtendedDataAttribute with error " << Blaze::ErrorHelp::getErrorName(uedErr));
        }
    }

    CHECK_PERIODIC_LOG("actionFollowBalanced: run count", 60 * 1000 * 1000)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowBalanced: Run count: " << mRunCount
            << ", list size: " << memberIdMap.size() << ", result: " << Blaze::ErrorHelp::getErrorName(err));
    }

    return err;
}

BlazeRpcError AssociationListsInstance::actionFollowAsCelebrity()
{
    bool persistentMessage = (mRunCount % mOwner->mPersistentMessageRate == 0);

    if (!persistentMessage && (mRunCount % mOwner->mTransientMessageRate != 0))
    {
        // nothing to do this time
        return Blaze::ERR_OK;
    }

    // how many fans/followers do i have?
    GetListForUserRequest getListRequest;
    ListMembers list;
    getListRequest.setBlazeId(mBlazeId);
    getListRequest.getListIdentification().setListType(LIST_TYPE_FOLLOWERS);
    getListRequest.setMaxResultCount(1);
    BlazeRpcError err = mAssociationListsProxy->getListForUser(getListRequest, list);
    if (err == Blaze::ERR_OK)
    {
        CHECK_PERIODIC_LOG("actionFollowAsCelebrity: fan count", 6 * 1000 * 1000)
        {
            BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowAsCelebrity: Fan count: " << list.getTotalCount());
        }

        Blaze::Messaging::ClientMessage sendMessageRequest;
        Blaze::Messaging::SendMessageResponse sendMessageResponse;

        sendMessageRequest.getFlags().clearFilterProfanity();

        eastl::string payload;
        if (persistentMessage)
        {
            /// @todo setup larger message payload (> 1K)
            payload.sprintf("%" PRId64 " says bigger blah blah blah %d", mBlazeId, mRunCount);
        }
        else
        {
            /// @todo setup minimal message payload (< 1K)
            payload.sprintf("%" PRId64 " says blah %d", mBlazeId, mRunCount);
        }
        sendMessageRequest.getAttrMap()[Blaze::Messaging::CUSTM_ATTR_MIN] = payload.c_str();

        if (persistentMessage)
        {
            // send to myself first -- my special 'public' (message type) inbox
            sendMessageRequest.getFlags().setPersistent();
            //sendMessageRequest.getFlags().setOmitMessageIds();
            sendMessageRequest.setType(MESSAGE_TYPE_PUBLIC);

            sendMessageRequest.setTargetType(ENTITY_TYPE_USER);
            sendMessageRequest.getTargetIds().clear();
            sendMessageRequest.getTargetIds().push_back(mBlazeId);

            err = mMessagingProxy->sendMessage(sendMessageRequest, sendMessageResponse);
            if (err == Blaze::ERR_OK)
            {
                CHECK_PERIODIC_LOG("actionFollowAsCelebrity: send persistent message", 60 * 1000 * 1000)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowAsCelebrity: Sent persistent message(" << payload.c_str()
                        << ") response => id: " << sendMessageResponse.getMessageId());
                }
            }
            else
            {
                BLAZE_WARN_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowAsCelebrity: Failed to send persistent message(" << payload.c_str()
                    << ") with error " << Blaze::ErrorHelp::getErrorName(err));
            }
        }

        sendMessageRequest.getFlags().clearPersistent();
        sendMessageRequest.setType(0);
        sendMessageRequest.setTargetType(list.getInfo().getBlazeObjId().type);
        sendMessageRequest.getTargetIds().clear();
        sendMessageRequest.getTargetIds().push_back(list.getInfo().getBlazeObjId().id);

        err = mMessagingProxy->sendMessage(sendMessageRequest, sendMessageResponse);
        if (err == Blaze::ERR_OK)
        {
            CHECK_PERIODIC_LOG("actionFollowAsCelebrity: send message", 60 * 1000 * 1000)
            {
                BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowAsCelebrity: Sent message(" << payload.c_str()
                    << ") response => id: " << sendMessageResponse.getMessageId());
            }
        }
        else if (err == Blaze::MESSAGING_ERR_TARGET_NOT_FOUND)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowAsCelebrity: No one to send message(" << payload.c_str()
                << ") -- converting error " << Blaze::ErrorHelp::getErrorName(err) << " to ERR_OK -- typically acceptable if ramping up");
            err = Blaze::ERR_OK;
        }
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowAsCelebrity: Failed to fetch fan count with error " << Blaze::ErrorHelp::getErrorName(err));
    }

    return err;
}

BlazeRpcError AssociationListsInstance::actionFollowAsFan()
{
    BlazeRpcError err = Blaze::ERR_OK;

    if (!mCelebritiesToFollow.empty())
    {
        AssociationListsModule::BlazeIdList::iterator itr = mCelebritiesToFollow.begin();
        BlazeId celebId = *itr;

        // update (e.g. add to) the follow list (assuming it's not me)
        UpdateListMembersRequest req;
        UpdateListMembersResponse resp;
        req.getListIdentification().setListType(LIST_TYPE_FOLLOW);
        ListMemberId* member = req.getListMemberIdVector().pull_back();
        member->getUser().setBlazeId(celebId);
        convertToPlatformInfo(member->getUser().getPlatformInfo(), celebId, nullptr, INVALID_ACCOUNT_ID, xone);
        member->setExternalSystemId(INVALID);
        err = mAssociationListsProxy->addUsersToList(req, resp);
        if (err != Blaze::ERR_OK)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowAsCelebrity: Failed to add with error " << Blaze::ErrorHelp::getErrorName(err));
        }
        if (err == Blaze::ERR_OK || err == Blaze::ASSOCIATIONLIST_ERR_MEMBER_ALREADY_IN_THE_LIST)
        {
            mCelebritiesFollowed.insert(celebId);
        }
        if (err != Blaze::ERR_SYSTEM)
        {
            // only trying to follow each celeb "once"
            mCelebritiesToFollow.erase(itr);
        }
    }
    else if (!mCelebritiesFollowed.empty())
    {
        /// @todo more randomness (or better distribution) here so that all these clients aren't fetching at about the same time
        if (mRunCount % mOwner->mFetchMessageRate == 0)
        {
            Blaze::Messaging::FetchMessageRequest fetchMessageRequest;
            Blaze::Messaging::FetchMessageResponse fetchMessageResponse;

            // pick a random celeb
            AssociationListsModule::BlazeIdList::iterator celebItr = mCelebritiesFollowed.begin();
            for (int celebIdx = Random::getRandomNumber(mCelebritiesFollowed.size()); celebIdx > 0; --celebIdx)
            {
                ++celebItr; // inefficient access is okay here b/c celeb pool is not expected to be large
            }

            fetchMessageRequest.getFlags().setMatchTarget();
            fetchMessageRequest.setTarget(EA::TDF::ObjectId(ENTITY_TYPE_USER, *celebItr));

            fetchMessageRequest.getFlags().setMatchTypes();
            fetchMessageRequest.getTypeList().push_back(MESSAGE_TYPE_PUBLIC);

            fetchMessageRequest.setOrderBy(Blaze::Messaging::ORDER_TIME_DESC);
            fetchMessageRequest.setPageSize(3); // arbitrary "small" amount of messages to fetch/get

            err = mMessagingProxy->fetchMessages(fetchMessageRequest, fetchMessageResponse);
            if (err == Blaze::ERR_OK)
            {
                CHECK_PERIODIC_LOG("actionFollowAsFan: fetch messages", 60 * 1000 * 1000)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowAsCelebrity: Fetching " << fetchMessageResponse.getCount() << " message(s)");
                }
            }
            else
            {
                BLAZE_WARN_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowAsCelebrity: Failed to fetch messages with error " << Blaze::ErrorHelp::getErrorName(err));
            }
        }

        CHECK_PERIODIC_LOG("actionFollowAsFan: waiting", 60 * 1000 * 1000)
        {
            BLAZE_INFO_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowAsCelebrity: Waiting for updates");
        }
    }
    else
    {
        CHECK_PERIODIC_LOG("actionFollowAsFan: not following", 60 * 1000 * 1000)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::associationlists, "[AssociationListsInstance].actionFollowAsCelebrity: Not following anyone!!!");
        }
    }

    return err;
}

} // Stress
} // Blaze

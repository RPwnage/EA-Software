/*************************************************************************************************/
/*!
    \file OSDKSeasonalPlayModule.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "osdkseasonalplaymodule.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/util/random.h"
#include "loginmanager.h"

namespace Blaze
{
namespace Stress
{

/*
* List of available actions that can be performed by the module.
*/
static const char8_t* ACTION_STR_GET_SEASON_CONFIGURATION = "getSeasonConfiguration";
static const char8_t* ACTION_STR_GET_SEASON_ID = "getSeasonId";

static const char8_t* ACTION_STR_GET_SEASON_DETAILS = "getSeasonDetails";
static const char8_t* ACTION_STR_GET_MY_SEASON_DETAILS = "getMySeasonDetails";
static const char8_t* ACTION_STR_GET_SEASON_RANKINGINFO = "getSeasonsRankingInfo";

static const char8_t* ACTION_STR_GET_AWARDS_CONFIGURATION = "getAwardConfiguration";
static const char8_t* ACTION_STR_GET_AWARDS_FOR_MEMBER = "getAwardsForMember";
static const char8_t* ACTION_STR_GET_AWARDS_FOR_SEASON = "getAwardsForSeason";

static const char8_t* ACTION_STR_SIMULATE_PRODUCTION = "sim";
static const char8_t* ACTION_STR_SLEEP = "sleep";
static const char8_t* ACTION_STR_TEST_NOTIFICATION = "notification";

// Report on each individual RPC
typedef struct {
    const char8_t* rpcName;
    uint32_t ctr; // total counter
    uint32_t errCtr; //error counter;
    uint32_t perCtr; // counter per measurement period
    uint32_t perErrCtr; // error counter per period
    int64_t durTotal;
    int64_t durPeriod;
    TimeValue periodStartTime;
} RpcDescriptor;

static RpcDescriptor rpcList[] = 
{
    {"getSeasonConfiguration"},
    {"getSeasonId"},

    {"getSeasonDetails"},
    {"getMySeasonDetails"},
    {"getSeasonsRankingInfo"},

    {"getAwardConfiguration"},
    {"getAwardsForMember"},
    {"getAwardsForSeason"},
};

// OSDKSeasonalPlayModule
/*** Public Methods ******************************************************************************/

// static
StressModule* OSDKSeasonalPlayModule::create()
{
    return new OSDKSeasonalPlayModule();
}

bool OSDKSeasonalPlayModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    memset(&mEventSelectedProbabilities[0], 0, sizeof(uint32_t) * ACTION_MAX_ENUM);
    memset(&mNoEventSelectedProbabilities[0], 0, sizeof(uint32_t) * ACTION_MAX_ENUM);

    bool initialized = StressModule::initialize(config, bootUtil);

    if (initialized)
    {
        const char8_t* action = config.getString("action", ACTION_STR_SIMULATE_PRODUCTION);
        mAction = stringToAction(action);
        if (ACTION_SIMULATE_PRODUCTION == mAction)
        {
            const ConfigMap* simMap = config.getMap("sim");
            if (NULL != simMap)
            {
                mMaxSleepTime = simMap->getUInt32("maxSleep", 5);

                const ConfigMap* probabilityMap = simMap->getMap("actionProbabilities");
                initialized = readProbabilities(probabilityMap);
            }
            else
            {
                BLAZE_ERR(BlazeRpcLog::osdkseasonalplay, "OSDKSeasonalPlayModule: no simulation data provided / found");
                initialized = false;
            }
        }
        else
		{
			if (ACTION_TEST_NOTIFICATION == mAction)
			{
				const ConfigMap* configMap = config.getMap(ACTION_STR_TEST_NOTIFICATION);
				if (NULL != configMap)
				{
					mMaxSleepTime = configMap->getUInt32("maxSleep", 5);
				}
				else
				{
					BLAZE_ERR(BlazeRpcLog::osdkseasonalplay, "OSDKSeasonalPlayModule: no notification test data provided / found");
					initialized = false;
				}
			}
			else
			{
				if (ACTION_MAX_ENUM == mAction)
				{
					initialized = false;
				}
			}
		}
    }

    return initialized;
}

StressInstance* OSDKSeasonalPlayModule::createInstance(StressConnection* connection, Login* login)
{
    return new OSDKSeasonalPlayInstance(this, connection, login);
}

OSDKSeasonalPlayModule::Action OSDKSeasonalPlayModule::getWeightedAction(bool isEventSelected) const
{
    uint32_t rand = static_cast<uint32_t>( Blaze::Random::getRandomNumber(100) );
    OSDKSeasonalPlayModule::Action action = ACTION_MAX_ENUM;
    
    if (isEventSelected)
    {
        for (int i = 0; i < ACTION_MAX_ENUM; ++i)
        {
            uint32_t current = mEventSelectedProbabilities[i];
            
            if (rand < current)
            {
                action = static_cast<OSDKSeasonalPlayModule::Action>(i);
                break;
            }
        }
    }
    else
    {
        for (int i = 0; i < ACTION_MAX_ENUM; ++i)
        {
            uint32_t current = mNoEventSelectedProbabilities[i];

            if (rand < current)
            {
                action = static_cast<OSDKSeasonalPlayModule::Action>(i);
                break;
            }
        }
    }

    if (action == ACTION_MAX_ENUM)
    {
        BLAZE_ERR(BlazeRpcLog::osdkseasonalplay, "OSDKSeasonalPlayModule: somehow arrived at invalid action - rand: %u", rand);
    }

    return action;
}

OSDKSeasonalPlayModule::Action OSDKSeasonalPlayModule::stringToAction(const char8_t* action)
{
    OSDKSeasonalPlayModule::Action convertedAction = ACTION_MAX_ENUM;
    
    if (blaze_stricmp(action, ACTION_STR_GET_SEASON_CONFIGURATION) == 0)
    {
        convertedAction = ACTION_GET_SEASON_CONFIGURATION;
    }
    else if (blaze_stricmp(action, ACTION_STR_GET_SEASON_ID) == 0)
    {
        convertedAction = ACTION_GET_SEASON_ID;
    }
    else if (blaze_stricmp(action, ACTION_STR_GET_SEASON_DETAILS) == 0)
    {
        convertedAction = ACTION_GET_SEASON_DETAILS;
    }
    else if (blaze_stricmp(action, ACTION_STR_GET_MY_SEASON_DETAILS) == 0)
    {
        convertedAction = ACTION_GET_MY_SEASON_DETAILS;
    }
    else if (blaze_stricmp(action, ACTION_STR_GET_SEASON_RANKINGINFO) == 0)
    {
        convertedAction = ACTION_GET_SEASON_RANKINGINFO;
    }
    else if (blaze_stricmp(action, ACTION_STR_GET_AWARDS_CONFIGURATION) == 0)
    {
        convertedAction = ACTION_GET_AWARDS_CONFIGURATION;
    }
    else if (blaze_stricmp(action, ACTION_STR_GET_AWARDS_FOR_MEMBER) == 0)
    {
        convertedAction = ACTION_GET_AWARDS_FOR_MEMBER;
    }
    else if (blaze_stricmp(action, ACTION_STR_GET_AWARDS_FOR_SEASON) == 0)
    {
        convertedAction = ACTION_GET_AWARDS_FOR_SEASON;
    }
    else if (blaze_stricmp(action, ACTION_STR_SIMULATE_PRODUCTION) == 0)
    {
        convertedAction = ACTION_SIMULATE_PRODUCTION;
    }
    else if (blaze_stricmp(action, ACTION_STR_SLEEP) == 0)
    {
        convertedAction = ACTION_SLEEP;
    }
    else if (blaze_stricmp(action, ACTION_STR_TEST_NOTIFICATION) == 0)
    {
        convertedAction = ACTION_TEST_NOTIFICATION;
    }
    else
    {
        BLAZE_ERR(BlazeRpcLog::osdkseasonalplay, "OSDKSeasonalPlayModule: unrecognized action: '%s'", action);
    }

    return convertedAction;
}

const char8_t* OSDKSeasonalPlayModule::actionToString(OSDKSeasonalPlayModule::Action action)
{
    switch (action)
    {
        case OSDKSeasonalPlayModule::ACTION_GET_SEASON_CONFIGURATION:
        {
            return ACTION_STR_GET_SEASON_CONFIGURATION;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_SEASON_ID:
        {
            return ACTION_STR_GET_SEASON_ID;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_SEASON_DETAILS:
        {
            return ACTION_STR_GET_SEASON_DETAILS;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_MY_SEASON_DETAILS:
        {
            return ACTION_STR_GET_MY_SEASON_DETAILS;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_SEASON_RANKINGINFO:
        {
            return ACTION_STR_GET_SEASON_RANKINGINFO;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_AWARDS_CONFIGURATION:
        {
            return ACTION_STR_GET_AWARDS_CONFIGURATION;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_AWARDS_FOR_MEMBER:
        {
            return ACTION_STR_GET_AWARDS_FOR_MEMBER;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_AWARDS_FOR_SEASON:
        {
            return ACTION_STR_GET_AWARDS_FOR_SEASON;
        }
        case OSDKSeasonalPlayModule::ACTION_SIMULATE_PRODUCTION:
        {
            return ACTION_STR_SIMULATE_PRODUCTION;
        }
        case OSDKSeasonalPlayModule::ACTION_SLEEP:
        {
            return ACTION_STR_SLEEP;
        }
        case OSDKSeasonalPlayModule::ACTION_MAX_ENUM:
        {
            return "";
        }
        case OSDKSeasonalPlayModule::ACTION_TEST_NOTIFICATION:
        {
            return ACTION_STR_TEST_NOTIFICATION;
        }
    }
    return "";
}

/*** Private Methods *****************************************************************************/

OSDKSeasonalPlayModule::OSDKSeasonalPlayModule() : 
    mAction(ACTION_GET_SEASON_CONFIGURATION),
    mMaxSleepTime(0)
{
	memset(mEventSelectedProbabilities, 0, ACTION_MAX_ENUM);
	memset(mNoEventSelectedProbabilities, 0, ACTION_MAX_ENUM);
}

OSDKSeasonalPlayModule::~OSDKSeasonalPlayModule()
{
}

bool OSDKSeasonalPlayModule::readProbabilities(const ConfigMap* probabilityMap)
{
    bool probabilitiesRead = false;

    if (NULL != probabilityMap)
    {
        bool selectedRead = false;
        bool noSelectedRead = false;

        // 'event selected' probabilities
        const ConfigMap* currentMap = probabilityMap->getMap("eventSelected");
        uint32_t totalSelectedProbability = 0;

        if (currentMap != NULL)
        {
            totalSelectedProbability = populateProbabilityArray(currentMap, mEventSelectedProbabilities);
            if (totalSelectedProbability > 0 && totalSelectedProbability <= 100)
            {
                selectedRead = true;   
            }
        }
        else
        {
            BLAZE_ERR(BlazeRpcLog::osdkseasonalplay, "OSDKSeasonalPlayModule: no action probabilities provided / found for eventSelected");
        }

        // 'no event selected' probabilities
        currentMap = probabilityMap->getMap("noEventSelected");
        uint32_t totalNoSelectedProbability = 0;

        if (currentMap != NULL)
        {
            totalNoSelectedProbability = populateProbabilityArray(currentMap, mNoEventSelectedProbabilities);
            if (totalNoSelectedProbability > 0 && totalNoSelectedProbability <= 100)
            {
                noSelectedRead = true;
            }
        }
        else
        {
            BLAZE_ERR(BlazeRpcLog::osdkseasonalplay, "OSDKSeasonalPlayModule: no action probabilities provided / found for noEventSelected");
        }

        if (noSelectedRead && selectedRead)
        {
            // organize the probabilities
            for (int i = 1; i < ACTION_MAX_ENUM; ++i)
            {
                mEventSelectedProbabilities[i] = mEventSelectedProbabilities[i] + mEventSelectedProbabilities[i - 1];
                mNoEventSelectedProbabilities[i] = mNoEventSelectedProbabilities[i] + mNoEventSelectedProbabilities[i - 1];
            }

            probabilitiesRead = true;  
        }
    }
    else
    {
        BLAZE_ERR(BlazeRpcLog::osdkseasonalplay, "OSDKSeasonalPlayModule: no action probabilities provided / found");
    }

    return probabilitiesRead;
}

int32_t OSDKSeasonalPlayModule::populateProbabilityArray(const ConfigMap* probabilityMap, uint32_t eventProbabilityArray[])
{
    int32_t totalProbability = 0;

    while ( probabilityMap->hasNext() )
    {
        const char8_t* actionKeyStr = probabilityMap->nextKey();
        int32_t actionKeyVal = stringToAction(actionKeyStr);
        if (ACTION_MAX_ENUM == actionKeyVal)
        {
            BLAZE_WARN(BlazeRpcLog::osdkseasonalplay, "action %s is not a valid action string", actionKeyStr);
            continue;
        }
        uint32_t actionVal = probabilityMap->getUInt32(actionKeyStr, 0);
        eventProbabilityArray[actionKeyVal] = actionVal;
        totalProbability += actionVal;
    }

    return totalProbability;
}

// OSDKSeasonalPlayInstance

/*** Public Methods ******************************************************************************/

OSDKSeasonalPlayInstance::OSDKSeasonalPlayInstance(OSDKSeasonalPlayModule* owner, StressConnection* connection, Login* login) : 
    StressInstance(owner, connection, login, BlazeRpcLog::osdkseasonalplay),
    mPreviousAction(OSDKSeasonalPlayModule::ACTION_MAX_ENUM),
    mEventBegun(false),
    mEventEnded(false),
    mEventSelected(false),
    mSelectedEventIndex(0),
    mOwner(owner),
    mSeasonalPlaySlave( new OSDKSeasonalPlay::OSDKSeasonalPlaySlave(*connection)),
    mClubsSlave( new Clubs::ClubsSlave(*connection))
{
    mName = "";

	mCurrentSeasonId = 0;
}

void OSDKSeasonalPlayInstance::onLogin(BlazeRpcError result)
{
    mPreviousAction = OSDKSeasonalPlayModule::ACTION_SLEEP;

    BLAZE_TRACE_LOG(BlazeRpcLog::osdkseasonalplay, "[" << getConnection()->getIdent() << "] onLogin() - Blaze Id: ("
                    << getLogin()->getUserLoginInfo()->getBlazeId() << ")");
}

//void OSDKSeasonalPlayInstance::onDisconnected(BlazeRpcError result)
//{
//    // anything on disconnect?
//}

void OSDKSeasonalPlayInstance::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    if (OSDKSeasonalPlay::OSDKSeasonalPlaySlave::COMPONENT_ID == component)
    {
		BLAZE_TRACE_LOG(BlazeRpcLog::osdkseasonalplay, "[" << getConnection()->getIdent() << "] onAsync() - Blaze Id: ("
                        << getLogin()->getUserLoginInfo()->getBlazeId()<< ")");
	}
}

/*** Private Methods *****************************************************************************/

BlazeRpcError OSDKSeasonalPlayInstance::execute()
{
    BlazeRpcError result = ERR_OK;
    OSDKSeasonalPlayModule::Action action = mOwner->getAction();

    if (action == OSDKSeasonalPlayModule::ACTION_SIMULATE_PRODUCTION)
    {
        action = mOwner->getWeightedAction(true);
    }
    //BLAZE_DEBUG2( BlazeRpcLog::osdkseasonalplay, 
    //    "[%02d] choosing action: [%s], isEventSelected: [%d], id: [%u]", 
    //    getConnection()->getIdent(), OSDKSeasonalPlayModule::actionToString(action), mEventSelected, mSelectedEventId );

    switch (action)
    {
        case OSDKSeasonalPlayModule::ACTION_GET_SEASON_CONFIGURATION:
        {
            mName = ACTION_STR_GET_SEASON_CONFIGURATION;
            result = getSeasonConfiguration();
            break;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_SEASON_ID:
        {
            mName = ACTION_STR_GET_SEASON_ID;
			result = getSeasonId();
            break;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_SEASON_DETAILS:
        {
            mName = ACTION_STR_GET_SEASON_DETAILS;
            result = getSeasonDetails();
            break;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_MY_SEASON_DETAILS:
        {
            mName = ACTION_STR_GET_MY_SEASON_DETAILS;
            result = getMySeasonDetails();
            break;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_SEASON_RANKINGINFO:
        {
            mName = ACTION_STR_GET_SEASON_RANKINGINFO;
            result = getSeasonsRankingInfo();
            break;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_AWARDS_CONFIGURATION:
        {
            mName = ACTION_STR_GET_AWARDS_CONFIGURATION;
            result = getAwardConfiguration();
            break;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_AWARDS_FOR_MEMBER:
        {
            mName = ACTION_STR_GET_AWARDS_FOR_MEMBER;
            result = getAwardsForMember();
            break;
        }
        case OSDKSeasonalPlayModule::ACTION_GET_AWARDS_FOR_SEASON:
        {
            mName = ACTION_STR_GET_AWARDS_FOR_SEASON;
            result = getAwardsForSeason();
            break;
        }
        case OSDKSeasonalPlayModule::ACTION_SLEEP:
        {
            mName = ACTION_STR_SLEEP;
            sleep();
            break;
        }
        case OSDKSeasonalPlayModule::ACTION_SIMULATE_PRODUCTION:
        {
            //wont ever get in here, but have the case to get rid of compiler warning.
            break;
        }
        case OSDKSeasonalPlayModule::ACTION_MAX_ENUM:
        {
            break;
        }
		case OSDKSeasonalPlayModule::ACTION_TEST_NOTIFICATION:
        {
            break;
        }
    }

    mPreviousAction = action;

    return result;
}


void OSDKSeasonalPlayInstance::addStatistics(int32_t rpcIndex, BlazeRpcError rpcResult)
{
    TimeValue endTime = mLastRpcEndTime;
    endTime -= mLastRpcStartTime;
    rpcList[rpcIndex].ctr++;
    rpcList[rpcIndex].perCtr++;
    
    if (rpcResult != ERR_OK)
    {
        rpcList[rpcIndex].errCtr++;
        rpcList[rpcIndex].perErrCtr++;
    }
    else
    {
        rpcList[rpcIndex].durTotal += endTime.getMicroSeconds();
        rpcList[rpcIndex].durPeriod += endTime.getMicroSeconds();
    }    
}

void OSDKSeasonalPlayInstance::sleep()
{
	int32_t sleepTime = Blaze::Random::getRandomNumber(mOwner->getMaxSleepTime());
	BLAZE_TRACE_LOG(BlazeRpcLog::osdkseasonalplay, "[" << getConnection()->getIdent() << "] OSDKSeasonalPlayInstance sleeping for " << sleepTime << " ms!");
    StressInstance::sleep(sleepTime);
}

BlazeRpcError OSDKSeasonalPlayInstance::getSeasonConfiguration()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::osdkseasonalplay, "[" << getConnection()->getIdent() << "] get season configuration ...");

	// Get all of seasons
    mLastRpcStartTime = TimeValue::getTimeOfDay();

	OSDKSeasonalPlay::GetSeasonConfigurationResponse response;
	BlazeRpcError result = mSeasonalPlaySlave->getSeasonConfiguration(response);

    mLastRpcEndTime = TimeValue::getTimeOfDay();

	addStatistics(OSDKSeasonalPlayModule::ACTION_GET_SEASON_CONFIGURATION, result);

    return result;
}


const char8_t* OSDKSeasonalPlayInstance::getRandomLang()
{
    static char8_t language[3];
    language[0] = 'A' + static_cast<char8_t>(Blaze::Random::getRandomNumber('Z' - 'A' + 1));
    language[1] = 'A' + static_cast<char8_t>(Blaze::Random::getRandomNumber('Z' - 'A' + 1));
    language[2] = '\0';
    return language;
}

const char8_t* OSDKSeasonalPlayInstance::getENLang()
{
	return "en";
}

uint32_t OSDKSeasonalPlayInstance::getENRegion()
{
	return 21843;
}

Clubs::ClubId OSDKSeasonalPlayInstance::getMyClubId()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::osdkseasonalplay, "[" << getConnection()->getIdent() << "] get clubs ...");

    Clubs::FindClubsRequest request;
    Clubs::FindClubsResponse response;

    request.setMaxResultCount(10);
    request.setAnyTeamId(true);
    request.setAnyDomain(true);
	request.setName("");

	// Current user id
	BlazeId currentUserId = getLogin()->getUserLoginInfo()->getBlazeId();
	request.getMemberFilterList().push_back(currentUserId);

    if (true)
    {
        request.setSkipMetadata(1);
    }
        
    BlazeRpcError result = mClubsSlave->findClubs(request, response);
    
	Clubs::ClubId id = 0;
    if (result == Blaze::ERR_OK)
	{
		Clubs::ClubList& clubList = response.getClubList();
		if (clubList.size() > 0)
		{
			Clubs::Club* club = clubList.front();
			id = club->getClubId();
		}
	}

	return id;
}

// if mCurrentSeasonId isn't set, get the season id by calling getSeasonId
bool8_t OSDKSeasonalPlayInstance::verifySeasonId()
{
    bool8_t bResult = true;

    if (0 == mCurrentSeasonId)
    {
        BlazeRpcError result = getSeasonId();
        if (Blaze::ERR_OK != result)
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::osdkseasonalplay, "[" << getConnection()->getIdent() << "] unable to retrieve season id");
            bResult = false;
        }
    }

    return bResult;
}

BlazeRpcError OSDKSeasonalPlayInstance::getSeasonId()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::osdkseasonalplay, "[" << getConnection()->getIdent() << "] get season id ...");

    mLastRpcStartTime = TimeValue::getTimeOfDay();

	OSDKSeasonalPlay::GetSeasonIdRequest request;
	OSDKSeasonalPlay::GetSeasonIdResponse response;
	request.setMemberType(OSDKSeasonalPlay::SEASONALPLAY_MEMBERTYPE_CLUB);
	Clubs::ClubId id = getMyClubId();
	request.setMemberId(id);
	BlazeRpcError result = mSeasonalPlaySlave->getSeasonId( request, response);

    mLastRpcEndTime = TimeValue::getTimeOfDay();

	addStatistics(OSDKSeasonalPlayModule::ACTION_GET_SEASON_ID, result);

    // set the current season id
    if (Blaze::ERR_OK == result)
    {
        mCurrentSeasonId = response.getSeasonId();
    }

    return result;

}

BlazeRpcError OSDKSeasonalPlayInstance::getSeasonDetails()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::osdkseasonalplay, "[" << getConnection()->getIdent() << "] getSeasonDetails ...");
	
	BlazeRpcError result = ERR_OK;
	// If a current season is assigned to the current stress instance
	if (true == verifySeasonId())
	{
		mLastRpcStartTime = TimeValue::getTimeOfDay();

		OSDKSeasonalPlay::GetSeasonDetailsRequest request;
		request.setSeasonId(mCurrentSeasonId); 

		OSDKSeasonalPlay::SeasonDetails seasonDetails; 
		result = mSeasonalPlaySlave->getSeasonDetails(request, seasonDetails);

		mLastRpcEndTime = TimeValue::getTimeOfDay();

		// If the season is not found, it is not an error
		if (result != OSDKSEASONALPLAY_ERR_SEASON_NOT_FOUND)
		{
			addStatistics(OSDKSeasonalPlayModule::ACTION_GET_SEASON_DETAILS, result);
		}
	}

	return result;
}

BlazeRpcError OSDKSeasonalPlayInstance::getMySeasonDetails()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::osdkseasonalplay, "[" << getConnection()->getIdent() << "] get getMySeasonDetails ...");

    BlazeRpcError result = ERR_OK;
    if (true == verifySeasonId())
    {
	    OSDKSeasonalPlay::GetMySeasonDetailsRequest request;
	    Clubs::ClubId id = getMyClubId();
	    request.setMemberId(id);
	    request.setSeasonId(mCurrentSeasonId);

	    request.setMemberType(OSDKSeasonalPlay::SEASONALPLAY_MEMBERTYPE_CLUB);

	    OSDKSeasonalPlay::MySeasonDetails seasonDetails; 
        mLastRpcStartTime = TimeValue::getTimeOfDay();
	    result = mSeasonalPlaySlave->getMySeasonDetails(request, seasonDetails);
        mLastRpcEndTime = TimeValue::getTimeOfDay();

	    // If the member is not registered to this season, it is not an error
	    if (result != OSDKSEASONALPLAY_ERR_NOT_REGISTERED)
	    {
		    addStatistics(OSDKSeasonalPlayModule::ACTION_GET_MY_SEASON_DETAILS, result);
	    }
    }
    
    return result;
}

BlazeRpcError OSDKSeasonalPlayInstance::getSeasonsRankingInfo()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::osdkseasonalplay, "[" << getConnection()->getIdent() << "] get seasons ranking info ...");

    mLastRpcStartTime = TimeValue::getTimeOfDay();
	OSDKSeasonalPlay::GetSeasonsRankingInfoResponse response;
	BlazeRpcError result = mSeasonalPlaySlave->getSeasonsRankingInfo(response);
    mLastRpcEndTime = TimeValue::getTimeOfDay();

	addStatistics(OSDKSeasonalPlayModule::ACTION_GET_SEASON_RANKINGINFO, result);

	return result;
}

BlazeRpcError OSDKSeasonalPlayInstance::getAwardConfiguration()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::osdkseasonalplay, "[" << getConnection()->getIdent() << "] getAwardConfiguration ...");

    mLastRpcStartTime = TimeValue::getTimeOfDay();
	OSDKSeasonalPlay::GetAwardConfigurationResponse response;
	BlazeRpcError result = mSeasonalPlaySlave->getAwardConfiguration(response);
    mLastRpcEndTime = TimeValue::getTimeOfDay();

	addStatistics(OSDKSeasonalPlayModule::ACTION_GET_AWARDS_CONFIGURATION, result);

	return result;
}

BlazeRpcError OSDKSeasonalPlayInstance::getAwardsForMember()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::osdkseasonalplay, "[" << getConnection()->getIdent() << "] getAwardsForMember ...");

    BlazeRpcError result = Blaze::ERR_OK;
    if (true == verifySeasonId())
    {
        mLastRpcStartTime = TimeValue::getTimeOfDay();
        OSDKSeasonalPlay::GetAwardsForMemberRequest request;
        request.setMemberType(OSDKSeasonalPlay::SEASONALPLAY_MEMBERTYPE_CLUB);
        Clubs::ClubId id = getMyClubId();
        request.setMemberId(id);
        request.setSeasonId(mCurrentSeasonId);
        request.setSeasonNumber(uint32_t(OSDKSeasonalPlay::AWARD_ANY_SEASON_NUMBER));
        OSDKSeasonalPlay::GetAwardsResponse response;
        result = mSeasonalPlaySlave->getAwardsForMember(request, response);
        mLastRpcEndTime = TimeValue::getTimeOfDay();

        addStatistics(OSDKSeasonalPlayModule::ACTION_GET_AWARDS_FOR_MEMBER, result);
    }

	return result;
}

BlazeRpcError OSDKSeasonalPlayInstance::getAwardsForSeason()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::osdkseasonalplay, "[" << getConnection()->getIdent() << "] getAwardsForSeason ...");

    BlazeRpcError result = Blaze::ERR_OK;
    if (true == verifySeasonId())
    {
        mLastRpcStartTime = TimeValue::getTimeOfDay();
        OSDKSeasonalPlay::GetAwardsForSeasonRequest request;
        request.setSeasonId(mCurrentSeasonId);
        request.setSeasonNumber(uint32_t(OSDKSeasonalPlay::AWARD_ANY_SEASON_NUMBER));
        OSDKSeasonalPlay::GetAwardsResponse response;
        result = mSeasonalPlaySlave->getAwardsForSeason(request, response);
        mLastRpcEndTime = TimeValue::getTimeOfDay();

        addStatistics(OSDKSeasonalPlayModule::ACTION_GET_AWARDS_FOR_SEASON, result);
    }
	return result;
}

} // Stress
} // Blaze

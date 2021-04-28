/*************************************************************************************************/
/*!
    \file FifaCupsModule.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/
#if 0
#include "framework/blaze.h"
#include "fifacupsmodule.h"
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
static const char8_t* ACTION_STR_GET_CUP_INFO = "getCupInfo";
static const char8_t* ACTION_STR_RESET_CUP_STATUS = "resetCupStatus";

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
	{"getCupInfo"},
	{"resetCupStatus"}
};

// FifaCupsModule
/*** Public Methods ******************************************************************************/

// static
StressModule* FifaCupsModule::create()
{
    return BLAZE_NEW FifaCupsModule();
}

bool FifaCupsModule::initialize(const ConfigMap& config)
{
    memset(&mEventSelectedProbabilities[0], 0, sizeof(uint32_t) * ACTION_MAX_ENUM);
    memset(&mNoEventSelectedProbabilities[0], 0, sizeof(uint32_t) * ACTION_MAX_ENUM);

    bool initialized = StressModule::initialize(config);

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
                ERR_LOG("FifaCupsModule: no simulation data provided / found");
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
					ERR_LOG("FifaCupsModule: no notification test data provided / found");
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

StressInstance* FifaCupsModule::createInstance(StressConnection* connection, Login* login)
{
    return BLAZE_NEW FifaCupsInstance(this, connection, login);
}

FifaCupsModule::Action FifaCupsModule::getWeightedAction(bool isEventSelected) const
{
    uint32_t rand = static_cast<uint32_t>( Blaze::Random::getRandomNumber(100) );
    FifaCupsModule::Action action = ACTION_MAX_ENUM;
    
    if (isEventSelected)
    {
        for (int i = 0; i < ACTION_MAX_ENUM; ++i)
        {
            uint32_t current = mEventSelectedProbabilities[i];
            
            if (rand < current)
            {
                action = static_cast<FifaCupsModule::Action>(i);
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
                action = static_cast<FifaCupsModule::Action>(i);
                break;
            }
        }
    }

    if (action == ACTION_MAX_ENUM)
    {
        ERR_LOG("FifaCupsModule: somehow arrived at invalid action - rand: "<<rand<<" ");
    }

    return action;
}

FifaCupsModule::Action FifaCupsModule::stringToAction(const char8_t* action)
{
    FifaCupsModule::Action convertedAction = ACTION_MAX_ENUM;

	if (blaze_stricmp(action, ACTION_STR_GET_CUP_INFO) == 0)
	{
	    convertedAction = ACTION_GET_CUP_INFO;
	}
	else if (blaze_stricmp(action, ACTION_STR_RESET_CUP_STATUS) == 0)
	{
		convertedAction = ACTION_RESET_CUP_STATUS;
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
        ERR_LOG("FifaCupsModule: unrecognized action: "<<action<<" ");
    }

    return convertedAction;
}

const char8_t* FifaCupsModule::actionToString(FifaCupsModule::Action action)
{
    switch (action)
    {
		case FifaCupsModule::ACTION_GET_CUP_INFO:
		{
		    return ACTION_STR_GET_CUP_INFO;
		}
		case FifaCupsModule::ACTION_RESET_CUP_STATUS:
		{
			return ACTION_STR_RESET_CUP_STATUS;
		}
        case FifaCupsModule::ACTION_SIMULATE_PRODUCTION:
        {
            return ACTION_STR_SIMULATE_PRODUCTION;
        }
        case FifaCupsModule::ACTION_SLEEP:
        {
            return ACTION_STR_SLEEP;
        }
        case FifaCupsModule::ACTION_MAX_ENUM:
        {
            return "";
        }
        case FifaCupsModule::ACTION_TEST_NOTIFICATION:
        {
            return ACTION_STR_TEST_NOTIFICATION;
        }
    }
    return "";
}

/*** Private Methods *****************************************************************************/

FifaCupsModule::FifaCupsModule() : 
    mAction(ACTION_GET_CUP_INFO),
    mMaxSleepTime(0)
{
}

FifaCupsModule::~FifaCupsModule()
{
}

bool FifaCupsModule::readProbabilities(const ConfigMap* probabilityMap)
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
            ERR_LOG("FifaCupsModule: no action probabilities provided / found for eventSelected");
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
            ERR_LOG("FifaCupsModule: no action probabilities provided / found for noEventSelected");
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
        ERR_LOG("FifaCupsModule: no action probabilities provided / found");
    }

    return probabilitiesRead;
}

int32_t FifaCupsModule::populateProbabilityArray(const ConfigMap* probabilityMap, uint32_t eventProbabilityArray[])
{
    int32_t totalProbability = 0;

    while ( probabilityMap->hasNext() )
    {
        const char8_t* actionKeyStr = probabilityMap->nextKey();
        int32_t actionKeyVal = stringToAction(actionKeyStr);
        if (ACTION_MAX_ENUM == actionKeyVal)
        {
            WARN_LOG("action "<<actionKeyStr<<" is not a valid action string");
            continue;
        }
        uint32_t actionVal = probabilityMap->getUInt32(actionKeyStr, 0);
        eventProbabilityArray[actionKeyVal] = actionVal;
        totalProbability += actionVal;
    }

    return totalProbability;
}

// FifaCupsInstance

/*** Public Methods ******************************************************************************/

FifaCupsInstance::FifaCupsInstance(FifaCupsModule* owner, StressConnection* connection, Login* login) : 
    StressInstance(owner, connection, login, BlazeRpcLog::fifacups),
    mPreviousAction(FifaCupsModule::ACTION_MAX_ENUM),
    mEventBegun(false),
    mEventEnded(false),
    mEventSelected(false),
    mSelectedEventIndex(0),
    mOwner(owner),
    mSeasonalPlayProxy( BLAZE_NEW FifaCups::FifaCupsSlaveProxy(connection->getAddress(), *connection))
{
    mName = "";
}

void FifaCupsInstance::onLogin(BlazeRpcError result)
{
    mPreviousAction = FifaCupsModule::ACTION_SLEEP;

    TRACE_LOG("["<<getConnection()->getIdent()<<"] onLogin() - Blaze Id: "<<getLogin()->getSessionInfo()->getBlazeUserId()<<" " );
}

//void FifaCupsInstance::onDisconnected(BlazeRpcError result)
//{
//    // anything on disconnect?
//}

void FifaCupsInstance::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    if (FifaCups::FifaCupsSlave::COMPONENT_ID == component)
    {
		TRACE_LOG("["<<getConnection()->getIdent()<<"] onAsync() - Blaze Id: "<<getLogin()->getSessionInfo()->getBlazeUserId()<<" ");
	}
}

/*** Private Methods *****************************************************************************/

BlazeRpcError FifaCupsInstance::execute()
{
    BlazeRpcError result = ERR_OK;
    FifaCupsModule::Action action = mOwner->getAction();

    if (action == FifaCupsModule::ACTION_SIMULATE_PRODUCTION)
    {
        action = mOwner->getWeightedAction(true);
    }
    TRACE_LOG("["<<getConnection()->getIdent()<<"] choosing action: ["<<FifaCupsModule::actionToString(action)<<"]");

    switch (action)
    {
		case FifaCupsModule::ACTION_GET_CUP_INFO:
		{
			mName = ACTION_STR_GET_CUP_INFO;
			result = getCupInfo();
			break;
		}
		case FifaCupsModule::ACTION_RESET_CUP_STATUS:
		{
			mName = ACTION_STR_RESET_CUP_STATUS;
			result = resetCupStatus();
			break;
		}
        case FifaCupsModule::ACTION_SLEEP:
        {
            mName = ACTION_STR_SLEEP;
            sleep();
            break;
        }
        case FifaCupsModule::ACTION_SIMULATE_PRODUCTION:
        {
            //wont ever get in here, but have the case to get rid of compiler warning.
            break;
        }
        case FifaCupsModule::ACTION_MAX_ENUM:
        {
            break;
        }
		case FifaCupsModule::ACTION_TEST_NOTIFICATION:
        {
            break;
        }
    }

    mPreviousAction = action;

    return result;
}


void FifaCupsInstance::addStatistics(int32_t rpcIndex, BlazeRpcError rpcResult)
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

void FifaCupsInstance::sleep()
{
	int32_t sleepTime = Blaze::Random::getRandomNumber(mOwner->getMaxSleepTime());
	TRACE_LOG("["<<getConnection()->getIdent()<<"] FifaCupsInstance sleeping for "<<sleepTime<<" ms!");
    StressInstance::sleep(sleepTime);
}

BlazeRpcError FifaCupsInstance::getCupInfo()
{
    TRACE_LOG("["<<getConnection()->getIdent()<<"]  get getCupInfo ...");

    mLastRpcStartTime = TimeValue::getTimeOfDay();

	FifaCups::GetCupInfoRequest request;
	FifaCups::CupInfo response;

	//set member id and member type
	BlazeId currentUserId = getLogin()->getSessionInfo()->getBlazeUserId();
	request.setMemberId(currentUserId);
	request.setMemberType(FifaCups::FIFACUPS_MEMBERTYPE_USER);

	BlazeRpcError result = mSeasonalPlayProxy->getCupInfo(request, response);

    mLastRpcEndTime = TimeValue::getTimeOfDay();

	addStatistics(FifaCupsModule::ACTION_GET_CUP_INFO, result);

    return result;
}

BlazeRpcError FifaCupsInstance::resetCupStatus()
{
    TRACE_LOG("["<<getConnection()->getIdent()<<"]  get resetCupStatus ...");

    mLastRpcStartTime = TimeValue::getTimeOfDay();

	FifaCups::ResetCupStatusRequest request;

	//set member id and member type
	BlazeId currentUserId = getLogin()->getSessionInfo()->getBlazeUserId();

	request.setMemberId(currentUserId);
	request.setMemberType(FifaCups::FIFACUPS_MEMBERTYPE_USER);

	BlazeRpcError result = mSeasonalPlayProxy->resetCupStatus(request);

    mLastRpcEndTime = TimeValue::getTimeOfDay();

	addStatistics(FifaCupsModule::ACTION_RESET_CUP_STATUS, result);

    return result;
}

} // Stress
} // Blaze
#endif


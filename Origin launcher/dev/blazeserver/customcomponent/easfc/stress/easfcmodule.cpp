/*************************************************************************************************/
/*!
    \file EasfcModule.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/
#if 0

#include "framework/blaze.h"
#include "easfcmodule.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/random.h"
#include "loginmanager.h"

namespace Blaze
{
namespace Stress
{

/*
* List of available actions that can be performed by the module.
*/
static const char8_t* ACTION_STR_PURCHASE_WIN_GAME = "purchaseGameWin";

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
	//{"getCupInfo"},
};

// EasfcModule
/*** Public Methods ******************************************************************************/

// static
StressModule* EasfcModule::create()
{
    return BLAZE_NEW EasfcModule();
}

bool EasfcModule::initialize(const ConfigMap& config)
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
                ERR_LOG("EasfcModule: no simulation data provided / found");
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
					ERR_LOG("EasfcModule: no notification test data provided / found");
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

StressInstance* EasfcModule::createInstance(StressConnection* connection, Login* login)
{
    return BLAZE_NEW EasfcInstance(this, connection, login);
}

EasfcModule::Action EasfcModule::getWeightedAction(bool isEventSelected) const
{
    uint32_t rand = static_cast<uint32_t>( Blaze::Random::getRandomNumber(100) );
    EasfcModule::Action action = ACTION_MAX_ENUM;
    
    if (isEventSelected)
    {
        for (int i = 0; i < ACTION_MAX_ENUM; ++i)
        {
            uint32_t current = mEventSelectedProbabilities[i];
            
            if (rand < current)
            {
                action = static_cast<EasfcModule::Action>(i);
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
                action = static_cast<EasfcModule::Action>(i);
                break;
            }
        }
    }

    if (action == ACTION_MAX_ENUM)
    {
        ERR_LOG("EasfcModule: somehow arrived at invalid action - rand: "<<rand<<" ");
    }

    return action;
}

EasfcModule::Action EasfcModule::stringToAction(const char8_t* action)
{
    EasfcModule::Action convertedAction = ACTION_MAX_ENUM;

	if (blaze_stricmp(action, ACTION_STR_PURCHASE_WIN_GAME) == 0)
	{
	    convertedAction = ACTION_PURCHASE_WIN_GAME;
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
        ERR_LOG("EasfcModule: unrecognized action: "<<action<<" ");
    }

    return convertedAction;
}

const char8_t* EasfcModule::actionToString(EasfcModule::Action action)
{
    switch (action)
    {
		case EasfcModule::ACTION_PURCHASE_WIN_GAME:
		{
		    return ACTION_STR_PURCHASE_WIN_GAME;
		}
        case EasfcModule::ACTION_SIMULATE_PRODUCTION:
        {
            return ACTION_STR_SIMULATE_PRODUCTION;
        }
        case EasfcModule::ACTION_SLEEP:
        {
            return ACTION_STR_SLEEP;
        }
        case EasfcModule::ACTION_MAX_ENUM:
        {
            return "";
        }
        case EasfcModule::ACTION_TEST_NOTIFICATION:
        {
            return ACTION_STR_TEST_NOTIFICATION;
        }
    }
    return "";
}

/*** Private Methods *****************************************************************************/

EasfcModule::EasfcModule() : 
    mAction(ACTION_GET_CUP_INFO),
    mMaxSleepTime(0)
{
}

EasfcModule::~EasfcModule()
{
}

bool EasfcModule::readProbabilities(const ConfigMap* probabilityMap)
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
            ERR_LOG("EasfcModule: no action probabilities provided / found for eventSelected");
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
            ERR_LOG("EasfcModule: no action probabilities provided / found for noEventSelected");
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
        ERR_LOG("EasfcModule: no action probabilities provided / found");
    }

    return probabilitiesRead;
}

int32_t EasfcModule::populateProbabilityArray(const ConfigMap* probabilityMap, uint32_t eventProbabilityArray[])
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

// EasfcInstance

/*** Public Methods ******************************************************************************/

EasfcInstance::EasfcInstance(EasfcModule* owner, StressConnection* connection, Login* login) : 
    StressInstance(owner, connection, login, BlazeRpcLog::easfc),
    mPreviousAction(EasfcModule::ACTION_MAX_ENUM),
    mEventBegun(false),
    mEventEnded(false),
    mEventSelected(false),
    mSelectedEventIndex(0),
    mOwner(owner),
    mEasfcProxy( BLAZE_NEW Easfc::EasfcSlaveProxy(connection->getAddress(), *connection))
{
    mName = "";
}

void EasfcInstance::onLogin(BlazeRpcError result)
{
    mPreviousAction = EasfcModule::ACTION_SLEEP;

    TRACE_LOG("["<<getConnection()->getIdent()<<"] onLogin() - Blaze Id: "<<getLogin()->getSessionInfo()->getBlazeUserId()<<" " );
}

//void EasfcInstance::onDisconnected(BlazeRpcError result)
//{
//    // anything on disconnect?
//}

void EasfcInstance::onAsync(uint16_t component, uint16_t type, RawBuffer* payload)
{
    if (EASFC::EasfcSlave::COMPONENT_ID == component)
    {
		TRACE_LOG("["<<getConnection()->getIdent()<<"] onAsync() - Blaze Id: "<<getLogin()->getSessionInfo()->getBlazeUserId()<<" ");
	}
}

/*** Private Methods *****************************************************************************/

BlazeRpcError EasfcInstance::execute()
{
    BlazeRpcError result = ERR_OK;
    EasfcModule::Action action = mOwner->getAction();

    if (action == EasfcModule::ACTION_SIMULATE_PRODUCTION)
    {
        action = mOwner->getWeightedAction(true);
    }
    TRACE_LOG("["<<getConnection()->getIdent()<<"] choosing action: ["<<EasfcModule::actionToString(action)<<"]");

    switch (action)
    {
		case EasfcModule::ACTION_PURCHASE_WIN_GAME:
		{
			mName = ACTION_STR_PURCHASE_WIN_GAME;
			//result = purchaseGameWin();
			break;
		}
        case EasfcModule::ACTION_SLEEP:
        {
            mName = ACTION_STR_SLEEP;
            sleep();
            break;
        }
        case EasfcModule::ACTION_SIMULATE_PRODUCTION:
        {
            //wont ever get in here, but have the case to get rid of compiler warning.
            break;
        }
        case EasfcModule::ACTION_MAX_ENUM:
        {
            break;
        }
		case EasfcModule::ACTION_TEST_NOTIFICATION:
        {
            break;
        }
    }

    mPreviousAction = action;

    return result;
}


void EasfcInstance::addStatistics(int32_t rpcIndex, BlazeRpcError rpcResult)
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

void EasfcInstance::sleep()
{
	int32_t sleepTime = Blaze::Random::getRandomNumber(mOwner->getMaxSleepTime());
	TRACE_LOG("["<<getConnection()->getIdent()<<"] EasfcInstance sleeping for "<<sleepTime<<" ms!");
    StressInstance::sleep(sleepTime);
}

/*
BlazeRpcError EasfcInstance::purchaseGameWin()
{
    TRACE_LOG("["<<getConnection()->getIdent()<<"]  get purchaseGameWin ...");

    mLastRpcStartTime = TimeValue::getTimeOfDay();

	Easfc::GetCupInfoRequest request;
	Easfc::CupInfo response;

	//set member id and member type
	BlazeId currentUserId = getLogin()->getSessionInfo()->getBlazeUserId();
	request.setMemberId(currentUserId);
	request.setMemberType(Easfc::EASFC_MEMBERTYPE_USER);

	BlazeRpcError result = mSeasonalPlayProxy->getCupInfo(request, response);

    mLastRpcEndTime = TimeValue::getTimeOfDay();

	addStatistics(EasfcModule::ACTION_GET_CUP_INFO, result);

    return result;
}
*/
} // Stress
} // Blaze
#endif


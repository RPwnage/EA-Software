/*************************************************************************************************/
/*!
    \file OSDKSeasonalPlayModule.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_OSDKSeasonalPlayModule_H
#define BLAZE_STRESS_OSDKSeasonalPlayModule_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "stressmodule.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave.h"
#include "clubs/rpc/clubsslave.h"
#include "stressinstance.h"
#include "EASTL/vector.h"
#include "clubs/tdf/clubs.h"

namespace Blaze
{
namespace Stress
{
class StressInstance;
class Login;
class OSDKSeasonalPlayInstance;

class OSDKSeasonalPlayModule : public StressModule
{
    NON_COPYABLE(OSDKSeasonalPlayModule);

public:
	//
	// Note: This enum is used as an index to access the rpcList. 
	// Check the rpcList when the order of the enums is changed
	//
    typedef enum
    {
		// Get methods
		ACTION_GET_SEASON_CONFIGURATION = 0,
		ACTION_GET_SEASON_ID,

        ACTION_GET_SEASON_DETAILS,
        ACTION_GET_MY_SEASON_DETAILS,
        ACTION_GET_SEASON_RANKINGINFO,

		ACTION_GET_AWARDS_CONFIGURATION,
		ACTION_GET_AWARDS_FOR_MEMBER,
		ACTION_GET_AWARDS_FOR_SEASON,

        ACTION_SIMULATE_PRODUCTION,
        ACTION_SLEEP,
        ACTION_MAX_ENUM,
        ACTION_TEST_NOTIFICATION
    } Action;

    virtual ~OSDKSeasonalPlayModule();

    virtual const char8_t*  getModuleName()						{ return "osdkseasonalplay"; };

    virtual bool            initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil);
    virtual StressInstance* createInstance(StressConnection* connection, Login* login);

    static Action           stringToAction(const char8_t* action);
    static const char8_t*   actionToString(OSDKSeasonalPlayModule::Action action);
    static StressModule*	create();
    
		   Action			getAction() const					{ return mAction; }
		   Action			getWeightedAction(bool isEventSelected) const;

		   uint32_t			getMaxSleepTime() const				{ return mMaxSleepTime; }


private:
			    OSDKSeasonalPlayModule();

    bool        readProbabilities(const ConfigMap* probabilityMap);
    int32_t     populateProbabilityArray(const ConfigMap* probabilityMap, uint32_t eventProbabilityArray[]);

    Action      mAction;  
    uint32_t    mMaxSleepTime;
    uint32_t    mEventSelectedProbabilities[ACTION_MAX_ENUM];
    uint32_t    mNoEventSelectedProbabilities[ACTION_MAX_ENUM];
};

class OSDKSeasonalPlayInstance : 
    public StressInstance, 
    public AsyncHandler
{
    NON_COPYABLE(OSDKSeasonalPlayInstance);

public:
    OSDKSeasonalPlayInstance(OSDKSeasonalPlayModule* owner, StressConnection* connection, Login* login);

    virtual ~OSDKSeasonalPlayInstance() 
	{
		delete mSeasonalPlaySlave;
		delete mClubsSlave;
	}

    virtual void    onLogin(BlazeRpcError result);
    //virtual void    onDisconnected();
    virtual void    onAsync(uint16_t component, uint16_t type, RawBuffer* payload);

private:
    const char8_t*  getName() const { return mName; }
    BlazeRpcError   execute();

	// Latest Rpc-run stats
    TimeValue		mLastRpcStartTime;
    TimeValue		mLastRpcEndTime;
	// Add the RPC statistics
    void			addStatistics(int32_t rpcIndex, BlazeRpcError rpcResult);

	// The current selected season id
	OSDKSeasonalPlay::SeasonId mCurrentSeasonId;

	const char8_t*	getRandomLang();
    static const char8_t* getENLang();
	static uint32_t getENRegion();

	Clubs::ClubId	getMyClubId();

    // checks if the season id is set for the current user and if not, gets it by
    // calling getSeasonId. Return value indicates if season id was set/fetched successfully
    bool8_t         verifySeasonId();

	// Seasons related stress test
	BlazeRpcError   getSeasonConfiguration();
	BlazeRpcError	getSeasonId();
	BlazeRpcError	getSeasonDetails();
	BlazeRpcError	getMySeasonDetails();
	BlazeRpcError	getSeasonsRankingInfo();

	// Awards related stress tests
	BlazeRpcError   getAwardConfiguration();
	BlazeRpcError   getAwardsForMember();
	BlazeRpcError   getAwardsForSeason();

    void            sleep();

private:
    OSDKSeasonalPlayModule::Action mPreviousAction;

    bool        mEventBegun;
    bool        mEventEnded;

    bool        mEventSelected;
    int32_t     mSelectedEventIndex;

    
    OSDKSeasonalPlayModule* mOwner;
    OSDKSeasonalPlay::OSDKSeasonalPlaySlave* mSeasonalPlaySlave;
	Clubs::ClubsSlave* mClubsSlave;
    const char8_t *mName;
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_OSDKSeasonalPlayModule_H


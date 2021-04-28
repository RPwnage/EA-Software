/*************************************************************************************************/
/*!
    \file FifaCupsModule.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_FIFACUPSMODULE_H
#define BLAZE_STRESS_FIFACUPSMODULE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "stressmodule.h"
#include "blazerpclog.h"
#include "fifacups/rpc/fifacupsslave_proxy.h"
#include "fifacups/tdf/fifacupstypes.h"
#include "clubs/rpc/clubsslave_proxy.h"
#include "stressinstance.h"
#include "EASTL/vector.h"
#include "clubs/tdf/clubs.h"

namespace Blaze
{
namespace Stress
{
class StressInstance;
class Login;
class FifaCupsInstance;

class FifaCupsModule : public StressModule
{
    NON_COPYABLE(FifaCupsModule);

public:
	//
	// Note: This enum is used as an index to access the rpcList. 
	// Check the rpcList when the order of the enums is changed
	//
    typedef enum
    {
		// Get methods
		ACTION_GET_CUP_INFO = 0,
		ACTION_RESET_CUP_STATUS,

        ACTION_SIMULATE_PRODUCTION,
        ACTION_SLEEP,
        ACTION_MAX_ENUM,
        ACTION_TEST_NOTIFICATION
    } Action;

    virtual ~FifaCupsModule();

    virtual const char8_t*  getModuleName()						{ return "fifacups"; };

    virtual bool            initialize(const ConfigMap& config);
    virtual StressInstance* createInstance(StressConnection* connection, Login* login);

    static Action           stringToAction(const char8_t* action);
    static const char8_t*   actionToString(FifaCupsModule::Action action);
    static StressModule*	create();
    
		   Action			getAction() const					{ return mAction; }
		   Action			getWeightedAction(bool isEventSelected) const;

		   uint32_t			getMaxSleepTime() const				{ return mMaxSleepTime; }


private:
			    FifaCupsModule();

    bool        readProbabilities(const ConfigMap* probabilityMap);
    int32_t     populateProbabilityArray(const ConfigMap* probabilityMap, uint32_t eventProbabilityArray[]);

    Action      mAction;  
    uint32_t    mMaxSleepTime;
    uint32_t    mEventSelectedProbabilities[ACTION_MAX_ENUM];
    uint32_t    mNoEventSelectedProbabilities[ACTION_MAX_ENUM];
};

class FifaCupsInstance : 
    public StressInstance, 
    public AsyncHandler
{
    NON_COPYABLE(FifaCupsInstance);

public:
    FifaCupsInstance(FifaCupsModule* owner, StressConnection* connection, Login* login);

    virtual ~FifaCupsInstance() 
	{
		delete mSeasonalPlayProxy;
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

	// Seasons related stress test
	BlazeRpcError   getCupInfo();
	BlazeRpcError   resetCupStatus();

    void            sleep();

private:
    FifaCupsModule::Action mPreviousAction;

    bool        mEventBegun;
    bool        mEventEnded;

    bool        mEventSelected;
    int32_t     mSelectedEventIndex;

    
    FifaCupsModule* mOwner;
    FifaCups::FifaCupsSlaveProxy* mSeasonalPlayProxy;
    const char8_t *mName;
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_FIFACUPSMODULE_H


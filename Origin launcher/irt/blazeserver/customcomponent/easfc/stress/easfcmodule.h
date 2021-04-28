/*************************************************************************************************/
/*!
    \file EasfcModule.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_EASFCMODULE_H
#define BLAZE_STRESS_EASFCMODULE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "stressmodule.h"
#include "blazerpclog.h"
#include "easfc/rpc/easfcslave_proxy.h"
#include "easfc/tdf/easfctypes.h"
#include "stressinstance.h"
#include "EASTL/vector.h"

namespace Blaze
{
namespace Stress
{
class StressInstance;
class Login;
class EasfcInstance;

class EasfcModule : public StressModule
{
    NON_COPYABLE(EasfcModule);

public:
	//
	// Note: This enum is used as an index to access the rpcList. 
	// Check the rpcList when the order of the enums is changed
	//
    typedef enum
    {
		// Get methods
		ACTION_PURCHASE_WIN_GAME = 0,

		ACTION_SLEEP,
        ACTION_MAX_ENUM,
        ACTION_TEST_NOTIFICATION
    } Action;

    virtual ~EasfcModule();

    virtual const char8_t*  getModuleName()						{ return "easfc"; };

    virtual bool            initialize(const ConfigMap& config);
    virtual StressInstance* createInstance(StressConnection* connection, Login* login);

    static Action           stringToAction(const char8_t* action);
    static const char8_t*   actionToString(EasfcModule::Action action);
    static StressModule*	create();
    
		   Action			getAction() const					{ return mAction; }
		   Action			getWeightedAction(bool isEventSelected) const;

		   uint32_t			getMaxSleepTime() const				{ return mMaxSleepTime; }


private:
			    EasfcModule();

    bool        readProbabilities(const ConfigMap* probabilityMap);
    int32_t     populateProbabilityArray(const ConfigMap* probabilityMap, uint32_t eventProbabilityArray[]);

    Action      mAction;  
    uint32_t    mMaxSleepTime;
    uint32_t    mEventSelectedProbabilities[ACTION_MAX_ENUM];
    uint32_t    mNoEventSelectedProbabilities[ACTION_MAX_ENUM];
};

class EasfcInstance : 
    public StressInstance, 
    public AsyncHandler
{
    NON_COPYABLE(EasfcInstance);

public:
    EasfcInstance(EasfcModule* owner, StressConnection* connection, Login* login);

    virtual ~EasfcInstance() 
	{
		delete mEasfcProxy;
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
	//BlazeRpcError   getCupInfo();

    void            sleep();

private:
    EasfcModule::Action mPreviousAction;

    bool        mEventBegun;
    bool        mEventEnded;

    bool        mEventSelected;
    int32_t     mSelectedEventIndex;

    
    EasfcModule* mOwner;
    Easfc::EasfcSlaveProxy* mEasfcProxy;
    const char8_t *mName;
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_EASFCMODULE_H


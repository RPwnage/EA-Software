/*************************************************************************************************/
/*!
    \file   useractivitytrackerslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_USERACTIVITYTRACKER_SLAVEIMPL_H
#define BLAZE_USERACTIVITYTRACKER_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "useractivitytracker/rpc/useractivitytrackerslave_stub.h"
#include "useractivitytracker/tdf/useractivitytrackertypes.h"
#include "useractivitytracker/tdf/useractivitytrackertypes_server.h"

#include "fifastats/tdf/fifastatstypes.h"

#include "framework/usersessions/usersessionmanager.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace UserActivityTracker
{

class UserActivityTrackerSlaveImpl : 
	public UserActivityTrackerSlaveStub
	, private UserSessionSubscriber
{
public:
    UserActivityTrackerSlaveImpl();
    ~UserActivityTrackerSlaveImpl();

	BlazeRpcError processUserActivity(ActivityModeTypeKeyScope mode, BlazeId blazeId, SubmitUserActivityRequest &request);
	BlazeRpcError initializeUserActivity(InitializeUserActivityTrackerRequest &request, InitializeUserActivityTrackerResponse &response);
	BlazeRpcError updateEadpStats(FifaStats::UpdateStatsRequest &request, FifaStats::UpdateStatsResponse &response);
	BlazeRpcError incrementMatchPlayed(ActivityModeTypeKeyScope mode, BlazeId blazeId);

private:
	virtual bool onConfigure();
	virtual bool onReconfigure();
	virtual bool onActivate();
	virtual void onShutdown();
	
	//! Called to perform tasks common to onConfigure and onReconfigure
	bool configureHelper();

	BlazeRpcError scheduleUpdateUserActivity(ActivityModeTypeKeyScope mode, BlazeId blazeId, SubmitUserActivityRequest &request);
	void updateUserActivity(ActivityModeTypeKeyScope modeType, BlazeId userId, SubmitUserActivityRequest request, bool incrementGamesPlayed);
	void setOptin(bool optIn);

	// event handlers for User Session Manager events
	virtual void onUserSessionExistence(const UserSession& userSession);

	/* \brief A user session is destroyed somewhere in the cluster. (global event) */
	virtual void onUserSessionExtinction(const UserSession& userSession);

	typedef eastl::hash_map<UserSessionId, uint8_t> OptInMap;
	
	OptInMap mOptinMap;
	Stats::StatPeriodType mRollOverPeriod;
	bool				  mRollOverHourly;
	eastl::string	mParentalControlsLink;
	eastl::string	mPlayerControlsLink;
	eastl::string	mPrivacySettingsLink;
	eastl::string	mOptinEadpStatsName;
	bool			mCalculateRolloverOnLogin;
	bool			mKillSwitchEnabled;
};

} // UserActivityTracker
} // Blaze

#endif // BLAZE_USERACTIVITYTRACKER_SLAVEIMPL_H


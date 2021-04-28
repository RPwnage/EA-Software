/*************************************************************************************************/
/*!
    \file   fifagroupsslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FifaGroupsSlaveImpl

    UserActivityTracker Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "useractivitytrackerslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

// useractivitytracker includes
#include "useractivitytracker/tdf/useractivitytrackertypes.h"
#include "useractivitytracker/tdf/useractivitytrackertypes_server.h"

#include "EATDF/time.h"

#include "component/stats/updatestatshelper.h"
#include "component/stats/updatestatsrequestbuilder.h"

#include "fifastats/rpc/fifastatsslave.h"
#include "util/rpc/utilslave.h"


namespace Blaze
{
namespace UserActivityTracker
{
// static
UserActivityTrackerSlave* UserActivityTrackerSlave::createImpl()
{
    return BLAZE_NEW_NAMED("UserActivityTrackerSlaveImpl") UserActivityTrackerSlaveImpl();
}

/*** Public Methods ******************************************************************************/
UserActivityTrackerSlaveImpl::UserActivityTrackerSlaveImpl() :
	mRollOverPeriod(Stats::STAT_PERIOD_WEEKLY)
	, mRollOverHourly(false)
{
}

UserActivityTrackerSlaveImpl::~UserActivityTrackerSlaveImpl()
{
}

BlazeRpcError UserActivityTrackerSlaveImpl::processUserActivity(ActivityModeTypeKeyScope mode, BlazeId blazeId, SubmitUserActivityRequest &request)
{
	Blaze::BlazeRpcError error = Blaze::ERR_OK;

	UserSessionId userSessionId = gCurrentUserSession->getSessionId();
	bool optedIn = (mOptinMap[userSessionId] == 1);

	if (optedIn)
	{
		error = scheduleUpdateUserActivity(mode, blazeId, request);
	}
	else
	{
		error = mKillSwitchEnabled ? USERACTIVITYTRACKER_ERR_KILL_SWITCHED : USERACTIVITYTRACKER_ERR_OPTED_OUT;
	}

	return error;
}

BlazeRpcError UserActivityTrackerSlaveImpl::initializeUserActivity(InitializeUserActivityTrackerRequest &request, InitializeUserActivityTrackerResponse &response)
{
	response.setRollOverPeriod(mRollOverPeriod);
	response.setParentalControlsLink(mParentalControlsLink.c_str());
	response.setPlayerControlsLink(mPlayerControlsLink.c_str());
	response.setPrivacySettingsLink(mPrivacySettingsLink.c_str());
	response.setKillSwitchEnabled(mKillSwitchEnabled);

	setOptin(request.getOptIn());

	TRACE_LOG("[UserActivityTrackerSlaveImpl:" << this << "].initializeUserActivity: user " << gCurrentUserSession->getBlazeId() << " opted " << (request.getOptIn() ? "IN" : "OUT") << " " );

	if (mCalculateRolloverOnLogin)
	{
		UserSessionId userSessionId = gCurrentUserSession->getSessionId();
		bool optedIn = (mOptinMap[userSessionId] == 1);

		if (optedIn)
		{
			const EA::TDF::TimeValue currentTime = TimeValue::getTimeOfDay();

			EA::TDF::TimeValue duration = 0;
			TRACE_LOG("[UserActivityTrackerSlaveImpl].initializeUserActivity - user " << gCurrentUserSession->getBlazeId() << " trigger rollover on login " << duration.getSec() << " secs of playtime");

			SubmitUserActivityRequest submitUserActivityRequest;
			submitUserActivityRequest.setTimeStamp(currentTime.getSec());

			UserActivity* userActivity = submitUserActivityRequest.getUserActivityList().allocate_element();
			userActivity->setActivityPeriodType(TYPE_CURRENT);
			userActivity->setTimeDuration(duration.getSec());
			userActivity->setNumMatches(0);

			submitUserActivityRequest.getUserActivityList().push_back(userActivity);

			scheduleUpdateUserActivity(ACTIVITY_MODE_TYPE_ONLINE, gCurrentUserSession->getBlazeId(), submitUserActivityRequest);
		}
		else
		{
			TRACE_LOG("[UserActivityTrackerSlaveImpl].initializeUserActivity - user " << gCurrentUserSession->getBlazeId() << " opted out no rollover on login");
		}

	}

	return ERR_OK;
}

BlazeRpcError UserActivityTrackerSlaveImpl::updateEadpStats(FifaStats::UpdateStatsRequest &request, FifaStats::UpdateStatsResponse &response)
{
	Blaze::BlazeRpcError error = Blaze::ERR_OK;
	int32_t httpStatusCode = HTTP_STATUS_OK;

	if (mKillSwitchEnabled)
	{
		error = USERACTIVITYTRACKER_ERR_KILL_SWITCHED;
	}
	else
	{
		Blaze::FifaStats::FifaStatsSlave *fifaStatsComponent = static_cast<Blaze::FifaStats::FifaStatsSlave*>(gController->getComponent(Blaze::FifaStats::FifaStatsSlave::COMPONENT_ID, false));

		if (fifaStatsComponent != NULL)
		{
			error = fifaStatsComponent->updateStats(request, response);
			httpStatusCode = response.getHttpStatusCode();
		}

		if (error == Blaze::ERR_OK && httpStatusCode == HTTP_STATUS_OK)
		{
			//check if the optin status has changed
			FifaStats::EntityList entityList = request.getEntities();
			FifaStats::EntityList::iterator iter, end;
			iter = entityList.begin();
			end = entityList.end();

			for (; iter != end; iter++)
			{
				FifaStats::Entity* entity = *iter;
				if (gCurrentUserSession->getBlazeId() == EA::StdC::AtoI64(entity->getEntityId()))
				{
					FifaStats::StatsUpdateList statsUpdateList = entity->getStatsUpdates();
					FifaStats::StatsUpdateList::iterator iterStats, endStats;
					iterStats = statsUpdateList.begin();
					endStats = statsUpdateList.end();
					for (; iterStats != endStats; iterStats++)
					{
						FifaStats::StatsUpdate* statsUpdate = *iterStats;
						if (blaze_strcmp(statsUpdate->getStatsName(), mOptinEadpStatsName.c_str()) == 0)
						{
							if (statsUpdate->getType() == FifaStats::TYPE_INT)
							{
								setOptin(statsUpdate->getStatsIValue() == 1);
								break;
							}
						}
					}
				}
			}
		}
	}

	return error;
}

BlazeRpcError UserActivityTrackerSlaveImpl::incrementMatchPlayed(ActivityModeTypeKeyScope mode, BlazeId blazeId)
{
	Blaze::BlazeRpcError error = Blaze::ERR_OK;

	UserSessionId userSessionId = gCurrentUserSession->getSessionId();
	bool optedIn = (mOptinMap[userSessionId] == 1);

	if (optedIn)
	{
		SubmitUserActivityRequest request;
		gSelector->scheduleFiberCall<UserActivityTrackerSlaveImpl, ActivityModeTypeKeyScope, BlazeId, SubmitUserActivityRequest, bool>(
			this,
			&UserActivityTrackerSlaveImpl::updateUserActivity,
			mode,
			blazeId,
			request,
			true,
			"UserActivityTrackerSlaveImpl::incrementMatchPlayed");
	}
	else
	{
		error = mKillSwitchEnabled ? USERACTIVITYTRACKER_ERR_KILL_SWITCHED : USERACTIVITYTRACKER_ERR_OPTED_OUT;
	}

	return error;
}

bool UserActivityTrackerSlaveImpl::onConfigure()
{
	bool success = configureHelper();
	TRACE_LOG("[UserActivityTrackerSlaveImpl:" << this << "].onConfigure: configuration " << (success ? "successful." : "failed."));
	return true;
}

bool UserActivityTrackerSlaveImpl::onReconfigure()
{
	bool success = configureHelper();
	TRACE_LOG("[UserActivityTrackerSlaveImpl:" << this << "].onReconfigure: configuration " << (success ? "successful." : "failed."));
	return true;
}

bool UserActivityTrackerSlaveImpl::onActivate()
{
	TRACE_LOG("[UserActivityTrackerSlaveImpl].onActivate");
	gUserSessionManager->addSubscriber(*this);

	return true;
}

void UserActivityTrackerSlaveImpl::onShutdown()
{
	TRACE_LOG("[UserActivityTrackerSlaveImpl].onShutdown");
	gUserSessionManager->removeSubscriber(*this);
}

bool UserActivityTrackerSlaveImpl::configureHelper()
{
	TRACE_LOG("[UserActivityTrackerSlaveImpl].configureHelper");
	const UserActivityTrackerConfig& config = getConfig();

	mRollOverPeriod = static_cast<Stats::StatPeriodType>(config.getRollOverPeriod());
	mRollOverHourly = config.getRollOverHourly();
	mParentalControlsLink = config.getParentalControlsLink();
	mPlayerControlsLink = config.getPlayerControlsLink();
	mPrivacySettingsLink = config.getPrivacySettingsLink();
	mOptinEadpStatsName = config.getOptinConigSettingName();
	mCalculateRolloverOnLogin = config.getCalculateRolloverOnLogin();
	mKillSwitchEnabled = config.getKillswitchEnabled();

	return true;
}

BlazeRpcError UserActivityTrackerSlaveImpl::scheduleUpdateUserActivity(ActivityModeTypeKeyScope mode, BlazeId blazeId, SubmitUserActivityRequest &request)
{
	gSelector->scheduleFiberCall<UserActivityTrackerSlaveImpl, ActivityModeTypeKeyScope, BlazeId, SubmitUserActivityRequest, bool>(
		this,
		&UserActivityTrackerSlaveImpl::updateUserActivity,
		mode,
		blazeId,
		request,
		false,
		"UserActivityTrackerSlaveImpl::processUserActivity");

	return ERR_OK;
}

void UserActivityTrackerSlaveImpl::updateUserActivity(ActivityModeTypeKeyScope modeType, BlazeId userId, SubmitUserActivityRequest request, bool incrementGamesPlayed)
{
	Blaze::BlazeRpcError error = Blaze::ERR_OK;

	//----------------------------------------------------------------------------------
	// Set up the SeasonPlay logic for processing the outcome
	//----------------------------------------------------------------------------------
	Blaze::Stats::UpdateStatsRequestBuilder builder;
	Blaze::Stats::UpdateStatsHelper updateStatsHelper;

	StatsUpdater userActivityStatsUpdater(&builder, &updateStatsHelper, &getConfig());

	if (incrementGamesPlayed)
	{
		GameManager::PlayerIdList playerIdList;
		playerIdList.push_back(userId);
		userActivityStatsUpdater.updateGamesPlayed(playerIdList);
		TRACE_LOG("[UserActivityTrackerSlaveImpl].updateUserActivity - update gamesplayed!");
	}
	else
	{
		userActivityStatsUpdater.addUserUpdateInfo(userId, modeType, request);
		TRACE_LOG("[UserActivityTrackerSlaveImpl].updateUserActivity - add offline update!");
	}

	//----------------------------------------------------------------------------------
	// Set up the initial selects for the stats we want to update
	//----------------------------------------------------------------------------------
	userActivityStatsUpdater.selectStats();

	bool strict = true;
	UserSession::pushSuperUserPrivilege();
	error = updateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)builder, strict);
	UserSession::popSuperUserPrivilege();
	if (Blaze::ERR_OK != error)
	{
		TRACE_LOG("[UserActivityTrackerSlaveImpl].updateUserActivity - initializeStatUpdate failed!" << error  << " (error)");
		return;
	}

	//----------------------------------------------------------------------------------
	// Perform the fetching of the select statements
	//----------------------------------------------------------------------------------
	UserSession::pushSuperUserPrivilege();
	error = updateStatsHelper.fetchStats();
	UserSession::popSuperUserPrivilege();
	if (Blaze::ERR_OK != error)
	{
		TRACE_LOG("[UserActivityTrackerSlaveImpl].updateUserActivity - fetchStats failed!" << error << " (error)");
		return;
	}

	//----------------------------------------------------------------------------------
	// Execute the logic to update the stats and construct update SQL instructions
	//----------------------------------------------------------------------------------
	userActivityStatsUpdater.transformStats();

	//----------------------------------------------------------------------------------
	// Perform all update SQL instructions
	//----------------------------------------------------------------------------------
	UserSession::pushSuperUserPrivilege();
	error = updateStatsHelper.commitStats();
	UserSession::popSuperUserPrivilege();
	if (Blaze::ERR_OK != error)
	{
		TRACE_LOG("[UserActivityTrackerSlaveImpl].updateUserActivity - commitStats failed!" << error << " (error)");
	}
}

void UserActivityTrackerSlaveImpl::setOptin(bool optIn)
{
	UserSessionId userSessionId = gCurrentUserSession->getSessionId();
	uint8_t optedIn = optIn ? 1 : 0;
	if (mKillSwitchEnabled)
	{
		optedIn = 0;
	}

	TRACE_LOG("[UserActivityTrackerSlaveImpl].setOptin - setting optin " << (optedIn == 1? "ON" : "OFF") << " ");

	mOptinMap[userSessionId] = optedIn;
	gUserSessionManager->updateExtendedData(userSessionId, COMPONENT_ID, StatsUpdater::EXTENDED_DATA_TRACKSTATS, optedIn);

	Blaze::Util::UtilSlave* utilSlave = static_cast<Blaze::Util::UtilSlave*>(gController->getComponent(Blaze::Util::UtilSlave::COMPONENT_ID, false));
	if (utilSlave != nullptr)
	{
		Blaze::Util::UserSettingsSaveRequest userSettingsSaveRequest;
		userSettingsSaveRequest.setBlazeId(gCurrentUserSession->getBlazeId());
		userSettingsSaveRequest.setKey("UAT_SETTING");
		userSettingsSaveRequest.setData(optedIn? "1": "0");
		Blaze::BlazeRpcError error = utilSlave->userSettingsSave(userSettingsSaveRequest);

		TRACE_LOG("[UserActivityTrackerSlaveImpl].setOptin - saved to user settings with error code " << error << " ");
	}
}


void UserActivityTrackerSlaveImpl::onUserSessionExistence(const UserSession& userSession)
{
	if (userSession.isLocal())
	{
		TRACE_LOG("[UserActivityTrackerSlaveImpl].onUserSessionExistence - user " << userSession.getBlazeId() << " ");
	}
}

void UserActivityTrackerSlaveImpl::onUserSessionExtinction(const UserSession& userSession)
{
	if(userSession.isLocal())
	{
		bool optedIn = (mOptinMap[userSession.getSessionId()] == 1);
		mOptinMap.erase(userSession.getSessionId());

		if (optedIn)
		{
			const EA::TDF::TimeValue creationTime = userSession.getCreationTime();
			const EA::TDF::TimeValue currentTime = TimeValue::getTimeOfDay();

			EA::TDF::TimeValue duration = currentTime - creationTime;
			TRACE_LOG("[UserActivityTrackerSlaveImpl].onUserSessionExtinction - user " << userSession.getBlazeId() << " logging out tracking " << duration.getSec() << " secs of playtime");

			SubmitUserActivityRequest submitUserActivityRequest;
			submitUserActivityRequest.setTimeStamp(currentTime.getSec());

			UserActivity* userActivity = submitUserActivityRequest.getUserActivityList().allocate_element();
			userActivity->setActivityPeriodType(TYPE_CURRENT);
			userActivity->setTimeDuration(duration.getSec());
			userActivity->setNumMatches(0);

			submitUserActivityRequest.getUserActivityList().push_back(userActivity);

			scheduleUpdateUserActivity(ACTIVITY_MODE_TYPE_ONLINE, userSession.getBlazeId(), submitUserActivityRequest);
		}
		else
		{
			TRACE_LOG("[UserActivityTrackerSlaveImpl].onUserSessionExtinction - user " << userSession.getBlazeId() << " opted out ");
		}
	}
}

} // UserActivityTracker
} // Blaze

// (c) Electronic Arts.  All Rights Reserved.

// main includes
#include "framework/blaze.h"
#include "heatshield/heatshieldslaveimpl.h"
#include "heatshield/modules/backendmodule.h"

// EA_TODO include
//#include <eamarkup/eamarkup.h>

#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace HeatShield
{

// static
HeatShieldSlave* HeatShieldSlave::createImpl()
{
    return BLAZE_NEW_NAMED("HeatShieldSlaveImpl") HeatShieldSlaveImpl();
}

/// Constructor
HeatShieldSlaveImpl::HeatShieldSlaveImpl()
	: m_backgroundJobQueue("HeatShieldSlaveImpl::mBackgroundJobQueue")
{
}

/// Destructor
HeatShieldSlaveImpl::~HeatShieldSlaveImpl()
{
}

/// Configures components that depend on the master
bool HeatShieldSlaveImpl::onResolve()
{
	// subscribe to user login logout events
	gUserSessionManager->addSubscriber(*this);
	return true;
}

/// Called when the slave is shutting down
void HeatShieldSlaveImpl::onShutdown()
{
	m_backgroundJobQueue.join();

	if (gUserSessionManager != NULL)
	{
		// Remove subscription to user session events
		gUserSessionManager->removeSubscriber(*this);
	}
}

bool HeatShieldSlaveImpl::onConfigure()
{
	m_backgroundJobQueue.setMaximumWorkerFibers(getConfig().getMaxFibers());

	return true;
}

bool HeatShieldSlaveImpl::onReconfigure()
{
	m_heartbeatRateLimitUserSet.clear();

	return onConfigure();
}

/// Handled when the user session leaves on the Blaze Cluster
/// @param userSession the user session data
void HeatShieldSlaveImpl::onUserSessionExtinction(const UserSession& userSession)
{
	// EA_TODO("cboyle", "01-30-2019", "02-28-2019", "Check the user type. Make sure we do not count non-Gameplay users.");
	// We cannot make a blocking call here so we will handle the disconnect on another fiber
	m_backgroundJobQueue.queueFiberJob(this, &HeatShieldSlaveImpl::clientLeaveHandler, 
		userSession.getUserId(), userSession.getAccountId(), userSession.getUserSessionId(), userSession.getClientPlatform(), userSession.getClientVersion(),
		"HeatShieldSlaveImpl::clientLeaveHandler");
}

/// Sends a message to the Shield Backend Service - is a Blocking Call
/// @param message the message to send
void HeatShieldSlaveImpl::clientLeaveHandler(const BlazeId blazeId, const AccountId accountId, const UserSessionId sessionId, Blaze::ClientPlatformType platform, const char8_t* clientVersion)
{
	BackendModule backendModule(this);
	BlazeRpcError error = backendModule.clientLeave(blazeId, accountId, sessionId, platform, clientVersion, FLOWTYPE_UNDEFINED, nullptr);
	if (error != ERR_OK)
	{
		ERR_LOG(THIS_FUNC << "Failed to send message to the Shield Backend with error [" << ErrorHelp::getErrorName(error) << "]");
	}
}

} // HeatShield
} // Blaze

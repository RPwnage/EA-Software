// (c) Electronic Arts.  All Rights Reserved.
#pragma once

#include "heatshield/rpc/heatshieldslave_stub.h"
#include "framework/usersessions/usersessionsubscriber.h"

namespace Blaze
{
namespace HeatShield
{

typedef eastl::set<UserSessionId> HeartbeatRateLimitUserSet;

class HeatShieldSlaveImpl : public HeatShieldSlaveStub, public UserSessionSubscriber
{
public:
	HeatShieldSlaveImpl();
	virtual ~HeatShieldSlaveImpl() override;
	
	HeartbeatRateLimitUserSet& getHeartbeatRateLimitUserSet() { return m_heartbeatRateLimitUserSet; }

private:
	// HeatShieldSlaveStub
	virtual bool onResolve() override;
	virtual void onShutdown() override;
	virtual bool onConfigure() override;
	virtual bool onReconfigure() override;

	// UserSessionSubscriber
	virtual void onUserSessionExtinction(const UserSession& userSession) override;

	void clientLeaveHandler(const BlazeId blazeId, const AccountId accountId, const UserSessionId sessionId, const ClientPlatformType platform, const char8_t* clientVersion);

	FiberJobQueue m_backgroundJobQueue; ///< The background job queue to kick off new fibers to process heartbeats
	HeartbeatRateLimitUserSet m_heartbeatRateLimitUserSet;
	const int32_t FLOWTYPE_UNDEFINED = 0;
};

} // HeatShield
} // Blaze
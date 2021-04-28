// (c) Electronic Arts.  All Rights Reserved.
#pragma once

#include <framework/blaze.h>

// shield protobuf includes
#include "eadp/shield/Trusted.grpc.pb.h"

// shield type includes
#include "heatshield/tdf/heatshieldtypes.h"

// Blaze includes
#include "framework/redis/rediskeymap.h"

namespace Blaze
{
namespace HeatShield
{

class HeatShieldSlaveImpl;
typedef Blaze::RedisKeyMap<UserSessionId, eastl::string> ShieldSessionPin;

class BackendModule
{
public:
	explicit BackendModule(HeatShieldSlaveImpl* component);
	virtual ~BackendModule();

	BlazeRpcError clientJoin(const BlazeId personaId, const AccountId accountId, const UserSessionId sessionId, const char8_t* pinSessionId, const ClientPlatformType platform, const char8_t* clientVersion, const int32_t flowType, EA::TDF::TdfBlob* response);
	BlazeRpcError clientLeave(const BlazeId personaId, const AccountId accountId, const UserSessionId sessionId, const ClientPlatformType platform, const char8_t* clientVersion, const int32_t flowType, EA::TDF::TdfBlob* response);
	BlazeRpcError clientRequest(const BlazeId personaId, const AccountId accountId, const UserSessionId sessionId, const ClientPlatformType platform, const char8_t* clientVersion, const uint8_t* data, size_t size, EA::TDF::TdfBlob* response);
	BlazeRpcError clientChallenge(const BlazeId blazeId, const AccountId accountId, const UserSessionId sessionId, const ClientPlatformType platform, const char8_t* clientVersion, const ClientChallengeType challengeType, const char8_t* gameType, EA::TDF::TdfBlob* response);


protected:
	BlazeRpcError sendToBackend(const BlazeId personaId, const UserSessionId sessionId, const ClientPlatformType platform, const Trusted::TrustedRequest& trustedRequest, EA::TDF::TdfBlob* response);
	bool isShieldEnabled() const;
	bool isAllowedGameType(const ClientChallengeType challengeType, const char8_t* gameTypeName);

private:
	// helpers
	void clientLeaveHandler(const BlazeId personaId, const UserSessionId sessionId, const ClientPlatformType platform);
	void fillTrustedRequestCommon(const BlazeId personaId, const AccountId accountId, const UserSessionId sessionId, const char8_t* pinSessionId, const char8_t* clientVersion, Trusted::TrustedRequest& request, Trusted::TrustedRequestMessage& message);
	bool checkRateLimit(UserSessionId sessionId);
	BlazeRpcError parseClientResponse(const BlazeId personaId, const uint8_t* data, size_t size, EA::TDF::TdfBlob* response, bool hasClientJoinOrLeaveGame);
	BlazeRpcError processRateLimitExceededPinEvent(const UserSessionId sessionId);
	BlazeRpcError getPinSessionId(const UserSessionId sessionId, eastl::string& pinSessionId);
	void removePinSessionId(const UserSessionId sessionId);

	HeatShieldSlaveImpl* m_component;
	ShieldSessionPin m_shieldSessionPin;
};

}
}
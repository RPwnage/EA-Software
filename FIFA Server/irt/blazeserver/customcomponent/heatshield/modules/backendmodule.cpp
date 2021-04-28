// (c) Electronic Arts.  All Rights Reserved.

#include "heatshield/modules/backendmodule.h"


// blaze includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/controller/controller.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/redis/rediskeymap.h"
#include "framework/tdf/userpineventtypes_server.h"

// heat shield includes
#include "heatshield/tdf/heatshieldtypes.h"
#include "heatshield/tdf/riverpostercustom_error.h"
#include "heatshield/tdf/heatshieldtypes_server.h"

// shield http slave includes
#include "shield/rpc/shieldslave.h"
#include "shield/tdf/shield.h"


namespace Blaze
{
namespace HeatShield
{

/// Constructor
BackendModule::BackendModule(HeatShieldSlaveImpl* component)
	: m_component(component),
	  m_shieldSessionPin(Blaze::RedisId("HeatShield", "shieldSessionPin"))
{
}

/// Destructor
BackendModule::~BackendModule()
{
}

/// Determines if interactions with the Shield Backend server is allowed
bool BackendModule::isShieldEnabled() const
{
	return m_component->getConfig().getEnableShieldBackend();
}

// Determines if the gameType should have shield enabled
// @param challengeType ClientChallengeType reason for client challenge
// @param gameType char8_t* game mode name as string
// @returns bool
bool BackendModule::isAllowedGameType(const ClientChallengeType challengeType, const char8_t* gameTypeName)
{
	const HeatShieldClientChallengeConfig& clientChallengeConfig = m_component->getConfig().getClientChallengeConfig();
	const ClientChallengeGameTypesList* allowedGameTypes;

	if (gameTypeName == nullptr)
	{
		ERR_LOG(THIS_FUNC << "gameTypeName cannot be null");
		return false;
	}

	switch (challengeType)
	{
	case CLIENT_CHALLENGE_MATCHEND:
		allowedGameTypes = &(clientChallengeConfig.getAllowedMatchEndGameTypes());
		break;
	case CLIENT_CHALLENGE_MATCHMAKING:
		allowedGameTypes = &(clientChallengeConfig.getAllowedMatchMakingTypes());
		break;
	default:
		ERR_LOG(THIS_FUNC << "Unknown game type specified");
		return false; // Intentional early return
	}

	if (allowedGameTypes->empty())
	{
		TRACE_LOG(THIS_FUNC << "No game types for ClientChallenge type [" << challengeType << "]");
		return false;
	}

	ClientChallengeGameTypesList::const_iterator it = allowedGameTypes->begin();
	for (; it != allowedGameTypes->end(); it++)
	{
		if (blaze_strcmp(it->c_str(), gameTypeName) == 0)
		{
			return true;
		}
	}

	return false;
}

/// Enqueues a Shield Backend message to inform that a user session has been created.
/// @param blazeId BlazeId of current user session
/// @param accountId AccountId of current user session
/// @param sessionId UserSessionId of current user session
/// @param platform ClientPlatformType of current user session
/// @param response TdfBlob opaque response from shield-server backend
/// @returns BlazeRpcError
BlazeRpcError BackendModule::clientJoin(const BlazeId blazeId, const AccountId accountId, const UserSessionId sessionId, const char8_t* pinSessionId, const ClientPlatformType platform, const char8_t* clientVersion, const int32_t flowType, EA::TDF::TdfBlob* response)
{
	if (!isShieldEnabled())
	{
		TRACE_LOG(THIS_FUNC << "Not allowing new messages to the Shield Backend because Shield is disabled.");
		return HEAT_SHIELD_DISABLED;
	}

	// Save the game-mode specific sessionId to the cache on clientJoin
	eastl::string pinSessionIdStr;
	pinSessionIdStr.append(pinSessionId);
	EA::TDF::TimeValue expiry = m_component->getConfig().getPinSessionCacheExpiry();
	m_shieldSessionPin.upsertWithTtl(sessionId, pinSessionId, expiry);

	Trusted::TrustedRequest trustedRequest;
	Trusted::TrustedRequestMessage* trustedRequestMsg = trustedRequest.add_messages();
	fillTrustedRequestCommon(blazeId, accountId, sessionId, pinSessionId, clientVersion, trustedRequest, *trustedRequestMsg);

	Trusted::ClientJoinGame* clientJoinGame = trustedRequestMsg->mutable_clientjoingame();
	clientJoinGame->set_flowtype(flowType);
	

	BlazeRpcError error = sendToBackend(blazeId, sessionId, platform, trustedRequest, response);
	if (error != ERR_OK)
	{
		ERR_LOG(THIS_FUNC << "Failed to send message to the Shield Backend with error [" << ErrorHelp::getErrorName(error) << "]");
	}
	return error;
}

/// Enqueues a Shield Backend message to inform that a user session has been destroyed.
/// @param blazeId BlazeId of current user session
/// @param accountId AccountId of current user session
/// @param sessionId UserSessionId of current user session
/// @param platform ClientPlatformType of current user session
/// @param response TdfBlob opaque response from shield-server backend
/// @returns BlazeRpcError
BlazeRpcError BackendModule::clientLeave(const BlazeId blazeId, const AccountId accountId, const UserSessionId sessionId, const ClientPlatformType platform, const char8_t* clientVersion, const int32_t flowType, EA::TDF::TdfBlob* response)
{
	if (!isShieldEnabled())
	{
		TRACE_LOG(THIS_FUNC << "Not allowing new messages to the Shield Backend because Shield is disabled.");
		return HEAT_SHIELD_DISABLED;
	}

	// Online mode check
	bool allowClientInOnlineFUTMode = m_component->getConfig().getEnableShieldOnlineFUTMode();
	bool allowClientInOnlineVoltaMode = m_component->getConfig().getEnableShieldOnlineVoltaMode();
	int32_t enterFUTFlowType = m_component->getConfig().getEnterFUTOnlineFlowType();
	int32_t enterVoltaFlowType = m_component->getConfig().getEnterVoltaOnlineFlowType();

	// Clear the rate limit exceeded flag for this user if it exists
	HeartbeatRateLimitUserSet &rateLimitUserSet = m_component->getHeartbeatRateLimitUserSet();
	rateLimitUserSet.erase(sessionId);

	// Pull game session ID from cache to inject into the TrustedRequest
	eastl::string pinSessionId = "";
	BlazeRpcError error = getPinSessionId(sessionId, pinSessionId);

	if (error == HEAT_SHIELD_SESSION_NOT_FOUND)
	{
		TRACE_LOG(THIS_FUNC << "No active session in cache, not emitting a ClientLeave event");
		return ERR_OK; // Intentional Early return
	}

	Trusted::TrustedRequest trustedRequest;
	Trusted::TrustedRequestMessage* trustedRequestMsg = trustedRequest.add_messages();
	fillTrustedRequestCommon(blazeId, accountId, sessionId, pinSessionId.c_str(), clientVersion, trustedRequest, *trustedRequestMsg);

	Trusted::ClientLeaveGame* clientLeaveGame = trustedRequestMsg->mutable_clientleavegame();
	clientLeaveGame->set_flowtype(flowType);

	error = sendToBackend(blazeId, sessionId, platform, trustedRequest, response);

	// If Online mode is disabled, then ignore shield back end response
	if ((!allowClientInOnlineFUTMode && enterFUTFlowType == flowType) || (!allowClientInOnlineVoltaMode && enterVoltaFlowType == flowType))
	{
		removePinSessionId(sessionId);
		return HEAT_SHIELD_ONLINE_MODE_DISABLED;
	}

	if (error != ERR_OK)
	{
		ERR_LOG(THIS_FUNC << "Failed to send message to the Shield Backend with error [" << ErrorHelp::getErrorName(error) << "]");
	}

	bool enabled = false;
	if (response != nullptr)
	{
		uint8_t* data = response->getData();
		const Trusted::ClientShieldOnlineModeStatus* clientShieldOnlineModeStatus = reinterpret_cast<const ::Trusted::ClientShieldOnlineModeStatus*>(data);
		enabled = clientShieldOnlineModeStatus == nullptr ? false : *clientShieldOnlineModeStatus == Trusted::ClientShieldOnlineModeStatus::ENABLED;
	}

	// Remove session id for only if shield is disabled for online game mode or user is exiting the game mode
	if (!enabled)
	{
		removePinSessionId(sessionId);
	}
	return error;
}

/// Enqueues a Shield Backend message to check for any heartbeat timeouts.
/// @param blazeId BlazeId of current user session
/// @param accountId AccountId of current user session
/// @param sessionId UserSessionId of current user session
/// @param platform ClientPlatformType of current user session
/// @param challengeType ClientChallengeType this challenge event is being sent for (matchmaking / gamereport)
/// @param gameType char8_t* the name of the game mode the challenge event is being sent for
/// @param response TdfBlob opaque response from shield-server backend
/// @returns BlazeRpcError
BlazeRpcError BackendModule::clientChallenge(const BlazeId blazeId, const AccountId accountId, const UserSessionId sessionId, const ClientPlatformType platform, const char8_t* clientVersion, const ClientChallengeType challengeType, const char8_t* gameType, EA::TDF::TdfBlob* response)
{
	if (!isShieldEnabled())
	{
		TRACE_LOG(THIS_FUNC << "Not allowing new messages to the Shield Backend because Shield is disabled.");
		return HEAT_SHIELD_DISABLED;
	}

	// Don't do a ClientChallenge if the game mode is not configured to run shield
	if (!isAllowedGameType(challengeType, gameType))
	{
		TRACE_LOG(THIS_FUNC << "Not emitting a client challenge for gameType [" << gameType << "]");
		return ERR_OK; // Intentional early return
	}

	// Pull game session ID from cache to inject into the TrustedRequest
	eastl::string pinSessionId = "";
	BlazeRpcError error = getPinSessionId(sessionId, pinSessionId);

	if (error == HEAT_SHIELD_SESSION_NOT_FOUND)
	{
		// Indicates that an adversarial client may have dropped the ClientJoin, we will
		// be able to detect this on the shield-backend via a PIN event.
		TRACE_LOG(THIS_FUNC << "ClientChallenge emitted without an active session");
	}

	Trusted::TrustedRequest trustedRequest;
	Trusted::TrustedRequestMessage* trustedRequestMsg = trustedRequest.add_messages();
	fillTrustedRequestCommon(blazeId, accountId, sessionId, pinSessionId.c_str(), clientVersion, trustedRequest, *trustedRequestMsg);

	// The ClientChallenge message is empty and there is nothing to fill in. It just needs to be non-null.
	trustedRequestMsg->mutable_clientchallenge();

	error = sendToBackend(blazeId, sessionId, platform, trustedRequest, response);
	if (error != ERR_OK)
	{
		ERR_LOG(THIS_FUNC << "Failed to send message to the Shield Backend with error [" << ErrorHelp::getErrorName(error) << "]");
	}

	return error;
}

/// Processes and fires a PIN event if a user exceeds the configured heartbeat rate limit
/// The pin event will fire at most once per user session
/// @param sessionId the user's session id
BlazeRpcError BackendModule::processRateLimitExceededPinEvent(const UserSessionId sessionId)
{
	HeartbeatRateLimitUserSet &rateLimitUserSet = m_component->getHeartbeatRateLimitUserSet();
	// If the user has already exceeded the rate limit this session, don't fire another event
	if (rateLimitUserSet.find(sessionId) != rateLimitUserSet.end())
	{
		return HEAT_SHIELD_RATE_LIMIT_PIN_FIRED;
	}

	// Set up the PIN event
	const HeatShieldPinConfig& pinConfig = m_component->getConfig().getRateLimitPinConfig();
	RiverPoster::CustomError customError;
	customError.getHeaderCore().setEventName(pinConfig.getPinEventName());
	customError.getHeaderCore().setEventType(pinConfig.getPinEventType());

	char8_t timeString[64];
	TimeValue::getTimeOfDay().toAccountString(timeString, sizeof(timeString), true);
	customError.getHeaderCore().setEventTimestamp(timeString);

	customError.setType(pinConfig.getPinType());

	// Fire the event
	PINSubmissionPtr pinSubmission = BLAZE_NEW PINSubmission;
	RiverPoster::PINEventList* pinEventList = pinSubmission->getEventsMap().allocate_element();
	EA::TDF::tdf_ptr<EA::TDF::VariableTdfBase> pinEvent = pinEventList->pull_back();
	pinEvent->set(customError.clone());
	pinSubmission->getEventsMap()[sessionId] = pinEventList;
	gUserSessionManager->sendPINEvents(pinSubmission);

	rateLimitUserSet.insert(sessionId);

	return ERR_OK;
}

/// Enqueues a Shield Backend message to inform that a user's client has sent an encrypted Shield message.
/// @param blazeId BlazeId of current user session
/// @param accountId AccountId of current user session
/// @param sessionId UserSessionId of current user session
/// @param platform ClientPlatformType of current user session
/// @param response TdfBlob opaque response from shield-server backend
/// @returns BlazeRpcError
BlazeRpcError BackendModule::clientRequest(const BlazeId blazeId, const AccountId accountId, const UserSessionId sessionId, const ClientPlatformType platform, const char8_t* clientVersion, const uint8_t* data, size_t size, EA::TDF::TdfBlob* response)
{
	if (!isShieldEnabled())
	{
		TRACE_LOG(THIS_FUNC << "Not allowing new messages to the Shield Backend because Shield is disabled.");
		return HEAT_SHIELD_DISABLED;
	}

	if (checkRateLimit(sessionId))
	{
		WARN_LOG(THIS_FUNC << "Exceeded the Shield rate limit. Ignoring client message.");
		BlazeRpcError error = processRateLimitExceededPinEvent(sessionId);
		if (error != ERR_OK)
		{
			ERR_LOG(THIS_FUNC << "Failed firing rate limit PIN event [" << ErrorHelp::getErrorName(error) << "]");
		}
		return HEAT_SHIELD_ERR_RATE_LIMIT;
	}

	// Pull game session ID from cache to inject into the TrustedRequest
	eastl::string pinSessionId = "";
	BlazeRpcError error = getPinSessionId(sessionId, pinSessionId);
	if (error == HEAT_SHIELD_SESSION_NOT_FOUND)
	{
		// The client shouldn't emit a heartbeat without an active session
		WARN_LOG(THIS_FUNC << "Client emitted heartbeat without active shield session.");
	}

	Trusted::TrustedRequest trustedRequest;
	Trusted::TrustedRequestMessage* trustedRequestMsg = trustedRequest.add_messages();
	fillTrustedRequestCommon(blazeId, accountId, sessionId, pinSessionId.c_str(), clientVersion, trustedRequest, *trustedRequestMsg);

	// The client request is an opaque blob to the current Blaze instance and it is likely
	// encrypted and only the Shield backend can decrypt it.
	trustedRequestMsg->set_clientrequest(data, size);

	error = sendToBackend(blazeId, sessionId, platform, trustedRequest, response);
	if (error != ERR_OK)
	{
		ERR_LOG(THIS_FUNC << "Failed to send message to the Shield Backend with error [" << ErrorHelp::getErrorName(error) << "]");
	}
	return error;
}

BlazeRpcError BackendModule::getPinSessionId(const UserSessionId sessionId, eastl::string& pinSessionId)
{
	Blaze::RedisError redisError = m_shieldSessionPin.find(sessionId, pinSessionId);

	if (redisError == REDIS_ERR_OK)
	{
		// Update the expiry when the key is accessed
		EA::TDF::TimeValue expiry = m_component->getConfig().getPinSessionCacheExpiry();
		m_shieldSessionPin.expire(sessionId, expiry);
		return ERR_OK;
	}
	else if (redisError == REDIS_ERR_NOT_FOUND)
	{
		return HEAT_SHIELD_SESSION_NOT_FOUND;
	}
	else
	{
		ERR_LOG(THIS_FUNC << "Got redis error [" << redisError <<
			"] trying to find game session id for " << sessionId);
		return HEAT_SHIELD_ERR_UNKNOWN;
	}
}

void BackendModule::removePinSessionId(const UserSessionId sessionId)
{
	Blaze::RedisError redisError = m_shieldSessionPin.erase(sessionId);
	if (redisError != REDIS_ERR_NOT_FOUND && redisError != REDIS_ERR_OK)
	{
		ERR_LOG(THIS_FUNC << "Got redis error [" << redisError <<
			"] trying to erase game session id for " << sessionId);
	}
}

/// Fills Common Data for the Trusted Request
/// @param blazeId the user's persona id
/// @param accountId the user's nucleus id
/// @param sessionId the user's session id
/// @param request the Trusted Request
/// @param message a message in the Trusted Request
void BackendModule::fillTrustedRequestCommon(const BlazeId blazeId, const AccountId accountId, const UserSessionId sessionId, const char8_t* pinSessionId, const char8_t* clientVersion, Trusted::TrustedRequest& request, Trusted::TrustedRequestMessage& message)
{
	Shared::Client* client = message.mutable_client();
	client->set_personaid(blazeId);
	client->set_nucleusid(accountId);
	client->set_pinsessionid(pinSessionId);
	client->set_clientversion(clientVersion);
	request.set_gameid(sessionId);
}

/// Checks the rate limiter in order to determine if we have exceeded our call rate limit to the Shield Backend
/// FIFA Divergence - Per session rate limiting
/// @param sessionId the user's session id
bool BackendModule::checkRateLimit(const UserSessionId sessionId)
{
	bool hasExceeded = false;
	const HeatShieldCallRate& maxRate = m_component->getConfig().getCallRate();
	const int64_t intervalSize = maxRate.getIntervalSize().getSec();
	const int64_t epochSeconds = TimeValue::getTimeOfDay().getSec();

	if (intervalSize == 0)
	{
		TRACE_LOG(THIS_FUNC << "Interval size is 0. Assuming infinite calls allowed.");
	}
	else
	{
		const int64_t currentInterval = epochSeconds / intervalSize;

		typedef eastl::pair<UserSessionId, int64_t> SessionInterval;
		SessionInterval sInterval = eastl::make_pair(sessionId, currentInterval);

		typedef Blaze::RedisKeyMap<SessionInterval, int64_t> ShieldCallRateMap;
		ShieldCallRateMap shieldBackendCallRateMap(Blaze::RedisId("HeatShield", "shieldBackendCallRateMap"));

		int64_t callsThisInterval = 0;
		Blaze::RedisError redisError = shieldBackendCallRateMap.incr(sInterval, &callsThisInterval);

		if (redisError != REDIS_ERR_OK)
		{
			ERR_LOG(THIS_FUNC << "Got redis error [" << redisError <<
				"] trying to increment interval [" << currentInterval << "] for Shield rate limits.");
		}
		else
		{
			if (callsThisInterval == 1)
			{
				// if we just started a new interval, set it to expire in 2 intervals time (just to keep redis clean)
				shieldBackendCallRateMap.expire(sInterval, maxRate.getIntervalSize() * 2);
			}

			if (callsThisInterval > maxRate.getIntervalCount())
			{
				hasExceeded = true;
			}
		}
	}

	return hasExceeded;
}

/// Sends a message to the Shield Backend Service
/// @param blazeId BlazeId of current user session
/// @param sessionId UserSessionId of current user session
/// @param platform ClientPlatformType of current user session
/// @param response TdfBlob opaque response from shield-server backend
/// @returns BlazeRpcError
BlazeRpcError BackendModule::sendToBackend(const BlazeId blazeId, const UserSessionId sessionId, const ClientPlatformType platform,
	const Trusted::TrustedRequest& trustedRequest, EA::TDF::TdfBlob* response)
{
	Blaze::Shield::ShieldSlave* shieldSlave = static_cast<Blaze::Shield::ShieldSlave*>(
		const_cast<Blaze::Component*>(Blaze::gOutboundHttpService->getService(Blaze::Shield::ShieldSlave::COMPONENT_INFO.name)));

	if (shieldSlave == nullptr)
	{
		ERR_LOG(THIS_FUNC << "Unable to find the Shield Slave Service!");
		return Blaze::HEAT_SHIELD_ERR_UNKNOWN;
	}

	// Not logging game id or client id because they must be the same as persona id and session id
	bool hasClientJoinGame = false;
	bool hasClientLeaveGame = false;
	size_t clientRequestCount = 0;
	size_t clientRequestSize = 0;
	for (const auto& message : trustedRequest.messages())
	{
		hasClientJoinGame |= message.has_clientjoingame();
		hasClientLeaveGame |= message.has_clientleavegame();
		clientRequestSize += message.clientrequest().size();
		if (message.clientrequest().size() > 0)
		{
			clientRequestCount++;
		}
	}
	INFO_LOG(THIS_FUNC << "Sending Trusted Request to Shield Backend for BlazeId [" << blazeId << "], " <<
		"UserSessionId [" << sessionId << "], " <<
		"hasClientJoinGame [" << hasClientJoinGame << "], " <<
		"hasClientLeaveGame [" << hasClientLeaveGame << "], " <<
		"clientRequestSize [" << clientRequestSize << "], " <<
		"clientRequestCount [" << clientRequestCount << "]");

	Blaze::Shield::ShieldBackendRequest shieldBackendReq;
	Blaze::Shield::ShieldBackendResponse shieldBackendResp;
	shieldBackendReq.setGame(gController->getDefaultServiceName());
	shieldBackendReq.setPlatform(ClientPlatformTypeToString(platform));
	const std::string data = trustedRequest.SerializeAsString();
	shieldBackendReq.getData().setData(reinterpret_cast<const uint8_t*>(data.data()), data.size());

	BlazeRpcError error = shieldSlave->sendToBackend(shieldBackendReq, shieldBackendResp);
	if (error == ERR_OK)
	{
		error = parseClientResponse(blazeId, shieldBackendResp.getData().getData(), shieldBackendResp.getData().getSize(), response, hasClientLeaveGame || hasClientJoinGame);
	}
	else
	{
		ERR_LOG(THIS_FUNC << "Failed to send message to the Shield Backend with HTTP error [" << ErrorHelp::getHttpStatusCode(error) << "]");
	}

	return error;
}

/// Parses the client response into a TrustedResponse object, finds the client response, then 
/// packages the client response (which is an opaque blob) into a TdfBlob to send to the client.
/// @param blazeId the user's blaze id
/// @param data the data blob from the Shield Backend
/// @param size the size of the data blob in bytes
/// @param response outparam TdfBlob response from shield-server backend
/// @param hasClientJoinOrLeaveGame whether it is a join or leave event from client or not
BlazeRpcError BackendModule::parseClientResponse(const BlazeId blazeId, const uint8_t* data, size_t size, EA::TDF::TdfBlob* response, bool hasClientJoinOrLeaveGame)
{
	if (response == nullptr)
	{
		TRACE_LOG(THIS_FUNC << "Response not found");
		return Blaze::ERR_OK; // Intentional Early return
	}

	if (data == nullptr || size == 0)
	{
		TRACE_LOG(THIS_FUNC << "Received an empty payload from the Shield Backend.");
		return Blaze::ERR_OK; // Intentional Early return
	}

	Trusted::TrustedResponse trustedResp;
	if (!trustedResp.ParseFromArray(const_cast<uint8_t*>(data), size))
	{
		ERR_LOG(THIS_FUNC << "Unable to parse a Trusted Response from the Shield Backend.");
		return Blaze::HEAT_SHIELD_ERR_UNKNOWN; // Intentional Early return
	}

	if (trustedResp.messages_size() > 0)
	{
		//	EA_TODO("cboyle", "01-29-2019", "02-28-2019", "Add support for multiple messages");
		if (trustedResp.messages_size() != 1)
		{
			INFO_LOG(THIS_FUNC << "Received >> [" <<
				trustedResp.messages_size() << "] TrustedResponse message from the Shield Backend, only handling the first");
		}

		// We will only be processing client shield online response if the message exist and the request type is either client join or leave
		if (hasClientJoinOrLeaveGame && trustedResp.messages(0).has_clientshieldonlineresponse())
		{
			// client leave or join response
			const Trusted::ClientShieldOnlineResponse clientShieldOnlineModeResponse = trustedResp.messages(0).clientshieldonlineresponse();
			const Trusted::ClientShieldOnlineModeStatus clientShieldOnlineModeStatus = clientShieldOnlineModeResponse.status();

			INFO_LOG(THIS_FUNC << "Received client shield online mode status from back end. Status for online game mode [" << clientShieldOnlineModeStatus 
				<< "] with size [" << clientShieldOnlineModeResponse.ByteSize() << "]");

			// max payload size = 0 means no maximum payload size. see `heatshieldtypes_server.tdf
			if (m_component->getConfig().getMaximumPayloadSize() != 0 && (uint32_t)clientShieldOnlineModeResponse.ByteSize() > m_component->getConfig().getMaximumPayloadSize())
			{
				ERR_LOG(THIS_FUNC << "Received an client shield online mode response from the Shield Backend with size ["
					<< clientShieldOnlineModeResponse.ByteSize() << "] max allowed [" << m_component->getConfig().getMaximumPayloadSize() << "]");
				return Blaze::HEAT_SHIELD_ERR_UNKNOWN;
			}
			else
			{
				(*response).setData(reinterpret_cast<const uint8_t*>(&clientShieldOnlineModeStatus), sizeof(Trusted::ClientShieldOnlineModeStatus));
			}
		}
		else
		{
			const std::string& clientResponseData = trustedResp.messages(0).clientresponse();

			INFO_LOG(THIS_FUNC << "Received Trusted Response from Shield Backend for BlazeId [" << blazeId << "], " <<
				"ClientResponseSize [" << clientResponseData.size() << "], " <<
				"GameId [" << trustedResp.gameid() << "]");

			// max payload size = 0 means no maximum payload size. see `heatshieldtypes_server.tdf
			if (m_component->getConfig().getMaximumPayloadSize() != 0 && clientResponseData.size() > m_component->getConfig().getMaximumPayloadSize())
			{
				ERR_LOG(THIS_FUNC << "Received an client response from the Shield Backend with size ["
					<< clientResponseData.size() << "] max allowed [" << m_component->getConfig().getMaximumPayloadSize() << "]");
				return Blaze::HEAT_SHIELD_ERR_UNKNOWN;
			}
			else
			{
				(*response).setData(reinterpret_cast<const uint8_t*>(clientResponseData.data()), clientResponseData.size());
			}
		}
	}
	return Blaze::ERR_OK;
}

}
}
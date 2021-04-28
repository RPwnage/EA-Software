//  *************************************************************************************************
//
//   File:    userpersonalizedservices.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "userpersonalizedservices.h"

namespace Blaze {
namespace Stress {
//  *************************************************************************************************
//  ups Stress code 
//  *************************************************************************************************
UPSProxy::UPSProxy(StressPlayerInfo* playerInfo) :mPlayerData(playerInfo)
{
	mHttpUPSProxyConnMgr = NULL;
	initUPSProxyHTTPConnection();
}

UPSProxy::~UPSProxy()
{
	if (mHttpUPSProxyConnMgr != NULL)
	{
		delete mHttpUPSProxyConnMgr;
	}
}

bool UPSProxy::initUPSProxyHTTPConnection()
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::initUPSProxyHTTPConnection]: Initializing the HttpConnectionManager");
	mHttpUPSProxyConnMgr = BLAZE_NEW OutboundHttpConnectionManager("fifa-2019-ps4-lt");
	if (mHttpUPSProxyConnMgr == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[UPSProxy::initUPSProxyHTTPConnection]: Failed to Initialize the HttpConnectionManager");
		return false;
	}
	const char8_t* serviceHostname = NULL;
	bool serviceSecure = false;

	HttpProtocolUtil::getHostnameFromConfig(StressConfigInst.getUPSServerUri(), serviceHostname, serviceSecure);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::initUPSProxyHTTPConnection]: UPSServerUri= " << StressConfigInst.getUPSServerUri());

	InetAddress* inetAddress;
	uint16_t portNumber = 443;
	bool8_t	isSecure = true;
	inetAddress = BLAZE_NEW InetAddress(serviceHostname, portNumber);
	if (inetAddress != NULL)
	{
		mHttpUPSProxyConnMgr->initialize(*inetAddress, 1, isSecure, false);
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::initUPSProxyHTTPConnection]: Failed to Create InetAddress with given hostname");
		return false;
	}
	return true;
}

bool UPSProxy::getMessage(const char8_t * authorizationAccessToken)
{
	bool result = false;

	UPSHttpResult httpResult;
	eastl::string jsonData;
	
	//{"dimensionSelector":{"gameNames":["FIFA 18"],"platforms":["ps4"],"modeTypes":["ut","story","career","exhibition","createtournament"],"releaseTypes":["prod"]}}
	jsonData.append("{\"dimensionSelector\":{\"gameNames\":[\"FIFA 21\"],\"platforms\":[\"");
	if (StressConfigInst.getPlatform() == PLATFORM_XONE)
	{
		jsonData.append("xbox_one");
	}
	else if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		jsonData.append("ps4");
	}
	// PS4 - projectId - 312054
	//jsonData.append("\",\"tid\":\"");
	jsonData.append("\"],\"modeTypes\":[\"ut\",\"story\",\"career\",\"exhibition\",\"createtournament\", \"unknown\"],\"releaseTypes\":[\"prod\",\"lt\",\"test\"]}}");
	//jsonData.append("\"]}");
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage] " << " [" << mPlayerData->getPlayerId() << "] JSONRPC request[" << jsonData.c_str() << "]");

	OutboundHttpConnection::ContentPartList contentList;
	OutboundHttpConnection::ContentPart content;
	content.mName = "content";
	content.mContentType = JSON_CONTENTTYPE;
	content.mContent = reinterpret_cast<const char8_t*>(jsonData.c_str());
	content.mContentLength = jsonData.size();
	contentList.push_back(&content);

	eastl::string strHttpHeaderAuth;
	StressHttpResult stressAuthCode;
	//eastl::string bearer = "AuthorizationHeader [Bearer ";
	eastl::string bearer = "Authorization: Bearer ";
	//bearer += mPlayerData->getLogin()->getOwner()->getAuthCode();
	bearer += authorizationAccessToken;
	strHttpHeaderAuth.sprintf(bearer.c_str());
	//strHttpHeader.sprintf(mHeaderType);

	const char8_t *httpHeaders[1];
	httpHeaders[0] = strHttpHeaderAuth.c_str();
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage] bearer.c_str()" << " [" << mPlayerData->getPlayerId() << "] bearer [" << bearer.c_str() << "]");
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage] httpHeaders[0]" << " [" << mPlayerData->getPlayerId() << "] " << httpHeaders[0]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage] getUPSTrackingUri()" << " [" << mPlayerData->getPlayerId() << "] getMessage URI[" << StressConfigInst.getUPSTrackingUri() << "]");

	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage] getUPSGetMessageUriPlayerId() " << " [" << mPlayerData->getPlayerId() << "] getMessage URI[ getUPSGetMessageUriPlayerId() " << getUPSGetMessageUriPlayerId() << "]");

	result = sendUPSHttpRequest(HttpProtocolUtil::HTTP_POST, getUPSGetMessageUriPlayerId(), httpHeaders, sizeof(httpHeaders) / sizeof(char8_t*), httpResult, contentList);

	if (false == result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage:result]:" << mPlayerData->getPlayerId() << " UPS returned error for getMessage.");
		return false;
	}

	if (httpResult.hasError())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage:hasError]:" << mPlayerData->getPlayerId() << " UPS returned error for getMessage");
		return false;
	}

	return true;
}

bool UPSProxy::sendUPSHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult& result, OutboundHttpConnection::ContentPartList& contentList)
{
	BlazeRpcError err = ERR_SYSTEM;
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::sendUPSHttpRequest][" << mPlayerData->getPlayerId() << "] method= " << method << " URI= " << URI << " httpHeaders= " << httpHeaders);

	err = mHttpUPSProxyConnMgr->sendRequest(method, URI, NULL, 0, httpHeaders, headerCount, &result, &contentList);

	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::sendUPSHttpRequest][" << mPlayerData->getPlayerId() << "] failed to send http request");
		return false;
	}
	return true;
}

const char8_t * UPSProxy::getUPSGetMessageUriPlayerId() {
	char8_t	playerIdUPS[256];
	blaze_snzprintf(playerIdUPS, sizeof(playerIdUPS), "%" PRId64, mPlayerData->getPlayerId());
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::getMessage] " << " [" << playerIdUPS << "] getMessage URI[" << "]");
	mUPSGetMessageUriPlayerId = StressConfigInst.getUPSGetMessageUri();
	mUPSGetMessageUriPlayerId += playerIdUPS;
	mUPSGetMessageUriPlayerId += "/";
	mUPSGetMessageUriPlayerId += "game-platform-mode";
	return mUPSGetMessageUriPlayerId.c_str();
}

bool  UPSProxyHandler::simulateUPSLoad()
{
	bool result = true;

	//get Access Token
	result = getMyAccessToken();
	if (result)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSProxy::simulateUPSLoad][" << mPlayerData->getPlayerId() << "] getMyAccessToken= " << mUPSAccessToken.c_str());
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[UPSProxy::simulateUPSLoad][" << mPlayerData->getPlayerId() << "] getMyAccessToken failed");
		return false;
	}

	//getMessage()
	UPSProxy* upsProxy = BLAZE_NEW UPSProxy(mPlayerData);
	upsProxy->getMessage(mUPSAccessToken.c_str());
	delete upsProxy;

	return true;
}

bool UPSProxyHandler::getMyAccessToken()
{
	bool result = true;
	//get Auth Code
	eastl::string authCode = "";
	eastl::string accessToken = "";
	NucleusProxy * nucleusProxy = BLAZE_NEW NucleusProxy(mPlayerData);
	result = nucleusProxy->getPlayerAuthCode(authCode);
	if (!result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerData->getPlayerId() << "][UPSProxyHandler::getMyAccessToken]: Couldn't retrieve AuthCode");
	}
	else
	{
		result = nucleusProxy->getUPSPlayerAccessToken(authCode, accessToken);
		if (!result)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[" << mPlayerData->getPlayerId() << "][UPSProxyHandler::getMyAccessToken]: Access token failed");
		}
		else
		{
			mUPSAccessToken = accessToken;
		}
	}
	delete nucleusProxy;
	return result;
}

void UPSHttpResult::setValue(const char8_t* fullname, const char8_t* data, size_t dataLen)
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSHttpResult::setValue]:Received data = " << data);
	//// Read UPSGovId
	//const char* token = strstr(data, "UPSGovId");
	//if (token != NULL)
	//{
	//	const char* startPos = token + strlen("UPSGovId") + 2;
	//	char idValue[256] = { '\0' };
	//	bool foundStart = false;
	//	for (int index = 0, resindex = 0; index < (int)(strlen(data)); index++)
	//	{
	//		if (foundStart && startPos[index] == '"')
	//		{
	//			break;
	//		}
	//		if (!foundStart && startPos[index] == '"')
	//		{
	//			foundStart = true;
	//			continue;

	//		}
	//		if (foundStart)
	//		{
	//			idValue[resindex] = startPos[index];
	//			resindex++;
	//		}
	//	}
	//	if (blaze_strcmp(idValue, "null") != 0)
	//	{
	//		blaze_strnzcpy(UPSGovId, idValue, sizeof(UPSGovId));
	//	}
	//	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSHttpResult::setValue]:Received UPSGovId [" << UPSGovId << "] and data = " << data);
	//}
	//else
	//{
	//	BLAZE_ERR(BlazeRpcLog::usersessions, "[UPSHttpResult::setValue]: UPSGovId is null, data = %s ", data);
	//}

	//// Read trackingtag
	//const char* tokenId = strstr(data, "trackingtag");
	//if (tokenId != NULL)
	//{
	//	const char* startPos = tokenId + strlen("trackingtag") + 1;
	//	char idValue[256] = { '\0' };
	//	bool foundStart = false;
	//	for (int index = 0, resindex = 0; index < (int)(strlen(data)); index++)
	//	{
	//		if (foundStart && startPos[index] == ',')
	//		{
	//			break;
	//		}
	//		if (!foundStart && startPos[index] == ':')
	//		{
	//			foundStart = true;
	//			continue;

	//		}
	//		if (foundStart)
	//		{
	//			if (startPos[index] == '"') { continue; }
	//			if (startPos[index] == '}') { continue; }
	//			idValue[resindex] = startPos[index];
	//			resindex++;
	//		}
	//	}
	//	if (blaze_strcmp(idValue, "null") != 0)
	//	{
	//		blaze_strnzcpy(upstrackingtaglist, idValue, sizeof(upstrackingtaglist));
	//	}
	//	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[UPSHttpResult::setValue]:Received upstrackingtaglist [" << upstrackingtaglist << "] and data = " << data);
	//}
	//else
	//{
	//	BLAZE_ERR(BlazeRpcLog::usersessions, "[UPSHttpResult::setValue]: trackingtag is null, data = %s ", data);
	//}
}

//  *************************************************************************************************
//  UPS Stress code 
//  *************************************************************************************************

}  // namespace Stress
}  // namespace Blaze


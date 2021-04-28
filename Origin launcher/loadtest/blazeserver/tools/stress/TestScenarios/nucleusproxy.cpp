//  *************************************************************************************************
//
//   File:    NucleusProxy.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "nucleusproxy.h"

namespace Blaze {
namespace Stress {
//  *************************************************************************************************
//  Nucleus Stress code 
//  *************************************************************************************************
NucleusProxy::NucleusProxy(StressPlayerInfo* playerInfo) :mPlayerData(playerInfo)
{
	mAuthenticationUri[0] = '\0';
	//initializeNucleus(const ConfigMap& config, const CmdLineArguments& arguments)
	initNucleusProxyHTTPConnection();
}

NucleusProxy::~NucleusProxy()
{
	if (mHttpNucleusProxyConnMgr != NULL)
	{
		delete mHttpNucleusProxyConnMgr;
	}
}

bool NucleusProxy::initNucleusProxyHTTPConnection()
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[NucleusProxy::initNucleusProxyHTTPConnection]: Initializing the HttpConnectionManager");
	mHttpNucleusProxyConnMgr = BLAZE_NEW OutboundHttpConnectionManager("fifa-2021-ps4-lt");
	if (mHttpNucleusProxyConnMgr == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusProxy::initNucleusProxyHTTPConnection]: Failed to Initialize the HttpConnectionManager");
		return false;
	}
	const char8_t* serviceHostname = NULL;
	bool serviceSecure = false;

	InetAddress* inetAddress;
	uint16_t portNumber = 443;
	bool8_t	isSecure = true;
	
	//For now commenting out since KickOff needs to be implemented
	HttpProtocolUtil::getHostnameFromConfig("https://accounts.lt.ea.com", serviceHostname, serviceSecure);
	inetAddress = BLAZE_NEW InetAddress(serviceHostname, portNumber);
	if (inetAddress != NULL)
	{
		mHttpNucleusProxyConnMgr->initialize(*inetAddress, 1, isSecure, false);
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[NucleusProxy::initNucleusProxyHTTPConnection]: Failed to Create InetAddress with given hostname");
		return false;
	}
	return true;
}

bool NucleusProxy::getUPSPlayerAccessToken(eastl::string mAuthCode, eastl::string& accessToken)
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[NucleusProxy::getUPSPlayerAccessToken]: mAuthCode= " << mAuthCode.c_str() << "  playerId " << mPlayerData->getPlayerId());
	NucleusHttpResult httpResult;
	HttpParam params[4];
	// Fill the request here
	uint32_t paramCount = 0;

	params[paramCount].name = "grant_type";
	params[paramCount].value = "authorization_code";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "client_id";
	if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		params[paramCount].value = "FIFA21_PS4_CLIENT";
	}
	else
	{
		params[paramCount].value = "FIFA21_XONE_CLIENT";
	}
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "client_secret";
	if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		params[paramCount].value = "uShEXj0akReYZK07lxCteUa9zhpR2FCR1fJDPlF3jJEREqMotJddWDccU5kBifiOdrESFRSPG8cSiVNh";
	}
	else
	{
		params[paramCount].value = "SHt02JkZUHlRkvttP0yTVsotqfWta0hrDhew6tAG8TnsdnrgavWGuiwb0vQqSH9zOqYE6kwW7rN9hfHq";
	}
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "code";
	params[paramCount].value = mAuthCode.c_str();
	params[paramCount].encodeValue = true;
	paramCount++;

	eastl::string strHttpHeader;
	strHttpHeader.sprintf("'Content-Type: application/x-www-form-urlencoded");
	const char8_t *httpHeaders[1];
	httpHeaders[0] = strHttpHeader.c_str();

	bool result = sendNucleusHttpRequest(HttpProtocolUtil::HTTP_POST, "connect/token?redirect_uri=http://127.0.0.1/login_successful.html",
		params,
		paramCount,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		&httpResult);

	if (!result)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusProxy::getUPSPlayerAccessToken]: failed send the request");
		return false;
	}
	if (httpResult.hasError() && httpResult.getHttpStatusCode() != 302)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusProxy::getUPSPlayerAccessToken]: Nucleus returned error");
		return false;
	}
	else
	{
		accessToken = httpResult.getAccessToken();
		mUPSAccessToken = accessToken;
		BLAZE_INFO_LOG(Log::SYSTEM, "[NucleusProxy::getUPSPlayerAccessToken] Access Token generated success: " << accessToken);
	}
	return true;
}

bool NucleusProxy::getPlayerAccessToken(eastl::string mAuthCode, eastl::string& accessToken)
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[NucleusProxy::getPlayerAccessToken]: mAuthCode= " << mAuthCode.c_str() <<"  playerId " << mPlayerData->getPlayerId());
	NucleusHttpResult httpResult;
	HttpParam params[4];
	// Fill the request here
	uint32_t paramCount = 0;

	params[paramCount].name = "grant_type";
	params[paramCount].value = "authorization_code";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "code";
	params[paramCount].value = mAuthCode.c_str();
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "client_id";
	if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		params[paramCount].value = "FIFA21_PS4_CLIENT";
	}
	else
	{
		params[paramCount].value = "FIFA21_XONE_CLIENT";
	}
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "client_secret";
	if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		params[paramCount].value = "uShEXj0akReYZK07lxCteUa9zhpR2FCR1fJDPlF3jJEREqMotJddWDccU5kBifiOdrESFRSPG8cSiVNh";
	}
	else 
	{
		params[paramCount].value = "SHt02JkZUHlRkvttP0yTVsotqfWta0hrDhew6tAG8TnsdnrgavWGuiwb0vQqSH9zOqYE6kwW7rN9hfHq";
	}

	params[paramCount].encodeValue = true;
	paramCount++;

	eastl::string strHttpHeader;
	strHttpHeader.sprintf("'Content-Type: application/x-www-form-urlencoded");
	const char8_t *httpHeaders[1];
	httpHeaders[0] = strHttpHeader.c_str();

	bool result = sendNucleusHttpRequest(HttpProtocolUtil::HTTP_POST, "connect/token?redirect_uri=http://127.0.0.1/login_successful.html",
		params,
		paramCount,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		&httpResult);

	if (!result)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusProxy::getPlayerAccessToken]: failed send the request");
		return false;
	}
	if (httpResult.hasError() && httpResult.getHttpStatusCode() != 302)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusProxy::getPlayerAccessToken]: Nucleus returned error");
		return false;
	}
	else
	{
		accessToken = httpResult.getAccessToken();
		mAccessToken = accessToken;
		BLAZE_INFO_LOG(Log::SYSTEM, "[NucleusProxy::getPlayerAccessToken] Access Token generated success: " << accessToken);
	}
	return true;
}
bool NucleusProxy::getGuestAuthCode(eastl::string& mGuestAuthCode)
{
	NucleusHttpResult httpResult;
	HttpParam params[6];
	// Fill the request here
	int paramCount = 0;

	params[paramCount].name = "response_type";
	params[paramCount].value = "code";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "guest_mode";
	params[paramCount].value = "true";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "isgen4";
	params[paramCount].value = "true";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "client_id";
	//params[paramCount].value = mClientId;
	//params[paramCount].value = "FIFA-18-PS4-Blaze-Server";
	params[paramCount].value = "FIFA-19-PS4-Guest-Client";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "locale";
	params[paramCount].value = "en_US";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "redirect_uri";
	params[paramCount].value = "http://127.0.0.1/login_successful.html";
	params[paramCount].encodeValue = true;
	paramCount++;

	eastl::string strHttpHeader;
	//blaze_strnzcpy(mTokenFormat, "psn_ticket: psn_", sizeof(mTokenFormat));
	//blaze_strnzcpy(mTokenFormat, "xtoken: xbox_", sizeof(mTokenFormat));
	//blaze_snzprintf(tokenName, sizeof(tokenName), mTokenFormat, index + id);

	strHttpHeader.sprintf("psn_ticket: psn_guest333");
	/*char8_t tokenFormat[256];
	memset(tokenFormat, '\0', 256);
	uint32_t startIndex = mPlayerData->getLogin()->getMyStartIndex();
	uint32_t identValue = mPlayerData->getLogin()->getMyIdent();
	blaze_snzprintf(tokenFormat, sizeof(tokenFormat), mPlayerData->getLogin()->getOwner()->getTokenFormat(), startIndex + identValue);*/
	//BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[NucleusProxy::getPlayerAuthCode]:Token format for: [" << mPlayerData->getPlayerId() << "] generated successfully: " << tokenFormat << ".");
	//strHttpHeader.sprintf(tokenFormat);
	const char8_t *httpHeaders[1];
	httpHeaders[0] = strHttpHeader.c_str();

	bool result = sendNucleusHttpRequest(HttpProtocolUtil::HTTP_GET, "/connect/auth",
		params,
		paramCount,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		&httpResult);

	if (!result)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusProxy::getPlayerAuthCode]: failed send the request");
		return false;
	}
	if (httpResult.hasError() && httpResult.getHttpStatusCode() != 302)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusProxy::getPlayerAuthCode]: Nucleus returned error");
		return false;
	}
	char8_t *authCode = httpResult.getCode((char *)"Location");

	if (authCode != NULL)
	{
		mGuestAuthCode = (eastl::string) authCode;
		//blaze_strnzcpy(mAuthCode, authCode, sizeof(mAuthCode));

		BLAZE_INFO_LOG(Log::SYSTEM, "[NucleusProxy::getPlayerAuthCode] Auth code generated success: " << authCode);
		delete[] authCode;   //delete temporary authCode created on Heap
		return true;
	}
	else
	{
		BLAZE_ERR_LOG(Log::SYSTEM, "[NucleusProxy::getPlayerAuthCode]Auth Code for generated Failed.");
		return false;
	}
	return true;
}

bool NucleusProxy::getGuestPersonaId(eastl::string& mGuestAccessToken, eastl::string& mGuestPersonaId)
{
	NucleusHttpResult httpResult;
	HttpParam params[4];
	uint8_t paramCount = 0;
	bool result = false;
	params[paramCount].name = "access_token";
	params[paramCount].value = mGuestAccessToken.c_str();
	params[paramCount].encodeValue = true;
	paramCount++;
	eastl::string strHttpHeader;
	strHttpHeader = "";

	const char8_t *httpHeaders[1];
	httpHeaders[0] = strHttpHeader.c_str();
	
	result = sendNucleusHttpRequest(HttpProtocolUtil::HTTP_GET, 
		"/connect/tokeninfo",
		params,
		paramCount,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		&httpResult);
	if (!result)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusProxy::getPlayerAccessToken]: failed send the request");
		return false;
	}
	if (httpResult.hasError() && httpResult.getHttpStatusCode() != 302)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusProxy::getGuestPersonaId]: Could not be generated the persona Id");
		return false;
	}

	return true;
}

bool NucleusProxy::getGuestAccessToken(eastl::string& mGuestAuthCode, eastl::string& mGuestAccessToken)
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[NucleusProxy::getPlayerAccessToken]: mAuthCode= " << mGuestAuthCode.c_str() << "  playerId " << mPlayerData->getPlayerId());
	NucleusHttpResult httpResult;
	HttpParam params[4];
	// Fill the request here
	uint32_t paramCount = 0;

	params[paramCount].name = "grant_type";
	params[paramCount].value = "authorization_code";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "code";
	params[paramCount].value = mGuestAuthCode.c_str();
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "client_id";
	params[paramCount].value = "FIFA-19-PS4-Guest-Client";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "client_secret";
	params[paramCount].value = "7BAHHXNHSaeISek3LZF3hLXJSFA3IVmvehQjpxOUBjN7y7cmPrvitQC3kJi3ntfWxlKjWMG";
	params[paramCount].encodeValue = true;
	paramCount++;

	eastl::string strHttpHeader;
	strHttpHeader.sprintf("'Content-Type: application/x-www-form-urlencoded");
	const char8_t *httpHeaders[1];
	httpHeaders[0] = strHttpHeader.c_str();

	bool result = sendNucleusHttpRequest(HttpProtocolUtil::HTTP_POST, "connect/token?redirect_uri=http://127.0.0.1/login_successful.html",
		params,
		paramCount,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		&httpResult);

	if (!result)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusProxy::getPlayerAccessToken]: failed send the request");
		return false;
	}
	if (httpResult.hasError() && httpResult.getHttpStatusCode() != 302)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusProxy::getPlayerAccessToken]: Nucleus returned error");
		return false;
	}
	else
	{
		mGuestAccessToken = httpResult.getAccessToken();
		BLAZE_INFO_LOG(Log::SYSTEM, "[NucleusProxy::getGuestAccesstoken] Access token generated success: " << mGuestAccessToken);
	}
	return true;
}

bool NucleusProxy::getPlayerAuthCode(eastl::string& mAuthCode)
{
	NucleusHttpResult httpResult;
	HttpParam params[4];
	// Fill the request here
	uint32_t paramCount = 0;

	params[paramCount].name = "response_type";
	params[paramCount].value = "code";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "client_id";
	if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		params[paramCount].value = "FIFA21_PS4_CLIENT";
	}
	else
	{
		params[paramCount].value = "FIFA21_XONE_CLIENT";
	}
	params[paramCount].encodeValue = true;
	paramCount++;

	eastl::string strHttpHeader;
	//blaze_strnzcpy(mTokenFormat, "psn_ticket: psn_", sizeof(mTokenFormat));
	//blaze_strnzcpy(mTokenFormat, "xtoken: xbox_", sizeof(mTokenFormat));
	//blaze_snzprintf(tokenName, sizeof(tokenName), mTokenFormat, index + id);
	
	//strHttpHeader.sprintf("xtoken: xbox_x1fifa19-0000001");
	char8_t tokenFormat[256];
	memset(tokenFormat, '\0', 256);
	uint32_t startIndex = mPlayerData->getLogin()->getMyStartIndex();
	uint32_t identValue = mPlayerData->getLogin()->getMyIdent();
	blaze_snzprintf(tokenFormat, sizeof(tokenFormat), mPlayerData->getLogin()->getOwner()->getTokenFormat(), startIndex + identValue);
	/*std::string key(":");
	std::string tokenFormats(tokenFormat);
	std::size_t found = tokenFormats.rfind(key);
	if (found != std::string::npos)
	{
		tokenFormats.replace(found, key.length(), "=");
	}*/
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[NucleusProxy::getPlayerAuthCode]:Token format for: [" << mPlayerData->getPlayerId() << "] generated successfully: " << tokenFormat << ".");
	strHttpHeader.sprintf(tokenFormat);
	const char8_t *httpHeaders[1];
	httpHeaders[0] = strHttpHeader.c_str();

	params[paramCount].name = "display";
	params[paramCount].value = "console2/welcome";
	params[paramCount].encodeValue = true;
	paramCount++;

	bool result = sendNucleusHttpRequest(HttpProtocolUtil::HTTP_GET,"/connect/auth",
		params,
		paramCount,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		&httpResult);

	if (!result)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusProxy::getPlayerAuthCode]: failed send the request");
		return false;
	}
	if (httpResult.hasError() && httpResult.getHttpStatusCode() != 302)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusProxy::getPlayerAuthCode]: Nucleus returned error");
		return false;
	}
	char8_t *authCode = httpResult.getCode((char *)"Location");
	
	if (authCode != NULL)
	{
		mAuthCode = (eastl::string) authCode;
		//blaze_strnzcpy(mAuthCode, authCode, sizeof(mAuthCode));
		
		BLAZE_INFO_LOG(Log::SYSTEM, "[NucleusProxy::getPlayerAuthCode] Auth code generated success: " << authCode);
		delete[] authCode;   //delete temporary authCode created on Heap
		return true;
	}
	else
	{
		BLAZE_ERR_LOG(Log::SYSTEM, "[NucleusProxy::getPlayerAuthCode]Auth Code for generated Failed.");
		return false;
	}
}

bool NucleusProxy::sendNucleusHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult& result, OutboundHttpConnection::ContentPartList& contentList)
{
	BlazeRpcError err = ERR_SYSTEM;
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[NucleusProxy::sendNucleusHttpRequest][" << mPlayerData->getPlayerId() << "] method= " << method << " URI= " << URI << " httpHeaders= " << httpHeaders);

	err = mHttpNucleusProxyConnMgr->sendRequest(method, URI, NULL, 0, httpHeaders, headerCount, &result, &contentList);

	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[NucleusProxy::sendNucleusHttpRequest][" << mPlayerData->getPlayerId() << "] failed to send http request");
		return false;
	}
	return true;
}

bool NucleusProxy::sendNucleusHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult* result)
{
	BlazeRpcError err = ERR_SYSTEM;
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[NucleusProxy::sendNucleusHttpRequest][" << mPlayerData->getPlayerId() << "] method= " << method << " URI= " << URI << " httpHeaders= " << httpHeaders);
	err = mHttpNucleusProxyConnMgr->sendRequest(method, URI, params, paramsize, httpHeaders, headerCount, result);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[NucleusProxy::sendNucleusHttpRequest][" << mPlayerData->getPlayerId() << "] failed to send http request");
		return false;
	}
	return true;
}

//  *************************************************************************************************
//  Nucleus Stress code 
//  *************************************************************************************************

}  // namespace Stress
}  // namespace Blaze


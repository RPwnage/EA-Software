//  *************************************************************************************************
//
//   File:    statsproxy.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "statsproxy.h"

namespace Blaze {
namespace Stress {
//  *************************************************************************************************
//  Stats Stress code 
//  *************************************************************************************************
StatsProxy::StatsProxy(StressPlayerInfo* playerInfo) :mPlayerData(playerInfo)
{
	mHttpStatsProxyConnMgr = NULL;
	initStatsProxyHTTPConnection();
}

StatsProxy::~StatsProxy()
{
	if (mHttpStatsProxyConnMgr != NULL)
	{
		delete mHttpStatsProxyConnMgr;
	}
}

bool StatsProxy::initStatsProxyHTTPConnection()
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[StatsProxy::initStatsProxyHTTPConnection]: Initializing the HttpConnectionManager");
	mHttpStatsProxyConnMgr = BLAZE_NEW OutboundHttpConnectionManager("fifa-2021-ps4-lt");
	if (mHttpStatsProxyConnMgr == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[StatsProxy::initStatsProxyHTTPConnection]: Failed to Initialize the HttpConnectionManager");
		return false;
	}
	const char8_t* serviceHostname = NULL;
	bool serviceSecure = false;

	HttpProtocolUtil::getHostnameFromConfig(StressConfigInst.getStatsProxyServerUri(), serviceHostname, serviceSecure);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[StatsProxy::initStatsProxyHTTPConnection]: StatsServerUri= " << StressConfigInst.getStatsProxyServerUri());

	InetAddress* inetAddress;
	//PORT number = 11000
	uint16_t portNumber = 11000;
	bool8_t	isSecure = true;
	inetAddress = BLAZE_NEW InetAddress(serviceHostname, portNumber);
	if (inetAddress != NULL)
	{
		mHttpStatsProxyConnMgr->initialize(*inetAddress, 1, isSecure, false);
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[StatsProxy::initStatsProxyHTTPConnection]: Failed to Create InetAddress with given hostname");
		return false;
	}
	return true;
}

bool StatsProxy::sendStatsHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult* result)
{
	BlazeRpcError err = ERR_SYSTEM;
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[StatsProxy::sendStatsHttpRequest][" << mPlayerData->getPlayerId() << "] method= " << method << " URI= " << URI << " httpHeaders= " << httpHeaders);
	err = mHttpStatsProxyConnMgr->sendRequest(method, URI, params, paramsize, httpHeaders, headerCount, result);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[StatsProxy::sendStatsHttpRequest][" << mPlayerData->getPlayerId() << "] failed to send http request");
		return false;
	}
	return true;
}

bool StatsProxy::viewEntityStats(eastl::string viewName, eastl::string entityName, eastl::string nucleusAccessToken)
{
	StatsHttpResult httpResult;
	bool result = false;

	BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[StatsProxy::viewEntityStats][" << mPlayerData->getPlayerId() << "] statsURI && viewName ");
	if (nucleusAccessToken == "")
	{
		eastl::string groupsAuthCode = "";
		NucleusProxy * nucleusProxy = BLAZE_NEW NucleusProxy(mPlayerData);
		result = nucleusProxy->getPlayerAuthCode(groupsAuthCode);
		if (result)
		{
			result = nucleusProxy->getPlayerAccessToken(groupsAuthCode, nucleusAccessToken);
		}
		delete nucleusProxy;
		if (!result)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[StatsProxy::viewEntityStats][" << mPlayerData->getPlayerId() << "] failed to get access token");
			return result;
		}
	}


	//GET https ://stats.int.gameservices.ea.com:11000/api/1.0/contexts/FIFA19/views/UserCustomStatsView/entities/stats?entity_ids=1000269138228
	//GET https ://stats.int.gameservices.ea.com:11000/api/1.0/contexts/FIFA19/views/UserCustomStatsView/entities/stats?entity_ids=1000270515715
	//GET https ://stats.int.gameservices.ea.com:11000/api/1.0/contexts/FIFA19/views/RivalStatsView/entities/stats?entity_ids=a2640131%2D00c7%2D47dc%2D8b2f%2D12892c7319c2 
	//GET https ://stats.int.gameservices.ea.com:11000/api/1.0/contexts/FIFA19/views/OverallStatsView/entities/stats?entity_ids=e800aedb%2D0b89%2D475b%2D98b5%2D6aa67dcfbe3d%2Ce86c3966%2Da591%2D4e1e%2Db3cb%2D2656cd8fd758
	//GET https ://stats.int.gameservices.ea.com:11000/api/1.0/contexts/FIFA19/views/ControllerStatsView/entities/stats?entity_ids=1000269138228%2C1000270515715

	//URI
	eastl::string statsURI = "";
	statsURI = StressConfigInst.getStatsProxyGetMessageUri() + viewName + "/entities/stats";

	HttpParam param[1];
	int paramCount = 0;

	param[paramCount].name = "entity_ids";
	param[paramCount].value = entityName.c_str(); 
	param[paramCount].encodeValue = true;
	paramCount++;

	eastl::string strHttpHeader = "";
	const char8_t *httpHeaders[2];

	strHttpHeader += "Authorization: NEXUS ";
	strHttpHeader += nucleusAccessToken.c_str();
	httpHeaders[0] = strHttpHeader.c_str();

	httpHeaders[1] = "Accept: application/json";

	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[StatsProxy::viewEntityStats][" << mPlayerData->getPlayerId() << "] httpHeaders[0]= " << httpHeaders[0]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[StatsProxy::viewEntityStats][" << mPlayerData->getPlayerId() << "] statsURI && viewName " << statsURI.c_str());

	result = sendStatsHttpRequest(HttpProtocolUtil::HTTP_GET, statsURI.c_str(),
		param,
		paramCount,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		&httpResult);

	if (!result)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[StatsProxy::viewEntityStats]: failed send the request");
		return false;
	}
	if (httpResult.hasError() && httpResult.getHttpStatusCode() != 302)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[StatsProxy::viewEntityStats]: Stats returned error");
		return false;
	}
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StatsProxy::viewEntityStats] viewEntityStats successful");
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[StatsProxy::viewEntityStats]::[" << mPlayerData->getPlayerId() << "] successful ");
	return true;
}

// if Stats Enabled
bool  StatsProxyHandler::viewEntityStats(eastl::string viewName, eastl::string entityName, eastl::string nucleusAccessToken)
{
	bool result = false;
	StatsProxy* statsProxy = BLAZE_NEW StatsProxy(mPlayerData);
	result = statsProxy->viewEntityStats(viewName, entityName, nucleusAccessToken);
	delete statsProxy;
	return result;
}

bool StatsProxy::trackPlayerHealth(eastl::string entityName, eastl::string nucleusAccessToken)
{
	StatsHttpResult httpResult;
	bool result = false;

	BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[StatsProxy::viewEntityStats][" << mPlayerData->getPlayerId() << "] statsURI && viewName ");
	if (nucleusAccessToken == "")
	{
		eastl::string groupsAuthCode = "";
		NucleusProxy * nucleusProxy = BLAZE_NEW NucleusProxy(mPlayerData);
		result = nucleusProxy->getPlayerAuthCode(groupsAuthCode);
		if (result)
		{
			result = nucleusProxy->getPlayerAccessToken(groupsAuthCode, nucleusAccessToken);
		}
		delete nucleusProxy;
		if (!result)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[StatsProxy::viewEntityStats][" << mPlayerData->getPlayerId() << "] failed to get access token");
			return result;
		}
	}


	//GET https://internal.fifa.stats.e2etest.gameservices.ea.com:11000/api/1.0/contexts/FIFA21_PH/views/AllPlayerHealthStatsView/entities/stats?entity_ids=1100476652436

	//URI
	eastl::string statsURI = "";
	statsURI = StressConfigInst.getStatsPlayerHealthUri();

	HttpParam param[1];
	int paramCount = 0;

	param[paramCount].name = "entity_ids";
	param[paramCount].value = entityName.c_str();
	param[paramCount].encodeValue = true;
	paramCount++;

	eastl::string strHttpHeader = "";
	const char8_t *httpHeaders[2];

	strHttpHeader += "Authorization: NEXUS ";
	strHttpHeader += nucleusAccessToken.c_str();
	httpHeaders[0] = strHttpHeader.c_str();

	httpHeaders[1] = "Accept: application/json";

	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[StatsProxy::trackPlayerHealth][" << mPlayerData->getPlayerId() << "] httpHeaders[0]= " << httpHeaders[0]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[StatsProxy::trackPlayerHealth][" << mPlayerData->getPlayerId() << "] statsURI && viewName " << statsURI.c_str());

	result = sendStatsHttpRequest(HttpProtocolUtil::HTTP_GET, statsURI.c_str(),
		param,
		paramCount,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		&httpResult);

	if (!result)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[StatsProxy::trackPlayerHealth]: failed send the request");
		return false;
	}
	if (httpResult.hasError() && httpResult.getHttpStatusCode() != 302)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[StatsProxy::trackPlayerHealth]: Stats returned error");
		return false;
	}

	uint32_t optInValue= httpResult.getOptIn();
	if (optInValue == -1)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[StatsProxy::trackPlayerHealth]: " << mPlayerData->getPlayerId() << " optInValue is not found");
		return false;
	}
	((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->optIn = optInValue;
	BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[StatsProxy::trackPlayerHealth]::[" << mPlayerData->getPlayerId() << "] successful, opt_in = " << optInValue);

	EadpStatsMap eadpStatsMapValue = httpResult.getEadpStatsMap();
	if (eadpStatsMapValue.empty())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[StatsProxy::trackPlayerHealth]: " << mPlayerData->getPlayerId() << " Map is not found");
		return false;
	}

	//EadpStatsMap& mEadpStatsMap = ((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->eadpStatsMap;
	//((Blaze::Stress::PlayerModule*)mPlayerInfo->getOwner())->futPlayerMap.insert(eastl::pair<PlayerId, eastl::string>(mPlayerInfo->getPlayerId(), mPlayerInfo->getPersonaName()));
	EadpStatsMap::iterator itr;
	for (itr = eadpStatsMapValue.begin(); itr != eadpStatsMapValue.end(); ++itr)
	{
		((Blaze::Stress::PlayerModule*)mPlayerData->getOwner())->eadpStatsMap.insert(eastl::pair<eastl::string, int>(itr->first, itr->second));
	}
	BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[StatsProxy::trackPlayerHealth] trackPlayerHealth successful");
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[StatsProxy::trackPlayerHealth]::[" << mPlayerData->getPlayerId() << "] successful ");
	return true;
}

bool  StatsProxyHandler::trackPlayerHealth(eastl::string entityName, eastl::string nucleusAccessToken)
{
	bool result = false;
	StatsProxy* statsProxy = BLAZE_NEW StatsProxy(mPlayerData);
	result = statsProxy->trackPlayerHealth(entityName, nucleusAccessToken);
	delete statsProxy;
	return result;
}

void StatsHttpResult::setValue(const char8_t* fullname, const char8_t* data, size_t dataLen)
{	
	//BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[StatsProxy::setValue]:Received data = " << data);
	const char* optIn = strstr(data, "opt_in");
	if (optIn != NULL)
	{
		const char* startPos = optIn + strlen("opt_in") + 2;
		char optInValue[256] = { '\0' };
		bool foundStart = false;
		for (int index = 0, resindex = 0; index < (int)(strlen(data)); index++)
		{
			if (foundStart && startPos[index] == '}')
			{
				break;
			}
			if (!foundStart && startPos[index] == ':')
			{
				foundStart = true;
				continue;

			}
			if (foundStart)
			{
				optInValue[resindex] = startPos[index];
				resindex++;
			}
		}
		
		if (blaze_strcmp(optInValue, "null") != 0)
		{
			uint32_t optInInteger = std::stoi(optInValue);
			mOptIn = optInInteger;
		}
		else
		{
			mOptIn = (uint32_t)-1;
		}
	}
	else
	{
		mOptIn = (uint32_t)-1;
	}

	const char* stats = strstr(data, "stats");
	if (stats != NULL)
	{
		const char* categoryId = strstr(data, "categoryId"); // Getting the start position of categoryId
		if (categoryId != NULL)
		{
			const char* startPos = categoryId + strlen("categoryId") + 2;
			char categoryIdValue[256] = { '\0' };
			bool foundStart = false;
			for (int index = 0, resindex = 0; index < (int)(strlen(data)); index++)
			{
				if (foundStart && startPos[index] == '"')
				{
					break;
				}
				if (!foundStart && startPos[index] == '"')
				{
					foundStart = true;
					continue;
				}
				if (foundStart)
				{
					categoryIdValue[resindex] = startPos[index];
					resindex++;
				}
			}
			if (blaze_strcmp(categoryIdValue, "null") != 0)
			{
				blaze_strnzcpy(mCategoryId, categoryIdValue, sizeof(mCategoryId));
			}
			//BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[StatsProxy::setValue]:Received categoryId [" << mCategoryId << "] and data = " << data);
		}
		eastl::string categoryID = mCategoryId;
		if (categoryID != "PlayTimeStats")
			return;
		const char* values = strstr(data, "values"); // Getting the start position of values
		if (values != NULL)
		{
			const char* startPos = values + strlen("values") + 2;
			char statsName[256] = { '\0' };
			bool foundStart = false;
			int resindex = 0;
			while (startPos != NULL && *startPos != ']')
			{
				if (foundStart && *startPos == '"')
				{
					foundStart = false;
					//std::cout<<"StatsName : "<<statsName<<"\n";
					startPos = getStatsValue(startPos, statsName);
					for (int i = 0; i < statsName[i]; i++)   // Clearing the array content and resetting the array index 
					{
						statsName[i] = '\0';
					}
					resindex = 0;
				}
				if (!foundStart && *startPos == '"')
				{
					foundStart = true;
					++startPos;
					continue;
				}
				if (foundStart)
				{
					statsName[resindex] = *startPos;
					resindex++;
				}
				++startPos;
			}
			EadpStatsMap::iterator itr;
			for (itr = mEadpStatsMap.begin(); itr != mEadpStatsMap.end(); ++itr) 
			{
				BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[StatsHttpResult::setValue]:Map = " << itr->first <<" : "<<itr->second);
			}
		}
	}
	//BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[StatsHttpResult::setValue]:viewEntityStats Received data = " << data);
	//// Read govid
	//const char* token = strstr(data, "govid");
	//if (token != NULL)
	//{
	//	const char* startPos = token + strlen("govid") + 2;
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
	//		blaze_strnzcpy(govid, idValue, sizeof(govid));
	//	}
	//	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[StatsHttpResult::setValue]:Received govid [" << govid << "] and data = " << data);
	//}
	//else
	//{
	//	BLAZE_ERR(BlazeRpcLog::usersessions, "[StatsHttpResult::setValue]: govid is null, data = %s ", data);
	//}
}

const char* StatsHttpResult::getStatsValue(const char* data, char statsName[])
{
	const char* value = strstr(data, "value");
	if (value != NULL)
	{
		const char* startPos = value + strlen("value") + 1;
		char statsValue[256] = { '\0' };
		bool foundStart = false;
		int resindex = 0;
		while (startPos != NULL)
		{
			if (*startPos == ',')    //If there is no value in the stats
			{
				data = startPos;
				return data;
			}
			if (foundStart && *startPos == '}')
			{
				break;
			}
			if (!foundStart && *startPos == ':')
			{
				foundStart = true;
				++startPos;
				continue;

			}
			if (foundStart)
			{
				statsValue[resindex] = *startPos;
				resindex++;
			}
			++startPos;
		}
		int statsValueInt = std::stoi(statsValue);

		mEadpStatsMap.insert(eastl::pair<string, int>(statsName, statsValueInt));
		
		data = startPos;    // Setting data to position after the value
		return data;
	}
	return data;
}


}  // namespace Stress
}  // namespace Blaze


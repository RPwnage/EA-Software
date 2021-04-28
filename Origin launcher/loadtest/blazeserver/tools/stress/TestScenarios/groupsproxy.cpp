//  *************************************************************************************************
//
//   File:    groupsproxy.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "groupsproxy.h"

namespace Blaze {
namespace Stress {
//  *************************************************************************************************
//  GroupsProxy Stress code 
//  *************************************************************************************************
GroupsProxy::GroupsProxy(StressPlayerInfo* playerInfo) :mPlayerData(playerInfo)
{
	mHttpGroupsProxyConnMgr = NULL;
	initGroupsProxyHTTPConnection();
}

GroupsProxy::~GroupsProxy()
{
	if (mHttpGroupsProxyConnMgr != NULL)
	{
		delete mHttpGroupsProxyConnMgr;
	}
}

bool GroupsProxy::initGroupsProxyHTTPConnection()
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::initGroupsProxyHTTPConnection]: Initializing the HttpConnectionManager");
	mHttpGroupsProxyConnMgr = BLAZE_NEW OutboundHttpConnectionManager("fifa-2021-ps4-lt");
	if (mHttpGroupsProxyConnMgr == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsProxy::initGroupsProxyHTTPConnection]: Failed to Initialize the HttpConnectionManager");
		return false;
	}
	const char8_t* serviceHostname = NULL;
	bool serviceSecure = false;

	HttpProtocolUtil::getHostnameFromConfig(StressConfigInst.getGroupsServerUri(), serviceHostname, serviceSecure);
	//HttpProtocolUtil::getHostnameFromConfig("https://groups-e2e-black.tntdev.tnt-ea.com:443", serviceHostname, serviceSecure);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::initGroupsProxyHTTPConnection]: GroupsServerUri= " << StressConfigInst.getGroupsServerUri());

	InetAddress* inetAddress;
	uint16_t portNumber = 443;
	bool8_t	isSecure = true;
	inetAddress = BLAZE_NEW InetAddress(serviceHostname, portNumber);
	if (inetAddress != NULL)
	{
		mHttpGroupsProxyConnMgr->initialize(*inetAddress, 1, isSecure, false);
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::initGroupsProxyHTTPConnection]: Failed to Create InetAddress with given hostname");
		return false;
	}
	return true;
}

bool GroupsProxy::sendGroupsHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult* result)
{
	BlazeRpcError err = ERR_SYSTEM;
	//BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::sendGroupsHttpRequest][" << mPlayerData->getPlayerId() << "] method= " << method << " URI= " << URI << " httpHeaders= " << httpHeaders);
	err = mHttpGroupsProxyConnMgr->sendRequest(method, URI, params, paramsize, httpHeaders, headerCount, result);
	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::sendGroupsHttpRequest][" << mPlayerData->getPlayerId() << "] failed to send http request");
		return false;
	}
	return true;
}

bool GroupsProxy::sendGroupsHttpPostRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult& result, const char8_t* httpHeader_ContentType, const char8_t* contentBody, uint32_t contentLength)
{
	BlazeRpcError err = ERR_SYSTEM;
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::sendGroupsHttpRequest][" << mPlayerData->getPlayerId() << "] method= " << method << " URI= " << URI << " httpHeaders= " << httpHeaders);

	err = mHttpGroupsProxyConnMgr->sendRequest(
		HttpProtocolUtil::HTTP_POST,
		URI,
		httpHeaders,
		5,
		&result,
		httpHeader_ContentType,
		contentBody,
		contentLength
	);

	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::sendGroupsHttpPostRequest][" << mPlayerData->getPlayerId() << "] failed to send http request");
		return false;
	}
	return true;
}

bool GroupsProxy::createInstance(eastl::string creatorUserId, eastl::string groupName, eastl::string groupShortName, eastl::string groupTypeId, eastl::string nucleusAccessToken)
{
	bool result = false;
	GroupsHttpResult httpResult;
	const int headerLen = 128;

	char8_t httpHeader_ActingUser[headerLen];
	char8_t httpHeader_ActingUserType[headerLen];
	char8_t httpHeader_ApiVersion[headerLen];
	char8_t httpHeader_ApplicationKey[headerLen];
	char8_t httpHeader_AuthToken[headerLen];
	char8_t httpHeader_AcceptType[headerLen];
	char8_t httpHeader_ContentType[headerLen];

	blaze_snzprintf(httpHeader_ActingUser, headerLen, "X-Acting-UserId: %s", creatorUserId.c_str());
	blaze_strnzcpy(httpHeader_ActingUserType, "X-Acting-UserType: NUCLEUS_PERSONA", headerLen);
	blaze_strnzcpy(httpHeader_ApiVersion, "X-Api-Version: 2", headerLen);
	blaze_strnzcpy(httpHeader_ApplicationKey, "X-Application-Key: mobile", headerLen);
	blaze_snzprintf(httpHeader_AuthToken, headerLen, "X-AuthToken: %s", nucleusAccessToken.c_str());
	blaze_strnzcpy(httpHeader_AcceptType, "Content-Type: application/json", headerLen);

	const char8_t *httpHeaders[5];
	httpHeaders[0] = httpHeader_ActingUser;
	httpHeaders[1] = httpHeader_ActingUserType;
	httpHeaders[2] = httpHeader_ApiVersion;
	httpHeaders[3] = httpHeader_ApplicationKey;
	httpHeaders[4] = httpHeader_AuthToken;

	//GET https ://groups.integration.gameservices.ea.com:443/group/instance/search?name=1000269138228&typeId=FIFA19FriendsPS4

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
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] failed to get access token");
			return result;
		}
	}

	eastl::string jsonData;
	jsonData.append("{\"name\":\"");
	jsonData.append(groupName);
	jsonData.append("\",\"shortName\":\"");
	jsonData.append(groupShortName);
	jsonData.append("\",\"groupTypeId\":\"");
	jsonData.append(groupTypeId);
	jsonData.append("\",\"creator\":\"");
	jsonData.append(creatorUserId);
	jsonData.append("\",\"size\":1,");
	jsonData.append("\"instanceJoinConfig\":{\"inviteURLKey\":\"\"},\"override\":{}}");
	size_t contentLength = 0;
	contentLength = jsonData.length();
	const char * groupsURI = "";
	groupsURI = "/group/instance";

	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::createInstanceInfo][" << mPlayerData->getPlayerId() << "] URI= " << groupsURI);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::createInstanceInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[0]= " << httpHeaders[0]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::createInstanceInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[1]= " << httpHeaders[1]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::createInstanceInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[2]= " << httpHeaders[2]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::createInstanceInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[3]= " << httpHeaders[3]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::createInstanceInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[4]= " << httpHeaders[4]);

	result = sendGroupsHttpPostRequest(
		HttpProtocolUtil::HTTP_POST,
		groupsURI,
		httpHeaders,
		5,
		httpResult,
		httpHeader_ContentType,
		jsonData.c_str(),
		contentLength
	);

	if (!result)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsProxy::createInstance]: to POST the createInstance request");
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::CreateInstance :] groupName = " << groupName);
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::CreateInstance :] groupShortName = " << groupShortName);
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::CreateInstance :] groupTypeID = " << groupTypeId);
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::CreateInstance :] creator id = " << creatorUserId);
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::CreateInstance :] JsonData = " << jsonData);
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::CreateInstance :] http error code = " << httpResult.getHttpStatusCode());

		return false;
	}
	if (httpResult.hasError() && httpResult.getHttpStatusCode() != 302)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsProxy::createInstance]: Groups returned error");
		return false;
	}
	//Checking for groupID
	if (httpResult.getGroupID() == "")
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsProxy::createInstance]: groupID is Null");
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::createInstanceInfo][ groupID is Null for groupShortName = "<<groupShortName.c_str());
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::createInstanceInfo][ groupID is Null for groupName = "<<groupName.c_str());
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::createInstanceInfo][ groupID is Null for CreatorUserId = " << creatorUserId);
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::createInstanceInfo][ groupID is Null , groupTypeId " << groupTypeId.c_str());
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::createInstanceInfo][ groupID is Null for groupShortName = " << jsonData);
		return false;
	}
	mPlayerData->setKickOffGroupId(httpResult.getGroupID());	
	return true;
}

bool GroupsProxyHandler::createInstance(eastl::string creatorUserId, eastl::string groupName, eastl::string groupShortName, eastl::string groupTypeId, eastl::string nucleusAccessToken)
{
	bool result = false;
	GroupsProxy* groupsProxy = BLAZE_NEW GroupsProxy(mPlayerData);
	result = groupsProxy->createInstance(creatorUserId, groupName, groupShortName, groupTypeId, nucleusAccessToken);
	delete groupsProxy;
	return result;
}
bool GroupsProxy::getGroupIDInfo(eastl::string groupName, eastl::string grouptType, eastl::string& groupID, eastl::string nucleusAccessToken)
{
	bool result = false;
	GroupsHttpResult httpResult;

	HttpParam param[2];
	int paramCount = 0;

	//GET https ://groups.integration.gameservices.ea.com:443/group/instance/search?name=1000269138228&typeId=FIFA19FriendsPS4

	param[paramCount].name = "name";
	param[paramCount].value = groupName.c_str();
	param[paramCount].encodeValue = true;
	paramCount++;

	param[paramCount].name = "typeId";
	param[paramCount].value = grouptType.c_str();
	param[paramCount].encodeValue = true;
	paramCount++;

	eastl::string strHttpHeader = "";
	const char8_t *httpHeaders[3];
	
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
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] failed to get access token");
			return result;
		}
	}

	strHttpHeader += "Authorization: Bearer ";
	strHttpHeader += nucleusAccessToken.c_str();
	httpHeaders[0] = strHttpHeader.c_str();
	
	httpHeaders[1] = "X-Application-Key: fifa";

	httpHeaders[2] = "X-Api-Version: 2";
	
	eastl::string groupsURI = "";
	groupsURI = StressConfigInst.getGroupsSearchUri();
	
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] URI= " << groupsURI );
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[0]= " << httpHeaders[0]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[1]= " << httpHeaders[1]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[2]= " << httpHeaders[2]);

	result = sendGroupsHttpRequest(HttpProtocolUtil::HTTP_GET, groupsURI.c_str(),
		param,
		paramCount,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		&httpResult);
	
	if (!result)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo]: failed sending  the getGroupIDInfo request");
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::CreateInstance :] groupName = " << groupName);
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::CreateInstance :] groupType = " << grouptType);
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::CreateInstance :] http error code = " << httpResult.getHttpStatusCode());
		return false;
	}
	if (httpResult.hasError() && httpResult.getHttpStatusCode() != 302)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo]: Groups returned error");
		return false;
	}
	//Copy the groupID
	groupID = httpResult.getGroupID();
	if (groupID == "")
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo]: groupID is Null");

		return false;
	}

	return true;
}

bool GroupsProxyHandler::getGroupIDInfo(eastl::string groupName, eastl::string grouptType, eastl::string& groupID, eastl::string nucleusAccessToken)
{
	bool result = false;
	GroupsProxy* groupsProxy = BLAZE_NEW GroupsProxy(mPlayerData);
	result = groupsProxy->getGroupIDInfo(groupName, grouptType, groupID, nucleusAccessToken);
	delete groupsProxy;
	return result;
}

bool GroupsProxyHandler::getGroupIDRivalList(eastl::string groupName, eastl::string grouptType, StringList& memberList, eastl::string nucleusAccessToken)
{
	bool result = false;
	GroupsProxy* groupsProxy = BLAZE_NEW GroupsProxy(mPlayerData);
	result = groupsProxy->getGroupIDRivalList(groupName, grouptType, memberList, nucleusAccessToken);
	delete groupsProxy;
	return result;
}

bool GroupsProxy::getGroupIDRivalList(eastl::string groupName, eastl::string grouptType, StringList& memberList, eastl::string nucleusAccessToken)
{
	bool result = false;
	GroupMembersHttpResult httpResult;

	HttpParam param[5];
	int paramCount = 0;

	//[GROUPS] Get all rival groups for lead user(1000031072799) based on access time
	//https ://groups.integration.gameservices.ea.com:443/group/instance/search?name=1000031072799&pagesize=50&sortBy=access%5Ftimestamp&sortDir=desc&typeId=FIFA19RivalPS4


	param[paramCount].name = "name";
	param[paramCount].value = groupName.c_str();
	param[paramCount].encodeValue = true;
	paramCount++;

	param[paramCount].name = "pagesize";
	param[paramCount].value = "50";
	param[paramCount].encodeValue = true;
	paramCount++;

	param[paramCount].name = "sortBy";
	param[paramCount].value = "access_timestamp";
	param[paramCount].encodeValue = true;
	paramCount++;

	param[paramCount].name = "sortDir";
	param[paramCount].value = "desc";
	param[paramCount].encodeValue = true;
	paramCount++;

	param[paramCount].name = "typeId";
	param[paramCount].value = grouptType.c_str();
	param[paramCount].encodeValue = true;
	paramCount++;

	eastl::string strHttpHeader = "";
	const char8_t *httpHeaders[3];

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
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] failed to get access token");
			return result;
		}
	}

	strHttpHeader += "Authorization: Bearer ";
	strHttpHeader += nucleusAccessToken.c_str();
	httpHeaders[0] = strHttpHeader.c_str();

	httpHeaders[1] = "X-Application-Key: fifa";

	httpHeaders[2] = "X-Api-Version: 2";

	eastl::string groupsURI = "";
	groupsURI = StressConfigInst.getGroupsSearchUri();

	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[0]= " << httpHeaders[0]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[1]= " << httpHeaders[1]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[2]= " << httpHeaders[2]);

	result = sendGroupsHttpRequest(HttpProtocolUtil::HTTP_GET, groupsURI.c_str(),
		param,
		paramCount,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		&httpResult);

	if (!result)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo]: failed sending  the getGroupIDInfo request");
		return false;
	}
	if (httpResult.hasError() && httpResult.getHttpStatusCode() != 302)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo]: Groups returned error");
		return false;
	}
	//Copy the groupID
	/*groupID = httpResult.getGroupID();
	if (groupID == "")
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo]: groupID is Null");
		return false;
	}*/
	memberList.push_back("");
	return true;
}

bool GroupsProxyHandler::getGroupIDList(eastl::string groupId, StringList& memberList, eastl::string nucleusAccessToken)
{
	bool result = false;
	GroupsProxy* groupsProxy = BLAZE_NEW GroupsProxy(mPlayerData);
	result = groupsProxy->getGroupIDList(groupId, memberList, nucleusAccessToken);
	delete groupsProxy;
	return result;
}

bool GroupsProxy::getGroupIDList(eastl::string groupId, StringList& memberList, eastl::string nucleusAccessToken)
{
	bool result = false;
	GroupMembersHttpResult httpResult;
	
	//[GROUPS] Get Member List for friends group(94a6d10f - ccb4 - 4fd8 - ae4d - dcff1370bf30)
	//https: //groups.integration.gameservices.ea.com:443/group/instance/94a6d10f-ccb4-4fd8-ae4d-dcff1370bf30/members?pagesize=50

	//[GROUPS] search the overall group A members if group exists(8894262f - 09e2 - 4c19 - b4ea - dd8048b48c56)
	//https: //groups.integration.gameservices.ea.com:443/group/instance/8894262f-09e2-4c19-b4ea-dd8048b48c56/members?pagesize=50

	//[GROUPS] search the overall group B members if group exists(8894262f - 09e2 - 4c19 - b4ea - dd8048b48c56)
	//https: //groups.integration.gameservices.ea.com:443/group/instance/8894262f-09e2-4c19-b4ea-dd8048b48c56/members?pagesize=50

	HttpParam param[1];
	int paramCount = 0;

	param[paramCount].name = "pagesize";
	param[paramCount].value = "50";
	param[paramCount].encodeValue = true;
	paramCount++;

	eastl::string strHttpHeader = "";
	const char8_t *httpHeaders[3];

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
			BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] failed to get access token");
			return result;
		}
	}

	strHttpHeader += "Authorization: Bearer ";
	strHttpHeader += nucleusAccessToken.c_str();
	httpHeaders[0] = strHttpHeader.c_str();

	httpHeaders[1] = "X-Application-Key: fifa";

	httpHeaders[2] = "X-Api-Version: 2";

	eastl::string groupsURI = "";
	groupsURI = "/group/instance/";
	groupsURI += groupId;
	groupsURI += "/members";

	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] URI= " << groupsURI);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[0]= " << httpHeaders[0]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[1]= " << httpHeaders[1]);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo][" << mPlayerData->getPlayerId() << "] httpHeaders[2]= " << httpHeaders[2]);

	result = sendGroupsHttpRequest(HttpProtocolUtil::HTTP_GET, groupsURI.c_str(),
		param,
		paramCount,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		&httpResult);

	if (!result)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo]: failed sending  the getGroupIDInfo request");
		return false;
	}
	if (httpResult.hasError() && httpResult.getHttpStatusCode() != 302)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo]: Groups returned error");
		return false;
	}
	//Copy the members list
	/*groupID = httpResult.getGroupID();
	if (groupID == "")
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsProxy::getGroupIDInfo]: groupID is Null");
		return false;
	}*/
	memberList.push_back("");

	return true;
}

void GroupsHttpResult::setValue(const char8_t* fullname, const char8_t* data, size_t dataLen)
{
	//BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsHttpResult::setValue]:Received data = " << data);
	// Read group Id
	const char* token = strstr(data, "_id");
	if (token != NULL)
	{
		const char* startPos = token + strlen("_id") + 2;
		char groupId[256]		= { '\0' };
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
				groupId[resindex] = startPos[index];
				resindex++;
			}
		}
		mGroupId = groupId;
		//mGroupId = ?
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsHttpResult::setValue]:Received groupID= " << mGroupId);
	}
	else
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsHttpResult::setValue]: Data is null - data value %s ", data);
	}
	return;
} 

void GroupMembersHttpResult::setValue(const char8_t* fullname, const char8_t* data, size_t dataLen)
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupMembersHttpResult::setValue]:Received data = " << data);
	// Read Members List
	////const char* token = strstr(data, "_id");
	////if (token != NULL)
	////{
	////	const char* startPos = token + strlen("_id") + 2;
	////	char groupId[256] = { '\0' };
	////	bool foundStart = false;
	////	for (int index = 0, resindex = 0; index < (int)(strlen(data)); index++)
	////	{
	////		if (foundStart && startPos[index] == '"')
	////		{
	////			break;
	////		}
	////		if (!foundStart && startPos[index] == '"')
	////		{
	////			foundStart = true;
	////			continue;

	////		}
	////		if (foundStart)
	////		{
	////			groupId[resindex] = startPos[index];
	////			resindex++;
	////		}
	////	}
	////	mGroupId = groupId;
	////	//mGroupId = ?
	////	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[GroupsHttpResult::setValue]:Received groupID= " << mGroupId);
	////}
	////else
	////{
	////	BLAZE_ERR(BlazeRpcLog::usersessions, "[GroupsHttpResult::setValue]: Data is null - data value %s ", data);
	////}
	return;
}

}  // namespace Stress
}  // namespace Blaze


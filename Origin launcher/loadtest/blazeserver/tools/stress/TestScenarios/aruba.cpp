//  *************************************************************************************************
//
//   File:    aruba.cpp
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#include "aruba.h"

namespace Blaze {
namespace Stress {
//  *************************************************************************************************
//  Aruba Stress code 
//  *************************************************************************************************
ArubaProxy::ArubaProxy(StressPlayerInfo* playerInfo) :mPlayerData(playerInfo)
{
	mHttpArubaProxyConnMgr = NULL;
	mGovId = "";
	mTrackingtaglist = "";
	initializeConnections();
}

ArubaProxy::~ArubaProxy()
{
	if (mHttpArubaProxyConnMgr != NULL)
	{
		delete mHttpArubaProxyConnMgr;
	}
}

bool ArubaProxy::initializeConnections()
{
	if (initArubaProxyHTTPConnection())
	{
		return true;
	}
	return false;
}

bool ArubaProxy::initArubaProxyHTTPConnection()
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::initArubaProxyHTTPConnection]: Initializing the HttpConnectionManager");
	mHttpArubaProxyConnMgr = BLAZE_NEW OutboundHttpConnectionManager("fifa-2019-ps4-lt");
	if (mHttpArubaProxyConnMgr == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[ArubaProxy::initArubaProxyHTTPConnection]: Failed to Initialize the HttpConnectionManager");
		return false;
	}
	const char8_t* serviceHostname = NULL;
	bool serviceSecure = false;

	HttpProtocolUtil::getHostnameFromConfig(StressConfigInst.getArubaServerUri(), serviceHostname, serviceSecure);
	//HttpProtocolUtil::getHostnameFromConfig("https://pin-em-lt.data.ea.com", serviceHostname, serviceSecure);
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::initArubaProxyHTTPConnection]: ArubaServerUri= " << StressConfigInst.getArubaServerUri());

	InetAddress* inetAddress;
	uint16_t portNumber = 443;
	bool8_t	isSecure = true;
	inetAddress = BLAZE_NEW InetAddress(serviceHostname, portNumber);
	if (inetAddress != NULL)
	{
		mHttpArubaProxyConnMgr->initialize(*inetAddress, 1, isSecure, false);
	}
	else
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::initArubaProxyHTTPConnection]: Failed to Create InetAddress with given hostname");
		return false;
	}
	return true;
}

void ArubaProxyHandler::simulateRecoLoad()
{
	bool mSuccess = false;
	//triggerids - fifalivemsg (50%), fifahubgen4 (50%) 
	//triggerids - futhubgen4 (50%), fifahubgen4 (50%) - new values
	mSuccess = getRecoMessage();
	if (!mSuccess)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ArubaProxyHandler::simulateRecoLoad]: [" << mPlayerData->getPlayerId() << "]: getRecoMessage failed." << mSuccess);
	}
	else
	{
		mSuccess = sendTracking();
		if (!mSuccess)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ArubaProxyHandler::simulateRecoLoad]: [" << mPlayerData->getPlayerId() << "]: sendTracking failed.");
		}
	}
	reset();
}

void ArubaProxyHandler::simulateArubaLoad()
{
	bool mSuccess = false;
	//triggerids - fifalivemsg (50%), fifahubgen4 (50%) 
	//triggerids - futhubgen4 (50%), fifahubgen4 (50%) - new values
	mSuccess = getArubaMessage();
	if (!mSuccess)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ArubaProxyHandler::simulateArubaLoad]: [" << mPlayerData->getPlayerId() << "]: getArubaMessage failed.");
	}/*
	else
	{
		mSuccess = sendTracking();
		if (!mSuccess)
		{
			BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[ArubaProxyHandler::simulateArubaLoad]: [" << mPlayerData->getPlayerId() << "]: sendTracking failed.");
		}
	}*/
	reset();
}

void ArubaProxyHandler::getRecoTriggerIdsType() 
{
	RecoTriggerIdsDistributionMap recoTriggerIdsMap = StressConfigInst.getRecoTriggerIdsDistributionMap();
	RecoTriggerIdsDistributionMap::iterator it = recoTriggerIdsMap.begin();
	int count = 0;
	while (it != recoTriggerIdsMap.end())
	{
		count++;
		if (it->first == DC_TRIGGER_SKILLGAME)
		{
			getRecoTriggerIdsToStringMap()[it->first] = "3040";
		}
		else if (it->first == DC_TRIGGER_JOURNEY_OFFBOARDING)
		{
			getRecoTriggerIdsToStringMap()[it->first] = "3042";
		}
		else
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::sponsoredevents, "Converted the enum value into string");
		}
		it++;
	}
	return;
}

void ArubaProxyHandler::getArubaTriggerIdsType() 
{
	ArubaTriggerIdsDistributionMap arubaTriggerIdsMap = StressConfigInst.getArubaTriggerIdsDistributionMap();
	ArubaTriggerIdsDistributionMap::iterator it = arubaTriggerIdsMap.begin();
	int count = 0;
	while (it != arubaTriggerIdsMap.end() || count <= arubaTriggerIdsMap.size())
	{
		count++;
		if (it->first == DC_TRIGGER_LIVEMSG)
		{
			getArubaTriggerIdsToStringMap()[it->first] = "fifalivemsg";
		}
		else if (it->first == DC_TRIGGER_FIFAHUB)
		{
			getArubaTriggerIdsToStringMap()[it->first] = "fifahubgen4";
		}
		else if (it->first == DC_TRIGGER_EATV_FIFA)
		{
			getArubaTriggerIdsToStringMap()[it->first] = "fifaeatv";
		}
		else if (it->first == DC_TRIGGER_EATV_FUT)
		{
			getArubaTriggerIdsToStringMap()[it->first] = "futeatv";
		}
		else if (it->first == DC_TRIGGER_CAREERLOAD)
		{
			getArubaTriggerIdsToStringMap()[it->first] = "fifacareerload";
		}
		else if (it->first == DC_TRIGGER_FUTLOAD)
		{
			getArubaTriggerIdsToStringMap()[it->first] = "futentryload";
		}
		else if (it->first == DC_TRIGGER_FUT_TILE)
		{
			getArubaTriggerIdsToStringMap()[it->first] = "fifahubfuttile";
		}
		else if (it->first == DC_TRIGGER_FUTHUB)
		{
			getArubaTriggerIdsToStringMap()[it->first] = "futhubgen4";
		}
		else if (it->first == DC_TRIGGER_DRAFT_OFFLINE)
		{
			getArubaTriggerIdsToStringMap()[it->first] = "futdraftofflinegen4";
		}
		else if (it->first == DC_TRIGGER_DRAFT_ONLINE)
		{
			getArubaTriggerIdsToStringMap()[it->first] = "futdraftonlinegen4";
		}
		else if (it->first == DC_TRIGGER_UPS)
		{
			getArubaTriggerIdsToStringMap()[it->first] = "UPS";
		}
		else {
			BLAZE_TRACE_LOG(BlazeRpcLog::sponsoredevents, "Converted the enum value into string");
		}
		it++;
	}
	return;
}

bool ArubaProxy::getMessage(eastl::string triggerIds)
{
	bool result = false;

	ArubaHttpResult httpResult;
	eastl::string jsonData;

	/*{"clientid":"fifa18","plat":"ps4","tid":"312054","tidt":"projectId","pidm":{"personaid":"1000170900231"},"age":"0","language":"en","country":"US","triggerids":["fifahubgen4"]}*/
	//jsonData.append("{\"clientid\":\"fifa18\",\"plat\":\"ps4\",\"tid\":\"312054\",\"tidt\":\"projectId\",\"pidm\":{\"personaid\":\"1000170900231\"},\"age\":\"0\",\"language\":\"en\",\"country\":\"US\",\"triggerids\":[\"fifahubgen4\"]}");
	//fifa18_ltc
	//fifa18_lta
	//fifa19_test
	jsonData.append("{\"clientid\":\"fifa21_lta\",\"plat\":\"");
	if (StressConfigInst.getPlatform() == PLATFORM_XONE)
	{
		jsonData.append("xbox_one");
	}
	else if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		jsonData.append("ps4");
	}
	// PS4 - projectId - 312054
	jsonData.append("\",\"tid\":\"");
	if (StressConfigInst.getPlatform() == PLATFORM_XONE)
	{
		//jsonData.append("312093");
		jsonData.append(StressConfigInst.getProjectId());
	}
	else if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		//jsonData.append("312054");
		jsonData.append(StressConfigInst.getProjectId());
	}
	jsonData.append("\",\"tidt\":\"projectId\",\"preferredpidtype\":\"personaId\",\"pidm\":{\"personaid\":\"");
	char8_t	personaID[256];
	memset(personaID, '\0', sizeof(personaID));
	blaze_snzprintf(personaID, sizeof(personaID), "%" PRId64, mPlayerData->getPlayerId());
	jsonData.append(personaID);
	jsonData.append("\",\"nucleus\":\"");
	char8_t	nucleusID[256];
	UserSessionLoginInfo notification;
	memset(nucleusID, '\0', sizeof(nucleusID));
	blaze_snzprintf(nucleusID, sizeof(nucleusID), "%" PRId64, mPlayerData->getAccountId());
	jsonData.append(nucleusID);
	//futhubgen4
	jsonData.append("\"},\"age\":\"0\",\"language\":\"en\",\"country\":\"US\",\"triggerids\":[\"");
	jsonData.append(triggerIds);
	
	jsonData.append("\"],");
	//jsonData.append(getArubaTriggerIdsToStringMap()[getArubaTriggerIdsType()]);
	if (StressConfigInst.getArubaEnabled())
	{
		jsonData.append("\"gamestate\":{\"countryCode\":\"us\",\"isFrontlineSubscrib\":\"false\",\"isEaAccessSubscriber\":\"false\"}}");
	}
	if (StressConfigInst.getRecoEnabled())
	{
		jsonData.append("\"num_items\":\"10\"}");
	}
	//eastl::string gameSessionData;
	//jsonData.append(jsonTriggerIdDifferentRequestData(gameSessionData.c_str()));
	//jsonData.append(gameSessionData);
	//jsonData.append("\"}");
 	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::getMessage] " << " [" << mPlayerData->getPlayerId() << "] JSONRPC request[" << jsonData.c_str() << "]");

	OutboundHttpConnection::ContentPartList contentList;
	OutboundHttpConnection::ContentPart content;
	content.mName = "content";
	content.mContentType = JSON_CONTENTTYPE;
	content.mContent = reinterpret_cast<const char8_t*>(jsonData.c_str());
	content.mContentLength = jsonData.size();
	contentList.push_back(&content);

	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::getMessage] " << " [" << mPlayerData->getPlayerId() << "] getMessage URI[" << StressConfigInst.getArubaGetMessageUri() << "]");

	result = sendArubaHttpRequest(HttpProtocolUtil::HTTP_POST, StressConfigInst.getArubaGetMessageUri(), NULL, 0, httpResult, contentList);

	if (false == result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::getMessage:result]:" << mPlayerData->getPlayerId() << " Aruba returned error for getMessage.");
		return false;
	}

	if (httpResult.hasError())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::getMessage:hasError]:" << mPlayerData->getPlayerId() << " Aruba returned error for getMessage");
		return false;
	}

	char* govIdValue = httpResult.getGovid();
	if (govIdValue == NULL)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::getMessage]: " << mPlayerData->getPlayerId() << " govIdValue is null");
		return false;
	}
	getGovId().sprintf(govIdValue);
	BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::getMessage]::[" << mPlayerData->getPlayerId() << "] successful, GovId = " << getGovId().c_str());

	char* trackingtaglistValue = httpResult.getTrackingtaglist();
	if (trackingtaglistValue == NULL)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::getMessage]: " << mPlayerData->getPlayerId() << " trackingtaglistValue is null");
		return false;
	}
	getTrackingtaglist().sprintf(trackingtaglistValue);
	BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::getMessage]::[" << mPlayerData->getPlayerId() << "] successful, Trackingtaglist = " << getTrackingtaglist().c_str());
	return true;
}

bool ArubaProxy::sendTracking(eastl::string govID, eastl::string trackingList)
{
	bool result = false;
	ArubaHttpResult httpResult;
	eastl::string jsonData;

	//{"govid":"7243276220036685486","clientid":"fifa18","plat":"ps4","tid":"312054","tidt":"projectId","pidm":{"personaid":"1000170900231"},"trackingtaglist":["M187dfTf43eeD187bfNintegration"]}
	//jsonData.append("{\"govid\":\"7243276220036685486\",\"clientid\":\"fifa18\",\"plat\":\"ps4\",\"tid\":\"312054\",\"tidt\":\"projectId\",\"pidm\":{\"personaid\":\"1000170900231\"},\"trackingtaglist\":[\"M187dfTf43eeD187bfNintegration\"]}");

	jsonData.append("{\"govid\":\"");
	jsonData.append(govID.c_str());
	jsonData.append("\",\"clientid\":\"fifa20_lta\",\"plat\":\"");
	if (StressConfigInst.getPlatform() == PLATFORM_XONE)
	{
		jsonData.append("xbox_one");
	}
	else if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		jsonData.append("ps4");
	}
	// PS4 - projectId - 312054
	jsonData.append("\",\"tid\":\"");
	if (StressConfigInst.getPlatform() == PLATFORM_XONE)
	{
		//jsonData.append("312093");
		jsonData.append(StressConfigInst.getProjectId());
	}
	else if (StressConfigInst.getPlatform() == PLATFORM_PS4)
	{
		//jsonData.append("312054");
		jsonData.append(StressConfigInst.getProjectId());
	}
	jsonData.append("\",\"tidt\":\"projectId\",\"pidm\":{\"personaid\":\"");
	char8_t	personaID[256];
	memset(personaID, '\0', sizeof(personaID));
	blaze_snzprintf(personaID, sizeof(personaID), "%" PRId64, mPlayerData->getPlayerId());
	jsonData.append(personaID);
	jsonData.append("\"},\"trackingtaglist\":[\"");
	jsonData.append(trackingList.c_str());
	jsonData.append("\"]}");

	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::sendTracking] " << " [" << mPlayerData->getPlayerId() << "] JSONRPC request[" << jsonData.c_str() << "]");

	OutboundHttpConnection::ContentPartList contentList;
	OutboundHttpConnection::ContentPart content;
	content.mName = "content";
	content.mContentType = JSON_CONTENTTYPE;
	content.mContent = reinterpret_cast<const char8_t*>(jsonData.c_str());
	content.mContentLength = jsonData.size();
	contentList.push_back(&content);

	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::sendTracking] " << " [" << mPlayerData->getPlayerId() << "] sendTracking URI[" << StressConfigInst.getArubaTrackingUri() << "]");

	result = sendArubaHttpRequest(HttpProtocolUtil::HTTP_POST, StressConfigInst.getArubaTrackingUri(), NULL, 0, httpResult, contentList);

	if (false == result)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::sendTracking:result]:" << mPlayerData->getPlayerId() << " Aruba returned error for sendTracking.");
		return false;
	}

	if (httpResult.hasError())
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::SendTracking:hasError]:" << mPlayerData->getPlayerId() << " Aruba returned error for sendTracking");
		return false;
	}
	return true;
}


bool ArubaProxy::sendArubaHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult& result, OutboundHttpConnection::ContentPartList& contentList)
{
	BlazeRpcError err = ERR_SYSTEM;
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::sendArubaHttpRequest][" << mPlayerData->getPlayerId() << "] method= " << method << " URI= " << URI << " httpHeaders= " << httpHeaders);

	err = mHttpArubaProxyConnMgr->sendRequest(method, URI, NULL, 0, httpHeaders, headerCount, &result, &contentList);

	if (ERR_OK != err)
	{
		BLAZE_ERR_LOG(BlazeRpcLog::usersessions, "[ArubaProxy::sendArubaHttpRequest][" << mPlayerData->getPlayerId() << "] failed to send http request");
		return false;
	}
	return true;
}
// If Reco is enabled 
bool  ArubaProxyHandler::getRecoMessage()
{
	bool result = false;
	getRecoTriggerIdsType();
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "Starting Reco shared services!!")
	RecoTriggerIdsToStringMap::iterator itr = getRecoTriggerIdsToStringMap().begin();

	ArubaProxy* arubaProxy = BLAZE_NEW ArubaProxy(mPlayerData);
	
	for (itr = getRecoTriggerIdsToStringMap().begin(); itr != getRecoTriggerIdsToStringMap().end(); itr++)
	{
		result = arubaProxy->getMessage(itr->second);

		//check for all messages need to call sendtracking
		if (result)
		{
			setGovId(arubaProxy->getGovId());
			setTrackingtaglist(arubaProxy->getTrackingtaglist());
		}
	/*	else
		{
			break;
		}*/
	}
	
	delete arubaProxy;
	return result;
}

// if Aruba Enabled
bool  ArubaProxyHandler::getArubaMessage()
{
	bool result = false;
	getArubaTriggerIdsType();
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "Starting Aruba shared services!!")
	ArubaTriggerIdsToStringMap::iterator itr;
	
	ArubaProxy* arubaProxy = BLAZE_NEW ArubaProxy(mPlayerData);

	for (itr = getArubaTriggerIdsToStringMap().begin(); itr != getArubaTriggerIdsToStringMap().end(); itr++)
	{
		
		result = arubaProxy->getMessage(itr->second);
		// check for all messages need to call sendtracking
		if (result)
		{
			setGovId(arubaProxy->getGovId());
			setTrackingtaglist(arubaProxy->getTrackingtaglist());
		}
	}

	delete arubaProxy;
	return result;
}

bool  ArubaProxyHandler::sendTracking()
{
	ArubaProxy* arubaProxy = BLAZE_NEW ArubaProxy(mPlayerData);
	bool result = arubaProxy->sendTracking(getGovId(), getTrackingtaglist());
	delete arubaProxy;
	return result;
}

void ArubaHttpResult::setValue(const char8_t* fullname, const char8_t* data, size_t dataLen)
{
	BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[ArubaHttpResult::setValue]:Received data = " << data);
	// Read govid
	const char* token = strstr(data, "govid");
	if (token != NULL)
	{
		const char* startPos = token + strlen("govid") + 2;
		char idValue[256] = { '\0' };
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
				idValue[resindex] = startPos[index];
				resindex++;
			}
		}
		if (blaze_strcmp(idValue, "null") != 0)
		{
			blaze_strnzcpy(govid, idValue, sizeof(govid));
		}
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[ArubaHttpResult::setValue]:Received govid [" << govid << "] and data = " << data);
	}
	else
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[ArubaHttpResult::setValue]: govid is null, data = %s ", data);
	}

	// Read trackingtag
	const char* tokenId = strstr(data, "trackingtag");
	if (tokenId != NULL)
	{
		const char* startPos = tokenId + strlen("trackingtag") + 1;
		char idValue[256] = { '\0' };
		bool foundStart = false;
		for (int index = 0, resindex = 0; index < (int)(strlen(data)); index++)
		{
			if (foundStart && startPos[index] == ',')
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
				if (startPos[index] == '"') { continue; }
				if (startPos[index] == '}') { continue; }
				idValue[resindex] = startPos[index];
				resindex++;
			}
		}
		if (blaze_strcmp(idValue, "null") != 0)
		{
			blaze_strnzcpy(trackingtaglist, idValue, sizeof(trackingtaglist));
		}
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[ArubaHttpResult::setValue]:Received trackingtaglist [" << trackingtaglist << "] and data = " << data);
	}
	else
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[ArubaHttpResult::setValue]: trackingtag is null, data = %s ", data);
	}
}

//  *************************************************************************************************
//  UPS Stress code 
//  *************************************************************************************************

}  // namespace Stress
}  // namespace Blaze


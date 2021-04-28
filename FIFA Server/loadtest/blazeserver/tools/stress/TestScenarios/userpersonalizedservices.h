//  *************************************************************************************************
//
//   File:    userpersonalizedservices.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef USERPERSONALIZEDSERVICES_H
#define USERPERSONALIZEDSERVICES_H

#include "playerinstance.h"
#include "nucleusproxy.h"
#include "playermodule.h"


namespace Blaze {
namespace Stress {

//enum UPSTriggerIds
//{
//	DC_TRIGGER_LIVEMSG				= 100,
//	DC_TRIGGER_FIFAHUB				= 101,
//	DC_TRIGGER_EATV_FIFA			= 102,
//	DC_TRIGGER_EATV_FUT				= 103,
//	DC_TRIGGER_CAREERLOAD			= 104,
//	DC_TRIGGER_FUTLOAD				= 105,
//	DC_TRIGGER_FUT_TILE				= 106,
//	DC_TRIGGER_FUTHUB				= 111,
//	DC_TRIGGER_DRAFT_OFFLINE		= 112,
//	DC_TRIGGER_DRAFT_ONLINE			= 113,
//	DC_TRIGGER_SKILLGAME			= 3040,
//	DC_TRIGGER_JOURNEY_OFFBOARDING	= 3042	
//};

//typedef eastl::map<UPSTriggerIds, uint8_t> UPSTriggerIdsDistributionMap;
//typedef eastl::map<UPSTriggerIds, eastl::string> UPSTriggerIdsToStringMap;
//*************************************************************************************************
//UPS Stress code 
//*************************************************************************************************
class UPSProxy
{
	NON_COPYABLE(UPSProxy);

public:
	UPSProxy(StressPlayerInfo* playerInfo);
	virtual				~UPSProxy();
	bool				getMessage(const char8_t * authorizationAccessToken);
	const char8_t *		getUPSGetMessageUriPlayerId();
	
	OutboundHttpConnectionManager*		mHttpUPSProxyConnMgr;

protected:
	bool sendUPSHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult& result, OutboundHttpConnection::ContentPartList& contentList);

private:
	bool				initUPSProxyHTTPConnection();
	StressPlayerInfo*	mPlayerData;

	// UPS related config value
	eastl::string       mUPSServerUri;
	eastl::string       mUPSGetMessageUri;
	eastl::string       mUPSTrackingUri;
	bool                mUPSEnabled;
	eastl::string		mUPSGetMessageUriPlayerId;
};

class UPSHttpResult : public OutboundHttpResult
{

private:
	BlazeRpcError	mErr;
	char			UPSGovId[256];
	char            upstrackingtaglist[256];

public:
	UPSHttpResult() : mErr(ERR_OK)
	{
		memset(UPSGovId, '\0', sizeof(UPSGovId));
		memset(upstrackingtaglist, '\0', sizeof(upstrackingtaglist));
	}
	void setHttpError(BlazeRpcError err = ERR_SYSTEM)
	{
		mErr = err;
	}
	bool hasError()
	{
		return (mErr != ERR_OK) ? true : false;
	}

	void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes,
		size_t attributeCount)
	{
		//Do nothing here
	}

	void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen);

	char* getUPSGovId()
	{
		return (strlen(UPSGovId) == 0) ? NULL : UPSGovId;
	}

	char* getupstrackingtaglist()
	{
		if (strlen(upstrackingtaglist) == 0)
		{
			return NULL;
		}
		return upstrackingtaglist;
	}

	bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attribuets,
		size_t attrCount)
	{
		return false;
	}

};

class UPSSimpleHttpResult : public OutboundHttpResult
{

private:
	BlazeRpcError	mErr;
	bool            mPrintResponse;
public:
	UPSSimpleHttpResult(bool printResponse = false)
	{
		mPrintResponse = printResponse;
	}

	void setHttpError(BlazeRpcError err = ERR_SYSTEM)
	{
		mErr = err;
	}
	bool hasError()
	{
		return (mErr != ERR_OK) ? false : true;
	}

	void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes,
		size_t attributeCount)
	{
		//Do nothing here
	}

	void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen)
	{
		if (mPrintResponse)
		{
			BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[UPSSimpleHttpResult::setValue]: data = " << data);
		}
		// do nothing here
	}

	bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attribuets,
		size_t attrCount)
	{
		return false;
	}

};

class UPSProxyHandler
{
	NON_COPYABLE(UPSProxyHandler);

public:
	UPSProxyHandler(StressPlayerInfo* playerInfo) : mPlayerData(playerInfo) {};
	virtual				~UPSProxyHandler() { };
	bool				simulateUPSLoad();
	bool				getMyAccessToken();

private:	
	StressPlayerInfo*	mPlayerData;
	eastl::string		mAccessToken;
	eastl::string		mUPSAccessToken;
	eastl::string		mAuthCode;
};


}  // namespace Stress
}  // namespace Blaze

#endif  //   UPS_H

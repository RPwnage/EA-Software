//  *************************************************************************************************
//
//   File:    aruba.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef ARUBA_H
#define ARUBA_H

#include "playerinstance.h"
//  #include "player.h"
#include "playermodule.h"


namespace Blaze {
namespace Stress {

//	enum ArubaTriggerIds
//	{
//		DC_TRIGGER_LIVEMSG = 100,
//		DC_TRIGGER_FIFAHUB = 101,
//		DC_TRIGGER_EATV_FIFA = 102,
//		DC_TRIGGER_EATV_FUT = 103,
//		DC_TRIGGER_CAREERLOAD = 104,
//		DC_TRIGGER_FUTLOAD = 105,
//		DC_TRIGGER_FUT_TILE = 106,
//		DC_TRIGGER_FUTHUB = 111,
//		DC_TRIGGER_DRAFT_OFFLINE = 112,
//		DC_TRIGGER_DRAFT_ONLINE = 113,
//		DC_TRIGGER_SKILLGAME = 3040,
//		DC_TRIGGER_JOURNEY_OFFBOARDING = 3042
//};


//*************************************************************************************************
//ARUBA Stress code 
//*************************************************************************************************
class ArubaProxy
{
	NON_COPYABLE(ArubaProxy);

public:
	ArubaProxy(StressPlayerInfo* playerInfo);
	virtual			~ArubaProxy();
	bool			initializeConnections();
	bool			getMessage(eastl::string triggerIds);

	//eastl::map<ArubaTriggerIds, eastl::string> StringConversionTriggerId;
	bool			sendTracking(eastl::string govID, eastl::string trackingList);
	eastl::string&	getGovId() { return mGovId; }
	eastl::string&	getTrackingtaglist() { return mTrackingtaglist; }

protected:
	bool sendArubaHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult& result, OutboundHttpConnection::ContentPartList& contentList);

private:
	bool			initArubaProxyHTTPConnection();

	OutboundHttpConnectionManager*	mHttpArubaProxyConnMgr;
	StressPlayerInfo*				mPlayerData;

	eastl::string					mGovId;
	eastl::string					mTrackingtaglist;
};

class ArubaHttpResult : public OutboundHttpResult
{

private:
	BlazeRpcError	mErr;
	char			govid[256];
	char            trackingtaglist[256];

public:
	ArubaHttpResult() : mErr(ERR_OK)
	{
		memset(govid, '\0', sizeof(govid));
		memset(trackingtaglist, '\0', sizeof(trackingtaglist));
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

	char* getGovid()
	{
		return (strlen(govid) == 0) ? NULL : govid;
	}

	char* getTrackingtaglist()
	{
		if (strlen(trackingtaglist) == 0)
		{
			return NULL;
		}
		return trackingtaglist;
	}

	bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attribuets,
		size_t attrCount)
	{
		return false;
	}

};

class SimpleHttpResult : public OutboundHttpResult
{

private:
	BlazeRpcError	mErr;
	bool            mPrintResponse;
public:
	SimpleHttpResult(bool printResponse = false)
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
			BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[SimpleHttpResult::setValue]: data = " << data);
		}
		// do nothing here
	}

	bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attribuets,
		size_t attrCount)
	{
		return false;
	}

};

class ArubaProxyHandler
{
	NON_COPYABLE(ArubaProxyHandler);

public:
	ArubaProxyHandler(StressPlayerInfo* playerInfo) : mPlayerData(playerInfo) { mGovId = ""; mTrackingtaglist = ""; };
	virtual			~ArubaProxyHandler() { };
	bool			getArubaMessage();
	bool			getRecoMessage();
	bool			sendTracking();
	void			reset() { mGovId = ""; mTrackingtaglist = ""; }
	eastl::string&	getGovId() { return mGovId; }
	eastl::string&	getTrackingtaglist() { return mTrackingtaglist; }
	void			setGovId(eastl::string govID) { mGovId = govID; }
	void			setTrackingtaglist(eastl::string trackingList) { mTrackingtaglist = trackingList; }
	void			getArubaTriggerIdsType();
	void			getRecoTriggerIdsType();
	void			simulateArubaLoad();
	void			simulateRecoLoad();
	ArubaTriggerIdsToStringMap&		getArubaTriggerIdsToStringMap() {
		return arubaTriggerIdsToStringMap;
	}
	RecoTriggerIdsToStringMap&		getRecoTriggerIdsToStringMap() {
		return recoTriggerIdsToStringMap;
	}

private:
	StressPlayerInfo*			mPlayerData;
	eastl::string				mGovId;
	eastl::string				mTrackingtaglist;
	ArubaTriggerIdsToStringMap	arubaTriggerIdsToStringMap;
	RecoTriggerIdsToStringMap	recoTriggerIdsToStringMap;
};


}  // namespace Stress
}  // namespace Blaze

#endif  //   ARUBA_H

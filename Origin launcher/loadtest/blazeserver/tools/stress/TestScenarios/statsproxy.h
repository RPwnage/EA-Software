//  *************************************************************************************************
//
//   File:    statsproxy.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef STATSPROXY_H
#define STATSPROXY_H

#include "playerinstance.h"
#include "nucleusproxy.h"
#include "playermodule.h"


namespace Blaze {
namespace Stress {

//*************************************************************************************************
//Stats Stress code 
//*************************************************************************************************
class StatsProxy
{
	NON_COPYABLE(StatsProxy);

public:
	StatsProxy(StressPlayerInfo* playerInfo);
	virtual				~StatsProxy();
	bool				viewEntityStats(eastl::string viewName, eastl::string entityName, eastl::string nucleusAccessToken);
	bool				trackPlayerHealth(eastl::string entityName, eastl::string nucleusAccessToken);

protected:
	bool sendStatsHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult* result);

private:
	bool				initStatsProxyHTTPConnection();

	OutboundHttpConnectionManager*	mHttpStatsProxyConnMgr;
	StressPlayerInfo*				mPlayerData;
};

class StatsProxyHandler
{
	NON_COPYABLE(StatsProxyHandler);

public:
	StatsProxyHandler(StressPlayerInfo* playerInfo) : mPlayerData(playerInfo) 
	{};
	virtual			~StatsProxyHandler() {};
	bool			viewEntityStats(eastl::string viewName, eastl::string entityName, eastl::string nucleusAccessToken);
	bool			trackPlayerHealth(eastl::string entityName, eastl::string nucleusAccessToken);


private:
	StressPlayerInfo*	mPlayerData;
};

class StatsHttpResult : public OutboundHttpResult
{

private:
	BlazeRpcError	mErr;
	uint32_t		mOptIn;
	EadpStatsMap	mEadpStatsMap;
	char			mCategoryId[256];
	/*char			govid[256];*/

public:
	StatsHttpResult() : mErr(ERR_OK)
	{
		memset(mCategoryId, '\0', sizeof(mCategoryId));
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

	uint32_t getOptIn()
	{
		return mOptIn;
	}

	EadpStatsMap getEadpStatsMap()
	{
		return mEadpStatsMap;
	}

	const char* getStatsValue(const char* data, char statsName[]);
	/*char* getGovid()
	{
	return (strlen(govid) == 0) ? NULL : govid;
	}*/

	bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attribuets,
		size_t attrCount)
	{
		return false;
	}

};

}  // namespace Stress
}  // namespace Blaze

#endif  //   Stats_H

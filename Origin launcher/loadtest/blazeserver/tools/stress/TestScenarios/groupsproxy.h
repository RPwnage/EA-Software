//  *************************************************************************************************
//
//   File:    groupsproxy.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef GROUPSPROXY_H
#define GROUPSPROXY_H

#include "nucleusproxy.h"
#include "reference.h"

namespace Blaze {
namespace Stress {

//*************************************************************************************************
//Groups Stress code 
//*************************************************************************************************
class GroupsProxy
{
	NON_COPYABLE(GroupsProxy);

public:
	GroupsProxy(StressPlayerInfo* playerInfo);
	virtual			~GroupsProxy();
	bool			createInstance(eastl::string creatorUserId, eastl::string groupName, eastl::string groupShortName, eastl::string groupTypeId, eastl::string nucleusAccessToken);
	bool			getGroupIDInfo(eastl::string groupName, eastl::string grouptType, eastl::string& groupID, eastl::string nucleusAccessToken = "");
	bool			getGroupIDRivalList(eastl::string groupName, eastl::string grouptType, StringList& memberList, eastl::string nucleusAccessToken = "");
	bool			getGroupIDList(eastl::string groupId, StringList& memberList, eastl::string nucleusAccessToken = "");

protected:
	bool			sendGroupsHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult* result);
	bool			sendGroupsHttpPostRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult& result, const char8_t* httpHeader_ContentType, const char8_t* contentBody, uint32_t contentLength);

private:
	bool							initGroupsProxyHTTPConnection();

	OutboundHttpConnectionManager*	mHttpGroupsProxyConnMgr;
	StressPlayerInfo*				mPlayerData;
};

class GroupsProxyHandler
{
	NON_COPYABLE(GroupsProxyHandler);

public:
	GroupsProxyHandler(StressPlayerInfo* playerInfo) : mPlayerData(playerInfo) 
	{};
	virtual		~GroupsProxyHandler() { };
	bool		createInstance(eastl::string creatorUserId, eastl::string groupName, eastl::string groupShortName, eastl::string groupTypeId, eastl::string nucleusAccessToken);
	bool		getGroupIDInfo(eastl::string groupName, eastl::string grouptType, eastl::string& groupID, eastl::string nucleusAccessToken="");
	bool		getGroupIDRivalList(eastl::string groupName, eastl::string grouptType, StringList& memberList, eastl::string nucleusAccessToken = "");
	bool		getGroupIDList(eastl::string groupName, StringList& memberList, eastl::string nucleusAccessToken = "");

private:
	StressPlayerInfo*	mPlayerData;
};

class GroupsHttpResult : public OutboundHttpResult
{

private:
	BlazeRpcError	mErr;
	eastl::string   mGroupId;

public:
	GroupsHttpResult() : mErr(ERR_OK)
	{
		mGroupId = "";
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

	eastl::string getGroupID()
	{
		return mGroupId;
	}

	bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attribuets,
		size_t attrCount)
	{
		return false;
	}
};

class GroupMembersHttpResult : public OutboundHttpResult
{

private:
	BlazeRpcError	mErr;
	//String list - TBD

public:
	GroupMembersHttpResult() : mErr(ERR_OK)
	{	
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


	bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attribuets,
		size_t attrCount)
	{
		return false;
	}
};



}  // namespace Stress
}  // namespace Blaze

#endif  //   Groups_H

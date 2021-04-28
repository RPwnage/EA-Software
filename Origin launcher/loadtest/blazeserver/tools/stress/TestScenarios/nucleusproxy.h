//  *************************************************************************************************
//
//   File:    NucleusProxy.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef NUCLEUSPROXY_H
#define NUCLEUSPROXY_H

#include "playerinstance.h"
#include "playermodule.h"

namespace Blaze {
namespace Stress {

//*************************************************************************************************
//Nucleus Stress code 
//*************************************************************************************************
class NucleusProxy
{
	NON_COPYABLE(NucleusProxy);

public:
	NucleusProxy(StressPlayerInfo* playerInfo);
	virtual ~NucleusProxy();
	bool getPlayerAuthCode(eastl::string& authCode);
	bool getPlayerAccessToken(eastl::string mAuthCode, eastl::string& mAccessToken);
	bool getUPSPlayerAccessToken(eastl::string mAuthCode, eastl::string& accessToken);
	bool getGuestAuthCode(eastl::string& mGuestAuthCode);
	bool getGuestAccessToken(eastl::string& mGuestAuthCode, eastl::string& mGuestAccessToken);
	bool getGuestPersonaId(eastl::string& mGuestAccessToken, eastl::string& mGuestPersonaId);
	//bool NucleusProxy::initializeNucleus(const ConfigMap& config, const CmdLineArguments& arguments);

protected:
	bool sendNucleusHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult* result);
	bool sendNucleusHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult& result, OutboundHttpConnection::ContentPartList& contentList);
	
private:
	bool							initNucleusProxyHTTPConnection();
	OutboundHttpConnectionManager*	mHttpNucleusProxyConnMgr;
	StressPlayerInfo*				mPlayerData;
	OutboundHttpConnectionManager*	mHttpNucleusConnMgr;
	char8_t							mAuthenticationUri[256];
	eastl::string					mAccessToken;
	eastl::string					mUPSAccessToken;
	
};

class NucleusHttpResult : public OutboundHttpResult
{

private:
	BlazeRpcError										mErr;
	char												mAuthCode[256];
	char												mGuestAuthCode[256];
	char												mAccessToken[256];
	char												mGuestAccessToken[256];
	char												mCode[256];
	char												mGuestPersonaId[256];

public:
	NucleusHttpResult() : mErr(ERR_OK)
	{
		memset(mAuthCode, '\0', 256);
		memset(mAccessToken, '\0', 256);
		memset(mGuestAccessToken, '\0', 256);
		memset(mGuestAuthCode, '\0', 256);
		memset(mCode, '\0', 256);
		memset(mGuestPersonaId, '\0', 256);
	}
	void setHttpError(BlazeRpcError err = ERR_SYSTEM)
	{
		mErr = err;
	}
	bool hasError()
	{
		if (mErr != ERR_OK)
			return true;
		return false;
	}

	void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes,
		size_t attributeCount)
	{
		//Do nothing here
	}

	void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[NucleusHttpResult::setValue]:Received data = " << data);
		const char* token = strstr(data, "access_token");
		const char* personaId = strstr(data, "persona_id");
		if (token != NULL)
		{
			const char* startPos = token + strlen("access_token") + 2;
			char accessToken[256] = { '\0' };
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
					accessToken[resindex] = startPos[index];
					resindex++;
				}
			}
			strncpy(mAccessToken, accessToken, strlen(accessToken) + 1);
			return;
		} // if token is NULL is token careoff
		else if (personaId != NULL)
		{
			BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[NucleusHttpResult::setValue]:Pratik = " << personaId);
			char persona_idToken[256] = { '\0' };
			int startIndex = 0;
			bool flag = false;
			for (int index = 0; index < (int)(strlen(personaId)); index++)
			{

				if (!flag && personaId[index] == ':')
				{
					flag = true;
					continue;
				}
				if (flag)
				{
					persona_idToken[startIndex] = personaId[index];
				}
				else
				{
					continue;
				}
				persona_idToken[startIndex++] = personaId[index];
			}
			strncpy(mGuestPersonaId, persona_idToken, strlen(persona_idToken) + 1);
			BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusHttpResult::setValue]: persona_id - data value %s ", mGuestPersonaId);
			return;
		}
		else
		{
			BLAZE_ERR(BlazeRpcLog::usersessions, "[NucleusHttpResult::setValue]: token is null - data value %s ", data);
		}
	}

	bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attribuets,
		size_t attrCount)
	{
		return false;
	}

	char* getAccessToken()
	{
		if (strlen(mAccessToken) == 0)
		{
			return NULL;
		}

		char* accesstoken = new char[strlen(mAccessToken) + 1];
		memset(accesstoken, '\0', strlen(mAccessToken) + 1);
		strcpy(accesstoken, mAccessToken);

		return accesstoken;
	}
	char* getCode(char* header)
	{
		char* code = NULL;
		HttpHeaderMap::const_iterator it = getHeaderMap().find(header);
		if (it != getHeaderMap().end())
		{
			BLAZE_INFO(BlazeRpcLog::usersessions, "[StressHttpResult::getCode]: %s : %s", it->first.c_str(), it->second.c_str());
			const char* address = it->second.c_str();
			int index = 0;
			for (int i = 0; i<(int)(strlen(address)); i++)
			{
				if (address[i] == '=')
				{
					index = i + 1;
					break;
				}
			}
			code = new char[strlen(address + index) + 1];
			memset(code, '\0', strlen(address + index) + 1);
			strcpy(code, address + index);
			return code;
		}
		else
		{
			return NULL;
		}
	}
};

//class NucleusProxyHandler
//{
//	NON_COPYABLE(NucleusProxyHandler);
//
//public:
//	NucleusProxyHandler(StressPlayerInfo* playerInfo) : mPlayerData(playerInfo) {};
//	virtual ~NucleusProxyHandler() { };
//
//private:
//	StressPlayerInfo*	mPlayerData;
//	NucleusProxy*       mNucleusProxy;
//};


}  // namespace Stress
}  // namespace Blaze

#endif  //   Nucleus_H

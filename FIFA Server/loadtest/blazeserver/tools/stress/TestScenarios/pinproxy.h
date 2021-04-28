//  *************************************************************************************************
//
//   File:    pinproxy.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef PINPROXY_H
#define PINPROXY_H

#include "playerinstance.h"
//  #include "player.h"
#include "playermodule.h"


namespace Blaze {
	namespace Stress {

	class PINProxy
	{
		NON_COPYABLE(PINProxy);

	public:
		PINProxy(StressPlayerInfo* playerInfo);
		virtual			~PINProxy();
		bool			initializeConnections();
		bool			getMessage(eastl::string pinEvent);

		//eastl::map<PINTriggerIds, eastl::string> StringConversionTriggerId;
		bool			sendTracking(eastl::string govID, eastl::string trackingList);
		eastl::string&	getGovId() { return mGovId; }
		eastl::string&	getTrackingtaglist() { return mTrackingtaglist; }

	protected:
		bool sendPINHttpRequest(HttpProtocolUtil::HttpMethod method, const char8_t* URI, const char8_t* httpHeaders[], uint32_t headerCount, OutboundHttpResult& result, OutboundHttpConnection::ContentPartList& contentList);

	private:
		bool			initPINProxyHTTPConnection();

		OutboundHttpConnectionManager*	mHttpPINProxyConnMgr;
		StressPlayerInfo*				mPlayerData;

		eastl::string					mGovId;
		eastl::string					mTrackingtaglist;
	};

	class PINHttpResult : public OutboundHttpResult
	{

	private:
		BlazeRpcError	mErr;
		char			govid[256];
		char            trackingtaglist[256];

	public:
		PINHttpResult() : mErr(ERR_OK)
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

	//class SimplHttpResult : public OutboundHttpResult
	//{

	//private:
	//	BlazeRpcError	mErr;
	//	bool            mPrintResponse;
	//public:
	//	SimpleHttpResult(bool printResponse = false)
	//	{
	//		mPrintResponse = printResponse;
	//	}

	//	void setHttpError(BlazeRpcError err = ERR_SYSTEM)
	//	{
	//		mErr = err;
	//	}
	//	bool hasError()
	//	{
	//		return (mErr != ERR_OK) ? false : true;
	//	}

	//	void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes,
	//		size_t attributeCount)
	//	{
	//		//Do nothing here
	//	}

	//	void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen)
	//	{
	//		if (mPrintResponse)
	//		{
	//			BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[SimpleHttpResult::setValue]: data = " << data);
	//		}
	//		// do nothing here
	//	}

	//	bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attribuets,
	//		size_t attrCount)
	//	{
	//		return false;
	//	}

	//};

	class PINProxyHandler
	{
		NON_COPYABLE(PINProxyHandler);

	public:
		PINProxyHandler(StressPlayerInfo* playerInfo) : mPlayerData(playerInfo) { mGovId = ""; mTrackingtaglist = ""; };
		virtual			~PINProxyHandler() { };
		bool			getPINMessage();
		bool			sendTracking();
		void			reset() { mGovId = ""; mTrackingtaglist = ""; }
		eastl::string&	getGovId() { return mGovId; }
		eastl::string&	getTrackingtaglist() { return mTrackingtaglist; }
		void			setGovId(eastl::string govID) { mGovId = govID; }
		void			setTrackingtaglist(eastl::string trackingList) { mTrackingtaglist = trackingList; }
		void			simulatePINLoad();

	private:
		StressPlayerInfo*			mPlayerData;
		eastl::string				mGovId;
		eastl::string				mTrackingtaglist;
		RecoTriggerIdsToStringMap	recoTriggerIdsToStringMap;
	};


	}  // namespace Stress
}  // namespace Blaze

#endif  //   ARUBA_H

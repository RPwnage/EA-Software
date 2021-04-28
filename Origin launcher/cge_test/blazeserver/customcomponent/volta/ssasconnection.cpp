/*************************************************************************************************/
/*!
    \file   SSASConnection.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/xmlbuffer.h"

#include "ssasconnection.h"

namespace Blaze
{
	namespace Volta
	{
		class SSASConnectionResponse
			: public OutboundHttpResult
		{
		public:
			SSASConnectionResponse() {}
			virtual ~SSASConnectionResponse() {}

			virtual void setHttpError(BlazeRpcError err = ERR_SYSTEM) {}
			virtual bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) { return false; }

			virtual void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) {}
			virtual void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) {}
		};
		/*** Public Methods ******************************************************************************/
		//=======================================================================================
		SSASConnection::SSASConnection(const SSASConfig& ssasConfig)
		{
			mMatchEndUrl = ssasConfig.getMatchEndUrl();
			mSkuMode = ssasConfig.getSkuMode();
			mMaxAttempts = ssasConfig.getMaxAttempts();
		}

		//=======================================================================================
		SSASConnection::~SSASConnection()
		{
		}

		BlazeRpcError SSASConnection::RequestMatchEnd(const SSASMatchEndRequest& ssasMatchEndRequest)
		{
			//BlazeRpcError error = SendRequest(ssasMatchEndRequest.getSidAsTdfString(), mMatchEndUrl.c_str(), HttpProtocolUtil::HttpMethod::HTTP_POST, ssasMatchEndRequest.getJsonAsTdfString());
			EA::TDF::TdfString json;
			json.set("{}");
			BlazeRpcError error = SendRequest(ssasMatchEndRequest.getSidAsTdfString(), "/sf/avatars", HttpProtocolUtil::HttpMethod::HTTP_GET, json);
			return error;
		}
		
		BlazeRpcError SSASConnection::SendRequest(
			const EA::TDF::TdfString& sid,
			const eastl::string& url,
			HttpProtocolUtil::HttpMethod httpMethod,
			const EA::TDF::TdfString& json)
		{
			HttpConnectionManagerPtr connectionManagerPtr = gOutboundHttpService->getConnection("ssas");

			if (connectionManagerPtr == nullptr)
			{
				return ERR_COMPONENT_NOT_FOUND;
			}

			eastl::string uri;
			uri.sprintf("%s?skuMode=%s", url.c_str(), mSkuMode.c_str());

			eastl::string sidHeader;
			sidHeader.sprintf("x-sf-sid: %s", sid.c_str());

			const int kNumHttpHeaders = 3;
			const char8_t* httpHeaders[kNumHttpHeaders] = {};
			httpHeaders[0] = "accept: application/json";
			httpHeaders[1] = "content-type: application/json";
			httpHeaders[2] = sidHeader.c_str();

			BlazeRpcError error = ERR_SYSTEM;
			uint32_t attempt = 0;

			for (; attempt < mMaxAttempts; ++attempt)
			{
				SSASConnectionResponse result;

				error = connectionManagerPtr->sendRequest(
					httpMethod,
					uri.c_str(),
					httpHeaders,
					kNumHttpHeaders,
					&result,
					"content-type: application/json",
					json.c_str(),
					json.length());

				if (error == Blaze::ERR_OK)
				{
					break;
				}
			}

			return error;
		}

		//=======================================================================================

	} // Volta
} // Blaze

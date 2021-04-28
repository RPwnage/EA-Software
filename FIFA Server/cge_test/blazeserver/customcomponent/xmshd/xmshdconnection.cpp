/*************************************************************************************************/
/*!
    \file   XmsHdConnection.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class XmsHdConnection

    Creates an HTTP connection manager to the XMS HD system.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/xmlbuffer.h"

#include "xmshdconnection.h"

// blaze includes


// xms includes
#include "xmshd/rpc/xmshdmaster.h"
#include "xmshd/tdf/xmshdtypes.h"

namespace Blaze
{
	namespace XmsHd
	{
		//===========================================================================================================================
		//===========================================================================================================================
		class XmsHdPartnerTokenResponse : public OutboundHttpResult
		{
		public:
			XmsHdPartnerTokenResponse() : OutboundHttpResult() { }
			virtual ~XmsHdPartnerTokenResponse() { }

			virtual void setHttpError(BlazeRpcError err = ERR_SYSTEM) { }
			virtual bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) { return false; }

			virtual void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) {}
			virtual void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) {}
		};

		/*** Public Methods ******************************************************************************/
		//=======================================================================================
		XmsHdConnection::XmsHdConnection(const ConfigMap *configRoot)
		{
			const ConfigMap* xmsHdData = configRoot->getMap("xmshd");

			// Clear the Partner Token.
			resetPartnerToken();

			// Set the variable for xmshd media calls
			mBaseUrl = eastl::string("/xmshd_collector/v1/");
			mApiKey = xmsHdData->getString("xmshdapikey", "by6pljeh2inmf6peckr3rtwq4reptcv00e7mhjpg");	// Default is FIFA PS4 XMS HD api key
			mSku = xmsHdData->getString("xmssku", "FFA18PS4");	// Default is FIFA PS4 XMS HD sku

			// Set the variables for partner token retrieval
			mPartnerTokenUrl = eastl::string("/authentication?username=partner/");
			mPartnerTokenSku = xmsHdData->getString("partnertokensku", "FFA16BLAZE");
			mPartnerTokenPassword = xmsHdData->getString("partnertokenpassword", "bx44ai5rq4bv1oy5bm98");

			mMaxAttempts = xmsHdData->getUInt32("maxattempts", 3);
		}

		//=======================================================================================
		XmsHdConnection::~XmsHdConnection()
		{
		}

		//=======================================================================================
		HttpConnectionManagerPtr XmsHdConnection::getConn()
		{
			// The string passed to getConnection must match the key of the entry you add to the httpServiceConfig.httpServices map in framework.cfg
			return gOutboundHttpService->getConnection("xmshd");
		}

		//=======================================================================================
		void XmsHdConnection::addBaseUrl(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, mBaseUrl.c_str(), dstStrLen);
		}

		//=======================================================================================
		void XmsHdConnection::addHttpParamAppendOp(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, "&", dstStrLen);
		}

		//=======================================================================================
		void XmsHdConnection::addHttpParam_ApiKey(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, "apikey=", dstStrLen);
			blaze_strnzcat(dstStr, mApiKey.c_str(), dstStrLen);
		}

		//=======================================================================================
		void XmsHdConnection::addHttpParam_Sku(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, "sku=", dstStrLen);
			blaze_strnzcat(dstStr, mSku.c_str(), dstStrLen);
		}

		//=======================================================================================
		void XmsHdConnection::addHttpParam_User(char8_t* dstStr, size_t dstStrLen, EntityId userId)
		{
			char8_t userHttpParamStr[64];
			blaze_snzprintf(userHttpParamStr, 64, "user=%" PRIi64, userId);

			blaze_strnzcat(dstStr, userHttpParamStr, dstStrLen);
		}

		//=======================================================================================
		void XmsHdConnection::addHttpParam_PartnerToken_Url(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, mPartnerTokenUrl.c_str(), dstStrLen);
		}

		//=======================================================================================
		void XmsHdConnection::addHttpParam_PartnerToken_Sku(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, mPartnerTokenSku.c_str(), dstStrLen);
		}

		//=======================================================================================
		void XmsHdConnection::addHttpParam_PartnerToken_Password(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, "password=", dstStrLen);
			blaze_strnzcat(dstStr, mPartnerTokenPassword.c_str(), dstStrLen);
		}

		//=======================================================================================
		void XmsHdConnection::addHttpParam_Auth(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, "auth=", dstStrLen);
			blaze_strnzcat(dstStr, getPartnerToken().c_str(), dstStrLen);
		}

		//=======================================================================================
		eastl::string XmsHdConnection::getPartnerToken()
		{
			TRACE_LOG("[Blaze::XmsHd::XmsHdConnection::getPartnerToken]");
			// Check to see if we have a Partner Token, if not, then go and fetch a new token.
			if (mPartnerToken.empty())
			{
				TRACE_LOG("[Blaze::XmsHd::XmsHdConnection::getPartnerToken] ==> Retrieving new EASW Token");

				const size_t kPartnerTokenUrlLen = 512;
				char8_t partnerTokenUrl[kPartnerTokenUrlLen];

				// Construct the URL for the HTTP GET request.
				blaze_snzprintf(partnerTokenUrl, kPartnerTokenUrlLen, "%s", "");

				// Add Base URL
				addHttpParam_PartnerToken_Url(partnerTokenUrl, kPartnerTokenUrlLen);

				// Add Sku Param
				addHttpParam_PartnerToken_Sku(partnerTokenUrl, kPartnerTokenUrlLen);

				// Add Password Param
				addHttpParamAppendOp(partnerTokenUrl, kPartnerTokenUrlLen);
				addHttpParam_PartnerToken_Password(partnerTokenUrl, kPartnerTokenUrlLen);

				//==================================================================================================
				// Construct the HTTP Header Params.
				char8_t httpHeader_Url [kPartnerTokenUrlLen];

				blaze_snzprintf(httpHeader_Url, kPartnerTokenUrlLen, "POST %s", partnerTokenUrl);

				const char8_t* httpHeaders[2];
				httpHeaders[0] = httpHeader_Url;
				httpHeaders[1] = "Keep-Alive: 115";

				// Send the HTTP Request.
				XmsHdPartnerTokenResponse results;

				HttpConnectionManagerPtr xmsHdConnection = getConn();

				if (xmsHdConnection != nullptr)
				{
					BlazeRpcError err = xmsHdConnection->sendRequest(
						HttpProtocolUtil::HTTP_POST,
						partnerTokenUrl,
						nullptr,
						0,
						httpHeaders,
						2,
						&results);

					if (err == Blaze::ERR_OK)
					{
						switch (results.getHttpStatusCode())
						{
						case HTTP_STATUS_OK:
						case HTTP_STATUS_SUCCESS:	// authenication call returns 201 status code
							mPartnerToken = results.getHeader("EASW-Token");
							break;
						default:
							break;
						}
					}
				}
				else
				{
					TRACE_LOG("[Blaze::XmsHd::XmsHdConnection::getPartnerToken] ==> Not able to get XmsHd Outgoing Http Connection!");
				}
			}

			return mPartnerToken;
		}

		//=======================================================================================
		void XmsHdConnection::resetPartnerToken()
		{
			mPartnerToken.clear();
		}

		//=======================================================================================

	} // XmsHd
} // Blaze

/*************************************************************************************************/
/*!
    \file   eaaccess_xblinventory.cpp

    $Header: //gosdev/games/FIFA/2015/Gen4/DEV/blazeserver/13.x/customcomponent/eaaccess/eaaccess_xblinventory.h $

    \attention
    (c) Electronic Arts Inc. 2014
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"
#include "framework/controller/controller.h"
#include "framework/oauth/accesstokenutil.h"

// easfc includes
#include "eaaccessslaveimpl.h"
#include "eaaccess/tdf/eaaccesstypes.h"

#include "EAJson/JsonDomReader.h"

namespace Blaze
{
    namespace EaAccess
    {

		//===========================================================================================================================
		//===========================================================================================================================
		class XblInventoryResponse : public OutboundHttpResult, public EA::Json::IJsonDomReaderCallback
		{
			public:
				XblInventoryResponse() : 
					OutboundHttpResult(),
					EA::Json::IJsonDomReaderCallback()
				{
					mState = "NotFound";
				}

				virtual ~XblInventoryResponse() { }

				/*** OutboundHttpResult Interface ************************************************************/
				virtual BlazeRpcError decodeResult(RawBuffer& response)
				{
					BlazeRpcError retval = Blaze::ERR_OK;

					// Use default Blaze allocator
					Allocator* allocator = Allocator::getAllocator();

					EA::Json::JsonDomReader reader(allocator);
					const char8_t* data = reinterpret_cast<const char8_t*>(response.data());

					if (reader.SetString(data, response.datasize(), false) == true)
					{
						EA::Json::JsonDomDocument node(allocator);

						if (reader.Build(node) == EA::Json::kSuccess)
						{
							 node.Iterate(this);
						}
						else
						{
							WARN_LOG("[XblInventoryResponse:" << this << "].decodeResult: Failed to build Json dom object.");

							// Copy data to temporary buffer in case data doesn't have null-terminator.
							eastl::string rawData(data, response.datasize());
							TRACE_LOG("[XblInventoryResponse:" << this << "].decodeResult: Received data: " << rawData.c_str());
						}
					}

					return retval;
				}

				virtual void setHttpError(BlazeRpcError err = ERR_SYSTEM) { }
				virtual bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) { return false; }
				virtual void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) { }
				virtual void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) { }

				/*** IJsonDomReaderCallback Interface ************************************************************/
				virtual bool BeginObject(EA::Json::JsonDomObject& domObject) { return true; }
				virtual bool EndObject(EA::Json::JsonDomObject& domObject) { return true; }

				virtual bool BeginObjectValue(EA::Json::JsonDomObject& domObject, const char* pName, size_t nNameLength, EA::Json::JsonDomObjectValue&)
				{
					mJsonFullName += '/';
					if (NULL != pName && 0 != nNameLength && '\0' != pName[0])
					{
						mJsonFullName += pName;
					}

					return true;
				}

				virtual bool EndObjectValue(EA::Json::JsonDomObject& domObject, EA::Json::JsonDomObjectValue&)
				{
					// Remove last added name
					eastl::string::size_type pos = mJsonFullName.rfind('/');
					if (eastl::string::npos != pos)
					{
						mJsonFullName.erase(pos);
					}
					else
					{
						BLAZE_ERR_LOG(BlazeRpcLog::eaaccess, ("[XblInventoryResponse].EndObjectValue: unexpected EndObjectValue."));
					}
					return true;
				}

				virtual bool String(EA::Json::JsonDomString&, const char* pValue, size_t nValueLength, const char* pText, size_t nLength)
				{
					TRACE_LOG("[XblInventoryResponse:" << this << "].String: FullName(" << mJsonFullName.c_str() << ") Received data: " << pValue);

					if (blaze_stricmp(mJsonFullName.c_str(), "/xblInventory/items/beginDate") == 0)
					{
						mBeginDate = pValue;
					}
					else if (blaze_stricmp(mJsonFullName.c_str(), "/xblInventory/items/endDate") == 0)
					{
						mEndDate = pValue;
					}
					else if (blaze_stricmp(mJsonFullName.c_str(), "/xblInventory/items/state") == 0)
					{
						mState = pValue;
					}
					else if (blaze_stricmp(mJsonFullName.c_str(), "/error/code") == 0)
					{
						mErrorCode = pValue;
					}

					return true;
				}

				virtual bool Integer(EA::Json::JsonDomInteger&, int64_t value, const char* pText, size_t nLength) 
				{ 
					TRACE_LOG("[XblInventoryResponse:" << this << "].Integer: FullName(" << mJsonFullName.c_str() << ") Received data: " << value);

					if (blaze_stricmp(mJsonFullName.c_str(), "/xblInventory/personaId") == 0)
					{
						mPersonaId = value;
					}

					return true; 
				}

				virtual bool Bool(EA::Json::JsonDomBool&, bool value, const char* pText, size_t nLength)
				{
					TRACE_LOG("[XblInventoryResponse:" << this << "].Bool: FullName(" << mJsonFullName.c_str() << ") Received data: " << value);

					if (blaze_stricmp(mJsonFullName.c_str(), "/xblInventory/trial") == 0)
					{
						mIsTrial = value;
					}

					return true; 

				}

				// Data to pass back to the caller.
				eastl::string mBeginDate;
				eastl::string mEndDate;
				eastl::string mState;

				int64_t mPersonaId;

				bool mIsTrial;

				eastl::string mErrorCode;

			private:

				XblInventoryResponse(const XblInventoryResponse& rhs); //!< Do not implement.
				XblInventoryResponse& operator=(const XblInventoryResponse& rhs); //!< Do not implement.

				eastl::string mJsonFullName; //!< This string stores full hierarchy path to currently parsed value ("/obj1/obj2/obj3")
		};

		/*===========================================================================================================================
		Make a Http call to Nexus to perform a B2B Xbox Live Inventory call.  Example HTTP call below
			
			GET https://gateway.int.ea.com/proxy/identity/personas/1000002439405/xblinventory?productId=f4fd26b2-6f93-4ff4-b811-b04804866acb
		===========================================================================================================================*/

		BlazeRpcError XblInventory(EaAccessConnection& connection, EaAccessSubInfoRequest& request, EaAccessSubInfoResponse& response)
		{
			TRACE_LOG("[Blaze::EaAccess::XblInventory]");
			BlazeRpcError retval = Blaze::ERR_OK;

			const size_t kXblInventoryUrlLen = 512;
			char8_t xblInventoryUrl[kXblInventoryUrlLen];

			int numPersonaIds = request.getPersonaIdList().size();

			for (int personaIndex = 0; personaIndex < numPersonaIds; ++personaIndex)
			{

				// Construct the URL for the HTTP GET request.
				blaze_snzprintf(xblInventoryUrl, kXblInventoryUrlLen, "%s", "");

				// Add Base URL
				connection.addBaseUrl(xblInventoryUrl, kXblInventoryUrlLen);
				connection.addPersonaIdUrl(xblInventoryUrl, kXblInventoryUrlLen, (request.getPersonaIdList())[personaIndex]);

				connection.addSuffixUrl(xblInventoryUrl, kXblInventoryUrlLen);

				blaze_strnzcat(xblInventoryUrl, "?", kXblInventoryUrlLen);

				// Add user Param
				connection.addHttpParam_ProductId(xblInventoryUrl, kXblInventoryUrlLen);

				//==================================================================================================
				// Construct the HTTP Header Params.
				char8_t httpHeader_Url[kXblInventoryUrlLen];

				blaze_snzprintf(httpHeader_Url, kXblInventoryUrlLen, "GET %s", xblInventoryUrl);

				// Fetch the Nucleus 2.0 auth token for this user to pass to the achievements service
				Blaze::OAuth::AccessTokenUtil tokenUtil;
				BlazeRpcError error = Blaze::ERR_OK;

				error = tokenUtil.retrieveCurrentUserAccessToken(Blaze::OAuth::TOKEN_TYPE_BEARER);

				if (error != Blaze::ERR_OK)
					return Blaze::EAACCESS_ERR_GENERAL;

				char8_t httpHeader_AuthToken[512];

				blaze_snzprintf(httpHeader_AuthToken, 512, "Authorization: %s", tokenUtil.getAccessToken());

				const char8_t* httpHeaders[3];
				httpHeaders[0] = httpHeader_Url;
				httpHeaders[1] = httpHeader_AuthToken;
				httpHeaders[2] = "Keep-Alive: 115";


				// Send the HTTP Request.
				XblInventoryResponse results;

				HttpConnectionManagerPtr eaAccessConnection = connection.getConn();

				if (eaAccessConnection != NULL)
				{

					BlazeRpcError err = eaAccessConnection->sendRequest(
						HttpProtocolUtil::HTTP_GET,
						xblInventoryUrl,
						NULL,
						0,
						httpHeaders,
						3,
						&results);

					if (err == Blaze::ERR_OK)
					{
						switch (results.getHttpStatusCode())
						{
							case HTTP_STATUS_OK:
							{
								Blaze::EaAccess::EaAccessSubAttributes* subAttributes = new Blaze::EaAccess::EaAccessSubAttributes();;

								subAttributes->setPersonaId(results.mPersonaId);

								subAttributes->setBeginDate(results.mBeginDate.c_str());
								subAttributes->setEndDate(results.mEndDate.c_str());
								subAttributes->setState(results.mState.c_str());
								subAttributes->setIsTrial(results.mIsTrial);

								subAttributes->setIsValid(true);

								response.getSubInfo().push_back(subAttributes);
							}
							break;
							case 404: // HTTP_STATUS_NOT_FOUND
							{
								Blaze::EaAccess::EaAccessSubAttributes* subAttributes = new Blaze::EaAccess::EaAccessSubAttributes();;

								subAttributes->setPersonaId(results.mPersonaId);

								subAttributes->setErrorCode(results.mErrorCode.c_str());

								subAttributes->setIsValid(false);

								response.getSubInfo().push_back(subAttributes);
							}
							break;
							default:
							{
								retval = Blaze::EAACCESS_ERR_GENERAL;
							}
							break;
						}
					}
					else
					{
						retval = Blaze::EAACCESS_ERR_GENERAL;
					}
				}
				else
				{
					retval = Blaze::EAACCESS_ERR_GENERAL;
				}

			}

			return retval;
		}

    } // EaAccess
} // Blaze

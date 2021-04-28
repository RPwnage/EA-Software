/*************************************************************************************************/
/*!
    \file   xmshd_filestatus.cpp

    $Header: //gosdev/games/FIFA/2014/Gen3/DEV/blazeserver/13.x/customcomponent/xmshd/xmshd_operations.h $

    \attention
    (c) Electronic Arts Inc. 2013
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

// easfc includes
#include "xmshdslaveimpl.h"
#include "xmshd/tdf/xmshdtypes.h"

#include "xmshdconnection.h"

namespace Blaze
{
    namespace XmsHd
    {

		//===========================================================================================================================
		//===========================================================================================================================
		class XmsHdFileStatusCheckResponse : public OutboundHttpResult
		{
			public:
				XmsHdFileStatusCheckResponse() : OutboundHttpResult() { }
				virtual ~XmsHdFileStatusCheckResponse() { }

				virtual void setHttpError(BlazeRpcError err = ERR_SYSTEM) { }
				virtual bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) { return false; }

				virtual void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) { }
				virtual void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) { }
		};

		/*===========================================================================================================================
		Make a Http call to Xms Hd to check the status.  Example HTTP call below
			
			GET http://fifa.dev.service.easports.com/xmshd_collector/v1/file/fifa_online_BAP.bin/status?apikey=3lt524nbmksox0dbv0m34kbo2kqwg3cz9mrse22z&sku=FIF14PS3&type=name&user=12304794142
		===========================================================================================================================*/

		BlazeRpcError DoesFileExist(XmsHdConnection& connection, PublishDataRequest& request)
		{
			TRACE_LOG("[Blaze::XmsHd::DoesFileExist]");
			BlazeRpcError retval = Blaze::ERR_OK;

			const char8_t* xmsAssetName = request.getXmsAssetName();

			if (strlen(xmsAssetName) > 0)
			{
				uint32_t attemptCount = 0;
				uint32_t maxAttemptCount = connection.getMaxAttempts();

				bool isSuccessful = false;

				while (!isSuccessful && attemptCount < maxAttemptCount)
				{
					const size_t kFileStatusUrlLen = 1024;
					char8_t fileStatusUrl[kFileStatusUrlLen];

					// Increment the attempt Count.
					++attemptCount;

					// Construct the URL for the HTTP GET request.
					blaze_snzprintf(fileStatusUrl, kFileStatusUrlLen, "%s", "");

					// Add Base URL
					connection.addBaseUrl(fileStatusUrl, kFileStatusUrlLen);

					// Add filename to check status of
					eastl::string fileNameUrl;
					fileNameUrl.sprintf("file/%s/status?", xmsAssetName);
					blaze_strnzcat(fileStatusUrl, fileNameUrl.c_str(), kFileStatusUrlLen);

					// Add ApiKey Param
					connection.addHttpParam_ApiKey(fileStatusUrl, kFileStatusUrlLen);

					// Add Sku Param
					connection.addHttpParamAppendOp(fileStatusUrl, kFileStatusUrlLen);
					connection.addHttpParam_Sku(fileStatusUrl, kFileStatusUrlLen);

					// Add type Param
					blaze_strnzcat(fileStatusUrl, "&type=name", kFileStatusUrlLen);

					// Add user Param
					connection.addHttpParamAppendOp(fileStatusUrl, kFileStatusUrlLen);
					connection.addHttpParam_User(fileStatusUrl, kFileStatusUrlLen, request.getNucleusId());

					// Add auth Param
					connection.addHttpParamAppendOp(fileStatusUrl, kFileStatusUrlLen);
					connection.addHttpParam_Auth(fileStatusUrl, kFileStatusUrlLen);

					//==================================================================================================
					// Construct the HTTP Header Params.
					char8_t httpHeader_Url [kFileStatusUrlLen];

					blaze_snzprintf(httpHeader_Url, kFileStatusUrlLen, "GET %s", fileStatusUrl);

					const char8_t* httpHeaders[2];
					httpHeaders[0] = httpHeader_Url;
					httpHeaders[1] = "Keep-Alive: 115";

					// Send the HTTP Request.
					XmsHdFileStatusCheckResponse results;

					HttpConnectionManagerPtr xmsHdConnection = connection.getConn();

					if (xmsHdConnection != nullptr)
					{

						BlazeRpcError err = xmsHdConnection->sendRequest(
							HttpProtocolUtil::HTTP_GET,
							fileStatusUrl,
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
									isSuccessful = true;
									retval = Blaze::XMSHD_ERR_FILE_STATUS_FOUND;
									break;
								case 400: // HTTP_STATUS_BAD_REQUEST
									isSuccessful = true;
									retval = Blaze::XMSHD_ERR_HTTP_BAD_REQUEST;
									break;
								case 401: // HTTP_STATUS_NOT_AUTHORIZED
									// If we get this result, we likely due to expired auth token
									connection.resetPartnerToken();
									retval = Blaze::XMSHD_ERR_HTTP_UNAUTHORIZED;
									break;
								case 403: // HTTP_STATUS_FORBIDDEN
									// Likely bad params like apiKeys passed in.
									isSuccessful = true;
									retval = Blaze::XMSHD_ERR_HTTP_FORBIDDEN_ACCESS;
									break;
								case 404: // HTTP_STATUS_NOT_FOUND
									isSuccessful = true;
									retval = Blaze::XMSHD_ERR_FILE_STATUS_NOTFOUND;
									break;
								default:
									isSuccessful = true;

									if ((results.getHttpStatusCode() >= 500) && (results.getHttpStatusCode() < 600))
									{
										retval = Blaze::XMSHD_ERR_HTTP_SERVER;
									}
									else
									{
										retval = Blaze::XMSHD_ERR_FILE_STATUS_FAILED;
									}
									break;
							}
						}
						else
						{
							isSuccessful = true;
							retval = err;
						}

					}
					else
					{
						TRACE_LOG("[Blaze::XmsHd::DoesFileExist] Unable to get XMSHD Outgoing Http Connection");
						retval = Blaze::XMSHD_ERR_NO_HTTP_CONNECTION_AVAILABLE;
					}
				}
			}

			return retval;
		}

    } // XmsHd
} // Blaze

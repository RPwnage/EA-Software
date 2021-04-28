/*************************************************************************************************/
/*!
    \file   xmshd_createmedia.cpp

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
#include "xmshdoperations.h"
#include "xmshd_httputil.h"

namespace Blaze
{
    namespace XmsHd
    {
		static const char8_t* CREATEMEDIA_MULTIPART_BOUNDARY = "-31415926535897932384626";

		//===========================================================================================================================
		//===========================================================================================================================
		class XmsHdCreateMediaResponse : public OutboundHttpResult
		{
			public:
				XmsHdCreateMediaResponse() : OutboundHttpResult() { }
				virtual ~XmsHdCreateMediaResponse() { }

				virtual void setHttpError(BlazeRpcError err = ERR_SYSTEM) { }
				virtual bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) { return false; }

				virtual void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) { }
				virtual void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) { }
		};

		/*===== CONTENT DATA FORMAT!!!! =====

		HTTP_MULTIPART_BOUNDARY_DELIMITER = "--"
		DEFAULT_MULTIPART_BOUNDARY = "-31415926535897932384626"
		HTTP_MULTIPART_BOUNDARY_SECTION_META = "Content-Disposition: form-data; name=\"metadata\"\r\nContent-Type: application/json;charset=ISO-8859-1\r\n";

		JSON format Metadata

		RETURN_NEW_LINE = "\r\n"
		HTTP_MULTIPART_BOUNDARY_DELIMITER = "--"
		DEFAULT_MULTIPART_BOUNDARY = "-31415926535897932384626"

		// For each file
		HTTP_MULTIPART_BLOCK_HEAD = "\r\nContent-Disposition: form-data; name=\"<filename>\"; filename=\"<filename>\"\r\nContent-Type: text/plain; charset=ISO-8859-1\r\n\r\n"
		RETURN_NEW_LINE = "\r\n"
		HTTP_MULTIPART_BOUNDARY_DELIMITER = "--"
		DEFAULT_MULTIPART_BOUNDARY = "-31415926535897932384626"

		File content

		// EOF
		HTTP_MULTIPART_BOUNDARY_DELIMITER = "--"
		================================= */

		/*===========================================================================================================================
		Make a Http call to Xms Hd to publish the file.  Example HTTP call below
			
			POST http://fifa.dev.service.easports.com/xmshd_collector/v1/file?apikey=3lt524nbmksox0dbv0m34kbo2kqwg3cz9mrse22z&sku=FIF14PS3&type=name&user=12304794142
		===========================================================================================================================*/

		BlazeRpcError CreateMedia(XmsHdConnection& connection, PublishDataRequest& request)
		{
			TRACE_LOG("[Blaze::XmsHd::CreateMedia]");
			BlazeRpcError retval = Blaze::ERR_OK;

			const char8_t* xmsAssetName = request.getXmsAssetName();

			if (strlen(xmsAssetName) > 0)
			{
				uint32_t attemptCount = 0;
				uint32_t maxAttemptCount = connection.getMaxAttempts();

				bool isSuccessful = false;

				while (!isSuccessful && attemptCount < maxAttemptCount)
				{
					const size_t kCreateMediaUrlLen = 1024;
					char8_t createMediaUrl[kCreateMediaUrlLen];

					// Increment the attempt Count.
					++attemptCount;

					// Construct the URL for the HTTP GET request.
					blaze_snzprintf(createMediaUrl, kCreateMediaUrlLen, "%s", "");

					// Add Base URL
					connection.addBaseUrl(createMediaUrl, kCreateMediaUrlLen);

					// Add file component of the URL
					blaze_strnzcat(createMediaUrl, "file?", kCreateMediaUrlLen);
					// Do we need to add the /<filename>

					// Add ApiKey Param
					connection.addHttpParam_ApiKey(createMediaUrl, kCreateMediaUrlLen);

					// Add Sku Param
					connection.addHttpParamAppendOp(createMediaUrl, kCreateMediaUrlLen);
					connection.addHttpParam_Sku(createMediaUrl, kCreateMediaUrlLen);

					// Add type Param
					blaze_strnzcat(createMediaUrl, "&type=name", kCreateMediaUrlLen);

					// Add user Param
					connection.addHttpParamAppendOp(createMediaUrl, kCreateMediaUrlLen);
					connection.addHttpParam_User(createMediaUrl, kCreateMediaUrlLen, request.getNucleusId());

					// Add auth Param
					connection.addHttpParamAppendOp(createMediaUrl, kCreateMediaUrlLen);
					connection.addHttpParam_Auth(createMediaUrl, kCreateMediaUrlLen);

					//==================================================================================================
					// Construct the HTTP Header Params.
					char8_t httpHeader_Url [kCreateMediaUrlLen];
					char8_t httpHeader_ContentType[kCreateMediaUrlLen];
					char8_t httpHeader_ContentDisposition[kCreateMediaUrlLen];

					blaze_snzprintf(httpHeader_Url, kCreateMediaUrlLen, "POST %s", createMediaUrl);
					blaze_snzprintf(httpHeader_ContentType, kCreateMediaUrlLen, HTTP_HEADER_FIELD_CONTENT_TYPE_MULTIPART, CREATEMEDIA_MULTIPART_BOUNDARY);
					blaze_snzprintf(httpHeader_ContentDisposition, kCreateMediaUrlLen, HTTP_HEADER_FIELD_CONTENT_DISPOSITION, xmsAssetName, xmsAssetName);

					const char8_t* httpHeaders[3];
					httpHeaders[0] = httpHeader_Url;
					httpHeaders[1] = httpHeader_ContentDisposition;
					httpHeaders[2] = "Keep-Alive: 115";

					//==================================================================================================
					// Construct the HTTP Body

					// Calculate Size of buffer required to submit.
					size_t dataSize = 0;

					WriteMetaDataHeader(nullptr, dataSize, 0, CREATEMEDIA_MULTIPART_BOUNDARY);
					WriteMetaData(nullptr, dataSize, 0, request);
					WriteMultiPartBoundary(NULL, dataSize, 0, CREATEMEDIA_MULTIPART_BOUNDARY);
					WriteContentDatas(nullptr, dataSize, 0, CREATEMEDIA_MULTIPART_BOUNDARY, request);
					WriteMultiPartBoundaryDelimiter(nullptr, dataSize, 0);

					size_t bufferSize = dataSize + 1;
					char8_t* buffer = BLAZE_NEW_ARRAY(char8_t, bufferSize);

					// Clear the new buffer
					memset(buffer, 0x00, bufferSize);
					dataSize = 0;

					// Generate the http body data
					WriteMetaDataHeader(buffer, dataSize, bufferSize, CREATEMEDIA_MULTIPART_BOUNDARY);
					WriteMetaData(buffer, dataSize, bufferSize, request);
					WriteMultiPartBoundary(buffer, dataSize, bufferSize, CREATEMEDIA_MULTIPART_BOUNDARY);
					WriteContentDatas(buffer, dataSize, bufferSize, CREATEMEDIA_MULTIPART_BOUNDARY, request);
					WriteMultiPartBoundaryDelimiter(buffer, dataSize, bufferSize);

					// Send the HTTP Request.
					XmsHdCreateMediaResponse results;

					HttpConnectionManagerPtr xmsHdConnection = connection.getConn();

					if (xmsHdConnection != nullptr)
					{
						BlazeRpcError err = xmsHdConnection->sendRequest(
							HttpProtocolUtil::HTTP_POST,
							createMediaUrl,
							httpHeaders,
							3,
							&results,
							httpHeader_ContentType,
							buffer,
							dataSize
						);

						delete[] buffer;

						if (err == Blaze::ERR_OK)
						{
							switch (results.getHttpStatusCode())
							{
								case HTTP_STATUS_SUCCESS:
									isSuccessful = true;
									retval = Blaze::ERR_OK;
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
										retval = Blaze::XMSHD_ERR_CREATE_MEDIA_FAILED;
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
						delete[] buffer;

						TRACE_LOG("[Blaze::XmsHd::CreateMedia] Unable to get XMSHD Outgoing Http Connection");
						retval = Blaze::XMSHD_ERR_NO_HTTP_CONNECTION_AVAILABLE;
					}
				}
			}

			return retval;
		}

    } // XmsHd
} // Blaze

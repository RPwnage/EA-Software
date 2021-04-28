/*************************************************************************************************/
/*!
    \file   xmshd_updatemedia.cpp

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
		static const char8_t* UPDATEMEDIA_MULTIPART_BOUNDARY = "-31415926535897932384626";

		//===========================================================================================================================
		//===========================================================================================================================
		class XmsHdUpdateMediaResponse : public OutboundHttpResult
		{
			public:
				XmsHdUpdateMediaResponse() : OutboundHttpResult() { }
				virtual ~XmsHdUpdateMediaResponse() { }

				virtual void setHttpError(BlazeRpcError err = ERR_SYSTEM) { }
				virtual bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) { return false; }

				virtual void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) { }
				virtual void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) { }
		};

		/*===== CONTENT DATA FORMAT!!!! =====

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
		Make a Http call to Xms Hd to update the file.  Example HTTP call below

			Update Metadata
			PUT http://fifa.dev.service.easports.com/xmshd_collector/v1/file/fifa_online_BAP.bin/metadata?apikey=3lt524nbmksox0dbv0m34kbo2kqwg3cz9mrse22z&sku=FIF14PS3&type=name&user=12304794142
		
			Update Binary
			POST http://fifa.dev.service.easports.com/xmshd_collector/v1/file/fifa_online_BAP.bin?apikey=3lt524nbmksox0dbv0m34kbo2kqwg3cz9mrse22z&sku=FIF14PS3&type=name&user=12304794142
		===========================================================================================================================*/
		BlazeRpcError UpdateMediaBinaries(XmsHdConnection& connection, PublishDataRequest& request)
		{
			TRACE_LOG("[Blaze::XmsHd::UpdateMediaBinaries]");
			BlazeRpcError retval = Blaze::XMSHD_ERR_UPDATE_MEDIA_FAILED;

			PublishDataRequest::XmsBinaryList::iterator binaryIdx = request.getBinaries().begin();
			PublishDataRequest::XmsBinaryList::iterator binaryEnd = request.getBinaries().end();

			const size_t kUpdateMediaUrlLen = 1024;
			char8_t updateMediaUrl[kUpdateMediaUrlLen];

			for (; binaryIdx != binaryEnd; ++binaryIdx)
			{
				uint32_t attemptCount = 0;
				uint32_t maxAttemptCount = connection.getMaxAttempts();

				bool isSuccessful = false;

				while (!isSuccessful && attemptCount < maxAttemptCount)
				{
					Blaze::XmsHd::XmsBinary* xmsBinary = (*binaryIdx);
					const char8_t* fileName = xmsBinary->getName();

					// Increment the attempt Count.
					++attemptCount;

					// Construct the URL for the HTTP GET request.
					blaze_snzprintf(updateMediaUrl, kUpdateMediaUrlLen, "%s", "");

					// Add Base URL
					connection.addBaseUrl(updateMediaUrl, kUpdateMediaUrlLen);

					// Add file component of the URL
					eastl::string fileCommandStr;
					fileCommandStr.sprintf("file/%s?", fileName);

					blaze_strnzcat(updateMediaUrl, fileCommandStr.c_str(), kUpdateMediaUrlLen);
					// Do we need to add the /<filename>

					// Add ApiKey Param
					connection.addHttpParam_ApiKey(updateMediaUrl, kUpdateMediaUrlLen);

					// Add Sku Param
					connection.addHttpParamAppendOp(updateMediaUrl, kUpdateMediaUrlLen);
					connection.addHttpParam_Sku(updateMediaUrl, kUpdateMediaUrlLen);

					// Add type Param
					blaze_strnzcat(updateMediaUrl, "&type=name", kUpdateMediaUrlLen);

					// Add user Param
					connection.addHttpParamAppendOp(updateMediaUrl, kUpdateMediaUrlLen);
					connection.addHttpParam_User(updateMediaUrl, kUpdateMediaUrlLen, request.getNucleusId());

					// Add auth Param
					connection.addHttpParamAppendOp(updateMediaUrl, kUpdateMediaUrlLen);
					connection.addHttpParam_Auth(updateMediaUrl, kUpdateMediaUrlLen);

					//==================================================================================================
					// Construct the HTTP Header Params.
					char8_t httpHeader_Url [kUpdateMediaUrlLen];
					char8_t httpHeader_ContentType[kUpdateMediaUrlLen];
					char8_t httpHeader_ContentDisposition[kUpdateMediaUrlLen];

					blaze_snzprintf(httpHeader_Url, kUpdateMediaUrlLen, "POST %s", updateMediaUrl);
					blaze_snzprintf(httpHeader_ContentType, kUpdateMediaUrlLen, HTTP_HEADER_FIELD_CONTENT_TYPE_MULTIPART, UPDATEMEDIA_MULTIPART_BOUNDARY);
					blaze_snzprintf(httpHeader_ContentDisposition, kUpdateMediaUrlLen, HTTP_HEADER_FIELD_CONTENT_DISPOSITION, fileName, fileName);

					const char8_t* httpHeaders[3];
					httpHeaders[0] = httpHeader_Url;
					httpHeaders[1] = httpHeader_ContentDisposition;
					httpHeaders[2] = "Keep-Alive: 115";

					//==================================================================================================
					// Construct the HTTP Body

					// Calculate Size of buffer required to submit.
					size_t dataSize = 0;

					WriteMultiPartBoundary(nullptr, dataSize, 0, UPDATEMEDIA_MULTIPART_BOUNDARY);
					WriteContentData(nullptr, dataSize, 0, UPDATEMEDIA_MULTIPART_BOUNDARY, xmsBinary);
					WriteMultiPartBoundaryDelimiter(nullptr, dataSize, 0);

					size_t bufferSize = dataSize + 1;
					char8_t* buffer = BLAZE_NEW_ARRAY(char8_t, bufferSize);

					// Clear the new buffer
					memset(buffer, 0x00, bufferSize);
					dataSize = 0;

					// Generate the http body data
					WriteMultiPartBoundary(buffer, dataSize, bufferSize, UPDATEMEDIA_MULTIPART_BOUNDARY);
					WriteContentData(buffer, dataSize, bufferSize, UPDATEMEDIA_MULTIPART_BOUNDARY, xmsBinary);
					WriteMultiPartBoundaryDelimiter(buffer, dataSize, bufferSize);

					// Send the HTTP Request.
					XmsHdUpdateMediaResponse results;

					HttpConnectionManagerPtr xmsHdConnection = connection.getConn();

					if (xmsHdConnection != nullptr)
					{
						BlazeRpcError err = xmsHdConnection->sendRequest(
							HttpProtocolUtil::HTTP_POST,
							updateMediaUrl,
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
										retval = Blaze::XMSHD_ERR_UPDATE_MEDIA_FAILED;
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

						TRACE_LOG("[Blaze::XmsHd::UpdateMediaBinaries] Unable to get XMSHD Outgoing Http Connection");
						retval = Blaze::XMSHD_ERR_NO_HTTP_CONNECTION_AVAILABLE;
					}
				}

				if (retval != Blaze::ERR_OK)
				{
					break;
				}
			}

			return retval;
		}
		//===========================================================================================================================
		BlazeRpcError UpdateMediaMetaData(XmsHdConnection& connection, PublishDataRequest& request)
		{
			TRACE_LOG("[Blaze::XmsHd::UpdateMediaMetaData]");
			BlazeRpcError retval = Blaze::XMSHD_ERR_UPDATE_MEDIA_FAILED;

			//==================================================================================================
			// Construct the HTTP Body

			// Calculate Size of buffer required to submit.
			size_t dataSize = 0;

			WriteMetaData(nullptr, dataSize, 0, request);

			size_t bufferSize = dataSize + 1;
			char8_t* buffer = BLAZE_NEW_ARRAY(char8_t, bufferSize);

			// Clear the new buffer
			memset(buffer, 0x00, bufferSize);
			dataSize = 0;

			// Generate the http body data
			WriteMetaData(buffer, dataSize, bufferSize, request);

			PublishDataRequest::XmsBinaryList::iterator binaryIdx = request.getBinaries().begin();
			PublishDataRequest::XmsBinaryList::iterator binaryEnd = request.getBinaries().end();

			const size_t kUpdateMediaUrlLen = 1024;
			char8_t updateMediaUrl[kUpdateMediaUrlLen];

			for (; binaryIdx != binaryEnd; ++binaryIdx)
			{
				uint32_t attemptCount = 0;
				uint32_t maxAttemptCount = connection.getMaxAttempts();

				bool isSuccessful = false;

				while (!isSuccessful && attemptCount < maxAttemptCount)
				{
					Blaze::XmsHd::XmsBinary* xmsBinary = (*binaryIdx);
					const char8_t* fileName = xmsBinary->getName();

					// Increment the attempt Count.
					++attemptCount;

					// Construct the URL for the HTTP GET request.
					blaze_snzprintf(updateMediaUrl, kUpdateMediaUrlLen, "%s", "");

					// Add Base URL
					connection.addBaseUrl(updateMediaUrl, kUpdateMediaUrlLen);

					// Add file component of the URL
					eastl::string fileCommandStr;
					fileCommandStr.sprintf("file/%s/metadata?", fileName);

					blaze_strnzcat(updateMediaUrl, fileCommandStr.c_str(), kUpdateMediaUrlLen);
					// Do we need to add the /<filename>

					// Add ApiKey Param
					connection.addHttpParam_ApiKey(updateMediaUrl, kUpdateMediaUrlLen);

					// Add Sku Param
					connection.addHttpParamAppendOp(updateMediaUrl, kUpdateMediaUrlLen);
					connection.addHttpParam_Sku(updateMediaUrl, kUpdateMediaUrlLen);

					// Add type Param
					blaze_strnzcat(updateMediaUrl, "&type=name", kUpdateMediaUrlLen);

					// Add user Param
					connection.addHttpParamAppendOp(updateMediaUrl, kUpdateMediaUrlLen);
					connection.addHttpParam_User(updateMediaUrl, kUpdateMediaUrlLen, request.getNucleusId());

					// Add auth Param
					connection.addHttpParamAppendOp(updateMediaUrl, kUpdateMediaUrlLen);
					connection.addHttpParam_Auth(updateMediaUrl, kUpdateMediaUrlLen);

					//==================================================================================================
					// Construct the HTTP Header Params.
					char8_t httpHeader_Url [kUpdateMediaUrlLen];
					char8_t httpHeader_ContentType[kUpdateMediaUrlLen];
					char8_t httpHeader_ContentDisposition[kUpdateMediaUrlLen];

					blaze_snzprintf(httpHeader_Url, kUpdateMediaUrlLen, "PUT %s", updateMediaUrl);
					blaze_snzprintf(httpHeader_ContentType, kUpdateMediaUrlLen, HTTP_HEADER_FIELD_CONTENT_TYPE_JSON);
					blaze_snzprintf(httpHeader_ContentDisposition, kUpdateMediaUrlLen, HTTP_HEADER_FIELD_CONTENT_DISPOSITION, fileName, fileName);

					const char8_t* httpHeaders[2];
					httpHeaders[0] = httpHeader_Url;
					httpHeaders[1] = "Keep-Alive: 115";


					// Send the HTTP Request.
					XmsHdUpdateMediaResponse results;

					HttpConnectionManagerPtr xmsHdConnection = connection.getConn();

					if (xmsHdConnection != nullptr)
					{
						BlazeRpcError err = xmsHdConnection->sendRequest(
							HttpProtocolUtil::HTTP_PUT,
							updateMediaUrl,
							httpHeaders,
							2,
							&results,
							httpHeader_ContentType,
							buffer,
							dataSize
						);

						if (err == Blaze::ERR_OK)
						{
							switch (results.getHttpStatusCode())
							{
								case HTTP_STATUS_OK:
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
										retval = Blaze::XMSHD_ERR_UPDATE_MEDIA_FAILED;
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
						TRACE_LOG("[Blaze::XmsHd::UpdateMediaMetaData] Unable to get XMSHD Outgoing Http Connection");
						retval = Blaze::XMSHD_ERR_NO_HTTP_CONNECTION_AVAILABLE;
					}
				}

				if (retval != Blaze::ERR_OK)
				{
					break;
				}

			}

			delete [] buffer;

			return retval;
		}


		//===========================================================================================================================
		BlazeRpcError UpdateMedia(XmsHdConnection& connection, PublishDataRequest& request)
		{
			TRACE_LOG("[Blaze::XmsHd::UpdateMedia]");
			BlazeRpcError retval = Blaze::ERR_OK;

			retval = UpdateMediaMetaData(connection, request);

			if (retval == Blaze::ERR_OK)
			{
				retval = UpdateMediaBinaries(connection, request);
			}

			return retval;
		}

    } // XmsHd
} // Blaze

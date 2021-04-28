/*************************************************************************************************/
/*!
    \file   setMultipleInstanceAttributesOp.cpp

    Header: fifagroupsoperations.h

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
#include "framework/connection/outboundhttpservice.h"
#include "framework/oauth/accesstokenutil.h"

#include "fifagroups/tdf/fifagroupstypes.h"
#include "fifagroupsslaveimpl.h"
#include "fifagroupsconnection.h"
#include "fifagroupshttpresponse.h"

#include "EAJson/JsonWriter.h"

namespace Blaze
{
    namespace FifaGroups
    {
		char8_t* ConstructSetMultipleInstanceAttributesBody(SetMultipleInstanceAttributesRequest& request, size_t& outBodySize)
		{
			EA::Json::StringWriteStream<eastl::string> jsonStream;
			EA::Json::JsonWriter jsonWriter;
			jsonWriter.SetFormatOption(EA::Json::JsonWriter::FormatOption::kFormatOptionLineEnd, 0);
			jsonWriter.SetStream(&jsonStream);

			jsonWriter.BeginObject();
			jsonWriter.BeginObjectValue(JSON_ATTRIBUTES_TAG);
			jsonWriter.BeginObject();

			Blaze::FifaGroups::AttributeList::iterator iter, end;
			iter = request.getAttributes().begin();
			end = request.getAttributes().end();

			for (; iter != end; iter++)
			{
				EA::TDF::tdf_ptr<Blaze::FifaGroups::Attribute> attribute = *iter;
				jsonWriter.BeginObjectValue(attribute->getKey());

				if (attribute->getType() == Blaze::FifaGroups::AttributeType::TYPE_STRING)
				{
					jsonWriter.String(attribute->getSValue());
				}
				else if (attribute->getType() == Blaze::FifaGroups::AttributeType::TYPE_INT)
				{
					jsonWriter.Integer(attribute->getIValue());
				}
			}
			jsonWriter.EndObject();
			jsonWriter.EndObject();

			outBodySize = jsonStream.mString.length();

			size_t bufferSize = outBodySize + 1;
			char8_t* body = BLAZE_NEW_ARRAY(char8_t, bufferSize);
			memset(body, 0x00, bufferSize);

			if (body != NULL && bufferSize != 0)
			{
				blaze_strnzcpy(body, jsonStream.mString.c_str(), bufferSize);
			}
			return body;
		};

		/*===========================================================================================================================
		Make a Http call to FifaGroups to set multiple instance attributes.  Example HTTP call below 
					
			DELETE https://groups.integration.gameservices.ea.com:10091/group//instance/{groupguid}/attributes  
		===========================================================================================================================*/

		BlazeRpcError SetMultipleInstanceAttributesOperation(FifaGroupsConnection& connection, SetMultipleInstanceAttributesRequest& request, SetMultipleInstanceAttributesResponse& response)
		{
			return SetMultipleInstanceAttributesOperation(connection, gCurrentUserSession->getServiceName(), gCurrentUserSession->getUserId(), request, response);
		}

		BlazeRpcError SetMultipleInstanceAttributesOperation(FifaGroupsConnection& connection, const char* serviceName, BlazeId userId, SetMultipleInstanceAttributesRequest& request, SetMultipleInstanceAttributesResponse& response)
		{
			BlazeRpcError retval = Blaze::ERR_OK;

			uint32_t attemptCount = 0;
			uint32_t maxAttemptCount = connection.getMaxAttempts();
			bool isSuccessful = false;

			// Fetch the Nucleus 2.0 auth token for this user to pass to the achievements service
			OAuth::AccessTokenUtil tokenUtil;
			BlazeRpcError error = Blaze::ERR_OK;

			const int tokenScopeLen = 32;
			char8_t tokenScope[tokenScopeLen];
			connection.getTokenScope(tokenScope, tokenScopeLen);

			error = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NONE, tokenScope);
			if (error != Blaze::ERR_OK)
			{
				return Blaze::FIFAGROUPS_ERR_TOKEN_RETRIEVAL_FAILED;
			}

			const int setMultipleInstanceAttributesUrlLen = 128;
			char8_t setMultipleInstanceAttributesUrl[setMultipleInstanceAttributesUrlLen];

			// Construct the URL for the HTTP GET request.
			blaze_snzprintf(setMultipleInstanceAttributesUrl, setMultipleInstanceAttributesUrlLen, "%s", "");
			connection.addBaseUrl(setMultipleInstanceAttributesUrl, setMultipleInstanceAttributesUrlLen);
			connection.addSetMultipeInstanceAttributesAPI(setMultipleInstanceAttributesUrl, setMultipleInstanceAttributesUrlLen, request.getGroupGUId());
						
			//Construct Body
			size_t datasize = 0;
			char8_t* bodyPayLoad = ConstructSetMultipleInstanceAttributesBody(request, datasize);

			//Construct headers
			const int headerLen = 128;

			char8_t httpHeader_AuthToken[headerLen];
			char8_t httpHeader_UserType[headerLen];
			char8_t httpHeader_UserId[headerLen];
			char8_t httpHeader_APIVersion[headerLen];
			char8_t httpHeader_ContentType[headerLen];
			char8_t httpHeader_AcceptType[headerLen];

			const int apiVersionLen = 32;
			char8_t apiVersion[apiVersionLen];
			connection.getApiVersion(apiVersion, apiVersionLen);

			blaze_snzprintf(httpHeader_AuthToken, headerLen, "Authorization: NEXUS_S2S %s", tokenUtil.getAccessToken());
			blaze_strnzcpy(httpHeader_UserType, "X-Forwarded-UserType: NUCLEUS_PERSONA", headerLen);
			blaze_snzprintf(httpHeader_UserId, 128, "X-Forwarded-UserId: %" PRIi64 "", userId);
			blaze_snzprintf(httpHeader_APIVersion, headerLen, "X-Api-Version: %s", apiVersion);
			blaze_strnzcpy(httpHeader_ContentType, "Content-Type: application/json", headerLen);
			blaze_strnzcpy(httpHeader_AcceptType, "Accept: application/json", headerLen);

			const char8_t* httpHeaders[5];
			httpHeaders[0] = httpHeader_AuthToken;
			httpHeaders[1] = httpHeader_UserType;
			httpHeaders[2] = httpHeader_UserId;
			httpHeaders[3] = httpHeader_APIVersion;	
			httpHeaders[4] = httpHeader_AcceptType;
				
			HttpConnectionManagerPtr httpConnManager = connection.getHttpConnManager();
			if (httpConnManager == NULL)
			{
				return Blaze::ERR_SYSTEM;
			}

			while (!isSuccessful && attemptCount < maxAttemptCount)
			{
				// Increment the attempt Count.
				attemptCount++;

				// Send the HTTP Request.
				HttpResponse results;

				BlazeRpcError err = httpConnManager->sendRequest(
					HttpProtocolUtil::HTTP_PUT,
					setMultipleInstanceAttributesUrl,
					httpHeaders,
					5,
					&results,
					httpHeader_ContentType,
					bodyPayLoad,
					datasize
				);

				if (err == Blaze::ERR_OK)
				{
					uint32_t statusCode = results.getHttpStatusCode();
					response.setHttpStatusCode(statusCode);

					if (statusCode == HTTP_STATUS_OK)
					{						
						isSuccessful = true;
					}				
					else
					{
						isSuccessful = false;
						response.setErrorCode(results.mErrorCode);
						response.setErrorMessage(results.mErrorMessage.c_str());
					}
				}
				else
				{
					isSuccessful = false;
					retval = Blaze::FIFAGROUPS_ERR_UNKNOWN;
				}
			}

			delete[] bodyPayLoad;
			return retval;
		}
	} // FifaGroups
} // Blaze

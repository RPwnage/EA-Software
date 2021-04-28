/*************************************************************************************************/
/*!
    \file   createInstanceOp.cpp

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
#include "fifagroupsoperations.h"
#include "fifagroupsconnection.h"
#include "fifagroupshttpresponse.h"

#include "EAJson/JsonWriter.h"

namespace Blaze
{
    namespace FifaGroups
    {
		char8_t* ConstructCreateInstanceBody(CreateInstanceRequest& request, size_t& outBodySize, bool isCreateAndJoin)
		{
			EA::Json::StringWriteStream<eastl::string> jsonStream;
			EA::Json::JsonWriter jsonWriter;
			jsonWriter.SetFormatOption(EA::Json::JsonWriter::FormatOption::kFormatOptionLineEnd, 0);
			jsonWriter.SetStream(&jsonStream);

			if (isCreateAndJoin)
			{
				jsonWriter.BeginObject();
				jsonWriter.BeginObjectValue(JSON_GROUP_TAG);

			}
			jsonWriter.BeginObject();

			jsonWriter.BeginObjectValue(JSON_CREATOR_TAG);
			jsonWriter.Integer(request.getCreatorUserId());
			jsonWriter.BeginObjectValue(JSON_DATE_CREATED_TAG);
			jsonWriter.Integer(0);
			jsonWriter.BeginObjectValue(JSON_SIZE_TAG);
			jsonWriter.Integer(0);
			jsonWriter.BeginObjectValue(JSON_LAST_ACCESS_TAG);
			jsonWriter.Integer(0);

			jsonWriter.BeginObjectValue(JSON_GROUP_NAME_TAG);
			jsonWriter.String(request.getGroupName());
			jsonWriter.BeginObjectValue(JSON_GROUP_TYPE_TAG);
			jsonWriter.String(request.getGroupType());
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

			jsonWriter.BeginObjectValue(JSON_GROUP_SHORT_NAME_TAG);
			jsonWriter.String(request.getGroupShortName());
			jsonWriter.BeginObjectValue(JSON_GROUP_GUID_TAG);
			jsonWriter.String("");

			jsonWriter.BeginObjectValue(JSON_INSTANCE_JOIN_CONFIG_TAG);
			jsonWriter.BeginObject();
			jsonWriter.BeginObjectValue(JSON_HASHED_PWD_TAG);
			jsonWriter.String("");
			jsonWriter.BeginObjectValue(JSON_INVITE_KEY_TAG);
			jsonWriter.String("");
			jsonWriter.BeginObjectValue(JSON_PASSWORD_SHORT_TAG);
			jsonWriter.String("");
			jsonWriter.EndObject();
			jsonWriter.EndObject();


			if (isCreateAndJoin)
			{
				jsonWriter.BeginObjectValue(JSON_GROUP_MEMBERS_TAG);
				jsonWriter.BeginArray();


				Blaze::FifaGroups::MemberList::iterator memberListIter, memberListEnd;
				memberListIter = request.getMembers().begin();
				memberListEnd = request.getMembers().end();

				for (; memberListIter != memberListEnd; memberListIter++)
				{
					eastl::string temp;
					temp.sprintf("%" PRIu64 "", *memberListIter);
					jsonWriter.String(temp.c_str());
				}


				jsonWriter.EndArray();

				jsonWriter.EndObject();
			}


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
		Make a Http call to FifaGroups to create instance.  Example HTTP call below
					
			DELETE https://groups.integration.gameservices.ea.com:10091/group/instance
		===========================================================================================================================*/
		BlazeRpcError CreateInstanceOperation(FifaGroupsConnection& connection, CreateInstanceRequest& request, CreateInstanceResponse& response)
		{
			BlazeRpcError retval = Blaze::ERR_OK;

			uint32_t attemptCount = 0;
			uint32_t maxAttemptCount = connection.getMaxAttempts();
			bool isSuccessful = false;

			// Fetch the Nucleus 2.0 auth token for this user to pass to the achievements service
			Blaze::OAuth::AccessTokenUtil tokenUtil;
			BlazeRpcError error = Blaze::ERR_OK;

			const int tokenScopeLen = 32;
			char8_t tokenScope[tokenScopeLen];
			connection.getTokenScope(tokenScope, tokenScopeLen);

			error = tokenUtil.retrieveServerAccessToken(Blaze::OAuth::TOKEN_TYPE_NONE, tokenScope);
			if (error != Blaze::ERR_OK)
			{
				return Blaze::FIFAGROUPS_ERR_TOKEN_RETRIEVAL_FAILED;
			}

			const int createInstanceUrlLen = 128;
			char8_t createInstanceUrl[createInstanceUrlLen];
				
			// Construct the URL for the HTTP GET request.
			blaze_snzprintf(createInstanceUrl, createInstanceUrlLen, "%s", "");
			connection.addBaseUrl(createInstanceUrl, createInstanceUrlLen);
			bool isCreateAndJoin = request.getMembers().size() > 0;

			if (isCreateAndJoin)
			{
				connection.addCreateAndJoinInstanceAPI(createInstanceUrl, createInstanceUrlLen);
			}
			else
			{
				connection.addCreateInstanceAPI(createInstanceUrl, createInstanceUrlLen);
			}

			//Construct Body
			size_t datasize = 0;
			char8_t* bodyPayLoad = ConstructCreateInstanceBody(request, datasize, isCreateAndJoin);

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
			blaze_snzprintf(httpHeader_UserId, 128, "X-Forwarded-UserId: %" PRIi64 "", gCurrentUserSession->getUserId());	
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
					HttpProtocolUtil::HTTP_POST,
					createInstanceUrl,
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

					if (statusCode == HTTP_STATUS_OK || statusCode == HTTP_STATUS_SUCCESS)
					{
						isSuccessful = true;
						response.setGUId(results.mGUID.c_str());

						int count = results.mAttributes.size();
						response.getAttributes().initVector(count);
						for (int index = 0; index < count; index++)
						{
							response.getAttributes()[index] = response.getAttributes().allocate_element();
							response.getAttributes()[index]->setKey(results.mAttributes[index]->getKey());
							response.getAttributes()[index]->setType(results.mAttributes[index]->getType());
							response.getAttributes()[index]->setIValue(results.mAttributes[index]->getIValue());
							response.getAttributes()[index]->setSValue(results.mAttributes[index]->getSValue());
						}
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

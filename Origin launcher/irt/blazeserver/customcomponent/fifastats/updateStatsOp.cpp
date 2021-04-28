/*************************************************************************************************/
/*!
    \file   updateStatsOp.cpp

    Header: fifastatsoperations.h

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

#include "fifastats/tdf/fifastatstypes.h"
#include "fifastatsslaveimpl.h"
#include "fifastatsoperations.h"
#include "fifastatsconnection.h"
#include "fifastatshttpresponse.h"

#include "EAJson/JsonWriter.h"

namespace Blaze
{
    namespace FifaStats
    {
		char8_t* ConstructUpdateStatsBody(UpdateStatsRequest& request, size_t& outBodySize)
		{
			EA::Json::StringWriteStream<eastl::string> jsonStream;
			EA::Json::JsonWriter jsonWriter;
			jsonWriter.SetFormatOption(EA::Json::JsonWriter::FormatOption::kFormatOptionLineEnd, 0);
			jsonWriter.SetStream(&jsonStream);

			jsonWriter.BeginObject();

			jsonWriter.BeginObjectValue(JSON_UPDATE_ID_TAG);
			jsonWriter.String(request.getUpdateId());

			jsonWriter.BeginObjectValue(JSON_ENTITIES_TAG);
			jsonWriter.BeginObject();

			Blaze::FifaStats::EntityList::iterator iter, end;
			iter = request.getEntities().begin();
			end = request.getEntities().end();

			for (; iter != end; iter++)
			{
				EA::TDF::tdf_ptr<Blaze::FifaStats::Entity> entity = *iter;

				jsonWriter.BeginObjectValue(entity->getEntityId());
				jsonWriter.BeginObject();
				jsonWriter.BeginObjectValue(JSON_ENTITY_ID_TAG);
				jsonWriter.String(entity->getEntityId());
				jsonWriter.BeginObjectValue(JSON_STATS_TAG);
				jsonWriter.BeginObject();

				Blaze::FifaStats::StatsUpdateList::iterator sIter, sEnd;
				sIter = entity->getStatsUpdates().begin();
				sEnd = entity->getStatsUpdates().end();
				for (; sIter != sEnd; sIter++)
				{
					EA::TDF::tdf_ptr<Blaze::FifaStats::StatsUpdate> statsupdate = *sIter;

					jsonWriter.BeginObjectValue(statsupdate->getStatsName());
					jsonWriter.BeginObject();
					jsonWriter.BeginObjectValue(JSON_STATS_VALUE_TAG);

					if (statsupdate->getType() == TYPE_INT)
					{
						jsonWriter.Integer(statsupdate->getStatsIValue());
					}
					else
					{
						jsonWriter.Double(statsupdate->getStatsFValue());
					}
					
					jsonWriter.BeginObjectValue(JSON_STATS_OP_TAG);

					switch (statsupdate->getOperator())
					{
					case Blaze::FifaStats::OPERATOR_SUM:
						jsonWriter.String(JSON_STATS_OP_SUM_TAG);
						break;
					case Blaze::FifaStats::OPERATOR_DECREMENT:
						jsonWriter.String(JSON_STATS_OP_DECREMENT_TAG);
						break;
					case Blaze::FifaStats::OPERATOR_MIN:
						jsonWriter.String(JSON_STATS_OP_MIN_TAG);
						break;
					case Blaze::FifaStats::OPERATOR_MAX:
						jsonWriter.String(JSON_STATS_OP_MAX_TAG);
						break;
					case Blaze::FifaStats::OPERATOR_REPLACE:
						jsonWriter.String(JSON_STATS_OP_REPLACE_TAG);
						break;
					default:
						break;
					}
					jsonWriter.EndObject();
				}
				jsonWriter.EndObject();
				jsonWriter.EndObject();
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
		Make a Http call to FifaStats to update stats.  Example HTTP call below
					
			POST https://internal.stats.int.gameservices.ea.com:11002/api/{api_version}/contexts/{context_id}/categories/{category_id}/entities/stats
		===========================================================================================================================*/

		BlazeRpcError UpdateStatsOperation(FifaStatsConnection& connection, UpdateStatsRequest& request, UpdateStatsResponse& response)
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
				return Blaze::FIFASTATS_ERR_TOKEN_RETRIEVAL_FAILED;
			}

			const int updateStatsUrlLen = 128;
			char8_t updateStatsUrl[updateStatsUrlLen];
				
			// Construct the URL for the HTTP GET request.
			blaze_snzprintf(updateStatsUrl, updateStatsUrlLen, "%s", "");	
			connection.addUpdateStatsAPI(updateStatsUrl, updateStatsUrlLen, request.getCategoryId(), request.getContextOverrideType());
				
			//Construct Body
			size_t datasize = 0;
			char8_t* bodyPayLoad = ConstructUpdateStatsBody(request, datasize);
											
			//Construct headers
			const int headerLen = 128;

			char8_t httpHeader_AuthToken[headerLen];
			//char8_t httpHeader_UserType[headerLen];
			//char8_t httpHeader_UserId[headerLen];
			char8_t httpHeader_ContentType[headerLen];
			char8_t httpHeader_AcceptType[headerLen];

			blaze_snzprintf(httpHeader_AuthToken, headerLen, "Authorization: NEXUS_S2S %s", tokenUtil.getAccessToken());			
			//blaze_snzprintf(httpHeader_UserId, headerLen, "X-Forwarded-UserId: %" PRIi64 "", gCurrentUserSession->getUserId());
			//blaze_strnzcpy(httpHeader_UserType, "X-Forwarded-UserType: NUCLEUS_PERSONA", headerLen);
			blaze_strnzcpy(httpHeader_ContentType, "Content-Type: application/json", headerLen);
			blaze_strnzcpy(httpHeader_AcceptType, "Accept: application/json", headerLen);

			const char8_t* httpHeaders[4];
			httpHeaders[0] = httpHeader_AuthToken;
			//httpHeaders[1] = httpHeader_UserType;
			//httpHeaders[2] = httpHeader_UserId;
			httpHeaders[1] = httpHeader_AcceptType;			

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
					updateStatsUrl,
					httpHeaders,
					2,
					&results,
					httpHeader_ContentType,
					bodyPayLoad,
					datasize
				);
				
				if (err == Blaze::ERR_OK)
				{
					uint32_t httpStatusCode = results.getHttpStatusCode();
					response.setHttpStatusCode(httpStatusCode);

					if (httpStatusCode == HTTP_STATUS_OK)
					{
						isSuccessful = true;
					}
					else
					{
						isSuccessful = false;
						response.setErrorMessage(results.mErrorMessage.c_str());
						response.setErrorCode(results.mErrorCode);
					}				
				}
				else
				{
					isSuccessful = false;
					retval = Blaze::FIFASTATS_ERR_UNKNOWN;
				}
			}
			
			delete[] bodyPayLoad;
			return retval;
		}					
    } // FifaGroups
} // Blaze

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
		/*===========================================================================================================================
		Make a Http call to FifaGroups to rename a group instance.  Example HTTP call below 
					
			PUT https://groups.integration.gameservices.ea.com:10091/group/instance/{group_id}/name?newName={new_name} 
		===========================================================================================================================*/
		BlazeRpcError RenameGroupInstanceOperation(FifaGroupsConnection& connection, const char* serviceName, BlazeId userId, RenameGroupInstanceRequest& request, RenameGroupInstanceResponse& response)
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

			const int renameGroupInstanceUrlLen = 256;
			char8_t renameGroupInstanceUrl[renameGroupInstanceUrlLen];

			// Construct the URL for the HTTP GET request.
			blaze_snzprintf(renameGroupInstanceUrl, renameGroupInstanceUrlLen, "%s", "");
			connection.addBaseUrl(renameGroupInstanceUrl, renameGroupInstanceUrlLen);
			if (request.getIsShortName())
			{
				connection.addRenameGroupInstanceShortNameAPI(renameGroupInstanceUrl, renameGroupInstanceUrlLen, request.getGroupGUId(), request.getNewName());
			}
			else
			{
				connection.addRenameGroupInstanceNameAPI(renameGroupInstanceUrl, renameGroupInstanceUrlLen, request.getGroupGUId(), request.getNewName());
			}
						
			//Construct Body
			size_t datasize = 0;
			char8_t* bodyPayLoad = NULL;

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
					renameGroupInstanceUrl,
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

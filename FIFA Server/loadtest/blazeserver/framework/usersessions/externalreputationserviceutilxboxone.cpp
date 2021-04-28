/*! ************************************************************************************************/
/*!
    \file externalreputationserviceutilxboxone.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/usersessions/externalreputationserviceutilxboxone.h"
#include "framework/usersessions/reputationserviceutilfactory.h"
#include "framework/util/shared/blazestring.h"
#include "framework/connection/outboundhttpservice.h"
#include "xblreputationconfigs/tdf/xblreputationconfigs.h"
#include "xblreputationconfigs/rpc/xblreputationconfigsslave.h"

#include "framework/rpc/oauthslave.h"
#include "framework/tdf/oauth_server.h"


namespace Blaze
{
namespace ReputationService
{

bool ExternalReputationServiceUtilXboxOne::doReputationUpdate(const UserSessionIdList &userSessionIdList, const UserIdentificationList& externalUserList) const
{
    ReputationUpdateMap repUpdateMap;

    UserIdentificationList::const_iterator extUserIter = externalUserList.begin();
    UserIdentificationList::const_iterator extUserEnd = externalUserList.end();
    for (; extUserIter != extUserEnd; ++extUserIter)
    {
        repUpdateMap[(*extUserIter)->getPlatformInfo().getExternalIds().getXblAccountId()];
    }
    
    ExternalXblAccountId validExternalId = INVALID_XBL_ACCOUNT_ID;
    ServiceName validServiceName;
    UserSessionIdList::const_iterator sessionIdIter = userSessionIdList.begin();
    UserSessionIdList::const_iterator sessionIdEnd = userSessionIdList.end();
    for (; sessionIdIter != sessionIdEnd; ++sessionIdIter)
    {
        UserSessionId userSessionId = *sessionIdIter;

        UserInfoData userInfoData;
        UserSessionExtendedData userExtendedData;

        if ((gUserSessionManager->getRemoteUserInfo(userSessionId, userInfoData) != ERR_OK) ||
            (gUserSessionManager->getRemoteUserExtendedData(userSessionId, userExtendedData) != ERR_OK))
        {
            // skip if we can't find the user session
            continue;
        }
        gUserSessionManager->getServiceName(userSessionId, validServiceName);

        BlazeId blazeId = userInfoData.getId();
        ExternalXblAccountId xblId = userInfoData.getPlatformInfo().getExternalIds().getXblAccountId();

        // Take the first external id found, or the external id that corresponds with the gCurrentUserSession (if it's valid)
        if (validExternalId == INVALID_XBL_ACCOUNT_ID || (!gCurrentUserSession || gCurrentUserSession->getPlatformInfo().getExternalIds().getXblAccountId() != validExternalId))
            validExternalId = xblId;

        if (validExternalId == INVALID_XBL_ACCOUNT_ID)
        {
            // skip if we don't have a correct external id.
            continue;
        }

        // grab the external reputation
        UserExtendedDataValue oldReputationValue = gUserSessionManager->getConfig().getReputation().getReputationDefaultValue();
        UserSession::getDataValue(userExtendedData.getDataMap(), UserSessionManager::COMPONENT_ID, EXTENDED_DATA_REPUTATION_VALUE_KEY, oldReputationValue);
        
        ReputationUpdateStruct& repUpdate = repUpdateMap[xblId];
        repUpdate.blazeId = blazeId;
        repUpdate.oldRepValue = oldReputationValue != 0;
        BLAZE_TRACE_LOG(Log::USER, "[ExternalReputationServiceUtilXboxOne].updateReputation: checking reputation for ExtId: " << xblId << " BlazeId: " << blazeId << " OldRep: " << repUpdate.oldRepValue);
    }

    if (validExternalId == INVALID_EXTERNAL_ID)
    {
        BLAZE_WARN_LOG(Log::USER, "[ExternalReputationServiceUtilXboxOne].doReputationUpdate: missing valid user session for reputation lookup.");
        return gUserSessionManager->getConfig().getReputation().getReputationDefaultValue();
    }

    // Send off the batched call for all the external ids, get reputation results for all blaze ids:
    bool overallHasPoorReputation = false;
    BlazeRpcError rpcErr = getBatchReputationByExternalIds(validExternalId, repUpdateMap, overallHasPoorReputation);
    if (rpcErr != ERR_OK)
    {
        overallHasPoorReputation = gUserSessionManager->getConfig().getReputation().getReputationDefaultValue();
        return overallHasPoorReputation;
    }

    // Update the UED data with the results for the users that are logged in
    updateReputations(repUpdateMap);

    return overallHasPoorReputation;
}

BlazeRpcError ExternalReputationServiceUtilXboxOne::getBatchReputationByExternalIds(ExternalId validExternalId, ExternalReputationServiceUtilXboxOne::ReputationUpdateMap& repUpdateMap, bool& overallHasPoorReputation) const
{
    bool foundResult = false;
    overallHasPoorReputation = gUserSessionManager->getConfig().getReputation().getReputationDefaultValue();

    const char8_t* scid = gUserSessionManager->getConfig().getReputation().getScid();
    const char8_t* reputationIdentifier = gUserSessionManager->getConfig().getReputation().getExternalReputationIdentifier();

    Blaze::OAuth::GetUserXblTokenRequest req;
    Blaze::OAuth::GetUserXblTokenResponse rsp;
    req.getRetrieveUsing().setExternalId(validExternalId);

    Blaze::OAuth::OAuthSlave *oAuthSlave = (Blaze::OAuth::OAuthSlave*) gController->getComponent(
        Blaze::OAuth::OAuthSlave::COMPONENT_ID, false, true);
    if (oAuthSlave == nullptr)
    {
        BLAZE_ERR_LOG(Log::USER, "[ExternalReputationServiceUtilXboxOne].getBatchReputationByExternalIds: OAuthSlave was unavailable, reputation lookup for ExternalId: " << validExternalId << " failed.");
        return ERR_SYSTEM;
    }

    // Super permissions are needed to lookup other users' tokens
    UserSession::SuperUserPermissionAutoPtr autoPtr(true);
    BlazeRpcError err = oAuthSlave->getUserXblToken(req, rsp);
    
    // To avoid blocking tests, ignore nucleus mock call errors (already logged)
    if ((err != ERR_OK) && !isMock())
    {
        BLAZE_WARN_LOG(Log::USER, "[ExternalReputationServiceUtilXboxOne].getBatchReputationByExternalIds: Failed to retrieve xbl auth token for ExternalId: " << validExternalId << " from service, for Scid: " << scid << ", with error '" << ErrorHelp::getErrorName(err) <<"'.");
        return err;
    }


    XBLReputation::PostBatchReputationRetrievalRequest getReputationRequest;
    getReputationRequest.getPostBatchReputationRequestHeader().setAuthToken(rsp.getXblToken());

    // Lookup the reputation stat:
    XBLReputation::RequestedScids* scids = getReputationRequest.getPostBatchReputationRequestBody().getRequestedScids().pull_back();
    scids->setScid(scid);
    scids->getRequestedStats().push_back(reputationIdentifier);

    // add all the external ids:
    ReputationUpdateMap::iterator mapIter = repUpdateMap.begin();
    ReputationUpdateMap::iterator mapEnd = repUpdateMap.end();
    for (; mapIter != mapEnd; ++mapIter)
    {
        getReputationRequest.getPostBatchReputationRequestBody().getRequestedUsers().push_back(mapIter->first);
    }


    XBLReputation::PostBatchReputationRetrievalResponse getReputationResponse;
    XBLReputation::XBLReputationConfigsSlave * xblReputationConfigsSlave = (Blaze::XBLReputation::XBLReputationConfigsSlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLReputation::XBLReputationConfigsSlave::COMPONENT_INFO.name);

    if (xblReputationConfigsSlave == nullptr)
    {
        BLAZE_ERR_LOG(Log::USER, "[ExternalReputationServiceUtilXboxOne].getBatchReputationByExternalIds: Failed to instantiate XBLReputationConfigsSlave object. Scid: " << scid << ".");
        return ERR_SYSTEM;
    }

    BLAZE_TRACE_LOG(Log::USER, "[ExternalReputationServiceUtilXboxOne].getBatchReputationByExternalIds: Beginning external reputation lookup. Scid: " << scid << ".");
    // update metrics
    UserSessionsManagerMetrics& metricsObj = gUserSessionManager->getMetricsObj();
    metricsObj.mReputationLookups.increment();

    // We need to run a shorter timeout here, because if the 1st party service is down, we don't want to block so long that the RPC that triggered this update times out on the client
    RpcCallOptions rpcCallOptions;
    rpcCallOptions.timeoutOverride = gUserSessionManager->getConfig().getReputation().getReputationLookupTimeout().getMicroSeconds();
    err = xblReputationConfigsSlave->getBatchReputationRetrieval(getReputationRequest, getReputationResponse, rpcCallOptions);
    if (err == XBLREPUTATION_AUTHENTICATION_REQUIRED)
    {
        BLAZE_TRACE_LOG(Log::USER, "[ExternalReputationServiceUtilXboxOne].getBatchReputationByExternalIds: Authentication with Microsoft failed, token may have expired. Forcing token refresh and retrying.");
        req.setForceRefresh(true);
        err = oAuthSlave->getUserXblToken(req, rsp);
        if (err == ERR_OK)
        {
            getReputationRequest.getPostBatchReputationRequestHeader().setAuthToken(rsp.getXblToken());
            err = xblReputationConfigsSlave->getBatchReputationRetrieval(getReputationRequest, getReputationResponse, rpcCallOptions);
        }
    }
    if (err != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::USER, "[ExternalReputationServiceUtilXboxOne].getBatchReputationByExternalIds: Failed to get external reputation. Scid: " << scid << ", with error '" << ErrorHelp::getErrorName(err) << "'.");
        return err;
    }

    XBLReputation::UsersResponseList::const_iterator userIter = getReputationResponse.getUsers().begin();
    XBLReputation::UsersResponseList::const_iterator userEnd = getReputationResponse.getUsers().end();
    for (; userIter != userEnd; ++userIter)
    {
        ExternalXblAccountId xblId = 0;
        blaze_str2int((*userIter)->getXuid(), &xblId);
        if ((*userIter)->getScids().size() < 1)
        {
            // Missing stats for this user?
            BLAZE_TRACE_LOG(Log::USER, "[ExternalReputationServiceUtilXboxOne].getBatchReputationByExternalIds: Missing external reputation value, '" 
                << reputationIdentifier << "', for xblId: " << xblId << ".");
            continue;
        }

        XBLReputation::ScidResponse& scidResp = *(*userIter)->getScids().front();

        // this should only have one item, but the return object is a list
        XBLReputation::Stats::const_iterator statsIter = scidResp.getStats().begin();
        XBLReputation::Stats::const_iterator statsEnd = scidResp.getStats().end();
        for (; statsIter != statsEnd; ++statsIter)
        {
            const XBLReputation::Stat* stat = *statsIter;
            if ((blaze_stricmp(stat->getStatname(), reputationIdentifier) == 0) && (blaze_stricmp(stat->getType(), "Integer") == 0))
            {
                int64_t externalRepValue; 
                blaze_str2int(stat->getValue(), &externalRepValue);
                bool hasPoorReputation = evaluateReputation(externalRepValue);

                if (hasPoorReputation || !foundResult)
                {
                    overallHasPoorReputation = hasPoorReputation;
                    foundResult = true;
                }

                // Add to the helper map:
                bool foundIdInMap = false;
                ReputationUpdateMap::iterator repMapIter = repUpdateMap.find(xblId);
                if (repMapIter != repUpdateMap.end())
                {
                    repMapIter->second.newRepValue = hasPoorReputation;
                    foundIdInMap = true;
                }

                BLAZE_TRACE_LOG(Log::USER, "[ExternalReputationServiceUtilXboxOne].getBatchReputationByExternalIds: Evaluated external reputation value, '" 
                    << reputationIdentifier << "', for xblId: " << xblId << ", value: " << externalRepValue << " evaluated to: " 
                    <<  (hasPoorReputation ? "true" : "false") << ". " << foundIdInMap);
                // done eval, drop out, even if other stats arrived
                break;
            }
        }
    }

    return ERR_OK; 
}

void ExternalReputationServiceUtilXboxOne::updateReputations(ExternalReputationServiceUtilXboxOne::ReputationUpdateMap& repUpdateMap) const
{ 
    // reputation changed (and the get succeeded), or wasn't yet set, update the UED
    UserExtendedDataKey reputationKey = UED_KEY_FROM_IDS(UserSessionManager::COMPONENT_ID,EXTENDED_DATA_REPUTATION_VALUE_KEY);

    //pack extended data
    UpdateExtendedDataRequest updateReq;
    updateReq.setComponentId(UserSessionManager::COMPONENT_ID);
    updateReq.setIdsAreSessions(false);

    ReputationUpdateMap::iterator mapIter = repUpdateMap.begin();
    ReputationUpdateMap::iterator mapEnd = repUpdateMap.end();
    for (; mapIter != mapEnd; ++mapIter)
    {
        ReputationUpdateStruct& repUpdateStruct = mapIter->second;
        if (repUpdateStruct.oldRepValue == repUpdateStruct.newRepValue || repUpdateStruct.blazeId == INVALID_BLAZE_ID)
            continue;

        UpdateExtendedDataRequest::ExtendedDataUpdate* reputationUpdate = updateReq.getUserOrSessionToUpdateMap().allocate_element();
        (updateReq.getUserOrSessionToUpdateMap())[repUpdateStruct.blazeId] = reputationUpdate;
        UserExtendedDataMap& uedMap = reputationUpdate->getDataMap();
        (uedMap)[reputationKey] = (UserExtendedDataValue)repUpdateStruct.newRepValue;

        BLAZE_TRACE_LOG(Log::USER, "[ExternalReputationServiceUtilXboxOne].updateReputation: updated reputation for ExtId: " << mapIter->first << " BlazeId: " << repUpdateStruct.blazeId << " NewRep: " << repUpdateStruct.newRepValue);
    }

    if (updateReq.getUserOrSessionToUpdateMap().size() > 0)
    {
        // update metric for UED change
        UserSessionsManagerMetrics& metricsObj = gUserSessionManager->getMetricsObj();
        metricsObj.mReputationLookupsWithValueChange.increment();

        // fire and forget, we can't do anything if this fails, and our caller only cares if 1st party retrieval fails.
        BlazeRpcError updateErr = Blaze::ERR_OK;
        updateErr = gUserSessionManager->updateExtendedData(updateReq);
        if (updateErr != Blaze::ERR_OK)
        {
            BLAZE_WARN_LOG(Log::USER, "[ExternalReputationServiceUtilXboxOne].updateReputation: " << (ErrorHelp::getErrorName(updateErr)) << " error updating user extended data");
        }

        BLAZE_TRACE_LOG(Log::USER, "[ExternalReputationServiceUtilXboxOne].updateReputation: updated reputation for " << updateReq.getUserOrSessionToUpdateMap().size() << " user sessions.");
    }
}

// retrieves the external reputation and updates the UED of the given UserSession if the value has changed
Blaze::BlazeRpcError ExternalReputationServiceUtilXboxOne::updateReputation(const UserInfoData &userInfoData, const char8_t* serviceName, UserSessionExtendedData &userExtendedData, bool& hasPoorReputation) const
{ 
    if (userInfoData.getPlatformInfo().getExternalIds().getXblAccountId() == INVALID_XBL_ACCOUNT_ID)
    {
        // non-xbox-one platform users e.g. dedicated servers or commanders in commander mode games, won't update external sessions.
        return ERR_OK;
    }

    ReputationUpdateMap repUpdateMap;

    // grab the external reputation
    UserExtendedDataValue oldReputationValue = gUserSessionManager->getConfig().getReputation().getReputationDefaultValue();
    UserSession::getDataValue(userExtendedData.getDataMap(), UserSessionManager::COMPONENT_ID, EXTENDED_DATA_REPUTATION_VALUE_KEY, oldReputationValue);

    ReputationUpdateStruct& repUpdate = repUpdateMap[userInfoData.getPlatformInfo().getExternalIds().getXblAccountId()];
    repUpdate.blazeId = userInfoData.getId();
    repUpdate.oldRepValue = oldReputationValue != 0;

    BlazeRpcError err = getBatchReputationByExternalIds(userInfoData.getPlatformInfo().getExternalIds().getXblAccountId(), repUpdateMap, hasPoorReputation);

    if (err != ERR_OK)
    {
        hasPoorReputation = gUserSessionManager->getConfig().getReputation().getReputationDefaultValue();
        return err;
    }

    updateReputations(repUpdateMap);

    return err;
}

}
}


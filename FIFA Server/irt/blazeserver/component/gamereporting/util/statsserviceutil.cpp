/*************************************************************************************************/
/*!
    \file   statsserviceutil.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/grpc/outboundgrpcmanager.h"

#include "framework/oauth/accesstokenutil.h"
#include "gamereportingslaveimpl.h"
#include "statsserviceutil.h"

namespace Blaze
{
namespace GameReporting
{
namespace Utilities
{

UpdateStatsServiceKey::UpdateStatsServiceKey()
{
    // intentional no-op constructor because map stores these objects by value, no point constructing them twice
}

UpdateStatsServiceKey::UpdateStatsServiceKey(const char8_t* _context, const char8_t* _category, const char8_t* _entityId)
    : context(_context)
    , category(_category)
    , entityId(_entityId)
{
}

bool UpdateStatsServiceKey::operator < (const UpdateStatsServiceKey& other) const
{
    int32_t result = blaze_strcmp(context, other.context);
    if (result != 0)
        return (result < 0);

    result = blaze_strcmp(category, other.category);
    if (result != 0)
        return (result < 0);

    return blaze_strcmp(entityId, other.entityId) < 0;
}

/*************************************************************************************************/
/*!
    \brief constructor
*/
/*************************************************************************************************/
StatsServiceUtil::StatsServiceUtil(GameReportingSlaveImpl& component, ReportType reportType, const char8_t* reportName)
    : mComponent(component)
    , mReportType(reportType)
    , mReportName(reportName)
    , mOpenRequest(nullptr)
    , mStatsServiceRpcFiberJobQueue("StatsServiceUtil::statsServiceRpc")
    , mRefCount(0)
{
    // the config setting is effectively "per FiberJobQueue" rather than "per slave" -- @todo rename when API change is allowed/convenient
    mStatsServiceRpcFiberJobQueue.setMaximumWorkerFibers(component.getConfig().getMaxConcurrentStatsServiceRpcsPerSlave());

    mComponent.getMetrics().mActiveStatsServiceUpdates.increment();
}

/*************************************************************************************************/
/*!
    \brief destructor

    Should not be called until after all the RPCs maintained by this util have been sent
    and their responses handled.
*/
/*************************************************************************************************/
StatsServiceUtil::~StatsServiceUtil()
{
    mComponent.getMetrics().mActiveStatsServiceUpdates.decrement();
    discardStats();
}

/*************************************************************************************************/
/*!
    \brief startStatRequest

    Select an UpdateEntityStatsRequest object to begin updating stats for

    \param[in]  context - context ID for the request
    \param[in]  category - category ID for the request
    \param[in]  entityId - entity ID for the request
*/
/*************************************************************************************************/
void StatsServiceUtil::startStatRequest(const char8_t* context, const char8_t* category, const char8_t* entityId)
{
    UpdateStatsServiceKey key(context, category, entityId); // key to ensure only unique updates are created
    UpdateStatsServiceMap::const_iterator itr = mUpdateStatsServiceMap.find(key);
    if (itr == mUpdateStatsServiceMap.end())
    {
        // we don't have a cached UpdateEntityStatsRequest* for this key yet, so create it
        mOpenRequest = BLAZE_NEW eadp::stats::UpdateEntityStatsRequest;
        UUID uuid;
        UUIDUtil::generateUUID(uuid);
        mOpenRequest->set_update_id(uuid.c_str());
        mOpenRequest->set_context_id(context);
        mOpenRequest->set_category_id(category);
        mOpenRequest->set_entity_id(entityId);
        key.context = mOpenRequest->context_id().c_str(); // point to memory we own
        key.category = mOpenRequest->category_id().c_str();
        key.entityId = mOpenRequest->entity_id().c_str();
        mUpdateStatsServiceMap[key].request = mOpenRequest;
    }
    else
    {
        // already have this request, so make it current
        mOpenRequest = itr->second.request;
    }
}

/*************************************************************************************************/
/*!
    \brief makeStat

    Create (or modify existing) stat update for the current UpdateEntityStatsRequest object

    \param[in]  name - name of stat to update
    \param[in]  value - value for update
    \param[in]  updateType - operation for update
*/
/*************************************************************************************************/
void StatsServiceUtil::makeStat(const char8_t* name, double value, int32_t updateType)
{
    eadp::stats::UpdateEntityStatsRequest::StatisticValue& statValue = (*mOpenRequest->mutable_stats())[name];
    statValue.mutable_value()->set_value(value);
    statValue.set_operator_((eadp::stats::CollectionOperator) updateType);
}
/*************************************************************************************************/
/*!
    \brief addDimensionListToStat

    Add a dimensional stat list for the current UpdateEntityStatsRequest object, in preparation
    to adding dimensions.

    \param[in]  statName - name of stat to update
    \param[in]  value - value for update (e.g. number of kills for 'kills' stat)
    \param[in]  updateType - operation for update
*/
/*************************************************************************************************/
void StatsServiceUtil::addDimensionListToStat(const char8_t* statName, double value, int32_t updateType)
{
    eadp::stats::DimensionedStatisticList& dimStatListValue = (*mOpenRequest->mutable_dim_stats())[statName];

    eadp::stats::DimensionedStatisticList_DimensionedStatisticValue* dimStatValue = dimStatListValue.add_dimensioned_statistic_values();
    dimStatValue->set_value(value);
    dimStatValue->set_operator_((eadp::stats::CollectionOperator) updateType);
}
/*************************************************************************************************/
/*!
    \brief addDimensionToStatList

    Create (or modify existing) dimensional stat list for the current UpdateEntityStatsRequest
    object, only adding dimensions to an existing list.

    \param[in]  statName - name of the dimensional stat to update
    \param[in]  listIndex - index of the list in the current dimensional stat to be updated
    \param[in]  dimName - dimension to be added
    \param[in]  dimValue - value for the dimension to be added
*/
/*************************************************************************************************/
void StatsServiceUtil::addDimensionToStatList(const char8_t* statName, int listIndex, const char8_t* dimName, const char8_t* dimValue)
{
    eadp::stats::DimensionedStatisticList& dimStatListValue = (*mOpenRequest->mutable_dim_stats())[statName];

    if (listIndex < 0 || listIndex >= dimStatListValue.dimensioned_statistic_values_size())
    {
        ERR_LOG("[StatsServiceUtil].addDimToStatList: index "
            << listIndex << " is not valid. Max index is " << dimStatListValue.dimensioned_statistic_values_size() - 1);
        return;
    }

    eadp::stats::DimensionedStatisticList_DimensionedStatisticValue* currentMap = dimStatListValue.mutable_dimensioned_statistic_values(listIndex);
    (*currentMap->mutable_dimensions())[dimName] = dimValue;
}

/*************************************************************************************************/
/*!
    \brief completeStatRequest

    Stop updating stats for current UpdateEntityStatsRequest object
*/
/*************************************************************************************************/
void StatsServiceUtil::completeStatRequest()
{
    mOpenRequest = nullptr;
}

/*************************************************************************************************/
/*!
    \brief discardStats

    Delete all requests
*/
/*************************************************************************************************/
void StatsServiceUtil::discardStats()
{
    completeStatRequest();

    for (auto& i : mUpdateStatsServiceMap)
    {
        delete i.second.request;
        // the response as unique_ptr will handle its own delete
    }
    mUpdateStatsServiceMap.clear();
}

/*************************************************************************************************/
/*!
    \brief addStatResponse

    Maintain an UpdateEntityStats response with its corresponding request

    \param[in]  context - context ID for the request
    \param[in]  category - category ID for the request
    \param[in]  entityId - entity ID for the request
    \param[in]  responsePtr - response object from the request
*/
/*************************************************************************************************/
void StatsServiceUtil::addStatResponse(const char8_t* context, const char8_t* category, const char8_t* entityId, Grpc::ResponsePtr& responsePtr)
{
    UpdateStatsServiceKey key(context, category, entityId); // key to ensure only unique updates are created
    UpdateStatsServiceMap::iterator itr = mUpdateStatsServiceMap.find(key);
    if (itr == mUpdateStatsServiceMap.end())
    {
        // we should be tracking every request sent by this util, so there should be some investigation to find out what is happening
        ERR_LOG("[StatsServiceUtil].addStatResponse: ignoring response of unknown request for context: "
            << context << ", category: " << category << ", entity: " << entityId);
        return;
    }

    if (itr->second.responsePtr != nullptr)
    {
        // possibly some retry flow caused this...
        WARN_LOG("[StatsServiceUtil].addStatResponse: replacing previous response for context: "
            << context << ", category: " << category << ", entity: " << entityId);
    }
    itr->second.responsePtr = eastl::move(responsePtr);
}

/*************************************************************************************************/
/*!
    \brief getUpdateStatsServiceKey

    Fetch the map key for an existing update stats request/response

    \param[in]  context - context ID for the request
    \param[in]  category - category ID for the request
    \param[in]  entityId - entity ID for the request

    \return - map key for the request (nullptr if request doesn't exist)
*/
/*************************************************************************************************/
const UpdateStatsServiceKey* StatsServiceUtil::getUpdateStatsServiceKey(const char8_t* context, const char8_t* category, const char8_t* entityId)
{
    UpdateStatsServiceKey key(context, category, entityId);
    UpdateStatsServiceMap::const_iterator itr = mUpdateStatsServiceMap.find(key);
    if (itr == mUpdateStatsServiceMap.end())
    {
        return nullptr;
    }
    return &itr->first;
}

eadp::stats::UpdateEntityStatsRequest* StatsServiceUtil::getStatRequest(const char8_t* context, const char8_t* category, const char8_t* entityId)
{
    UpdateStatsServiceKey key(context, category, entityId);
    return getStatRequest(key);
}

eadp::stats::UpdateEntityStatsRequest* StatsServiceUtil::getStatRequest(const UpdateStatsServiceKey& key)
{
    UpdateStatsServiceMap::const_iterator itr = mUpdateStatsServiceMap.find(key);
    if (itr == mUpdateStatsServiceMap.end())
    {
        return nullptr;
    }
    return itr->second.request;
}

eadp::stats::UpdateEntityStatsResponse* StatsServiceUtil::getStatResponse(const char8_t* context, const char8_t* category, const char8_t* entityId)
{
    UpdateStatsServiceKey key(context, category, entityId);
    return getStatResponse(key);
}

eadp::stats::UpdateEntityStatsResponse* StatsServiceUtil::getStatResponse(const UpdateStatsServiceKey& key)
{
    UpdateStatsServiceMap::const_iterator itr = mUpdateStatsServiceMap.find(key);
    if (itr == mUpdateStatsServiceMap.end())
    {
        return nullptr;
    }
    if (itr->second.responsePtr == nullptr)
    {
        return nullptr;
    }
    return static_cast<eadp::stats::UpdateEntityStatsResponse*>(itr->second.responsePtr.get());
}

/*************************************************************************************************/
/*!
    \brief addCustomHeaders

    Add authorization header to a gRPC request

    \param[in]  context - client context for request

    \return - result code (ERR_OK if success)
*/
/*************************************************************************************************/
BlazeRpcError StatsServiceUtil::addCustomHeaders(grpc::ClientContext& context)
{
    UserSession::SuperUserPermissionAutoPtr ptr(true);
    OAuth::AccessTokenUtil tokenUtil;
    BlazeRpcError tokErr = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NEXUS_S2S);
    if (tokErr != ERR_OK)
    {
        WARN_LOG("[StatsServiceUtil].addCustomHeaders: failed to retrieve server access token with error: " << ErrorHelp::getErrorName(tokErr));
        return tokErr;
    }

    context.AddMetadata("authorization", tokenUtil.getAccessToken());

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief sendUpdates

    Send Stats Service updates in a non-blocking fashion via FiberJobQueue
*/
/*************************************************************************************************/
void StatsServiceUtil::sendUpdates()
{
    bool hasUpdate = false;
    uint64_t numUpdatesRemaining = mUpdateStatsServiceMap.size();

    for (const auto& itr : mUpdateStatsServiceMap)
    {
        eadp::stats::UpdateEntityStatsRequest* request = itr.second.request;
        if (request != nullptr && ((request->stats_size() > 0) || (request->dim_stats_size() > 0)))
        {
            hasUpdate = true;

            if (mComponent.getMetrics().mActiveStatsServiceRpcs.get() >= mComponent.getConfig().getMaxQueuedStatsServiceRpcsPerSlave())
            {
                if (TimeValue::getTimeOfDay() >= mComponent.getStatsServiceNextPeriodicLogTime())
                {
                    // a simple mechanism (and the interval doesn't have to be exact) to avoid spamming this log message
                    mComponent.getStatsServiceNextPeriodicLogTime() = TimeValue::getTimeOfDay() + 5 * 1000 * 1000;

                    ERR_LOG("[StatsServiceUtil].sendUpdates: dropping requests because max queue size ("
                        << mComponent.getConfig().getMaxQueuedStatsServiceRpcsPerSlave() << ") reached");
                }

                mComponent.getMetrics().mStatsServiceUpdateQueueFails.increment(numUpdatesRemaining, getReportType(), getReportName());
                break;
            }

            // in this "fire-and-forget" flow, use a fiber job queue maintained by the component because any of these Stats Service requests could outlast this util
            mComponent.getStatsServiceRpcFiberJobQueue().queueFiberJob<StatsServiceUtilPtr, const eadp::stats::UpdateEntityStatsRequest&>
                (&StatsServiceUtil::sendUpdateEntityStats, this, *request, "StatsServiceUtil::sendUpdateEntityStats");
        }

        --numUpdatesRemaining;
    }

    if (hasUpdate)
    {
        mComponent.getMetrics().mGamesWithStatsServiceUpdates.increment(1, getReportType(), getReportName());
    }
}

/*************************************************************************************************/
/*!
    \brief sendUpdatesAndWait

    Send Stats Service updates in a BLOCKING fashion via local FiberJobQueue
*/
/*************************************************************************************************/
BlazeRpcError StatsServiceUtil::sendUpdatesAndWait()
{
    bool hasUpdate = false;

    for (const auto& itr : mUpdateStatsServiceMap)
    {
        eadp::stats::UpdateEntityStatsRequest* request = itr.second.request;
        if (request != nullptr && ((request->stats_size() > 0) || (request->dim_stats_size() > 0)))
        {
            hasUpdate = true;

            // in this BLOCKING flow, use a local fiber job queue
            mStatsServiceRpcFiberJobQueue.queueFiberJob<StatsServiceUtilPtr, const eadp::stats::UpdateEntityStatsRequest&>
                (&StatsServiceUtil::sendUpdateEntityStats, this, *request, "StatsServiceUtil::sendUpdateEntityStats");
        }
    }

    if (hasUpdate)
    {
        mComponent.getMetrics().mGamesWithStatsServiceUpdates.increment(1, getReportType(), getReportName());
    }

    // now wait ...
    BlazeRpcError result = mStatsServiceRpcFiberJobQueue.join();
    if (result != Blaze::ERR_OK)
    {
        mStatsServiceRpcFiberJobQueue.cancelAllWork();
    }
    return result;
}

/*************************************************************************************************/
/*!
    \brief sendUpdateEntityStats

    Helper for sendUpdates to call the blocking UpdateStats RPC

    \param[in]  util - StatsServiceUtil object that maintains the request
    \param[in]  req - UpdateEntityStatsRequest for RPC
*/
/*************************************************************************************************/
void StatsServiceUtil::sendUpdateEntityStats(StatsServiceUtilPtr util, const eadp::stats::UpdateEntityStatsRequest& req)
{
    uint8_t retryCount = 0;

    while (true)
    {
        TRACE_LOG("[StatsServiceUtil:" << util.get() << "].sendUpdateEntityStats: starting for context: "
            << req.context_id().c_str() << ", category: " << req.category_id().c_str() << ", entity: " << req.entity_id().c_str() << ", update id: " << req.update_id().c_str() << ", retry count:" << retryCount);

        Grpc::OutboundRpcBasePtr rpc = CREATE_GRPC_UNARY_RPC("statsService", eadp::stats::EntityStatistics, AsyncUpdateStats);

        if (rpc == nullptr)
        {
            ERR_LOG("[StatsServiceUtil:" << util.get() << "].sendUpdateEntityStats: failed to create RPC for context: "
                << req.context_id().c_str() << ", category: " << req.category_id().c_str() << ", entity: " << req.entity_id().c_str() << ", update id: " << req.update_id().c_str());
            util->getComponent().getMetrics().mStatsServiceUpdateSetupFails.increment(1, util->getReportType(), util->getReportName());
            return; // won't retry
        }

        BlazeRpcError err = addCustomHeaders(rpc->getClientContext());
        if (err != ERR_OK)
        {
            WARN_LOG("[StatsServiceUtil:" << util.get() << "].sendUpdateEntityStats: unable to add headers; ("<< ErrorHelp::getErrorName(err) <<") RPC not sent for context: "
                << req.context_id().c_str() << ", category: " << req.category_id().c_str() << ", entity: " << req.entity_id().c_str() << ", update id: " << req.update_id().c_str());
            util->getComponent().getMetrics().mStatsServiceUpdateSetupFails.increment(1, util->getReportType(), util->getReportName());
            return; // won't retry
        }

        err = rpc->sendRequest(&req);
        if (err != ERR_OK)
        {
            // Unary RPC will fail to send if the RPC is shutting down (as per a restart / reconfig)
            ERR_LOG("[StatsServiceUtil].sendUpdateEntityStats: Failed to send outbound gRPC request with error " << ErrorHelp::getErrorName(err));
            return;
        }

        bool retry = false;
        Grpc::ResponsePtr responsePtr;
        err = rpc->waitForResponse(responsePtr);

        TRACE_LOG("[StatsServiceUtil:" << util.get() << "].sendUpdateEntityStats: RPC response status for context: "
            << req.context_id().c_str() << ", category: " << req.category_id().c_str() << ", entity: " << req.entity_id().c_str() << ", update id: " << req.update_id().c_str() << ", request id: " << rpc->getRequestId().c_str()
            << " ==> code: " << rpc->getStatus().error_code() << ", msg: " << rpc->getStatus().error_message().c_str() << ", details: " << rpc->getStatus().error_details().c_str());

        if (rpc->getStatus().ok() && err == ERR_OK)
        {
            util->getComponent().getMetrics().mStatsServiceUpdateSuccesses.increment(1, util->getReportType(), util->getReportName());

            if (responsePtr != nullptr)
            {
                eadp::stats::UpdateEntityStatsResponse* response = static_cast<eadp::stats::UpdateEntityStatsResponse*>(responsePtr.get());
                TRACE_LOG("[StatsServiceUtil:" << util.get() << "].sendUpdateEntityStats: response is update_id: " << response->update_id().c_str()
                    << ", entity_id: " << response->entity_id().c_str() << ", update id: " << req.update_id().c_str() << ", request id: " << rpc->getRequestId().c_str() << ", commit_id: " << response->commit_id().c_str());

                // keep the success responses
                util->addStatResponse(req.context_id().c_str(), req.category_id().c_str(), req.entity_id().c_str(), responsePtr);
            }
            else
            {
                // check for an error logged by OutboundUnaryRpc::waitForResponseImpl()
                WARN_LOG("[StatsServiceUtil:" << util.get() << "].sendUpdateEntityStats: missing RPC response for context: "
                    << req.context_id().c_str() << ", category: " << req.category_id().c_str() << ", entity: " << req.entity_id().c_str() << ", update id: " << req.update_id().c_str() << ", request id: " << rpc->getRequestId().c_str());
            }
        }
        else
        {
            util->getComponent().getMetrics().mStatsServiceUpdateErrorResponses.increment(1, util->getReportType(), util->getReportName());

            // depending on the error code, we'll retry
            switch (rpc->getStatus().error_code())
            {
            case grpc::UNKNOWN:
            case grpc::ABORTED:
            case grpc::UNAVAILABLE:
                retry = true;
                break;
            default:
                break;
            }
        }

        if (!retry)
            break;

        if (retryCount >= util->getComponent().getConfig().getStatsServiceRpcRetryLimit())
        {
            // don't bother with logging a message if we don't want retries
            if (retryCount != 0)
            {
                WARN_LOG("[StatsServiceUtil:" << util.get() << "].sendUpdateEntityStats: retry limit reached (" << retryCount << "); RPC not sent for context: "
                    << req.context_id().c_str() << ", category: " << req.category_id().c_str() << ", entity: " << req.entity_id().c_str() << ", update id: " << req.update_id().c_str() << ", request id: " << rpc->getRequestId().c_str());
            }
            return;
        }

        ++retryCount;
        util->getComponent().getMetrics().mStatsServiceUpdateRetries.increment(1, util->getReportType(), util->getReportName());
        // retrying, so back to beginning of loop...
    }
}

} //namespace Utilities
} //namespace GameReporting
} //namespace Blaze

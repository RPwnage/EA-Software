/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_COMMANDMETRICS_H
#define BLAZE_COMMANDMETRICS_H

/*** Include files *******************************************************************************/

#include "EATDF/time.h"
#include "framework/connection/endpoint.h"
#include "framework/metrics/metrics.h"
#include "framework/database/dbconn.h"
#include "framework/component/inboundrpccomponentglue.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class CommandMetricInfo;
struct CommandInfo;

struct CommandMetrics
{
    CommandMetrics(const CommandInfo& commandInfo)
        : mCommandInfo(commandInfo)
        , mMetricsCollection(Metrics::gMetricsCollection->getTaglessCollection("rpc").getCollection(Metrics::Tag::component, commandInfo.componentInfo->name).getCollection(Metrics::Tag::command, commandInfo.name))
        , successCount(mMetricsCollection, "rpc.successCount", Metrics::Tag::endpoint, Metrics::Tag::client_type)
        , failCount(mMetricsCollection, "rpc.failCount", Metrics::Tag::rpc_error, Metrics::Tag::endpoint, Metrics::Tag::client_type)
        , timer(mMetricsCollection, "rpc.timer", Metrics::Tag::endpoint, Metrics::Tag::client_type)
        , dbOps(mMetricsCollection, "rpc.dbOps", Metrics::Tag::db_op, Metrics::Tag::endpoint, Metrics::Tag::client_type)
        , dbQueryTime(mMetricsCollection, "rpc.dbQueryTime", Metrics::Tag::endpoint, Metrics::Tag::client_type)
        , dbOnThreadTime(mMetricsCollection, "rpc.dbOnThreadTime", Metrics::Tag::db_thread_stage, Metrics::Tag::endpoint, Metrics::Tag::client_type)
        , fiberTime(mMetricsCollection, "rpc.fiberTime", Metrics::Tag::endpoint, Metrics::Tag::client_type)
        , waitTime(mMetricsCollection, "rpc.waitTime", Metrics::Tag::wait_context, Metrics::Tag::endpoint, Metrics::Tag::client_type)
        , waitCount(mMetricsCollection, "rpc.waitCount", Metrics::Tag::wait_context, Metrics::Tag::endpoint, Metrics::Tag::client_type)
    {
    }

    void fillOutMetricsInfo(CommandMetricInfo& info, const CommandInfo& command) const;


    const CommandInfo& mCommandInfo;
    Metrics::MetricsCollection& mMetricsCollection;

    Metrics::TaggedCounter<Metrics::EndpointName, ClientType> successCount;      // count of ERR_OK returned by RPC
    Metrics::TaggedCounter<BlazeRpcError, Metrics::EndpointName, ClientType> failCount;         // count of error codes other than ERR_OK returned by RPC
    Metrics::TaggedTimer<Metrics::EndpointName, ClientType> timer;
    Metrics::TaggedCounter<DbConnMetrics::DbOp, Metrics::EndpointName, ClientType> dbOps;
    Metrics::TaggedCounter<Metrics::EndpointName, ClientType> dbQueryTime;         // Total time spent executing DB Query on Fiber
    Metrics::TaggedCounter<DbConnMetrics::DbThreadStage, Metrics::EndpointName, ClientType> dbOnThreadTime; // Time spent processing a query on the DB thread
    Metrics::TaggedCounter<Metrics::EndpointName, ClientType> fiberTime;           // Total amount of time, in microseconds, spent executing in a fiber
    Metrics::TaggedCounter<RpcContext::WaitContext, Metrics::EndpointName, ClientType> waitTime;
    Metrics::TaggedCounter<RpcContext::WaitContext, Metrics::EndpointName, ClientType> waitCount;

};

} // Blaze

#endif // BLAZE_COMMANDMETRICS_H


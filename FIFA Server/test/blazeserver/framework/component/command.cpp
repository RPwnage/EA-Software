
#include "framework/blaze.h"
#include "framework/component/command.h"
#include "framework/component/commandmetrics.h"
#include "framework/connection/selector.h"
#include "framework/database/dbconn.h"
#include "framework/system/fiber.h"
#include "framework/system/fibermanager.h"
#include "framework/connection/selector.h"
#include "framework/connection/endpoint.h"
#include "framework/usersessions/usersession.h"
#include "framework/controller/controller.h"
#include "framework/system/debugsys.h"
#include "framework/protocol/shared/decoderfactory.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/component/inboundrpccomponentglue.h"
#include "google/rpc/status.pb.h"
#include "framework/protobuf/protobuftdfconversionhandler.h"
#include "framework/grpc/inboundgrpcjobhandlers.h"


namespace Blaze
{

Command::Command(const CommandInfo& commandInfo, const Message* message)
    : mCommandInfo(commandInfo), mMessage(nullptr), mStartTime(TimeValue::getTimeOfDay()),  
    mSlaveSessionId(SlaveSession::INVALID_SESSION_ID), mUserSessionId(INVALID_USER_SESSION_ID),
    mConnectionUserIndex(0)
{
    if (message != nullptr)
        mMessage = message;
    else if (gRpcContext != nullptr)
        mMessage = &gRpcContext->mMessage;

    if (mMessage != nullptr)
    {
        mSlaveSessionId = mMessage->getSlaveSessionId();
        mUserSessionId = mMessage->getUserSessionId();
        mConnectionUserIndex = mMessage->getFrame().userIndex;
    }
}

Command::~Command()
{
}


BlazeRpcError Command::executeCommand(ClientPlatformType callerPlatform)
{
    BlazeRpcError rc = ERR_OK;

    StringBuilder sb;
    if (BLAZE_IS_LOGGING_ENABLED(getLogCategory(), Logging::T_RPC))
    {
        if (mMessage != nullptr)
            sb.append("%" PRIu32, mMessage->getMsgNum());
        else
            sb.append("%s", "N/A");
    }

    const bool isShuttingDownBefore = gController->isShuttingDown();
    if (EA_UNLIKELY(isShuttingDownBefore))
    {
        BLAZE_ERR_LOG(getLogCategory(), "[component=" << getComponentNameForLogging() << 
            " command=" << getCommandNameForLogging() << " seqno=" << sb.get() <<
            "] beginning execution while this instance is shutting down; any sent notifications will fail.");
    }

    // The DebugSystem is used to globally force RPC errors and timing delays (only for blockable commands)
    BlazeRpcError forcedErrorCode = ERR_OK;
    bool overrideBeforeExecution = false;
    DebugSystem *debugSys = gController->getDebugSys();
    if (EA_UNLIKELY(debugSys != nullptr && debugSys->isEnabled()))
    {
        forcedErrorCode = debugSys->getForcedError(getComponentId(), getCommandId());
        overrideBeforeExecution = debugSys->isOverrideBeforeExecution();

        if (Fiber::isCurrentlyBlockable())
        {
            TimeValue debugDelay = debugSys->getDelayPerCommand(getComponentId(), getCommandId());        
            if (EA_UNLIKELY(debugDelay > 0))
            { 
                BLAZE_TRACE_RPC_LOG(getLogCategory(), "[component=" << getComponentNameForLogging() << 
                    " command=" << getCommandNameForLogging() << " seqno=" << sb.get() <<
                    "] debug delay active, waiting for " << debugDelay.getMillis() << " ms before continuing");

                BlazeRpcError e = gSelector->sleep(debugDelay);
                if (e != ERR_OK)
                {
                    BLAZE_ERR(Log::SYSTEM, "Selector::sleep failed with error %d", e);
                }
            }
        }
    }

    // Check if there's an override error (before executeInternal) for this command.
    if (EA_UNLIKELY(forcedErrorCode != ERR_OK && overrideBeforeExecution))
    {
        rc = forcedErrorCode;

        BLAZE_TRACE_RPC_LOG(getLogCategory(), "[component=" << getComponentNameForLogging() <<
            " command=" << getCommandNameForLogging() << " seqno=" << sb.get() << "] debug sys active, overriding error code (before executeInternal) to " << rc);
    }

    if (rc == ERR_OK)  //did we pass all our preliminary checks?
    {
        if (executeRequestHook(rc))
        {
            rc = static_cast<BlazeRpcError>(executeInternal());
        }
        executeResponseHook(rc);

        if (rc == ERR_OK && callerPlatform != common && getResponse() != nullptr)
        {
            ComponentStub* component = getComponentStub();
            if (component != nullptr)
                component->obfuscatePlatformInfo(callerPlatform, *getResponse());
        }
    }

    // Check if there's an override error (after executeInternal) for this command.
    if (EA_UNLIKELY(forcedErrorCode != ERR_OK && !overrideBeforeExecution))
    {
        rc = forcedErrorCode;

        BLAZE_TRACE_RPC_LOG(getLogCategory(), "[component=" << getComponentNameForLogging() <<
            " command=" << getCommandNameForLogging() << " seqno=" << sb.get() << "] debug sys active, overriding  error code (after executeInternal) to " << rc);
    }

    const bool isShuttingDownAfter = gController->isShuttingDown();
    if (EA_UNLIKELY(isShuttingDownAfter && !isShuttingDownBefore))
    {
        BLAZE_WARN_LOG(getLogCategory(), "[component=" << getComponentNameForLogging() << 
            " command=" << getCommandNameForLogging() << " seqno=" << sb.get() <<
            "] was still executing before this instance began shutting down.");
    }

    if (rc == Blaze::ERR_OK)
    {
        BLAZE_TRACE_RPC_LOG(getLogCategory(), "-> finish [component=" << getComponentNameForLogging() << 
            " command=" << getCommandNameForLogging() << " seqno=" << sb.get() << " ec=ERR_OK]" << 
            ((getResponse() != nullptr) ? "\n" : "") << getResponse());
    } 
    else
    {
        // we hit both log lines so we always get the RPC_TRACE if enabled.
        if (rc == Blaze::ERR_TIMEOUT)
        {
            BLAZE_WARN_LOG(getLogCategory(), "[component=" << getComponentNameForLogging() <<
                " command=" << getCommandNameForLogging() << " timed out" << " ec=" <<
                ErrorHelp::getErrorName(rc) << "(" << SbFormats::HexLower(rc) << ")]");
        }

        BLAZE_TRACE_RPC_LOG(getLogCategory(), "-> finish [component=" << getComponentNameForLogging() << 
            " command=" << getCommandNameForLogging() << " seqno=" << sb.get() << " ec=" << 
            ErrorHelp::getErrorName(rc) << "(" << SbFormats::HexLower(rc) << ")]" <<
            ((getErrorResponse() != nullptr) ? "\n" : "") << getErrorResponse());
    }

    tickMetrics(rc);

    return rc;
}

Blaze::ComponentStub* Command::getComponentStub() const
{
    return ::Blaze::gController->getComponentManager().getLocalComponentStub(getComponentId());
}

void Command::tickMetrics(BlazeRpcError err)
{
    ComponentStub* component = getComponentStub();
    if (component != nullptr)
    {
        component->tickMetrics(getPeerInfo(), mCommandInfo, EA::TDF::TimeValue::getTimeOfDay() - mStartTime, err);
    }
}

void Command::populateGrpcStatusFromErrCode(google::rpc::Status& status, BlazeRpcError err)
{
    Blaze::FixedString128 errorMsg(FixedString128::CtorSprintf(), "%s:%s", ErrorHelp::getErrorName(err), ErrorHelp::getErrorDescription(err));
    status.set_code(err);
    status.set_message(errorMsg.c_str());
}

void Command::convertTdfToAny(const EA::TDF::Tdf* tdf, google::protobuf::Any* any)
{
    Blaze::protobuf::ProtobufTdfConversionHandler::get().convertTdfToAny(tdf, *any);
}

Message::Message(SlaveSessionId slaveSessionId, UserSessionId userSessionId, PeerInfo& peerInfo,
        const RpcProtocol::Frame &frame, RawBufferPtr payload, const google::protobuf::Message* request) 
    : mSlaveSessionId(slaveSessionId)
    , mUserSessionId(userSessionId)
    , mPeerInfo(peerInfo)
    , mFrame(frame)
    , mPayload(payload)
    , mLocale(mFrame.locale != 0 ? mFrame.locale : peerInfo.getLocale())
    , mProtobufRequest(request)
{

}

void CommandMetrics::fillOutMetricsInfo(CommandMetricInfo& info, const CommandInfo& command) const
{
    info.setComponent(command.componentInfo->id);
    info.setComponentName(command.componentInfo->name);
    info.setCommand(command.commandId);
    info.setCommandName(command.name);

    info.setCount(successCount.getTotal() + failCount.getTotal());
    info.setSuccessCount(successCount.getTotal());
    info.setFailCount(failCount.getTotal());
    info.setTotalTime(fiberTime.getTotal() + waitTime.getTotal());
    info.setQueryTime(dbQueryTime.getTotal());

    Metrics::TagValue buf;
    info.setQuerySetupTimeOnThread(dbOnThreadTime.getTotal({ { Metrics::Tag::db_thread_stage, Metrics::Tag::db_thread_stage->getValue(DbConnMetrics::DB_STAGE_SETUP, buf) } }));
    info.setQueryExecutionTimeOnThread(dbOnThreadTime.getTotal({ { Metrics::Tag::db_thread_stage, Metrics::Tag::db_thread_stage->getValue(DbConnMetrics::DB_STAGE_EXECUTE, buf) } }));
    info.setQueryCleanupTimeOnThread(dbOnThreadTime.getTotal({ { Metrics::Tag::db_thread_stage, Metrics::Tag::db_thread_stage->getValue(DbConnMetrics::DB_STAGE_CLEANUP, buf) } }));

    info.setGetConnTime(waitTime.getTotal({ { Metrics::Tag::wait_context, Metrics::Tag::DB_GET_CONN_CONTEXT } }));

    info.setFiberTime(fiberTime.getTotal());

    info.setQueryCount(dbOps.getTotal({ { Metrics::Tag::db_op, Metrics::Tag::db_op->getValue(DbConnMetrics::DB_OP_QUERY, buf) } }));
    info.setTxnCount(dbOps.getTotal({ { Metrics::Tag::db_op, Metrics::Tag::db_op->getValue(DbConnMetrics::DB_OP_TRANSACTION, buf) } }));
    info.setMultiQueryCount(dbOps.getTotal({ { Metrics::Tag::db_op, Metrics::Tag::db_op->getValue(DbConnMetrics::DB_OP_MULTI_QUERY, buf) } }));
    info.setPreparedStatementCount(dbOps.getTotal({ { Metrics::Tag::db_op, Metrics::Tag::db_op->getValue(DbConnMetrics::DB_OP_PREPARED_STATEMENT, buf) } }));

    Blaze::ErrorCountMap& errorCountMap = info.getErrorCountMap();                        
    failCount.iterate([&errorCountMap](const Metrics::TagPairList& tags, const Metrics::Counter& value) {
            const char8_t* err = tags[0].second.c_str();
            errorCountMap[err] += value.getTotal();
            });

    Metrics::TagValue errBuf;
    auto errItr = errorCountMap.find(Metrics::Tag::rpc_error->getValue(ERR_AUTHORIZATION_REQUIRED, errBuf));
    if (errItr != errorCountMap.end())
        info.setAuthorizationFailureCount(errItr->second);
}


} // namespace Blaze


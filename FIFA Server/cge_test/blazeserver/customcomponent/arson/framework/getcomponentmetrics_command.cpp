/*************************************************************************************************/
/*!
    \file   getcomponentmetrics_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getcomponentmetrics_stub.h"

namespace Blaze
{
namespace Arson
{
class GetComponentMetricsCommand : public GetComponentMetricsCommandStub
{
public:
    GetComponentMetricsCommand(
        Message* message, ComponentMetricsRequest* request, ArsonSlaveImpl* componentImpl)
        : GetComponentMetricsCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetComponentMetricsCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetComponentMetricsCommand::Errors execute() override
    {
        
        Blaze::ComponentMetricsRequest request;
        Blaze::ComponentMetricsResponse response;

        request.setCommandId(mRequest.getCommandId());
        request.setComponentId(mRequest.getComponentId());

        //retrieve the component metrics from gController
        BlazeRpcError error = gController->getComponentMetrics(request,response);
        
        if(error == Blaze::ERR_OK)
        {
            EA::TDF::TdfStructVector<Blaze::CommandMetricInfo>::const_iterator it = response.getMetrics().begin();
            EA::TDF::TdfStructVector<Blaze::CommandMetricInfo>::const_iterator itEnd = response.getMetrics().end();

            for(; it != itEnd; ++it)
            {
                // creating Arson equivalent tdf in heap
                Blaze::Arson::CommandMetricInfo* pArsonCommandMetricInfo = mResponse.getMetrics().pull_back();
                
                // NOTE: need to keep this in-sync with the Blaze's server-only equivalent tdf
                // if you want to verify for all of the metrics members
                // source: framework/gen/controllertypes_server.tdf

                // copy Blaze's TDF values into Arson's
                pArsonCommandMetricInfo->setComponent((*it)->getComponent());
                pArsonCommandMetricInfo->setCommand((*it)->getCommand());
                pArsonCommandMetricInfo->setComponentName((*it)->getComponentName());
                pArsonCommandMetricInfo->setCommandName((*it)->getCommandName());
                pArsonCommandMetricInfo->setCount((*it)->getCount());
                pArsonCommandMetricInfo->setSuccessCount((*it)->getSuccessCount());
                pArsonCommandMetricInfo->setFailCount((*it)->getFailCount());
                pArsonCommandMetricInfo->setTotalTime((*it)->getTotalTime());
                pArsonCommandMetricInfo->setGetConnTime((*it)->getGetConnTime());
                pArsonCommandMetricInfo->setTxnCount((*it)->getTxnCount());
                pArsonCommandMetricInfo->setQueryCount((*it)->getQueryCount());
                pArsonCommandMetricInfo->setMultiQueryCount((*it)->getMultiQueryCount());
                pArsonCommandMetricInfo->setPreparedStatementCount((*it)->getPreparedStatementCount());
                pArsonCommandMetricInfo->setQueryTime((*it)->getQueryTime());
                pArsonCommandMetricInfo->setQuerySetupTimeOnThread((*it)->getQuerySetupTimeOnThread());
                pArsonCommandMetricInfo->setQueryExecutionTimeOnThread((*it)->getQueryExecutionTimeOnThread());
                pArsonCommandMetricInfo->setQueryCleanupTimeOnThread((*it)->getQueryCleanupTimeOnThread());
                pArsonCommandMetricInfo->setFiberTime((*it)->getFiberTime());
                // also copy the entire error map
                (*it)->getErrorCountMap().copyInto(pArsonCommandMetricInfo->getErrorCountMap());
            }
        }
        if(error == Blaze::CTRL_ERROR_INVALID_INTERVAL)
            return ARSON_ERR_CTRL_ERROR_INVALID_INTERVAL;
        
            return commandErrorFromBlazeError(error);
    }
};

DEFINE_GETCOMPONENTMETRICS_CREATE();

} //Arson
} //Blaze

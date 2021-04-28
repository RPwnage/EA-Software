/*************************************************************************************************/
/*!
    \file   examplemasterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ExampleMasterImpl

    Example Master implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "examplemasterimpl.h"

// global includes
#include "framework/controller/controller.h"

#include "framework/replication/replicatedmap.h"
#include "framework/replication/replicator.h"

// example includes
#include "example/tdf/exampletypes.h"

namespace Blaze
{
namespace Example
{

// static
ExampleMaster* ExampleMaster::createImpl()
{
    return BLAZE_NEW_NAMED("ExampleMasterImpl") ExampleMasterImpl();
}

/*** Public Methods ******************************************************************************/
ExampleMasterImpl::ExampleMasterImpl()    
{
}

ExampleMasterImpl::~ExampleMasterImpl()
{
}


bool ExampleMasterImpl::onConfigure()
{
    TRACE_LOG("[ExampleMasterImpl].configure: configuring component.");

    return true;
}

PokeMasterError::Error ExampleMasterImpl::processPokeMaster(
    const ExampleRequest &request, ExampleResponse &response, const Message *message)
{
    TRACE_LOG("[ExampleMasterImpl].start() - Num(" << request.getNum() << "), Text(" << request.getText() << ")");

    // Example check for failure.
    if (request.getNum() >= 0)
    {
        INFO_LOG("[ExampleMasterImpl].start() - Respond Success.");

        response.setRegularEnum(ExampleResponse::EXAMPLE_ENUM_SUCCESS);
        char msg[1024] = "";
        blaze_snzprintf(msg, sizeof(msg), "Master got poked: %d, %s", request.getNum(), request.getText());
        response.setMessage(msg);
        ExampleResponse::IntList* l = &response.getMyList();
        l->push_back(1);
        l->push_back(2);
        l->push_back(3);
        l->push_back(4);
        return PokeMasterError::ERR_OK;
    }
    else
    {
        WARN_LOG("[ExampleMasterImpl].start() - Respond Failure");        
        return PokeMasterError::EXAMPLE_ERR_UNKNOWN;
    } // if
}

RpcPassthroughMasterError::Error ExampleMasterImpl::processRpcPassthroughMaster(const Message* message)
{
    Fiber::sleep(50 * 1000, "ExampleMasterImpl::processRpcPassthroughMaster");
    return RpcPassthroughMasterError::ERR_OK;
}

} // Example
} // Blaze

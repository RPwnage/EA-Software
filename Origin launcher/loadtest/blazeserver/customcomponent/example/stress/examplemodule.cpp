/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DummyModule

    This is a sample module to demostrate how to write a stress module.  Each stress instance
    created for this module will continuously issue an RPC to the dummy component on a blaze
    server.  It uses information in the configuration file to determine which RPC to call and
    how long to wait inbetween calls.  It also performs some primitive timing of the RPC calls.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "stressinstance.h"
#include "loginmanager.h"
#include "framework/connection/selector.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"
#include "framework/util/random.h"
#include "examplemodule.h"
#include "example/rpc/exampleslave.h"
#include "framework/tdf/userdefines.h"
#include "example/tdf/exampletypes.h"
#include "time.h"

namespace Blaze
{
namespace Stress
{

ExampleModule::ExampleModule() :
    mTotalWeight(0)
{
    mActionList.push_back(ActionDescriptor("poke", &ExampleInstance::poke));
    mActionList.push_back(ActionDescriptor("rpcPassthrough", &ExampleInstance::rpcPassthrough));
    mActionList.push_back(ActionDescriptor("requestToUserSessionOwner", &ExampleInstance::requestToUserSessionOwner));
}

ExampleModule::~ExampleModule()
{
}

StressInstance* ExampleModule::createInstance(StressConnection* connection, Login* login)
{
    return BLAZE_NEW ExampleInstance(*this, connection, login);
}

bool ExampleModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if (!StressModule::initialize(config, bootUtil)) return false;

    const ConfigMap* actionWeightMap = config.getMap("actionWeights");
    if (actionWeightMap == nullptr)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::example, "ExampleModule.initialize: 'example.actionWeights' section is not found.  All actions will be given the same weight.");
    }

    mTotalWeight = 0;
    for (ActionList::iterator it = mActionList.begin(), end = mActionList.end(); it != end; ++it)
    {
        uint16_t weight = actionWeightMap->getUInt16(it->name, 0);
        if (weight != 0)
        {
            it->weightRangeMin = mTotalWeight + 1;
            mTotalWeight += weight;
            it->weightRangeMax = mTotalWeight;
        }
    }

    return true;
}

BlazeRpcError ExampleInstance::execute()
{
    int32_t rnd = Random::getRandomNumber(getowner().getTotalWeight())+1; // rnd is set to a random number in the range 1..TotalWeight (inclusive)

    for (ExampleModule::ActionList::const_iterator it = getowner().getActionList().begin(), end = getowner().getActionList().end(); it != end; ++it)
    {
        const ExampleModule::ActionDescriptor& action = *it;
        if ((rnd >= action.weightRangeMin) && (rnd <= action.weightRangeMax))
        {
            Fiber::setCurrentContext(action.name);
            return (this->*(action.action))();
        }
    }

    return ERR_OK;
}

BlazeRpcError ExampleInstance::poke()
{
    Example::ExampleRequest req;
    Example::ExampleResponse resp;
    Example::ExampleError errResp;
    BlazeRpcError err = mProxy.poke(req, resp, errResp);
    return err;
}

BlazeRpcError ExampleInstance::rpcPassthrough()
{
    BlazeRpcError err = mProxy.rpcPassthrough();
    return err;
}

BlazeRpcError ExampleInstance::requestToUserSessionOwner()
{
    BlazeRpcError err = mProxy.requestToUserSessionOwner();
    return err;
}

} // Stress
} // Blaze

/*************************************************************************************************/
/*!
    \file   getreconfigurablefeatures.cpp

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
#include "arson/rpc/arsonslave/getreconfigurablefeatures_stub.h"

namespace Blaze
{
namespace Arson
{
class GetReconfigurableFeaturesCommand : public GetReconfigurableFeaturesCommandStub
{
public:
    GetReconfigurableFeaturesCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : GetReconfigurableFeaturesCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~GetReconfigurableFeaturesCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetReconfigurableFeaturesCommandStub::Errors execute() override
    {
        Blaze::ReconfigurableFeatures resp;
        BlazeRpcError err = gController->getConfigFeatureList(resp); //call the slave's getreconfigurablefeatures() which will in turn call the master's getreconfigurablefeatures()

        Blaze::ConfigFeatureList::const_iterator itr =  resp.getFeatures().begin();
        Blaze::ConfigFeatureList::const_iterator endItr =  resp.getFeatures().end();
        for (; itr != endItr; ++itr)
        {
            mResponse.getFeatureNames().push_back(itr->c_str());
        }

        return commandErrorFromBlazeError(err);
    }
};

DEFINE_GETRECONFIGURABLEFEATURES_CREATE()


} //Arson
} //Blaze

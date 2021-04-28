/*************************************************************************************************/
/*!
    \file   reconfigure.cpp

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
#include "arson/rpc/arsonslave/reconfigure_stub.h"

namespace Blaze
{
namespace Arson
{
class ReconfigureCommand : public ReconfigureCommandStub
{
public:
    ReconfigureCommand(Message* message, ReconfigureRequest* request, ArsonSlaveImpl* componentImpl)
        : ReconfigureCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~ReconfigureCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    ReconfigureCommandStub::Errors execute() override
    {
        Blaze::ReconfigureRequest req;
        req.setValidateOnly(mRequest.getValidateOnly());
        req.getFeatures().insert(req.getFeatures().end(), mRequest.getComponents().begin(), mRequest.getComponents().end());

        ReconfigureErrorResponse errorResp;
        BlazeRpcError err = gController->reconfigure(req, errorResp); //call the slave's reconfigure() which will in turn call the master's reconfigure()
        
        if (err == Blaze::ERR_SYSTEM)
        {
            return ARSON_ERR_CONFIG_RELOAD_FAILED;
        }

        if (err == Blaze::CTRL_ERROR_RECONFIGURE_SKIPPED)
        {
            return ARSON_ERR_CONFIG_RECONFIGURE_SKIPPED;
        }

        if (err == Blaze::CTRL_ERROR_RECONFIGURE_IN_PROGRESS)
        {
            return ARSON_ERR_CONFIG_RECONFIGURE_IN_PROGRESS;
        }

        if (!errorResp.getValidationErrorMap().empty())
        {
            // Copy the Blaze error response into the Arson error response
            Blaze::Arson::ReconfigurationFailureMap& arsonReconfigurationFailureMap = mErrorResponse.getReconfigErrors();

            Blaze::ConfigureValidationFailureMap& configureValidationFailureMap = errorResp.getValidationErrorMap();
            Blaze::ConfigureValidationFailureMap::const_iterator iter = configureValidationFailureMap.begin();
            Blaze::ConfigureValidationFailureMap::const_iterator endIter = configureValidationFailureMap.end();
            for (; iter != endIter; ++iter)
            {
                Blaze::Arson::ValidateConfigErrors* arsonValidateConfigErrors = arsonReconfigurationFailureMap.allocate_element();
                iter->second->getErrorMessages().copyInto(arsonValidateConfigErrors->getErrorMessages());
                if (iter->second->getConfigTdf() != nullptr)
                {
                    arsonValidateConfigErrors->setConfigTdf(*(iter->second->getConfigTdf()));
                }

                arsonReconfigurationFailureMap[iter->first] = arsonValidateConfigErrors;
            }

            return ARSON_ERR_CONFIG_VALIDATION_FAILED;
        }

        return commandErrorFromBlazeError(err);
    }
};

DEFINE_RECONFIGURE_CREATE()


} //Arson
} //Blaze

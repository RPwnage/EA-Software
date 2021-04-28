/*************************************************************************************************/
/*!
    \file   fetchcomponentconfiguration_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "blazerpcerrors.h"

#include "framework/tdf/controllertypes_server.h"
#include "framework/rpc/blazecontrollerslave/fetchcomponentconfiguration_stub.h"

#include "framework/controller/controller.h"
#include "framework/system/fibermanager.h"


namespace Blaze
{
    class FetchComponentConfigurationCommand : public FetchComponentConfigurationCommandStub
    {
    public:
        FetchComponentConfigurationCommand(Message *message, FetchComponentConfigurationRequest *request, Controller *controller);
        ~FetchComponentConfigurationCommand() override {}

        FetchComponentConfigurationCommandStub::Errors execute() override;

    private:
        Controller *mController;
    };

    DEFINE_FETCHCOMPONENTCONFIGURATION_CREATE_COMPNAME(Controller);

    FetchComponentConfigurationCommand::FetchComponentConfigurationCommand(Message *message, FetchComponentConfigurationRequest *request, Controller *controller) 
        : FetchComponentConfigurationCommandStub(message, request), mController(controller)
    {
    }

    FetchComponentConfigurationCommandStub::Errors FetchComponentConfigurationCommand::execute()
    {
        if (gController == nullptr)
        {
            return FetchComponentConfigurationCommandStub::ERR_SYSTEM;
        }

        if (mRequest.getFeatures().empty())
        {
            return FetchComponentConfigurationCommandStub::CTRL_ERROR_FEATURE_NAME_INVALID;
        }

        BlazeRpcError result = Blaze::CTRL_ERROR_FEATURE_NAME_INVALID;
        for (ConfigFeatureList::const_iterator i = mRequest.getFeatures().begin(),
             e = mRequest.getFeatures().end(); i != e; ++i)
        {   
            ConfigFeature feature = *i;
            // Already in the response...
            if (mResponse.getConfigTdfs().find(feature) != mResponse.getConfigTdfs().end())
            {
                continue;
            }
            
            ComponentId componentId = Blaze::INVALID_BLAZE_ID;
            EA::TDF::TdfPtr configTdf = nullptr;

            if (blaze_strcmp(feature, "logging") == 0)
            {
                configTdf = BLAZE_NEW LoggingConfig();
            }
            else if (blaze_strcmp(feature, "framework") == 0)
            {
                configTdf = BLAZE_NEW FrameworkConfig();
            }
            else
            {
                // Need to check if blaze supports this componet.
                componentId = BlazeRpcComponentDb::getComponentIdByName(feature);
                if (componentId != Blaze::INVALID_BLAZE_ID && gController->getComponent(componentId, false, true) == nullptr)
                {
                    // Provide valid feature name, but not supported in blaze,
                    // we cannot return ERROR.
                    result = Blaze::ERR_OK;
                    continue;
                }
            }

            if (configTdf != nullptr || componentId != Blaze::INVALID_BLAZE_ID)
            {
                // if the feature is not logging or framework, need to create the configTdf from the factory.
                if (configTdf == nullptr)
                {
                    configTdf = BlazeRpcComponentDb::createConfigTdf(componentId);
                }

                if (configTdf != nullptr)
                {
                    result = gController->getConfigTdf(feature, false, *configTdf);

                    if (result == Blaze::ERR_OK)
                    {
                        EA::TDF::VariableTdfBase* resultTdf = BLAZE_NEW EA::TDF::VariableTdfBase();
                        resultTdf->set(configTdf);
                        mResponse.getConfigTdfs()[feature] = resultTdf;

                        result = Blaze::ERR_OK;
                    }
                }
            }
        }

        return commandErrorFromBlazeError(result);
    }

} // Blaze

/*************************************************************************************************/
/*!
    \file   getcensusdataforcomponents_command.cpp


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
#include "framework/rpc/blazecontrollerslave/getcensusdataforcomponents_stub.h"

#include "framework/controller/controller.h"
#include "framework/component/censusdata.h"


namespace Blaze
{
    class GetCensusDataForComponentsCommand : public GetCensusDataForComponentsCommandStub
    {
    public:
        GetCensusDataForComponentsCommand(Message *message, GetCensusDataForComponentsRequest *request, Controller *controller);
        ~GetCensusDataForComponentsCommand() override {}

        GetCensusDataForComponentsCommandStub::Errors execute() override;

    private:
        Controller *mController;
    };

    DEFINE_GETCENSUSDATAFORCOMPONENTS_CREATE_COMPNAME(Controller);

    GetCensusDataForComponentsCommand::GetCensusDataForComponentsCommand(
        Message *message, 
        GetCensusDataForComponentsRequest *request, 
        Controller *controller) 
        : GetCensusDataForComponentsCommandStub(message, request), mController(controller)
    {
    }

    GetCensusDataForComponentsCommandStub::Errors GetCensusDataForComponentsCommand::execute()
    {
        BlazeRpcError err = gCensusDataManager->populateCensusDataForComponents(mRequest.getComponentIds(), mResponse.getCensusDataByComponent());
        if (err != Blaze::ERR_OK)
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[Controller].processGetCensusDataForComponents(): populateCensusData got error " << (ErrorHelp::getErrorName(err)) 
                        << ", line:" << __LINE__ << "!");
            return commandErrorFromBlazeError(err);
        }
            
        return commandErrorFromBlazeError(err);
    }

} // Blaze

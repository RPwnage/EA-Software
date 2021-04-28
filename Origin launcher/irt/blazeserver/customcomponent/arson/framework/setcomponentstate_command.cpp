/*************************************************************************************************/
/*!
    \file   setcomponentstatecommand.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/component/componentmanager.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/setcomponentstate_stub.h"

namespace Blaze
{
namespace Arson
{
class SetComponentStateCommand : public SetComponentStateCommandStub
{
public:
    SetComponentStateCommand(
        Message* message, SetComponentStateRequest* request, ArsonSlaveImpl* componentImpl)
        : SetComponentStateCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~SetComponentStateCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    SetComponentStateCommandStub::Errors execute() override
    {
        //translate the Arson REQ to Blaze REQ
        Blaze::SetComponentStateRequest blazeREQ;
        translateArsonREQToBlazeREQ(mRequest, blazeREQ);

        //make the call
        BlazeRpcError err = gController->setComponentState(blazeREQ);

        //try to retrieve and set the response's error code
        uint16_t componentId = BlazeRpcComponentDb::getComponentIdByName(blazeREQ.getComponentName());  //resolve the component id from name in req
        ComponentStub* component = gController->getComponentManager().getLocalComponentStub(componentId);
        if (component != nullptr)
        {
            //retrieve and set the response's error code
            mResponse.setDisableErrorReturnCode(component->getDisabledReturnCode());
        }
        else
        {
            //set the error code to be ERR_SYSTEM
            mResponse.setDisableErrorReturnCode(ERR_SYSTEM);
            ERR_LOG("[ArsonSetComponentStateCommand].getComponent: component " << blazeREQ.getComponentName() 
                     << " is not found. DisableErrorReturnCode set to ERR_SYSTEM");
            return ERR_SYSTEM;
        };      

        //return back with the resulting rpc error code
        return commandErrorFromBlazeError(err);     
    }   

    static void translateArsonREQToBlazeREQ(Blaze::Arson::SetComponentStateRequest & inArsonTDF, Blaze::SetComponentStateRequest & outBlazeTDF);
};

DEFINE_SETCOMPONENTSTATE_CREATE()

//translate from arson type into blaze type as Blaze::SetComponentStateRequest is only a server side TDF
//will need to revisit this when the Blaze::SetComponentStateRequest is changed
void SetComponentStateCommand::translateArsonREQToBlazeREQ(Blaze::Arson::SetComponentStateRequest & inArsonTDF, Blaze::SetComponentStateRequest & outBlazeTDF)
{
    outBlazeTDF.setComponentName(inArsonTDF.getComponentName());
    outBlazeTDF.setAction(static_cast<Blaze::SetComponentStateRequest::Action>(inArsonTDF.getAction()));    
    outBlazeTDF.setErrorCode(inArsonTDF.getErrorCode());    
}


} //Arson
} //Blaze

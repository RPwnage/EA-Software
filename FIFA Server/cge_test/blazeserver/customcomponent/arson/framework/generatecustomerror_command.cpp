/*************************************************************************************************/
/*!
    \file   customerrorgenerate_command.cpp


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
#include "arson/rpc/arsonslave/generatecustomerror_stub.h"
#include "arsoncustomerrors.h"
#

namespace Blaze
{
namespace Arson
{
class GenerateCustomErrorCommand : public GenerateCustomErrorCommandStub
{
public:
    GenerateCustomErrorCommand(
        Message* message, Blaze::Arson::GenerateCustomErrorRequest* request, ArsonSlaveImpl* componentImpl)
        : GenerateCustomErrorCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GenerateCustomErrorCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GenerateCustomErrorCommandStub::Errors execute() override
    {
        if(mRequest.getCustomError() == CUSTOM_ERROR_SDK_SERVER)
        {
            return commandErrorFromBlazeError(BLAZE_COMPONENT_CUSTOM_ERROR(Arson::ArsonSlave::COMPONENT_ID, mRequest.getCustomError()));
        }
        else
        {
            return commandErrorFromBlazeError(ARSON_ERR_INVALID_PARAMETER);
        }
    }

};

// static factory method impl
DEFINE_GENERATECUSTOMERROR_CREATE()

} //Arson
} //Blaze

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
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/generatecommandfibererror_stub.h"

namespace Blaze
{
namespace Arson
{
class GenerateCommandFiberErrorCommand : public GenerateCommandFiberErrorCommandStub
{
public:
    GenerateCommandFiberErrorCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : GenerateCommandFiberErrorCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~GenerateCommandFiberErrorCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl *mComponent;

    GenerateCommandFiberErrorCommandStub::Errors execute() override
    {
        return ARSON_ERR_COMMAND_FIBER_ERROR;
    }
};

DEFINE_GENERATECOMMANDFIBERERROR_CREATE()

} //Arson
} //Blaze

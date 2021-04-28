/*************************************************************************************************/
/*!
    \file   slaveexception.cpp

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
#include "arson/rpc/arsonslave/slaveexception_stub.h"

namespace Blaze
{
namespace Arson
{
class SlaveExceptionCommand : public SlaveExceptionCommandStub
{
public:
    SlaveExceptionCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : SlaveExceptionCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~SlaveExceptionCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl *mComponent;

    SlaveExceptionCommandStub::Errors execute() override
    {
        int *p = nullptr;
        *p = 1;

        return ERR_SYSTEM;
    }
};

DEFINE_SLAVEEXCEPTION_CREATE();

} //Arson
} //Blaze

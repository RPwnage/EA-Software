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
#include "arson/rpc/arsonslave/slaveexceptionmemallocator_stub.h"

namespace Blaze
{
namespace Arson
{
class SlaveExceptionMemAllocatorCommand : public SlaveExceptionMemAllocatorCommandStub
{
public:
    SlaveExceptionMemAllocatorCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : SlaveExceptionMemAllocatorCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~SlaveExceptionMemAllocatorCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl *mComponent;

    SlaveExceptionMemAllocatorCommandStub::Errors execute() override
    {
        //GOS-9910
        int *x = BLAZE_NEW int;
        delete x;

        memset(x - 1, 0, sizeof(int) * 2);
        delete x; 

        return ERR_SYSTEM;
    }
};

DEFINE_SLAVEEXCEPTIONMEMALLOCATOR_CREATE();

} //Arson
} //Blaze

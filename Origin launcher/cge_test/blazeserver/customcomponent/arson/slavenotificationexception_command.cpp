/*************************************************************************************************/
/*!
    \file   slavenotificationexception.cpp

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
#include "arson/rpc/arsonslave/slavenotificationexception_stub.h"

namespace Blaze
{
namespace Arson
{
class SlaveNotificationExceptionCommand : public SlaveNotificationExceptionCommandStub
{
public:
    SlaveNotificationExceptionCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : SlaveNotificationExceptionCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~SlaveNotificationExceptionCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl *mComponent;

    SlaveNotificationExceptionCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(mComponent->getMaster()->generateSlaveNotificationException());
    }
};

DEFINE_SLAVENOTIFICATIONEXCEPTION_CREATE();

} //Arson
} //Blaze

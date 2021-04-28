/*************************************************************************************************/
/*!
    \file   masterexception.cpp

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
#include "arson/rpc/arsonslave/masterexception_stub.h"

namespace Blaze
{
namespace Arson
{
class MasterExceptionCommand : public MasterExceptionCommandStub
{
public:
    MasterExceptionCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : MasterExceptionCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~MasterExceptionCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl *mComponent;

    MasterExceptionCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(mComponent->getMaster()->generateMasterException());
    }
};

DEFINE_MASTEREXCEPTION_CREATE();


} //Arson
} //Blaze

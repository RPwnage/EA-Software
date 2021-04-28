/*************************************************************************************************/
/*!
    \file   replicationexception.cpp

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
#include "arson/rpc/arsonslave/replicationexception_stub.h"

namespace Blaze
{
namespace Arson
{
class ReplicationExceptionCommand : public ReplicationExceptionCommandStub
{
public:
    ReplicationExceptionCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : ReplicationExceptionCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~ReplicationExceptionCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl *mComponent;

    ReplicationExceptionCommandStub::Errors execute() override
    {
        ExceptionMapRequest req;
        req.setCollectionId(0);
        Blaze::BlazeRpcError err = mComponent->createMap(req);

        // Cleanup
        mComponent->destroyMap(req);

        return commandErrorFromBlazeError(err);
    }
};

DEFINE_REPLICATIONEXCEPTION_CREATE();

} //Arson
} //Blaze

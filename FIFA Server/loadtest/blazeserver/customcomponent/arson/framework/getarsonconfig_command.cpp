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
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getarsonconfig_stub.h"

namespace Blaze
{
namespace Arson
{
class GetArsonConfigCommand : public GetArsonConfigCommandStub
{
public:
    GetArsonConfigCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : GetArsonConfigCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~GetArsonConfigCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetArsonConfigCommandStub::Errors execute() override
    {
        mComponent->getConfig().copyInto(mResponse.getConfig());

        return ERR_OK;
    }
};

DEFINE_GETARSONCONFIG_CREATE()

} //Arson
} //Blaze

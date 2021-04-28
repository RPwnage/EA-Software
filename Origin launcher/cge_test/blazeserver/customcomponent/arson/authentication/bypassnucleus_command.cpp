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
#include "arson/rpc/arsonslave/bypassnucleus_stub.h"

#include "authentication/authenticationimpl.h"

namespace Blaze
{
namespace Arson
{
class BypassNucleusCommand : public BypassNucleusCommandStub
{
public:
    BypassNucleusCommand(Message* message, BypassNucleusRequest* request, ArsonSlaveImpl* componentImpl)
        : BypassNucleusCommandStub(message, request),
        mComponent(componentImpl)
    {
        mAuthComponent = static_cast<Blaze::Authentication::AuthenticationSlaveImpl*>(
            gController->getComponent(Authentication::AuthenticationSlave::COMPONENT_ID));
    }

    ~BypassNucleusCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl *mComponent;
    Authentication::AuthenticationSlaveImpl *mAuthComponent;

    BypassNucleusCommandStub::Errors execute() override
    {
        mAuthComponent->setBypassNucleus(mRequest.getBypass());

        return ERR_OK;
    }
};

DEFINE_BYPASSNUCLEUS_CREATE()

} //Arson
} //Blaze

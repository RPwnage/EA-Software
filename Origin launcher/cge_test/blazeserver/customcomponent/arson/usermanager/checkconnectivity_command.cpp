/*************************************************************************************************/
/*!
    \file   checkconnectivity_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/usersessions/usersessionmanager.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/checkconnectivity_stub.h"

namespace Blaze
{
namespace Arson
{

class CheckConnectivityCommand : public CheckConnectivityCommandStub
{
public:
    CheckConnectivityCommand(Message* message, Blaze::Arson::CheckConnectivityREQ* request, ArsonSlaveImpl* componentImpl)
        : CheckConnectivityCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~CheckConnectivityCommand() override
    {
    }

private:
    CheckConnectivityCommandStub::Errors execute() override
    {
        // CAVEAT:  This test RPC only calls the UserSessionManager's util, so we'll only know if the session is
        // connected (true/ERR_OK) or not (false/ERR_SYSTEM).  Any underlying reason for the session failing its
        // connectivity check is not known here.
        if (!gUserSessionManager->checkConnectivity(mRequest.getUserSessionId(), mRequest.getConnectivityTimeout()))
            return ERR_SYSTEM;

        return ERR_OK;
    }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;
};

DEFINE_CHECKCONNECTIVITY_CREATE()

} //Arson
} //Blaze

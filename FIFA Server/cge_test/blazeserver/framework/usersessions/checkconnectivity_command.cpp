/*************************************************************************************************/
/*!
    \file   checkconnectivity_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class CheckConnectivityCommand 

    Internal RPC to check a local session's connectivity to the client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/checkconnectivity_stub.h"
#include "framework/tdf/usersessiontypes_server.h"

namespace Blaze
{

class CheckConnectivityCommand : public CheckConnectivityCommandStub
{
public:
    CheckConnectivityCommand(Message* message, Blaze::CheckConnectivityRequest* request, UserSessionsSlave* componentImpl)
        : CheckConnectivityCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~CheckConnectivityCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    CheckConnectivityCommandStub::Errors execute() override
    {
        UserSessionMasterPtr session = gUserSessionMaster->getLocalSession(mRequest.getUserSessionId());
        if (session == nullptr || session->getPeerInfo() == nullptr)
        {
            BLAZE_WARN_LOG(Log::USER, "[CheckConnectivityCommand] no session or peer for id " << mRequest.getUserSessionId());
            return ERR_SYSTEM;
        }

        BlazeRpcError err = session->getPeerInfo()->checkConnectivity(TimeValue::getTimeOfDay() + mRequest.getConnectivityTimeout());
        mResponse.setConnectivityResult((uint32_t) err);

        return ERR_OK;
    }

private:
    UserSessionsSlave* mComponent; // memory owned by creator, don't free
};

// static factory method impl
DEFINE_CHECKCONNECTIVITY_CREATE_COMPNAME(UserSessionManager)

} // Blaze

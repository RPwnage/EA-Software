/*************************************************************************************************/
/*!
    \file   GetClientVersion_command.cpp


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
#include "arson/rpc/arsonslave/getclientversion_stub.h"

namespace Blaze
{
namespace Arson
{

class GetClientVersionCommand : public GetClientVersionCommandStub
{
public:
    GetClientVersionCommand(Message* message, Blaze::Arson::GetClientVersionREQ* request, ArsonSlaveImpl* componentImpl)
        : GetClientVersionCommandStub(message, request)
    {
    }

    ~GetClientVersionCommand() override
    {
    }

private:
    GetClientVersionCommandStub::Errors execute() override
    {
       mResponse.setClientVersion(gUserSessionManager->getClientVersion(mRequest.getUserSessionId()));
       return ERR_OK;
    }
};

DEFINE_GETCLIENTVERSION_CREATE()

} //Arson
} //Blaze

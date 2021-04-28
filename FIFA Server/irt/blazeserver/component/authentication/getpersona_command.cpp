/*************************************************************************************************/
/*!
    \file   getaccount_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetAccountCommand

    Gets the Account details.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/getpersona_stub.h"
#include "authentication/tdf/authentication.h"

#include "authentication/util//getpersonautil.h"

namespace Blaze
{
namespace Authentication
{

class GetPersonaCommand : public GetPersonaCommandStub
{
public:
    GetPersonaCommand(Message* message, AuthenticationSlaveImpl* componentImpl)
      : GetPersonaCommandStub(message),
        mComponent(componentImpl),
        mGetPersonaUtil(getPeerInfo())
    {
    }

    ~GetPersonaCommand() override {};

/* Private methods *******************************************************************************/
private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    GetPersonaUtil mGetPersonaUtil;

    // States
    GetPersonaCommandStub::Errors execute() override
    {
        BlazeRpcError err = mGetPersonaUtil.getPersona(gCurrentUserSession->getBlazeId(), &mResponse.getPersonaInfo());
        if (err != Blaze::ERR_OK)
        {
            return (commandErrorFromBlazeError(err));
        }

        // Populate results.
        mResponse.setAccountId(gCurrentUserSession->getAccountId());

        // Return results.
        return ERR_OK;
    }
};

DEFINE_GETPERSONA_CREATE();

} // Authentication
} // Blaze

/*************************************************************************************************/
/*!
    \file   listpersonas_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ListPersonasCommand

    List all the Nucleus personas for an account.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/listpersonas_stub.h"
#include "authentication/util/getpersonautil.h"
#include "authentication/util/authenticationutil.h"
#include "framework/usersessions/usersession.h"
#include "framework/controller/controller.h"


namespace Blaze
{
namespace Authentication
{

class ListPersonasCommand : public ListPersonasCommandStub
{
public:

    ListPersonasCommand(Message* message, AuthenticationSlaveImpl* componentImpl)
        : ListPersonasCommandStub(message),
        mComponent(componentImpl),
        mGetPersona(getPeerInfo())
    {
    }

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    GetPersonaUtil mGetPersona;

/* Private methods *******************************************************************************/
    ListPersonasCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(mGetPersona.getPersonaList(gCurrentUserSession->getAccountId(),
            gCurrentUserSession->getPersonaNamespace(), mResponse.getList()));
    }

};
DEFINE_LISTPERSONAS_CREATE()

} // Authentication
} // Blaze

/**************************************************************************************************/
/*! 
    \file expresslogin_command.cpp
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

/*** Include Files ********************************************************************************/
#include "framework/blaze.h"
#include "authentication/rpc/authenticationslave/createwalusersession_stub.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/usersessions/usersessionmanager.h"
#include "authentication/authenticationimpl.h"

#include "framework/usersessions/usersessionmanager.h"
#include "framework/controller/controller.h"

namespace Blaze
{
namespace Authentication
{

/*** Public Methods *******************************************************************************/


/*! ***********************************************************************************************/
/*! \class CreateWalUserSessionCommand

\brief Uses other session key to replicate a new session for the web
***************************************************************************************************/
class CreateWalUserSessionCommand : public CreateWalUserSessionCommandStub
{
public:

    CreateWalUserSessionCommand(Message* message, AuthenticationSlaveImpl* componentImpl)
    :   CreateWalUserSessionCommandStub(message),
        mComponent(componentImpl)
    {
    }

/*** Private Methods ******************************************************************************/
private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

private:
    CreateWalUserSessionCommandStub::Errors execute() override
    {
        bool geoIpSucceeded = true;
        UserSessionMasterPtr clonedSession;
        BlazeRpcError loginErr = gUserSessionMaster->createWalUserSession(*gCurrentLocalUserSession, clonedSession, geoIpSucceeded);

        if (loginErr == ERR_OK)
        {
            char8_t sessionKey[MAX_SESSION_KEY_LENGTH];
            clonedSession->getKey(sessionKey);

            // fill out response
            mResponse.setSessionKey(sessionKey);
            mResponse.setBlazeId(clonedSession->getBlazeId());
            clonedSession->getPlatformInfo().copyInto(mResponse.getPlatformInfo());
            mResponse.setAccountId(mResponse.getPlatformInfo().getEaIds().getNucleusAccountId());
            mResponse.getPersonaDetails().setDisplayName(clonedSession->getUserInfo().getPersonaName());
            mResponse.getPersonaDetails().setPersonaId(clonedSession->getBlazeId()); // PersonaId == BlazeId in 3.0
            mResponse.setGeoIpSucceeded(geoIpSucceeded);
        }

        return commandErrorFromBlazeError(loginErr);
    }

};
DEFINE_CREATEWALUSERSESSION_CREATE()

} // Authentication
} // Blaze

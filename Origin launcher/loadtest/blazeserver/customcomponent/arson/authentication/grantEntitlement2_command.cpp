/*************************************************************************************************/
/*!
    \file   setgamesettings_command.cpp

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
#include "arson/rpc/arsonslave/grantentitlement2_stub.h"
#include "authentication/rpc/authenticationslave.h"


namespace Blaze
{
namespace Arson
{
    class GrantEntitlement2Command : public GrantEntitlement2CommandStub
{
public:
    GrantEntitlement2Command(
        Message* message, Blaze::Arson::GrantEntitlement2Request* request, ArsonSlaveImpl* componentImpl)
        : GrantEntitlement2CommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GrantEntitlement2Command() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GrantEntitlement2Command::Errors execute() override
    {
      //  BlazeRpcError error;
        Authentication::AuthenticationSlave* component = 
            static_cast<Authentication::AuthenticationSlave*>(gController->getComponent(Authentication::AuthenticationSlave::COMPONENT_ID));    
        if (component == nullptr)
        {
            WARN_LOG("[GrantEntitlement2Stub].execute() - authentication component is not available");
            return ERR_SYSTEM;
        }
       

        if(mRequest.getNullCurrentUserSession())
        {
            // Set the gCurrentUserSession to nullptr
            UserSession::setCurrentUserSessionId(INVALID_USER_SESSION_ID);
        }

        {
            UserSession::SuperUserPermissionAutoPtr autoPtr(true);
            BlazeRpcError err = component->grantEntitlement2(mRequest.getGrantEntitlement2Request(),mResponse);       
            return arsonErrorFromAuthenticationError(err);
        }
    }
    static Errors arsonErrorFromAuthenticationError(BlazeRpcError error);
};

DEFINE_GRANTENTITLEMENT2_CREATE()

GrantEntitlement2Command::Errors GrantEntitlement2Command::arsonErrorFromAuthenticationError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case ERR_AUTHENTICATION_REQUIRED: result = ERR_AUTHENTICATION_REQUIRED; break;
        case AUTH_ERR_INVALID_USER: result = ARSON_ERR_INVALID_USER; break;
        case AUTH_ERR_GROUPNAME_REQUIRED: result = ARSON_ERR_GROUPNAME_REQUIRED; break;
        case AUTH_ERR_ENTITLEMENT_TAG_REQUIRED: result = ARSON_ERR_ENTITLEMENT_TAG_REQUIRED; break;
        case AUTH_ERR_WHITELIST: result = ARSON_ERR_WHITELIST; break;
        case AUTH_ERR_LINK_PERSONA: result = ARSON_ERR_LINK_PERSONA; break;
        
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[GrantEntitlement2Command].arsonErrorFromClubsError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}
} //Arson
} //Blaze

/*************************************************************************************************/
/*!
    \file   lookupentitybyid_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// framework includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/lookupentitybyid_stub.h"

namespace Blaze
{
namespace Arson
{
class LookupEntityByIdCommand : public LookupEntityByIdCommandStub
{
public:
    LookupEntityByIdCommand(
        Message* message, Blaze::EntityLookupByIdRequest* request, ArsonSlaveImpl* componentImpl)
        : LookupEntityByIdCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~LookupEntityByIdCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    LookupEntityByIdCommandStub::Errors execute() override
    {
        
        UserSessionsSlave * userSessionsComponent = nullptr;
        userSessionsComponent = static_cast<UserSessionsSlave*>(gController->getComponent(UserSessionsSlave::COMPONENT_ID));
        
        if (userSessionsComponent == nullptr)
        {
            WARN_LOG("userSessionsComponent.execute() - usersessions component is not available");
            return ERR_SYSTEM;
        }

        BlazeRpcError err = userSessionsComponent->lookupEntityById(mRequest, mResponse);
        
        return arsonErrorFromLookupError(err);
    }

    static Errors arsonErrorFromLookupError(BlazeRpcError error);
};

LookupEntityByIdCommandStub* LookupEntityByIdCommandStub::create(Message *msg, Blaze::EntityLookupByIdRequest* request, ArsonSlave *component)
{
    return BLAZE_NEW LookupEntityByIdCommand(msg, request, static_cast<ArsonSlaveImpl*>(component));
}

LookupEntityByIdCommandStub::Errors LookupEntityByIdCommand::arsonErrorFromLookupError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case ERR_ENTITY_TYPE_NOT_FOUND : result = ARSON_ERR_ERR_ENTITY_TYPE_NOT_FOUND; break;
        case ERR_ENTITY_NOT_FOUND : result = ARSON_ERR_ERR_ENTITY_NOT_FOUND; break;
        case ERR_NOT_SUPPORTED : result = ARSON_ERR_ERR_NOT_SUPPORTED; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[LookupEntityByIdCommand].arsonErrorFromLookupError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze

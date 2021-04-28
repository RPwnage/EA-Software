/*************************************************************************************************/
/*!
    \file   LookupEntitiesByIds_command.cpp


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
#include "arson/rpc/arsonslave/lookupentitiesbyids_stub.h"

namespace Blaze
{
namespace Arson
{
class LookupEntitiesByIdsCommand : public LookupEntitiesByIdsCommandStub
{
public:
    LookupEntitiesByIdsCommand(
        Message* message, Blaze::EntitiesLookupByIdsRequest* request, ArsonSlaveImpl* componentImpl)
        : LookupEntitiesByIdsCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~LookupEntitiesByIdsCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    LookupEntitiesByIdsCommandStub::Errors execute() override
    {
        
        UserSessionsSlave * userSessionsComponent = nullptr;
        userSessionsComponent = static_cast<UserSessionsSlave*>(gController->getComponent(UserSessionsSlave::COMPONENT_ID));
        
        if (userSessionsComponent == nullptr)
        {
            WARN_LOG("userSessionsComponent.execute() - usersessions component is not available");
            return ERR_SYSTEM;
        }

        BlazeRpcError err = userSessionsComponent->lookupEntitiesByIds(mRequest, mResponse);
        
        return arsonErrorFromLookupError(err);
    }

    static Errors arsonErrorFromLookupError(BlazeRpcError error);
};

LookupEntitiesByIdsCommandStub* LookupEntitiesByIdsCommandStub::create(Message *msg, Blaze::EntitiesLookupByIdsRequest* request, ArsonSlave *component)
{
    return BLAZE_NEW LookupEntitiesByIdsCommand(msg, request, static_cast<ArsonSlaveImpl*>(component));
}

LookupEntitiesByIdsCommandStub::Errors LookupEntitiesByIdsCommand::arsonErrorFromLookupError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case ERR_ENTITY_TYPE_NOT_FOUND : result = ARSON_ERR_ERR_ENTITY_TYPE_NOT_FOUND; break;
        case ERR_NOT_SUPPORTED : result = ARSON_ERR_ERR_NOT_SUPPORTED; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[LookupEntitiesByIdsCommand].arsonErrorFromLookupError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze

/*************************************************************************************************/
/*!
    \file   lookupentitybyname_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \classlookupEntityByNameCommand 

    process user session extended data retrieval requests from the client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/rpc/usersessionsslave/lookupentitybyname_stub.h"
#include "framework/identity/identity.h"

namespace Blaze
{

    class LookupEntityByNameCommand : public LookupEntityByNameCommandStub
    {
    public:

       LookupEntityByNameCommand(Message* message, Blaze::EntityLookupRequest* request, UserSessionsSlave* componentImpl)
            :LookupEntityByNameCommandStub(message, request),
              mComponent(componentImpl)
        {
        }

        ~LookupEntityByNameCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

       LookupEntityByNameCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[LookupEntityByNameCommand].start()");

            EA::TDF::ObjectType entityType;
            const char8_t* entityTypeName = mRequest.getEntityTypeName();
            if (entityTypeName != nullptr)
            {
                entityType = BlazeRpcComponentDb::getBlazeObjectTypeByName(entityTypeName);
            }
            else
            {
                entityType = EA::TDF::OBJECT_TYPE_INVALID;
            }

            if (entityType == EA::TDF::OBJECT_TYPE_INVALID)
                return ERR_ENTITY_TYPE_NOT_FOUND;

            IdentityInfo result;
            BlazeRpcError err = gIdentityManager->getIdentityByName(entityType, mRequest.getEntityName(), result);

            if (err == Blaze::ERR_OK)            
                mResponse.setBlazeObjectId(result.getBlazeObjectId());
        
            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_LOOKUPENTITYBYNAME_CREATE_COMPNAME(UserSessionsSlaveStub)

} // Blaze

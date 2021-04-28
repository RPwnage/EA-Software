/*************************************************************************************************/
/*!
    \file   lookupentitybyid_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \classlookupEntityByIdCommand

    Lookup the name associated with an entity id.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/lookupentitybyid_stub.h"

namespace Blaze
{

    class LookupEntityByIdCommand : public LookupEntityByIdCommandStub
    {
    public:

       LookupEntityByIdCommand(Message* message, Blaze::EntityLookupByIdRequest* request, UserSessionsSlave* componentImpl)
            :LookupEntityByIdCommandStub(message, request),
              mComponent(componentImpl)
        {
        }

        ~LookupEntityByIdCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

       LookupEntityByIdCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[LookupEntityByIdCommand].start()");

            IdentityInfo result;
            BlazeRpcError err = gIdentityManager->getIdentity(
                mRequest.getBlazeObjectType(), mRequest.getEntityId(), result);

            if (err == Blaze::ERR_OK)
                mResponse.setEntityName(result.getIdentityName());
        
            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    
    DEFINE_LOOKUPENTITYBYID_CREATE_COMPNAME(UserSessionManager)
   


} // Blaze

/*************************************************************************************************/
/*!
    \file   lookupentitiesbyids_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \classlookupEntitiesByIdsCommand

    Lookup the names associated with entity ids.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/lookupentitiesbyids_stub.h"

namespace Blaze
{

    class LookupEntitiesByIdsCommand : public LookupEntitiesByIdsCommandStub
    {
    public:

       LookupEntitiesByIdsCommand(Message* message, Blaze::EntitiesLookupByIdsRequest* request, UserSessionsSlave* componentImpl)
            :LookupEntitiesByIdsCommandStub(message, request),
              mComponent(componentImpl)
        {
        }

        ~LookupEntitiesByIdsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

       LookupEntitiesByIdsCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[LookupEntitiesByIdsCommand].start()");

            IdentityInfoByEntityIdMap results;
            BlazeRpcError err = gIdentityManager->getIdentities(mRequest.getBlazeObjectType(), mRequest.getEntityIds().asVector(), results);

            if (err == Blaze::ERR_OK)
            {
                EntityIds::const_iterator it = mRequest.getEntityIds().begin();
                while ( it != mRequest.getEntityIds().end() )
                {
                    IdentityInfoByEntityIdMap::const_iterator idIt = results.find(*it++);
                    mResponse.getEntityNames().push_back(idIt == results.end() ? "" : idIt->second->getIdentityName());
                }
            }
        
            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_LOOKUPENTITIESBYIDS_CREATE_COMPNAME(UserSessionManager)

} // Blaze

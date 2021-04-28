/*************************************************************************************************/
/*!
    \file   getidentities_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetIdentitiesCommand

    resolve identity lookup requests from client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/identity/identity.h"
#include "framework/rpc/identityslave/getidentities_stub.h"
namespace Blaze
{
    class GetIdentitiesCommand : public GetIdentitiesCommandStub
    {
    public:

        GetIdentitiesCommand(Message* message, Blaze::IdentitiesRequest* request, IdentityManager* componentImpl)
            : GetIdentitiesCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~GetIdentitiesCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetIdentitiesCommandStub::Errors execute() override
        {
            return commandErrorFromBlazeError(mComponent->getLocalIdentities(mRequest.getBlazeObjectType(), mRequest.getEntityIds().asVector(), mResponse.getIdentityInfoByEntityIdMap()));
        }

    private:
        IdentityManager* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_GETIDENTITIES_CREATE_COMPNAME(IdentityManager)

} // Blaze

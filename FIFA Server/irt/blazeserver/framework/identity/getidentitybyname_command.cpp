/*************************************************************************************************/
/*!
    \file   getidentitybyname_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetIdentityByNameCommand

    resolve identity lookup requests from client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

#include "framework/identity/identity.h"
#include "framework/rpc/identityslave/getidentitybyname_stub.h"
namespace Blaze
{
    class GetIdentityByNameCommand : public GetIdentityByNameCommandStub
    {
    public:

        GetIdentityByNameCommand(Message* message, Blaze::IdentityByNameRequest* request, IdentityManager* componentImpl)
            : GetIdentityByNameCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~GetIdentityByNameCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetIdentityByNameCommandStub::Errors execute() override
        {
            return commandErrorFromBlazeError(mComponent->getLocalIdentityByName(mRequest.getBlazeObjectType(), mRequest.getEntityName(), mResponse));
        }

    private:
        IdentityManager* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_GETIDENTITYBYNAME_CREATE_COMPNAME(IdentityManager)

} // Blaze

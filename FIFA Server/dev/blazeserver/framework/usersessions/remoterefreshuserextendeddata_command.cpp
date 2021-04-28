/*************************************************************************************************/
/*!
    \file   remoterefreshuserextendeddata_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RemoteRefreshUserExtendedDataCommand

    Lookup the name associated with an entity id.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/remoterefreshuserextendeddata_stub.h"

namespace Blaze
{

    class RemoteRefreshUserExtendedDataCommand : public RemoteRefreshUserExtendedDataCommandStub
    {
    public:

       RemoteRefreshUserExtendedDataCommand(Message* message, Blaze::RemoteUserExtendedDataUpdateRequest* request, UserSessionsSlave* componentImpl)
            :RemoteRefreshUserExtendedDataCommandStub(message, request),
              mComponent(componentImpl)
        {
        }

        ~RemoteRefreshUserExtendedDataCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        RemoteRefreshUserExtendedDataCommand::Errors execute() override
        {
            UserSessionManager *manager = static_cast<UserSessionManager*>(mComponent);

            // find the provider
            ExtendedDataProvider *provider = manager->getLocalUserExtendedDataProvider(mRequest.getComponentId());
            if (provider == nullptr)
                return ERR_SYSTEM;

            mRequest.getUserSessionExtendedData().copyInto(mResponse);
            BlazeRpcError err = provider->onRefreshExtendedData(mRequest.getUserInfo(), mResponse);

            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    
    DEFINE_REMOTEREFRESHUSEREXTENDEDDATA_CREATE_COMPNAME(UserSessionManager)
   


} // Blaze

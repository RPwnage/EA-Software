/*************************************************************************************************/
/*!
    \file   remoteloaduserextendeddata_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RemoteLoadUserExtendedDataCommand

    Lookup the name associated with an entity id.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/remoteloaduserextendeddata_stub.h"

namespace Blaze
{

    class RemoteLoadUserExtendedDataCommand : public RemoteLoadUserExtendedDataCommandStub
    {
    public:

       RemoteLoadUserExtendedDataCommand(Message* message, Blaze::RemoteUserExtendedDataUpdateRequest* request, UserSessionsSlave* componentImpl)
            :RemoteLoadUserExtendedDataCommandStub(message, request),
              mComponent(componentImpl)
        {
        }

        ~RemoteLoadUserExtendedDataCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        RemoteLoadUserExtendedDataCommandStub::Errors execute() override
        {
            UserSessionManager *manager = static_cast<UserSessionManager*>(mComponent);

            // find the provider
            ExtendedDataProvider *provider = manager->getLocalUserExtendedDataProvider(mRequest.getComponentId());
            if (provider == nullptr)
                return ERR_SYSTEM;

            mRequest.getUserSessionExtendedData().copyInto(mResponse);
            BlazeRpcError err = provider->onLoadExtendedData(mRequest.getUserInfo(), mResponse);
            return commandErrorFromBlazeError(err);
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    
    DEFINE_REMOTELOADUSEREXTENDEDDATA_CREATE_COMPNAME(UserSessionManager)
   


} // Blaze

/*************************************************************************************************/
/*!
    \file   listallpermissions_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ListAllPermissionsCommand 

    process user session extended data retrieval requests from the client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/listallpermissions_stub.h"

namespace Blaze
{
    class ListAllPermissionsCommand : public ListAllPermissionsCommandStub
    {
    public:
        ListAllPermissionsCommand(Message* message, ListAllPermissionsRequest* request, UserSessionsSlave* componentImpl)
            : ListAllPermissionsCommandStub(message, request),
              mComponent(componentImpl)
        {
        }

        ~ListAllPermissionsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:
        ListAllPermissionsCommandStub::Errors execute() override
        {
            BLAZE_TRACE_LOG(Log::USER, "[ListAllPermissionsCommand].start()");
            if(EA_UNLIKELY(gUserSessionManager == nullptr))
            {
                return ERR_SYSTEM;
            }

            ComponentId reqComponentId = mRequest.getComponentId();
            PermissionsListMap& permissionByName = mResponse.getPermissionsByComponent();

            const ComponentBaseInfo::PermissionInfoByNameMap& allPermissions = ComponentBaseInfo::getAllPermissionsByName();
            for (ComponentBaseInfo::PermissionInfoByNameMap::const_iterator i = allPermissions.begin(), e = allPermissions.end(); i != e; ++i)
            {
                ComponentId componentId = BLAZE_COMPONENT_FROM_PERMISSION(i->second->id);
                BlazeComponentName componentName = BlazeRpcComponentDb::getComponentNameById(componentId);

                if (componentId == reqComponentId || reqComponentId == 0)
                {
                    if (blaze_strcmp(componentName.c_str(), "<UNKNOWN>") != 0 && componentId != 0)
                    {
                        if (permissionByName.find(componentName) == permissionByName.end())
                        {
                            permissionByName[componentName] = BLAZE_NEW PermissionsList();
                        }

                        const char8_t* permissionName = i->first;

                        permissionByName[componentName]->push_back(permissionName);
                    }

                }
            }

            return ERR_OK;
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_LISTALLPERMISSIONS_CREATE_COMPNAME(UserSessionManager)
} // Blaze

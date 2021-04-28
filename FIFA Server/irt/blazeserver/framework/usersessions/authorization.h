/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_AUTHORIZATION_H
#define BLAZE_AUTHORIZATION_H

#include "EASTL/bitset.h"
#include "framework/tdf/accessgroup_server.h"
#include "framework/connection/inetaddress.h"
#include "framework/component/blazepermissions.h"

namespace Blaze
{
    class InetFilter;

namespace Authorization
{
    // forward declaration
    class GroupManager;

    const uint8_t GROUP_NAME_INVALID = 0;  

    typedef eastl::map<Permission, eastl::string> PermissionMap;

    class Group
    {
        NON_COPYABLE(Group);
    public:
        Group(const AccessGroupName& groupName, GroupManager& groupManager); 
        virtual ~Group() {};
        
        void grantPermission(const Permission& permission, const char8_t* permissionName = nullptr);
        void grantIpPermission(const char8_t* ipWhiteListName);

        bool isAuthorized(const Permission& requestedPermission) const;
        bool isAuthorized(const InetAddress& addr) const;

        void setRequireTrustedClient(bool requireTrustedClient) { mRequireTrustedClient = requireTrustedClient; }
        bool getRequireTrustedClient() const { return mRequireTrustedClient; }

        void getPermissionMap(PermissionStringMap& permissionMap) const;
        const char8_t* getName() const { return mName.c_str(); }

    private:
        AccessGroupName mName;
        PermissionMap mGrantedPermission;
        bool mHasAllPermissions;

        InetFilter* mInetFilter; // a reference from groupmanager, should not delete by this object
        bool mRequireTrustedClient;
        GroupManager& mGroupManager; 
    };

}

}

#endif

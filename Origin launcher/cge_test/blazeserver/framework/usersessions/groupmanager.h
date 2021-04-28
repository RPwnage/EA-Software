/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef GROUPMANAGER_H
#define GROUPMANAGER_H

#include <EASTL/map.h>
#include "blazerpcerrors.h"
#include "framework/tdf/accessgroup_server.h"
#include "framework/util/shared/blazestring.h"
#include "framework/usersessions/authorization.h"

namespace Blaze
{
    class InetFilter;

namespace Authorization
{
    class GroupManager
    {
    public:
        GroupManager();
        ~GroupManager();

    public:
        typedef eastl::map<AccessGroupName, Group*, CaseInsensitiveStringLessThan> GroupObjByNameMap;
        typedef eastl::map<ClientType, AccessGroupName> GroupByClientTypeMap;
        typedef eastl::map<IpWhiteListName, InetFilter*> InetFilterByWhiteListNameMap;
        typedef EA::TDF::TdfString AccessGroupKey;
        typedef eastl::map<AccessGroupKey, AccessGroupName> AccessGroupMap;

    public:
        // Add/remove group to/from group manager - GroupObjByNameMap
        Group* addGroup(const char8_t* groupName); 

        // Default group for client type related functions - GroupByClientTypeMap
        const AccessGroupName* getDefaultGroupName(ClientType clientType) const;
        void  setDefaultGroup(ClientType clientType, const char8_t* groupName);

        // Ip whitelist related functions - InetFilterByWhiteListNameMap
        InetFilter* addIpWhiteList(const char8_t* ipWhiteListName, const NetworkFilterConfig& ipList);
        InetFilter* getIpFilterByName(const IpWhiteListName& ipWhiteListName) const;

        // retrieve group obj from group name
        Group* getGroup(const char8_t* groupName);
        const Group* getGroup(const char8_t* groupName) const;

        // retrieve inetfilter obj from whitelist name
        InetFilter* getInetFilter(const char8_t* ipWhiteListName);
        const InetFilter* getInetFilter(const char8_t* ipWhiteListName) const;

       // retrieve allowed permissions for a given group
        bool getPermissionMap(const char8_t* groupName, PermissionStringMap& permissionMap) const;
        void getAuthorizationList(AuthorizationList& authList) const;
        
    private:
        bool isValidGroupName(const char8_t* groupName) const;
        bool isDefaultGroupName(ClientType clientType, const char8_t* groupName) const;

    private:
        GroupObjByNameMap  mAccessGroupObjMap;            // group objects keyed by group name (from config file)
        GroupByClientTypeMap mDefaultGroupMap;            // group name keyed by client type, used to keep default group for each client type (from config file)
        InetFilterByWhiteListNameMap mIpWhiteListMap;    // InetFilter objects keyed by Ip white list name (from config file)
        AccessGroupMap mAccessGroupMap; // AccessGroupName by AccessGroupKey(ExternalKey  + ClienType) Map, data cached from db
    };
}
}

#endif //GROUPMANAGER_H

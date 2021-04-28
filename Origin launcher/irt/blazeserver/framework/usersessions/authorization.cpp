/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/usersessions/authorization.h"
#include "framework/usersessions/groupmanager.h"
#include "framework/connection/inetfilter.h"

namespace Blaze
{
namespace Authorization
{


Group::Group(const AccessGroupName& groupName, GroupManager& groupManager)
    : mName(groupName),   
      mHasAllPermissions(false),
      mInetFilter(nullptr),
      mRequireTrustedClient(false),
      mGroupManager(groupManager)
{   
}

bool Group::isAuthorized(const Permission& requestedPermission) const 
{   
    return (mHasAllPermissions || (mGrantedPermission.find(requestedPermission) != mGrantedPermission.end()));
}

/*! ************************************************************************************************/
/*! \brief check if the given IP address is a match to ip filter for this group

    \param[in] addr - Address to check if match for (in network byte order).

    \return - true if address is a match; false otherwise.
***************************************************************************************************/
bool Group::isAuthorized(const InetAddress& addr) const 
{
    if (mInetFilter)
    {
        return mInetFilter->match(addr);
    }   

    // no filter means no ip requirement, always allowed
    return true;
}

/*! ************************************************************************************************/
/*! \brief get all granted permissions for this group

    \param[out] permissionMap - permission map (bitset > string)
***************************************************************************************************/
void Group::getPermissionMap(PermissionStringMap& permissionMap) const
{
    if (mHasAllPermissions)
    {
        const ComponentBaseInfo::PermissionInfoByNameMap& allPermissions = ComponentBaseInfo::getAllPermissionsByName();
        ComponentBaseInfo::PermissionInfoByNameMap::const_iterator i = allPermissions.begin();
        ComponentBaseInfo::PermissionInfoByNameMap::const_iterator e = allPermissions.end();

        for ( ;i != e; ++i)
        {
            const char8_t* permissionS = i->first;
            Permission permission = i->second->id;            
            uint64_t permissionValue = static_cast<uint64_t>(permission);
            permissionMap[permissionValue] = permissionS;
        }
    }
    else
    {
        PermissionMap::const_iterator i = mGrantedPermission.begin();
        PermissionMap::const_iterator e = mGrantedPermission.end();

        for ( ;i != e; ++i)
        {
            Permission permission = i->first;
            const char8_t* permissionS = (i->second).c_str();
            uint64_t permissionValue = static_cast<uint64_t>(permission);
            permissionMap[permissionValue] = permissionS;
        }
    }
}

void Group::grantPermission(const Permission& permission, const char8_t* permissionName)
{   
    if (permission == PERMISSION_ALL)
    {
        mHasAllPermissions = true;
    }
    else
    {
        if (mGrantedPermission.find(permission) == mGrantedPermission.end())
        {
            mGrantedPermission[permission] = permissionName;
        }
    }
}

/*! **********************************************************************************************/
/*! \brief grant permission to indicated ip white list name

    \param[in] ipWhiteListName - ip white list name for given group from config file
*************************************************************************************************/
void Group::grantIpPermission(const char8_t* ipWhiteListName)
{   
    if ((ipWhiteListName != nullptr) && (ipWhiteListName[0] != '\0'))
    {
        mInetFilter = mGroupManager.getIpFilterByName(ipWhiteListName);
    }
}
}//namespace Authorization

}//namespace Blaze

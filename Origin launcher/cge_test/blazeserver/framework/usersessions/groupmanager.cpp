/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


#include "framework/blaze.h"
#include "framework/usersessions/groupmanager.h"
#include "framework/usersessions/authorization.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/connection/inetfilter.h"

namespace Blaze
{
namespace Authorization
{

GroupManager::GroupManager()
{
}

GroupManager::~GroupManager()
{
    // clean all the map and group objs created
    GroupObjByNameMap::iterator it = mAccessGroupObjMap.begin();
    GroupObjByNameMap::iterator itend = mAccessGroupObjMap.end();
    for (; it != itend; ++it)
    {
        Group *group = it->second;
        delete group;
    }

    InetFilterByWhiteListNameMap::iterator itIp = mIpWhiteListMap.begin();
    InetFilterByWhiteListNameMap::iterator itIpend = mIpWhiteListMap.end();
    for (; itIp != itIpend; ++itIp)
    {
        InetFilter *filter = itIp->second;
        delete filter;
    }
}

void GroupManager::getAuthorizationList(AuthorizationList& authList) const
{
    AccessGroupMap::const_iterator it = mAccessGroupMap.begin();
    AccessGroupMap::const_iterator itEnd = mAccessGroupMap.end();
    authList.reserve(mAccessGroupMap.size());
    for (; it != itEnd; ++it)
    {
        AuthorizationRecord* authRecord = authList.pull_back();
        const AccessGroupKey& key = it->first;
        const AccessGroupName& name = it->second;
        authRecord->setGroupName(name.c_str());

        char8_t keyCopy[MAX_EXTERNALKEY_LEN];
        blaze_strnzcpy(keyCopy, key.c_str(), sizeof(keyCopy));

        char8_t* nextToken = nullptr;
        const char8_t* seperator = ":";
        char8_t* externalKey = blaze_strtok(keyCopy, seperator, &nextToken);
        authRecord->setExternalKey(externalKey);
        char8_t* clientTypeStr = blaze_strtok(nullptr, seperator, &nextToken);

        int32_t clientType;
        blaze_str2int(clientTypeStr, &clientType);
        authRecord->setClientType(ClientType(clientType));
        BLAZE_TRACE_LOG(Log::USER, "GroupManager::getAuthorizationList: Key(" << key.c_str() << ": (" << name.c_str() << ") " << externalKey << ":" << clientType << ")");
    }
}

/*! ************************************************************************************************/
/*! \brief create a new group with given index and name

    \param[in]groupName - name of the group (from config file)

    \return group new created, if it's nullptr, means the memory allocation with "new" failed or group
        with the index and name already exists
***************************************************************************************************/
Group* GroupManager::addGroup(const char8_t* groupName)
{
    if (groupName == nullptr || groupName[0] == '\0')
    {
        BLAZE_ERR_LOG(Log::USER, "GroupManager::addGroup - passed in groupName is invalid.");
        return nullptr;
    }

    // truncate access group name if needed
    if (strlen(groupName) > MAX_ACCESSGROUPNAME_LEN)
    {
        BLAZE_WARN_LOG(Log::USER, "[GroupManager].addGroup: Length of access group name(" << groupName << ") is over the max length (" << MAX_ACCESSGROUPNAME_LEN << "), access group name will be truncated.");
    }

    AccessGroupName aGroupName(groupName);
    if (mAccessGroupObjMap.find(aGroupName) != mAccessGroupObjMap.end())
    {
        BLAZE_WARN_LOG(Log::USER, "Try to add a group already exist in the set, group name: " << groupName << ".");
        return nullptr;
    }

    Group* newGroup = BLAZE_NEW_MGID(UserSessionManager::COMPONENT_MEMORY_GROUP, "Group") Group(aGroupName, *this);
    mAccessGroupObjMap[aGroupName] = newGroup;

    return newGroup;
}

/*! ************************************************************************************************/
/*! \brief add default group for each client type (from config file)

    \param[in]clientType - client type
    \return default group name for the given client type, nullptr if no default group is found
***************************************************************************************************/
const AccessGroupName* GroupManager::getDefaultGroupName(const ClientType clientType) const
{
    GroupByClientTypeMap::const_iterator it = mDefaultGroupMap.find(clientType);
    if (it != mDefaultGroupMap.end())
    {
        return &(it->second);
    }

    return nullptr;
}

/*! ************************************************************************************************/
/*! \brief add default group for given client type (from config file)

    \param[in]clientType - client type
    \param[in]groupName - default group name for the given client type, always use upper case to compare
***************************************************************************************************/
void GroupManager::setDefaultGroup(const ClientType clientType, const char8_t* groupName)
{
    if (groupName == nullptr)
    {
        BLAZE_ERR_LOG(Log::USER, "GroupManager::setDefaultGroup - passed in groupName is invalid.");
        return;
    }

    // truncate access group name if need, convert access group to upper case
    if (strlen(groupName) > MAX_ACCESSGROUPNAME_LEN)
    {
        BLAZE_WARN_LOG(Log::USER, "[GroupManager].setDefaultGroup: Length of access group name(" << groupName
            << ") is over the max length (" << MAX_ACCESSGROUPNAME_LEN << "), access group name will be truncated.");
    }

    AccessGroupName aGroupName(groupName);
    bool inserted = mDefaultGroupMap.insert(eastl::make_pair(clientType, aGroupName)).second;
    if (inserted)
    {
        BLAZE_INFO_LOG(Log::USER, "GroupManager::setDefaultGroup: default group set for client type: " << ClientTypeToString(clientType) << " = " << groupName);
    }
    else
    {
        BLAZE_WARN_LOG(Log::USER, "GroupManager::setDefaultGroup: default group for client type: " << ClientTypeToString(clientType) << " already exist");
    }
}

/*! ************************************************************************************************/
/*! \brief create a InetFilter with given name

    \param[in]ipWhiteListName - name of the whiteList

    \return InetFilter new created, if it's nullptr, means the memory allocation with "new" failed or group
        with the index and name already exists
***************************************************************************************************/
InetFilter* GroupManager::addIpWhiteList(const char8_t* ipWhiteListName, const NetworkFilterConfig& ipList)
{
    if (strlen(ipWhiteListName) > MAX_IPWHITELISTNAME_LEN)
    {
        BLAZE_WARN_LOG(Log::USER, "[GroupManager].addIpWhiteList: Length of ip white list name(" << ipWhiteListName << ") is over the max length (" << MAX_IPWHITELISTNAME_LEN << "), access group name will be truncated.");
    }

    IpWhiteListName newIpWhiteListName(ipWhiteListName);
    if (mIpWhiteListMap.find(newIpWhiteListName) != mIpWhiteListMap.end())
    {
        BLAZE_WARN_LOG(Log::USER, "GroupManager::addIpWhiteList: ip white list with that name(" << ipWhiteListName << ") already exist");
        return nullptr;
    }

    InetFilter * newFilter = BLAZE_NEW_MGID(UserSessionManager::COMPONENT_MEMORY_GROUP, "InetFilter") InetFilter();
    if (!newFilter->initialize(true, ipList))
    {
        BLAZE_ERR_LOG(Log::USER, "GroupManager::addIpWhiteList: Failed to create InetFilter for the given name(" << ipWhiteListName << ")");
        delete newFilter;
        return nullptr;
    }
    mIpWhiteListMap[ipWhiteListName] = newFilter;

    return newFilter;
}

/*! ************************************************************************************************/
/*! \brief retrieve the inetFilter for specified white list name (from config file)

    \param[in]ipWhiteListName - name of the white list 
    \return InetFilter for given whiteList name, if no filter found, return nullptr;
***************************************************************************************************/
InetFilter* GroupManager::getIpFilterByName(const IpWhiteListName& ipWhiteListName) const
{
    InetFilterByWhiteListNameMap::const_iterator it = mIpWhiteListMap.find(ipWhiteListName);
    while (it != mIpWhiteListMap.end())
    {
        return it->second;
    }

    return nullptr;
}

/*! ************************************************************************************************/
/*! \brief get all granted permissions for this group

    \param[in] groupName - name of the group to retrieve permission list
    \param[out] permissionMap - permission map (permission string keyed by bitset)
    return true if permission list if retrieved, otherwise false
***************************************************************************************************/
bool GroupManager::getPermissionMap(const char8_t* groupName, PermissionStringMap& permissionMap) const
{
    if (groupName == nullptr || groupName[0] == '\0')
    {
        BLAZE_ERR_LOG(Log::USER, "GroupManager::getPermissionList - passed in groupName is invalid.");
        return false;
    }

    const AccessGroupName aGroupName(groupName);
    GroupObjByNameMap::const_iterator it = mAccessGroupObjMap.find(aGroupName);
    if (it == mAccessGroupObjMap.end())
    {
        BLAZE_WARN_LOG(Log::USER, "GroupManager::getPermissionList - No access group with given group name(" << groupName << ") is found.");
        return false;
    }

    Group* group = it->second;
    group->getPermissionMap(permissionMap);

    return true;
}

/*! ************************************************************************************************/
/*! \brief check if the given group name exist in local group name list

    \param[in]groupName - name of the group (from config file)

    \return true if group is there, otherwise false
***************************************************************************************************/
bool GroupManager::isValidGroupName(const char8_t* groupName) const
{
    if (groupName == nullptr || groupName[0] == '\0')
    {
        BLAZE_ERR_LOG(Log::USER, "GroupManager::isValidGroupName - passed in groupName is invalid.");
        return false;
    }

    const AccessGroupName aGroupName(groupName);
    GroupObjByNameMap::const_iterator it = mAccessGroupObjMap.find(aGroupName);
    return it != mAccessGroupObjMap.end();
}

/*! ************************************************************************************************/
/*! \brief check if the given group name is default group for given client type

    \param[in]clientType - client type to retrieve default group
    \param[in]groupName - name of the group (from config file)

    \return true if given group name is default group name for given client type o, otherwise, false
***************************************************************************************************/
bool GroupManager::isDefaultGroupName(ClientType clientType, const char8_t* groupName) const
{
    if (groupName == nullptr || groupName[0] == '\0')
    {
        BLAZE_ERR_LOG(Log::USER, "GroupManager::isDefaultGroupName - passed in groupName is invalid.");
        return false;
    }

    const AccessGroupName* defaultGroupName = getDefaultGroupName(clientType);
    if (defaultGroupName == nullptr)
    {
        BLAZE_WARN_LOG(Log::USER, "isDefaultGroupName - No default group name is found for the client type: " << ClientTypeToString(clientType) << ".");
        return false;
    }

    if (blaze_strncmp(defaultGroupName->c_str(), groupName, strlen(defaultGroupName->c_str())) == 0)
    {
        return true;
    }

    return false;
}

/*! ************************************************************************************************/
/*! \brief get group object for given group name

    \param[in]groupName - name of the group

    \return group object for the specified, nullptr if no object is found
***************************************************************************************************/
const Group* GroupManager::getGroup(const char8_t* groupName) const
{
    if (groupName == nullptr || groupName[0]=='\0')
    {
        BLAZE_ERR_LOG(Log::USER, "GroupManager::getGroup - passed in groupName is invalid.");
        return nullptr;
    }

    const AccessGroupName aGroupName(groupName);
    GroupObjByNameMap::const_iterator it = mAccessGroupObjMap.find(aGroupName);
    if (it != mAccessGroupObjMap.end())
    {
        return it->second;
    }

    return nullptr;
}

/*! ************************************************************************************************/
/*! \brief get group object for given group name

    \param[in]groupName - name of the group, 

    \return group object for the specified, nullptr if no object is found
***************************************************************************************************/
Group* GroupManager::getGroup(const char8_t* groupName)
{
    if (groupName == nullptr || groupName[0] == '\0')
    {
        BLAZE_ERR_LOG(Log::USER, "GroupManager::getGroup - passed in groupName is invalid.");
        return nullptr;
    }

    const AccessGroupName aGroupName(groupName);
    GroupObjByNameMap::iterator it = mAccessGroupObjMap.find(aGroupName);
    if (it != mAccessGroupObjMap.end())
    {
        return it->second;
    }
    return nullptr;
}

/*! ************************************************************************************************/
/*! \brief get InetFilter object for given whitelist name

    \param[in]ipwhitelistName - name of the whitelist

    \return InetFilter object for the specified, nullptr if no object is found
***************************************************************************************************/
InetFilter* GroupManager::getInetFilter(const char8_t* ipWhiteListName)
{
    if (ipWhiteListName == nullptr || ipWhiteListName[0] == '\0')
    {
        BLAZE_WARN_LOG(Log::USER, "GroupManager::getInetFilter - passed in whitelist name is invalid.");
        return nullptr;
    }

    const IpWhiteListName ipName(ipWhiteListName);
    InetFilterByWhiteListNameMap::const_iterator it = mIpWhiteListMap.find(ipName);
    if (it != mIpWhiteListMap.end())
    {
        return it->second;
    }

    return nullptr;
}


/*! ************************************************************************************************/
/*! \brief get InetFilter object for given whitelist name

    \param[in]ipwhitelistName - name of the whitelist

    \return InetFilter object for the specified, nullptr if no object is found
***************************************************************************************************/
const InetFilter* GroupManager::getInetFilter(const char8_t* ipWhiteListName) const
{
    if (ipWhiteListName == nullptr || ipWhiteListName[0] == '\0')
    {
        BLAZE_WARN_LOG(Log::USER, "GroupManager::getInetFilter - passed in whitelist name is invalid.");
        return nullptr;
    }

    const IpWhiteListName ipName(ipWhiteListName);
    InetFilterByWhiteListNameMap::const_iterator it = mIpWhiteListMap.find(ipName);
    if (it != mIpWhiteListMap.end())
    {
        return it->second;
    }

    return nullptr;
}

}//namespace Authorization
}//namespace Blaze

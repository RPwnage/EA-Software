/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "framework/blaze.h"
#include "framework/usersessions/groupmanager.h"
#include "framework/tdf/accessgroup_server.h"
#include "framework/test/blazeunittest.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
using namespace Blaze;

#define GROUP_NAME_1 "CONSOLE_USER"
#define GROUP_NAME_2 "WAL_USER"

static Blaze::Authorization::GroupManager* gGroupManager = new Blaze::Authorization::GroupManager();

/*** Test Classes ********************************************************************************/
class TestAccessGroupManager
{
public:

    // add group and client type to group manager
    void testAddGroup()
    {
        // add one valid group
        Blaze::Authorization::Group* group1 = gGroupManager->addGroup(GROUP_NAME_1);
        BUT_ASSERT(group1 != nullptr);

        // add valid group
        Blaze::Authorization::Group* group2 = gGroupManager->addGroup(GROUP_NAME_2);
        BUT_ASSERT(group2 != nullptr);
    
        // test grant permission vs get permission list
        group1->grantPermission(Blaze::Authorization::PERMISSION_RELOAD_CONFIG);
        group1->grantPermission(Blaze::Authorization::PERMISSION_SPONSORED_EVENT_QUERY_USER_DATA);
        group1->grantPermission(Blaze::Authorization::PERMISSION_SPONSORED_EVENT_EDIT_USER);
        Blaze::PermissionStringMap permissionStringMap;
        gGroupManager->getPermissionMap(GROUP_NAME_1, permissionStringMap);
        BUT_ASSERT(permissionStringMap.size() == 3);
        Blaze::PermissionStringMap::const_iterator it = permissionStringMap.begin();

        Blaze::Authorization::Permission permission;
        permission = it->first;
        BUT_ASSERT((permission & Blaze::Authorization::PERMISSION_RELOAD_CONFIG) == Blaze::Authorization::PERMISSION_RELOAD_CONFIG);
        BUT_ASSERT(blaze_stricmp(it->second.c_str(), "PERMISSION_RELOAD_CONFIG") == 0);
        
        it++;
        permission = it->first;
        BUT_ASSERT((permission & Blaze::Authorization::PERMISSION_SPONSORED_EVENT_QUERY_USER_DATA) == Blaze::Authorization::PERMISSION_SPONSORED_EVENT_QUERY_USER_DATA);
        BUT_ASSERT(blaze_stricmp(it->second.c_str(), "PERMISSION_SPONSORED_EVENT_QUERY_USER_DATA") == 0);

        it++;
        permission = it->first;
        BUT_ASSERT((permission & Blaze::Authorization::PERMISSION_SPONSORED_EVENT_EDIT_USER) == Blaze::Authorization::PERMISSION_SPONSORED_EVENT_EDIT_USER);
        BUT_ASSERT(blaze_stricmp(it->second.c_str(), "PERMISSION_SPONSORED_EVENT_EDIT_USER") == 0);
    }

    // set default group for client type to group manager
    void testSetDefaultGroup()
    {
        // add default group for client type CLIENT_TYPE_HTTP_USER
        gGroupManager->setDefaultGroup(CLIENT_TYPE_HTTP_USER, GROUP_NAME_2);
        const AccessGroupName* groupName1 = gGroupManager->getDefaultGroupName(CLIENT_TYPE_HTTP_USER);
        BUT_ASSERT(strcmp(groupName1->c_str(), GROUP_NAME_2) == 0);

        // add default group for client type CLIENT_TYPE_GAMEPLAY_USER
        gGroupManager->setDefaultGroup(CLIENT_TYPE_GAMEPLAY_USER, GROUP_NAME_1);
        const AccessGroupName* groupName2 = gGroupManager->getDefaultGroupName(CLIENT_TYPE_GAMEPLAY_USER);
        BUT_ASSERT(strcmp(groupName2->c_str(), GROUP_NAME_1) == 0);
    }

    // assign to an invalid group
    void testAssignUserToAnInvalidGroup()
    {
        BlazeRpcError err = gGroupManager->assignUserToGroup("nexus001@ea.com", CLIENT_TYPE_HTTP_USER, "ABC_GROUP");
        BUT_ASSERT(err == ACCESS_GROUP_ERR_INVALID_GROUP);
    }
    
    // assign to default group
    void testAssignUserToDefaultGroup()
    {
        BlazeRpcError err = gGroupManager->assignUserToGroup("nexus001@ea.com", CLIENT_TYPE_HTTP_USER, GROUP_NAME_2);
        BUT_ASSERT(err == ACCESS_GROUP_ERR_DEFAULT_GROUP);
    }

    // remove from an invalid group
    void testRemoveUserFromAnInvalidGroup()
    {
        BlazeRpcError err = gGroupManager->removeUserFromGroup("nexus001@ea.com", CLIENT_TYPE_HTTP_USER, "ABC_GROUP");
        BUT_ASSERT(err == ACCESS_GROUP_ERR_INVALID_GROUP);
    }

    // remove from default group
    void testRemoveUserFromADefaultGroup()
    {
        BlazeRpcError err = gGroupManager->removeUserFromGroup("nexus001@ea.com", CLIENT_TYPE_HTTP_USER, GROUP_NAME_2);
        BUT_ASSERT(err == ACCESS_GROUP_ERR_DEFAULT_GROUP);
    }

    // delete groupmanager
    void deleteGroupManager()
    {
        if (gGroupManager)
            delete gGroupManager;
    }
};

// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_GROUPMANAGER\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestAccessGroupManager testAccessGroupManager;
    testAccessGroupManager.testAddGroup();
    testAccessGroupManager.testSetDefaultGroup();
    testAccessGroupManager.testAssignUserToAnInvalidGroup();
    testAccessGroupManager.testAssignUserToDefaultGroup();
    testAccessGroupManager.testRemoveUserFromAnInvalidGroup();
    testAccessGroupManager.testRemoveUserFromADefaultGroup();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}


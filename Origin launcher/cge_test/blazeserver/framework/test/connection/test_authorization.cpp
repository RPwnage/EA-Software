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
#include "framework/config/config_file.h"
#include "framework/usersessions/authorization.h"
#include "framework/usersessions/groupmanager.h"
#include "framework/connection/nameresolver.h"
#include "framework/test/blazeunittest.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
using namespace Blaze;

#define GROUP_NAME_1 "CONSOLE_USER"
#define IPWHITELIST "GOS_VIVI"

static Blaze::Authorization::GroupManager* gGroupManager = new Blaze::Authorization::GroupManager();
static Blaze::Authorization::Group* gGroup = new Blaze::Authorization::Group(GROUP_NAME_1, *gGroupManager);

/*** Test Classes ********************************************************************************/
class TestAuthorization
{
public:

    // test grant permission
    void testGrantPermission()
    {
        // add one valid group
        gGroup->grantPermission(Blaze::Authorization::PERMISSION_SET_AUTHGROUP_PERMISSION);
        gGroup->grantPermission(Blaze::Authorization::PERMISSION_RELOAD_CONFIG);
        
        bool authorTest = gGroup->isAuthorized(Blaze::Authorization::PERMISSION_SET_AUTHGROUP_PERMISSION);
        BUT_ASSERT(authorTest == true);

        authorTest = gGroup->isAuthorized(Blaze::Authorization::PERMISSION_RELOAD_CONFIG);
        BUT_ASSERT(authorTest == true);

        authorTest = gGroup->isAuthorized(Blaze::Authorization::PERMISSION_JOIN_GAME_SESSION);
        BUT_ASSERT(authorTest == false);
    }

    // set grant ippermission
    void testGrantIpPermission()
    {
        ConfigFile* map = ConfigFile::createFromString(
            "{ filter = [ 127.0.0.0/8, 10.0.0.0/8, 159.153.0.0/16, 192.168.0.0/16 ] }");
        BUT_ASSERT_MSG("test1/map create", map != nullptr);

        const ConfigSequence* seq = map->getSequence("filter");
        BUT_ASSERT_MSG("test1/sequence find", seq != nullptr);

        gGroupManager->addIpWhiteList(IPWHITELIST, *seq);
        gGroup->grantIpPermission(IPWHITELIST);
        
        Logger::initialize();
        NameResolver nr("test");

        // test allowed address
        InetAddress addr1("192.168.10.20");
        nr.blockingResolve(addr1);
        bool authorTest = gGroup->isAuthorized(addr1);
        BUT_ASSERT(authorTest == true);

        // test not allowed address
        InetAddress addr2("1.2.3.4");
        nr.blockingResolve(addr2);
        authorTest = gGroup->isAuthorized(addr2);
        BUT_ASSERT(authorTest == false);

        delete map;
    }
    
    void cleanHeap()
    {
        if (gGroupManager)
            delete gGroupManager;

        if (gGroup)
            delete gGroup;
    }
};


// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_AUTHORIZATION\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestAuthorization testAuthorization;
    testAuthorization.testGrantPermission();
    testAuthorization.testGrantIpPermission();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}

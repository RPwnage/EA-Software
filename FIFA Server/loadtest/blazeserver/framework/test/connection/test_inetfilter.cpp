/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <stdio.h>
#include <time.h>
#include "framework/blaze.h"
#include "framework/connection/inetfilter.h"
#include "framework/connection/inetaddress.h"
#include "framework/connection/nameresolver.h"
#include "framework/config/config_file.h"
#include "framework/test/blazeunittest.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
using namespace Blaze;

/*** Test Classes ********************************************************************************/
class TestInetFilter
{
public:
    TestInetFilter()
    {
    }

    // Test basic accept/deny cases
    void test1()
    {
        ConfigFile* map = ConfigFile::createFromString(
                "{ filter = [ 127.0.0.0/8, 10.0.0.0/8, 159.153.0.0/16, 192.168.0.0/16 ] }");
        BUT_ASSERT_MSG("test1/map create", map != nullptr);

        const ConfigSequence* seq = map->getSequence("filter");
        BUT_ASSERT_MSG("test1/sequence find", seq != nullptr);

        InetFilter filter;
        bool result = filter.initialize(true, *seq);
        BUT_ASSERT_MSG("test1/initialize", result);

        Logger::initialize();
        NameResolver nr("test");

        // Check some allowed addresses
        InetAddress addr1("192.168.10.20");
        nr.blockingResolve(addr1);
        result = filter.match(addr1);
        BUT_ASSERT_MSG("test1/1", result);

        InetAddress addr2("127.0.0.1");
        nr.blockingResolve(addr2);
        result = filter.match(addr2);
        BUT_ASSERT_MSG("test1/2", result);

        InetAddress addr3("10.20.30.40");
        nr.blockingResolve(addr3);
        result = filter.match(addr3);
        BUT_ASSERT_MSG("test1/3", result);

        InetAddress addr4("159.153.1.2");
        nr.blockingResolve(addr4);
        result = filter.match(addr4);
        BUT_ASSERT_MSG("test1/4", result);
        
        // Check some disallowed addresses
        InetAddress addr5("1.2.3.4");
        nr.blockingResolve(addr5);
        result = filter.match(addr5);
        BUT_ASSERT_MSG("test1/5", !result);

        InetAddress addr6("159.154.1.2");
        nr.blockingResolve(addr6);
        result = filter.match(addr6);
        BUT_ASSERT_MSG("test1/6", !result);

        delete map;
    }

    // Test invalid configs
    void test2()
    {
        ConfigFile* map;
        const ConfigSequence* seq;
        bool result;
        InetFilter filter;

        // Out of range octet
        map = ConfigFile::createFromString("{ filter = [ 256.0.0.0/8 ] }");
        BUT_ASSERT_MSG("test2/map create", map != nullptr);
        seq = map->getSequence("filter");
        BUT_ASSERT_MSG("test2/sequence find", seq != nullptr);
        result = filter.initialize(true, *seq);
        BUT_ASSERT_MSG("test2/initialize1", !result);
        delete map;

        // Out of range mask
        map = ConfigFile::createFromString("{ filter = [ 127.0.0.0/33 ] }");
        BUT_ASSERT_MSG("test2/map create", map != nullptr);
        seq = map->getSequence("filter");
        BUT_ASSERT_MSG("test2/sequence find", seq != nullptr);
        result = filter.initialize(true, *seq);
        BUT_ASSERT_MSG("test2/initialize2", !result);
        delete map;

        // Invalid characters
        map = ConfigFile::createFromString("{ filter = [ 127.0.a.0/8 ] }");
        BUT_ASSERT_MSG("test2/map create", map != nullptr);
        seq = map->getSequence("filter");
        BUT_ASSERT_MSG("test2/sequence find", seq != nullptr);
        result = filter.initialize(true, *seq);
        BUT_ASSERT_MSG("test2/initialize3", !result);
        delete map;

        // Missing octets
        map = ConfigFile::createFromString("{ filter = [ 127.0/8 ] }");
        BUT_ASSERT_MSG("test2/map create", map != nullptr);
        seq = map->getSequence("filter");
        BUT_ASSERT_MSG("test2/sequence find", seq != nullptr);
        result = filter.initialize(true, *seq);
        BUT_ASSERT_MSG("test2/initialize4", !result);
        delete map;

        map = ConfigFile::createFromString("{ filter = [ 127.00./8 ] }");
        BUT_ASSERT_MSG("test2/map create", map != nullptr);
        seq = map->getSequence("filter");
        BUT_ASSERT_MSG("test2/sequence find", seq != nullptr);
        result = filter.initialize(true, *seq);
        BUT_ASSERT_MSG("test2/initialize5", !result);
        delete map;

        map = ConfigFile::createFromString("{ filter = [ .0.0.1/8 ] }");
        BUT_ASSERT_MSG("test2/map create", map != nullptr);
        seq = map->getSequence("filter");
        BUT_ASSERT_MSG("test2/sequence find", seq != nullptr);
        result = filter.initialize(true, *seq);
        BUT_ASSERT_MSG("test2/initialize6", !result);
        delete map;
    }

    // Test explicit ip addresses
    void test3()
    {
        ConfigFile* map;
        const ConfigSequence* seq;
        bool result;
        InetFilter filter;

        Logger::initialize();
        NameResolver nr("test");

        // Out of range octet
        map = ConfigFile::createFromString("{ filter = [ 1.2.3.4, 10.20.30.40 ] }");
        BUT_ASSERT_MSG("test3/map create", map != nullptr);
        seq = map->getSequence("filter");
        BUT_ASSERT_MSG("test3/sequence find", seq != nullptr);

        result = filter.initialize(true, *seq);
        BUT_ASSERT_MSG("test3/initialize1", result);

        InetAddress addr1("1.2.3.4");
        nr.blockingResolve(addr1);
        result = filter.match(addr1);
        BUT_ASSERT_MSG("test3/1", result);

        InetAddress addr2("192.168.1.1");
        nr.blockingResolve(addr2);
        result = filter.match(addr2);
        BUT_ASSERT_MSG("test3/2", !result);

        delete map;
    }
};



// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_INETFILTER\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestInetFilter testInetFilter;
    testInetFilter.test1();
    testInetFilter.test2();
    testInetFilter.test3();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}


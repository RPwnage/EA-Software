/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <stdio.h>
#include "framework/blaze.h"
#include "framework/connection/channel.h"
#include "framework/connection/socketchannel.h"
#include "framework/connection/serversocketchannel.h"
#include "framework/connection/inetaddress.h"
#include "framework/test/blazeunittest.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/
using namespace Blaze;


/*** Test Classes ********************************************************************************/
class TestChannel
{
public:
    void testDatagramChannel()
    {
/*
        Channel::initializeNetworking();

        InetAddress* addr = new InetAddress("127.0.0.1:12345");

        ServerSocketChannel* server = new ServerSocketChannel(nullptr, addr);
        server->setBlocking(false);
        server->listen();
*/
    }

    void testSocketChannel()
    {
/*
        Channel::initializeNetworking();

        InetAddress* addr = new InetAddress("10.40.7.84:80");

        SocketChannel* sc = new SocketChannel(nullptr);

        bool result = sc->connect(addr);
        BUT_ASSERT(result);

        BUT_ASSERT(sc->isBlocking() == false);
        BUT_ASSERT(sc->isConnected() == false);
        BUT_ASSERT(sc->isConnectionPending() == false);
*/
    }
};


// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_CHANNEL\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestChannel testChannel;
    testChannel.testDatagramChannel();
    testChannel.testSocketChannel();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}


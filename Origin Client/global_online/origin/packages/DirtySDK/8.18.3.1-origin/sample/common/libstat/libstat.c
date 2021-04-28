/*H*************************************************************************************************/
/*!

    \File    libstat.c

    \Description
        This sample links in all DirtySock EE code, so that the linker-generated map file may
        be used to estimate DirtySock EE code size.

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2004.    ALL RIGHTS RESERVED.

    \Version    1.0        02/02/04 (JLB) First Version

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"

// dirtysock includes
#include "connapi.h"
#include "dirtysock.h"
#include "dirtymem.h"
#include "netconn.h"
#include "dirtygif.h"
#include "dirtygraph.h"
#include "dirtyjpg.h"

// crypt includes
#include "cryptarc4.h"
#include "cryptmd5.h"
#include "cryptrsa.h"
#include "cryptsha1.h"
#include "cryptssc2.h"
#include "cryptstp1.h"

// lobby includes
#include "buddyapi.h"
#include "hlbudapi.h"
#include "lobbydisplist.h"
#include "lobbyhasher.h"
#include "lobbytagfield.h"
#include "lobbybase64.h"
#include "lobbylan.h"
#include "lobbyutf8.h"
#include "porttestapi.h"
#include "porttestpkt.h"
#include "tickerapi.h"

// netgame includes
#include "netgamedist.h"
#include "netgamedistserv.h"
#include "netgamelink.h"
#include "netgameutil.h"

// proto includes
#include "protoaries.h"
#include "protohttp.h"
#if !defined(DIRTYCODE_XENON)
#include "protomangle.h"
#endif
#include "protoname.h"
#include "protoping.h"
#include "protoudp.h"
#include "protostream.h"
#include "protoupnp.h"

// voip includes
#include "voip.h"
#include "voiptunnel.h"

// web includes
#include "digobjapi.h"
#include "lockerapi.h"
#include "netresource.h"
#include "weboffer.h"

// xml includes
#include "xmlformat.h"
#include "xmlparse.h"
#include "xmllist.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/


// Private variables

// Public variables


/*** Public Functions ******************************************************************/


/*

Required alloc/free functions

*/

void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    return(malloc(iSize));
}

void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    free(pMem);
}

/*

   init routines - the following routines are provided to make sure the modules
   link into the application

*/


/*

    main routine

*/                      

static void _CryptModules(void)
{
    // cryptarc4
    CryptArc4Init(NULL, NULL, 0, 0);
    
    // cryptmd5
    CryptMD5Init(NULL);
    
    // cryptrsa
    CryptRSAInit(NULL, NULL, 0, NULL, 0);
    
    // cryptsha1            
    CryptSha1Init(NULL);
    
    // cryptssc2
    CryptSSC2Init(NULL, NULL, 0, 0);

    // cryptstp1    
    CryptStp1EncryptSize(NULL, 0);
}

static void _DirtysockModules(void)
{
    // dirtylibps2
    NetTick();

    //dirtygif
    DirtyGifIdentify(NULL, 0);

    //dirtygraph
    DirtyGraphCreate();
 
    //dirtyjpg
    DirtyJpgCreate();
    
    // netconn/netconnps2
    NetConnMAC();
    
    // Proto Includes
    // protoaries
    ProtoAriesCreate(0);
    
    // protohttp
    ProtoHttpCreate(0);

    // protomangle
    #if !defined(DIRTYCODE_XENON)
    ProtoMangleCreate("", 0, "", "");
    #endif
    
    // protoname
    ProtoNameAsync("", 0);
    
    // protoping
    ProtoPingCreate(0);
    
    // protoudp
    ProtoUdpCreate(0, 0);
    
    // protostream
    ProtoStreamCreate(0);

    // protoupnp
    ProtoUpnpStatus(NULL, 0, NULL, 0);
}

static void _NetgameModules(void)
{
    // netgamelink-e
    NetGameLinkCreate(NULL, 0, 0);
    
    // netgameutil-e
    NetGameUtilCreate();
    
    // netgamedist
    NetGameDistCreate(NULL, NULL, NULL, NULL, 0, 0);

    // netgamedistserv
    NetGameDistServCreate(0, 0, 0);

    //connapi
    ConnApiGetClientList(NULL);
}

static void _LobbyModules(void)
{
    // buddyapi
    #if !defined(DIRTYCODE_PS3) && !defined(DIRTYCODE_XENON)
    BuddyApiCreate2(NULL, NULL, "Chatter", "1.0", 0, NULL);
    #endif

    // favorites
    // FavCreate(0);
    
    // hlbudapi
    HLBApiCreate(NULL, NULL, 0, NULL);
    
    // lobbyapi brings in lobbyapiasync, lobbyapilist, and lobbyapiutil
    
    // lobbybase64
    LobbyBase64Encode(0, NULL, NULL);
    
    // lobbydisplist - lobbyapi included
    
    // lobbyhasher - lobbyapi included
    
    // lobbylan
    LobbyLanCreate(NULL, NULL, NULL, 0);
    
    // lobbylib - lobbyapi included
    
    // lobbynames
    DirtyUsernameCompare(NULL, NULL);
    
    // protopingmanager - lobbyapi included
    
    // lobbysort - lobbyapi included
    
    // lobbytagfield - lobbyapi included

    // porttestapi
    PortTesterCreate(0, 0, 0);
 
    //porttestpkt
    DirtyPortResponseDecode(NULL, NULL, 0);
 
    // lobbyutf8
    Utf8Strip(NULL, 0, NULL);
    
    // tickerapi
    TickerApiCreate(0);
   
}

static void _WebModules(void)
{
    LockerApiCreate(NULL, NULL, NULL);
    NetResourceCreate(NULL);
    WebOfferCreate(NULL, 0);

    //digobjapi
    DigObjApiGetLastError(NULL);
}

static void _XmlModules(void)
{
    // xmlformat
    XmlValidate(NULL, NULL, 0);

    // xmllist
    XmlListGetRawXml(NULL);
    
    // xmlparse
    XmlSkip(NULL);
}

static void _ContribModules(void)
{
    // voip
    VoipStartup(1, 1);

    //voiptunnel
    VoipTunnelCreate(0, 0, 0);
}

int main(int32_t argc, char *argv[])
{
    // pull in crypt modules
    _CryptModules();
    
    // pull in DirtySock modules
    _DirtysockModules();
    
    // pull in lobby modules
    _LobbyModules();

    // pull in netgame modules
    _NetgameModules();

    // pull in web modules
    _WebModules();

    // pull in xml modules
    _XmlModules();

    // pull in contrib modules
    _ContribModules();

    // done
    return(0);
}





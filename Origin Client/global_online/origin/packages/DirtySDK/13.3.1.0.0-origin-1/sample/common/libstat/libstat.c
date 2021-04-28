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

#include "DirtySDK/platform.h"

// dirtysock includes
#include "DirtySDK/game/connapi.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/graph/dirtygif.h"
#include "DirtySDK/graph/dirtygraph.h"
#include "DirtySDK/graph/dirtyjpg.h"

// crypt includes
#include "DirtySDK/crypt/cryptarc4.h"
#include "DirtySDK/crypt/cryptmd5.h"
#include "DirtySDK/crypt/cryptrsa.h"
#include "DirtySDK/crypt/cryptsha1.h"
#include "DirtySDK/crypt/cryptstp1.h"

// lobby includes
#include "DirtySDK/buddy/buddyapi.h"
#include "DirtySDK/buddy/hlbudapi.h"
#include "DirtySDK/util/displist.h"
#include "DirtySDK/util/hasher.h"
#include "DirtySDK/util/tagfield.h"
#include "DirtySDK/util/base64.h"
#include "DirtySDK/misc/lobbylan.h"
#include "DirtySDK/util/utf8.h"

// netgame includes
#include "DirtySDK/game/netgamedist.h"
#include "DirtySDK/game/netgamedistserv.h"
#include "DirtySDK/game/netgamelink.h"
#include "DirtySDK/game/netgameutil.h"

// proto includes
#include "DirtySDK/proto/protoaries.h"
#include "DirtySDK/proto/protohttp.h"
#if !defined(DIRTYCODE_XENON)
#include "DirtySDK/proto/protomangle.h"
#endif
#include "DirtySDK/proto/protoname.h"
#include "DirtySDK/proto/protoping.h"
#include "DirtySDK/proto/protoudp.h"
#include "DirtySDK/proto/protostream.h"
#include "DirtySDK/proto/protoupnp.h"

// voip includes
#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voiptunnel.h"

// web includes
#include "DirtySDK/web/netresource.h"
#include "weboffer.h"

// xml includes
#include "DirtySDK/xml/xmlformat.h"
#include "DirtySDK/xml/xmlparse.h"

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
    NetGameDistServCreate(0, 0);

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
    
    // base64
    Base64Encode(0, NULL, NULL);
    
    // displist - lobbyapi included
    
    // hasher - lobbyapi included
    
    // lobbylan
    LobbyLanCreate(NULL, NULL, NULL, 0);
    
    // lobbylib - lobbyapi included
    
    // lobbynames
    DirtyUsernameCompare(NULL, NULL);
    
    // protopingmanager - lobbyapi included
    
    // lobbysort - lobbyapi included
    
    // lobbytagfield - lobbyapi included

    // lobbyutf8
    Utf8Strip(NULL, 0, NULL);
}

static void _WebModules(void)
{
    NetResourceCreate(NULL);
    WebOfferCreate(NULL, 0);
}

static void _XmlModules(void)
{
    // xmlformat
    XmlValidate(NULL, NULL, 0);

    // xmlparse
    XmlSkip(NULL);
}

static void _ContribModules(void)
{
    // voip
    VoipStartup(1, 1, 0);

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





// EA Sports Online Console stuff

// WEBSITENAME is used in the redirectorclient.cfg to redirect editorial (WEBHOST) 
// traffic to the appropriate port with the LSP.  WEBHOST does not need the port anymore
// because redirectorclient.cfg will see the request for the WEBHOST host and forward it 
// to the LSP with port 90 (WEBSITENAME) which will then redirect back to the WEBHOST
// on the default port (80).  This way, the value of WEBHOST does not change depending
// on whether or not we're using secure mode.  Also, the WebOffer scripts from editorial
// don't need to worry about setting the port inside the scripts depending on the secure
// mode.
#define WEBHOST "gos.stest.ea.com"
#define WEBSITENAME "gos-core"
#define WEBDIR "Ping"
#define WEBYEAR 2013

#define EDITORIAL_WEB_SCHEME "https"
#define EDITORIAL_WEB_HOST ""
#define EDITORIAL_WEB_DIR "ping"
#define EDITORIAL_APP_NAME "editorial/ping13"
#define EDITORIAL_BASE_URL "#EDITORIAL_WEB_SCHEME#://#EDITORIAL_WEB_HOST#/#EDITORIAL_APP_NAME#"

// Address of the Sports World server
#if PLATFORM == "xbl2"
    #define EASWXLSPHOST "*.test.easportsworld.ea.com"
    #define EASWXLSPSITE "easo-core"
    #define EASWHOST "http://pg.ping12.test.easportsworld.ea.com:8307"
    #define EASWREQHOST "http://pg.ping12.test.easportsworld.ea.com:8309"
    #define EASWMEDIAHOST "http://pg.ping12.test.easportsworld.ea.com:8309"
#else
    #define EASWXLSPHOST ""
    #define EASWXLSPSITE ""
    #define EASWHOST "http://ping.dev.service.easports.com"
    #define EASWREQHOST "http://pg.ping12.test.easportsworld.ea.com:8309"
    #define EASWMEDIAHOST "http://pg.ping12.test.easportsworld.ea.com:8309"
#endif

#define ESPN_NEWS_DBG ""

#define OSDKDYNAMICMESSAGING_HOST "#DBHOST#"
#define OSDKDYNAMICMESSAGING_DBASE "ping_v6_3_dynamicmessaging_stress"
#define OSDKDYNAMICMESSAGING_USER "#DBUSER#"
#define OSDKDYNAMICMESSAGING_PASS "#DBPASS#"
#define OSDKDYNAMICMESSAGING_DBMAX 8
#define OSDKDYNAMICMESSAGING_DBPORT "#DBPORT#"

#define OSDKDBOBFUSCATOR "#DBOBFUSCATOR#"

// Game teams should be updating game specific EASOC Tollbooth Season Ticket Entitlement OFB (OSDK_SEASON_TICKET_DLC_ENTITLEMENT_PRODUCT_ID).
// EASOC Tollbooth Season Ticket Entitlement OFB. The values should be the same across game titles.
#if (PLATFORM == "xbl2")
    #define OSDK_SEASON_TICKET_DLC_ENTITLEMENT_PRODUCT_ID "OFB-EASO:109534829"
    #define OSDK_SEASON_TICKET_1_DAY_PRODUCT_ID_PERSONA1 "OFB-EASO:109531436"
    #define OSDK_SEASON_TICKET_1_YEAR_PRODUCT_ID_PERSONA1 "OFB-EASO:109530788"
    #define OSDK_SEASON_TICKET_1_DAY_PRODUCT_ID_PERSONA2 "OFB-EASO:109531434"
    #define OSDK_SEASON_TICKET_1_YEAR_PRODUCT_ID_PERSONA2 "OFB-EASO:109530840"
#elif (PLATFORM == "ps3")
    #define OSDK_SEASON_TICKET_DLC_ENTITLEMENT_PRODUCT_ID "OFB-EASO:109534828"
    #define OSDK_SEASON_TICKET_1_DAY_PRODUCT_ID_PERSONA1 "OFB-EASO:109531438"
    #define OSDK_SEASON_TICKET_1_YEAR_PRODUCT_ID_PERSONA1 "OFB-EASO:109530808"
    #define OSDK_SEASON_TICKET_1_DAY_PRODUCT_ID_PERSONA2 "OFB-EASO:109531432"
    #define OSDK_SEASON_TICKET_1_YEAR_PRODUCT_ID_PERSONA2 "OFB-EASO:109530805"
#endif

// Game teams should be updating game specific Digital Download Retail Entitlement OFB
#if (PLATFORM == "xbl2")
    #define OSDK_ONLINE_DDR_PRODUCT_ID ""
#elif (PLATFORM == "ps3")
    #define OSDK_ONLINE_DDR_PRODUCT_ID "OFB-EASO:109541569"
#else
    #define OSDK_ONLINE_DDR_PRODUCT_ID ""
#endif

// OSDK Arena Configuration (which varies per environment)
#define OSDK_ARENA_PROVIDER_REPORTING_HOST "gvstress.worldgaming.com"
#define OSDK_ARENA_PROVIDER_REPORTING_PORT 443
#define OSDK_ARENA_PROVIDER_REPORTING_POOLSIZE 5 

// account mail=osdk_arena_admin@ea.com, pass=arenaosdkarena
#define OSDK_ARENA_ADMIN_NUCLEUS_ID "12302433565"

// define to flag whether challenges are removed or not when cancelChallenge is requested.
// this is so that game reports can be examined by QA, as they get deleted with the challenge.
// DO NOT CHANGE unless you know what you're doing
#define OSDK_ARENA_PRESERVE_CHALLENGES 0

// FOG (Facebook Open Graph) ENV/PLATFORM specific configs
// AUTH and SKU params are required by the rs4 FOG app.
// HOST, AUTH, SKU are title specific. Contact rs4/EASO engine team for the values.
#if (PLATFORM == "xbl2")
    #define FACEBOOK_OPEN_GRAPH_HOST ""
    #define FACEBOOK_OPEN_GRAPH_AUTH_PARAM ""
    #define FACEBOOK_OPEN_GRAPH_SKU_PARAM ""
#elif (PLATFORM == "ps3")
    #define FACEBOOK_OPEN_GRAPH_HOST ""
    #define FACEBOOK_OPEN_GRAPH_AUTH_PARAM ""
    #define FACEBOOK_OPEN_GRAPH_SKU_PARAM ""
#else
    #define FACEBOOK_OPEN_GRAPH_HOST ""
    #define FACEBOOK_OPEN_GRAPH_AUTH_PARAM ""
    #define FACEBOOK_OPEN_GRAPH_SKU_PARAM ""
#endif

#define DYNAMICCONTENT_CLIENT_ID "fifa20"
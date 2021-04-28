#ifndef CTS_ENV_NAME
    #define CTS_ENV_NAME "#ENV_NAME#"
#endif

#define CTS_YEAR 22
#if (ENV == "prod")
    #define CTS_ENDPOINT_URL            "https://fifa#CTS_YEAR#.service.easports.com"
    #define CTS_CONTENT_SERVER          "https://fifa#CTS_YEAR#.content.easports.com"
#else
    #define CTS_ENDPOINT_URL            "https://fifa#CTS_YEAR#.#CTS_ENV_NAME#.service.easports.com"
    #define CTS_CONTENT_SERVER          "https://fifa#CTS_YEAR#.#CTS_ENV_NAME#.content.easports.com"
#endif

#define RS4HOST 					"#CTS_ENDPOINT_URL#"

#define CONTENT_YEAR				"20#CTS_YEAR#"
#define CONTENT_GUID				"221A7BAA-5DFD-4316-827D-5F0869A434D8"
#define CONTENT_HOST_PATH			"fifa/fltOnlineAssets/#CONTENT_GUID#/#CONTENT_YEAR#"
#define CONTENT_PATH 				"#CTS_CONTENT_SERVER#/#CONTENT_HOST_PATH#"

#define FIFA_POW_CONTENT_SERVER_URL_DEF 	"#CTS_CONTENT_SERVER#"
#define FIFA_POW_NUCLEUS_PROXY_URL_DEF 		"#CTS_ENDPOINT_URL#"

#define FUTCUSTOMMESSAGING_HOST 	            "#CONTENT_PATH#"
#define FUTCUSTOMMESSAGING_URI_GETMESSAGES 		"/fifa/messages"
#define FUTTUTORIALMESSAGING_URI_GETLMESSAGES 	"/fut/tutorials/#PLATFORM#"

#ifndef POW_PAS_HOST_SSL
    #define POW_PAS_HOST_SSL    "pas.#ENV_NAME#.easfc.ea.com"
#endif
#ifndef FUT_UT_HOST_SSL
    #define FUT_UT_HOST_SSL     "utas.external.#ENV_NAME#.s1.fut.ea.com"
#endif
#ifndef FUT_PORT
    #define FUT_PORT			443
#endif
#ifndef POW_PORT
    #define POW_PORT			8095
#endif
#ifndef FIFA_SSF_HOST_SSL
    #define FIFA_SSF_HOST_SSL   "ssas.external.#ENV_NAME#.s1.ssf.ea.com"
#endif

#define FUTRS4_BASE_URL		"https://#FUT_UT_HOST_SSL#:#FUT_PORT#/"
#define FIFA_POW_URL		"https://#POW_PAS_HOST_SSL#:#POW_PORT#"
#define FIFA_SSF_URL	    "https://#FIFA_SSF_HOST_SSL#:443"

#define FIFA_SSF_CLIENT_ID		"FIFA_SSF_GAME_SERVER"

#if PLATFORM == "ps4"
      #define FIFA_SSF_SKUID			"FFA22PS4"
#elif PLATFORM == "ps5"
      #define FIFA_SSF_SKUID			"FFA22PS5"
#elif PLATFORM == "xone"
      #define FIFA_SSF_SKUID			"FFA22XBO"
#elif PLATFORM == "xbsx"
      #define FIFA_SSF_SKUID			"FFA22XSX"  
#elif PLATFORM == "stadia"
      #define FIFA_SSF_SKUID			"FFA22STA"
#else
      #define FIFA_SSF_SKUID			"FFA22PCC"
	  #define FIFA_SSF_STEAM_SKUID		"FFA22STM"
#endif	


// XMS HD Base URL Setting
#define XMS_BASE_URL "#CTS_ENDPOINT_URL#/xmshd_collector/v1/"
#define CMS_UPLOAD_PERMISSION_URL "#CTS_ENDPOINT_URL#/xmshd_collector/v1/file/doupload.json?"

//Temp setting to 21
#if defined (DYNAMICCONTENT_CLIENT_ENV) 
    #define DYNAMICCONTENT_CLIENT_ID "fifa#CTS_YEAR#_#DYNAMICCONTENT_CLIENT_ENV#"
#else
    #define DYNAMICCONTENT_CLIENT_ID "fifa#CTS_YEAR#_#ENV_NAME#"
#endif
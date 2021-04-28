#define CTS_ENV_NAME "#ENV_NAME#a"

#define POW_PAS_HOST_SSL	    "pas.external.#ENV_NAME#.easfc.ea.com"
#if (PLATFORM == "xone" || PLATFORM == "xbsx")
      #define FUT_UT_HOST_SSL   "utas.external.#ENV_NAME#.s3.fut.ea.com"
#else
      #define FUT_UT_HOST_SSL   "utas.external.#ENV_NAME#.s2.fut.ea.com"
#endif

#if (PLATFORM == "xone" || PLATFORM == "xbsx")
    #define FIFA_SSF_HOST_SSL		"ssas.external.#ENV_NAME#.s3.ssf.ea.com"
#else
    #define FIFA_SSF_HOST_SSL		"ssas.external.#ENV_NAME#.s2.ssf.ea.com"
#endif

#include "osdk/env/osdk_common.boot"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL            "http://utas.#ENV_NAME#.internal.fut.ea.com:8314/"
    #if (PLATFORM == "xone" || PLATFORM == "xbsx")
        #define FIFA_ADMIN_SSF_URL			"http://ssas.#ENV_NAME#.s3.internal.ea.com:20852"
    #else
        #define FIFA_ADMIN_SSF_URL			"http://ssas.#ENV_NAME#.s2.internal.ea.com:20852"
    #endif
#endif
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************

// FIFA Cheat Detection - Shield
#if (PLATFORM == "pc")
	#define SHIELD_ENABLED 1
	#define SHIELD_ONLINE_VOLTA_ENABLED 0
	#define SHIELD_ONLINE_FUT_ENABLED 0
	#define SHIELD_BACKEND_URL "http://fifa21-shield-stage.bio-dub.ea.com:19984"
#endif

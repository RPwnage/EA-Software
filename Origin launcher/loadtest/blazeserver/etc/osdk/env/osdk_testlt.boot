#define POW_PAS_HOST_SSL        "pas.external.lta.s1.easfc.ea.com"
#define FUT_UT_HOST_SSL         "utas.external.lta.s2.fut.ea.com"
#define FIFA_SSF_HOST_SSL		"ssas.external.lta.s2.ssf.ea.com"

#include "osdk/env/osdk_common.boot"

//This needs to be enabled in PROD and DEV/TEST only. It will fail cert if enabled on any cert environment.
#define PSPLUSUPSELL_LINK       "plus--branded_upsell--fifa21"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL            "http://utas.lta.s2.internal.fut.ea.com:10542/" 
    #define FIFA_ADMIN_SSF_URL			"http://ssas.lta.s2.internal.ea.com:10852"
#endif
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************

// FIFA Cheat Detection - Shield
#if (PLATFORM == "pc")
	#define SHIELD_ENABLED 1
	#define SHIELD_BACKEND_URL "http://fifa21-shield-lt.bio-dub.ea.com:19984"
#endif
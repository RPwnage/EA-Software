#define POW_PAS_HOST_SSL        "pas.external.prod.easfc.ea.com"
#if (PLATFORM == "xone" || PLATFORM == "xbsx")
    #define FUT_UT_HOST_SSL     "utas.external.s3.fut.ea.com"
#else
    #define FUT_UT_HOST_SSL     "utas.external.s2.fut.ea.com"
#endif

#if (PLATFORM == "xone" || PLATFORM == "xbsx")
    #define FIFA_SSF_HOST_SSL		"ssas.external.s3.ssf.ea.com"
#else
    #define FIFA_SSF_HOST_SSL		"ssas.external.s2.ssf.ea.com"
#endif

#define DYNAMICCONTENT_CLIENT_ENV "production"

#include "osdk/env/osdk_common.boot"

//This needs to be enabled in PROD and DEV/TEST only. It will fail cert if enabled on any cert environment.
#define PSPLUSUPSELL_LINK       "plus--branded_upsell--fifa21"

// FIFA Cheat Detection - Shield
#if (PLATFORM == "pc")
    #define SHIELD_ENABLED 1
	#define SHIELD_ONLINE_VOLTA_ENABLED 0
	#define SHIELD_ONLINE_FUT_ENABLED 0
    #define SHIELD_BACKEND_URL "http://fifa21-shield-pr.bio-dub.ea.com:19984"
#endif 
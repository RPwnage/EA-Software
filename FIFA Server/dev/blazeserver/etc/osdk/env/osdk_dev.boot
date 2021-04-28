#define CTS_ENV_NAME "test"

#define POW_PAS_HOST_SSL        "pas.external.ftest.s1.ea.com"
#define FUT_UT_HOST_SSL         "utas.external.ftest.s1.ea.com"
#define FIFA_SSF_HOST_SSL		"ssas.external.ftest.s1.ea.com"

#define DYNAMICCONTENT_CLIENT_ENV "test"

#include "osdk/env/osdk_common.boot"

//This needs to be enabled in PROD and DEV/TEST only. It will fail cert if enabled on any cert environment.
#define PSPLUSUPSELL_LINK		"plus--branded_upsell--fifa21"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL            "http://utas.ftest.s1.internal.ea.com:10542/"
    #define FIFA_ADMIN_SSF_URL			"http://ssas.ftest.s1.internal.ea.com:10852"
#endif
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************


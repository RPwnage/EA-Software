#define POW_PAS_HOST_SSL	    "pas.fos5.easfc.ea.com"
#define FUT_UT_HOST_SSL		    "utas.fos5.fut.ea.com"

#define DYNAMICCONTENT_CLIENT_ENV "press"

#include "osdk/env/osdk_common.boot"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL			"http://#FUT_UT_HOST_SSL#:8314/"
    #define FIFA_ADMIN_SSF_URL			"http://ssas.press1.s1.internal.ea.com:23852"
#endif
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
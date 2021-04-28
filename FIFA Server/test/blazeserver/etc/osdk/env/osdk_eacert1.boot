#define POW_PAS_HOST_SSL        "pas.fos1.easfc.ea.com"
#define FUT_UT_HOST_SSL         "utas.fos1.fut.ea.com"

#include "osdk/env/osdk_common.boot"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL            "http://utas.internal.fos1.fut.ea.com:8314/"
    #define FIFA_ADMIN_SSF_URL		    "http://ssas.eacert1.s1.internal.ea.com:13852"
#endif
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
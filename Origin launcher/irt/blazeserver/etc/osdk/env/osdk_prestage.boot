#define CTS_ENV_NAME "gencert2"

#define POW_PAS_HOST_SSL	    "pas.external.#CTS_ENV_NAME#.s1.ea.com"
#define FUT_UT_HOST_SSL		    "utas.external.#CTS_ENV_NAME#.s1.fut.ea.com"
#define FIFA_SSF_HOST_SSL		"ssas.external.#CTS_ENV_NAME#.s1.ssf.ea.com"

#include "osdk/env/osdk_common.boot"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL            "http://utas.#CTS_ENV_NAME#.internal.fut.ea.com:8314/"
    #define FIFA_ADMIN_SSF_URL			"http://ssas.#CTS_ENV_NAME#.s1.internal.ea.com:33852"
#endif
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************

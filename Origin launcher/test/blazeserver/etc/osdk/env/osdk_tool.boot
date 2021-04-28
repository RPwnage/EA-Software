#define CTS_ENV_NAME "press2"

#define POW_PAS_HOST_SSL 	    "pas.#CTS_ENV_NAME#.easfc.ea.com"
#define FUT_UT_HOST_SSL 	    "utas.#CTS_ENV_NAME#.fut.ea.com"
#define FIFA_SSF_HOST_SSL		"ssas.external.#CTS_ENV_NAME#.s1.ssf.ea.com"

#include "osdk/env/osdk_common.boot"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL			"http://462518-fifa14ltapp266.rspc-lhr.ea.com:12541/"
    #define FIFA_ADMIN_SSF_URL			"http://ssas.#CTS_ENV_NAME#.s1.internal.ea.com:10852"
#endif
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
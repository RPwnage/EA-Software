#include "osdk/env/osdk_common.boot"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL			"http://#FUT_UT_HOST_SSL#:8314/"
    #define FIFA_ADMIN_SSF_URL	        "http://ssas.#ENV_NAME#.s1.internal.ea.com:10852"
#endif
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************

#define POW_PAS_HOST_SSL        "pas.external.#ENV_NAME#.s1.ea.com"
#define FUT_UT_HOST_SSL         "utas.external.#ENV_NAME#.s1.ea.com"
#define FIFA_SSF_HOST_SSL		"ssas.external.#ENV_NAME#.s1.ea.com"

#include "osdk/env/osdk_common.boot"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL          "http://utas.internal.#ENV_NAME#.s1.ea.com:10542/"
    #define FIFA_ADMIN_SSF_URL        "http://ssas.#ENV_NAME#.s1.internal.ea.com:10852"
    
#endif
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
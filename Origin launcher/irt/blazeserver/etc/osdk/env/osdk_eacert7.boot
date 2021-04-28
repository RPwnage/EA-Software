
#include "osdk/env/osdk_common.boot"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL            "http://utas-#ENV_NAME#-s1-admin.ea.com:40000/"
    #define FIFA_ADMIN_SSF_URL		    "http://ssas.#ENV_NAME#.s1.internal.ea.com:13852"
#endif
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
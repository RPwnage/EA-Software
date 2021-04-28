#include "osdk/env/osdk_common.boot"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL            "http://utas.internal.eacert3.fut.ea.com:8314/"
    #define FIFA_ADMIN_SSF_URL			"http://ssas.eacert3.s1.internal.ea.com:17852"
#endif 
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
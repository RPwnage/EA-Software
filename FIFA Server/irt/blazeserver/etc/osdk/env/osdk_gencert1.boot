#define FIFA_SSF_HOST_SSL		"ssas.external.gencert1.s1.ssf.ea.com"

#include "osdk/env/osdk_common.boot"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL            "http://utas.ftest.s1.internal.ea.com:10542/"
    #define FIFA_ADMIN_SSF_URL			"http://ssas.gencert1.s1.internal.ea.com:30852"
#endif
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************

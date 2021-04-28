#define FIFA_SSF_HOST_SSL		"ssas.external.gt.s1.ea.com"

#include "osdk/env/osdk_common.boot"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL            "utas.internal.gencert3.fut.ea.com:8314/"
    #define FIFA_ADMIN_SSF_URL			"http://ssas.gt.s1.internal.ea.com:10852"
#endif
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************

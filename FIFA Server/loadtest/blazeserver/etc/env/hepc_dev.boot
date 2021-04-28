#define ENV "dev"
#define ENV_NAME "hepc"

#ifndef DBBASE_SUFFIX
    #if PLATFORM == "pc"
        #define DBBASE_SUFFIX "hepc"
    #endif
#endif

#include "env/common.boot"

#if PLATFORM == "pc"
    #define BLAZE_SERVICE_NAME_SUFFIX "hepc-#DEV_USER_NAME_LOWER#"
#else
    #define BLAZE_SERVICE_NAME_SUFFIX "#DEV_USER_NAME_LOWER#"
#endif

#include "osdk/env/osdk_dev.boot"
#include "blaze.boot"



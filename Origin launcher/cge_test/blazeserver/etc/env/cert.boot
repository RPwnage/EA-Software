#define ENV "cert"
#if (PLATFORM == "xone")
    #define ENV_NAME "mscert2"
#elif (PLATFORM == "ps4")
    #define ENV_NAME "sonycert2"
#elif (PLATFORM == "pc")
    #define ENV_NAME "eacert3"
#elif (PLATFORM == "xbsx")
    #define ENV_NAME "mscert4"
#elif (PLATFORM == "ps5")
    #define ENV_NAME "sonycert4"
#elif (PLATFORM == "stadia")
    #define ENV_NAME "gencert1"
#else
    #define ENV_NAME "eacert8"
#endif

#include "env/common.boot"
#include "osdk/env/osdk_cert.boot"
#include "blaze.boot"



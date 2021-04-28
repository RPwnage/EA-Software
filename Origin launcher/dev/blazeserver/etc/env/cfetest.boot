#define ENV "test"
#define ENV_NAME "cfe"

#include "env/common.boot"

#if PLATFORM == "xone"
    #define PROJECTID "313946"
#elif PLATFORM == "ps4"
	#define PROJECTID "313945"
#endif

#define ACCESS_ENTITLEMENT_TAG "CFE_ONLINE_ACCESS"
#define FIFASTATS_CONTEXTID "FIFA20_CFE"
#define FIFASTATS_CONTEXTIDFUT "FIFA20_FUT_CFE"
#define DEFAULTSERVICENAME_RELEASETYPE "#ENV_NAME"

#include "osdk/env/osdk_cfetest.boot"
#include "blaze.boot"


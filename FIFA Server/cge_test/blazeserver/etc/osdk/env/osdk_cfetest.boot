#define FIFA_SSF_HOST_SSL		"ssas.external.gencert4.s1.ea.com"

#define DYNAMICCONTENT_CLIENT_ENV "cfetest"

#include "osdk/env/osdk_common.boot"

// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************
#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define FUTRS4_ADMIN_URL            "http://utas.internal.gencert4.fut.ea.com:8314/"
    #define FIFA_ADMIN_SSF_URL			"http://ssas.gencert4.s1.internal.ea.com:10852"
#endif
// ************** DO NOT PROPAGATE THIS TO PROD!!! ******************************

#if (ENV == "dev" || ENV == "test" || ENV=="cert")
    #define CLUBS_AVOID_LAST_X_OPPONENTS 1
	#define H2H_AVOID_LAST_X_OPPONENTS 1
	#define LIVECOMP_AVOID_LAST_X_OPPONENTS 1
#else
    #define CLUBS_AVOID_LAST_X_OPPONENTS 0
	#define H2H_AVOID_LAST_X_OPPONENTS 0
	#define LIVECOMP_AVOID_LAST_X_OPPONENTS 0
#endif

#define GP_PIN_RECORD_FREQUENCY 1

#if (PLATFORM == "xone")
	#define EADP_GROUPS_RIVAL_TYPE "FIFA20RivalXOneCFE"
	#define EADP_GROUPS_OVERALL_TYPE "FIFA20OvaXOneCFE"
	#define EADP_GROUPS_FRIEND_TYPE "FIFA20FdsXOneCFE"
#elif (PLATFORM == "ps4")
	#define EADP_GROUPS_RIVAL_TYPE "FIFA20RivalPS4CFE"
	#define EADP_GROUPS_OVERALL_TYPE "FIFA20OvaPS4CFE"
	#define EADP_GROUPS_FRIEND_TYPE "FIFA20FdsPS4CFE"
#endif
#define EADP_STATS_CONTEXT_ID "FIFA20_CFE"

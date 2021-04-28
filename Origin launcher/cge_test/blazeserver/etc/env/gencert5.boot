#define ENV "test"
#define ENV_NAME "gencert5"

#include "env/common.boot"

#define REDIRECTOR_ADDRESS "internal.gosredirector.stest.ea.com"
#define FRIENDLY_NAME "FIFA 2022"

#define LFUHOST "sdevbesl01.online.ea.com"
#define LFUPORT 8000
#define LFUEXTPORT 80

// Override defaults from common.boot
#if (ENV == "test")
        #define DBHOST "fifa22-gencert5-db-01.bio-dub.ea.com"
        #define DBSLAVEHOST "fifa22-gencert5-db-02.bio-dub.ea.com"
        #if PLATFORM == "ps5"
                #define DBBASE "fifa_2022_ps5_gencert5"
        #elif PLATFORM == "xbsx"
                #define DBBASE "fifa_2022_xbsx_gencert5"
        #else
                #define DBBASE "fifa_2022_ps5_gencert5"
        #endif
#endif

#define CSEMAILHOST "betacsrapp01.beta.eao.abn-iad.ea.com:8001"

// This is an environmental specific configuration item. See matchmaker configuration for more details
#define EVALUATE_CONNECTIBILITY_DURING_CREATE_GAME_FINALIZATION "false"

#define ALLOW_STRESS_LOGIN "true"

#define BASE_PORT ""
#define CONFIG_MASTER_PORT ""
#define AUX_MASTER_PORT ""

#define UED_GEO_IP "true"

#include "osdk/env/osdk_gencert5.boot"
#include "blaze.boot"


#define VAULT_ENABLED "true"
#define VAULT_SHARED_NAMESPACE "true"

#define CLOUD_REDIS_NODES "true"

#define CORE_FILE_PREFIX "/var/core-dump/core"

#ifndef LOG_OUTPUT
    #define LOG_OUTPUT "stdout"
#endif

#define DBHOST ""
#define DBPORT 3306
#define DBBASE_PREFIX ""
#define DBUSER ""
#define DBPASS ""
#define DBOBFUSCATOR ""
#define DBMAX 8

// used by core slaves
#define PORT_RANGE_LARGE 20
// used by all other instances
#define PORT_RANGE_SMALL 12

#define CORE_SLAVE1_PORT            BASE_PORT
#define CORE_SLAVE2_PORT            (BASE_PORT + PORT_RANGE_LARGE)
#define CORE_MASTER1_PORT           (BASE_PORT + (PORT_RANGE_LARGE*2))
#define CORE_MASTER2_PORT           (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*1))
#define CONFIG_MASTER_PORT          (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*2))
#define MM_SLAVE1_PORT              (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*3))
#define MM_SLAVE2_PORT              (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*4))
#define SEARCH_SLAVE1_PORT          (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*5))
#define SEARCH_SLAVE2_PORT          (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*6))
#define CORE_NONSHARDED_MASTER_PORT (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*7))
#define GR_SLAVE1_PORT              (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*8))
#define GR_SLAVE2_PORT              (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*9))
#define GDPR_SLAVE_PORT             (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*10))

#ifndef CONTAINER
// Specifies a list of servers loaded by this process.
serverInstances = [
#ifndef BLAZE_LITE
    { type="configMaster", name="configMaster", port=#CONFIG_MASTER_PORT# }
    { type="coreMaster", name="coreMaster1", port=#CORE_MASTER1_PORT# }
    { type="coreMaster", name="coreMaster2", port=#CORE_MASTER2_PORT# }
    { type="coreSlave", name="coreSlave1", port=#CORE_SLAVE1_PORT# }
    { type="coreSlave", name="coreSlave2", port=#CORE_SLAVE2_PORT# }
    { type="mmSlave", name="mmSlave1", port=#MM_SLAVE1_PORT# }
    { type="mmSlave", name="mmSlave2", port=#MM_SLAVE2_PORT# }
    { type="searchSlave", name="searchSlave1", port=#SEARCH_SLAVE1_PORT# }
    { type="searchSlave", name="searchSlave2", port=#SEARCH_SLAVE2_PORT# }   
    { type="coreNSMaster", name="coreNSMaster", port=#CORE_NONSHARDED_MASTER_PORT# }   
    { type="grSlave", name="grSlave1", port=#GR_SLAVE1_PORT# }
    { type="grSlave", name="grSlave2", port=#GR_SLAVE2_PORT# }
    { type="gdprAuxSlave", name="gdprAuxSlave", port=#GDPR_SLAVE_PORT# }
#else
    { type="configMaster", name="configMaster", port=#CONFIG_MASTER_PORT# }
    { type="coreMaster", name="coreMaster1", port=#CORE_MASTER1_PORT# }
    { type="coreMaster", name="coreMaster2", port=#CORE_MASTER2_PORT# }
    { type="coreSlave", name="coreSlave1", port=#CORE_SLAVE1_PORT# }
    { type="coreSlave", name="coreSlave2", port=#CORE_SLAVE2_PORT# }
#endif
]
#endif

// CONFIG_OVERRIDE_DIR is set by BitC on the command line (usually for ILT runs)
#ifdef CONFIG_OVERRIDE_DIR
    rootConfigDirectories = [".","#CONFIG_OVERRIDE_DIR#"]
#endif

#include "blaze.boot"

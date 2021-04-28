#define ENV "dev"
#define ENV_NAME "dev"

// See logging.cfg for details on how this affects log output.
// Setting this to "both" will cause logs to be printed in the console, and if --logname is provided, to a file as well.
#define LOG_OUTPUT "both"

#define SSL_KEY "ssl/key.pem"
#define SSL_CERT "ssl/cert.pem" 
#define INTERNAL_SSL_KEY "ssl/key.pem"
#define INTERNAL_SSL_CERT "ssl/cert.pem" 

#ifndef ENABLE_STADIA_LOCAL_ENV
    #define ENABLE_STADIA_LOCAL_ENV 0
#endif

#ifdef ENABLE_STADIA_LOCAL_ENV
    #if ENABLE_STADIA_LOCAL_ENV == "1"
        #define USE_LOCALHOST_NETWORKING 1
        #define HOST_OVERRIDE "localhost"
        #define INT_HOST_OVERRIDE "localhost"
    #endif
#endif

// pc is gen4. pc-hepc uses gen5 settings. Refer to GenGB branch
#ifndef PLATFORM
    #define PLATFORM "ps5"
    //#define PLATFORM "ps4"
    //#define PLATFORM "pc"
    //#define PLATFORM "xbsx"
    //#define PLATFORM "xone"
	//#define PLATFORM "stadia"
    //#define PLATFORM "common"
#endif

// Determines whether ESS Vault will be used for secrets
#if defined(VAULT_SECRET_ID) || defined(VAULT_SECRET_PATH)
    // if either defined already (e.g., in your exec command arguments), then assume...
    #define VAULT_ENABLED "true"
#endif
#ifndef VAULT_ENABLED
    #define VAULT_ENABLED "false"
#endif

// These values are only relevant if Vault is enabled, and are common
// for either shared namespace or team-specific namespace
#ifndef VAULT_SECRET_PATH
    // default to your working directory
    #define VAULT_SECRET_PATH "secretid.cfg"
#endif
#define SSL_PATH_TYPE "VAULT_SHARED"

// If Vault is enabled, this determines whether the newer shared namespace will be used,
// or an older team-specific namespace
#define VAULT_SHARED_NAMESPACE "false"

#if VAULT_SHARED_NAMESPACE == "true"
  #define VAULT_ROLE_ID "gs-dev"
  #define VAULT_DBNAME "#ENV#/gs-dev-common"
#else
  #define VAULT_NAMESPACE "eadp-gs-blaze-dev"
  #define VAULT_ROLE_ID "blaze-dev"
  
  #define VAULT_ROOT "blaze-dev-#PLATFORM#"
#endif


#include "env/common.boot"

// local hosts are not allowed to connect to EU1 dbs 
#define DBHOST "gossdevmydb004.bio-sjc.ea.com"

//#define BLAZE_SERVICE_NAME_PREFIX "fifa-2022-#DEV_USER_NAME_LOWER#"
#define BLAZE_SERVICE_NAME_SUFFIX "#DEV_USER_NAME_LOWER#"

// Async DB connections are enabled by default.  Set DB_ASYNC_CONNS to 0 to disable them.
#define DB_ASYNC_CONNS 1

// Developers: please create and use your own platform specific schemas
// Use a schema prefix equal to your username on the system you are using
// Or you can customize DBBASE_PREFIX individually below
#ifndef DBBASE_PREFIX
    #define DBBASE_PREFIX "#DEV_USER_NAME#"
#endif

#ifndef DBBASE_SUFFIX 
	#define DBBASE 	"fifa_2022_#DBBASE_PREFIX#_#PLATFORM#"
#else
	#define DBBASE 	"fifa_2022_#DBBASE_PREFIX#_#PLATFORM#_#DBBASE_SUFFIX#"
#endif

// Start in COLOC mode
#ifndef BASE_PORT
    #define BASE_PORT 11000
#endif

// used by core slaves
#ifndef PORT_RANGE_LARGE
    #define PORT_RANGE_LARGE 20
#endif

// used by all other instances
#ifndef PORT_RANGE_SMALL
    #define PORT_RANGE_SMALL 12
#endif

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
#define OSDK_AUX_MASTER_PORT 		(BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*10))
#define OSDK_AUX_SLAVE1_PORT 		(BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*11))
#define OSDK_AUX_SLAVE2_PORT 		(BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*12))
#define GDPR_SLAVE_PORT             (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*13))
#define GP_MASTER1_PORT             (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*14))
#define GP_MASTER2_PORT             (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*15))
#define GP_SLAVE1_PORT              (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*16))
#define GP_SLAVE2_PORT              (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*17))

//These are from Urraca.0 but we don't need them 
// FIFA SPECIFIC CODE BEGIN
//#define AUXSLAVE1_PORT              (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*18))
//#define AUXSLAVE2_PORT              (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*19))
//#define AUXMASTER1_PORT             (BASE_PORT + (PORT_RANGE_LARGE*2) + (PORT_RANGE_SMALL*20))
// FIFA SPECIFIC CODE END

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
    { type="osdkAuxMaster", name="osdkAuxMaster", port=#OSDK_AUX_MASTER_PORT# }
    { type="osdkAuxSlave", name="osdkAuxSlave1", port=#OSDK_AUX_SLAVE1_PORT# }
    { type="osdkAuxSlave", name="osdkAuxSlave2", port=#OSDK_AUX_SLAVE2_PORT# }
	{ type="gdprAuxSlave", name="gdprAuxSlave", port=#GDPR_SLAVE_PORT# }
	
    //These are from Urraca.0 but we don't need them 
	// FIFA SPECIFIC CODE BEGIN
	//{ type="exampleAuxSlave", name="exampleSlave1", port=#AUXSLAVE1_PORT# }
    //{ type="exampleAuxSlave", name="exampleSlave2", port=#AUXSLAVE2_PORT# }
    //{ type="exampleAuxMaster", name="exampleMaster", port=#AUXMASTER1_PORT# }
	// FIFA SPECIFIC CODE END
#else
    { type="configMaster", name="configMaster", port=#CONFIG_MASTER_PORT# }
    { type="coreMaster", name="coreMaster1", port=#CORE_MASTER1_PORT# }
    { type="coreMaster", name="coreMaster2", port=#CORE_MASTER2_PORT# }
    { type="coreSlave", name="coreSlave1", port=#CORE_SLAVE1_PORT# }
    { type="coreSlave", name="coreSlave2", port=#CORE_SLAVE2_PORT# }
#endif
]
#endif

#if PLATFORM == "xone" || PLATFORM == "xbsx"
    #define BASE_PORT BASE_PORT + 20
#elif PLATFORM == "ps4" || PLATFORM == "ps5"
    #define BASE_PORT BASE_PORT + 40
#endif

#define SERVER_INSTANCES "\"configMaster:43700\",\"auxMaster:43710\",\"slave:slave1:43720\",\"slave:slave2:43730\",\"osdkAuxMaster:osdkAuxM:43740\",\"osdkAuxSlave:osdkAuxS1:43750\",\"osdkAuxSlave:osdkAuxS2:43760\",\"coreNSMaster:43770\""

#include "osdk/env/osdk_local.boot"
#include "blaze.boot"

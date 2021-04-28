// arsondefines.cfg does not exist in perforce. This is generated on the fly by arson due to options chosen in Arson GUI. 
#include "arsondefines.cfg" 

#define ARSON 1
#define ENV "dev"

#define SSL_KEY "ssl/key.pem"
#define SSL_CERT "ssl/cert.pem" 
#define INTERNAL_SSL_KEY "ssl/key.pem"
#define INTERNAL_SSL_CERT "ssl/cert.pem" 

#define ENABLE_STADIA_LOCAL_ENV 0 

#ifdef ENABLE_STADIA_LOCAL_ENV
    #if ENABLE_STADIA_LOCAL_ENV == "1"
        #define USE_LOCALHOST_NETWORKING 1
        #define HOST_OVERRIDE "localhost"
        #define INT_HOST_OVERRIDE "localhost"
    #endif
#endif

#define CCS_POOL "gsqa2"

// Determines whether ESS Vault will be used for secrets
#define VAULT_ENABLED "false"

// These values are only relevant if Vault is enabled, and are common
// for either shared namespace or team-specific namespace
#define VAULT_SECRET_PATH "secretid.cfg"
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
#endif

#ifndef PLATFORM
    #define PLATFORM "pc"
#endif

#ifndef BLAZE_SERVICE_NAME_PREFIX
    #define BLAZE_SERVICE_NAME_PREFIX "gosblaze-#DEV_USER_NAME_LOWER#"
#endif

//ARSONGREEN_TODO - Need Arson to be upgraded with new client ids before we can turn following into true.
#define VERIFY_AUTH_SOURCE "false"

#ifndef DBHOST
    #define DBHOST "gossdevmydb004.bio-sjc.ea.com"
#endif
#ifndef DBPORT
    #define DBPORT 3306
#endif

#ifndef DBUSER
    #define DBUSER "gosqa"
#endif
#ifndef DBPASS
    #define DBPASS "gosqa"
#endif

// Async DB connections are enabled by default.  Set DB_ASYNC_CONNS to 0 to disable them.
#define DB_ASYNC_CONNS 1

// DB related stuff
// ARSONGREEN_TODO - Does Arson really have DB schemas on different hosts or we can combine all this?
// ARSONGREEN_TODO - Does the ARSON_DB_SCHEMA is a valid prefix?

#define MAIN_HOSTNAME "#DBHOST#"
#define MAIN_PORT #DBPORT#
#define MAIN_USERNAME "#DBUSER#"
#define MAIN_PASSWORD "#DBPASS#"
#define MAIN_MAXCONNCOUNT 6
#define MAIN_CLIENT "MYSQL"
#define MAIN_DB_ALIAS "main"
#define MAIN_DATABASE_PREFIX "#DBBASE_PREFIX#_#MAIN_DB_ALIAS#"

#define ASSOCIATIONLISTS_HOSTNAME "#DBHOST#"
#define ASSOCIATIONLISTS_PORT #DBPORT#
#define ASSOCIATIONLISTS_USERNAME "#DBUSER#"
#define ASSOCIATIONLISTS_PASSWORD "#DBPASS#"
#define ASSOCIATIONLISTS_MAXCONNCOUNT   2
#define ASSOCIATIONLISTS_HEALTHCHECK "false"
#define ASSOCIATIONLISTS_CLIENT "MYSQL"
#define ASSOCIATIONLISTS_DB_ALIAS "associationlists"
#define ASSOCIATIONLISTS_DATABASE_PREFIX "#DBBASE_PREFIX#_#ASSOCIATIONLISTS_DB_ALIAS#"

#define STATS_HOSTNAME "#DBHOST#"
#define STATS_PORT #DBPORT#
#define STATS_USERNAME "#DBUSER#"
#define STATS_PASSWORD "#DBPASS#"
#define STATS_MAXCONNCOUNT  2
#define STATS_HEALTHCHECK "false"
#define STATS_CLIENT "MYSQL"
#define STATS_DB_ALIAS "stats"
#define STATS_DATABASE_PREFIX "#DBBASE_PREFIX#_#STATS_DB_ALIAS#"


#define LEADERBOARDS_HOSTNAME "#DBHOST#"
#define LEADERBOARDS_PORT #DBPORT#
#define LEADERBOARDS_USERNAME "#DBUSER#"
#define LEADERBOARDS_PASSWORD "#DBPASS#"
#define LEADERBOARDS_MAXCONNCOUNT   2
#define LEADERBOARDS_HEALTHCHECK "false"
#define LEADERBOARDS_CLIENT "MYSQL"
#define LEADERBOARDS_DB_ALIAS "leaderboards"
#define LEADERBOARDS_DATABASE_PREFIX "#DBBASE_PREFIX#_#LEADERBOARDS_DB_ALIAS#"

#define MESSAGING_HOSTNAME "#DBHOST#"
#define MESSAGING_PORT #DBPORT#
#define MESSAGING_USERNAME "#DBUSER#"
#define MESSAGING_PASSWORD "#DBPASS#"
#define MESSAGING_MAXCONNCOUNT 2
#define MESSAGING_HEALTHCHECK "false"
#define MESSAGING_CLIENT "MYSQL"
#define MESSAGING_DB_ALIAS "messaging"
#define MESSAGING_DATABASE_PREFIX "#DBBASE_PREFIX#_#MESSAGING_DB_ALIAS#"


#define CLUBS_HOSTNAME "#DBHOST#"
#define CLUBS_PORT #DBPORT#
#define CLUBS_DATABASE "#DBBASE_PREFIX#"
#define CLUBS_USERNAME "#DBUSER#"
#define CLUBS_PASSWORD "#DBPASS#"
#define CLUBS_MAXCONNCOUNT 6
#define CLUBS_HEALTHCHECK "false"
#define CLUBS_CLIENT "MYSQL"
#define CLUBS_DB_ALIAS "clubs"
#define CLUBS_DATABASE_PREFIX "#DBBASE_PREFIX#_#CLUBS_DB_ALIAS#"

#define USERSESSIONS_HOSTNAME "#DBHOST#"
#define USERSESSIONS_PORT #DBPORT#
#define USERSESSIONS_DATABASE "#DBBASE_PREFIX#"
#define USERSESSIONS_USERNAME "#DBUSER#"
#define USERSESSIONS_PASSWORD "#DBPASS#"
#define USERSESSIONS_MAXCONNCOUNT 2
#define USERSESSIONS_HEALTHCHECK "false"
#define USERSESSIONS_CLIENT "MYSQL"
#define USERSESSIONS_DB_ALIAS "usersessions"
#define USERSESSIONS_DATABASE_PREFIX "#DBBASE_PREFIX#_#USERSESSIONS_DB_ALIAS#"


#define GAMEREPORTING_HOSTNAME "#DBHOST#"
#define GAMEREPORTING_PORT #DBPORT#
#define GAMEREPORTING_DATABASE "#DBBASE_PREFIX#"
#define GAMEREPORTING_USERNAME "#DBUSER#"
#define GAMEREPORTING_PASSWORD "#DBPASS#"
#define GAMEREPORTING_MAXCONNCOUNT 2
#define GAMEREPORTING_HEALTHCHECK "false"
#define GAMEREPORTING_CLIENT "MYSQL"
#define GAMEREPORTING_DB_ALIAS "gamereporting"
#define GAMEREPORTING_DATABASE_PREFIX "#DBBASE_PREFIX#_#GAMEREPORTING_DB_ALIAS#"

#define UTIL_HOSTNAME "#DBHOST#"
#define UTIL_PORT #DBPORT#
#define UTIL_DATABASE "#DBBASE_PREFIX#"
#define UTIL_USERNAME "#DBUSER#"
#define UTIL_PASSWORD "#DBPASS#"
#define UTIL_MAXCONNCOUNT 2
#define UTIL_HEALTHCHECK "false"
#define UTIL_CLIENT "MYSQL"
#define UTIL_DB_ALIAS "util"
#define UTIL_DATABASE_PREFIX "#DBBASE_PREFIX#_#UTIL_DB_ALIAS#"


#define ARSON_HOSTNAME "#DBHOST#"
#define ARSON_PORT #DBPORT#
#define ARSON_DATABASE "#DBBASE_PREFIX#"
#define ARSON_USERNAME "#DBUSER#"
#define ARSON_PASSWORD "#DBPASS#"
#define ARSON_MAXCONNCOUNT 2
#define ARSON_HEALTHCHECK "false"
#define ARSON_CLIENT "MYSQL"
#define ARSON_DB_ALIAS "arson"
#define ARSON_DATABASE_PREFIX "#DBBASE_PREFIX#_#ARSON_DB_ALIAS#"


#define GAMEMANAGER_HOSTNAME "#DBHOST#"
#define GAMEMANAGER_PORT #DBPORT#
#define GAMEMANAGER_DATABASE "#DBBASE_PREFIX#"
#define GAMEMANAGER_USERNAME "#DBUSER#"
#define GAMEMANAGER_PASSWORD "#DBPASS#"
#define GAMEMANAGER_MAXCONNCOUNT 2
#define GAMEMANAGER_HEALTHCHECK "false"
#define GAMEMANAGER_CLIENT "MYSQL"
#define GAMEMANAGER_DB_ALIAS "gamemanager"
#define GAMEMANAGER_DATABASE_PREFIX "#DBBASE_PREFIX#_#GAMEMANAGER_DB_ALIAS#"


// Obfuscator is used to decode obfuscated password
// Empty obfuscator means password is not obfuscated
// Use tools/obfuscate to create obfuscated password
#define DBOBFUSCATOR ""
#define DBMAX 8















// Start in COLOC mode
#ifndef BASE_PORT
    #define BASE_PORT 10000
#endif

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

#include "blaze.boot"

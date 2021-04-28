// Blazeserver Boot file to run a local windows stress test
// Default configuration uses a local DB
//
// Run Blazeserver with the following arguments
//     bin\blazeserver.exe -DPLATFORM=pc -DGAMEPLAY_USER_ACCESS_ALL=1 --dbdestructive --logdir logs --logname stress --bootFileOverride ../etc.ilt.sports/sports.boot env/localstress.boot
// Run Stress clients with the following arguments (this example runs mm quick match only)
//     bin\stress.exe -c ../etc.ilt.sports/stress/stress-integrated.cfg -DSERVER_CONFIG=../etc.ilt.sports/stress/integrated/stress-integrated-servers-local.cfg -DSTRESS_SECURE=false -DCONN_INSTANCE_DELAY=533 -DNUM_CONNECTIONS=250 -DTEST_CONFIG=../etc.ilt.sports/stress/integrated/stress-fifa-mm-quick.cfg -DSTART_INDEX=10000 

#ifndef PLATFORM
#define PLATFORM "pc"
#endif

#ifndef ENV
#define ENV "dev"
#endif

#define NUCLEUS_ENV "lt"

#define DEFAULT_LOGGING "INFO"

#define DBHOST "gossdevmydb004.bio-sjc.ea.com"
#define DBSLAVE1 "gossdevmydb004.bio-sjc.ea.com"
#define DBSLAVE2 "gossdevmydb004.bio-sjc.ea.com"
#define DBPORT 3306

#define STATS_DBHOST "gossdevmydb004.bio-sjc.ea.com"
#define STATS_DBSLAVE1 "gossdevmydb004.bio-sjc.ea.com"
#define STATS_DBSLAVE2 "gossdevmydb004.bio-sjc.ea.com"

// These variables were needed to be overridden for ILT.
// Real world PROD usage may need to specify *different* values.
#define EXTERNAL_FIRE_RATE_LIMITER ""
#define EXTERNAL_FIRE_MAX_CONNS 25000
#define UED_GEO_IP "true"
#define TOTAL_LOGINS_PER_MIN 5000
#define GAMEPLAY_USER_STRESS_TESTER "true"
#define DEDICATED_SERVER_STRESS_TESTER "true"

// developer please create and use your own schema/login
// ** DO NOT use demo database blaze_user **
// please use the form unixusername if you have not yet created one
// Or you can customize DBBASE_PREFIX, DBUSER, DBPASS individually below
#ifndef DBBASE_PREFIX
    #define DBBASE_PREFIX "#DEV_USER_NAME#"
#endif
#ifndef DBUSER
    #define DBUSER "blaze_dev"
#endif
#ifndef DBPASS
    #define DBPASS "blaze_dev"
#endif

// Obfuscator is used to decode obfuscated password
// Empty obfuscator means password is not obfuscated
// Use tools/obfuscate to create obfuscated password
#define DBOBFUSCATOR ""
#define DBMAX 8
#define DEFAULT_DBMAXPING "600ms"

//#define PLATFORM "pc"
#define BASE_PORT 10000
#define ALLOW_STRESS_LOGIN "true"

#define INACTIVITY_TIMEOUT "500s"
#define COMMAND_TIMEOUT "500s"
//#define INACTIVITY_TIMEOUT "20s"
//#define COMMAND_TIMEOUT "10s

// override values in slivers.cfg
#define GAMEREPORTING_SLIVERCOUNT 10
#define GAMEMANAGER_SLIVERCOUNT 10
#define GAMEMANAGER_PARTITIONCOUNT 0
#define USERSESSIONS_SLIVERCOUNT 10

#define NUCLEUS2CONNECTHOST "https://accounts.lt.internal.ea.com"
#define NUCLEUS2PROXYHOST "https://gateway.lt.internal.ea.com"
#define NUCLEUS2REDIRECT "http://127.0.0.1/success"
#define NUCLEUS2DISPLAY "console2/welcome"

#define NUCLEUS2CONNECTHOSTEXT "https://accounts.lt.ea.com"
#define NUCLEUS2CONNECTHOSTEXTTRUSTED "https://accounts2s.lt.ea.com"
#define NUCLEUS2PROXYHOSTEXT "https://gateway.lt.ea.com"
#define NUCLEUS2PORTALHOST "https://signin.lt.ea.com"

#include "env/local.boot"

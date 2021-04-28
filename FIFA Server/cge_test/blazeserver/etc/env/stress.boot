#ifndef ENV
#define ENV "test"
#endif

#define NUCLEUS_ENV "lt"

#define PIN_RELEASE_TYPE_DEFAULT "load_test"

#define DEFAULT_LOGGING "INFO"

#define DBHOST "470996-gosltmdb660.rspc-iad.ea.com"
#define DBSLAVE1 "470997-gosltmdb661.rspc-iad.ea.com"
#define DBSLAVE2 "470998-gosltmdb662.rspc-iad.ea.com"
#define DBPORT 3306

#define STATS_DBHOST "470999-gosltmdb663.rspc-iad.ea.com"
#define STATS_DBSLAVE1 "471000-gosltmdb664.rspc-iad.ea.com"
#define STATS_DBSLAVE2 "471001-gosltmdb665.rspc-iad.ea.com"

// These variables were needed to be overridden for ILT.
// Real world PROD usage may need to specify *different* values.
#define EXTERNAL_FIRE_RATE_LIMITER ""
#define EXTERNAL_GRPC_RATE_LIMITER ""
#define EXTERNAL_FIRE_MAX_CONNS 25000
#define EXTERNAL_GRPC_MAX_CONNS 25000
#define UED_GEO_IP "true"
#define TOTAL_LOGINS_PER_MIN 5000
#define GAMEPLAY_USER_STRESS_TESTER "true"
#define DEDICATED_SERVER_STRESS_TESTER "true"

// developer please create and use your own schema/login
// ** DO NOT use demo database blaze_user **
// please use the form unixusername if you have not yet created one
// Or you can customize DBBASE_PREFIX, DBUSER, DBPASS individually below
#ifndef DBBASE_PREFIX
    #define DBBASE_PREFIX "gos_ilt_01"
#endif
#ifndef DBUSER
    #define DBUSER "gos_ilt"
#endif
#ifndef DBPASS
    #define DBPASS "gos_ilt_123"
#endif
// Obfuscator is used to decode obfuscated password
// Empty obfuscator means password is not obfuscated
// Use tools/obfuscate to create obfuscated password
#define DBOBFUSCATOR ""
#define DBMAX 30

#define EVENT_CACHE_SIZE 200

#define REDIRECTOR_ADDRESS "stress.internal.gosredirector.stest.ea.com"

#define GAMEREPORTING_SLIVERCOUNT 500

#include "blaze.boot"
#include "osdk/env/osdk_stress.boot"
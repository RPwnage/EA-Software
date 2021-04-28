// created from ILT control script
//
// The original template file includes double-backslash which denote
// escaping the following character when processed by the control script.

#ifndef ENV
    #define ENV \\"test\\"
#endif
#define DEFAULT_LOGGING \\"INFO\\"

#define DBHOST \\"470996-gosltmdb660.rspc-iad.ea.com\\"
#define DBSLAVE1 \\"470997-gosltmdb661.rspc-iad.ea.com\\"
#define DBSLAVE2 \\"470998-gosltmdb662.rspc-iad.ea.com\\"
#define DBPORT 3306

#define STATS_DBHOST \\"470999-gosltmdb663.rspc-iad.ea.com\\"
#define STATS_DBSLAVE1 \\"471000-gosltmdb664.rspc-iad.ea.com\\"
#define STATS_DBSLAVE2 \\"471001-gosltmdb665.rspc-iad.ea.com\\"

// These variables were needed to be overridden for ILT.
// Real world PROD usage may need to specify *different* values.
#define EXTERNAL_FIRE_RATE_LIMITER \\"\\"
#define EXTERNAL_FIRE_MAX_CONNS 25000
#define UED_GEO_IP \\"true\\"
#define TOTAL_LOGINS_PER_MIN 5000
#define GAMEPLAY_USER_STRESS_TESTER \\"true\\"
#define DEDICATED_SERVER_STRESS_TESTER \\"true\\"

// developer please create and use your own schema/login
// ** DO NOT use demo database blaze_user **
// please use the form unixusername if you have not yet created one
// Or you can customize DBBASE_PREFIX, DBUSER, DBPASS individually below
#define DBBASE_PREFIX \\"gos_ilt_01\\"
#define DBUSER \\"gos_ilt\\"
#define DBPASS \\"gos_ilt_123\\"
// Obfuscator is used to decode obfuscated password
// Empty obfuscator means password is not obfuscated
// Use tools/obfuscate to create obfuscated password
#define DBOBFUSCATOR \\"\\"
#define DBMIN 10
#define DBMAX 30

#define NUCLEUSHOST \\"https://nucleus.ears.lt.ea.com\\"
#define NUCLEUS2CONNECTHOST \\"https://accounts.lt.internal.ea.com\\"
#define NUCLEUS2PROXYHOST \\"https://gateway.lt.internal.ea.com\\"
#define NUCLEUS2REDIRECT \\"http://127.0.0.1/success\\"
#define NUCLEUS2DISPLAY \\"console/welcome\\"
#define XBLTOKENURN \\"accounts.lt.internal.ea.com\\"

#define NUCLEUS2CONNECTHOSTEXT \\"https://accounts.lt.ea.com\\"
#define NUCLEUS2CONNECTHOSTEXTTRUSTED \\"https://accounts2s.lt.ea.com\\"
#define NUCLEUS2PROXYHOSTEXT \\"https://gateway.lt.ea.com\\"

#define NOTIFYHOST \\"https://notify.lt.gameservices.ea.com:8097\\"

#define EVENT_CACHE_SIZE 200

#define REDIRECTOR_ADDRESS \\"stress.internal.gosredirector.stest.ea.com\\"

#include \\"blaze.boot\\"

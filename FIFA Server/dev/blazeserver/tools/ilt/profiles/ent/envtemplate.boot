// created from ILT control script
//
// The original template file includes double-backslash which denote
// escaping the following character when processed by the control script.

#define NUCLEUS_ENV \\"lt\\"

#define DBHOST \\"gs-sandbox-mysql-#ILT_POOL#-01#ILT_DOMAIN#\\"
#define DBSLAVE1 \\"gs-sandbox-mysql-#ILT_POOL#-02#ILT_DOMAIN#\\"

#define SSL_KEY \\"ssl/key.pem\\"
#define SSL_CERT \\"ssl/cert.pem\\"
#define INTERNAL_SSL_KEY \\"#SSL_KEY#\\"
#define INTERNAL_SSL_CERT \\"#SSL_CERT#\\"

#ifndef ENV
    #define ENV \\"test\\"
#endif
#define DEFAULT_LOGGING \\"INFO\\"

// These values are only relevant if Vault is enabled, and are common
// for either shared namespace or team-specific namespace
#define SSL_PATH_TYPE \\"VAULT_SHARED\\"

// If Vault is enabled, this determines whether the newer shared namespace will be used,
// or an older team-specific namespace
#define VAULT_SHARED_NAMESPACE \\"false\\"

#if VAULT_SHARED_NAMESPACE == \\"true\\"
    #define VAULT_ROLE_ID \\"gs-dev\\"
    #define VAULT_DBNAME \\"#ENV#/gs-dev-common\\"
#else
    #define VAULT_NAMESPACE \\"eadp-gs-blaze-dev\\"
    #define VAULT_ROLE_ID \\"#ILT_VAULT_ROLE_ID#\\"
    #define VAULT_ROOT \\"#ILT_VAULT_ROOT_PREFIX#-#PLATFORM#\\"
#endif

#define DBPORT 3306
#define DB_ASYNC_CONNS 1

#define STATS_DBHOST \\"#DBHOST#\\"
#define STATS_DBSLAVE1 \\"#DBSLAVE1#\\"

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
#if PLATFORM == \\"common\\"
    #define DBBASE_PREFIX \\"blaze\\"
#else
    #define DBBASE \\"blaze\\"
#endif
#define DBUSER \\"blaze\\"
#define DBPASS \\"Abcdefghijklmno-12345\\"
// Obfuscator is used to decode obfuscated password
// Empty obfuscator means password is not obfuscated
// Use tools/obfuscate to create obfuscated password
#define DBOBFUSCATOR \\"\\"
#define DBMAX 30

#define EVENT_CACHE_SIZE 200

#define REDIRECTOR_ADDRESS \\"stress.internal.gosredirector.stest.ea.com\\"

#include \\"blaze.boot\\"

#ifndef ENV
    #define ENV "test"
#endif

#ifndef DBBASE
    #define DBBASE "fifa_2022_#PLATFORM#"
#endif

#ifdef DBBASE_SUFFIX
    #define DBBASE "#DBBASE#_#DBBASE_SUFFIX#"
#endif

#if (ENV != "prod") && (ENV != "dev")
    #define DBBASE "#DBBASE#_#ENV_NAME#"
#endif

#define DBPORT 3306
#if ENV == "dev"
    #define DBHOST "eu1gssdevdb1-01.bio-dub.ea.com"
    #define DBUSER "fifa_user"
    #define DBPASS "fifa_user"
    #define DBMIN 1
    #define DBMAX 16
#else
    #if (ENV == "test") || (ENV == "cert")
        #define DBHOST "fifa22-stest-write.bio-dub.ea.com"
        #define DBSLAVEHOST "fifa22-stest-read.bio-dub.ea.com"
        #define DBPASS "K_hdgf76mnbvglkdfg123"
    #endif

    #define DBUSER "fifa_2022"
    #define DBMIN 6
    #define DBMAX 32
#endif
#define DBOBFUSCATOR ""
#define DBCLIENT "MYSQL"

#define CLUBS_PERIOD_ROLLOVER   0
#define CLUBS_PERIOD_SEASON     0x8

#define UED_GEO_IP "true"
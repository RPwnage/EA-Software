#define ENV "test"
#define FRIENDLY_NAME "FIFA 2022"
#define ENV_NAME "load"
#ifndef NUCLEUS_IAD1
    #define NUCLEUS_IAD1 "true"
#endif

#ifndef LT_REDIRECTOR
    #define LT_REDIRECTOR "true"
#endif

#if LT_REDIRECTOR == "true"
	#define REDIRECTOR_ADDRESS "stress.internal.gosredirector.stest.ea.com"
#else
	//STEST_REDIRECTOR
	#define REDIRECTOR_ADDRESS "internal.gosredirector.stest.ea.com"
#endif

#ifndef MOCKSERVICE_ENABLED
    #define MOCKSERVICE_ENABLED "false"
#endif

#ifndef SONY_LOADTEST_ENABLED
    #define SONY_LOADTEST_ENABLED "false"
#endif

#define LFUHOST "sdevbesl01.online.ea.com"
#define LFUPORT 8000
#define LFUEXTPORT 80

#define BLAZE_SERVICE_NAME_DEFAULT "fifa-2022-#PLATFORM#-lt"

#if PLATFORM == "xone"
    #define DBBASE "fifa_2022_xone_main_lt"
	#define DBGAMEREPORTINGBASE "fifa_2022_xone_grnonfut_lt"
	#define DBFUTGAMEREPORTINGBASE "fifa_2022_xone_grfut_lt"
	#define DBSTATSBASE "fifa_2022_xone_stats_lt"
	#define DBHOST "fifa22-lt-g4-main-write.bio-dub.ea.com"
	#define DBSLAVE_HOST "fifa22-lt-g4-main-read.bio-dub.ea.com"
	#define DB_STATS_HOST "fifa22-lt-g4-stats-write.bio-dub.ea.com"
	#define DB_STATS_SLAVE_HOST "fifa22-lt-g4-stats-read.bio-dub.ea.com"
	#define DB_GAMEREPORTING_HOST "fifa22-lt-g4-grnf-write.bio-dub.ea.com"
	#define DB_GAMEREPORTING_SLAVE_HOST "fifa22-lt-g4-grnf-read.bio-dub.ea.com"
	#define DB_FUTGAMEREPORTING_HOST "fifa22-lt-g4-grfut-write.bio-dub.ea.com"
	#define DB_FUTGAMEREPORTING_SLAVE_HOST "fifa22-lt-g4-grfut-read.bio-dub.ea.com"
    #if MOCKSERVICE_ENABLED == "true"
		#define MOCK_EXTERNAL_SERVICE_URL_XONE "http://420350-fifa13ltapp21.rspc-lhr.ea.com:8080"
    #endif
#elif PLATFORM == "ps4"
    #define DBBASE "fifa_2022_ps4_main_lt"
	#define DBGAMEREPORTINGBASE "fifa_2022_ps4_grnonfut_lt"
	#define DBFUTGAMEREPORTINGBASE "fifa_2022_ps4_grfut_lt"
	#define DBSTATSBASE "fifa_2022_ps4_stats_lt"
	#define DBHOST "fifa22-lt-g4-main-write.bio-dub.ea.com"
	#define DBSLAVE_HOST "fifa22-lt-g4-main-read.bio-dub.ea.com"
	#define DB_STATS_HOST "fifa22-lt-g4-stats-write.bio-dub.ea.com"
	#define DB_STATS_SLAVE_HOST "fifa22-lt-g4-stats-read.bio-dub.ea.com"
	#define DB_GAMEREPORTING_HOST "fifa22-lt-g4-grnf-write.bio-dub.ea.com"
	#define DB_GAMEREPORTING_SLAVE_HOST "fifa22-lt-g4-grnf-read.bio-dub.ea.com"
	#define DB_FUTGAMEREPORTING_HOST "fifa22-lt-g4-grfut-write.bio-dub.ea.com"
	#define DB_FUTGAMEREPORTING_SLAVE_HOST "fifa22-lt-g4-grfut-read.bio-dub.ea.com"
	#if MOCKSERVICE_ENABLED == "true"
		#define MOCK_EXTERNAL_SERVICE_URL_PS4 "http://420350-fifa13ltapp21.rspc-lhr.ea.com:8000"
		#define MOCK_EXTERNAL_SERVICE_URL_PS5 "54.147.69.37:8080"
	#endif
	#if SONY_LOADTEST_ENABLED == "true"
        #define SONY_LOADTEST_URL "https://auth.api.sp-int.ac.playstation.net"
	#endif

#elif PLATFORM == "xbsx"
    #define DBBASE "fifa_2022_xbsx_main_lt"
	#define DBGAMEREPORTINGBASE "fifa_2022_xbsx_grnonfut_lt"
	#define DBFUTGAMEREPORTINGBASE "fifa_2022_xbsx_grfut_lt"
	#define DBSTATSBASE "fifa_2022_xbsx_stats_lt"
	#define DBHOST "fifa22-lt-g5-main-write.bio-dub.ea.com"
	#define DBSLAVE_HOST "fifa22-lt-g5-main-read.bio-dub.ea.com"
	//#define DB_MAIL_ASSOC_HOST "dubfifal0140.bio-dub.ea.com"
	//#define DB_MAIL_ASSOC_SLAVE1 "gosltmdb0867.bio-dub.ea.com"
	#define DB_STATS_HOST "fifa22-lt-g5-stats-write.bio-dub.ea.com"
	#define DB_STATS_SLAVE_HOST "fifa22-lt-g5-stats-read.bio-dub.ea.com"
	#define DB_GAMEREPORTING_HOST "fifa22-lt-g5-grnf-write.bio-dub.ea.com"
	#define DB_GAMEREPORTING_SLAVE_HOST "fifa22-lt-g5-grnf-read.bio-dub.ea.com"
	#define DB_FUTGAMEREPORTING_HOST "fifa22-lt-g5-grfut-write.bio-dub.ea.com"
	#define DB_FUTGAMEREPORTING_SLAVE_HOST "fifa22-lt-g5-grfut-read.bio-dub.ea.com"
	#if MOCKSERVICE_ENABLED == "true"
		#define MOCK_EXTERNAL_SERVICE_URL_PS4 "http://420350-fifa13ltapp21.rspc-lhr.ea.com:8000"
		#define MOCK_EXTERNAL_SERVICE_URL_PS5 "54.147.69.37:8080"
	#endif
	#if SONY_LOADTEST_ENABLED == "true"
        #define SONY_LOADTEST_URL "https://auth.api.sp-int.ac.playstation.net"
	#endif

#elif PLATFORM == "ps5"
    #define DBBASE "fifa_2022_ps5_main_lt"
	#define DBGAMEREPORTINGBASE "fifa_2022_ps5_grnonfut_lt"
	#define DBFUTGAMEREPORTINGBASE "fifa_2022_ps5_grfut_lt"
	#define DBSTATSBASE "fifa_2022_ps5_stats_lt"
	#define DBHOST "fifa22-lt-g5-main-write.bio-dub.ea.com"
	#define DBSLAVE_HOST "fifa22-lt-g5-main-read.bio-dub.ea.com"
	//#define DB_MAIL_ASSOC_HOST "dubfifal0140.bio-dub.ea.com"
	//#define DB_MAIL_ASSOC_SLAVE1 "gosltmdb0867.bio-dub.ea.com"
	#define DB_STATS_HOST "fifa22-lt-g5-stats-write.bio-dub.ea.com"
	#define DB_STATS_SLAVE_HOST "fifa22-lt-g5-stats-read.bio-dub.ea.com"
	#define DB_GAMEREPORTING_HOST "fifa22-lt-g5-grnf-write.bio-dub.ea.com"
	#define DB_GAMEREPORTING_SLAVE_HOST "fifa22-lt-g5-grnf-read.bio-dub.ea.com"
	#define DB_FUTGAMEREPORTING_HOST "fifa22-lt-g5-grfut-write.bio-dub.ea.com"
	#define DB_FUTGAMEREPORTING_SLAVE_HOST "fifa22-lt-g5-grfut-read.bio-dub.ea.com"
	#if MOCKSERVICE_ENABLED == "true"
		#define MOCK_EXTERNAL_SERVICE_URL_PS4 "http://420350-fifa13ltapp21.rspc-lhr.ea.com:8000"
		#define MOCK_EXTERNAL_SERVICE_URL_PS5 "54.147.69.37:8080"
	#endif
	#if SONY_LOADTEST_ENABLED == "true"
        #define SONY_LOADTEST_URL "https://auth.api.sp-int.ac.playstation.net"
	#endif

#elif PLATFORM == "stadia"
    #define DBBASE "fifa_2022_stadia_all_lt"
    #define DBHOST "fifa22-lt-g5-main-write.bio-dub.ea.com"
    #define DBSLAVE_HOST "fifa22-lt-g5-main-read.bio-dub.ea.com"

#elif PLATFORM == "pc"
    #define DBBASE "fifa_2022_pc_all_lt"
    #define DBHOST "fifa22-lt-pnx-write.bio-dub.ea.com"
    #define DBSLAVE_HOST "fifa22-lt-pnx-read.bio-dub.ea.com"
#endif

#define DBPORT 3306
#define DBNEWPORT 3306
#define DBUSER "fifa_2022"
#define DBPASS "MHJmUENhUnJQc0_0rNFRIV2_1ETlJOUT09"
#define DBOBFUSCATOR ""
#define DBMAX 16
#define DBMIN 6
#define DBCLIENT "MYSQL"

#define CSEMAILHOST "betacsrapp01.beta.eao.abn-iad.ea.com:8001"

#if PLATFORM == "xone"
    #define PROJECTID "318781"
#elif PLATFORM == "ps4"
	#define PROJECTID "318779"
#elif PLATFORM == "xbsx"
	#define PROJECTID "318978"
#elif PLATFORM == "ps5"
	#define PROJECTID "318982"
#elif PLATFORM == "stadia"
	#define PROJECTID "319764"
#else
    #define PROJECTID "318776"	
#endif

// This is an environmental specific configuration item. See matchmaker configuration for more details
#define EVALUATE_CONNECTIBILITY_DURING_CREATE_GAME_FINALIZATION "false"

#define ALLOW_STRESS_LOGIN "true"

#define BASE_PORT ""
#define CONFIG_MASTER_PORT ""
#define AUX_MASTER_PORT ""

#define CLUBS_PERIOD_ROLLOVER   0
#define CLUBS_PERIOD_SEASON     0x8

#define UED_GEO_IP "true"
//#include "env/common.boot"
#include "osdk/env/osdk_testlt.boot"
#include "blaze.boot"

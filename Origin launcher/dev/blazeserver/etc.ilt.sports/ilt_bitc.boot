#define GAMEMANAGER_SLIVERCOUNT 1000
#define GAMEMANAGER_PARTITIONCOUNT 0
#define USERSESSIONS_SLIVERCOUNT 1000

#define DEFAULT_LOGGING "INFO"

#ifndef LOG_OUTPUT
    #define LOG_OUTPUT "stdout"
#endif

#ifndef NUCLEUS_ENV
    #define NUCLEUS_ENV "lt"
#endif
 
#define DB_ASYNC_CONNS 1

// These variables were needed to be overridden for ILT.
// Real world PROD usage may need to specify *different* values.
#define EXTERNAL_FIRE_RATE_LIMITER ""
#define EXTERNAL_FIRE_MAX_CONNS 25000
#define UED_GEO_IP "true"
#define TOTAL_LOGINS_PER_MIN 5000
#define GAMEPLAY_USER_STRESS_TESTER "true"
#define DEDICATED_SERVER_STRESS_TESTER "true"

#define DBMAX 30

#define EVENT_CACHE_SIZE 200

#define REDIRECTOR_ADDRESS "stress.internal.gosredirector.stest.ea.com"

#define WEBHOST "gos.stest.ea.com"
#define CONTENTHOST "goscontent.stest.ea.com"
#define WEBDIR "Ping"
// WebTitle should only be used when the title of the game differs from the WEBDIR value.  Games that release
// once per product year won't have to set WEBTITLE explicitly.  Ping had to because we release multiple 
// versions in a single product year.
#define WEBTITLE "PingV6"
#define WEBYEAR 2011

// Address of the Sports World server
#define EASWHOST "https://secure.easportsworld.ea.com:443/ping11test"
#define EASWREQHOST "http://pg.ping11.test.easportsworld.ea.com:8309"
#define EASWMEDIAHOST "http://pg.ping11.test.easportsworld.ea.com:8309"
#define RS4HOST "http://blah"
#define ESPN_NEWS_DBG ""


#define WEBHOST "gos.stest.ea.com"
#define CONTENTHOST "goscontent.stest.ea.com"
#define WEBDIR "Ping"
// WebTitle should only be used when the title of the game differs from the WEBDIR value.  Games that release
// once per product year won't have to set WEBTITLE explicitly.  Ping had to because we release multiple 
// versions in a single product year.
#define WEBTITLE "PingV6"
#ifndef WEBTITLE
#define WEBTITLE "#WEBDIR#"
#endif
#define WEBYEAR 2011

// Address of the Sports World server
#define EASWHOST "https://secure.easportsworld.ea.com:443/ping11test"
#define EASWREQHOST "http://pg.ping11.test.easportsworld.ea.com:8309"
#define EASWMEDIAHOST "http://pg.ping11.test.easportsworld.ea.com:8309"
#define RS4HOST "http://blah"
#define ESPN_NEWS_DBG ""


rootConfigDirectories = [".","../etc.ilt.sports"]



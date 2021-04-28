#define ENV "prod"
#define ENV_NAME "retailtest1"

#include "env/common.boot"

// override defaults
#define DBHOST "fifa22-retailtest1-write.bio-dub.ea.com"
#define DBSLAVEHOST "fifa22-retailtest1-read.bio-dub.ea.com"
#define DBPASS "fifa_2022b3NHcpkX-G94QxeC5Q9"
#define DBBASE "fifa_2022_#PLATFORM#_qvt"

#define EADP_STATS_CONTEXT_ID "FIFA22QVT"

#include "osdk/env/osdk_retailtest1.boot"
#include "blaze.boot"


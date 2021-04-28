#define FIFA_SSF_HOST_SSL		"ssas.external.retailtest1.s1.ssf.ea.com"

#include "osdk/env/osdk_common.boot"

//This needs to be enabled in PROD and DEV/TEST only. It will fail cert if enabled on any cert environment.
#define PSPLUSUPSELL		"plus--branded_upsell--fifa22"

// FIFA Cheat Detection - Shield
#if (PLATFORM == "pc")
	#define SHIELD_ENABLED 1
	#define SHIELD_BACKEND_URL "http://fifa21-shield-pr.bio-dub.ea.com:19984"
#endif

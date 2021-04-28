#define ENV "test"
#define ENV_NAME "eacert1"

#include "env/common.boot"

//override defaults
#define DBHOST "fifa21-eacert-write-vip.bio-dub.ea.com"
#define DBSLAVEHOST "fifa21-eacert-read-vip.bio-dub.ea.com"
#define DBPASS "Tdgdfa+b3NHcpkX-G94QxhcfQ9ngJpHm"

#include "osdk/env/osdk_eacert1.boot"
#include "blaze.boot"


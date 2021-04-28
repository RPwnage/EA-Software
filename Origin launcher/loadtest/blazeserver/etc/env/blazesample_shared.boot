//
// Boot file used to deploy sample blaze server as a shared cluster but no crossplay support
//



#ifndef ENV
#define ENV "test"
#endif

#define PLATFORM "common"

#define DBHOST "eadpgos0049.bio-iad.ea.com"
#define DBSLAVEHOST "eadpgos0050.bio-iad.ea.com"
#define DBPORT 3306
#define DBBASE_PREFIX "blazesample_14_2_x_shared"
#define DBUSER "blaze_user"
#define DBPASS "blaze_user"
// Obfuscator is used to decode obfuscated password
// Empty obfuscator means password is not obfuscated
// Use tools/obfuscate to create obfuscated password
#define DBOBFUSCATOR ""
#define DBMAX 8

#include "blaze.boot"

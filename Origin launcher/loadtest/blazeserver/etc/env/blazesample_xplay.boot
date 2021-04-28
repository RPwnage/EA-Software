//
// Boot file used to deploy sample blaze server with crossplay support
//


#ifndef ENV
#define ENV "test"
#endif

#define PLATFORM "common"
#define BLAZESAMPLE_UNRESTRICTED_XPLAY 1 // If you want to turn off unrestricted xplay, comment out the define (just changing to 0 won't work)

#define DBHOST "eadpgos0049.bio-iad.ea.com"
#define DBSLAVEHOST "eadpgos0050.bio-iad.ea.com"
#define DBPORT 3306
#define DBBASE_PREFIX "blazesample_14_2_x_xplay"
#define DBUSER "blaze_user"
#define DBPASS "blaze_user"
// Obfuscator is used to decode obfuscated password
// Empty obfuscator means password is not obfuscated
// Use tools/obfuscate to create obfuscated password
#define DBOBFUSCATOR ""
#define DBMAX 8

#include "blaze.boot"

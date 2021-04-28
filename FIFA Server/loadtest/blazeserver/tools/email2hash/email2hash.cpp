#include "EABase/eabase.h"  //include eabase.h before any std lib includes for VS2012 (see EABase/2.02.00/doc/FAQ.html#Info.17 for details)

#define BUFFER_SIZE    512

#include <iostream>

#include "framework/blaze.h"
#include "framework/util/hashutil.h"

namespace email2hash
{

int usage(const char *arg0)
{
    std::cout << "Usage: " << arg0 << " email ...\n";
    std::cout << "  email : The email address that is to be converted into a hash.";
    return 1;
}

int email2hash_main(int argc, char** argv)
{
    if (argc <= 1 || blaze_strcmp(argv[1], "-h") == 0)
    {
        return usage(argv[0]);
    }

    const char8_t *email;
    char8_t hashBuffer[Blaze::HashUtil::SHA256_STRING_OUT];

    for(int i = 1; i < argc; i++)
    {
        email = argv[i];
        Blaze::HashUtil::generateSHA256Hash(email, strlen(email), hashBuffer, Blaze::HashUtil::SHA256_STRING_OUT);
        std::cout << "Hash for '" << email << "' is: " << hashBuffer << std::endl;
    }

    return 0;
}

}

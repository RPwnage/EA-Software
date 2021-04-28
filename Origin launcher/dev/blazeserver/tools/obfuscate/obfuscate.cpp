/*************************************************************************************************/
/*!
*     \File obfuscate.cpp
*
*     \Description
*         Produce obfuscated string to use as database password
*
*     \Notes
*         None
*
*     \Copyright
*         (c) Electronic Arts. All Rights Reserved.
*
*     \Version
*         06/02/2010 (divkovic) First version
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/database/dbobfuscator.h"

#include <stdio.h>
#include <stdlib.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace obfuscate
{

// Main
int obfuscate_main(int argc, char** argv)
{
    // Check for valid arguments
    if (argc < 3 || argc > 4 || (argc == 4 && strcmp(argv[1], "-d") != 0))
    {
        char *prgName = argv[0] + strlen(argv[0]);
        while(prgName != argv[0] && *(prgName - 1) != '\\' && *(prgName - 1) != '/')
            --prgName;
        printf("usage: %s [-d] OBFUSCATOR TARGET_STRING\n", prgName);
        printf("    -d             decode target (if this option is left out then obfuscate target)\n");
        printf("    OBFUSCATOR     magic phrase used in obfuscation/decoding\n");
        printf("    TARGET_STRING  target string to obfuscate/decode\n");
        exit(0);
    }

    char buffer[BUFFER_SIZE_256];
    
    size_t len = 0;

    if (argc == 3)
        len = Blaze::DbObfuscator::obfuscate(argv[1], argv[2], buffer, BUFFER_SIZE_256);
    else
        len = Blaze::DbObfuscator::decode(argv[2], argv[3], buffer, BUFFER_SIZE_256);

    if (len < BUFFER_SIZE_256)
    {
        printf("%s\n", buffer);
    }
    else
    {
        printf("Could not obfuscate: insufficient buffer size.\n");
    }

    return 0;
}

}

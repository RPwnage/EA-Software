/*H********************************************************************************/
/*!
    \File crypt.c

    \Description
        Test the crypto modules.

    \Copyright
        Copyright (c) 2013 Electronic Arts Inc.

    \Version 01/11/2013 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/crypt/cryptarc4.h"
#include "DirtySDK/crypt/cryptrand.h"

#include "testermodules.h"

#include "zlib.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

// Variables

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _CmdCryptTestArc4

    \Description
        Test the CryptArc4 module

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list
    
    \Output
        int32_t     - zero=success, else number of failed tests

    \Version 01/11/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdCryptTestArc4(ZContext *argz, int32_t argc, char *argv[])
{
    static const char *_strEncrypt[] =
    {
        "Test first string encryption.",
        "Another string to test encrypt.",
        "Strings are fun to encrypt!",
        "This string is exactly 127 characters long, sans null.  It is meant to test the behavior of Encrypt/Decrypt with a full buffer.",
        "This string is more than 127 characters long.  It will safely truncate the result string, which will result in the test failing.",
    };
    char strEncryptBuf[128], strDecryptBuf[128];
    uint8_t aEncryptKey[32];
    int32_t iFailed, iString;
    
    ZPrintf("%s: testing CryptArc4\n", argv[0]);

    // generate a random encryption key
    CryptRandGet(aEncryptKey, sizeof(aEncryptKey));

    // test string encryption/decryption
    for (iString = 0, iFailed = 0; iString < (signed)(sizeof(_strEncrypt)/sizeof(_strEncrypt[0])); iString += 1)
    {
        CryptArc4StringEncrypt(strEncryptBuf, sizeof(strEncryptBuf), _strEncrypt[iString], aEncryptKey, sizeof(aEncryptKey), 1);
        CryptArc4StringDecrypt(strDecryptBuf, sizeof(strDecryptBuf), strEncryptBuf, aEncryptKey, sizeof(aEncryptKey), 1);
        ZPrintf("%s: '%s'->'%s'\n", argv[0], _strEncrypt[iString], strEncryptBuf);
        if (strcmp(strDecryptBuf, _strEncrypt[iString]))
        {
            ZPrintf("%s: encrypt/decrypt failed; '%s' != '%s'\n", argv[0], _strEncrypt[iString], strDecryptBuf);
            iFailed += 1;
        }
    }

    ZPrintf("%s: ---------------------\n", argv[0]);

    // one test intentionally fails, so subtract that out
    iFailed -= 1;
    return(iFailed);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdCrypt

    \Description
        Test the crypto modules

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list
    
    \Output standard return value

    \Version 01/11/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdCrypt(ZContext *argz, int32_t argc, char *argv[])
{
    int32_t iFailed = 0;

    iFailed += _CmdCryptTestArc4(argz, argc, argv);

    ZPrintf("%s: %d tests failed\n", argv[0], iFailed);
    return(0);
}



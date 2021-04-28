/*H********************************************************************************/
/*!
    \File base64.c

    \Description
        Test the Base64 encoder and decoder

    \Copyright
        Copyright (c) 2018 Electronic Arts Inc.

    \Version 12/16/2018 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/util/base64.h"
#include "testermodules.h"

#include "libsample/zlib.h"
#include "libsample/zfile.h"
#include "libsample/zmem.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

// Variables

/*** Private Functions ************************************************************/


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CmdBase64

    \Description
        Test base64 encoding and decoding

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output standard return value

    \Version 12/16/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdBase64(ZContext *argz, int32_t argc, char *argv[])
{
    char *pInpData, *pOutData;
    int32_t iInpSize, iOutSize;
    ZFileT iOutFileId;

    // check arg count
    if ((argc < 3) || (argc > 4))
    {
        ZPrintf("usage: %s [decode|decodeurl|encode|encodeurl|encodeurlnopad] <inpfile> [outfile]\n", argv[0]);
    }

    // load input file
    if ((pInpData = ZFileLoad(argv[2], &iInpSize, FALSE)) == NULL)
    {
        ZPrintf("%s: unable to load input file '%s'\n", argv[0], argv[2]);
        return(-1);
    }

    // calculate output size
    if ((!ds_stricmp(argv[1], "decode")) || (!ds_stricmp(argv[1], "decodeurl")))
    {
        iOutSize = Base64DecodedSize(iInpSize);
    }
    else if ((!ds_stricmp(argv[1], "encode")) || (!ds_stricmp(argv[1], "encodeurl")) || (!ds_stricmp(argv[1], "encodeurlnopad")))
    {
        // add one for required null terminator
        iOutSize = Base64EncodedSize(iInpSize)+1;
    }
    else
    {
        ZPrintf("%s: unrecognized request '%s'\n", argv[0], argv[1]);
        ZMemFree(pInpData);
        return(-1);
    }

    // allocate output buffer
    if ((pOutData = ZMemAlloc(iOutSize)) == NULL)
    {
        ZPrintf("%s: unable to allocate output buffer\n", argv[0]);
        ZMemFree(pInpData);
        return(-1);
    }

    // do the decode/encode
    if (!ds_stricmp(argv[1], "decode"))
    {
        iOutSize = Base64Decode3(pInpData, iInpSize, pOutData, iOutSize);
    }
    else if (!ds_stricmp(argv[1], "decodeurl"))
    {
        iOutSize = Base64DecodeUrl(pInpData, iInpSize, pOutData, iOutSize);
    }
    else if (!ds_stricmp(argv[1], "encode"))
    {
        iOutSize = Base64Encode2(pInpData, iInpSize, pOutData, iOutSize);
    }
    else if (!ds_stricmp(argv[1], "encodeurl"))
    {
        iOutSize = Base64EncodeUrl(pInpData, iInpSize, pOutData, iOutSize);
    }
    else if (!ds_stricmp(argv[1], "encodeurlnopad"))
    {
        iOutSize = Base64EncodeUrl2(pInpData, iInpSize, pOutData, iOutSize, FALSE);
    }

    // check result
    if (iOutSize > 0)
    {
        // write to output file if we have one
        if (argc == 4)
        {
            if ((iOutFileId = ZFileOpen(argv[3], ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_BINARY|ZFILE_OPENFLAG_CREATE)) != ZFILE_INVALID)
            {
                ZFileWrite(iOutFileId, pOutData, iOutSize);
                ZFileClose(iOutFileId);
                ZPrintf("%s: %s output successful (%d bytes)\n", argv[0], argv[1], iOutSize);
            }
            else
            {
                ZPrintf("%s: could not open file '%s' for writing", argv[0], argv[3]);
            }
        }
    }
    else
    {
        ZPrintf("%s: %s operation failed\n", argv[0], argv[1]);
    }

    // free output data buffer
    ZMemFree(pOutData);
    // free input data buffer
    ZMemFree(pInpData);

    return(0);
}



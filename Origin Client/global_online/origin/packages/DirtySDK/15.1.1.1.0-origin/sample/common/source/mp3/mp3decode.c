/*H********************************************************************************/
/*!
    \File mp3decode.c

    \Description
        Decode an mp3 from memory.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 11/16/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"

#include "zmem.h"
#include "zlib.h"

#include "mp3play.h"
#include "mp3decode.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

struct MP3DecodeStateT
{
    MP3PlayStateT *pPlayer;
    uint8_t aBuffer[4096];
    int32_t iBufPos;
    uint16_t *pDecodeBuffer;
    int32_t iDecoded;
    int32_t iStreamOffset;
};

/*** Variables ********************************************************************/

const char *_strDecodeErrors[] =
{
    "MP3_ERROR_NONE",
    "MP3_ERROR_UNKNOWN",
    "MP3_ERROR_INVALID_PARAMETER",
    "MP3_ERROR_INVALID_SYNC",   
    "MP3_ERROR_INVALID_HEADER",
    "MP3_ERROR_OUT_OF_BUFFER"
};


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _mp3decodereset

    \Description
        Reset buffer and player state

    \Input *pState      - module state
    
    \Output
        None.

    \Version 11/22/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _mp3decodereset(MP3DecodeStateT *pState)
{
    if (pState->pPlayer != NULL)
    {
        mp3playdestroy(pState->pPlayer);
        pState->pPlayer = NULL;
    }
    if (pState->pDecodeBuffer != NULL)
    {
        ZMemFree(pState->pDecodeBuffer);
        pState->pDecodeBuffer = NULL;
    }
    pState->iDecoded = 0;
}

/*F********************************************************************************/
/*!
    \Function _mp3decodesync

    \Description
        Sync to first valid mp3 header in input.

    \Input *pState      - module state
    \Input *pInput      - input stream
    \Input iInputSize   - amount of data available
    
    \Output
        int32_t         - 

    \Version 11/17/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _mp3decodesync(MP3DecodeStateT *pState, const uint8_t *pInput, int32_t iInputSize)
{
    const uint8_t *pInputStart = pInput;

    // reset state
    _mp3decodereset(pState);

    // sync to a valid header
    for ( ; iInputSize > 4; pInput++, iInputSize--)
    {
        // does this look like an mp3 chunk header?
        if ((pInput[0] != 0xff) || ((pInput[1] & 0xe0) != 0xe0))
        {
            continue;
        }

        //$$ TODO - implement mp3 decode

        // create the player
        //pState->pPlayer = mp3playcreate(pState->MpegInfo.channels, pState->MpegInfo.frequency, pState->MpegInfo.outputSize, 32);
    }
    return(pInput - pInputStart);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function mp3decodecreate

    \Description
        Create the mp3decode module.

    \Input None.
    
    \Output
        MP3DecodeStateT *   - decoder state, or NULL

    \Version 11/17/2005 (jbrookes)
*/
/********************************************************************************F*/
MP3DecodeStateT *mp3decodecreate(void)
{
    MP3DecodeStateT *pState;
    
    if ((pState = ZMemAlloc(sizeof(*pState))) == NULL)
    {
        NetPrintf(("mp3decode: could not allocate state\n"));
        return(NULL);
    }
    memset(pState, 0, sizeof(*pState));
    return(pState);
}

/*F********************************************************************************/
/*!
    \Function mp3decodestroy

    \Description
        Destroy the mp3decode module.

    \Input *pState  - module state
    
    \Output
        None.

    \Version 11/21/2005 (jbrookes)
*/
/********************************************************************************F*/
void mp3decodedestroy(MP3DecodeStateT *pState)
{
    _mp3decodereset(pState);
    ZMemFree(pState);
}

/*F********************************************************************************/
/*!
    \Function mp3decodereset

    \Description
        Reset the mp3decode module.

    \Input *pState  - module state
    
    \Output
        None.

    \Version 11/22/2005 (jbrookes)
*/
/********************************************************************************F*/
void mp3decodereset(MP3DecodeStateT *pState)
{
    _mp3decodereset(pState);
    pState->iBufPos = 0;
}

/*F********************************************************************************/
/*!
    \Function mp3decodeprocess

    \Description
        Update the decoder

    \Input *pState  - decoder state
    \Input *pInput  - stream input
    \Input iInputSize - amount of data available
    
    \Output
        int32_t     - amount of data consumed

    \Version 11/17/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t mp3decodeprocess(MP3DecodeStateT *pState, const uint8_t *pInput, int32_t iInputSize)
{
    _mp3decodesync(pState, pInput, iInputSize);
    return(0);
}

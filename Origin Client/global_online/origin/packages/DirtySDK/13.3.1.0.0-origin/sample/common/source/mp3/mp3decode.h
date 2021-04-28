/*H********************************************************************************/
/*!
    \File mp3decode.h

    \Description
        Basic MP3 decoder and player.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 11/17/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _mp3decode_h
#define _mp3decode_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct MP3DecodeStateT MP3DecodeStateT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create decoder
MP3DecodeStateT *mp3decodecreate(void);

// destroy decoder
void mp3decodedestroy(MP3DecodeStateT *pState);

// reset decoder
void mp3decodereset(MP3DecodeStateT *pState);

// update decoder
int32_t mp3decodeprocess(MP3DecodeStateT *pState, const uint8_t *pInput, int32_t iInputSize);

#ifdef __cplusplus
}
#endif

#endif // _mp3decode_h


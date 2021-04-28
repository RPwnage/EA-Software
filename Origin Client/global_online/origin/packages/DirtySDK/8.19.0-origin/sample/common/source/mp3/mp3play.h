/*H********************************************************************************/
/*!
    \File mp3play.h

    \Description
        Interface to play mp3.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 11/17/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _mp3play_h
#define _mp3play_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct MP3PlayStateT MP3PlayStateT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module state
MP3PlayStateT *mp3playcreate(int32_t iNumChannels, int32_t iSampleRate, int32_t iFrameSize, int32_t iNumFrames);

// destroy the module
void mp3playdestroy(MP3PlayStateT *pState);

// play some data
int32_t mp3playdata(MP3PlayStateT *pState, uint16_t *pSampleData, int32_t iFrameSize);

// control module behavior
int32_t mp3playcontrol(MP3PlayStateT *pState, int32_t iControl, int32_t iValue);


#ifdef __cplusplus
}
#endif

#endif // _mp3play_h


/*H********************************************************************************/
/*!
    \File voipmixer.h

    \Description
        VoIP data packet definitions.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 07/29/2004 (jbrookes) Split from voipheadset
*/
/********************************************************************************H*/

#ifndef _voipmixer_h
#define _voipmixer_h

/*** Include files ****************************************************************/

#include "voipcodec.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque module state ref
typedef struct VoipMixerRefT VoipMixerRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

// create module state
VoipMixerRefT *VoipMixerCreate(uint32_t uMixBuffers, uint32_t uFrameSamples);

// destroy module state
void VoipMixerDestroy(VoipMixerRefT *pMixerRef);

// accumulate data into mixbuffer
int32_t VoipMixerAccumulate(VoipMixerRefT *pMixerRef, unsigned char *pInputData, int32_t iDataSize, int32_t iMixBuffer, int32_t iMixMask, int32_t iChannel);

// process mix data currently in accumulator and return a mixed frame
int32_t VoipMixerProcess(VoipMixerRefT *pMixerRef, unsigned char *pFrameData);

#endif // _voipmixer_h


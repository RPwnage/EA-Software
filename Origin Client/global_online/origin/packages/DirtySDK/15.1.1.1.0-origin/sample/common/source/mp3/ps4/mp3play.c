/*H********************************************************************************/
/*!
    \File mp3play.c

    \Description
        Interface to play mp3 on PS3

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 10/15/2012 (akirchner) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <audioout.h>
#include <string.h>
#include <sdk_version.h>

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "zmem.h"
#include "zlib.h"

#include "mp3play.h"

/*** Defines **********************************************************************/

#define FRAME_SIZE_MAX      (65472)
#define FRAME_SIZE_FRAGMENT (64)

/*** Type Definitions *************************************************************/

struct MP3PlayStateT
{
    int32_t     iFrameSize;
    int32_t     iNumChannels;
    int32_t     bSpeakerOpen;     //!< have we successfully opened the speaker device
    int32_t     iSpeakerPort;     //!< the port number of the output device
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function mp3playcreate

    \Description
        Create the mp3play module

    \Input iNumChannels - number of channels of audio to be played (one or two)
    \Input iSampleRate  - sample rate to play at
    \Input iFrameSize   - size of wave data
    \Input iNumFrames   - any valud can be used because this parameter is ignored

    \Output
        int32_t         - pointer to module state on success

    \Version 10/15/2012 (akirchner)
*/
/********************************************************************************F*/
MP3PlayStateT *mp3playcreate(int32_t iNumChannels, int32_t iSampleRate, int32_t iFrameSize, int32_t iNumFrames)
{
    MP3PlayStateT *pState = NULL;
#if 1
    // validate the number of channels
    if ((iNumChannels != 1)
     && (iNumChannels != 2))
    {
        ZPrintf("mp3play: 1 or 2 channels supported\n");
        return(NULL);
    }

    // validate the sample rate
    if ((iSampleRate !=  8000)
     && (iSampleRate != 11025)
     && (iSampleRate != 12000)
     && (iSampleRate != 16000)
     && (iSampleRate != 22050)
     && (iSampleRate != 24000)
     && (iSampleRate != 32000)
     && (iSampleRate != 44100)
     && (iSampleRate != 48000))
    {
        ZPrintf("mp3play: invalid sample rate\n");
        return(NULL);
    }

    // validate the frame size
    if ((iFrameSize < FRAME_SIZE_FRAGMENT) || (iFrameSize > FRAME_SIZE_MAX))
    {
        ZPrintf("mp3play: frame size is smaller then %d or bigger then %d\n", FRAME_SIZE_FRAGMENT, FRAME_SIZE_MAX);
        return(NULL);
    }

    if (iFrameSize % FRAME_SIZE_FRAGMENT)
    {
        ZPrintf("mp3play: frame size is not a multiple of %d\n", FRAME_SIZE_FRAGMENT);
        return(NULL);
    }

    // allocate and init state
    if ((pState = ZMemAlloc(sizeof(*pState))) == NULL)
    {
        ZPrintf("mp3play: unable to allocate module state\n");
        return(NULL);
    }

    memset(pState, 0, sizeof(*pState));
    pState->iNumChannels = iNumChannels;
    pState->iFrameSize   = iFrameSize;

    // open the output device
    if (iNumChannels == 1)
    {
        if ((pState->iSpeakerPort = sceAudioOutOpen(0, SCE_AUDIO_OUT_PORT_TYPE_VOICE, 0, iFrameSize, iSampleRate, SCE_AUDIO_OUT_PARAM_FORMAT_S16_MONO)) < 0)
        {
            ZPrintf("mp3play: unable to init output device\n");
            ZMemFree(pState);
            return(NULL);
        }
    }
    else
    {
        if ((pState->iSpeakerPort = sceAudioOutOpen(0, SCE_AUDIO_OUT_PORT_TYPE_VOICE, 0, iFrameSize, iSampleRate, SCE_AUDIO_OUT_PARAM_FORMAT_S16_STEREO)) < 0)
        {
            ZPrintf("mp3play: unable to init output device\n");
            ZMemFree(pState);
            return(NULL);
        }
    }

    pState->bSpeakerOpen = TRUE;
#endif
    return(pState);
}

/*F********************************************************************************/
/*!
    \Function mp3playdestroy

    \Description
        Destroy the mp3 player.

    \Input *pState  - module state

    \Output
        None.

    \Version 10/15/2012 (akirchner)
*/
/********************************************************************************F*/

void mp3playdestroy(MP3PlayStateT *pState)
{
    if (!pState->bSpeakerOpen)
    {
        return;
    }

    sceAudioOutClose(pState->iSpeakerPort);

    pState->bSpeakerOpen = FALSE;

    // free state
    ZMemFree(pState);
}

/*F********************************************************************************/
/*!
    \Function mp3playdata

    \Description
        Try and play some data.

    \Input *pState - module state
    \Input *pSampleData - sample data to be played
    \Input iFrameSize - number of bytes pointed to by pSampleData

    \Output
        int32_t - return 1 if successful, 0 otherwise

    \Version 10/15/2012 (akirchner)
*/
/********************************************************************************F*/
int32_t mp3playdata(MP3PlayStateT *pState, uint16_t *pSampleData, int32_t iFrameSize)
{
#if 1
    int32_t iSample;

    // only play whole frames
    if (iFrameSize != pState->iFrameSize)
    {
        NetPrintf(("mp3play: frame size invalid\n"));
        return(0);
    }

    // verify that the device is open
    if (pState->bSpeakerOpen != TRUE)
    {
        NetPrintf(("mp3play: device not open\n"));
        return(0);
    }

    if (pState->iNumChannels == 1)
    {
        if (sceAudioOutOutput(pState->iSpeakerPort, pSampleData) < 0)
        {
            NetPrintf(("mp3play: failed to output mono samples\n"));
            return(0);
        }
    }
    else if (pState->iNumChannels == 2)
    {
        int16_t iStereoSampleData[2 * FRAME_SIZE_MAX];

        memset(iStereoSampleData, 0, sizeof(iStereoSampleData));

        for (iSample = 0; iSample < pState->iFrameSize; iSample++)
        {
            iStereoSampleData[2 * iSample]     = pSampleData[iSample];
            iStereoSampleData[2 * iSample + 1] = pSampleData[iSample];
        }

        if (sceAudioOutOutput(pState->iSpeakerPort, iStereoSampleData) < 0)
        {
            NetPrintf(("mp3play: failed to output stereo samples\n"));
            return(0);
        }
    }
#endif
    return(1);
}

/*F********************************************************************************/
/*!
    \Function mp3playcontrol

    \Description
        Control module behavior

    \Input *pState  - pointer to module state
    \Input iControl - control selector
    \Input iValue   - selector specific

    \Output
        int32_t     - selector specific

    \Version 10/15/2012 (akirchner)
*/
/********************************************************************************F*/
int32_t mp3playcontrol(MP3PlayStateT *pState, int32_t iControl, int32_t iValue)
{
    // no such selector
    return(-1);
}

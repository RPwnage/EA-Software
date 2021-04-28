/*H********************************************************************************/
/*!
    \File mp3play.c

    \Description
        Interface to play mp3 on PS3

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 08/17/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <sdk_version.h>
#include <cell/audio.h>

#include "dirtylib.h"
#include "dirtyerr.h"
#include "zmem.h"
#include "zlib.h"

#include "mp3play.h"

/*** Defines **********************************************************************/

#define MAX_FRAME_SIZE      (2048)

// this needs to be a multiple of 512 as well as a multiple of 640 for all the libraries
// to be happy.
#define OUTPUT_BUFFER_SIZE  (2560)

/*** Type Definitions *************************************************************/

struct MP3PlayStateT
{
    int32_t      iFrameSize;
    int32_t      iBlockSize;

    int32_t     bSpeakerOpen;     //!< have we successfully opened the speaker device

    int32_t     iSpeakerPort;     //!< the port number of the output device
    sys_addr_t  pBufferAddress;   //!< a pointer to the sony-allocated buffer for writing speaker data
    sys_addr_t  pBufferEnd;       //!< a pointer to the end of the sony-allocated buffer
    sys_addr_t  pNextWriteLoc;    //!< pointer to the next location where we should send data to the speaker

    float       fOutputBuffer[OUTPUT_BUFFER_SIZE]; //!< the data to be written to the speaker
    int32_t     iNextOutputIndex;                  //!< the next index in fOutputBuffer to write to the speaker
    int32_t     iNextInputIndex;                   //!< the next index in fOutputBuffer to place input data

    int32_t     iUpsampleIndex;   //!< loop counter for managing upsampling from 11025Hz to 48kHz
    float       fPrevSample;      //!< keep track of the previous sample - used for upsampling functions

};

/*** Variables ********************************************************************/


/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _MP3OpenOutputDevice

    \Description
        Open output device for playing.

    \Input *pState      - module state
    \Input iNumChannels - number of channels of audio to be played (one or two)
    \Input iSampleRate  - sample rate to play at
    \Input iFrameSize   - size of wave data
    
    \Output
        int32_t         - negative=failure, else success

    \Version 08/17/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _MP3OpenOutputDevice(MP3PlayStateT *pState, int32_t iNumChannels, int32_t iSampleRate, int32_t iFrameSize)
{
    int32_t iResult;
    CellAudioPortParam audioParam;
    CellAudioPortConfig audioConfig;

    if (pState->bSpeakerOpen)
    {
        NetPrintf(("mp3play: output device is already opened\n"));
        return(0);
    }

    // set up the port parameters
    audioParam.nBlock   = CELL_AUDIO_BLOCK_32;
    audioParam.attr     = CELL_AUDIO_PORTATTR_OUT_STREAM1;
    if (iNumChannels == 1)
    {
        // TODO: may need to implement upsampling from 1 channel to 2 for this to work.
        audioParam.nChannel = CELL_AUDIO_PORT_2CH;
    }
    else if (iNumChannels == 2)
    {
        audioParam.nChannel = CELL_AUDIO_PORT_2CH;
    }
    else if (iNumChannels == 8)
    {
        audioParam.nChannel = CELL_AUDIO_PORT_8CH;
    }
    else
    {
        NetPrintf(("mp3play: number of channels (%d) not supported\n", iNumChannels));
        return(-1);
    }
    pState->iBlockSize = audioParam.nChannel * sizeof(float) * CELL_AUDIO_BLOCK_SAMPLES;
    
    iResult = cellAudioInit();
    if (iResult != CELL_OK)
    {
        NetPrintf(("mp3play: cellAudioInit failed: %s\n", DirtyErrGetName(iResult)));
        return(-1);
    }

    if ((iResult = cellAudioPortOpen(&audioParam, &pState->iSpeakerPort)) != CELL_OK)
    {
        NetPrintf(("mp3play: failed to open audio port 0x%08x\n", iResult));
        return(-1);
    }

    // get the pointer to port buffer
    if ((iResult = cellAudioGetPortConfig(pState->iSpeakerPort, &audioConfig)) != CELL_OK)
    {
        NetPrintf(("mp3play: get port config failed\n"));
        cellAudioPortClose(pState->iSpeakerPort);
        return(-1);
    }

    // set our internal pointers to point at the buffer created by the sony library and keep track of
    // where we are in the buffer, the size of the buffer, and the location of the end of the buffer
    pState->pBufferAddress = audioConfig.portAddr;
    pState->pNextWriteLoc = pState->pBufferAddress;
    pState->pBufferEnd = (pState->pBufferAddress + audioConfig.portSize);

    // start outputing data
    if ((iResult = cellAudioPortStart(pState->iSpeakerPort)) != CELL_OK)
    {
        NetPrintf(("mp3play: failed to start playback\n"));
        cellAudioPortClose(pState->iSpeakerPort);
        return(-1);
    }

    pState->bSpeakerOpen = TRUE;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _MP3CloseOutputDevice

    \Description
        Close output device.

    \Input *pState      - module state
    
    \Output
        int32_t         - negative=failure, else success

    \Version 11/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _MP3CloseOutputDevice(MP3PlayStateT *pState)
{
    // make sure we have a device    
    if (!pState->bSpeakerOpen)
    {
        return;
    }
    
    pState->bSpeakerOpen = FALSE;

    cellAudioPortStop(pState->iSpeakerPort);

    cellAudioPortClose(pState->iSpeakerPort);
}

/*F********************************************************************************/
/*!
    \Function   _MP3TransferBlock

    \Description
        Transfers a block of input data to the output device.

    \Input *pState    - pointer to module state
    
    \Output
        None

    \Version 1.0 08/14/2006 (tdills) First Version
*/
/********************************************************************************F*/
static void _MP3TransferBlock(MP3PlayStateT *pState)
{
    // copy the input data to the output buffer
    memcpy((void*)pState->pNextWriteLoc, &(pState->fOutputBuffer[pState->iNextOutputIndex]), pState->iBlockSize);

    // advance the destination pointer
    pState->pNextWriteLoc += pState->iBlockSize;
    if (pState->pNextWriteLoc == pState->pBufferEnd)
    {
        pState->pNextWriteLoc = pState->pBufferAddress;
    }

    // advance the source index
    pState->iNextOutputIndex += (pState->iBlockSize / sizeof(float));
    // the source buffer is circular so deal with reaching the end
    if (pState->iNextOutputIndex == OUTPUT_BUFFER_SIZE)
    {
        pState->iNextOutputIndex = 0;
    }
}

/*F********************************************************************************/
/*!
    \Function   _MP3Upsample1MonoTo5Stereo

    \Description
        Upsample data from 1 16-bit integer mono sample to 5 32-bit floating-point
        stereo samples (a 1:10 sample conversion)

    \Input  fSample     - the input sample
    \Input  fPrevSample - the previous input sample
    \Input *pOutSamples - the array of floats in which to place the output
    
    \Output
        float - the sample to use as the previous sample for the next frame

    \Version 08/21/2006 (tdills) First Version
*/
/********************************************************************************F*/
static float _MP3Upsample1MonoTo5Stereo(float fSample, float fPrevSample, float *pOutSamples)
{
    const float f1Fifth  = (1.0f/5.0f);
    const float f2Fifths = (2.0f/5.0f);
    const float f3Fifths = (3.0f/5.0f);
    const float f4Fifths = (4.0f/5.0f);
    const float fScale   = 1.0f / 32768.0f;

    float fScaledCurr = fSample * fScale;

    return(fScaledCurr);

    // upsample 1 sample to 5 samples
    pOutSamples[0] = fPrevSample * f4Fifths + fScaledCurr * f1Fifth;
    pOutSamples[2] = fPrevSample * f3Fifths + fScaledCurr * f2Fifths;
    pOutSamples[4] = fPrevSample * f2Fifths + fScaledCurr * f3Fifths;
    pOutSamples[6] = fPrevSample * f1Fifth  + fScaledCurr * f4Fifths;
    pOutSamples[8] =                          fScaledCurr;

    // convert to stereo
    pOutSamples[1] = pOutSamples[0];
    pOutSamples[3] = pOutSamples[2];
    pOutSamples[5] = pOutSamples[4];
    pOutSamples[7] = pOutSamples[6];
    pOutSamples[9] = pOutSamples[8];

    return(fScaledCurr);
}

/*F********************************************************************************/
/*!
    \Function   _MP3Upsample1MonoTo4Stereo

    \Description
        Upsample data from 1 16-bit integer mono sample to 4 32-bit floating-point
        stereo samples (a 1:8 sample conversion)

    \Input  fSample     - the input sample
    \Input  fPrevSample - the previous input sample
    \Input *pOutSamples - the array of floats in which to place the output
    
    \Output
        float - the sample to use as the previous sample for the next frame

    \Version 08/21/2006 (tdills) First Version
*/
/********************************************************************************F*/
static float _MP3Upsample1MonoTo4Stereo(float fSample, float fPrevSample, float *pOutSamples)
{
    const float f1Fourth  = (1.0f/4.0f);
    const float f2Fourths = (2.0f/4.0f);
    const float f3Fourths = (3.0f/4.0f);
    const float fScale   = 1.0f / 32768.0f;

    float fScaledCurr = fSample * fScale;

    return(fScaledCurr);

    // upsample 1 sample to 4 samples
    pOutSamples[0] = fPrevSample * f3Fourths + fScaledCurr * f1Fourth;
    pOutSamples[2] = fPrevSample * f2Fourths + fScaledCurr * f2Fourths;
    pOutSamples[4] = fPrevSample * f1Fourth  + fScaledCurr * f3Fourths;
    pOutSamples[6] =                           fScaledCurr;

    // convert to stereo
    pOutSamples[1] = pOutSamples[0];
    pOutSamples[3] = pOutSamples[2];
    pOutSamples[5] = pOutSamples[4];
    pOutSamples[7] = pOutSamples[6];

    return(fScaledCurr);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function mp3playcreate

    \Description
        Create the mp3play module

    \Input None.
    
    \Output
        MP3PlayStateT *   - pointer to module state, or NULL

    \Version 08/17/2005 (tdills)
*/
/********************************************************************************F*/
MP3PlayStateT *mp3playcreate(int32_t iNumChannels, int32_t iSampleRate, int32_t iFrameSize, int32_t iNumFrames)
{
    MP3PlayStateT *pState;
    
    // allocate and init state
    if ((pState = ZMemAlloc(sizeof(*pState))) == NULL)
    {
        ZPrintf("mp3play: unable to allocate module state\n");
        return(NULL);
    }
    memset(pState, 0, sizeof(*pState));
    pState->iFrameSize = iFrameSize;
    
    // validate the frame size
    if (pState->iFrameSize > MAX_FRAME_SIZE)
    {
        ZPrintf("mp3play: frame size is too large\n");
        ZMemFree(pState);
        return(NULL);
    }

    // open the output device
    if (_MP3OpenOutputDevice(pState, iNumChannels, iSampleRate, iFrameSize) < 0)
    {
        ZPrintf("mp3play: unable to init output device\n");
        ZMemFree(pState);
        return(NULL);
    }

    // return state to caller    
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

    \Version 08/17/2005 (tdills)
*/
/********************************************************************************F*/
void mp3playdestroy(MP3PlayStateT *pState)
{
    // close device
    _MP3CloseOutputDevice(pState);
    
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

    \Version 08/17/2005 (tdills)
*/
/********************************************************************************F*/
int32_t mp3playdata(MP3PlayStateT *pState, uint16_t *pSampleData, int32_t iFrameSize)
{
    int32_t iNumFloatsInFrame = iFrameSize / sizeof(int16_t);
    int32_t iNumBlocksInFrame = iFrameSize / pState->iBlockSize;
    int32_t iIndex;

    // only play whole frames
    if (iFrameSize != pState->iFrameSize)
    {
        NetPrintf(("mp3play: frame size invalid\n"));
        return(0);
    }

    return(1);

    // convert 16-bit mono 11025Hz sample to 32-bit floating-point stereo 48kHz data
    for (iIndex=0; iIndex < iNumFloatsInFrame; iIndex++)
    {
        // a coversion from 147 samples to 640 produces the proper ratio to upsample from 11025Hz to 48kHz
        // generally we upsample in a 1:4 ratio which produces 588 samples
        // we add an extra upsample for every third sample producing 49 extra upsamples for a sum of 637 samples
        // we add 3 extra upsamples roughly at each 49th source sample except the special case of sample #0 which would
        // be divisible by both 3 and 49 - thus we are adding an upsample at sample #1. This produces a total of 640 samples

        // in each cycle of 0-146 this will happen exactly 3 times producing   15 samples
        if ((pState->iUpsampleIndex == 1) || (pState->iUpsampleIndex == 49) || (pState->iUpsampleIndex == 98))
        {
            pState->fPrevSample = _MP3Upsample1MonoTo5Stereo(pSampleData[iIndex], pState->fPrevSample, &pState->fOutputBuffer[pState->iNextInputIndex]);
            pState->iNextInputIndex += 10;
        }
        // in each cycle of 0-146 this will happen exactly 49 times producing 245 samples
        else if (pState->iUpsampleIndex % 3 == 0)
        {
            pState->fPrevSample = _MP3Upsample1MonoTo5Stereo(pSampleData[iIndex], pState->fPrevSample, &pState->fOutputBuffer[pState->iNextInputIndex]);
            pState->iNextInputIndex += 10;
        }
        // in each cycle of 0-146 this will happen exactly 95 times producing 380 samples
        else
        {
            pState->fPrevSample = _MP3Upsample1MonoTo4Stereo(pSampleData[iIndex], pState->fPrevSample, &pState->fOutputBuffer[pState->iNextInputIndex]);
            pState->iNextInputIndex += 8;
        }
        // thus the total: 15+245+380 = 640 samples from 147 source samples

        // increment the upsample index in a cycle from 0 to 146
        pState->iUpsampleIndex++;
        if (pState->iUpsampleIndex >= 147)
        {
            pState->iUpsampleIndex = 0;
        }

        // OUTPUT_BUFF_SIZE should be a multiple of 640
        if (pState->iNextInputIndex == OUTPUT_BUFFER_SIZE)
        {
            pState->iNextInputIndex = 0;
        }
    } 

    // write the source data to the output buffer
    for (iIndex=0; iIndex < iNumBlocksInFrame; iIndex++)
    {
        _MP3TransferBlock(pState);
    } 

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

    \Version 12/13/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t mp3playcontrol(MP3PlayStateT *pState, int32_t iControl, int32_t iValue)
{
    // no such selector
    return(-1);
}

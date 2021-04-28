/*H********************************************************************************/
/*!
    \File mp3play.c

    \Description
        Interface to play mp3.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 11/17/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <kernel.h>
#include <libwave.h>

#include "platform.h"
#include "dirtylib.h"

#include "zmem.h"
#include "zlib.h"

#include "mp3play.h"

/*** Defines **********************************************************************/

#define FRAME_SIZE      (16*1024)

#define MP3PLAY_VERBOSE (FALSE)

/*** Type Definitions *************************************************************/

typedef struct MP3WaveDataT
{
    uint8_t     aFrameData[FRAME_SIZE];
} MP3WaveDataT;

struct MP3PlayStateT
{
    volatile int32_t    iThreadId;
    volatile int32_t    iWaveBufferIn;
    volatile int32_t    iWaveBufferOut;
    uint32_t            bRebuffer;
    int32_t             iSampleRate;
    int32_t             iNumChannels;
    int32_t             iFrameSize;
    int32_t             iNumFrames;
    MP3WaveDataT        WaveData[1];
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _MP3PlayThread

    \Description
        Handle mp3 playback.

    \Input uArgSize     - size of input arg data
    \Input *pArgData    - pointer to input arg data
    
    \Output
        int32_t         - zero

    \Version 11/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _MP3PlayThread(uint32_t uArgSize, void *pArgData)
{
    MP3PlayStateT *pState = *(MP3PlayStateT **)pArgData;
    uint32_t bPlaying = FALSE;
    int32_t iNumFrames;
    
    NetPrintf(("mp3play: play thread running\n"));

    // play loop
    while(pState->iThreadId != 0)
    {
        // calculate number of frames available
        iNumFrames = (pState->iWaveBufferIn - pState->iWaveBufferOut);
        if (iNumFrames < 0)
        {
            iNumFrames += pState->iNumFrames;
        }
        
        // if not playing, wait until max data is queued before starting
        if (bPlaying == FALSE)
        {
            // make sure there is enough data to play
            if (iNumFrames < (pState->iNumFrames - 1))
            {
                sceKernelDelayThread(1000);
                continue;
            }
            
            // start playing
            NetPrintf(("mp3play: play start\n"));
            bPlaying = TRUE;
        }
        else if (iNumFrames == 0)
        {
            uint16_t aSilence[64*2];
            int32_t iResult;
            
            if (pState->bRebuffer == TRUE)
            {
                // out of data... halt playing
                NetPrintf(("mp3play: out of data -- rebuffering\n"));
                bPlaying = FALSE;
            }
            else
            {
                memset(aSilence, 0, sizeof(aSilence));
                if ((iResult = sceWaveAudioSetSample(0, pState->iNumChannels*sizeof(uint16_t)*64)) < 0)
                {
                    NetPrintf(("mp3play: oops\n"));
                }
                sceWaveAudioWriteBlocking(0, (32768 * 80) / 100, (32768 * 80) / 100, aSilence);
                if ((iResult = sceWaveAudioSetSample(0, pState->iFrameSize / (pState->iNumChannels*sizeof(uint16_t)))) < 0)
                {
                    NetPrintf(("mp3play: oops2\n"));
                }
            }
            continue;
        }
        
        // play the data
        #if MP3PLAY_VERBOSE
        NetPrintf(("mp3play: playing from slot %d at %d\n", pState->iWaveBufferOut, NetTick()));
        #endif
        sceWaveAudioWriteBlocking(0, (32768 * 80) / 100, (32768 * 80) / 100, pState->WaveData[pState->iWaveBufferOut].aFrameData);
        
        // next buffer
        pState->iWaveBufferOut = (pState->iWaveBufferOut + 1) % pState->iNumFrames;
    }

    // report termination
    NetPrintf(("mp3play: play thread exiting\n"));
    pState->iThreadId = -1;
    
    // terminate ourselves
    sceKernelExitDeleteThread(0);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _MP3OpenOutputDevice

    \Description
        Start up wave output.

    \Input *pState      - module state
    \Input iNumChannels - number of channels (1=mono, 2=stereo)
    \Input iSampleRate  - sample rate
    \Input iFrameSize   - frame size (bytes)
    
    \Output
        int32_t         - negative=failure, else success

    \Version 11/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _MP3OpenOutputDevice(MP3PlayStateT *pState, int32_t iNumChannels, int32_t iSampleRate, int32_t iFrameSize)
{
    int32_t iFormat, iResult;

    // make sure we've got a supported sample rate
    if ((iSampleRate != 44100) && (iSampleRate != 22050) && (iSampleRate != 11025))
    {
        NetPrintf(("mp3play: sample rate of %d is unsupported\n", iSampleRate));
        return(-1);
    }

    // save sample rate
    pState->iSampleRate = iSampleRate;
    
    // calculate frame size based on sample rate
    iFrameSize = (iFrameSize * 44100) / iSampleRate;
    pState->iFrameSize = iFrameSize;
    pState->iNumChannels = iNumChannels;
    
    // init wave library
    if ((iResult = sceWaveInit()) < 0)
    {
        NetPrintf(("mp3play: sceWaveInit() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(-1);
    }
    
    // set audio format
    iFormat = (iNumChannels == 2) ? SCE_WAVE_AUDIO_FMT_S16_STEREO : SCE_WAVE_AUDIO_FMT_S16_MONO;
    if ((iResult = sceWaveAudioSetFormat(0, iFormat)) < 0)
    {
        NetPrintf(("mp3play: sceWaveAudioSetFormat() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(-1);
    }

    // set audio frame size $$ note -- assume 16 bit samples! $$
    if ((iResult = sceWaveAudioSetSample(0, iFrameSize / (iNumChannels*sizeof(uint16_t)))) < 0)
    {
        NetPrintf(("mp3play: sceWaveAudioSetSample() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(-1);
    }

    // create and start play thread
    if ((pState->iThreadId = sceKernelCreateThread("mp3play", _MP3PlayThread, SCE_KERNEL_USER_HIGHEST_PRIORITY-1, 8192, 0, NULL)) < 0)
    {
        NetPrintf(("mp3play: sceKernelCreateThread() failed (err=%s)\n", pState->iThreadId));
        sceWaveExit();
        return(-1);
    }
    if ((iResult = sceKernelStartThread(pState->iThreadId, sizeof(pState), &pState)) < 0)
    {
        NetPrintf(("mp3play: unable to start play thread (err=%s)\n", DirtyErrGetName(iResult)));
    }
    
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _MP3CloseOutputDevice

    \Description
        Stop wave output.

    \Input *pState      - module state
    
    \Output
        None.

    \Version 11/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _MP3CloseOutputDevice(MP3PlayStateT *pState)
{
    int32_t iResult;

    // signal thread shutdown
    pState->iThreadId = 0;

    // wait for thread shutdown    
    while (pState->iThreadId == 0)
    {
        sceKernelDelayThread(1);
    }

    // shut down wave library
    if ((iResult = sceWaveExit()) < 0)
    {
        NetPrintf(("mp3play: sceWaveEnd() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return;
    }
}

/*F********************************************************************************/
/*!
    \Function _mp3copydata

    \Description
        Copy data for playback.

    \Input *pState      - module state 
    \Input *pSampleData - pointer to input data
    \Input iFrameSize   - frame size
    
    \Output
        MP3PlayStateT *   - pointer to module state, or NULL

    \Notes
        Supports 44.1khz playback (native PSP rate) and 22.5khz/11.025khz playback
        with a cheezy linear interpolation filter.

    \Version 12/12/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _mp3copydata(MP3PlayStateT *pState, int16_t *pSampleData, int32_t iFrameSize)
{
    int16_t *pSampleBuf = pState->WaveData[pState->iWaveBufferIn].aFrameData;
        
    // at native rate?
    if (pState->iSampleRate == 44100)
    {
        memcpy(pSampleBuf, pSampleData, iFrameSize);
    }
    else if (pState->iSampleRate == 22050)
    {
        int32_t iSample;
        for (iSample = 0; iSample < (iFrameSize/2)-1; iSample++)
        {
            pSampleBuf[0] = pSampleData[0];
            pSampleBuf[1] = (int16_t)(((int32_t)pSampleData[0]+(int32_t)pSampleData[1])/2);
            pSampleBuf += 2;
            pSampleData += 1;
        }
        pSampleBuf[0] = pSampleData[0];
        pSampleBuf[1] = pSampleData[0];
    }
    else if (pState->iSampleRate == 11025)
    {
        int32_t iSample;
        int32_t iStep;
        for (iSample = 0; iSample < (iFrameSize/2)-1; iSample++)
        {
            iStep = ((int32_t)pSampleData[1] - (int32_t)pSampleData[0]) / 4;
            
            pSampleBuf[0] = pSampleData[0];
            pSampleBuf[1] = (int16_t)((int32_t)pSampleData[0]+iStep);
            pSampleBuf[2] = (int16_t)((int32_t)pSampleData[0]+iStep+iStep);
            pSampleBuf[3] = (int16_t)((int32_t)pSampleData[0]+iStep+iStep+iStep);
            pSampleBuf += 4;
            pSampleData += 1;
        }
        pSampleBuf[0] = pSampleData[0];
        pSampleBuf[1] = pSampleData[0];
        pSampleBuf[2] = pSampleData[0];
        pSampleBuf[3] = pSampleData[0];
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function mp3playcreate

    \Description
        Create the mp3play module

    \Input iNumChannels     - 1=mono, 2=stereo
    \Input iSampleRate      - sample rate
    \Input iFrameSize       - frame size, in bytes
    \Input iNumFrames       - number of frames to buffer    
    
    \Output
        MP3PlayStateT *     - pointer to module state, or NULL

    \Version 11/17/2005 (jbrookes)
*/
/********************************************************************************F*/
MP3PlayStateT *mp3playcreate(int32_t iNumChannels, int32_t iSampleRate, int32_t iFrameSize, int32_t iNumFrames)
{
    MP3PlayStateT *pState;
    int32_t iStateSize = sizeof(*pState) + ((iNumFrames-1) * sizeof(MP3WaveDataT));
    
    // allocate and init state
    if ((pState = ZMemAlloc(iStateSize)) == NULL)
    {
        NetPrintf(("mp3play: unable to allocate module state\n"));
        return(NULL);
    }
    memset(pState, 0, iStateSize);
    pState->iNumFrames = iNumFrames;
    pState->bRebuffer = TRUE;
    
    // open the output device
    if (_MP3OpenOutputDevice(pState, iNumChannels, iSampleRate, iFrameSize) < 0)
    {
        NetPrintf(("mp3play: unable to init output device\n"));
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

    \Version 11/21/2005 (jbrookes)
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

    \Input None.
    
    \Output
        MP3PlayStateT *   - pointer to module state, or NULL

    \Version 11/17/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t mp3playdata(MP3PlayStateT *pState, uint16_t *pSampleData, int32_t iFrameSize)
{
    // any room in buffer?
    if (((pState->iWaveBufferIn + 1) % pState->iNumFrames) != pState->iWaveBufferOut)
    {
        // copy sample data into next available buffer
        #if MP3PLAY_VERBOSE
        NetPrintf(("mp3play: queued in slot %d at %d\n", pState->iWaveBufferIn, NetTick()));
        #endif
        _mp3copydata(pState, pSampleData, iFrameSize);
        
        // index to next buffer and return success
        pState->iWaveBufferIn = (pState->iWaveBufferIn + 1) % pState->iNumFrames;
        return(1);
    }
    else
    {
        #if MP3PLAY_VERBOSE
        NetPrintf(("mp3play: did not queue data in=%d out=%d\n", pState->iWaveBufferIn, pState->iWaveBufferOut));
        #endif
    }
    
    // did not buffer sample data
    return(0);
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
    // enable/disable rebuffering behavior
    if (iControl == 'rbuf')
    {
        pState->bRebuffer = iValue;
        return(0);
    }
    // no such selector
    return(-1);
}

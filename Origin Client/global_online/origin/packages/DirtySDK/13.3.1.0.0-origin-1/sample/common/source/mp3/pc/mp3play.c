/*H********************************************************************************/
/*!
    \File mp3play.c

    \Description
        Interface to play mp3 data.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 11/17/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"

#include "zmem.h"
#include "zlib.h"

#include "mp3play.h"

/*** Defines **********************************************************************/

#define NUM_FRAMES (16)

/*** Type Definitions *************************************************************/

typedef struct MP3WaveDataT
{
    WAVEHDR     WaveHdr;
    uint8_t     *pFrameData;
} MP3WaveDataT;

struct MP3PlayStateT
{
    HWAVEOUT    hDevice;
    int32_t     iCurWaveBuffer;
    int32_t     iFrameSize;
    uint8_t     *pFrameData;
    MP3WaveDataT WaveData[NUM_FRAMES];
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _MP3PrepareWaveHeaders

    \Description
        Prepare wave headers for playing.

    \Input *pState      - module state
    \Input *pWaveData   - pointer to wave data
    \Input iNumBuffers  - number of buffers of wave data
    \Input iFrameSize   - size of wave data
    
    \Output
        int32_t         - negative=error, else success

    \Version 11/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _MP3PrepareWaveHeaders(MP3PlayStateT *pState, MP3WaveDataT *pWaveData, int32_t iNumBuffers, int32_t iFrameSize)
{
    HRESULT hResult;
    int32_t iPlayBuf;

    for (iPlayBuf = 0; iPlayBuf < iNumBuffers; iPlayBuf++)
    {
        pWaveData[iPlayBuf].pFrameData = pState->pFrameData + (iPlayBuf * iFrameSize);
        pWaveData[iPlayBuf].WaveHdr.lpData = (LPSTR)pWaveData[iPlayBuf].pFrameData;
        pWaveData[iPlayBuf].WaveHdr.dwBufferLength = iFrameSize;
        pWaveData[iPlayBuf].WaveHdr.dwFlags = 0L;
        pWaveData[iPlayBuf].WaveHdr.dwLoops = 0L;

        if ((hResult = waveOutPrepareHeader(pState->hDevice, &pWaveData[iPlayBuf].WaveHdr, sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
        {
            NetPrintf(("mp3play: error %d preparing output buffer %d\n", hResult, iPlayBuf));
            return(-1);
        }
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _MP3UnrepareWaveHeaders

    \Description
        Unprepare wave headers for shutdown

    \Input *pState      - module state
    \Input *pWaveData   - pointer to wave data
    \Input iNumBuffers  - number of buffers of wave data
    
    \Output
        None.

    \Version 11/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _MP3UnprepareWaveHeaders(MP3PlayStateT *pState, MP3WaveDataT *pWaveData, int32_t iNumBuffers)
{
    HRESULT hResult;
    int32_t iPlayBuf;

    // unprepare headers
    for (iPlayBuf = 0; iPlayBuf < iNumBuffers; iPlayBuf++)
    {
        if ((hResult = waveOutUnprepareHeader(pState->hDevice, &pWaveData[iPlayBuf].WaveHdr, sizeof(pWaveData[iPlayBuf].WaveHdr))) != MMSYSERR_NOERROR)
        {
            NetPrintf(("mp3play: error %d unpreparing output buffer %d\n", hResult, iPlayBuf));
        }
    }
}

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

    \Version 11/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _MP3OpenOutputDevice(MP3PlayStateT *pState, int32_t iNumChannels, int32_t iSampleRate, int iFrameSize)
{
    WAVEFORMATEX WaveFormatEx;
    HRESULT hResult;
    const int32_t iDevice = 0;

    // alloc ring buffer
    if ((pState->pFrameData = ZMemAlloc(iFrameSize * NUM_FRAMES)) == NULL)
    {
        return(-1);
    }

    WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormatEx.nChannels = iNumChannels;
    WaveFormatEx.nSamplesPerSec = iSampleRate;
    WaveFormatEx.wBitsPerSample = 16;
    WaveFormatEx.nAvgBytesPerSec = WaveFormatEx.nSamplesPerSec * WaveFormatEx.wBitsPerSample/8;
    WaveFormatEx.nBlockAlign = WaveFormatEx.nChannels * (WaveFormatEx.wBitsPerSample/8);
    WaveFormatEx.cbSize = 0;

    // open output device
    if ((hResult = waveOutOpen(&pState->hDevice, iDevice, &WaveFormatEx, 0, 0, 0)) != S_OK)
    {
        NetPrintf(("mp3play: error %d opening output device\n", hResult));
        return(-1);
    }

    // set up output wave data
    if (_MP3PrepareWaveHeaders(pState, pState->WaveData, NUM_FRAMES, iFrameSize) < 0)
    {
        waveOutClose(pState->hDevice);
        pState->hDevice = 0;
    }

    return((pState->hDevice == 0) ? -2 : 0);
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
    if (pState->hDevice == 0)
    {
        return;
    }
    
    // reset stream
    waveOutReset(pState->hDevice);

    // unprepare output wave data
    _MP3UnprepareWaveHeaders(pState, pState->WaveData, NUM_FRAMES);

    // close output device
    waveOutClose(pState->hDevice);
    pState->hDevice = 0;

    // free ring buffer
    if (pState->pFrameData != NULL)
    {
        ZMemFree(pState->pFrameData);
        pState->pFrameData = NULL;
    }
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

    \Version 11/17/2005 (jbrookes)
*/
/********************************************************************************F*/
MP3PlayStateT *mp3playcreate(int32_t iNumChannels, int32_t iSampleRate, int32_t iFrameSize, int32_t iNumFrames)
{
    MP3PlayStateT *pState;

    NetPrintf(("mp3play: create ch=%d sr=%d fs=%d\n", iNumChannels, iSampleRate, iFrameSize));

    // allocate and init state
    if ((pState = ZMemAlloc(sizeof(*pState))) == NULL)
    {
        NetPrintf(("mp3play: unable to allocate module state\n"));
        return(NULL);
    }
    memset(pState, 0, sizeof(*pState));
    pState->iFrameSize = iFrameSize;
    
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
    int32_t iResult;

    // only play whole frames
    if (iFrameSize != pState->iFrameSize)
    {
        NetPrintf(("mp3play: invalid framesize %d\n", iFrameSize));
        return(0);
    }

    // try and play some data
    memcpy(pState->WaveData[pState->iCurWaveBuffer].pFrameData, pSampleData, iFrameSize);
    if ((iResult = waveOutWrite(pState->hDevice, &pState->WaveData[pState->iCurWaveBuffer].WaveHdr, sizeof(WAVEHDR))) == MMSYSERR_NOERROR)
    {
        pState->iCurWaveBuffer = (pState->iCurWaveBuffer + 1) % NUM_FRAMES;
        return(1);
    }
    else
    {
        if (iResult != WAVERR_STILLPLAYING)
        {
            NetPrintf(("mp3play: error %d trying to play data\n", iResult));
        }
        return(0);
    }
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

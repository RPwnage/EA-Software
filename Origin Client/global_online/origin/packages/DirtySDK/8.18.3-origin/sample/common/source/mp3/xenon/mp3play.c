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

#include <xtl.h>

#include "platform.h"
#include "dirtylib.h"

#include "zmem.h"
#include "zlib.h"

#include "mp3play.h"

/*** Defines **********************************************************************/

#define NUM_FRAMES (16)

/*** Type Definitions *************************************************************/

typedef struct MP3WaveDataT
{
    uint8_t     *pFrameData;
} MP3WaveDataT;

struct MP3PlayStateT
{
    int32_t     iCurWaveBuffer;
    int32_t     iFrameSize;
    uint8_t     *pFrameData;
    MP3WaveDataT WaveData[NUM_FRAMES];
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

    \Version 11/21/2005 (jbrookes)
*/
/********************************************************************************F*/

static int32_t _MP3OpenOutputDevice(MP3PlayStateT *pState, int32_t iNumChannels, int32_t iSampleRate, int iFrameSize)
{
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

    ZPrintf("mp3play: create ch=%d sr=%d fs=%d\n", iNumChannels, iSampleRate, iFrameSize);

    // allocate and init state
    if ((pState = ZMemAlloc(sizeof(*pState))) == NULL)
    {
        ZPrintf("mp3play: unable to allocate module state\n");
        return(NULL);
    }
    memset(pState, 0, sizeof(*pState));
    NetPrintf(("mp3play: state=0x%08x\n", (uint32_t)pState));
    pState->iFrameSize = iFrameSize;

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
    // only play whole frames
    if (iFrameSize != pState->iFrameSize)
    {
        ZPrintf("mp3play: invalid framesize %d\n", iFrameSize);
        return(0);
    }

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
    // no such selector
    return(-1);
}

/*H********************************************************************************/
/*!
    \File voipheadsetps3.c

    \Description
        VoIP headset manager.

    \Copyright
        Copyright (c) Electronic Arts 2006. ALL RIGHTS RESERVED.

    \Notes
        Summary of PS3 voice/voip operations:

        Voip producer:
            * voice data samples (floats) are obtained from the mic with calls to cellMicRead()
            * voice data samples are grouped into "voice data bundles" in place, i.e. in the buffer being filled by cellMicRead()
            * voice data bundles get encoded/compressed with VoipCodecEncode()
            * the resulting data buffer is then passed to the voip transmission code for sending over the network
            * the voip transmission code packs the compresssed voice data bundle as a sub-packet of a given voip packet (a voip packet
              can contain several sub-packets)

        Voip consumer:
            * the voip reception code unpacks sub-packets found in incoming voip packets
            * each sub-packet is passed to the matching voip conduit (one conduit per local user)
            * voip data is then decoded and mixed in the mix buffer
            * mixed data is copied out of the mix buffer and fed into the audio device in chunks of size VOIP_HEADSET_BLOCK_SIZE bytes
            * left-over mixed data (codec frame sizes tend to not match block output frame sizes) is retained for the next iteration

        NOTE - we make the assumption that any left-over data that remains in the FrameData buffer and has not been written out
        to the audio output buffer when VAD kicks in can be disposed of; this data will be close to the VAD threshold and therefore
        it should not be easy to detect its absence.

        Current configuration:
            * voice bundles are set to capture from 20-40ms of samples, depending on input rate and codec
            * codec input frame size and output data size depend on the codec; input frames range from 80 to 320 samples
            * LPCM is supported only in loopback mode

    \Version 1.0 08/10/2006 (tdills) Port from PSP to PS3
    \Version 2.0 10/20/2011 (jbrookes) Major rewrite to fix issues and support more codecs/sample rates
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>  // sprintf
#include <string.h>
#include <sdk_version.h>

#include <cell/audio.h>
#include <cell/mic.h>

#include "platform.h"
#include "dirtysock.h"
#include "dirtymem.h"
#include "dirtyerr.h"

#include "voipdef.h"
#include "voippriv.h"
#include "voipconnection.h"
#include "voippacket.h"
#include "voipmixer.h"
#include "voipconduit.h"
#include "voipcodec.h"
#include "voipheadset.h"

// codecs
#include "voipdvi.h"
#include "voippcm.h"
#include "voipcelp.h"

/*** Defines **********************************************************************/

// default params
#define VOIP_HEADSET_DEFAULTSAMPLERATE          (8000)      //!< default sample rate in hz
#define VOIP_HEADSET_DEFAULTFRAMESAMPLES        (160)       //!< default number of samples/frame (20ms at 8khz)

// mic read parameters
#define VOIP_HEADSET_MAX_SAMPLES                (320)       //!< CELP variants MPE_0 and MPE_1 require 320 samples; RPE_0, RPE_2, and RPE_3 require 240
#define VOIP_HEADSET_MAX_READ_SAMPLES           (16000/10)  //!< 100ms worth of samples data at 16khz

// audio write parameters
#define VOIP_HEADSET_BLOCK_SIZE                 (CELL_AUDIO_PORT_2CH * sizeof(float) * CELL_AUDIO_BLOCK_SAMPLES)//!< block size (in bytes) used to empty mix buffer and feed headset
#define VOIP_HEADSET_OUTBUFFER_BLOCKS           (CELL_AUDIO_BLOCK_32) //!< port size in blocks
#define VOIP_HEADSET_MAX_BLOCKS                 (9)         //!< max number of blocks that may be written out to audio at one time

// VAD constants
#define VOIP_HEADSET_SOUND_TRIGGER_DECIBEL      (50.0f)     //!< min volume to not trigger VAD
#define VOIP_HEADSET_TALKING_MIN_DURATION       (200)       //!< min duration to not trigger VAD


/*** Macros ***********************************************************************/


/*** Type Definitions *************************************************************/

//! headset sampling parameters
typedef struct VoipHeadsetParamT
{
    int32_t iCodecIdent;            //!< fourcc codec ident
    int32_t iSampleRate;            //!< input sample rate (8000 or 16000)
    int32_t iUpsampleRate;          //!< factor to upsample from input rate to output rate of 48khz
    int32_t iCodecSamples;          //!< number of samples codec expects as input
    int32_t iFrameSamples;          //!< number of samples we choose to call a 'frame' will be same or x2 iCodecSamples)
    int32_t iMaxReadSamples;        //!< max number of samples we will read from the mic at one time
    int32_t iCmpFrameSize;          //!< size in bytes of codec output frame
} VoipHeadsetParamT;

//! VOIP module state data
struct VoipHeadsetRefT
{
    // sample-rate derived parameters
    VoipHeadsetParamT Param;

    // conduit info
    int32_t iMaxConduits;

    // network data status
    uint8_t uSendSeq;
    uint8_t bLoopback;
    uint32_t uFrameSend;

    // user callback data
    VoipHeadsetMicDataCbT *pMicDataCb;
    VoipHeadsetStatusCbT *pStatusCb;
    void *pCbUserData;

    // speaker callback data
    VoipSpkrCallbackT *pSpkrDataCb;
    void *pSpkrCbUserData;

    // misc buffers
    VoipMicrPacketT PacketData;     //!< VOIP packet for sending data to the mixer

    VoipConduitRefT *pConduitRef;
    VoipMixerRefT   *pMixerRef;

    // mic configuration parameters
    int32_t iAGCLevel;
    float   fBkgndNoiseGain;
    int32_t iReverb;

    int32_t iSpeakerPort;           //!< port number of the speaker device we opened.
    int32_t bPlaying;               //!< are we currently playing to the speaker?
    int32_t bStarted;               //!< whether cellAudioPortStarted() has been successfully performed or not
    int32_t bRecording;             //!< are we currently reading from the microphone?
    int32_t bTriedToOpen;           //!< have we already tried to open the port? (reset on disconnect)
    int32_t bAudioLibOwner;         //!< we initialized the audio library and should therefore shut it down

    // used for VAD
    uint32_t uLastMicSoundTick;     //!< the last time the mic picked up a sound loud enough for VOIP_HEADSET_SOUND_TRIGGER_DECIBEL_

    // mic read buffer variables
    int16_t aReadBuffer[VOIP_HEADSET_MAX_READ_SAMPLES]; //!< data buffer for data read from mic
    int32_t iCurrWriteIndex;        //!< current index into aReadBuffer where new voice samples should be placed
    int32_t iCurrReadIndex;         //!< current index into aReadBuffer where new voice samples should be retrieved from

    // frame data buffer after mixing
    int16_t aFrameData[VOIP_HEADSET_MAX_BLOCKS*128];      //!< buffer holding mixed 16-bit signed audio data ready for output; must be able to hold up to 9 128-sample blocks
    int32_t iFrameOffset;           //!< offset into frame data buffer

    // ring buffer variables
    int32_t iRingBuffData;          //!< amount of data in ring buffer
    int32_t iLastReadIndex;         //!< most recent read index from ring buffer
    sys_addr_t pBufferAddress;      //!< a pointer to the sony-allocated buffer for writing speaker data
    sys_addr_t pBufferEnd;          //!< a pointer to the end of the sony-allocated buffer
    sys_addr_t piPortReadIndex;     //!< a pointer to the sony-allocated 64-bit integer containing the current block index
    sys_addr_t pNextWriteLoc;       //!< pointer to the next location where we should send data to the speaker

    #if DIRTYCODE_LOGGING
    int32_t iDebugLevel;            //!< module debuglevel
    #endif
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Public Variables

// Private Variables

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSetParam

    \Description
        Set params based on input sample rate.

    \Input *pHeadset    - module state
    \Input *pParam      - [out] headset param struct to fill in
    \Input iCodecIdent  - codec ident fourcc
    \Input iSampleRate  - input sample rate (8000 or 16000)
    \Input iCodecSamples - number of samples per 8khz frame (doubled for 16khz)

    \Version 10/04/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetSetParam(VoipHeadsetRefT *pHeadset, VoipHeadsetParamT *pParam, int32_t iCodecIdent, int32_t iSampleRate, int32_t iCodecSamples)
{
    // save codec ident
    pParam->iCodecIdent = iCodecIdent;

    // reset frame offset to clear any previous mixed data
    pHeadset->iFrameOffset = 0;
    // reset audio input pointers to clear any previous mic data
    pHeadset->iCurrReadIndex = pHeadset->iCurrWriteIndex = 0;

    // calculate sample rate constants
    pParam->iSampleRate = iSampleRate;
    pParam->iUpsampleRate = (48000 * 2) / pParam->iSampleRate;

    // save codec-required sample size
    pParam->iCodecSamples = iCodecSamples;

    /* double frame size at 16khz up to a max 320 sample frame; this restriction is
       intended to keep the maximum size of some of our read buffers somewhat
       constrained */
    pParam->iFrameSamples = (pParam->iCodecSamples <= 160) ? iCodecSamples * (pParam->iSampleRate/8000) : pParam->iCodecSamples;

    // query codec output size
    pParam->iCmpFrameSize = VoipCodecStatus(pParam->iCodecIdent, 'fsiz', pParam->iFrameSamples, NULL, 0);

    // we need to make sure the number of samples we read from the mic results in an integral number of frames
    pParam->iMaxReadSamples = (VOIP_HEADSET_MAX_READ_SAMPLES / pParam->iFrameSamples) * pParam->iFrameSamples;

    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: iCodecIdent='%C'\n", pParam->iCodecIdent));
    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: iSampleRate=%d\n", pParam->iSampleRate));
    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: iUpsampleRate=%d\n", pParam->iUpsampleRate));
    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: iCodecSamples=%d\n", pParam->iCodecSamples));
    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: iFrameSamples=%d\n", pParam->iFrameSamples));
    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: iMaxReadSamples=%d\n", pParam->iMaxReadSamples));
    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: iCmpFrameSize=%d\n", pParam->iCmpFrameSize));
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetCreateMixer

    \Description
        Create (or re-create) Mixer and update Conduit with new ref

    \Input *pHeadset    - module reference

    \Version 10/26/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetCreateMixer(VoipHeadsetRefT *pHeadset)
{
    if (pHeadset->pMixerRef != NULL)
    {
        VoipMixerDestroy(pHeadset->pMixerRef);
    }
    if ((pHeadset->pMixerRef = VoipMixerCreate(10, pHeadset->Param.iFrameSamples)) == NULL)
    {
        NetPrintf(("voipheadsetps3: unable to create mixer\n"));
    }

    VoipConduitSetMixer(pHeadset->pConduitRef, pHeadset->pMixerRef);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAudioInit

    \Description
        Initialize audio system

    \Input *pHeadset    - module reference

    \Output
        int32_t         - negative=error, zero=success

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetAudioInit(VoipHeadsetRefT *pHeadset)
{
    int32_t iResult;

    iResult = cellAudioInit();
    if (iResult != CELL_OK)
    {
        if (iResult == CELL_AUDIO_ERROR_ALREADY_INIT)
        {
            NetPrintf(("voipheadsetps3: cellAudio already initialized\n"));
        }
        else
        {
            NetPrintf(("voipheadsetps3: cellAudioInit() failed (err=%s)\n", DirtyErrGetName(iResult)));
            return(-1);
        }
    }
    else
    {
        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: cellAudioInit succeeded\n"));
        pHeadset->bAudioLibOwner = TRUE;
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAudioShutdown

    \Description
        Shut down the audio system

    \Input *pHeadset    - module reference

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetAudioShutdown(VoipHeadsetRefT *pHeadset)
{
    int32_t iResult;
    if (pHeadset->bAudioLibOwner)
    {
        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: cellAudioQuit()\n"));
        if ((iResult = cellAudioQuit()) != CELL_OK)
        {
            NetPrintf(("voipheadsetps3: cellAudioQuit failed (err=%s)\n", DirtyErrGetName(iResult)));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAudioStart

    \Description
        Start audio playback

    \Input *pHeadset    - module reference

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetAudioStart(VoipHeadsetRefT *pHeadset)
{
    CellAudioPortParam audioParam;
    CellAudioPortConfig audioConfig;
    int32_t iResult;

    // set up the port parameters
    audioParam.nChannel = CELL_AUDIO_PORT_2CH;
    audioParam.nBlock   = VOIP_HEADSET_OUTBUFFER_BLOCKS;
    audioParam.attr     = CELL_AUDIO_PORTATTR_OUT_SECONDARY;

    // open the port
    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: opening audio port\n"));
    if ((iResult = cellAudioPortOpen(&audioParam, (uint32_t *)&pHeadset->iSpeakerPort)) != CELL_OK)
    {
        NetPrintf(("voipheadsetps3: failed to open audio port (err=%s)\n", DirtyErrGetName(iResult)));
        return;
    }
    else
    {
        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: speaker port %d opened\n", pHeadset->iSpeakerPort));
    }

    // get the pointer to port buffer
    if ((iResult = cellAudioGetPortConfig(pHeadset->iSpeakerPort, &audioConfig)) != CELL_OK)
    {
        NetPrintf(("voipheadsetps3: get port config failed (err=%s)\n", DirtyErrGetName(iResult)));
        cellAudioPortClose(pHeadset->iSpeakerPort);
        return;
    }

    // set our internal pointers to point at buffer created by the sony library and keep track of
    // where we are in the buffer, the size of the buffer, and the location of the end of the buffer
    pHeadset->pBufferAddress = audioConfig.portAddr;
    pHeadset->pNextWriteLoc = pHeadset->pBufferAddress;
    pHeadset->piPortReadIndex = audioConfig.readIndexAddr;
    pHeadset->pBufferEnd = (pHeadset->pBufferAddress + audioConfig.portSize);

    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: audio port parameters channels=%d blocks=%d portsize=%d portaddr=%p\n",
        audioConfig.nChannel, audioConfig.nBlock, audioConfig.portSize, audioConfig.portAddr));

    // mark us as playing
    pHeadset->bPlaying = TRUE;

    // start the audio port
    if ((iResult = cellAudioPortStart(pHeadset->iSpeakerPort)) != CELL_OK)
    {
        NetPrintf(("voipheadsetps3: failed to start playback (err=%s)\n", DirtyErrGetName(iResult)));
        cellAudioPortClose(pHeadset->iSpeakerPort);
        return;
    }
    pHeadset->bStarted = TRUE;
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAudioStop

    \Description
        Stop audio playback

    \Input *pHeadset    - module reference

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetAudioStop(VoipHeadsetRefT *pHeadset)
{
    int32_t iResult;

    if (pHeadset->bPlaying == TRUE)
    {
        pHeadset->bPlaying = FALSE;

        if (pHeadset->bStarted)
        {
            NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: stopping audio port\n"));
            if ((iResult = cellAudioPortStop(pHeadset->iSpeakerPort)) != CELL_OK)
            {
                NetPrintf(("voipheadsetps3: cellAudioPortStop failed (err=%s)\n", DirtyErrGetName(iResult)));
            }
            pHeadset->bStarted = FALSE;
        }

        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: closing audio port\n"));
        if ((iResult = cellAudioPortClose(pHeadset->iSpeakerPort)) != CELL_OK)
        {
            NetPrintf(("voipheadsetps3: cellAudioPortClose failed (err=%s)\n", DirtyErrGetName(iResult)));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAudioWrite

    \Description
        Writes a block of input data to the output audio buffer.

    \Input *pHeadset    - pointer to headset state
    \Input *pSourceData - pointer to the prepared input data to transfer

    \Output
        void* - updated pSourceData pointing to the next block of input data

    \Version 08/14/2006 (tdills)
*/
/********************************************************************************F*/
static void *_VoipHeadsetAudioWrite(VoipHeadsetRefT *pHeadset, void *pSourceData)
{
    /* if there is no voice data in the ring buffer, we reset the write pointer to
       be 1/2 offset from the read pointer.  this adds 1/2 buffer of latency (85ms)
       to absorb any network jitter we might encounter without introducing audio
       artifacts. */
    if (pHeadset->iRingBuffData <= 0)
    {
        // read ring buffer data index, generate 1/2 offset for write pointer
        int32_t iRingBuffDataIndex = (int32_t)*(uint64_t *)pHeadset->piPortReadIndex;
        iRingBuffDataIndex = (iRingBuffDataIndex + (VOIP_HEADSET_OUTBUFFER_BLOCKS/2)) % VOIP_HEADSET_OUTBUFFER_BLOCKS;
        pHeadset->pNextWriteLoc = pHeadset->pBufferAddress + VOIP_HEADSET_BLOCK_SIZE*iRingBuffDataIndex;
        pHeadset->iRingBuffData = VOIP_HEADSET_BLOCK_SIZE*(VOIP_HEADSET_OUTBUFFER_BLOCKS/2);
        // clear any previously buffered voice data
        pHeadset->iFrameOffset = 0;
    }
    // copy the input data to the output buffer
    memcpy((void*)pHeadset->pNextWriteLoc, pSourceData, VOIP_HEADSET_BLOCK_SIZE);
    pHeadset->iRingBuffData += VOIP_HEADSET_BLOCK_SIZE;

    // advance the source and destination pointers
    pSourceData = (void *)((uintptr_t)pSourceData + VOIP_HEADSET_BLOCK_SIZE);
    pHeadset->pNextWriteLoc += VOIP_HEADSET_BLOCK_SIZE;

    // the destination pointer is a circular buffer so deal with reaching the end
    if (pHeadset->pNextWriteLoc == pHeadset->pBufferEnd)
    {
        pHeadset->pNextWriteLoc = pHeadset->pBufferAddress;
    }

    // return the updated source pointer
    return(pSourceData);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetMicInit

    \Description
        Initialize microphone system

    \Input *pHeadset    - module reference

    \Output
        int32_t         - negative=error, zero=success

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetMicInit(VoipHeadsetRefT *pHeadset)
{
    int32_t iResult;
    if ((iResult = cellMicInit()) != CELL_OK)
    {
        NetPrintf(("voipheadsetps3: cellMicInit() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(-1);
    }
    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: cellMicInit succeeded\n"));
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetMicShutdown

    \Description
        Shut down the mic system

    \Input *pHeadset    - module reference

    \Version 10/04/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetMicShutdown(VoipHeadsetRefT *pHeadset)
{
    int32_t iResult;
    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: cellMicEnd()\n"));
    if ((iResult = cellMicEnd()) != CELL_OK)
    {
        NetPrintf(("voipheadsetps3: cellMicEnd failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetMicOpen

    \Description
        Open up microphone, set input rate, start microphone recording.

    \Input *pHeadset    - module reference

    \Version 10/04/2011 (jbrookes) Refactored from _VoipHeadsetProcessMicrophone
*/
/********************************************************************************F*/
static void _VoipHeadsetMicOpen(VoipHeadsetRefT *pHeadset)
{
    int32_t iRetVal;

    if (!cellMicIsAttached(0))
    {
        pHeadset->bTriedToOpen = FALSE;
        return;
    }
    if (pHeadset->bTriedToOpen)
    {
        return;
    }

    // remember that we tried to open the mic
    pHeadset->bTriedToOpen = TRUE;

    // open the mic
    if ((iRetVal = cellMicOpen(0, pHeadset->Param.iSampleRate)) != CELL_OK)
    {
        NetPrintf(("voipheadsetps3: cellMicOpen failed (err=%s)\n", DirtyErrGetName(iRetVal)));
        return;
    }

    // headset inserted, so triger callback
    pHeadset->pStatusCb(0, TRUE, pHeadset->pCbUserData);

    // display initial device status
    #if DIRTYCODE_LOGGING
    {
        CellMicInputFormatI FormatData;
        float fBackgroundNoiseGain;
        int32_t iAGCLevel, iReverb;

        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: initial mic status\n"));

        cellMicGetFormatDsp(0, &FormatData);
        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: channels: %d, subframesize: %d, bit resolution: %d, data type: %d, sample rate: %d\n",
            FormatData.bNrChannels, FormatData.bSubframeSize, FormatData.bBitResolution, FormatData.bDataType,
            FormatData.uiSampRate));

        cellMicGetSignalAttr(0, CELLMIC_SIGATTR_AGCLEVEL, &iAGCLevel);
        cellMicGetSignalAttr(0, CELLMIC_SIGATTR_BKNGAIN, &fBackgroundNoiseGain);
        cellMicGetSignalAttr(0, CELLMIC_SIGATTR_REVERB, &iReverb);
        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: background noise gain: %2.1f db, agc target level: %d, reverb: %d\n",
            fBackgroundNoiseGain, iAGCLevel, iReverb));
    }
    #endif

    // set signal attributes
    cellMicSetSignalAttr(0, CELLMIC_SIGATTR_AGCLEVEL, &pHeadset->iAGCLevel);
    cellMicSetSignalAttr(0, CELLMIC_SIGATTR_BKNGAIN, &pHeadset->fBkgndNoiseGain);
    cellMicSetSignalAttr(0, CELLMIC_SIGATTR_REVERB, &pHeadset->iReverb);
    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: set background noise gain: %2.1f db, agc target level: %d, reverb: %d\n",
        pHeadset->fBkgndNoiseGain, pHeadset->iAGCLevel, pHeadset->iReverb));

    // start the mic and remember that it is started
    if ((iRetVal = cellMicStart(0)) != CELL_OK)
    {
        NetPrintf(("voipheadsetps3: cellMicOpen failed (err=%s)\n", DirtyErrGetName(iRetVal)));
        if ((iRetVal = cellMicClose(0)) != CELL_OK)
        {
            NetPrintf(("voipheadsetps3: cellMicClose failed (err=%s)\n", DirtyErrGetName(iRetVal)));
        }
        return;
    }
    pHeadset->bRecording = TRUE;
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetMicClose

    \Description
        Close the mic and mark as no longer recording

    \Input *pHeadset    - pointer to headset state

    \Version 10/04/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetMicClose(VoipHeadsetRefT *pHeadset)
{
    if (pHeadset->bRecording)
    {
        cellMicClose(0);
    }

    pHeadset->bRecording = FALSE;
    pHeadset->bTriedToOpen = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetMicSetInputSignalAttr

    \Description
        Set input signal attributes

    \Input *pHeadset    - module reference
    \Input eSignalAttr  - signal attribute to set (CELLMIC_SIGATTR_*)
    \Input *pVal        - pointer to value to set

    \Output
        int32_t         - zero=success, else failure

    \Version 10/24/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetMicSetInputSignalAttr(VoipHeadsetRefT *pHeadset, CellMicSignalAttr eSignalAttr, void *pVal)
{
    int32_t iResult = 0;

    // set signal attribute if device is already open
    if (pHeadset->bRecording)
    {
        if ((iResult = cellMicSetSignalAttr(0, eSignalAttr, pVal)) < 0)
        {
            NetPrintf(("voipheadsetps3: cellMicSetSignalAttr() failed (err=%s)\n", DirtyErrGetName(iResult)));
        }
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetMicRead

    \Description
        Accumulate data from the mic into our buffer.

    \Input *pHeadset    - pointer to headset state

    \Output
        int32_t - the number of samples that are now in the buffer. Negative
                  if an error occurred and the mic is now disconnected.

    \Version 08/11/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetMicRead(VoipHeadsetRefT *pHeadset)
{
    static float fSampleData[VOIP_HEADSET_MAX_READ_SAMPLES] __attribute__ ((aligned(128)));
    int32_t iIndex, iLocal, iReadLen, iNumSamples, iCurrSamples;
    float fMic;
    uint32_t uTickNow;

    // find out how many valid samples currently in the read buffer
    if ((iCurrSamples = pHeadset->iCurrWriteIndex - pHeadset->iCurrReadIndex) == 0)
    {
        pHeadset->iCurrWriteIndex = 0;
        pHeadset->iCurrReadIndex = 0;
    }

    // read from the mic
    iReadLen = cellMicRead(0, &fSampleData[0], (pHeadset->Param.iMaxReadSamples - pHeadset->iCurrWriteIndex) * sizeof(float));

    /// check for read error
    if (iReadLen < 0)
    {
        // this is most likely due to a disconnect
        NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: mic disconnected\n"));
        cellMicClose(0);
        pHeadset->bRecording = FALSE;
        pHeadset->bTriedToOpen = FALSE;
        return(-1);
    }

    // early exit if no data read
    if (iReadLen == 0)
    {
        return(iCurrSamples);
    }

    iNumSamples = iReadLen / sizeof(float);

    //determine if the user is talking
    cellMicGetSignalState(0, CELLMIC_SIGSTATE_LOCTALK, &iLocal);
    cellMicGetSignalState(0, CELLMIC_SIGSTATE_MICENG, &fMic);
    if((iLocal >= 9) && (fMic >= VOIP_HEADSET_SOUND_TRIGGER_DECIBEL))
    {
        pHeadset->uLastMicSoundTick = NetTick();
    }
    uTickNow = NetTick();
    if (NetTickDiff(uTickNow, pHeadset->uLastMicSoundTick) <= VOIP_HEADSET_TALKING_MIN_DURATION)
    {
        NetPrintfVerbose((pHeadset->iDebugLevel, 2, "voipheadsetps3: iLocal=%2d, iMic=%8f TALKING\n", iLocal, fMic));
    }
    else
    {
        // exit out early if the user isn't talking, and clear any leftover mic data
        NetPrintfVerbose((pHeadset->iDebugLevel, 2, "voipheadsetps3: iLocal=%2d, iMic=%8f\n", iLocal, fMic));
        pHeadset->iCurrReadIndex = pHeadset->iCurrWriteIndex = 0;
        return(-2);
    }

    // convert the data to ints
    for (iIndex = 0; iIndex < iNumSamples; iIndex++)
    {
        // we need to gate the input between -32768 and 32767
        if (fSampleData[iIndex] > 1.0f)
        {
            fSampleData[iIndex] = 1.0f;
        }
        else if (fSampleData[iIndex] < -1.0f)
        {
            fSampleData[iIndex] = -1.0f;
        }

        pHeadset->aReadBuffer[pHeadset->iCurrWriteIndex+iIndex] = (int16_t)(fSampleData[iIndex] * 32768.0f);
    }

    // add in new samples
    pHeadset->iCurrWriteIndex += iNumSamples;

    // return current sample count
    return(pHeadset->iCurrWriteIndex - pHeadset->iCurrReadIndex);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetShutdown

    \Description
        Stop recording & playback, close USB audio device, and shut down mic/audio systems

    \Input  *pHeadset  - module reference

    \Notes
        This function is safe to call regardless of device state.

    \Version 11/03/2003 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetShutdown(VoipHeadsetRefT *pHeadset)
{
    // stop audio playback
    _VoipHeadsetAudioStop(pHeadset);

    // shut down the audio library
    _VoipHeadsetAudioShutdown(pHeadset);

    // close the input device
    _VoipHeadsetMicClose(pHeadset);

    // shut down the mic library
    _VoipHeadsetMicShutdown(pHeadset);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAudioUpsample8To48

    \Description
        Upsample data from 8kHz to 48kHz

    \Input fPrevSample  - the previous sample - already scaled
    \Input fCurrSample  - the current samples - will be scaled
    \Input *pOutSamples - pointer to the 12 floats to contain the output

    \Output
        float - returns the scaled current sample for use as the next fPrevSample

    \Version 04/01/2004 (jbrookes)
*/
/********************************************************************************F*/
static float _VoipHeadsetAudioUpsample8To48(float fPrevSample, float fCurrSample, float *pOutSamples)
{
    const float f1Sixth  = (1.0f/6.0f);
    const float f2Sixths = (2.0f/6.0f);
    const float f3Sixths = (3.0f/6.0f);
    const float f4Sixths = (4.0f/6.0f);
    const float f5Sixths = (5.0f/6.0f);

    const float fScale = 1.0f / 32768.0f;

    float fScaledCurr = fCurrSample * fScale;

    pOutSamples[0]  = fPrevSample;
    pOutSamples[2]  = fPrevSample * f5Sixths + fScaledCurr * f1Sixth;
    pOutSamples[4]  = fPrevSample * f4Sixths + fScaledCurr * f2Sixths;
    pOutSamples[6]  = fPrevSample * f4Sixths + fScaledCurr * f3Sixths;
    pOutSamples[8]  = fPrevSample * f2Sixths + fScaledCurr * f4Sixths;
    pOutSamples[10] = fPrevSample * f1Sixth  + fScaledCurr * f5Sixths;

    pOutSamples[1]  = pOutSamples[0];
    pOutSamples[3]  = pOutSamples[2];
    pOutSamples[5]  = pOutSamples[4];
    pOutSamples[7]  = pOutSamples[6];
    pOutSamples[9]  = pOutSamples[8];
    pOutSamples[11] = pOutSamples[10];

    return(fScaledCurr);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAudioUpsample16To48

    \Description
        Upsample data from 16kHz to 48kHz

    \Input fPrevSample  - the previous sample - already scaled
    \Input fCurrSample  - the current samples - will be scaled
    \Input *pOutSamples - pointer to the 6 floats to contain the output

    \Output
        float - returns the scaled current sample for use as the next fPrevSample

    \Version 04/01/2004 (jbrookes)
*/
/********************************************************************************F*/
static float _VoipHeadsetAudioUpsample16To48(float fPrevSample, float fCurrSample, float *pOutSamples)
{
    const float f1Third  = (1.0f/3.0f);
    const float f2Thirds = (2.0f/3.0f);

    const float fScale = 1.0f / 32768.0f;

    float fScaledCurr = fCurrSample * fScale;

    pOutSamples[0] = fPrevSample;
    pOutSamples[2] = fPrevSample * f2Thirds + fScaledCurr * f1Third;
    pOutSamples[4] = fPrevSample * f1Third  + fScaledCurr * f2Thirds;

    pOutSamples[1] = pOutSamples[0];
    pOutSamples[3] = pOutSamples[2];
    pOutSamples[5] = pOutSamples[4];

    return(fScaledCurr);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSetSampleRate

    \Description
        Sets the specified sample rate, if valid and different from the current.

    \Input *pHeadset    - pointer to module state
    \Input iSampleRate  - mic sample rate (8000hz or 16000hz)

    \Output
        int32_t         - zero=success, negative=failure

    \Version 10/05/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetSetSampleRate(VoipHeadsetRefT *pHeadset, int32_t iSampleRate)
{
    // if sample rate didn't change, so don't change anything
    if (iSampleRate == pHeadset->Param.iSampleRate)
    {
        return(0);
    }

    // validate requested rate
    if ((iSampleRate != 8000) && (iSampleRate != 16000))
    {
        NetPrintf(("voipheadsetps3: invalid sample rate %d requested (must be 8000 or 16000)\n", iSampleRate));
        return(-1);
    }

    /* sample rate changed, so we need to recalculate some sampling parameters,
       re-create the mixer, and re-open the mic */
    NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: setting input rate to %dhz\n", iSampleRate));

    // recalc sampling parameters
    _VoipHeadsetSetParam(pHeadset, &pHeadset->Param, pHeadset->Param.iCodecIdent, iSampleRate, pHeadset->Param.iCodecSamples);

    // re-create the mixer
    _VoipHeadsetCreateMixer(pHeadset);

    // close and re-open mic
    _VoipHeadsetMicClose(pHeadset);
    _VoipHeadsetMicOpen(pHeadset);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSetCodec

    \Description
        Sets the specified codec.

    \Input *pHeadset    - pointer to module state
    \Input iCodecIdent  - codec identifier to set

    \Output
        int32_t         - zero=success, negative=failure

    \Version 10/11/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetSetCodec(VoipHeadsetRefT *pHeadset, int32_t iCodecIdent)
{
    int32_t iCodecSamples, iResult;

    // pass through codec creation request
    if ((iResult = VoipCodecCreate(iCodecIdent, pHeadset->iMaxConduits)) < 0)
    {
        return(iResult);
    }

    // get codec frame size
    if ((iCodecSamples = VoipCodecStatus(iCodecIdent, 'fsmp', 0, NULL, 0)) < 0)
    {
        // frame size not available from this codec, so go with the default
        iCodecSamples = VOIP_HEADSET_DEFAULTFRAMESAMPLES;
    }

    // recalc sampling parameters
    _VoipHeadsetSetParam(pHeadset, &pHeadset->Param, iCodecIdent, pHeadset->Param.iSampleRate, iCodecSamples);

    // re-create the mixer
    _VoipHeadsetCreateMixer(pHeadset);

    return(iResult);
}


/*F********************************************************************************/
/*!
    \Function _VoipHeadsetMicProcess

    \Description
        Process data received from the microphone.

    \Input *pHeadset    - pointer to headset state

    \Version 04/01/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetMicProcess(VoipHeadsetRefT *pHeadset)
{
    if (!pHeadset->bRecording)
    {
        _VoipHeadsetMicOpen(pHeadset);
    }

    if (pHeadset->bRecording)
    {
        VoipMicrPacketT *pMicrPacket = &pHeadset->PacketData;
        uint8_t *pData = pMicrPacket->aData;
        int32_t iCompBytes, iNumSamples;

        if (!cellMicIsAttached(0))
        {
            // mic is no longer connected
            cellMicClose(0);
            pHeadset->bRecording = FALSE;

            // headset removed, so triger callback
            NetPrintfVerbose((pHeadset->iDebugLevel, 0, "voipheadsetps3: headset removed\n"));
            pHeadset->pStatusCb(0, FALSE, pHeadset->pCbUserData);
        }

        // process voice data from mic
        for (iNumSamples = _VoipHeadsetMicRead(pHeadset); iNumSamples >= pHeadset->Param.iFrameSamples; iNumSamples -= pHeadset->Param.iFrameSamples)
        {
            // compress the data (note that the celp encoder will return zero the first two iterations)
            if ((iCompBytes = VoipCodecEncode(pData, &pHeadset->aReadBuffer[pHeadset->iCurrReadIndex], pHeadset->Param.iFrameSamples)) == 0)
            {
                continue;
            }

            // process the data
            if (pHeadset->bLoopback == FALSE)
            {
                // send to mic data callback
                pHeadset->pMicDataCb(&pMicrPacket->aData, iCompBytes, 0, pHeadset->uSendSeq++, pHeadset->pCbUserData);
                pHeadset->uFrameSend = NetTick();
            }
            else
            {
                pMicrPacket->MicrInfo.uSubPacketSize = iCompBytes;
                pMicrPacket->MicrInfo.uSeqn += 1;
                VoipConduitReceiveVoiceData(pHeadset->pConduitRef, (VoipUserT *)"", pMicrPacket->aData, iCompBytes);
            }

            // bundle of voice samples consumed, update read pointer accordingly
            pHeadset->iCurrReadIndex += pHeadset->Param.iFrameSamples;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetAudioProcess

    \Description
        Process playback of data received from the network to the headset earpiece.

    \Input *pHeadset    - pointer to headset state

    \Version 04/01/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetAudioProcess(VoipHeadsetRefT *pHeadset)
{
    // only play if we have a playback device open
    if (pHeadset->bPlaying == FALSE)
    {
        _VoipHeadsetAudioStart(pHeadset);
    }

    if (pHeadset->bPlaying == TRUE)
    {
        int8_t bExit = FALSE;

        // update our tracking of ring buffer position and how much voice data remains in the ring buffer
        {
            int32_t iRingBuffDataRead, iRingBuffDataIndex = (int32_t)*(uint64_t *)pHeadset->piPortReadIndex;
            iRingBuffDataRead = (iRingBuffDataIndex - pHeadset->iLastReadIndex + 32) & 31;
            pHeadset->iLastReadIndex = iRingBuffDataIndex;
            if ((iRingBuffDataRead > 0) && (pHeadset->iRingBuffData > 0))
            {
                pHeadset->iRingBuffData -= iRingBuffDataRead * VOIP_HEADSET_BLOCK_SIZE;
                NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetps3: [%08x] iRingBuffDataRead=%d, iRingBuffData=%d\n", NetTick(), iRingBuffDataRead, pHeadset->iRingBuffData));
            }
        }

        while (!bExit)
        {
            /* if we have unconsumed data in the buffer, check to make sure we have room
               to write more before proceeding to mix and output new data */
            if (pHeadset->iRingBuffData > 0)
            {
                int32_t iRingBuffRdIndex = (int32_t)*(uint64_t *)pHeadset->piPortReadIndex;
                int32_t iRingBuffWrIndex = (pHeadset->pNextWriteLoc - pHeadset->pBufferAddress) / VOIP_HEADSET_BLOCK_SIZE;
                int32_t iRingBuffOffset = (iRingBuffRdIndex - iRingBuffWrIndex + VOIP_HEADSET_OUTBUFFER_BLOCKS) % VOIP_HEADSET_OUTBUFFER_BLOCKS;
                NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetps3: [%08x] rdidx=%2d, wridx=%2d, offset=%2d\n", NetTick(), iRingBuffRdIndex, iRingBuffWrIndex, iRingBuffOffset));
                if (iRingBuffOffset <= 15)
                {
                    NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetps3: [%08x] waiting for room to write\n", NetTick()));
                    bExit = TRUE;
                }
            }

            if (!bExit)
            {
                int32_t iMixedBytes;

                // get data from voip mixer
                iMixedBytes = VoipMixerProcess(pHeadset->pMixerRef, (uint8_t *)&pHeadset->aFrameData[pHeadset->iFrameOffset]);
                if (iMixedBytes == (pHeadset->Param.iFrameSamples * 2))
                {
                    static float fTranslatedBuffer[VOIP_HEADSET_MAX_BLOCKS*VOIP_HEADSET_BLOCK_SIZE];
                    static float fPrevSample = 0.0f;
                    void* pSourceData = fTranslatedBuffer;
                    int32_t iIndex = 0, iAllSamples, iNumSamples, iNumBlocks;

                    // forward data to speaker callback, if callback is specified
                    if (pHeadset->pSpkrDataCb != NULL)
                    {
                        pHeadset->pSpkrDataCb(pHeadset->aFrameData, pHeadset->Param.iFrameSamples, pHeadset->pSpkrCbUserData);
                    }

                    /* the mixer will output frames with sample counts that do not result in an integral
                       block size when translated to 48khz stereo audio.  here we calculate the number of
                       integral blocks we have in samples; the excess will be moved to the start of the
                       FrameData buffer and saved for the next iteration. */
                    iAllSamples = pHeadset->iFrameOffset + pHeadset->Param.iFrameSamples;
                    // calculate integral number of blocks
                    iNumBlocks  = (iAllSamples * pHeadset->Param.iUpsampleRate * sizeof(float)) / VOIP_HEADSET_BLOCK_SIZE;
                    // now calculate output-integral number of blocks (must be a multiple of three)
                    iNumBlocks  = (iNumBlocks / 3) * 3;
                    // calculate number of samples that fit into an integral number of blocks (this will be a multiple of 64)
                    iNumSamples = (iNumBlocks * VOIP_HEADSET_BLOCK_SIZE) / (pHeadset->Param.iUpsampleRate * sizeof(float));

                    NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetps3: [%08x] fo=%3d, as=%3d, nb=%d, ns=%3d\n", NetTick(), pHeadset->iFrameOffset, iAllSamples, iNumBlocks, iNumSamples));

                    // convert 16bit integer mono sample data to 48kHz 32-bit floating-point stereo data
                    if (pHeadset->Param.iUpsampleRate == 6)
                    {
                        for (iIndex = 0; iIndex < iNumSamples; iIndex += 1)
                        {
                            fPrevSample = _VoipHeadsetAudioUpsample8To48(fPrevSample, pHeadset->aFrameData[iIndex], &fTranslatedBuffer[iIndex*6]);
                        }
                    }
                    else if (pHeadset->Param.iUpsampleRate == 12)
                    {
                        for (iIndex = 0; iIndex < iNumSamples; iIndex += 1)
                        {
                            fPrevSample = _VoipHeadsetAudioUpsample16To48(fPrevSample, pHeadset->aFrameData[iIndex], &fTranslatedBuffer[iIndex*12]);
                        }
                    }

                    /* adjust frame offset to consume submitted data, and shift the leftover samples
                       back to the start of the buffer */
                    pHeadset->iFrameOffset = iAllSamples - iNumSamples;
                    memmove(&pHeadset->aFrameData[0], &pHeadset->aFrameData[iNumSamples], pHeadset->iFrameOffset * sizeof(pHeadset->aFrameData[0]));

                    // write translated data to output buffer
                    while (pSourceData < (void *)&fTranslatedBuffer[iNumSamples*pHeadset->Param.iUpsampleRate])
                    {
                        // transfer a BLOCK of source data to the headset
                        pSourceData = _VoipHeadsetAudioWrite(pHeadset, pSourceData);
                    }
                }
                else if (iMixedBytes > 0)
                {
                    NetPrintf(("voipheadsetps3: ERROR - received partial frame.\n"));
                }
                else
                {
                    bExit = TRUE;
                }
            } // if (!bExit)
        } // while (!bExit)
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipHeadsetCreate

    \Description
        Create the headset manager.

    \Input iMaxConduits - max number of conduits
    \Input iMaxLocal    - max number of local users
    \Input *pMicDataCb  - pointer to user callback to trigger when mic data is ready
    \Input *pStatusCb   - pointer to user callback to trigger when headset status changes
    \Input *pCbUserData - pointer to user callback data

    \Output
        VoipHeadsetRefT *   - pointer to module state, or NULL if an error occured

    \Version 03/30/2004 (jbrookes)
*/
/********************************************************************************F*/
VoipHeadsetRefT *VoipHeadsetCreate(int32_t iMaxConduits, int32_t iMaxLocal, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetStatusCbT *pStatusCb, void *pCbUserData)
{
    VoipHeadsetRefT *pHeadset;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // make sure we don't exceed maxconduits
    if (iMaxConduits > VOIP_MAXCONNECT)
    {
        NetPrintf(("voipheadsetps3: request for %d conduits exceeds max\n", iMaxConduits));
        return(NULL);
    }

    // allocate and clear module state
    if ((pHeadset = DirtyMemAlloc(sizeof(*pHeadset), VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipheadsetps3: not enough memory to allocate module state\n"));
        return(NULL);
    }
    memset(pHeadset, 0, sizeof(*pHeadset));

    // set default debuglevel
    #if DIRTYCODE_LOGGING
    pHeadset->iDebugLevel = 1;
    #endif

    // register codecs
    VoipCodecRegister('celp', &VoipCelp_CodecDef);
    VoipCodecRegister('cel8', &VoipCelp8_CodecDef);
    VoipCodecRegister('dvid', &VoipDVI_CodecDef);
    VoipCodecRegister('lpcm', &VoipPCM_CodecDef);

    // allocate conduit manager
    if ((pHeadset->pConduitRef = VoipConduitCreate(iMaxConduits)) == NULL)
    {
        NetPrintf(("voipheadsetps3: unable to allocate conduit manager\n"));
        DirtyMemFree(pHeadset->pMixerRef, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }
    pHeadset->iMaxConduits = iMaxConduits;

    // set up to use dvi codec with 8khz audio by default (note this also will allocate the mixer)
    pHeadset->Param.iSampleRate = VOIP_HEADSET_DEFAULTSAMPLERATE;
    _VoipHeadsetSetCodec(pHeadset, 'dvid');

    // save callback info
    pHeadset->pMicDataCb = pMicDataCb;
    pHeadset->pStatusCb = pStatusCb;
    pHeadset->pCbUserData = pCbUserData;

    // init mic signal attributes
    pHeadset->iAGCLevel = 10000;            // agc target level
    pHeadset->fBkgndNoiseGain = 20.0f;      // 5db noise reduction gain
    pHeadset->iReverb = 240;                // reverb in milliseconds

    // init headset I/O systems
    _VoipHeadsetAudioInit(pHeadset);
    _VoipHeadsetMicInit(pHeadset);

    // return module ref to caller
    return(pHeadset);
}

/*F********************************************************************************/
/*!
    \Function   VoipHeadsetDestroy

    \Description
        Destroy the headset manager.

    \Input *pHeadset    - pointer to headset state

    \Version 03/31/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetDestroy(VoipHeadsetRefT *pHeadset)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // stop mic/audio and shut down mic/audio subsystems
    _VoipHeadsetShutdown(pHeadset);

    // free conduit manager
    VoipConduitDestroy(pHeadset->pConduitRef);

    // free mixer
    VoipMixerDestroy(pHeadset->pMixerRef);

    // destroy active codec, if any
    VoipCodecDestroy();

    // dispose of module memory
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function   VoipReceiveVoiceDataCb

    \Description
        Connectionlist callback to handle receiving a voice packet from a remote peer.

    \Input *pRemoteUser  - user we're receiving the voice data from
    \Input *pMicrPacket  - micr packet we're receiving
    \Input *pUserData    - VoipHeadsetT ref

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetReceiveVoiceDataCb(VoipUserT *pRemoteUser, VoipMicrPacketT *pMicrPacket, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;
    uint32_t uDataMap, uRemoteIndex, uMicrPkt;
    const uint8_t *pDataPacket;

    // if we're not playing, ignore it
    if (pHeadset->bPlaying == FALSE)
    {
        #if DIRTYCODE_LOGGING
        if ((pMicrPacket->MicrInfo.uSeqn % 30) == 0)
        {
            NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetps3: playback disabled, discarding voice data (seqn=%d)\n", pMicrPacket->MicrInfo.uSeqn));
        }
        #endif

        return;
    }

    // validate subpacket size
    if (pMicrPacket->MicrInfo.uSubPacketSize != pHeadset->Param.iCmpFrameSize)
    {
        NetPrintf(("voipheadsetps3: discarding voice packet with %d voice bundles and mismatched sub-packet size %d (expecting %d)\n",
            pMicrPacket->MicrInfo.uNumSubPackets, pMicrPacket->MicrInfo.uSubPacketSize, pHeadset->Param.iCmpFrameSize));
        return;
    }

    // extract data map
    uDataMap = (pMicrPacket->MicrInfo.aDataMap[0] << 24) | (pMicrPacket->MicrInfo.aDataMap[1] << 16) |
        (pMicrPacket->MicrInfo.aDataMap[2] << 8) | pMicrPacket->MicrInfo.aDataMap[3];

    // submit voice sub-packets
    for (uMicrPkt = 0; uMicrPkt < pMicrPacket->MicrInfo.uNumSubPackets; uMicrPkt++)
    {
        // get remote user index from data map
        uRemoteIndex = uDataMap >> ((pMicrPacket->MicrInfo.uNumSubPackets - uMicrPkt - 1) * 2) & 0x3;

        // get pointer to data subpacket
        pDataPacket = (const uint8_t *)pMicrPacket->aData + (uMicrPkt * pMicrPacket->MicrInfo.uSubPacketSize);

        // send it to conduit manager
        VoipConduitReceiveVoiceData(pHeadset->pConduitRef, &pRemoteUser[uRemoteIndex], pDataPacket, pMicrPacket->MicrInfo.uSubPacketSize);
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipRegisterUserCb

    \Description
        Connectionlist callback to register a new user with the XHV Engine

    \Input *pRemoteUser - user to register
    \Input iLocalIdx    - local index to register to
    \Input bRegister    - true=register, false=unregister
    \Input *pUserData   - voipheadset module ref

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetRegisterUserCb(VoipUserT *pRemoteUser, int32_t iLocalIdx, uint32_t bRegister, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;

    VoipConduitRegisterUser(pHeadset->pConduitRef, pRemoteUser, bRegister);
}

/*F********************************************************************************/
/*!
    \Function   VoipHeadsetProcess

    \Description
        Headset process function.

    \Input *pHeadset    - pointer to headset state
    \Input uFrameCount  - current frame count

    \Version 03/31/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetProcess(VoipHeadsetRefT *pHeadset, uint32_t uFrameCount)
{
    _VoipHeadsetMicProcess(pHeadset);

    _VoipHeadsetAudioProcess(pHeadset);
}

/*F********************************************************************************/
/*!
    \Function   VoipHeadsetSetVolume

    \Description
        Sets play and record volume.

    \Input *pHeadset    - pointer to headset state
    \Input iPlayVol     - play volume to set
    \Input iRecVol      - record volume to set

    \Notes
        To not set a value, specify it as -1.

    \Version 03/31/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetSetVolume(VoipHeadsetRefT *pHeadset, int32_t iPlayVol, uint32_t iRecVol)
{
    return; // not implemented
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetControl

    \Description
        Control function.

    \Input *pHeadset    - headset module state
    \Input iSelect      - control selector
    \Input iValue       - control value
    \Input iValue2      - control value
    \Input *pValue      - control value

    \Output
        int32_t         - selector specific, or -1 if no such selector

    \Notes
        iSelect can be one of the following:

        \verbatim
            'agcl' - set automatic gain control target value
            'bkgn' - set background noise level
            'cdec' - create a new codec
            'loop' - enable/disable loopback
            'mrat' - set mic input sample rate (8000hz or 16000hz)
            'rvrb' - set avg reverb time
        \endverbatim

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipHeadsetControl(VoipHeadsetRefT *pHeadset, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iSelect == 'agcl')
    {
        pHeadset->iAGCLevel = iValue;
        NetPrintf(("voipheadsetps3: setting AGCLEVEL to %d\n", pHeadset->iAGCLevel));
        return(_VoipHeadsetMicSetInputSignalAttr(pHeadset, CELLMIC_SIGATTR_AGCLEVEL, (void *)&pHeadset->iAGCLevel));
    }
    if (iSelect == 'bkgn')
    {
        void *pTemp = (void *)&iValue;
        pHeadset->fBkgndNoiseGain = *(float *)pTemp;
        NetPrintf(("voipheadsetps3: setting BKNGAIN to %f\n", pHeadset->fBkgndNoiseGain));
        return(_VoipHeadsetMicSetInputSignalAttr(pHeadset, CELLMIC_SIGATTR_BKNGAIN, (void *)&pHeadset->fBkgndNoiseGain));
    }
    if (iSelect == 'cdec')
    {
        return(_VoipHeadsetSetCodec(pHeadset, iValue));
    }
    if (iSelect == 'loop')
    {
        pHeadset->bLoopback = iValue;
        return(0);
    }
    if (iSelect == 'mrat')
    {
        return(_VoipHeadsetSetSampleRate(pHeadset, iValue));
    }
    if (iSelect == 'rvrb')
    {
        pHeadset->iReverb = iValue;
        NetPrintf(("voipheadsetps3: setting REVERB to %d\n", pHeadset->iReverb));
        return(_VoipHeadsetMicSetInputSignalAttr(pHeadset, CELLMIC_SIGATTR_REVERB, &pHeadset->iReverb));
    }
    #if DIRTYCODE_LOGGING
    if (iSelect == 'spam')
    {
        NetPrintf(("voipheadsetps3: setting debuglevel=%d\n", iValue));
        pHeadset->iDebugLevel = iValue;
        return(0);
    }
    #endif
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function    VoipHeadsetStatus

    \Description
        Status function.

    \Input *pHeadset    - headset module state
    \Input iSelect      - control selector
    \Input iValue       - selector specific
    \Input *pBuf        - buffer pointer
    \Input iBufSize     - buffer size

    \Output
        int32_t         - selector specific, or -1 if no such selector

    \Notes
        iSelect can be one of the following:

    \Version 07/28/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipHeadsetStatus(VoipHeadsetRefT *pHeadset, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetSpkrCallback

    \Description
        Set speaker output callback.

    \Input *pHeadset    - headset module state
    \Input *pCallback   - what to call when output data is available
    \Input *pUserData   - user data for callback

    \Version 10/31/2011 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetSpkrCallback(VoipHeadsetRefT *pHeadset, VoipSpkrCallbackT *pCallback, void *pUserData)
{
    pHeadset->pSpkrDataCb = pCallback;
    pHeadset->pSpkrCbUserData = pUserData;
}

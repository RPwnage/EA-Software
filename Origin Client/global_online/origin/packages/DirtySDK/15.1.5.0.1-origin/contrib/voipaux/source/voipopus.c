/*H*************************************************************************************/
/*!
    \File voipopus.c

    \Description
        PC Audio Encoder / Decoder using Opus

    \Copyright
        Copyright (c) Electronic Arts 2017. ALL RIGHTS RESERVED.

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <opus.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/voip/voip.h"
#include "voipopus.h"

/*** Defines ***************************************************************************/

//! maximum duration frame
#define VOIPOPUS_MAX_FRAME (5760)

//! sampling rate we support in Hz
#define VOIPOPUS_DEFAULT_SAMPLING_RATE (16000)

//! number of channels we support (mono or stereo)
#define VOIPOPUS_DEFAULT_CHANNELS      (1)

//! hard-coded maximum output used when encoding, this is taken from value in voippacket.h (VOIP_MAXMICRPKTSIZE)
#define VOIPOPUS_MAX_OUTPUT (1238)

/*** Type Definition *******************************************************************/

//! voipopus module state
typedef struct VoipOpusRefT
{
    VoipCodecRefT CodecState;

    int32_t iMemGroup;          //!< memgroup id
    void *pMemGroupUserData;    //!< memgroup userdata

    int32_t iVerbose;           //!< logging verbosity level
    int32_t iOutputVolume;      //!< volumn configuration

    OpusEncoder *pEncoder;      //!< opus encoder
    OpusDecoder *aDecoders[1];  //!< opus variable decoders (must come last!)
} VoipOpusRefT;

/*** Function Prototypes ***************************************************************/

static VoipCodecRefT *_VoipOpusCreate(int32_t iNumDecoders);
static void _VoipOpusDestroy(VoipCodecRefT *pState);
static int32_t _VoipOpusEncodeBlock(VoipCodecRefT *pState, uint8_t *pOutput, const int16_t *pInput, int32_t iNumSamples);
static int32_t _VoipOpusDecodeBlock(VoipCodecRefT *pState, int32_t *pOutput, const uint8_t *pInput, int32_t iInputBytes, int32_t iChannel);
static void _VoipOpusReset(VoipCodecRefT *pState);
static int32_t _VoipOpusControl(VoipCodecRefT *pState, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);
static int32_t _VoipOpusStatus(VoipCodecRefT *pState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize);

/*** Variables *************************************************************************/

//! voipopus codec definition
const VoipCodecDefT VoipOpus_CodecDef =
{
    _VoipOpusCreate,
    _VoipOpusDestroy,
    _VoipOpusEncodeBlock,
    _VoipOpusDecodeBlock,
    _VoipOpusReset,
    _VoipOpusControl,
    _VoipOpusStatus
};

#if DIRTYSOCK_ERRORNAMES
//! errors returned from the opus api
static const DirtyErrT _VoipOpus_aErrList[] =
{
    DIRTYSOCK_ErrorName(OPUS_OK),                   //  0; No Error
    DIRTYSOCK_ErrorName(OPUS_BAD_ARG),              // -1; One of more invalid/out of range arguments
    DIRTYSOCK_ErrorName(OPUS_BUFFER_TOO_SMALL),     // -2; The mode struct passed is invalid
    DIRTYSOCK_ErrorName(OPUS_INTERNAL_ERROR),       // -3; An internal error was detected
    DIRTYSOCK_ErrorName(OPUS_INVALID_PACKET),       // -4; The compressed data passed is corrupted
    DIRTYSOCK_ErrorName(OPUS_UNIMPLEMENTED),        // -5; Invalid/unsupported request number
    DIRTYSOCK_ErrorName(OPUS_INVALID_STATE),        // -6; An encoder or decoder structure is invalid or already freed
    DIRTYSOCK_ErrorName(OPUS_ALLOC_FAIL),           // -7; Memory allocation has failed
    DIRTYSOCK_ListEnd()
};
#endif

/*** Private Functions *****************************************************************/

/*F*************************************************************************************/
/*!
    \Function _VoipOpusCreate

    \Description
        Create a Opus codec state.

    \Input iNumDecoders     - number of decoder channels

    \Output
        VoipCodecStateT *   - pointer to opus codec state

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static VoipCodecRefT *_VoipOpusCreate(int32_t iNumDecoders)
{
    VoipOpusRefT *pState;
    int32_t iResult, iMemGroup, iDecoder, iMemSize;
    void *pMemGroupUserData;

    // query memgroup information
    iMemGroup = VoipStatus(NULL, 'mgrp', 0, NULL, 0);
    VoipStatus(NULL, 'mgud', 0, &pMemGroupUserData, sizeof(pMemGroupUserData));

    // allocate and initialize module state
    iMemSize = sizeof(*pState) + (sizeof(OpusDecoder *) * (iNumDecoders - 1));
    if ((pState = (VoipOpusRefT *)DirtyMemAlloc(iMemSize, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipopus: unable to allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pState, iMemSize);
    pState->CodecState.pCodecDef = &VoipOpus_CodecDef;
    pState->CodecState.iDecodeChannels = iNumDecoders;
    pState->CodecState.bVadEnabled = TRUE;
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iVerbose = 2;
    pState->iOutputVolume = 1 << VOIP_CODEC_OUTPUT_FRACTIONAL;

    // allocate and initialize the encoder
    if ((iMemSize = opus_encoder_get_size(VOIPOPUS_DEFAULT_CHANNELS)) <= 0)
    {
        NetPrintf(("voipopus: unable to get encoder size for allocation\n"));
        _VoipOpusDestroy(&pState->CodecState);
        return(NULL);
    }
    if ((pState->pEncoder = (OpusEncoder *)DirtyMemAlloc(iMemSize, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipopus: unable to allocate the encoder\n"));
        _VoipOpusDestroy(&pState->CodecState);
        return(NULL);
    }
    if ((iResult = opus_encoder_init(pState->pEncoder, VOIPOPUS_DEFAULT_SAMPLING_RATE, VOIPOPUS_DEFAULT_CHANNELS, OPUS_APPLICATION_VOIP)) != OPUS_OK)
    {
        NetPrintf(("voipopus: unable to initialize the encoder (err=%s)\n", DirtyErrGetNameList(iResult, _VoipOpus_aErrList)));

        _VoipOpusDestroy(&pState->CodecState);
        return(NULL);
    }
    // allocate and initialize the decoders
    if ((iMemSize = opus_decoder_get_size(VOIPOPUS_DEFAULT_CHANNELS)) <= 0)
    {
        NetPrintf(("voipopus: unable to get decoder size for allocation\n"));
        _VoipOpusDestroy(&pState->CodecState);
        return(NULL);
    }
    for (iDecoder = 0; iDecoder < iNumDecoders; iDecoder += 1)
    {
        if ((pState->aDecoders[iDecoder] = (OpusDecoder *)DirtyMemAlloc(iMemSize, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
        {
            NetPrintf(("voipopus: unable to allocate the decoder\n"));
            _VoipOpusDestroy(&pState->CodecState);
            return(NULL);
        }
        if ((iResult = opus_decoder_init(pState->aDecoders[iDecoder], VOIPOPUS_DEFAULT_SAMPLING_RATE, VOIPOPUS_DEFAULT_CHANNELS)) != OPUS_OK)
        {
            NetPrintf(("voipopus: unable to initialize the decoder (err=%s)\n", DirtyErrGetNameList(iResult, _VoipOpus_aErrList)));
            _VoipOpusDestroy(&pState->CodecState);
            return(NULL);
        }
    }
    return(&pState->CodecState);
}

/*F*************************************************************************************/
/*!
    \Function _VoipOpusDestroy

    \Description
        Destroy the Opus codec state

    \Input *pState  - codec state

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static void _VoipOpusDestroy(VoipCodecRefT *pState)
{
    OpusDecoder **pDecoder;
    int32_t iDecoder;
    VoipOpusRefT *pOpus = (VoipOpusRefT *)pState;

    for (iDecoder = 0; iDecoder < pOpus->CodecState.iDecodeChannels; iDecoder += 1)
    {
        if ((pDecoder = &pOpus->aDecoders[iDecoder]) != NULL)
        {
            DirtyMemFree(*pDecoder, VOIP_MEMID, pOpus->iMemGroup, pOpus->pMemGroupUserData);
            *pDecoder = NULL;
        }
    }
    if (pOpus->pEncoder != NULL)
    {
        DirtyMemFree(pOpus->pEncoder, VOIP_MEMID, pOpus->iMemGroup, pOpus->pMemGroupUserData);
        pOpus->pEncoder = NULL;
    }
    DirtyMemFree(pOpus, VOIP_MEMID, pOpus->iMemGroup, pOpus->pMemGroupUserData);
}

/*F*************************************************************************************/
/*!
    \Function _VoipOpusEncodeBlock

    \Description
        Encode a buffer 16-bit audio sample using the Opus encoder

    \Input *pState      - codec state
    \Input *pOutput     - [out] outbut buffer
    \Input *pInput      - input buffer
    \Input iNumSamples  - the number of samples to encode

    \Output
        int32_t         - positive=number of encoded bytes, negative=error

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static int32_t _VoipOpusEncodeBlock(VoipCodecRefT *pState, uint8_t *pOutput, const int16_t *pInput, int32_t iNumSamples)
{
    VoipOpusRefT *pOpus = (VoipOpusRefT *)pState;
    int32_t iResult;

    if ((iResult = opus_encode(pOpus->pEncoder, pInput, iNumSamples, pOutput, VOIPOPUS_MAX_OUTPUT)) < 0)
    {
        NetPrintf(("voipopus: unable to encode (err=%s)\n", DirtyErrGetNameList(iResult, _VoipOpus_aErrList)));
    }
    return(iResult);
}

/*F*************************************************************************************/
/*!
    \Function _VoipOpusDecodeBlock

    \Description
        Decode a Opus encoded input to 16-bit linear PCM samples

    \Input *pState      - codec state
    \Input *pOutput     - [out] outbut buffer
    \Input *pInput      - input buffer
    \Input iInputBytes  - size of the input buffer
    \Input iChannel     - the decode channel for which we are decoding data

    \Output
        int32_t         - positive=number of decoded samples, negative=error

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static int32_t _VoipOpusDecodeBlock(VoipCodecRefT *pState, int32_t *pOutput, const uint8_t *pInput, int32_t iInputBytes, int32_t iChannel)
{
    VoipOpusRefT *pOpus = (VoipOpusRefT *)pState;
    int32_t iResult, iSample;
    int16_t aOutput[VOIPOPUS_MAX_FRAME];

    if ((iChannel < 0) || (iChannel > pOpus->CodecState.iDecodeChannels))
    {
        NetPrintf(("voipopus: trying to decode with invalid decoder channel\n"));
        return(-1);
    }

    if ((iResult = opus_decode(pOpus->aDecoders[iChannel], pInput, iInputBytes, aOutput, VOIPOPUS_MAX_FRAME, 0)) < 0)
    {
        NetPrintf(("voipopus: unable to decode (err=%s)\n", DirtyErrGetNameList(iResult, _VoipOpus_aErrList)));
    }

    // accumulate output in the expected format
    for (iSample = 0; iSample < iResult; iSample += 1)
    {
        pOutput[iSample] += (aOutput[iSample] * pOpus->iOutputVolume) >> VOIP_CODEC_OUTPUT_FRACTIONAL;
    }
    return(iResult);
}

/*F*************************************************************************************/
/*!
    \Function _VoipOpusReset

    \Description
        Reset the codec state

    \Input *pState      - codec state

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static void _VoipOpusReset(VoipCodecRefT *pState)
{
    int32_t iChannel;
    VoipOpusRefT *pOpus = (VoipOpusRefT *)pState;

    opus_encoder_ctl(pOpus->pEncoder, OPUS_RESET_STATE);

    for (iChannel = 0; iChannel < pOpus->CodecState.iDecodeChannels; iChannel += 1)
    {
        opus_decoder_ctl(pOpus->aDecoders[iChannel], OPUS_RESET_STATE);
    }
}

/*F*************************************************************************************/
/*!
    \Function _VoipOpusControl

    \Description
        Set control options

    \Input *pState  - codec state
    \Input iControl - control selector
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t     - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'plvl'  - Set the output volumn
            'spam'  - Set debug output verbosity
        \endverbatim

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static int32_t _VoipOpusControl(VoipCodecRefT *pState, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    VoipOpusRefT *pOpus = (VoipOpusRefT *)pState;

    if (iControl == 'plvl')
    {
        pOpus->iOutputVolume = iValue;
        return(0);
    }
    if (iControl == 'spam')
    {
        pOpus->iVerbose = iValue;
        return(0);
    }
    // unhandled control
    return(-1);
}

/*F*************************************************************************************/
/*!
    \Function _VoipOpusStatus

    \Description
        Get codec status

    \Input *pState  - codec state
    \Input iSelect  - status selector
    \Input iValue   - selector-specific
    \Input *pBuffer - [out] storage for selector output
    \Input iBufSize - size of output buffer

    \Output
        int32_t     - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'vlen'      - returns TRUE to indicate we are using variable length frames
        \endverbatim

    \Version 07/03/2017 (eesponda)
*/
/*************************************************************************************F*/
static int32_t _VoipOpusStatus(VoipCodecRefT *pState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize)
{
    if (iSelect == 'vlen')
    {
        *(uint8_t *)pBuffer = TRUE;
        return(0);
    }
    // unhandle selector
    return(-1);
}

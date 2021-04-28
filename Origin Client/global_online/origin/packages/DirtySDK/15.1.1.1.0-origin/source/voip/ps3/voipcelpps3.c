/*H********************************************************************************/
/*!
    \File voipcelpps3.c

    \Description
        Sony CELP Audio Encoder / ADEC decoder

    \Copyright
        Copyright (c) Electronic Arts 2006. ALL RIGHTS RESERVED.

    \Version 1.0 10/25/2006 (tdills) First version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcelp.h"

#include <cell/codec.h>

/*** Defines **********************************************************************/

#define VOIP_CODEC_CELP_PPU_THREAD_PRIORITY 430
#define VOIP_CODEC_CELP_SPU_THREAD_PRIORITY 250
#define VOIP_CODEC_CELP_PPU_STACK_SIZE      8192

#define VOIP_CODEC_CELP_MAXFRAMENUM         10
#define VOIP_CODEC_CELP_MAXFRAMESAMPLES     320
#define VOIP_CODEC_CELP_MAXAUSIZE           43

#define VOIP_CODEC_ADEC_PPU_THREAD_PRIORITY 500
#define VOIP_CODEC_ADEC_THREAD_STACK_SIZE   8192
#define VOIP_CODEC_ADEC_SPU_THREAD_PRIORITY 150

#define VOIP_CODEC_RPE_DEFAULT_CONFIG       CELL_CELPENC_RPE_CONFIG_1
#define VOIP_CODEC_MPE_DEFAULT_CONFIG       CELL_CELP8ENC_MPE_CONFIG_6

/*** Macros ***********************************************************************/

#if DIRTYCODE_LOGGING
    #define VOIP_CODEC_CONFIGNAME(__strName)    __strName
#else
    #define VOIP_CODEC_CONFIGNAME(__strName)
#endif

/*** Type Definitions *************************************************************/

typedef struct VoipCelpCodecConfigT
{
    int16_t iAuSize;                //!< codec output size in bytes
    int16_t iNumSamples;            //!< number of samples codec expects as input
    #if DIRTYCODE_LOGGING
    char    strConfigName[32];
    #endif
} VoipCelpCodecConfigT;

typedef struct VoipChannelToCelpDecoderT
{
    //  index to the celp decoder object
    int32_t iDecoder;
} VoipChannelToCelpDecoderT;

typedef struct VoipCelpStateT VoipCelpStateT;

typedef struct VoipCelpDecoderT
{
    int32_t         iChannel;
    VoipCelpStateT  *pModule;
    CellAdecHandle  DecoderHandle;
    int32_t         bWaitForCb;
    int32_t         bWaitForSeqDone;

    // work area for the decoder thread
    void            *pBuffer;
} VoipCelpDecoderT;

struct VoipCelpStateT
{
    VoipCodecRefT       CodecState;

    // module memory group
    int32_t             iMemGroup;          //!< module mem group id
    void                *pMemGroupUserData; //!< user data associated with mem group

    // encoder data
    char*               pEncodeBuffer;
    CellCelpEncHandle   hEncodeHandle;

    // array of decoder structs
    int32_t             iNumChannels;
    VoipChannelToCelpDecoderT *pChannelDecoders;
    int32_t             iNumDecoders;
    VoipCelpDecoderT    *pDecoders;

    // size of encoder output/decoder input
    int32_t             iAuSize;

    // size of encoder input
    int32_t             iFrameSamples;

    #if DIRTYCODE_LOGGING
    int32_t             iNumMsBlocked;
    int32_t             iNumBlockingCalls;
    #endif
};

//! celp encoder output buffer
static uint8_t _VoipCelp_aEncodeOutputBuff[VOIP_CODEC_CELP_MAXAUSIZE * VOIP_CODEC_CELP_MAXFRAMENUM] __attribute__((aligned(128)));

//! celp verbosity (debug only)
#if DIRTYCODE_LOGGING
static int8_t _VoipCelp_iDebugLevel = 0;
#endif

/*** Function Prototypes **********************************************************/

// 16khz celp encoder
static VoipCodecRefT *_VoipCelpCreate(int32_t iNumDecoders);
static int32_t _VoipCelpEncodeBlock(VoipCodecRefT *pState, uint8_t *pOut, const int16_t *pInp, int32_t iNumSamples);

// 8khz celp encoder
static VoipCodecRefT *_VoipCelp8Create(int32_t iNumDecoders);
static int32_t _VoipCelp8EncodeBlock(VoipCodecRefT *pState, uint8_t *pOut, const int16_t *pInp, int32_t iNumSamples);

// shared (8khz and 16khz)
static void _VoipCelpDestroy(VoipCodecRefT *pState);
static int32_t _VoipCelpDecodeBlock(VoipCodecRefT *pState, int32_t *pOut, const uint8_t *pInp, int32_t iInputBytes, int32_t iChannel);
static void _VoipCelpReset(VoipCodecRefT *pState);
static int32_t _VoipCelpControl(VoipCodecRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue);
static int32_t _VoipCelpStatus(VoipCodecRefT *pCodecState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize);

/*** Variables ********************************************************************/

// Private variables

//! table of AU sizes per RPE mode (0=not supported)
static const VoipCelpCodecConfigT _VoipCelp_aRPEConfig[CELL_CELPENC_RPE_CONFIG_3+1] =
{
    { 27, 240, VOIP_CODEC_CONFIGNAME("CELL_CELPENC_RPE_CONFIG_0") },    // 14400bps, 27 bytes/au
    { 20, 160, VOIP_CODEC_CONFIGNAME("CELL_CELPENC_RPE_CONFIG_1") },    // 16000bps, 20 bytes/au
    { 35, 240, VOIP_CODEC_CONFIGNAME("CELL_CELPENC_RPE_CONFIG_2") },    // 18667bps, 35 bytes/au
    { 43, 240, VOIP_CODEC_CONFIGNAME("CELL_CELPENC_RPE_CONFIG_3") }     // 22533bps, 43 bytes/au
};

//! table of AU sizes per MPE mode (0=not supported, -1=invalid)
static const VoipCelpCodecConfigT _VoipCelp_aMPEConfig[CELL_CELP8ENC_MPE_CONFIG_26+1] =
{
    { 20, 320, VOIP_CODEC_CONFIGNAME("CELL_CELP8ENC_MPE_CONFIG_0") },   //  3850bps, 20 bytes/au
    { -1,  -1, VOIP_CODEC_CONFIGNAME("") },
    { 24, 320, VOIP_CODEC_CONFIGNAME("CELL_CELP8ENC_MPE_CONFIG_2") },   //  4650bps, 24 bytes/au
    { -1,  -1, VOIP_CODEC_CONFIGNAME("") }, { -1, -1, VOIP_CODEC_CONFIGNAME("") }, { -1, -1, VOIP_CODEC_CONFIGNAME("") },
    { 15, 160, VOIP_CODEC_CONFIGNAME("CELL_CELP8ENC_MPE_CONFIG_6") },   //  5700bps, 15 bytes/au
    { -1,  -1, VOIP_CODEC_CONFIGNAME("") }, { -1, -1, VOIP_CODEC_CONFIGNAME("") },
    { 17, 160, VOIP_CODEC_CONFIGNAME("CELL_CELP8ENC_MPE_CONFIG_9") },   //  6600bps, 17 bytes/au
    { -1,  -1, VOIP_CODEC_CONFIGNAME("") }, { -1, -1, VOIP_CODEC_CONFIGNAME("") },
    { 19, 160, VOIP_CODEC_CONFIGNAME("CELL_CELP8ENC_MPE_CONFIG_12") },  //  7300bps, 19 bytes/au
    { -1,  -1, VOIP_CODEC_CONFIGNAME("") }, { -1, -1, VOIP_CODEC_CONFIGNAME("") },
    { 22, 160, VOIP_CODEC_CONFIGNAME("CELL_CELP8ENC_MPE_CONFIG_15") },  //  8700bps, 22 bytes/au
    { -1,  -1, VOIP_CODEC_CONFIGNAME("") }, { -1, -1, VOIP_CODEC_CONFIGNAME("") },
    { 25, 160, VOIP_CODEC_CONFIGNAME("CELL_CELP8ENC_MPE_CONFIG_18") },  //  9900bps, 25 bytes/au
    { -1,  -1, VOIP_CODEC_CONFIGNAME("") }, { -1, -1, VOIP_CODEC_CONFIGNAME("") },
    { 27, 160, VOIP_CODEC_CONFIGNAME("CELL_CELP8ENC_MPE_CONFIG_21") },  // 10700bps, 27 bytes/au
    { -1,  -1, VOIP_CODEC_CONFIGNAME("") }, { -1, -1, VOIP_CODEC_CONFIGNAME("") },
    { 15,  80, VOIP_CODEC_CONFIGNAME("CELL_CELP8ENC_MPE_CONFIG_24") },  // 11800bps, 15 bytes/au
    { -1,  -1, VOIP_CODEC_CONFIGNAME("") },
    { 16,  80, VOIP_CODEC_CONFIGNAME("CELL_CELP8ENC_MPE_CONFIG_26") },  // 12200bps, 16 bytes/au
};

//! RPE config to use
static int32_t _VoipCelp_iRPEConfig = VOIP_CODEC_RPE_DEFAULT_CONFIG;

//! MPE config to use
static int32_t _VoipCelp_iMPEConfig = VOIP_CODEC_MPE_DEFAULT_CONFIG;

//! SPURS info to use
static union
{
    CellCelpEncResourceEx EncodeResEx;      //!< for cellCelpEncOpenEx
    CellCelp8EncResourceEx Encode8ResEx;    //!< for cellCelp8EncOpenEx
    // the totalMemSize and startAddr of EncodeResEx/Encode8ResEx are ignored
} _VoipCelp_SPURSInfo;

// Public variables

//! 16khz celp codec (RPE)
const VoipCodecDefT VoipCelp_CodecDef =
{
    _VoipCelpCreate,
    _VoipCelpDestroy,
    _VoipCelpEncodeBlock,
    _VoipCelpDecodeBlock,
    _VoipCelpReset,
    _VoipCelpControl,
    _VoipCelpStatus
};

//! 8khz celp codec (MPE)
const VoipCodecDefT VoipCelp8_CodecDef =
{
    _VoipCelp8Create,
    _VoipCelpDestroy,
    _VoipCelp8EncodeBlock,
    _VoipCelpDecodeBlock,
    _VoipCelpReset,
    _VoipCelpControl,
    _VoipCelpStatus
};


/*** Private Functions ************************************************************/


/*F*************************************************************************************************/
/*!
    \Function _VoipCelpSetConfig

    \Description
        Set RPE or MPE config to use for celp codec.

    \Input *pConfigList - RPE or MPE config list to select from
    \Input iNewConfig   - requested config index
    \Input iCurConfig   - currently selected config (returned if new config is not valid or supported)
    \Input iMaxConfig   - max valid config index
    \Input *pConfigType - "RPE" or "MPE"

    \Output
        int32_t         - iNewConfig if valid, else iCurConfig

    \Version 10/06/2011 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _VoipCelpSetConfig(const VoipCelpCodecConfigT *pConfigList, int32_t iNewConfig, int32_t iCurConfig, int32_t iMaxConfig, const char *pConfigType)
{
    // validate config
    if ((iNewConfig < 0) || (iNewConfig > iMaxConfig) || (pConfigList[iNewConfig].iAuSize == -1))
    {
        NetPrintf(("voipcelpps3: invalid %s config %d selected\n", pConfigType, iNewConfig));
        return(iCurConfig);
    }
    // supported?
    if (pConfigList[iNewConfig].iAuSize == 0)
    {
        NetPrintf(("voipcelpps3: %s config %s is not supported\n", pConfigType, pConfigList[iNewConfig].strConfigName));
        return(iCurConfig);
    }
    // set new config
    NetPrintf(("voipcelpps3: setting %s config to %s\n", pConfigType, pConfigList[iNewConfig].strConfigName));
    return(iNewConfig);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelpCreateEncoder

    \Description
        Create and start the celp encoder

    \Input  pState - the module state

    \Output
        int32_t     - (bool) success or failure.

    \Version 10/25/2006 (tdills)
*/
/*************************************************************************************************F*/
static int32_t _VoipCelpCreateEncoder(VoipCelpStateT *pState)
{
    int32_t iResult;
    CellCelpEncAttr EncodeAttr;
    CellCelpEncParam EncodeParams;

    // get the size of the memory buffer needed by the encoder
    if ((iResult = cellCelpEncQueryAttr(&EncodeAttr)) < CELL_OK)
    {
        NetPrintf(("voipcelpps3: cellCelpEncQueryAttr() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(FALSE);
    }
    NetPrintfVerbose((_VoipCelp_iDebugLevel, 0, "voipcelpps3: got size as %d from cellCelpEncQueryAttr\n", EncodeAttr.workMemSize));

    // allocate memory for the encoder to use
    pState->pEncodeBuffer = (char*)DirtyMemAlloc(EncodeAttr.workMemSize, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    if (pState->pEncodeBuffer == NULL)
    {
        NetPrintf(("voipcelpps3: DirtyMemAlloc() failed\n"));
        return(FALSE);
    }
    NetPrintfVerbose((_VoipCelp_iDebugLevel, 0, "voipcelpps3: encode buffer memory address = %p\n", pState->pEncodeBuffer));

    // if we're re-using spurs instance
    if (_VoipCelp_SPURSInfo.EncodeResEx.spurs != NULL)
    {
        CellCelpEncResourceEx EncodeResEx;
        // setup the parameters and call cellCelpEncOpenEx
        ds_memcpy_s(&EncodeResEx, sizeof(EncodeResEx), &_VoipCelp_SPURSInfo.EncodeResEx, sizeof(_VoipCelp_SPURSInfo.EncodeResEx));
        EncodeResEx.startAddr = pState->pEncodeBuffer;
        EncodeResEx.totalMemSize = EncodeAttr.workMemSize;
        if ((iResult = cellCelpEncOpenEx(&EncodeResEx, &pState->hEncodeHandle)) < CELL_OK)
        {
            NetPrintf(("voipcelpps3: cellCelpEncOpenEx() failed (err=%s)\n", DirtyErrGetName(iResult)));
            DirtyMemFree(pState->pEncodeBuffer, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            return(FALSE);
        }
    }
    else
    {
        CellCelpEncResource EncodeRes;
        // setup the parameters and call cellCelpEncOpen
        EncodeRes.ppuThreadPriority = VOIP_CODEC_CELP_PPU_THREAD_PRIORITY;
        EncodeRes.spuThreadPriority = VOIP_CODEC_CELP_SPU_THREAD_PRIORITY;
        EncodeRes.ppuThreadStackSize = VOIP_CODEC_CELP_PPU_STACK_SIZE;
        EncodeRes.startAddr = pState->pEncodeBuffer;
        EncodeRes.totalMemSize = EncodeAttr.workMemSize;
        if ((iResult = cellCelpEncOpen(&EncodeRes, &pState->hEncodeHandle)) < CELL_OK)
        {
            NetPrintf(("voipcelpps3: cellCelpEncOpen() failed (err=%s)\n", DirtyErrGetName(iResult)));
            DirtyMemFree(pState->pEncodeBuffer, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            return(FALSE);
        }
    }
    NetPrintfVerbose((_VoipCelp_iDebugLevel, 0, "voipcelpps3: celp encoder opened: handle = %p\n", pState->hEncodeHandle));

    // set up the parameters and start the encoder
    EncodeParams.excitationMode = CELL_CELPENC_EXCITATION_MODE_RPE;
    EncodeParams.sampleRate = CELL_CELPENC_FS_16kHz;
    EncodeParams.configuration = _VoipCelp_iRPEConfig;
    EncodeParams.wordSize = CELL_CELPENC_WORD_SZ_FLOAT;
    EncodeParams.outBuff = _VoipCelp_aEncodeOutputBuff;
    if ((iResult = cellCelpEncStart(pState->hEncodeHandle, &EncodeParams)) < CELL_OK)
    {
        NetPrintf(("voipcelpps3: cellCelpEncStart() failed (err=%s)\n", DirtyErrGetName(iResult)));
        DirtyMemFree(pState->pEncodeBuffer, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(FALSE);
    }

    // remember AU size (encoder output, decoder input)
    pState->iAuSize       = _VoipCelp_aRPEConfig[EncodeParams.configuration].iAuSize;
    pState->iFrameSamples = _VoipCelp_aRPEConfig[EncodeParams.configuration].iNumSamples;
    NetPrintf(("voipcelpps3: config=%s (framesamples=%d, ausize=%d)\n", _VoipCelp_aRPEConfig[EncodeParams.configuration].strConfigName,
        pState->iFrameSamples, pState->iAuSize));
    NetPrintf(("voipcelpps3: codec bps is %d (8000hz) or %d (16000hz)\n", (8*8000*pState->iAuSize)/pState->iFrameSamples,
        (8*16000*pState->iAuSize)/pState->iFrameSamples));
    return(TRUE);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelp8CreateEncoder

    \Description
        Create and start the celp8 encoder

    \Input  pState - the module state

    \Output
        int32_t     - (bool) success or failure.

    \Version 10/05/2011 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _VoipCelp8CreateEncoder(VoipCelpStateT *pState)
{
    int32_t iResult;
    CellCelp8EncAttr EncodeAttr;
    CellCelp8EncParam EncodeParams;

    // get the size of the memory buffer needed by the encoder
    if ((iResult = cellCelp8EncQueryAttr(&EncodeAttr)) < CELL_OK)
    {
        NetPrintf(("voipcelpps3: cellCelp8EncQueryAttr() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(FALSE);
    }
    NetPrintfVerbose((_VoipCelp_iDebugLevel, 0, "voipcelpps3: got size as %d from cellCelp8EncQueryAttr\n", EncodeAttr.workMemSize));

    // allocate memory for the encoder to use
    pState->pEncodeBuffer = (char*)DirtyMemAlloc(EncodeAttr.workMemSize, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    if (pState->pEncodeBuffer == NULL)
    {
        NetPrintf(("voipcelpps3: DirtyMemAlloc() failed\n"));
        return(FALSE);
    }
    NetPrintfVerbose((_VoipCelp_iDebugLevel, 0, "voipcelpps3: encode buffer memory address = %p\n", pState->pEncodeBuffer));

    // if we're re-using spurs instance
    if (_VoipCelp_SPURSInfo.Encode8ResEx.spurs != NULL)
    {
        CellCelp8EncResourceEx EncodeResEx;
        // setup the parameters and call cellCelp8EncOpenEx
        ds_memcpy_s(&EncodeResEx, sizeof(EncodeResEx), &_VoipCelp_SPURSInfo.Encode8ResEx, sizeof(_VoipCelp_SPURSInfo.Encode8ResEx));
        EncodeResEx.startAddr = pState->pEncodeBuffer;
        EncodeResEx.totalMemSize = EncodeAttr.workMemSize;
        if ((iResult = cellCelp8EncOpenEx(&EncodeResEx, &pState->hEncodeHandle)) < CELL_OK)
        {
            NetPrintf(("voipcelpps3: cellCelp8EncOpenEx() failed (err=%s)\n", DirtyErrGetName(iResult)));
            DirtyMemFree(pState->pEncodeBuffer, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            return(FALSE);
        }
    }
    else
    {
        CellCelp8EncResource EncodeRes;
        // setup the parameters and call cellCelp8EncOpen
        EncodeRes.ppuThreadPriority = VOIP_CODEC_CELP_PPU_THREAD_PRIORITY;
        EncodeRes.spuThreadPriority = VOIP_CODEC_CELP_SPU_THREAD_PRIORITY;
        EncodeRes.ppuThreadStackSize = VOIP_CODEC_CELP_PPU_STACK_SIZE;
        EncodeRes.startAddr = pState->pEncodeBuffer;
        EncodeRes.totalMemSize = EncodeAttr.workMemSize;
        if ((iResult = cellCelp8EncOpen(&EncodeRes, &pState->hEncodeHandle)) < CELL_OK)
        {
            NetPrintf(("voipcelpps3: cellCelp8EncOpen() failed (err=%s)\n", DirtyErrGetName(iResult)));
            DirtyMemFree(pState->pEncodeBuffer, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            return(FALSE);
        }
    }
    NetPrintfVerbose((_VoipCelp_iDebugLevel, 0, "voipcelpps3: celp8 encoder opened: handle = %p\n", pState->hEncodeHandle));

    // set up the parameters and start the encoder
    EncodeParams.excitationMode = CELL_CELP8ENC_EXCITATION_MODE_MPE;
    EncodeParams.sampleRate = CELL_CELP8ENC_FS_8kHz;
    EncodeParams.configuration = _VoipCelp_iMPEConfig;
    EncodeParams.wordSize = CELL_CELP8ENC_WORD_SZ_FLOAT;
    EncodeParams.outBuff = _VoipCelp_aEncodeOutputBuff;
    if ((iResult = cellCelp8EncStart(pState->hEncodeHandle, &EncodeParams)) < CELL_OK)
    {
        NetPrintf(("voipcelpps3: cellCelp8EncStart() failed (err=%s)\n", DirtyErrGetName(iResult)));
        DirtyMemFree(pState->pEncodeBuffer, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(FALSE);
    }

    // remember AU size (encoder output, decoder input)
    pState->iAuSize       = _VoipCelp_aMPEConfig[EncodeParams.configuration].iAuSize;
    pState->iFrameSamples = _VoipCelp_aMPEConfig[EncodeParams.configuration].iNumSamples;
    NetPrintf(("voipcelpps3: config=%s (framesamples=%d, ausize=%d)\n", _VoipCelp_aMPEConfig[EncodeParams.configuration].strConfigName,
        pState->iFrameSamples, pState->iAuSize));
    NetPrintf(("voipcelpps3: codec bps is %d (8000hz) or %d (16000hz)\n", (8*8000*pState->iAuSize)/pState->iFrameSamples,
        (8*16000*pState->iAuSize)/pState->iFrameSamples));
    return(TRUE);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelpDestroyEncoder

    \Description
        Undo what _VoipCelpCreateEncode does.

    \Input  pModuleState - The module reference

    \Version 10/25/2006 (tdills)
*/
/*************************************************************************************************F*/
static void _VoipCelpDestroyEncoder(VoipCelpStateT *pModuleState)
{
    int32_t iResult;

    // end the encoder
    if ((iResult = cellCelpEncEnd(pModuleState->hEncodeHandle)) < CELL_OK)
    {
        NetPrintf(("voipcelpps3: cellCelpEncEnd failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // stop the encoder
    if ((iResult = cellCelpEncClose(pModuleState->hEncodeHandle)) < CELL_OK)
    {
        NetPrintf(("voipcelpps3: cellCelpEncClose failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // destroy the encoder memory
    DirtyMemFree(pModuleState->pEncodeBuffer, VOIP_MEMID, pModuleState->iMemGroup, pModuleState->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelp8DestroyEncoder

    \Description
        Undo what _VoipCelp8CreateEncode does.

    \Input  pModuleState - The module reference

    \Version 10/05/2011 (jbrookes)
*/
/*************************************************************************************************F*/
static void _VoipCelp8DestroyEncoder(VoipCelpStateT *pModuleState)
{
    int32_t iResult;

    // end the encoder
    if ((iResult = cellCelp8EncEnd(pModuleState->hEncodeHandle)) < CELL_OK)
    {
        NetPrintf(("voipcelpps3: cellCelpEncEnd failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // stop the encoder
    if ((iResult = cellCelp8EncClose(pModuleState->hEncodeHandle)) < CELL_OK)
    {
        NetPrintf(("voipcelpps3: cellCelpEncClose failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // destroy the encoder memory
    DirtyMemFree(pModuleState->pEncodeBuffer, VOIP_MEMID, pModuleState->iMemGroup, pModuleState->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelpDecodeCb

    \Description
        Callback for receiving notifications from the celp decoder
        Note: Netprintfs where removed from this function as they caused issues to appear in nested
        system calls, like 'new' and variable arguments processing

    \Input  AdecHandle  - handle to the Adec module
    \Input  MsgType     - the type of message
    \Input  iMessage    - the message data
    \Input  *pArg       - our decoder object

    \Output
        int32_t         - 0

    \Version 10/26/2006 (tdills)
*/
/*************************************************************************************************F*/
static int32_t _VoipCelpDecodeCb(CellAdecHandle AdecHandle, CellAdecMsgType MsgType, int32_t iMessage, void *pArg)
{
    VoipCelpDecoderT *pDecoder = (VoipCelpDecoderT*)pArg;

    switch (MsgType)
    {
        case CELL_ADEC_MSG_TYPE_AUDONE:
            break;
        case CELL_ADEC_MSG_TYPE_PCMOUT:
            pDecoder->bWaitForCb = FALSE;
            break;
        case CELL_ADEC_MSG_TYPE_SEQDONE:
            pDecoder->bWaitForSeqDone = FALSE;
            break;
        default:
            break;
    }

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelpCreateDecoder

    \Description
        Create a celp decoder - one per channel

    \Input pState       - the module state
    \Input *pDecoder    - the decoder state object we are initializing
    \Input iDecoder     - the decoder index (identifier)
    \Input iSampleRate  - sample rate (8000 or 16000)

    \Output
        int32_t         - (bool) success or failure

    \Version 10/26/2006 (tdills)
*/
/*************************************************************************************************F*/
static int32_t _VoipCelpCreateDecoder(VoipCelpStateT *pState, VoipCelpDecoderT *pDecoder, int32_t iDecoder, int32_t iSampleRate)
{
    int32_t iResult;
    CellAdecType adecType;
    CellAdecAttr adecAttr;
    CellAdecResource adecRes = { 0, NULL, VOIP_CODEC_ADEC_PPU_THREAD_PRIORITY, VOIP_CODEC_ADEC_SPU_THREAD_PRIORITY, VOIP_CODEC_ADEC_THREAD_STACK_SIZE };
    CellAdecCb      adecCb = { _VoipCelpDecodeCb, pDecoder };
    CellAdecParamCelp   adecDecoderParams;
    CellAdecParamCelp8  adecDecoderParams8;
    void *pDecoderParams;

    // set type and rate
    if (iSampleRate == 16000)
    {
        adecType.audioCodecType = CELL_ADEC_TYPE_CELP;
        adecDecoderParams.excitationMode = CELL_ADEC_CELP_EXCITATION_MODE_RPE;
        adecDecoderParams.sampleRate = CELL_ADEC_FS_16kHz;
        adecDecoderParams.configuration = _VoipCelp_iRPEConfig;
        adecDecoderParams.wordSize = CELL_ADEC_CELP_WORD_SZ_FLOAT;
        pDecoderParams = &adecDecoderParams;
    }
    else
    {
        adecType.audioCodecType = CELL_ADEC_TYPE_CELP8;
        adecDecoderParams8.excitationMode = CELL_ADEC_CELP8_EXCITATION_MODE_MPE;
        adecDecoderParams8.sampleRate = CELL_ADEC_FS_8kHz;
        adecDecoderParams8.configuration = _VoipCelp_iMPEConfig;
        adecDecoderParams8.wordSize = CELL_ADEC_CELP8_WORD_SZ_FLOAT;
        pDecoderParams = &adecDecoderParams8;
    }

    // initialize the decoder object with the data we'll need in the callback
    memset(pDecoder, 0, sizeof(VoipCelpDecoderT));
    pDecoder->iChannel = iDecoder;
    pDecoder->pModule = pState;

    // find out how much memory we need to allocate
    if ((iResult = cellAdecQueryAttr(&adecType, &adecAttr)) < CELL_OK)
    {
        NetPrintf(("voipcelpps3: cellAdecQueryAttr failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(FALSE);
    }

    // allocate memory for the decoder
    pDecoder->pBuffer = DirtyMemAlloc(adecAttr.workMemSize, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    if (pDecoder->pBuffer == NULL)
    {
        NetPrintf(("voipcelpps3: failed to allocate %d bytes for the adec decoder\n", adecAttr.workMemSize));
        return(FALSE);
    }

    // open the decoder
    adecRes.startAddr = pDecoder->pBuffer;
    adecRes.totalMemSize = adecAttr.workMemSize;
    if ((iResult = cellAdecOpen(&adecType, &adecRes, &adecCb, &pDecoder->DecoderHandle)) < CELL_OK)
    {
        NetPrintf(("voipcelpps3: cellAdecOpen failed (err=%s)\n", DirtyErrGetName(iResult)));
        DirtyMemFree(pDecoder->pBuffer, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(FALSE);
    }

    // start the decoder
    if ((iResult = cellAdecStartSeq(pDecoder->DecoderHandle, pDecoderParams)) < CELL_OK)
    {
        NetPrintf(("voipcelpps3: cellAdecStartSeq failed (err=%s)\n", DirtyErrGetName(iResult)));
        DirtyMemFree(pDecoder->pBuffer, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(FALSE);
    }

    return(TRUE);
}

/*F*************************************************************************************************/
/*!
    \Function __VoipCelpCreate

    \Description
        Create a celp codec state.

    \Input iDecodeChannels  - number of decoder channels
    \Input iSampleRate      - sample rate (8000 or 16000, used to choose celp or celp8)

    \Output
        VoipCodecStateT *   - pointer to celp codec state

    \Version 10/25/2006 (tdills)
*/
/*************************************************************************************************F*/
static VoipCodecRefT *__VoipCelpCreate(int32_t iDecodeChannels, int32_t iSampleRate)
{
    int32_t iChannel;
    int32_t iDecoder;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query mem group data
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate memory for the state structure and initialize the variables
    VoipCelpStateT *pState = (VoipCelpStateT *)DirtyMemAlloc(sizeof(VoipCelpStateT), VOIP_MEMID, iMemGroup, pMemGroupUserData);
    memset(pState, 0, sizeof(VoipCelpStateT));
    NetPrintfVerbose((_VoipCelp_iDebugLevel, 0, "voipcelpps3: allocated module reference at %p\n", pState));

    // initialize the state variables
    pState->CodecState.iDecodeChannels = iDecodeChannels;
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;

    // create and start the encoder
    if (iSampleRate == 16000)
    {
        pState->CodecState.pCodecDef = &VoipCelp_CodecDef;
        if (!_VoipCelpCreateEncoder(pState))
        {
            DirtyMemFree(pState, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            return(NULL);
        }
    }
    else
    {
        pState->CodecState.pCodecDef = &VoipCelp8_CodecDef;
        if (!_VoipCelp8CreateEncoder(pState))
        {
            DirtyMemFree(pState, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            return(NULL);
        }
    }

    // allocate an array of pointers to hold pointers to one decoder per channel
    pState->iNumDecoders = 1;
    pState->pDecoders = DirtyMemAlloc(sizeof(VoipCelpDecoderT) * pState->iNumDecoders, VOIP_MEMID, iMemGroup, pMemGroupUserData);
    NetPrintfVerbose((_VoipCelp_iDebugLevel, 0, "voipcelpps3: decoders allocated %d bytes at %p\n", sizeof(VoipCelpDecoderT) * pState->iNumDecoders, pState->pDecoders));

    for (iDecoder=0; iDecoder < pState->iNumDecoders; iDecoder++)
    {
        if (!_VoipCelpCreateDecoder(pState, &pState->pDecoders[iDecoder], iDecoder, iSampleRate))
        {
            NetPrintf(("voipcelpps3: _VoipCelpCreateDecoder() failed for decoder %d\n", iDecoder));
            _VoipCelpDestroyEncoder(pState);
            DirtyMemFree(pState->pDecoders, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            DirtyMemFree(pState, VOIP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            return(NULL);
        }
    }
    NetPrintfVerbose((_VoipCelp_iDebugLevel, 0, "voipcelpps3: celp decoders allocated = %d\n", pState->iNumDecoders));

    // allocate a celp decoder(s) for a set of decoder channels based on the ratio of iNumDecoders to iNumChannels.
    pState->iNumChannels = iDecodeChannels;
    pState->pChannelDecoders = (VoipChannelToCelpDecoderT *)DirtyMemAlloc(sizeof(VoipChannelToCelpDecoderT) * iDecodeChannels, VOIP_MEMID, iMemGroup, pMemGroupUserData);
    memset(pState->pChannelDecoders, 0, sizeof(VoipChannelToCelpDecoderT) * iDecodeChannels);
    NetPrintfVerbose((_VoipCelp_iDebugLevel, 0, "voipcelpps3: channel->decoder map allocated %d bytes at %p\n", sizeof(VoipChannelToCelpDecoderT) * iDecodeChannels, pState->pChannelDecoders));

    for (iChannel=0; iChannel < pState->iNumChannels; iChannel++)
    {
        pState->pChannelDecoders[iChannel].iDecoder = iChannel * pState->iNumDecoders /  pState->iNumChannels;
    }

    NetPrintfVerbose((_VoipCelp_iDebugLevel, 0, "voipcelpps3: returning %p\n", &pState->CodecState));
    return(&pState->CodecState);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelpCreate

    \Description
        Create 16khz celp codec state.

    \Input iDecodeChannels  - number of decoder channels

    \Output
        VoipCodecStateT *   - pointer to celp codec state

    \Version 10/05/2011 (jbrookes)
*/
/*************************************************************************************************F*/
static VoipCodecRefT *_VoipCelpCreate(int32_t iDecodeChannels)
{
    return(__VoipCelpCreate(iDecodeChannels, 16000));
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelp8Create

    \Description
        Create 8khz celp codec state.

    \Input iDecodeChannels  - number of decoder channels

    \Output
        VoipCodecStateT *   - pointer to celp codec state

    \Version 10/05/2011 (jbrookes)
*/
/*************************************************************************************************F*/
static VoipCodecRefT *_VoipCelp8Create(int32_t iDecodeChannels)
{
    return(__VoipCelpCreate(iDecodeChannels, 8000));
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelpDestroy

    \Description
        Destroy a celp codec state.

    \Input  *pState - state to destroy

    \Version 10/25/2006 (tdills)
*/
/*************************************************************************************************F*/
static void _VoipCelpDestroy(VoipCodecRefT *pState)
{
    VoipCelpStateT* pModuleState = (VoipCelpStateT*)pState;
    int32_t iDecoder;

    // clean up the encoder
    if (pModuleState->CodecState.pCodecDef == &VoipCelp_CodecDef)
    {
        _VoipCelpDestroyEncoder(pModuleState);
    }
    else
    {
        _VoipCelp8DestroyEncoder(pModuleState);
    }

    // clean up decoder(s)
    for (iDecoder = 0; iDecoder < pModuleState->iNumDecoders; iDecoder++)
    {
        // set to wait for decoder end
        pModuleState->pDecoders[iDecoder].bWaitForSeqDone = TRUE;

        // stop the decoder and wait for the callback
        cellAdecEndSeq(pModuleState->pDecoders[iDecoder].DecoderHandle);
        while (pModuleState->pDecoders[iDecoder].bWaitForSeqDone)
        {
            sys_timer_usleep(500);
        }

        // close the decoder
        cellAdecClose(pModuleState->pDecoders[iDecoder].DecoderHandle);

        // destroy the decoder memory
        DirtyMemFree(pModuleState->pDecoders[iDecoder].pBuffer, VOIP_MEMID, pModuleState->iMemGroup, pModuleState->pMemGroupUserData);
    }

    // destroy the array of decoder pointers
    DirtyMemFree(pModuleState->pDecoders, VOIP_MEMID, pModuleState->iMemGroup, pModuleState->pMemGroupUserData);
    DirtyMemFree(pModuleState->pChannelDecoders, VOIP_MEMID, pModuleState->iMemGroup, pModuleState->pMemGroupUserData);

    // destroy the module
    DirtyMemFree(pModuleState, VOIP_MEMID, pModuleState->iMemGroup, pModuleState->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelpEncodeBlock

    \Description
        Encode a buffer 16khz audio samples using Sony's celp encoder.

    \Input *pState      - pointer to encode state
    \Input *pOut        - pointer to output buffer
    \Input *pInp        - pointer to input buffer
    \Input iNumSamples  - number of samples to encode

    \Output
        int32_t         - size of compressed data in bytes, or zero

    \Notes
        From the Sony documentation:

        There is a logical delay of 2 frames in the CELP encoder specifications.  The data output
        in the two frames after starting the encoding sequence will be 0 bytes, and the AU
        corresponding to the first frame of input will be output the third time this function is
        called.

        Also, to flush out all the frames input to the encoder, it is necessary to retrieve AUs
        twice after the last frame is input.

    \Version 10/25/2006 (tdills)
*/
/*************************************************************************************************F*/
static int32_t _VoipCelpEncodeBlock(VoipCodecRefT *pState, uint8_t *pOut, const int16_t *pInp, int32_t iNumSamples)
{
    CellCelpEncPcmInfo PcmInfo;
    CellCelpEncAuInfo  AuInfo;
    VoipCelpStateT *pModule = (VoipCelpStateT *)pState;
    int32_t iResult;
    float fFrameBuffer[VOIP_CODEC_CELP_MAXFRAMESAMPLES];
    uint8_t *pOutBuff = pOut;
    const int16_t *pInBuff = pInp;
    int32_t iNumPackets = iNumSamples / pModule->iFrameSamples;
    int32_t iPacket, iSample, iNumEncodedBytes = 0;
    #if DIRTYCODE_LOGGING
    int32_t iStartTick;
    #endif

    NetPrintfVerbose((_VoipCelp_iDebugLevel, 2, "voipcelpps3: encoding block at %p of %d samples\n", pInp, iNumSamples));

    if ((iNumSamples % pModule->iFrameSamples) != 0)
    {
        NetPrintf(("voipcelpps3: error - celp encoder can only encode multiples of %d samples (%d submitted).\n", pModule->iFrameSamples, iNumSamples));
        return(0);
    }
    if (iNumPackets == 0)
    {
        NetPrintf(("voipcelpps3: celp encoder received no data to encode\n"));
        return(0);
    }

    // compress incoming samples into packet frames
    for (iPacket = 0; iPacket < iNumPackets; iPacket++)
    {
        // convert the frame to floats
        for (iSample = 0; iSample < pModule->iFrameSamples; iSample++)
        {
            fFrameBuffer[iSample] = pInBuff[iSample];
            fFrameBuffer[iSample] = fFrameBuffer[iSample] / 32767.0f;
        }

        // send the input data to the encoder
        PcmInfo.startAddr = fFrameBuffer;
        PcmInfo.size = pModule->iFrameSamples * sizeof(float);
        if ((iResult = cellCelpEncEncodeFrame(pModule->hEncodeHandle, &PcmInfo)) < CELL_OK)
        {
            NetPrintf(("voipcelpps3: cellCelpEncEncodeFrame failed (err=%s)\n", DirtyErrGetName(iResult)));
            return(0);
        }

        // verbose debug will determine how long the blocking call blocks
        #if DIRTYCODE_LOGGING
        iStartTick = NetTick();
        #endif

        // wait for the encoder to finish ** this is blocking **
        if ((iResult = cellCelpEncWaitForOutput(pModule->hEncodeHandle)) < CELL_OK)
        {
            NetPrintf(("voipcelpps3: cellCelpEncWaitForOutput failed (err=%s)\n", DirtyErrGetName(iResult)));
            return(0);
        }

        #if DIRTYCODE_LOGGING
        pModule->iNumMsBlocked += NetTickDiff(NetTick(), iStartTick);
        pModule->iNumBlockingCalls += 1;
        if (pModule->iNumBlockingCalls >= 100)
        {
            NetPrintfVerbose((_VoipCelp_iDebugLevel, 1, "voipcelpps3: WaitForOutput blocked for a total of %dms over %d packets (%2.2f ms/packet)\n", pModule->iNumMsBlocked, pModule->iNumBlockingCalls,
                (float)pModule->iNumMsBlocked/(float)pModule->iNumBlockingCalls));
            pModule->iNumBlockingCalls = 0;
            pModule->iNumMsBlocked = 0;
        }
        #endif

        // get the output data
        if ((iResult = cellCelpEncGetAu(pModule->hEncodeHandle, pOutBuff, &AuInfo)) < CELL_OK)
        {
            NetPrintf(("voipcelpps3: cellCelpEncGetAu failed (err=%s)\n", DirtyErrGetName(iResult)));
            return(0);
        }

        // move the pointers
        pInBuff += pModule->iFrameSamples;
        pOutBuff += AuInfo.size;
        iNumEncodedBytes += AuInfo.size;
        NetPrintfVerbose((_VoipCelp_iDebugLevel, 3, "voipcelpps3: encoded a block of %d bytes and output %d bytes\n", pModule->iFrameSamples * sizeof(float), AuInfo.size));
    }

    return(iNumEncodedBytes);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelp8EncodeBlock

    \Description
        Encode a buffer 16khz audio samples using Sony's celp encoder.

    \Input *pState      - pointer to encode state
    \Input *pOut        - pointer to output buffer
    \Input *pInp        - pointer to input buffer
    \Input iNumSamples  - number of samples to encode

    \Output
        int32_t         - size of compressed data in bytes

    \Notes
        From the Sony documentation:

        There is a logical delay of 2 frames in the CELP encoder specifications.  The data output
        in the two frames after starting the encoding sequence will be 0 bytes, and the AU
        corresponding to the first frame of input will be output the third time this function is
        called.

        Also, to flush out all the frames input to the encoder, it is necessary to retrieve AUs
        twice after the last frame is input.

    \Version 10/05/2011 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _VoipCelp8EncodeBlock(VoipCodecRefT *pState, uint8_t *pOut, const int16_t *pInp, int32_t iNumSamples)
{
    CellCelp8EncPcmInfo PcmInfo;
    CellCelp8EncAuInfo  AuInfo;
    VoipCelpStateT *pModule = (VoipCelpStateT *)pState;
    int32_t iResult;
    float fFrameBuffer[VOIP_CODEC_CELP_MAXFRAMESAMPLES];
    uint8_t *pOutBuff = pOut;
    const int16_t *pInBuff = pInp;
    int32_t iNumPackets = iNumSamples / pModule->iFrameSamples;
    int32_t iPacket, iSample, iNumEncodedBytes = 0;
    #if DIRTYCODE_LOGGING
    int32_t iStartTick;
    #endif

    NetPrintfVerbose((_VoipCelp_iDebugLevel, 2, "voipcelpps3: encoding block at %p of %d samples\n", pInp, iNumSamples));

    if ((iNumSamples % pModule->iFrameSamples) != 0)
    {
        NetPrintf(("voipcelpps3: celp - celp8 encoder can only encode multiples of %d samples (%d submitted).\n", pModule->iFrameSamples, iNumSamples));
        return(0);
    }
    if (iNumPackets == 0)
    {
        NetPrintf(("voipcelpps3: celp8 encoder received no data to encode\n"));
        return(0);
    }

    // compress incoming samples into packet frames
    for (iPacket = 0; iPacket < iNumPackets; iPacket++)
    {
        // convert the frame to floats
        for (iSample = 0; iSample < pModule->iFrameSamples; iSample++)
        {
            fFrameBuffer[iSample] = pInBuff[iSample];
            fFrameBuffer[iSample] = fFrameBuffer[iSample] / 32767.0f;
        }

        // send the input data to the encoder
        PcmInfo.startAddr = fFrameBuffer;
        PcmInfo.size = pModule->iFrameSamples * sizeof(float);
        if ((iResult = cellCelp8EncEncodeFrame(pModule->hEncodeHandle, &PcmInfo)) < CELL_OK)
        {
            NetPrintf(("voipcelpps3: cellCelp8EncEncodeFrame failed (err=%s)\n", DirtyErrGetName(iResult)));
            return(0);
        }

        // verbose debug will determine how long the blocking call blocks
        #if DIRTYCODE_LOGGING
        iStartTick = NetTick();
        #endif

        // wait for the encoder to finish ** this is blocking **
        if ((iResult = cellCelp8EncWaitForOutput(pModule->hEncodeHandle)) < CELL_OK)
        {
            NetPrintf(("voipcelpps3: cellCelp8EncWaitForOutput failed (err=%s)\n", DirtyErrGetName(iResult)));
            return(0);
        }

        #if DIRTYCODE_LOGGING
        pModule->iNumMsBlocked += NetTickDiff(NetTick(), iStartTick);
        pModule->iNumBlockingCalls += 1;
        if (pModule->iNumBlockingCalls >= 100)
        {
            NetPrintfVerbose((_VoipCelp_iDebugLevel, 1, "voipcelpps3: WaitForOutput blocked for a total of %dms over %d packets (%2.2f ms/packet)\n",
                (float)pModule->iNumMsBlocked/(float)pModule->iNumBlockingCalls));
            pModule->iNumBlockingCalls = 0;
            pModule->iNumMsBlocked = 0;
        }
        #endif

        // get the output data
        if ((iResult = cellCelp8EncGetAu(pModule->hEncodeHandle, pOutBuff, &AuInfo)) < CELL_OK)
        {
            NetPrintf(("voipcelpps3: cellCelp8EncGetAu failed (err=%s)\n", DirtyErrGetName(iResult)));
            return(0);
        }

        // move the pointers
        pInBuff += pModule->iFrameSamples;
        pOutBuff += AuInfo.size;
        iNumEncodedBytes += AuInfo.size;
        NetPrintfVerbose((_VoipCelp_iDebugLevel, 3, "voipcelpps3: encoded a block of %d bytes and output %d bytes\n", pModule->iFrameSamples * sizeof(float), AuInfo.size));
    }

    return(iNumEncodedBytes);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelpDecodeBlock

    \Description
        Decode CELP-encoded input to 16-bit linear PCM samples, and accumulate in the given output buffer.

    \Input *pState      - pointer to decode state
    \Input *pOut        - pointer to output buffer
    \Input *pInp        - pointer to input buffer
    \Input iInputBytes  - size of input data
    \Input iChannel     - the decode channel for which we are decoding data

    \Output
        int32_t         - number of samples decoded

    \Version 10/25/2006 (tdills)
*/
/*************************************************************************************************F*/
static int32_t _VoipCelpDecodeBlock(VoipCodecRefT *pState, int32_t *pOut, const uint8_t *pInp, int32_t iInputBytes, int32_t iChannel)
{
    static uint8_t InputBuff[VOIP_CODEC_CELP_MAXAUSIZE*4] __attribute__((aligned(128)));
    static float OutputBuff[VOIP_CODEC_CELP_MAXFRAMESAMPLES] __attribute__((aligned(128)));

    VoipCelpStateT *pModule = (VoipCelpStateT*)pState;

    CellAdecAuInfo auInfo = { NULL, pModule->iAuSize, { CELL_ADEC_PTS_INVALID, CELL_ADEC_PTS_INVALID }, 0 };
    const CellAdecPcmItem *pcmItem;
    int32_t iNumSamples = 0;
    int32_t iResult;
    int32_t iSample;
    int32_t iNumFrames = iInputBytes / pModule->iAuSize;
    int32_t iFrame;
    int32_t iDecoder;
    const uint8_t *pInBuff = pInp;
    int32_t *pOutBuff = pOut;

    NetPrintfVerbose((_VoipCelp_iDebugLevel, 2, "voipcelpps3: decoding block at %p of %d bytes\n", pInp, iInputBytes));

    // make sure our statically allocated buffer is big enough
    if (iInputBytes > (int32_t)sizeof(InputBuff))
    {
        NetPrintf(("voipcelpps3: static aligned input buffer is too small for %d input bytes\n", iInputBytes));
        return(0);
    }
    if ((iInputBytes % pModule->iAuSize) != 0)
    {
        NetPrintf(("voipcelpps3: celp decoder can only decode multiples of %d bytes (%d submitted)\n", pModule->iAuSize, iInputBytes));
        return(0);
    }
    if (iNumFrames == 0)
    {
        NetPrintf(("voipcelpps3: no frames to decode\n"));
        return(0);
    }

    // find the decoder allocated for this channel
    iDecoder = pModule->pChannelDecoders[iChannel].iDecoder;

    for (iFrame=0; iFrame < iNumFrames; iFrame++)
    {
        int32_t iSamplesInFrame;

        // copy the input data to the aligned buffer
        ds_memcpy(InputBuff, pInBuff, pModule->iAuSize);

        // decode the data
        auInfo.startAddr = (void*)InputBuff;
        auInfo.userData = iChannel;
        NetPrintfVerbose((_VoipCelp_iDebugLevel, 3, "voipcelpps3: calling cellAdecDecodeAu handle=%p, startAddr=%p, size=%d\n",
            pModule->pDecoders[iChannel].DecoderHandle, auInfo.startAddr, auInfo.size));
        while ((iResult = cellAdecDecodeAu(pModule->pDecoders[iDecoder].DecoderHandle, &auInfo)) == CELL_ADEC_ERROR_BUSY)
        {
            NetPrintf(("voipcelpps3: cellAdecDecodeAu() returned busy - retrying\n"));
            if ((iResult = cellAdecGetPcm(pModule->pDecoders[iDecoder].DecoderHandle, pOut)) < CELL_OK)
            {
                NetPrintf(("voipcelpps3: cellAdecGetPcm failed (err=%s)\n", DirtyErrGetName(iResult)));
                return(0);
            }
        }
        if (iResult < CELL_OK)
        {
            NetPrintf(("voipcelpps3: cellAdecDecodeAu failed (err=%s)\n", DirtyErrGetName(iResult)));
            return(0);
        }

        // wait for Sony to call the callback
        pModule->pDecoders[iDecoder].bWaitForCb = TRUE;
        while (pModule->pDecoders[iDecoder].bWaitForCb)
        {
            sys_timer_usleep(500);
        }

        // get the size of the output data
        if ((iResult = cellAdecGetPcmItem(pModule->pDecoders[iDecoder].DecoderHandle, &pcmItem)) < CELL_OK)
        {
            NetPrintf(("voipcelpps3: cellAdecGetPcmItem failed (err=%s)\n", DirtyErrGetName(iResult)));
            return(0);
        }
        iSamplesInFrame = pcmItem->size / sizeof(float);
        iNumSamples += iSamplesInFrame;

        // get the decoded audio data
        if ((iResult = cellAdecGetPcm(pModule->pDecoders[iDecoder].DecoderHandle, OutputBuff)) < CELL_OK)
        {
            NetPrintf(("voipcelpps3: cellAdecGetPcm failed (err=%s)\n", DirtyErrGetName(iResult)));
            return(0);
        }

        // convert from floats to ints and accumulate into the output buffer
        for (iSample=0; iSample < iSamplesInFrame; iSample++)
        {
            pOutBuff[iSample] += (OutputBuff[iSample] * 32767.0f);
        }

        // move the pointers
        pInBuff += pModule->iAuSize;
        pOutBuff += iSamplesInFrame;
    }

    // return the number of samples decoded
    return(iNumSamples);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelpReset

    \Description
        Resets codec state.

    \Input *pState      - pointer to decode state

    \Version 10/25/2006 (tdills)
*/
/*************************************************************************************************F*/
static void _VoipCelpReset(VoipCodecRefT *pState)
{
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelpControl

    \Description
        Modifies parameters of the codec

    \Input *pState  - pointer to decode state
    \Input iControl - control selector
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t         - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'spam' - set verbose debug level (debug builds only)
            'sprs' - set SPURS info to open an encoder          (must be called before codec creation)
            'smpe' - select MPE version to use (8khz codec)     (must be called before codec creation)
            'srpe' - select RPE version to use (16khz codec)    (must be called before codec creation)
        \endverbatim

    \Version 10/06/2011 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _VoipCelpControl(VoipCodecRefT *pState, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    #if DIRTYCODE_LOGGING
    if (iControl == 'spam')
    {
        NetPrintf(("voipcelpps3: setting debuglevel=%d\n", iValue));
        _VoipCelp_iDebugLevel = iValue;
        return(0);
    }
    #endif
    if (iControl == 'sprs')
    {
        // clear spurs info
        if (pValue == NULL)
        {
            memset(&_VoipCelp_SPURSInfo, 0, sizeof(_VoipCelp_SPURSInfo));
        }
        // save given spurs info
        else if (iValue >= (signed)sizeof(_VoipCelp_SPURSInfo))
        {
            ds_memcpy(&_VoipCelp_SPURSInfo, pValue, sizeof(_VoipCelp_SPURSInfo));
        }
        else
        {
            NetPrintf(("voipcelpps3: invalid spurs info size, got %d - should be at least %d\n", iValue, sizeof(_VoipCelp_SPURSInfo)));
            return(-1);
        }
        NetPrintf(("voipcelpps3: setting spurs=0x%x, maxContention=%d, priority={%d,%d,%d,%d,%d,%d,%d,%d}\n", 
                   _VoipCelp_SPURSInfo.EncodeResEx.spurs,
                   _VoipCelp_SPURSInfo.EncodeResEx.maxContention,
                   _VoipCelp_SPURSInfo.EncodeResEx.priority[0], _VoipCelp_SPURSInfo.EncodeResEx.priority[1], 
                   _VoipCelp_SPURSInfo.EncodeResEx.priority[2], _VoipCelp_SPURSInfo.EncodeResEx.priority[3],
                   _VoipCelp_SPURSInfo.EncodeResEx.priority[4], _VoipCelp_SPURSInfo.EncodeResEx.priority[5], 
                   _VoipCelp_SPURSInfo.EncodeResEx.priority[6], _VoipCelp_SPURSInfo.EncodeResEx.priority[7]));
        return(0);
    }
    if (iControl == 'smpe')
    {
        _VoipCelp_iMPEConfig = _VoipCelpSetConfig(_VoipCelp_aMPEConfig, iValue, _VoipCelp_iMPEConfig, CELL_CELP8ENC_MPE_CONFIG_26, "MPE");
        return(0);
    }
    if (iControl == 'srpe')
    {
        _VoipCelp_iRPEConfig = _VoipCelpSetConfig(_VoipCelp_aRPEConfig, iValue, _VoipCelp_iRPEConfig, CELL_CELPENC_RPE_CONFIG_3, "RPE");
        return(0);
    }
    NetPrintf(("voipcelpps3: unrecognized control selector '%C'\n", iControl));
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function _VoipCelpStatus

    \Description
        Get codec status

    \Input *pCodecState - pointer to decode state
    \Input iSelect      - status selector
    \Input iValue       - selector-specific
    \Input *pBuffer     - [out] storage for selector output
    \Input iBufSize     - size of output buffer

    \Output
        int32_t         - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'fsiz' - size of encoder output / decoder input in bytes (iValue=samples per frame)
            'fsmp' - frame sample size for current codec
        \endverbatim

    \Version 10/11/2011 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _VoipCelpStatus(VoipCodecRefT *pCodecState, int32_t iSelect, int32_t iValue, void *pBuffer, int32_t iBufSize)
{
    VoipCelpStateT *pModuleState = (VoipCelpStateT *)pCodecState;

    // these options require module state
    if (pModuleState != NULL)
    {
        if (iSelect == 'fsmp')
        {
            return(pModuleState->iFrameSamples);
        }
        if (iSelect == 'fsiz')
        {
            return((iValue/pModuleState->iFrameSamples) * pModuleState->iAuSize);
        }
    }
    NetPrintf(("voipcelpps3: unhandled status selector '%C'\n", iSelect));
    return(-1);
}


/*H********************************************************************************/
/*!
    \File voipheadsetps4.c

    \Description
        VoIP headset manager.

    \Copyright
        Copyright (c) Electronic Arts 2013. ALL RIGHTS RESERVED.

    \Version 01/16/2013 (mclouatre) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include <scebase_common.h>
#include <voice.h>
#include <audioin.h>    // for SCE_AUDIO_IN_TYPE_VOICE
#include <audioout.h>   // for SCE_AUDIO_OUT_PORT_TYPE_VOICE
#include <user_service.h>

// dirtysock includes
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/netconn.h"

// voip includes
#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voippacket.h"
#include "voipheadset.h"

/*** Defines **********************************************************************/
#define VOIPHEADSET_INVALID_INDEX                       (-1)
#define VOIPHEADSET_DECODER_OWNERSHIP_AGING             (1 * 1000)  // user-to-decoder owner relationship broken after 1 sec of inactivity
#define VOIPHEADSET_DEBUG                               (DIRTYCODE_DEBUG && FALSE)
#define VOIPHEADSET_VOICELIB_DEFAULT_ENCODER_BITRATE    (SCE_VOICE_BITRATE_16000)


/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//!< decoder data space (used to wrap a lib voice IN_VOICE port)
typedef struct
{
    uint32_t uDecoderPortId;                         //!< id of the IN_VOICE port wrapped by this entry
    uint32_t uLastUsage;                             //!< tick for last usage of the IN_VOICE port by the owner
    uint32_t uConnectedSpkrPort;                     //!< keep tracks of which local user's speaker port is connected (1 user per bit)
    int32_t iOwnerIndex;                             //!< local user index or remote user space index  (discriminated with bOwnerRemote;  VOIPHEADSET_INVALID_INDEX if not used)
    uint8_t bOwnerRemote;                            //!< whether the owner is a remote user or a local user
    uint8_t bValid;                                  //!< whether the other fields in this structure are valid or not
    uint8_t _pad[2];

} PS4DecoderT;

//!< local user data space
typedef struct
{
    VoipUserT user;
    uint32_t uMicPortId;            //!< id of the IN_DEVICE port associated with this local user
    uint32_t uEncoderPortId;        //!< id of the OUT_VOICE port associated with this local user
    uint32_t uSpkrPortId;           //!< id of the OUT_DEVICE port associated with this local user 
    int32_t iDecoderIndex;          //!< index of decoder used in the pool of decoders  (VOIPHEADSET_INVALID_INDEX if no such association exists)
    uint8_t bParticipating;         //!< TRUE if this user is a participating user FALSE if not
    uint8_t _pad[3];
} PS4LocalVoipUserT;

//!< remote user data space
typedef struct
{
    VoipUserT user;
    int32_t iDecoderIndex;          //!< index of decoder used in the pool of decoders  (VOIPHEADSET_INVALID_INDEX if no such association exists)
    uint32_t uEnablePlayback;       //!< bit field use to disable or enable playback of voice for a specific remote user (1 user per bit)
} PS4RemoteVoipUserT;

//! voipheadset module state data
struct VoipHeadsetRefT
{
    // headset status
    uint32_t uLastHeadsetStatus;    //!< bit field capturing last reported headset status (on/off) for each local player

    //! channel info
    int32_t iMaxChannels;

    //! maximum number of remote users
    int32_t iMaxRemoteUsers;
   
    //! encoding config (bps)
    int32_t iEncoderBps;

    //! user callback data
    VoipHeadsetMicDataCbT *pMicDataCb;
    VoipHeadsetTextDataCbT *pTextDataCb;
    VoipHeadsetStatusCbT *pStatusCb;
    void *pCbUserData;

    //! speaker callback data
    VoipSpkrCallbackT *pSpkrDataCb;
    void *pSpkrCbUserData;

    //! pool of decoders
    PS4DecoderT *pDecoders;      //!< array of decoder entries
    int32_t iDecoderPoolSize;       //!< number of entries in the array

    #if DIRTYCODE_LOGGING
    int32_t iDebugLevel;            //!< module debuglevel
    #endif

    uint8_t uSendSeq;
    uint8_t bLoopback;

    //! tracks decoder connection updates
    uint8_t bDecoderConnUpdateNeeded;
    uint8_t bDecoderConnUpdateInProgress;

    //! track device chat privilege
    uint32_t uChatPrivileges;    //!< bit field to keep track of the chat privileges of all the users
    uint8_t bSharedPrivilege;

    //! mem pool and init struct required when calling SceVoiceStart()
    SceVoiceStartParam sceStartParam;
    uint8_t aMemPool[SCE_VOICE_MEMORY_CONTAINER_SIZE];

    PS4LocalVoipUserT aLocalUsers[VOIP_MAXLOCALUSERS];

    //! pointer to global array tracking most restrictive voip contributors
    uint8_t *pVoipMostRestrictiveVoipContributors;

    //! remote user list - must come last in ref as it is variable length
    PS4RemoteVoipUserT aRemoteUsers[1];
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetVoiceEventCb

    \Description
        Event handler registered with the Voice Library

    \Input *pEventData  - event data

    \Version 01/21/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetVoiceEventCb(SceVoiceEventData *pEventData)
{
    if (pEventData->eventType == SCE_VOICE_EVENT_AUDIOINPUT_CONTROL_STATUS)
    {
        NetPrintf(("voipheadsetps4: SCE_VOICE_EVENT_AUDIOINPUT_CONTROL_STATUS event\n"));
    }
    else if (pEventData->eventType == SCE_VOICE_EVENT_AUDIOINPUT_MUTE_STATUS)
    {
        NetPrintf(("voipheadsetps4: SCE_VOICE_EVENT_AUDIOINPUT_MUTE_STATUS event\n"));
    }
    else if (pEventData->eventType != SCE_VOICE_EVENT_DATA_READY)
    {
        NetPrintf(("voipheadsetps4: unknown event\n"));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetVoiceLibInit

    \Description
        Initialize the Sony Voice library.

    \Input *pHeadset    - module reference

    \Output
        int32_t         - negative=error, zero=success

    \Version 01/17/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetVoiceLibInit(VoipHeadsetRefT *pHeadset)
{
    SceVoiceInitParam sceVoiceInitParams;
    SceVoiceResourceInfo sceVoiceResourceInfo;
    int32_t iResult;
    int32_t iRetCode = 0; // default to success

    ds_memclr(&sceVoiceInitParams, sizeof(sceVoiceInitParams));
    sceVoiceInitParams.appType = SCE_VOICE_APPTYPE_GAME;
    sceVoiceInitParams.onEvent = _VoipHeadsetVoiceEventCb;
    sceVoiceInitParams.pUserData = pHeadset;

    if ((iResult = sceVoiceInit(&sceVoiceInitParams , SCE_VOICE_VERSION_100)) == SCE_OK)
    {
        NetPrintf(("voipheadsetps4: successfully initialized Sony Voice Lib\n", DirtyErrGetName(iResult)));
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceInit() failed (err=%s)\n", DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    if ((iResult = sceVoiceGetResourceInfo(&sceVoiceResourceInfo)) == SCE_OK)
    {
        pHeadset->iDecoderPoolSize = sceVoiceResourceInfo.maxInVoicePort;
        NetPrintf(("voipheadsetps4: max IN_VOICE ports supported by the Voice Lib =        %d\n", sceVoiceResourceInfo.maxInVoicePort));
        NetPrintf(("voipheadsetps4: max OUT_VOICE ports supported by the Voice Lib =       %d\n", sceVoiceResourceInfo.maxOutVoicePort));
        NetPrintf(("voipheadsetps4: max IN_DEVICE ports supported by the Voice Lib =       %d\n", sceVoiceResourceInfo.maxInDevicePort));
        NetPrintf(("voipheadsetps4: max OUT_DEVICE ports supported by the Voice Lib =      %d\n", sceVoiceResourceInfo.maxOutDevicePort));
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceGetResourceInfo() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetVoiceLibShutdown

    \Description
        Shut down the sony Voice library.

    \Output
        int32_t         - negative=error, zero=success

    \Version 01/17/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetVoiceLibShutdown(void)
{
    int32_t iResult;
    int32_t iRetCode = 0; // default to success

    if ((iResult = sceVoiceEnd()) == SCE_OK)
    {
        NetPrintf(("voipheadsetps4: successfully shut down Sony Voice Lib\n"));
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceEnd() failed (err=%s)\n", DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetVoiceSetThreadsParams

    \Description
        Sets the internal voice and audio device threads.

    \Input *pHeadset        - module reference
    \Input cpuAffinity       - CPU affinity
    \Input iSchedPriority   - thread priority

    \Output
        int32_t         - negative=error, zero=success

    \Version 11/01/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetVoiceSetThreadsParams(VoipHeadsetRefT *pHeadset, SceKernelCpumask cpuAffinity, int32_t iSchedPriority)
{
    SceVoiceThreadsParams sceVoiceThreadsParams;
    int32_t iResult;
    int32_t iRetCode = 0; // default to success

    sceVoiceThreadsParams.paramVersion = 1;
    sceVoiceThreadsParams.audioInThreadAffinityMask = cpuAffinity;
    sceVoiceThreadsParams.audioInThreadPriority = iSchedPriority;
    sceVoiceThreadsParams.audioOutThreadAffinityMask = cpuAffinity;
    sceVoiceThreadsParams.audioOutThreadPriority = iSchedPriority;
    sceVoiceThreadsParams.voiceThreadAffinityMask = cpuAffinity;
    sceVoiceThreadsParams.voiceThreadPriority = iSchedPriority;

    if ((iResult = sceVoiceSetThreadsParams(&sceVoiceThreadsParams)) == SCE_OK)
    {
        NetPrintf(("voipheadsetps4: successfully modified attributes of Sony voip threads - affinity is 0x%02x and priority is %d\n", cpuAffinity, iSchedPriority));
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceSetThreadsParams() failed (err=%s)\n", DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetVoiceLibStart

    \Description
        Start the Sony Voice library.

    \Input *pHeadset    - module reference

    \Output
        int32_t         - negative=error, zero=success

    \Version 01/17/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetVoiceLibStart(VoipHeadsetRefT *pHeadset)
{
    int32_t iResult;
    int32_t iRetCode = 0; // default to success

    pHeadset->sceStartParam.container = &pHeadset->aMemPool;
    pHeadset->sceStartParam.memSize = sizeof(pHeadset->aMemPool);

    if ((iResult = sceVoiceStart(&pHeadset->sceStartParam)) == SCE_OK)
    {
        NetPrintf(("voipheadsetps4: successfully started Sony Voice Lib\n"));
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceStart() failed (err=%s)\n", DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetVoiceLibStop

    \Description
        Stop the Sony Voice library.

    \Output
        int32_t         - negative=error, zero=success

    \Version 01/17/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetVoiceLibStop(void)
{
    int32_t iResult;
    int32_t iRetCode = 0; // default to success

    if ((iResult = sceVoiceStop()) == SCE_OK)
    {
        NetPrintf(("voipheadsetps4: successfully stopped Sony Voice Lib\n"));
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceStop() failed (err=%s)\n", DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}


/*F********************************************************************************/
/*!
    \Function _VoipHeadsetPortDelete

    \Description
        Delete port.

    \Input uPortId      - port id

    \Output
        int32_t         - negative=error, zero=success

    \Version 01/21/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetPortDelete(uint32_t uPortId)
{
    int32_t iResult;
    int32_t iRetCode = 0; // default to success

    if ((iResult = sceVoiceDeletePort(uPortId)) == SCE_OK)
    {
        NetPrintf(("voipheadsetps4: sceVoiceDeletePort(%d) succeeded\n", uPortId));
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceDeletePort(%d) failed (err=%s)\n", uPortId, DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}


/*F********************************************************************************/
/*!
    \Function _VoipHeadsetMicPortCreate

    \Description
        Create IN_DEVICE port used to acquire voice from microphone.

    \Input iUserIndex   - local user index
    \Input *pPortId     - [OUT] variable to be filled with port id upon success

    \Output
        int32_t         - negative=error, zero=success

    \Version 01/21/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetMicPortCreate(int32_t iUserIndex, uint32_t *pPortId)
{
    SceVoicePortParam sceVoicePortParam;
    int32_t iResult;
    int32_t iRetCode = 0; // default to success
    SceUserServiceUserId sceUserId = NetConnStatus('suid', iUserIndex, NULL, 0);

    sceVoicePortParam.portType = SCE_VOICE_PORTTYPE_IN_DEVICE;
    sceVoicePortParam.bMute = false;
    sceVoicePortParam.threshold = 0;    // not applicable for IN_DEVICE
    sceVoicePortParam.volume = 1.0f;
    sceVoicePortParam.device.userId = sceUserId;
    sceVoicePortParam.device.type = SCE_AUDIO_IN_TYPE_VOICE;
    sceVoicePortParam.device.index = 0; // index when a single user uses multiple virtual device

    if ((iResult = sceVoiceCreatePort(pPortId, &sceVoicePortParam)) == SCE_OK)
    {
        NetPrintf(("voipheadsetps4: created IN_DEVICE port %d for user index %d (user ID = %d/0x%08x)\n", *pPortId, iUserIndex, sceUserId, sceUserId));
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceCreatePort(IN_DEVICE, %d) failed (err=%s)\n", iUserIndex, DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSpkrPortCreate

    \Description
        Create OUT_DEVICE port used to submit voice to speakers.

    \Input iUserIndex   - local user index
    \Input *pPortId     - [OUT] variable to be filled with port id upon success

    \Output
        int32_t         - negative=error, zero=success

    \Version 01/21/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetSpkrPortCreate(int32_t iUserIndex, uint32_t *pPortId)
{
    SceVoicePortParam sceVoicePortParam;
    int32_t iResult;
    int32_t iRetCode = 0; // default to success
    SceUserServiceUserId sceUserId = NetConnStatus('suid', iUserIndex, NULL, 0);

    sceVoicePortParam.portType = SCE_VOICE_PORTTYPE_OUT_DEVICE;
    sceVoicePortParam.bMute = false;
    sceVoicePortParam.threshold = 0;    // not applicable for OUT port
    sceVoicePortParam.volume = 1.0f;
    sceVoicePortParam.device.userId = sceUserId;
    sceVoicePortParam.device.type = SCE_AUDIO_OUT_PORT_TYPE_VOICE;
    sceVoicePortParam.device.index = 0; // index when a single user uses multiple virtual device

    if ((iResult = sceVoiceCreatePort(pPortId, &sceVoicePortParam)) == SCE_OK)
    {
        NetPrintf(("voipheadsetps4: created OUT_DEVICE port %d for user index %d (user ID = %d/0x%08x)\n", *pPortId, iUserIndex, sceUserId, sceUserId));
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceCreatePort(OUT_DEVICE, %d) failed (err=%s)\n", iUserIndex, DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetEncoderPortCreate

    \Description
        Create OUT_VOICE port used to encode voice from local user.

    \Input iBitRate - selected bit rate
    \Input *pPortId - [OUT] variable to be filled with port id upon success

    \Output
        int32_t         - negative=error, zero=success

    \Version 01/21/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetEncoderPortCreate(int32_t iBitRate, uint32_t *pPortId)
{
    SceVoicePortParam sceVoicePortParam;
    int32_t iResult;
    int32_t iRetCode = 0; // default to success

    sceVoicePortParam.portType = SCE_VOICE_PORTTYPE_OUT_VOICE;
    sceVoicePortParam.bMute = false;
    sceVoicePortParam.threshold = 0; // not applicable for OUT port
    sceVoicePortParam.volume = 1.0f;
    sceVoicePortParam.voice.bitrate = iBitRate;

    if ((iResult = sceVoiceCreatePort(pPortId, &sceVoicePortParam)) == SCE_OK)
    {
        NetPrintf(("voipheadsetps4: created OUT_VOICE port %d\n", *pPortId));
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceCreatePort(OUT_VOICE) failed (err=%s)\n", DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetDecoderPortCreate

    \Description
        Create IN_VOICE port used to decode voice to be played back locally.

    \Input iBitRate     - selected bit rate
    \Input *pPortId     - [OUT] variable to be filled with port id upon success

    \Output
        int32_t         - negative=error, zero=success

    \Version 01/21/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetDecoderPortCreate(int32_t iBitRate, uint32_t *pPortId)
{
    SceVoicePortParam sceVoicePortParam;
    int32_t iResult;
    int32_t iRetCode = 0; // default to success

    sceVoicePortParam.portType = SCE_VOICE_PORTTYPE_IN_VOICE;
    sceVoicePortParam.bMute = false;
    sceVoicePortParam.threshold = 100; // compensate network jitter
    sceVoicePortParam.volume = 1.0f;
    sceVoicePortParam.voice.bitrate = iBitRate;

    if ((iResult = sceVoiceCreatePort(pPortId, &sceVoicePortParam)) == SCE_OK)
    {
        NetPrintf(("voipheadsetps4: created IN_VOICE port %d\n", *pPortId));
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceCreatePort(IN_VOICE) failed (err=%s)\n", DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetConnectPorts

    \Description
        Connect the specified input port to the specified output port.

    \Input uInPortId    - id of input port to be connected
    \Input uOutPortId   - id of output port to be connected

    \Output
        int32_t         - negative=error, zero=success

    \Version 01/21/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetConnectPorts(uint32_t uInPortId, uint32_t uOutPortId)
{
    int32_t iResult;
    int32_t iRetCode = 0; // default to success

    if ((iResult = sceVoiceConnectIPortToOPort(uInPortId, uOutPortId)) == SCE_OK)
    {
        NetPrintf(("voipheadsetps4: successfully connected IN port %d to OUT port %d\n", uInPortId, uOutPortId));
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceCConnectIPortToOport(%d, %d) failed (err=%s)\n", uInPortId, uOutPortId, DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetDisconnectPorts

    \Description
        Disconnect the specified input port from the specified output port.

    \Input uInPortId  - id of input port to be disconnected
    \Input uOutPortId - id of output port to be disconnected

    \Output
        int32_t         - negative=error, zero=success

    \Version 01/21/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetDisconnectPorts(uint32_t uInPortId, uint32_t uOutPortId)
{
    int32_t iResult;
    int32_t iRetCode = 0; // default to success

    if ((iResult = sceVoiceDisconnectIPortFromOPort(uInPortId, uOutPortId)) == SCE_OK)
    {
        NetPrintf(("voipheadsetps4: successfully disconnected IN port %d from OUT port %d\n", uInPortId, uOutPortId));
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceDisconnectIPortFromOport(%d, %d) failed (err=%s)\n", uInPortId, uOutPortId, DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetWriteToPort

    \Description
        Write to IN_VOICE port.

    \Input uInPortId    - id of input port to be written to
    \Input *pFrame      - pointer to encoded voice frame
    \Input uFrameSize   - frame size (in bytes)

    \Output
        int32_t         - negative=error, zero=success

    \Version 01/21/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetWriteToPort(uint32_t uInPortId, const uint8_t *pFrame, uint32_t uFrameSize)
{
    int32_t iResult;
    int32_t iRetCode = 0; // default to success
    uint32_t uBytesInOut = uFrameSize;

    if ((iResult = sceVoiceWriteToIPort(uInPortId, (void *)pFrame, &uBytesInOut, 0)) == SCE_OK)
    {
        if (uBytesInOut != uFrameSize)
        {
            NetPrintf(("voipheadsetps4: sceVoiceWriteToIPort(%d) did not consume the entire frame (%d vs %d)\n", uInPortId, uFrameSize, uBytesInOut));
            iRetCode = -1;  // signal failure
        }
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceWriteToIPort(%d, %d) failed (err=%s)\n", uInPortId, uFrameSize, DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetReadFromPort

    \Description
        Read from OUT_VOICE port.

    \Input uOutPortId   - id of output port to be read from
    \Input *pFrame      - pointer to buffer to be filled with encoded voice frame
    \Input *pFrameSize  - pointer to be filled with frame size (number of bytes read)

    \Output
        int32_t         - negative=error, zero=success

    \Version 01/21/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetReadFromPort(uint32_t uOutPortId, const uint8_t *pFrame, uint32_t *pFrameSize)
{
    int32_t iResult;
    int32_t iRetCode = 0; // default to success

    if ((iResult = sceVoiceReadFromOPort(uOutPortId, (void *)pFrame, pFrameSize)) != SCE_OK)
    {
        NetPrintf(("voipheadsetps4: sceVoiceReadFromOPort(%d) failed (err=%s)\n", uOutPortId,  DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetIsLocalVoiceFrameReady

    \Description
        Read from OUT_VOICE port.

    \Input uOutPortId   - id of output port to be read from

    \Output
        int32_t         - frame size if frame is ready; 0 if frame is not ready; negative if error

    \Version 01/21/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetIsLocalVoiceFrameReady(uint32_t uOutPortId)
{
    int32_t iResult;
    int32_t iRetCode = 0; // default to "frame not ready"
    SceVoiceBasePortInfo sceVoiceBasePortInfo;
    sceVoiceBasePortInfo.pEdge = NULL;  // to avoid time-consuming process within voice lib (port's connection info)

    if ((iResult = sceVoiceGetPortInfo(uOutPortId, &sceVoiceBasePortInfo)) == SCE_OK)
    {
        #if DIRTYCODE_LOGGING
        if (sceVoiceBasePortInfo.portType != SCE_VOICE_PORTTYPE_OUT_VOICE)
        {
            NetPrintf(("voipheadsetps4: sceVoiceGetPortInfo(%d) called on port of wrong type (%d)\n", uOutPortId, sceVoiceBasePortInfo.portType));
            return(-1);
        }
        #endif

        if (sceVoiceBasePortInfo.numByte >= sceVoiceBasePortInfo.frameSize)
        {
            iRetCode = sceVoiceBasePortInfo.frameSize;
        }
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceGetPortInfo(%d) failed (err=%s)\n", uOutPortId, DirtyErrGetName(iResult)));
        iRetCode = -2;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetIsHeadsetDetected

    \Description
        Check if headset is connected.

    \Input uInPortId            - id of input port to be checked for headset
    \Input *pHeadsetDetected    - [OUT] pointer to boolean var to be filled with headset status

    \Output
        uint8_t                 - 0 for success, negative for error

    \Version 08/26/2013 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t _VoipHeadsetIsHeadsetDetected(uint32_t uInPortId, uint8_t *pHeadsetDetected)
{
    int32_t iResult;
    int32_t iRetCode = 0; // default to "success"
    bool bIsUsable;

    if ((iResult = sceVoiceGetPortAttr(uInPortId, SCE_VOICE_ATTR_AUDIOINPUT_USABLE, &bIsUsable, sizeof(bIsUsable))) == SCE_OK)
    {
        *pHeadsetDetected = (bIsUsable ? TRUE : FALSE);
    }
    else
    {
        NetPrintf(("voipheadsetps4: sceVoiceGetPortAttr(%d, SCE_VOICE_ATTR_AUDIOINPUT_USABLE) failed (err=%s)\n", uInPortId, DirtyErrGetName(iResult)));
        iRetCode = -1;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUpdateDecoderConnections

    \Description
        Make sure all decoders are connected to all local user speaker ports

    \Input *pHeadset        - pointer to headset state

    \Version 04/29/2014 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetUpdateDecoderConnections(VoipHeadsetRefT *pHeadset)
{
    int32_t iLocalUserIndex;
    int32_t iDecoderIndex;

    for (iDecoderIndex = 0; iDecoderIndex < pHeadset->iDecoderPoolSize; iDecoderIndex++)
    {
        if (pHeadset->pDecoders[iDecoderIndex].bValid && (pHeadset->pDecoders[iDecoderIndex].iOwnerIndex != VOIPHEADSET_INVALID_INDEX))
        {
            for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; iLocalUserIndex++)
            {
                if (!VOIP_NullUser(&pHeadset->aLocalUsers[iLocalUserIndex].user) && (pHeadset->aLocalUsers[iLocalUserIndex].bParticipating == TRUE))
                {
                    int32_t iResult;

                    // case for normal voip between remote user
                    if (pHeadset->pDecoders[iDecoderIndex].bOwnerRemote == TRUE)
                    {
                        uint8_t bPrivilege;
                        int32_t iPlaybackFlagIndex;
                        int32_t iOutput = VoipHeadsetStatus(pHeadset, 'hpao', iLocalUserIndex, NULL, 0);

                        /*
                        We allow the decoder to connect to the speaker port when the audio output device is unknown because according to sony ticket 
                        https://ps4.scedev.net/support/issue/92213/_sceVoiceGetPortAttr_SCE_VOICE_ATTR_AUDIOOUTPUT_DESTINATION_returns_an_Output_of_0_when_I_have_restricted_(underage_sub_account)_login#n1262954
                        the audio output attribute will only be valid if data is first written to the output port.
                        */
                        if ((iOutput & SCE_AUDIO_OUT_STATE_OUTPUT_CONNECTED_HEADPHONE) || (iOutput == SCE_AUDIO_OUT_STATE_OUTPUT_UNKNOWN))
                        {
                            iPlaybackFlagIndex = iLocalUserIndex;
                        }
                        else
                        {
                            iPlaybackFlagIndex = VOIP_SHARED_USER_INDEX;
                        }

                        /*
                        Unconditionally (i.e. ignore headsets or TV/cam_mic being used) apply most restrictive voip settings to comply with Sony TR 4061 when there are
                        multiple local users interacting with a game (note: that's different from EA MLU which implies multiple users authenticated with Blaze).
                        The TR itself is not super clear aobut this, but we got some clear guidance from Sony about this here:
                        https://ps4.scedev.net/support/issue/131717/_Need_help_with_interpretation_of_TR_4061. The argument is that there is no reliable way to make
                        sure that the restricted user is not hearing the voice even if headsets are detected because what is detected as a headset connected to a
                        controller can rather be a speaker.
                        */
                        bPrivilege = pHeadset->bSharedPrivilege;

                        // if connected, disconnect if necessary
                        if (pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort & (1 << iLocalUserIndex))
                        {
                            // proceed with disconnect if voip privileges are restrictive or if playback is disabled for the user
                            if (!bPrivilege || !(pHeadset->aRemoteUsers[pHeadset->pDecoders[iDecoderIndex].iOwnerIndex].uEnablePlayback & (1 << iPlaybackFlagIndex)))
                            {
                                if (_VoipHeadsetDisconnectPorts(pHeadset->pDecoders[iDecoderIndex].uDecoderPortId, pHeadset->aLocalUsers[iLocalUserIndex].uSpkrPortId) == 0)
                                {
                                    pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort &= ~(1 << iLocalUserIndex);

                                    if (pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort == 0)
                                    {
                                        if ((iResult = sceVoiceResetPort(pHeadset->pDecoders[iDecoderIndex].uDecoderPortId)) != SCE_OK)
                                        {
                                            NetPrintf(("voipheadsetps4: failed to reset decoder port %d (err=%s)\n", pHeadset->pDecoders[iDecoderIndex].uDecoderPortId, DirtyErrGetName(iResult)));
                                        }
                                    }
                                }
                            }
                        }
                        else // if not connected, connect if necessary
                        {
                            // only proceed with connect if voip privileges allow it and if playback is enabled for the user
                            if (bPrivilege && pHeadset->aRemoteUsers[pHeadset->pDecoders[iDecoderIndex].iOwnerIndex].uEnablePlayback & (1 << iPlaybackFlagIndex))
                            {
                                if (_VoipHeadsetConnectPorts(pHeadset->pDecoders[iDecoderIndex].uDecoderPortId, pHeadset->aLocalUsers[iLocalUserIndex].uSpkrPortId) == 0)
                                {
                                    pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort |= (1 << iLocalUserIndex);
                                }
                            }
                        }
                    }
                    // case for loopback functionality
                    else
                    {
                        // connect decoder to its own speaker port for loopback
                        if ((pHeadset->pDecoders[iDecoderIndex].iOwnerIndex == iLocalUserIndex) && !(pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort & (1 << iLocalUserIndex)))
                        {
                            if (_VoipHeadsetConnectPorts(pHeadset->pDecoders[iDecoderIndex].uDecoderPortId, pHeadset->aLocalUsers[iLocalUserIndex].uSpkrPortId) == 0)
                            {
                                pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort |= (1 << iLocalUserIndex);
                            }
                        }
                        // disconnect decoder from other local speaker port not own by the current local user
                        else if ((pHeadset->pDecoders[iDecoderIndex].iOwnerIndex != iLocalUserIndex) && (pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort & (1 << iLocalUserIndex)))
                        {
                            if (_VoipHeadsetDisconnectPorts(pHeadset->pDecoders[iDecoderIndex].uDecoderPortId, pHeadset->aLocalUsers[iLocalUserIndex].uSpkrPortId) == 0)
                            {
                                pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort &= ~(1 << iLocalUserIndex);

                                if (pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort == 0)
                                {
                                    if ((iResult = sceVoiceResetPort(pHeadset->pDecoders[iDecoderIndex].uDecoderPortId)) != SCE_OK)
                                    {
                                        NetPrintf(("voipheadsetps4: failed to reset decoder port %d (err=%s)\n", pHeadset->pDecoders[iDecoderIndex].uDecoderPortId, DirtyErrGetName(iResult)));
                                    }
                                }
                            }
                        }
                    } // if owner is remote
                } // if local user is valid
            } //for all local user
        } // if decoder is valid
    } // for all decoder
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetCreateDecoder

    \Description
        Create a PS4DecoderT entry in the pool of decoders.

    \Input *pHeadset    - pointer to headset state

    \Output
        int32_t         - negative=error, zero=success

    \Version 09/18/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetCreateDecoder(VoipHeadsetRefT *pHeadset)
{
    int32_t iRetCode = 0; // default to success
    int32_t iDecoderIndex;

    // check if there is a free spot in the pool of decoders
    for (iDecoderIndex = 0; iDecoderIndex < pHeadset->iDecoderPoolSize; iDecoderIndex++)
    {
        if (!pHeadset->pDecoders[iDecoderIndex].bValid)
        {
            break;
        }
    }

    // did we find a free spot?
    if (iDecoderIndex < pHeadset->iDecoderPoolSize)
    {
        int32_t iResult;

        /*
        free spot found!
        */

        if ((iResult = _VoipHeadsetDecoderPortCreate(pHeadset->iEncoderBps, &pHeadset->pDecoders[iDecoderIndex].uDecoderPortId)) == 0)
        {
            NetPrintf(("voipheadsetps4: added IN_VOICE port (%d) at index %d in pool of decoders\n",
                pHeadset->pDecoders[iDecoderIndex].uDecoderPortId, iDecoderIndex));

            pHeadset->pDecoders[iDecoderIndex].bValid = TRUE;
            pHeadset->pDecoders[iDecoderIndex].iOwnerIndex = VOIPHEADSET_INVALID_INDEX;
            pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort = 0;
        }
        else
        {
            NetPrintf(("voipheadsetps4: failed to create a new decoder port)\n"));
            iRetCode = -1;  // signal failure
        }
    }
    else
    {
        NetPrintf(("voipheadsetps4: IN_VOICE port creation ignored because pool of decoders is already full\n"));
        iRetCode = -2;  // signal failure
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetReleaseDecoderOwnership

    \Description
        Break the ownership association between the specified PS4DecoderT entry
        and its owner.

    \Input *pHeadset        - pointer to headset state
    \Input iDecoderIndex    - index of decoder entry in pool of decoders

    \Output
        int32_t             - negative=error, zero=success

    \Version 09/18/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetReleaseDecoderOwnership(VoipHeadsetRefT *pHeadset, int32_t iDecoderIndex)
{
    int32_t iRetCode = -1;  // default to failure
    PS4DecoderT *pDecoder = &pHeadset->pDecoders[iDecoderIndex];

    if (pDecoder->bValid)
    {
        if (pDecoder->iOwnerIndex != VOIPHEADSET_INVALID_INDEX)
        {
            if (pDecoder->bOwnerRemote)
            {
                NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetps4: remote user %d released ownership of decoder entry %d\n", pDecoder->iOwnerIndex, iDecoderIndex));
                pHeadset->aRemoteUsers[pDecoder->iOwnerIndex].iDecoderIndex = VOIPHEADSET_INVALID_INDEX;
            }
            else
            {
                NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetps4: local user %d released ownership of decoder entry %d\n", pDecoder->iOwnerIndex, iDecoderIndex));
                pHeadset->aLocalUsers[pDecoder->iOwnerIndex].iDecoderIndex = VOIPHEADSET_INVALID_INDEX;
            }

            pDecoder->uLastUsage = 0;
            pDecoder->iOwnerIndex = VOIPHEADSET_INVALID_INDEX;
        }
        else
        {
            NetPrintf(("voipheadsetps4: can't release ownership on a decoder entry with no owner (index = %d)\n", iDecoderIndex));
        }
    }
    else
    {
        NetPrintf(("voipheadsetps4: can't release ownership on an invalid decoder entry (index = %d)\n", iDecoderIndex));
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetTakeDecoderOwnership

    \Description
        Create the ownership association between the specified PS4DecoderT entry
        and a user.

    \Input *pHeadset        - pointer to headset state
    \Input iDecoderIndex    - index of decoder entry in pool of decoders
    \Input iUserIndex       - local user index or remote user space index  (discriminated with bRemote)
    \Input bRemote          - whether the owner is a remote user or a local user

    \Output
        int32_t             - negative=error, zero=success

    \Version 09/18/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetTakeDecoderOwnership(VoipHeadsetRefT *pHeadset, int32_t iDecoderIndex, int32_t iUserIndex, uint8_t bRemote)
{
    int32_t iRetCode = -1;  // default to failure
    PS4DecoderT *pDecoder = &pHeadset->pDecoders[iDecoderIndex];

    if (pDecoder->bValid)
    {
        if (pDecoder->iOwnerIndex == VOIPHEADSET_INVALID_INDEX)
        {
            pDecoder->iOwnerIndex = iUserIndex;

            if (bRemote)
            {
                NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetps4: remote user %d took ownership of decoder entry %d\n", iUserIndex, iDecoderIndex));
                pHeadset->aRemoteUsers[iUserIndex].iDecoderIndex = iDecoderIndex;
            }
            else
            {
                NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetps4: local user %d took ownership of decoder entry %d\n", iUserIndex, iDecoderIndex));
                pHeadset->aLocalUsers[iUserIndex].iDecoderIndex = iDecoderIndex;
            }

            
            pDecoder->bOwnerRemote = bRemote;

            // refresh decoder connections
            pHeadset->bDecoderConnUpdateNeeded = TRUE;
        }
        else
        {
            NetPrintf(("voipheadsetps4: can't take ownership on a decoder entry that already has an owner (index = %d)\n", iDecoderIndex));
        }
    }
    else
    {
        NetPrintf(("voipheadsetps4: can't take ownership on an invalid decoder entry (index = %d)\n", iDecoderIndex));
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetFindFreeDecoder

    \Description
        Identify a free decoder.

    \Input *pHeadset        - pointer to headset state

    \Output
        int32_t             - VOIPHEADSET_INVALID_INDEX for no decoder found; decoder index otherwise

    \Version 09/17/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetFindFreeDecoder(VoipHeadsetRefT *pHeadset)
{
    int32_t iDecoderIndex;
    int32_t iFoundDecoderIndex = VOIPHEADSET_INVALID_INDEX;  // default to failure

    for (iDecoderIndex = 0; iDecoderIndex < pHeadset->iDecoderPoolSize; iDecoderIndex++)
    {
        if (pHeadset->pDecoders[iDecoderIndex].bValid)
        {
            if (pHeadset->pDecoders[iDecoderIndex].iOwnerIndex == VOIPHEADSET_INVALID_INDEX)
            {
                iFoundDecoderIndex = iDecoderIndex;
                break;
            }
        }
    }

    return(iFoundDecoderIndex);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUpdateDecodersOwnership

    \Description
        Check if the decoders owner are still active. If not, break the
        ownership association.

    \Input *pHeadset        - pointer to headset state

    \Version 09/17/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetUpdateDecodersOwnership(VoipHeadsetRefT *pHeadset)
{
    int32_t iDecoderIndex;

    for (iDecoderIndex = 0; iDecoderIndex < pHeadset->iDecoderPoolSize; iDecoderIndex++)
    {
        if (pHeadset->pDecoders[iDecoderIndex].bValid)
        {
            if (pHeadset->pDecoders[iDecoderIndex].iOwnerIndex != VOIPHEADSET_INVALID_INDEX)
            {
                if (NetTickDiff(NetTick(), pHeadset->pDecoders[iDecoderIndex].uLastUsage) > VOIPHEADSET_DECODER_OWNERSHIP_AGING)
                {
                    _VoipHeadsetReleaseDecoderOwnership(pHeadset, iDecoderIndex);
                }
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUpdateStatus

    \Description
        Headset process function.

    \Input *pHeadset    - pointer to headset state

    \Version 01/21/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetUpdateStatus(VoipHeadsetRefT *pHeadset)
{
    int32_t iLocalUserIndex;

    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; iLocalUserIndex++)
    {
        if (!VOIP_NullUser(&pHeadset->aLocalUsers[iLocalUserIndex].user))
        {
            int32_t iResult;
            uint8_t bHeadsetDetected = FALSE;

            if ((iResult = _VoipHeadsetIsHeadsetDetected(pHeadset->aLocalUsers[iLocalUserIndex].uMicPortId, &bHeadsetDetected)) == 0)
            {
                uint32_t uUserHeadsetStatus = (bHeadsetDetected ? 1 : 0) << iLocalUserIndex;
                uint32_t uLastUserHeadsetStatus = pHeadset->uLastHeadsetStatus & (1 << iLocalUserIndex);

                if (uUserHeadsetStatus != uLastUserHeadsetStatus)
                {
                    NetPrintf(("voipheadsetps4: headset %s for local user index %d\n", (bHeadsetDetected ? "inserted" : "removed"), iLocalUserIndex));

                    // trigger callback
                    pHeadset->pStatusCb(iLocalUserIndex, bHeadsetDetected, VOIP_HEADSET_STATUS_INOUT, pHeadset->pCbUserData);

                    // update last headset status
                    pHeadset->uLastHeadsetStatus &= ~(1 << iLocalUserIndex);  // clear bit first
                    pHeadset->uLastHeadsetStatus |= uUserHeadsetStatus;       // now set bit status if applicable
                }
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetDeleteLocalTalker

    \Description
        Disconnect the "IN_DEVICE" port from the "OUT_VOICE" port associated with the
        local talker. Then delete both ports.

        Also disconnect all decoder ports from the speaker port of this local user.

    \Input *pHeadset        - headset module
    \Input iLocalUserIndex  - local user index
    \Input *pLocalUser      - user identifier (optional)

    \Output
        int32_t             - negative=error, zero=success

    \Version 01/22/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetDeleteLocalTalker(VoipHeadsetRefT *pHeadset, int32_t iLocalUserIndex, VoipUserT *pLocalUser)
{
    int32_t iRetCode = 0; // default to success

    if (pLocalUser)
    {
        if (pHeadset->aLocalUsers[iLocalUserIndex].bParticipating != FALSE)
        {
            NetPrintf(("voipheadsetps4: cannot delete local talker %s at local user index %d because the user must first be deactivated\n", pHeadset->aLocalUsers[iLocalUserIndex].user.strUserId, iLocalUserIndex));
            return(-1);
        }

        if (!VOIP_SameUser(pLocalUser, &pHeadset->aLocalUsers[iLocalUserIndex].user))
        {
            NetPrintf(("voipheadsetps4: deleting local talker %s failed because it does not match user at index %d (%s)\n",
                pLocalUser->strUserId, iLocalUserIndex, pHeadset->aLocalUsers[iLocalUserIndex].user.strUserId));
            return(-1);
        }
    }

    NetPrintf(("voipheadsetps4: deleting local talker %s at local user index %d\n", pHeadset->aLocalUsers[iLocalUserIndex].user.strUserId, iLocalUserIndex));

    if (_VoipHeadsetDisconnectPorts(pHeadset->aLocalUsers[iLocalUserIndex].uMicPortId, pHeadset->aLocalUsers[iLocalUserIndex].uEncoderPortId) < 0)
    {
        iRetCode = -1; // signal failure
    }

    if (_VoipHeadsetPortDelete(pHeadset->aLocalUsers[iLocalUserIndex].uEncoderPortId) < 0)
    {
        iRetCode = -1; // signal failure
    }

    if (_VoipHeadsetPortDelete(pHeadset->aLocalUsers[iLocalUserIndex].uMicPortId) < 0)
    {
        iRetCode = -2; // signal failure
    }

    // clear last headset status
    pHeadset->uLastHeadsetStatus &= ~(1 << iLocalUserIndex);
    VOIP_ClearUser(&pHeadset->aLocalUsers[iLocalUserIndex].user);

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadSetAddLocalUser

    \Description
        Create a Mic Port for User (needed for headsetstatus)

    \Input *pHeadset        - headset module
    \Input iLocalUserIndex  - local user index
    \Input *pLocalUser      - local user

    \Output
        int32_t         - negative=error, zero=success

    \Version 05/06/2014 (tcho)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetAddLocalTalker(VoipHeadsetRefT *pHeadset, int32_t iLocalUserIndex, VoipUserT *pLocalUser)
{
    int32_t iRetCode = -1;

    if (VOIP_NullUser(pLocalUser))
    {
        NetPrintf(("voipheadsetps4: can't add NULL local voip user at local user index %d\n", pLocalUser->strUserId, iLocalUserIndex));
        return(iRetCode);
    }

    VOIP_CopyUser(&pHeadset->aLocalUsers[iLocalUserIndex].user, pLocalUser);

    NetPrintf(("voipheadsetps4: adding voip local user %s at local user index %d\n", pLocalUser->strUserId, iLocalUserIndex));

    pHeadset->aLocalUsers[iLocalUserIndex].iDecoderIndex = VOIPHEADSET_INVALID_INDEX;

    if (_VoipHeadsetMicPortCreate(iLocalUserIndex, &pHeadset->aLocalUsers[iLocalUserIndex].uMicPortId) == 0)
    {
        iRetCode = 0; // signal success
    }

    if (_VoipHeadsetEncoderPortCreate(pHeadset->iEncoderBps, &pHeadset->aLocalUsers[iLocalUserIndex].uEncoderPortId) == 0)
    {
        if (_VoipHeadsetConnectPorts(pHeadset->aLocalUsers[iLocalUserIndex].uMicPortId, pHeadset->aLocalUsers[iLocalUserIndex].uEncoderPortId) < 0)
        {
            NetPrintf(("voipheadsetps4: cannot create an encoder port for user %i\n", iLocalUserIndex));
        }
    }

    if (iRetCode < 0)
    {
        _VoipHeadsetDeleteLocalTalker(pHeadset, iLocalUserIndex, pLocalUser);
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetDeactivateLocalTalker

    \Description
        Disconnect the "IN_DEVICE" port from the "OUT_VOICE" port associated with the
        local talker. Then delete both ports.

        Also disconnect all decoder ports from the speaker port of this local user.

    \Input *pHeadset        - headset module
    \Input iLocalUserIndex  - local index
    \Input *pLocalUser      - local user

    \Output
        int32_t             - negative=error, zero=success

    \Version 01/22/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetDeactivateLocalTalker(VoipHeadsetRefT *pHeadset, int32_t iLocalUserIndex, VoipUserT *pLocalUser)
{
    int32_t iRetCode = 0; // default to success
    int32_t iDecoderIndex;

    if (pHeadset->aLocalUsers[iLocalUserIndex].bParticipating != FALSE)
    {
        pHeadset->aLocalUsers[iLocalUserIndex].bParticipating = FALSE;

        // release decoder  (the one being fed back into during loopback mode)
        if (pHeadset->aLocalUsers[iLocalUserIndex].iDecoderIndex != VOIPHEADSET_INVALID_INDEX)
        {
            _VoipHeadsetReleaseDecoderOwnership(pHeadset, pHeadset->aLocalUsers[iLocalUserIndex].iDecoderIndex);
        }

        // disconnect all decoder ports from the speaker port of this local user
        for (iDecoderIndex = 0; iDecoderIndex < pHeadset->iDecoderPoolSize; iDecoderIndex++)
        {
            if ((pHeadset->pDecoders[iDecoderIndex].bValid) && (pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort & (1 << iLocalUserIndex)))
            {
                if (_VoipHeadsetDisconnectPorts(pHeadset->pDecoders[iDecoderIndex].uDecoderPortId, pHeadset->aLocalUsers[iLocalUserIndex].uSpkrPortId) == 0)
                {
                    pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort &= ~(1 << iLocalUserIndex);

                    if (pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort == 0)
                    {
                        int32_t iResult;

                        if ((iResult = sceVoiceResetPort(pHeadset->pDecoders[iDecoderIndex].uDecoderPortId)) != SCE_OK)
                        {
                            NetPrintf(("voipheadsetps4: failed to reset decoder port %d (err=%s)\n", pHeadset->pDecoders[iDecoderIndex].uDecoderPortId, DirtyErrGetName(iResult)));
                        }
                    }
                }
                else
                {
                    iRetCode = -1; // signal failure
                }
            }
        }

        if (_VoipHeadsetPortDelete(pHeadset->aLocalUsers[iLocalUserIndex].uSpkrPortId) < 0)
        {
            iRetCode = -1; // signal failure
        }

        #if DIRTYCODE_LOGGING
        if (iRetCode == 0)
        {
            NetPrintf(("voipheadsetps4: local user %s at local user index %d is no longer participating\n",
                pHeadset->aLocalUsers[iLocalUserIndex].user.strUserId, iLocalUserIndex));
        }
        #endif
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetActivateLocalTalker

    \Description
        Prepare a user for full voip functionality

    \Input *pHeadset        - headset module
    \Input iLocalUserIndex  - local user index
    \Input *pLocalUser      - local user

    \Output
        int32_t         - negative=error, zero=success

    \Version 05/06/2014 (tcho)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetActivateLocalTalker(VoipHeadsetRefT *pHeadset, int32_t iLocalUserIndex, VoipUserT *pLocalUser)
{
    int32_t iRetCode = -1;

    if (VOIP_NullUser(pLocalUser))
    {
        NetPrintf(("voipheadsetps4: cannot promote a null user to a participating state (local user index %d)\n", iLocalUserIndex));
        return(iRetCode);
    }

    if (pHeadset->aLocalUsers[iLocalUserIndex].bParticipating == FALSE)
    {
        if (_VoipHeadsetSpkrPortCreate(iLocalUserIndex, &pHeadset->aLocalUsers[iLocalUserIndex].uSpkrPortId) == 0)
        {
            pHeadset->aLocalUsers[iLocalUserIndex].bParticipating = TRUE;

            // attempt to create an additional decoder in the pool (if there is still room for it)
            _VoipHeadsetCreateDecoder(pHeadset);

            // make sure all decoders are connected to all speaker ports of all participating users
            pHeadset->bDecoderConnUpdateNeeded = TRUE;

            iRetCode = 0; // signal success

            NetPrintf(("voipheadsetps4: local user %s at local user index %d is now participating\n",
                pHeadset->aLocalUsers[iLocalUserIndex].user.strUserId, iLocalUserIndex));
        }
        else
        {
            _VoipHeadsetDeactivateLocalTalker(pHeadset, iLocalUserIndex, pLocalUser);
        }
    }
    else
    {
        NetPrintf(("voipheadsetps4: local user %s at local user index %d is already participating\n",
            pHeadset->aLocalUsers[iLocalUserIndex].user.strUserId, iLocalUserIndex));
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetPollForVoiceData

    \Description
        Poll to see if voice data is available.

    \Input *pHeadset    - pointer to headset state

    \Version 01/21/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipHeadsetPollForVoiceData(VoipHeadsetRefT *pHeadset)
{
    uint8_t bCameraMic = FALSE;
    uint8_t aBuffer[128];
    uint32_t iFrameSize;
    int32_t iLocalUserIndex;

    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; iLocalUserIndex++)
    {
        if (!VOIP_NullUser(&pHeadset->aLocalUsers[iLocalUserIndex].user) && (pHeadset->aLocalUsers[iLocalUserIndex].bParticipating ==TRUE))
        {
            while ((iFrameSize = _VoipHeadsetIsLocalVoiceFrameReady(pHeadset->aLocalUsers[iLocalUserIndex].uEncoderPortId)) > 0)
            {
                if (iFrameSize < sizeof(aBuffer))
                {
                    if (_VoipHeadsetReadFromPort(pHeadset->aLocalUsers[iLocalUserIndex].uEncoderPortId, aBuffer, (uint32_t *)&iFrameSize) == 0)
                    {
                        if (pHeadset->bLoopback == FALSE)
                        {
                            int32_t iUserIndex;

                            if ((bCameraMic = VoipHeadsetStatus(pHeadset, 'camr', 0, NULL, 0)) == TRUE)
                            { 
                                iUserIndex = VOIP_SHARED_USER_INDEX;
                            }
                            else
                            {
                                iUserIndex = iLocalUserIndex;
                            }

                            /*
                            Unconditionally (i.e. ignore headsets or TV/cam_mic being used) apply most restrictive voip settings to comply with Sony TR 4061 when there are
                            multiple local users interacting with a game (note: that's different from EA MLU which implies multiple users authenticated with Blaze).
                            The TR itself is not super clear aobut this, but we got some clear guidance from Sony about this here:
                            https://ps4.scedev.net/support/issue/131717/_Need_help_with_interpretation_of_TR_4061. The argument is that there is no reliable way to make
                            sure that the restricted user is not hearing the voice even if headsets are detected because what is detected as a headset connected to a
                            controller can rather be a speaker.
                            */
                            if (pHeadset->bSharedPrivilege)
                            {
                                // send to mic data callback
                                pHeadset->pMicDataCb(aBuffer, iFrameSize, NULL, 0, iUserIndex, pHeadset->uSendSeq++, pHeadset->pCbUserData);
                            }
                        }
                        else
                        {
                            int32_t iDecoderIndex = VOIPHEADSET_INVALID_INDEX;

                            _VoipHeadsetUpdateDecodersOwnership(pHeadset);

                            if (pHeadset->aLocalUsers[iLocalUserIndex].iDecoderIndex == VOIPHEADSET_INVALID_INDEX)
                            {
                                iDecoderIndex = _VoipHeadsetFindFreeDecoder(pHeadset);
                                if (iDecoderIndex != VOIPHEADSET_INVALID_INDEX)
                                {
                                    _VoipHeadsetTakeDecoderOwnership(pHeadset, iDecoderIndex, iLocalUserIndex, FALSE);
                                }
                            }
                            else
                            {
                                iDecoderIndex = pHeadset->aLocalUsers[iLocalUserIndex].iDecoderIndex;
                            }

                            if (iDecoderIndex != VOIPHEADSET_INVALID_INDEX)
                            {
                                // loop back for local playback
                                _VoipHeadsetWriteToPort(pHeadset->pDecoders[iDecoderIndex].uDecoderPortId, aBuffer, iFrameSize);
                                pHeadset->pDecoders[iDecoderIndex].uLastUsage = NetTick();
                            }
                            else
                            {
                                NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetps4: dropping chat data for local user index %d because no decoder available\n", iLocalUserIndex));
                            }
                        }
                    }
                }
                else
                {
                    NetPrintf(("voipheadsetps4: static buffer not big enough to read inbound voice frame on port %d for local user index %d\n",
                        pHeadset->aLocalUsers[iLocalUserIndex].uEncoderPortId, iLocalUserIndex));
                    break;
                }
            }
        }

        // if the we are using the camera mic we only need to read the voice packet from one of the players
        if (bCameraMic == TRUE)
        {
            break;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetUpdateChatPrivileges

    \Description
        Updates Chat Privileges for all the local users

    \Input *pHeadset    - pointer to headset state

    \Version 01/15/2015 (tcho)
*/
/********************************************************************************F*/
static void _VoipHeadsetUpdateChatPrivileges(VoipHeadsetRefT *pHeadset)
{
    int32_t iLocalUserIndex;
    uint32_t uMask = NetConnStatus('mask', 0 , NULL, 0);
    uint32_t uOldChatPrivilege = pHeadset->uChatPrivileges;
    uint32_t bOldSharedPrivilege = pHeadset->bSharedPrivilege;

    pHeadset->bSharedPrivilege = TRUE;
    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; ++iLocalUserIndex)
    {
        // make sure user is signed in with PSN
        if (uMask & (1 << iLocalUserIndex))
        {
            // query wether user can do voip or not
            uint8_t bPrivilege = !NetConnStatus('chat', iLocalUserIndex, NULL, 0);

            /* The shared privilege is set to be the most restrictive one from the set of "participating" users
            as defined by Sony. (Reference:  see R4061G from PS4-TestCase_for_TRC1.5_e.pdf)  That matches any
            user that is registered with DirtySDK voip OR any user that was identified as a contributor to
            the most restrictive voip privilege restriction by the game code via usage of '+pri' selector.
            Note: That however does not imply that the user has the bParticipating flag set to TRUE because
            that rather means the user is actively sending/receiving voip in the context of a blaze mesh. */
            if (!VOIP_NullUser(&pHeadset->aLocalUsers[iLocalUserIndex].user) || pHeadset->pVoipMostRestrictiveVoipContributors[iLocalUserIndex])
            {
                pHeadset->bSharedPrivilege = (bPrivilege ? pHeadset->bSharedPrivilege : FALSE);
            }

            if (!VOIP_NullUser(&pHeadset->aLocalUsers[iLocalUserIndex].user))
            {
                if (bPrivilege)
                {
                    pHeadset->uChatPrivileges |= (1 << iLocalUserIndex);
                }
                else
                {
                    pHeadset->uChatPrivileges &= ~(1 << iLocalUserIndex);
                }
            }
        }
    }

    // if the chat privileges changed, update the decoder connections accordingly
    if (pHeadset->uChatPrivileges != uOldChatPrivilege)
    {
        NetPrintf(("voipheadsetps4: decoder-to-speaker connections update scheduled because of change in users' chat privileges (0x%08x vs 0x%08x)\n",
            uOldChatPrivilege, pHeadset->uChatPrivileges));
        pHeadset->bDecoderConnUpdateNeeded = TRUE;
    }

    if (pHeadset->bSharedPrivilege != bOldSharedPrivilege)
    {
        NetPrintf(("voipheadsetps4: decoder-to-speaker connections update scheduled because of change in shared user's chat privilege (%s vs %s)\n",
            (bOldSharedPrivilege?"TRUE":"FALSE"), (pHeadset->bSharedPrivilege?"TRUE":"FALSE")));
        pHeadset->bDecoderConnUpdateNeeded = TRUE;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetChangeEncoderBps

    \Description
        Change bitrate configuration of all encoder and decoder ports.

    \Input *pHeadset    - pointer to headset state
    \Input iBitRate     - new bit rate

    \Output
        int32_t         - 0 for success; negative for failure

    \Version 05/14/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetChangeEncoderBps(VoipHeadsetRefT *pHeadset, int32_t iBitRate)
{
    int32_t iUserIndex;
    int32_t iDecoderIndex;
    int32_t iResult;
    int32_t iRetCode = 0; // default to success

    if (iBitRate == SCE_VOICE_BITRATE_3850 || iBitRate == SCE_VOICE_BITRATE_4650 || iBitRate == SCE_VOICE_BITRATE_5700 || iBitRate == SCE_VOICE_BITRATE_7300 ||
        iBitRate == SCE_VOICE_BITRATE_14400 || iBitRate == SCE_VOICE_BITRATE_16000 || iBitRate == SCE_VOICE_BITRATE_22533)
    {
        NetPrintf(("voipheadsetps4: encoding configuration changed from %d bps to %d bps\n", pHeadset->iEncoderBps, iBitRate));
        pHeadset->iEncoderBps = iBitRate;

        // modify bit rate of all encoder ports
        for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS; iUserIndex++)
        {
            if (!VOIP_NullUser(&pHeadset->aLocalUsers[iUserIndex].user))
            {
                if ((iResult = sceVoiceSetBitRate(pHeadset->aLocalUsers[iUserIndex].uEncoderPortId, pHeadset->iEncoderBps)) == SCE_OK)
                {
                    NetPrintf(("voipheadsetps4: successfully changed bitrate for encoder port %d belonging to local user index %d\n",
                        pHeadset->aLocalUsers[iUserIndex].uEncoderPortId, iUserIndex));
                }
                else
                {
                    NetPrintf(("voipheadsetps4: sceVoiceSetBitRate(%d) for local user index %d failed (err=%s)\n",
                        pHeadset->aLocalUsers[iUserIndex].uEncoderPortId, iUserIndex, DirtyErrGetName(iResult)));
                }
            }
        }

        // modify bit rate of all decoder ports
        for (iDecoderIndex = 0; iDecoderIndex < pHeadset->iDecoderPoolSize; iDecoderIndex++)
        {
            if (pHeadset->pDecoders[iDecoderIndex].bValid)
            {
                if ((iResult = sceVoiceSetBitRate(pHeadset->pDecoders[iDecoderIndex].uDecoderPortId, pHeadset->iEncoderBps)) == SCE_OK)
                {
                    NetPrintf(("voipheadsetps4: successfully changed bitrate for decoder port %d\n",
                        pHeadset->pDecoders[iDecoderIndex].uDecoderPortId));
                }
                else
                {
                    NetPrintf(("voipheadsetps4: sceVoiceSetBitRate(%d) failed (err=%s)\n",
                        pHeadset->pDecoders[iDecoderIndex].uDecoderPortId, DirtyErrGetName(iResult)));
                }
            }
        }
    }
    else
    {
        NetPrintf(("voipheadsetps4: %d bps is not a supported encoding configuration\n", iBitRate));
        iRetCode = -1;
    }

    return(iRetCode);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipHeadsetCreate

    \Description
        Create the headset manager.

    \Input iMaxChannels - max number of remote users
    \Input *pMicDataCb  - pointer to user callback to trigger when mic data is ready
    \Input *pTextDataCb  - pointer to user callback to trigger when transcribed text is ready
    \Input *pStatusCb   - pointer to user callback to trigger when headset status changes
    \Input *pCbUserData - pointer to user callback data
    \Input iAttr        - platform-specific - use to specify the voice lib thread attributes on PS4 

    \Output
        VoipHeadsetRefT *   - pointer to module state, or NULL if an error occured
    
    \Notes
        \verbatim
            Valid value for iAttr:
            The higher 16 bit is the CPU Affinity value. The lower 16 bits is the scheduling priority.
            For example iAttr = (SCE_KERNEL_CPUMASK_6CPU_ALL << 16) | SCE_KERNEL_PRIO_FIFO_HIGHEST.
        \endverbatim

    \Version 16/01/2013 (mclouatre)
*/
/********************************************************************************F*/
VoipHeadsetRefT *VoipHeadsetCreate(int32_t iMaxChannels, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetTextDataCbT *pTextDataCb, VoipHeadsetStatusCbT *pStatusCb, void *pCbUserData, int32_t iAttr)
{
    VoipHeadsetRefT *pHeadset;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    int32_t iSize;
    SceKernelCpumask cpuAffinity; 
    int32_t iSchedPriority;

    // query current mem group data
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // make sure we don't exceed maxchannels
    if (iMaxChannels > VOIP_MAXCONNECT)
    {
        NetPrintf(("voipheadsetps4: request for %d channels exceeds max\n", iMaxChannels));
        return(NULL);
    }

    // calculate size of module state
    // the size of the remote user array is the max number of remote peers + the max number of remote shared users
    iSize = sizeof(*pHeadset) + sizeof(PS4RemoteVoipUserT) * (iMaxChannels + VOIP_MAX_LOW_LEVEL_CONNS);

    // allocate and clear module state
    if ((pHeadset = DirtyMemAlloc(iSize, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipheadsetps4: not enough memory to allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pHeadset, iSize);

    pHeadset->iMaxRemoteUsers = iMaxChannels + VOIP_MAX_LOW_LEVEL_CONNS;
    pHeadset->iEncoderBps = VOIPHEADSET_VOICELIB_DEFAULT_ENCODER_BITRATE;

    // default voice lib thread attribute
    if ((cpuAffinity = NetConnStatus('affn', 0, NULL, 0)) <= 0)
    {
        cpuAffinity = SCE_KERNEL_CPUMASK_6CPU_ALL;
    }
    iSchedPriority = SCE_KERNEL_PRIO_FIFO_HIGHEST;

    // applying custom voice lib thread attributes if specified
    if (iAttr != 0)
    {
        iSchedPriority = iAttr & 0xFFFF;
        cpuAffinity = (SceKernelCpumask)(iAttr >> 16);
    }

    // set default debuglevel
    #if DIRTYCODE_LOGGING
    pHeadset->iDebugLevel = 1;
    #endif

    // save callback info
    pHeadset->pMicDataCb = pMicDataCb;
    pHeadset->pTextDataCb = pTextDataCb;
    pHeadset->pStatusCb = pStatusCb;
    pHeadset->pCbUserData = pCbUserData;
    pHeadset->iMaxChannels = iMaxChannels;

    // init sony voice lib (audio I/O systems)
    if (_VoipHeadsetVoiceLibInit(pHeadset) < 0)
    {
        NetPrintf(("voipheadsetps4: failed to initialize voice lib\n"));
        DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }

    // allocate and init pool of decoders
    // (needs to be performed after _VoipHeadsetVoiceLibInit() which initializes pHeadset->iDecoderPoolSize)
    if ((pHeadset->pDecoders = (PS4DecoderT *)DirtyMemAlloc(sizeof(PS4DecoderT) * pHeadset->iDecoderPoolSize, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        _VoipHeadsetVoiceLibShutdown();
        NetPrintf(("voipheadsetps4: not enough memory to allocate pool of decoders\n"));
        DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }
    ds_memclr(pHeadset->pDecoders, sizeof(PS4DecoderT) * pHeadset->iDecoderPoolSize);

    // set priority and affinity of internal voice and audio ports threads
    if (_VoipHeadsetVoiceSetThreadsParams(pHeadset, cpuAffinity, iSchedPriority) < 0)
    {
        DirtyMemFree(pHeadset->pDecoders, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        _VoipHeadsetVoiceLibShutdown();
        NetPrintf(("voipheadsetps4: failed to modify attributes of internal voice trheads\n"));
        DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }

    // start sony voice lib
    if (_VoipHeadsetVoiceLibStart(pHeadset) < 0)
    {
        DirtyMemFree(pHeadset->pDecoders, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        _VoipHeadsetVoiceLibShutdown();
        NetPrintf(("voipheadsetps4: failed to start voice lib\n"));
        DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }

    // return module ref to caller
    return(pHeadset);
}

/*F********************************************************************************/
/*!
    \Function   VoipHeadsetDestroy

    \Description
        Destroy the headset manager.

    \Input *pHeadset    - pointer to headset state

    \Version 16/01/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetDestroy(VoipHeadsetRefT *pHeadset)
{
    int32_t iMemGroup;
    int32_t iLocalUserIndex;
    int32_t iDecoderIndex;
    void *pMemGroupUserData;

    // release all user resources
    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; iLocalUserIndex++)
    {
        if (!VOIP_NullUser(&pHeadset->aLocalUsers[iLocalUserIndex].user))
        {
            if (pHeadset->aLocalUsers[iLocalUserIndex].bParticipating)
            {
                _VoipHeadsetDeactivateLocalTalker(pHeadset, iLocalUserIndex, &pHeadset->aLocalUsers[iLocalUserIndex].user);
            }
            _VoipHeadsetDeleteLocalTalker(pHeadset, iLocalUserIndex, &pHeadset->aLocalUsers[iLocalUserIndex].user);
        }
    }

    // release all decoder resources
    for (iDecoderIndex = 0; iDecoderIndex < pHeadset->iDecoderPoolSize; iDecoderIndex++)
    {
        if (pHeadset->pDecoders[iDecoderIndex].bValid)
        {
            _VoipHeadsetPortDelete(pHeadset->pDecoders[iDecoderIndex].uDecoderPortId);
        }
    }

    // stop sony voice lib
    _VoipHeadsetVoiceLibStop();

    // shutdown sony voice lib (audio I/O systems)
    _VoipHeadsetVoiceLibShutdown();

    // dispose of module memory and pool of decoders
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    DirtyMemFree(pHeadset->pDecoders, VOIP_MEMID, iMemGroup, pMemGroupUserData);
    DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function   VoipReceiveVoiceDataCb

    \Description
        Connectionlist callback to handle receiving a voice packet from a remote peer.

    \Input *pRemoteUsers - user we're receiving the voice data from
    \Input *pMicrInfo    - micr info from inbound packet
    \Input *pPacketData  - pointer to beginning of data in packet payload
    \Input *pUserData    - VoipHeadsetT ref

    \Version 16/01/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetReceiveVoiceDataCb(VoipUserT *pRemoteUsers, VoipMicrInfoT *pMicrInfo, uint8_t *pPacketData, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;
    uint32_t uMicrPkt;
    const uint8_t *pSubPacket;

    _VoipHeadsetUpdateDecodersOwnership(pHeadset);

    // submit voice sub-packets
    for (uMicrPkt = 0; uMicrPkt < pMicrInfo->uNumSubPackets; uMicrPkt++)
    {
        int32_t iRemoteUserSpaceIndex;

        // get pointer to data subpacket
        pSubPacket = (const uint8_t *)pPacketData + (uMicrPkt * pMicrInfo->uSubPacketSize);

        #if VOIPHEADSET_DEBUG
        NetPrintf(("voipheadsetps4: submitting chat data for remote user %s at index %d\n", pRemoteUsers[pMicrInfo->uUserIndex].strUserId, pMicrInfo->uUserIndex));
        #endif

        for (iRemoteUserSpaceIndex = 0; iRemoteUserSpaceIndex < pHeadset->iMaxRemoteUsers; iRemoteUserSpaceIndex++)
        {
            if (VOIP_SameUser(&pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].user, &pRemoteUsers[pMicrInfo->uUserIndex]))
            {
            
                int32_t iDecoderIndex;
                if (pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].iDecoderIndex == VOIPHEADSET_INVALID_INDEX)
                {
                    iDecoderIndex = _VoipHeadsetFindFreeDecoder(pHeadset);
                    if (iDecoderIndex >= 0)
                    {
                        _VoipHeadsetTakeDecoderOwnership(pHeadset, iDecoderIndex, iRemoteUserSpaceIndex, TRUE);
                    }
                }
                else
                {
                    iDecoderIndex = pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].iDecoderIndex;
                }

                if (iDecoderIndex >= 0)
                {
                    if (pHeadset->pDecoders[iDecoderIndex].uConnectedSpkrPort != 0)
                    {
                        _VoipHeadsetWriteToPort(pHeadset->pDecoders[iDecoderIndex].uDecoderPortId, pSubPacket, pMicrInfo->uSubPacketSize);
                        pHeadset->pDecoders[iDecoderIndex].uLastUsage = NetTick();
                    }
                    else
                    {
                        NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetps4: dropping chat data for remote user %s at index %d because decoder has no connection\n",
                            pRemoteUsers[pMicrInfo->uUserIndex].strUserId, pMicrInfo->uUserIndex));
                    }
                }
                else
                {
                    NetPrintfVerbose((pHeadset->iDebugLevel, 1, "voipheadsetps4: dropping chat data for remote user %s at index %d because no decoder available\n",
                        pRemoteUsers[pMicrInfo->uUserIndex].strUserId, pMicrInfo->uUserIndex));
                }
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipRegisterUserCb

    \Description
        Connectionlist callback to register a new remote user with the 1st party voice system

    \Input *pRemoteUser     - user to register
    \Input bRegister        - true=register, false=unregister
    \Input *pUserData       - voipheadset module ref

    \Version 16/01/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetRegisterUserCb(VoipUserT *pRemoteUser, uint32_t bRegister, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;
    int32_t iRemoteUserSpaceIndex;

    // early exit if invalid remote talker
    if (VOIP_NullUser(pRemoteUser))
    {
        NetPrintf(("voipheadsetps4: can't %s NULL remote talker\n", (bRegister?"register":"unregister")));
        return;
    }

    if (bRegister)
    {
        /*
        register
        */

        // find free remote user space
        for (iRemoteUserSpaceIndex = 0; iRemoteUserSpaceIndex < pHeadset->iMaxRemoteUsers; iRemoteUserSpaceIndex++)
        {
            if (VOIP_NullUser(&pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].user))
            {
                VOIP_CopyUser(&pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].user, pRemoteUser);
                pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].iDecoderIndex = VOIPHEADSET_INVALID_INDEX;
                pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].uEnablePlayback = -1; // enable playback by default
                NetPrintf(("voipheadsetps4: registered remote talker %s at remote user index %d\n", pRemoteUser->strUserId, iRemoteUserSpaceIndex));
                break;
            }
        }

        if (iRemoteUserSpaceIndex < pHeadset->iMaxRemoteUsers)
        {
            // attempt to create an additional decoder in the pool (if there is still room for it)
            if (_VoipHeadsetCreateDecoder(pHeadset) == 0)
            {
                // make sure all decoders are connected to all speaker ports of all participating users
                pHeadset->bDecoderConnUpdateNeeded = TRUE;
            }
        }
        else
        {
            NetPrintf(("voipheadsetps4: failed to register remote talker %s, max number of channels reached\n", pRemoteUser->strUserId));
        }
    }
    else
    {
        /*
        unregister
        */

        // find matching user 
        for (iRemoteUserSpaceIndex = 0; iRemoteUserSpaceIndex < pHeadset->iMaxRemoteUsers; iRemoteUserSpaceIndex++)
        {
            if (VOIP_SameUser(&pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].user, pRemoteUser))
            {
                // release decoder if needed
                if (pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].iDecoderIndex != VOIPHEADSET_INVALID_INDEX)
                {
                    _VoipHeadsetReleaseDecoderOwnership(pHeadset, pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].iDecoderIndex);
                }

                pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].uEnablePlayback = -1; // clear playback bit field by default
                NetPrintf(("voipheadsetps4: unregistered remote talker %s at remote user index %d\n", pRemoteUser->strUserId, iRemoteUserSpaceIndex));

                VOIP_ClearUser(&pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].user);
                break;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipHeadsetProcess

    \Description
        Headset process function.

    \Input *pHeadset    - pointer to headset state
    \Input uFrameCount  - current frame count

    \Version 16/01/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetProcess(VoipHeadsetRefT *pHeadset, uint32_t uFrameCount)
{
    _VoipHeadsetUpdateChatPrivileges(pHeadset);

    if (pHeadset->bDecoderConnUpdateNeeded == TRUE)
    {
        if (pHeadset->bDecoderConnUpdateInProgress == FALSE)
        {
           pHeadset->bDecoderConnUpdateInProgress = TRUE;
           pHeadset->bDecoderConnUpdateNeeded = FALSE;

           _VoipHeadsetUpdateDecoderConnections(pHeadset);

           pHeadset->bDecoderConnUpdateInProgress = FALSE;
        }
    }
    
    _VoipHeadsetUpdateStatus(pHeadset);
    _VoipHeadsetPollForVoiceData(pHeadset);
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

    \Version 16/01/2013 (mclouatre)
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
    \Input iControl     - control selector
    \Input iValue       - control value
    \Input iValue2      - control value
    \Input *pValue      - control value

    \Output
        int32_t         - selector specific, or -1 if no such selector

    \Notes
        iControl can be one of the following:

        \verbatim
            '+pbk' - enable voice playback for a given remote user (pValue is the remote user VoipUserT).
            '-pbk' - disable voice playback for a given remote user (pValue is the remote user VoipUserT).
            'aloc' - activate/deactivate local user (enter/exit "participating" state)
            'epbs' - modified encoding bps generation configuration
            'loop' - enable/disable loopback
            'rloc' - register/unregister local user
            'spam' - debug verbosity level
            'vpri' - used by the voip layer to pass pointer to array of contributors to voip most restrictive privilege
        \endverbatim

    \Version 16/01/2013 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipHeadsetControl(VoipHeadsetRefT *pHeadset, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'aloc')
    {
        int32_t iResult;

        if (iValue2)
        {
            iResult = _VoipHeadsetActivateLocalTalker(pHeadset, iValue, (VoipUserT *)pValue);
        }
        else 
        {
            iResult = _VoipHeadsetDeactivateLocalTalker(pHeadset, iValue, (VoipUserT *)pValue);
        }

        return(iResult);
    }
    if (iControl == 'ebps')
    {
        return(_VoipHeadsetChangeEncoderBps(pHeadset, iValue));
    }
    if (iControl == 'loop')
    {
        pHeadset->bLoopback = iValue;
        return(0);
    }
    if ((iControl == '-pbk') || (iControl == '+pbk' ))
    {
        int8_t bVoiceEnable = (iControl == '+pbk')? TRUE: FALSE; 
        int32_t iIndex;
        VoipUserT *pRemoteUser;

        if ((pValue != NULL) && (iValue < VOIP_MAXLOCALUSERS))
        {
            pRemoteUser = (VoipUserT *)pValue;

            // look up the remote user and set playback
            for (iIndex = 0; iIndex < pHeadset->iMaxRemoteUsers; ++iIndex)
            {
                if (VOIP_SameUser(&pHeadset->aRemoteUsers[iIndex].user, pRemoteUser))
                {
                    if (bVoiceEnable == TRUE)
                    {
                         pHeadset->aRemoteUsers[iIndex].uEnablePlayback |= (1 << iValue);
                         NetPrintf(("voipheadsetps4:VoipHeadsetControl(+pbk) voice playback enabled for remote User Index %i, bPlayback bit field 0x%08X\n", iIndex, pHeadset->aRemoteUsers[iIndex].uEnablePlayback));
                    }
                    else
                    {
                         pHeadset->aRemoteUsers[iIndex].uEnablePlayback &= ~(1 << iValue);
                         NetPrintf(("voipheadsetps4:VoipHeadsetControl(-pbk) voice playback disabled for remote User Index %i, bPlayback bit field 0x%08X\n", iIndex, pHeadset->aRemoteUsers[iIndex].uEnablePlayback));
                    }

                    pHeadset->bDecoderConnUpdateNeeded =TRUE;
                    return(0);
                }
            }
            
            NetPrintf(("voipheadsetps4: VoipHeadsetControl('%s') no remote user was found. User: %s\n", bVoiceEnable? "+pbk":"-pbk", pRemoteUser->strUserId));
            return(-2);
        }
        else
        {
            NetPrintf(("voipheadsetps4: VoipHeadsetControl('%s') invalid arguments\n", bVoiceEnable? "+pbk":"-pbk"));
            return(-1);
        }
    }
    if (iControl == 'rloc')
    {
        if (iValue2)
        {
            return(_VoipHeadsetAddLocalTalker(pHeadset, iValue, (VoipUserT *)pValue));
        }
        else
        {
            return(_VoipHeadsetDeleteLocalTalker(pHeadset, iValue, (VoipUserT *)pValue));
        }
    }
    if (iControl == 'vpri')
    {
        pHeadset->pVoipMostRestrictiveVoipContributors = (uint8_t *) pValue;
    }
    #if DIRTYCODE_LOGGING
    if (iControl == 'spam')
    {
        NetPrintf(("voipheadsetps4: setting debuglevel=%d\n", iValue));
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

        \verbatim
            'camr' - return TRUE is the PS Camera Mic is in use.
            'ruid' - return TRUE if the given remote user (pBuf) is registered with voipheadset, FALSE if not.
            'sact' - return TRUE if shared device (PS Camera) is active
            'hpao' - return the audio device output flag for a given user index (iValue). IMPORTANT: value is not valid until something has been written to the outpout port
        \endverbatim

    \Version 16/01/2013 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipHeadsetStatus(VoipHeadsetRefT *pHeadset, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'camr')
    {
        int32_t iResult;
        int32_t iLocalUserIndex;
        uint8_t bCamera = TRUE;
        uint8_t bUserFound = FALSE;

        for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; ++iLocalUserIndex)
        {
            uint8_t bAttr = 0;
            if (!VOIP_NullUser(&pHeadset->aLocalUsers[iLocalUserIndex].user) && (pHeadset->aLocalUsers[iLocalUserIndex].bParticipating == TRUE))
            {
                if ((iResult = sceVoiceGetPortAttr(pHeadset->aLocalUsers[iLocalUserIndex].uMicPortId, SCE_VOICE_ATTR_AUDIOINPUT_DEVICE_CAMERA, &bAttr, sizeof(bAttr))) == SCE_OK)
                {
                    bUserFound = TRUE;
                    bCamera &= bAttr;
                }
                else
                {
                    // print out the error but still continue on with the other users
                    NetPrintf(("voipheadsetstatus: 'camr' for user index %i failed with error %s\n",iLocalUserIndex, DirtyErrGetName(iResult)));
                }
            }
        }

        if (bUserFound == TRUE)
        {
            return(bCamera);
        }
        else
        {
            return(FALSE);
        }
    }
    if (iSelect == 'ruid')
    {
        int32_t iRemoteUserSpaceIndex = 0;

        for (iRemoteUserSpaceIndex = 0; iRemoteUserSpaceIndex < pHeadset->iMaxRemoteUsers; ++iRemoteUserSpaceIndex)
        {
            if (VOIP_SameUser(&pHeadset->aRemoteUsers[iRemoteUserSpaceIndex].user, (VoipUserT *)pBuf))
            {
                // remote user found and it is registered with voipheadset
                return (TRUE);
            }
        }

        return(FALSE);
    }
    if (iSelect == 'sact')
    {
        return(VoipHeadsetStatus(pHeadset, 'camr', 0, NULL, 0) && (pHeadset->uLastHeadsetStatus == 0));
    }
    if (iSelect == 'hpao')
    {
        int32_t iResult;
        int16_t iOutput = 0;

        if (!VOIP_NullUser(&pHeadset->aLocalUsers[iValue].user) && (pHeadset->aLocalUsers[iValue].bParticipating == TRUE))
        {
            if ((iResult = sceVoiceGetPortAttr(pHeadset->aLocalUsers[iValue].uSpkrPortId, SCE_VOICE_ATTR_AUDIOOUTPUT_DESTINATION , &iOutput, sizeof(iOutput))) == SCE_OK)
            {
                return(iOutput);
            }
        }
    }
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

    \Version 16/01/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipHeadsetSpkrCallback(VoipHeadsetRefT *pHeadset, VoipSpkrCallbackT *pCallback, void *pUserData)
{
    pHeadset->pSpkrDataCb = pCallback;
    pHeadset->pSpkrCbUserData = pUserData;
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetDisplayTranscribedText

    \Description
        Display transcribed text in PS4 UI

    \Input *pHeadset    - headset module state
    \Input *pOriginator - originator of the message
    \Input *pText       - message to be displayed
    \Output
        int32_t         - 0 if success, negative otherwise

    \Version 06/13/2017 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipHeadsetDisplayTranscribedText(VoipHeadsetRefT *pHeadset, VoipUserT *pOriginator, const wchar_t *pText)
{
    NetPrintf(("voipheadsetpc: VoipHeadsetDisplayTranscribedText() not yet supported on PS4\n"));
    return(-1);
}

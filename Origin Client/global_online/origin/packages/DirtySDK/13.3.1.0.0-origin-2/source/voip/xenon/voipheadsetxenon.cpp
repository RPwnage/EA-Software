/*H********************************************************************************/
/*!
    \File voipheadsetxenon.cpp

    \Description
        VoIP headset manager.

    \Copyright
        Copyright (c) 2009 Electronic Arts Inc.

    \Version 11/10/2009 (jbrookes) Split from voipxenon.cpp
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <xtl.h>
#include <xonline.h>
#include <xaudio2.h>
#include <xhv2.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipconnection.h"
#include "voipheadset.h"

/*** Defines **********************************************************************/

#define VOIP_XHV_DEBUG      (DIRTYCODE_DEBUG && FALSE)

/*** Type Definitions *************************************************************/

//! voip headset module state data
struct VoipHeadsetRefT
{
    IXAudio2 *pXAudio2;
    PIXHV2ENGINE pXHVEngine;     //!< XHV Engine pointer

    //! channel info
    int32_t iMaxChannels;

    //! TRUE if the mic is on, else FALSE
    volatile uint8_t bMicOn;
    uint8_t uSendSeq;
    uint8_t bLoopback;
    uint8_t _pad;

    uint32_t uFrameSend;

    VoipHeadsetMicDataCbT *pMicDataCb;  //!< mic data callback
    VoipHeadsetStatusCbT *pStatusCb;    //!< headset status callback
    void *pCbUserData;                  //!< user callback data

    uint32_t uHeadsetStatus;

    XUID LocalUsers[4];
};

/*** Variables ********************************************************************/

static PFNMICRAWDATAREADY   gpRawDataReadyCb = NULL;

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _VoipHeadsetXHVCreate

    \Description
        Create the headset manager.

    \Input *pHeadset        - headset module state
    \Input iMaxPeers        - max number of remote users
    \Input iMaxLocal        - max number of local users
    \Input iProcessor       - Xbox360 hardware thread to pass to XAudio2Create() (user 0 for default)

    \Output
        int32_t         - negative=failure; else success

    \Notes
        \verbatim
            Valid value range for iProcessor:
            0                           - use default, i.e. XAUDIO2_DEFAULT_PROCESSOR
            XboxThread0                 - value 0x01
            XboxThread1                 - value 0x02
            XboxThread2                 - value 0x04
            XboxThread3                 - value 0x08
            XboxThread4                 - value 0x10
            XboxThread5                 - value 0x20
            XAUDIO2_ANY_PROCESSOR       - value 0x3f
            XAUDIO2_DEFAULT_PROCESSOR   - (XboxThread4|XboxThread5)
        \endverbatim

    \Version 11/10/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetXHVCreate(VoipHeadsetRefT *pHeadset, int32_t iMaxPeers, int32_t iMaxLocal, int32_t iProcessor)
{
    XHV_INIT_PARAMS XHVParams;
    PIXHV2ENGINE pXHVEngine;
    IXAudio2 *pXAudio2;
    HANDLE hWorkerThread;
    HRESULT hResult;
    XAUDIO2_PROCESSOR XAudio2Processor;

    // set up parameters
    memset(&XHVParams, 0, sizeof(XHVParams));
    XHVParams.dwMaxLocalTalkers = iMaxLocal;
    XHVParams.dwMaxRemoteTalkers = iMaxPeers;

    XHV_PROCESSING_MODE rgModes[] = { XHV_VOICECHAT_MODE, XHV_LOOPBACK_MODE };
    XHVParams.dwNumLocalTalkerEnabledModes = 2;
    XHVParams.dwNumRemoteTalkerEnabledModes = 1;
    XHVParams.localTalkerEnabledModes = rgModes;
    XHVParams.remoteTalkerEnabledModes = rgModes;

    if (gpRawDataReadyCb != NULL)
    {
        XHVParams.pfnMicrophoneRawDataReady = gpRawDataReadyCb;
        XHVParams.bCustomVADProvided = TRUE;
    }

    // find out which hardware thread to pass to XAudio2Create()
    if (iProcessor)
    {
        // use user-specified value
        XAudio2Processor =  iProcessor;
    }
    else
    {
        // use default
        XAudio2Processor = XAUDIO2_DEFAULT_PROCESSOR;
    }

    // create the XVH engine
    HRESULT error = XAudio2Create(&pXAudio2, 0, XAudio2Processor);
    IXAudio2MasteringVoice *pXAudio2MasteringVoice;
    error = pXAudio2->CreateMasteringVoice(&pXAudio2MasteringVoice, 6, 48000);

    XHVParams.pXAudio2 = pXAudio2;
    hResult = XHV2CreateEngine(&XHVParams, &hWorkerThread, &pXHVEngine);

    if(FAILED(hResult))
    {
        NetPrintf(("voipheadsetxenon: error 0x%08x creating XVH engine\n", hResult));
        pXAudio2->Release();
        return(-1);
    }

    /*
    Raise the priority of the XHV worker thread to be same priority as _NetLib, _VoipThread, _SocketRecvThread.
    This is to avoid a deadlock caused by a thread priority inversion between the XHV worker thread and these
    other threads when XAudio2 is used.
    */
    SetThreadPriority(hWorkerThread, THREAD_PRIORITY_HIGHEST);

    // save XHV ref
    pHeadset->pXHVEngine = pXHVEngine;

    // save the XAudio2 ref
    pHeadset->pXAudio2 = pXAudio2;

    // success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetGetLocalChatData

    \Description
        pulls chat data that is queued

    \Input *pHeadset    - module state
    \Input *pBuffer     - contains the data that has been read on exit
    \Input dwSize       - length of data that was read

    \Output
        int32_t         - user index associated with the chat data

    \Version 03/23/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetGetLocalChatData(VoipHeadsetRefT *pHeadset, BYTE *pBuffer, DWORD dwSize)
{
    int32_t iPort;
    DWORD dwResult, dwTemp;

    for (iPort = 0; iPort < 4; iPort++)
    {
        // don't poll for ports we haven't registered
        if (pHeadset->LocalUsers[iPort] == 0)
        {
            continue;
        }

        dwTemp = dwSize;
        dwResult = pHeadset->pXHVEngine->GetLocalChatData(iPort, pBuffer, &dwTemp, NULL);

        if (SUCCEEDED(dwResult) && (dwTemp > 0))
        {
            // make sure we got the expected data amount
            #if DIRTYCODE_LOGGING
            if (dwSize != XHV_VOICECHAT_MODE_PACKET_SIZE)
            {
                NetPrintf(("voipheadsetxenon: invalid voice data size %d (should be %d)\n", dwSize, XHV_VOICECHAT_MODE_PACKET_SIZE));
            }
            #endif

            #if VOIP_XHV_DEBUG
            NetPrintf(("voipheadsetxenon: read %d byte voice packet from port %d\n", dwSize, iPort));
            #endif

            break;
        }
    }

    return((iPort < 4) ? iPort : -1);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetPollForVoiceData

    \Description
        Poll to see if voice data is available.

    \Input *pHeadset    - module state

    \Version 05/17/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetPollForVoiceData(VoipHeadsetRefT *pHeadset)
{
    BYTE  aBuffer[XHV_VOICECHAT_MODE_PACKET_SIZE];
    int32_t iPort;

    // pull any data that is queued up
    while ((iPort = _VoipHeadsetGetLocalChatData(pHeadset, aBuffer, sizeof(aBuffer))) >= 0)
    {
        // forward data to upper-layer module
        pHeadset->pMicDataCb(aBuffer, sizeof(aBuffer), iPort, pHeadset->uSendSeq++, pHeadset->pCbUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipUpdateHeadsetStatus

    \Description
        Update headset status bitmask.

    \Input  *pHeadset   - module reference

    \Version 09/07/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetUpdateStatus(VoipHeadsetRefT *pHeadset)
{
    uint32_t bHeadsetPresent;
    uint32_t uStatus;
    int32_t iPort;

    // update headset status
    for (iPort = 0, uStatus = 0; iPort < VOIP_MAXLOCALUSERS; iPort++)
    {
        // get headset status for port (note this will only return TRUE if RegisterLocalTalker() has been called on this port!)
        bHeadsetPresent = pHeadset->pXHVEngine->IsHeadsetPresent(iPort);
        uStatus |= bHeadsetPresent << iPort;
        #if DIRTYCODE_LOGGING
        if ((1 << iPort) & (pHeadset->uHeadsetStatus ^ uStatus))
        {
            NetPrintf(("voipheadsetxenon: headset in port %d %s\n", iPort, (uStatus & (1 << iPort)) ? "inserted" : "removed"));
        }
        #endif
    }

    // save updated status
    pHeadset->uHeadsetStatus = uStatus;

    // trigger callback
    pHeadset->pStatusCb(iPort, uStatus, pHeadset->pCbUserData);
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetSubmitVoiceData

    \Description
        Submit voice data to XHV.

    \Input *pHeadset     - module state
    \Input *pRemoteUser  - user we're receiving the voice data from
    \Input uNumPackets   - number of packets in voice bundle
    \Input *pVoiceData   - voice data to submit

    \Version 01/10/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetSubmitVoiceData(VoipHeadsetRefT *pHeadset, VoipUserT *pRemoteUser, uint32_t uNumPackets, const BYTE *pVoiceData)
{
    HRESULT hResult;
    XUID Xuid;

    #if VOIP_XHV_DEBUG
    NetPrintf(("voipheadsetxenon: submitting %d packets of chat data for remote user %s\n", uNumSubPackets, pRemoteUser->strUserId));
    #endif
    VoipXuidFromUser(&Xuid, pRemoteUser);

    DWORD dwSize = uNumPackets * XHV_VOICECHAT_MODE_PACKET_SIZE;
    DWORD dwWrittenSize = dwSize;
    hResult = pHeadset->pXHVEngine->SubmitIncomingChatData(Xuid, pVoiceData, &dwSize);

    if (hResult != S_OK)
    {
        NetPrintf(("voipheadsetxenon: SubmitIncomingChatData() failed (size=%d, err=%s) for remote user %s\n",
            dwWrittenSize, DirtyErrGetName(hResult), pRemoteUser->strUserId));
    }

    if (dwSize < dwWrittenSize)
    {
        NetPrintf(("voipheadsetxenon: SubmitIncomingChatData() could not consume all voip data (size=%d, consumed=%d) from remote user %s\n",
            dwWrittenSize, dwSize, pRemoteUser->strUserId));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetRegisterRemoteTalker

    \Description
        Connectionlist callback to register a new user with the XHV Engine

    \Input *pHeadset    - headset module
    \Input iLocalIdx    - local index (required when unregistering user)
    \Input *pRemoteUser - remote user
    \Input bRegister    - TRUE to register, FALSE to unregister

    \Version 03/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipHeadsetRegisterRemoteTalker(VoipHeadsetRefT *pHeadset, int32_t iLocalIdx, VoipUserT *pRemoteUser, uint32_t bRegister)
{
    PIXHV2ENGINE pXHVEngine = pHeadset->pXHVEngine;
    XUID Xuid;

    // don't unregister a null user
    if (VOIP_NullUser(pRemoteUser))
    {
        return;
    }

    // convert from VoipUserT to Xuid
    VoipXuidFromUser(&Xuid, pRemoteUser);

    // register remote user
    if (bRegister)
    {
        NetPrintf(("voipheadsetxenon: registering remote talker %s\n", pRemoteUser->strUserId));
        pXHVEngine->RegisterRemoteTalker(Xuid, NULL, NULL, NULL);
        pXHVEngine->StartRemoteProcessingModes(Xuid, (PXHV_PROCESSING_MODE)&XHV_VOICECHAT_MODE, 1);
        NetPrintf(("voipheasetxenon: enabling playback of voice from remote user '%s' for local user %d because that remote user has just been registered\n",
            pRemoteUser->strUserId, iLocalIdx));
        pXHVEngine->SetPlaybackPriority(Xuid, iLocalIdx, XHV_PLAYBACK_PRIORITY_MAX);
    }
    else
    {
        NetPrintf(("voipheasetxenon: disabling playback of voice from remote user '%s' for local user %d because that remote user is about to be unregistered\n",
            pRemoteUser->strUserId, iLocalIdx));
        pXHVEngine->SetPlaybackPriority(Xuid, iLocalIdx, XHV_PLAYBACK_PRIORITY_NEVER);
        NetPrintf(("voipheadsetxenon: unregistering remote talker %s from port %d\n", pRemoteUser->strUserId, iLocalIdx));
        pXHVEngine->UnregisterRemoteTalker(Xuid);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipHeadsetRegisterLocalTalker

    \Description
        Register the local talker on the given port.

    \Input *pHeadset    - headset module
    \Input iPortID      - port to register talker on
    \Input bRegister    - TRUE to register user, FALSE to unregister
    \Input *pVoipUser   - VoipUserT of user to register (not required for unregister)

    \Output
        int32_t         - 1 if the local talker was successfully registered, else 0

    \Version 03/26/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipHeadsetRegisterLocalTalker(VoipHeadsetRefT *pHeadset, int32_t iPortID, uint32_t bRegister, VoipUserT *pVoipUser)
{
    HRESULT hResult;
    int32_t iResult;

    if (bRegister)
    {
        // register local talker
        NetPrintf(("voipheadsetxenon: registering user '%s' as local talker to port %d\n", pVoipUser->strUserId, iPortID));
        if ((hResult = pHeadset->pXHVEngine->RegisterLocalTalker(iPortID)) == S_OK)
        {
            // enable voice chat
            pHeadset->pXHVEngine->StartLocalProcessingModes(iPortID, &XHV_VOICECHAT_MODE, 1);
            // remember registration
            VoipXuidFromUser(&pHeadset->LocalUsers[iPortID], pVoipUser);
            iResult = 1;
        }
        else
        {
            NetPrintf(("voipheadsetxenon: RegisterLocalTalker(%d) failed (err=%s)\n", iPortID, DirtyErrGetName(hResult)));
            iResult = 0;
        }
    }
    else
    {
        NetPrintf(("voipheadsetxenon: unregistering local talker from port %d\n", iPortID));
        pHeadset->pXHVEngine->StopLocalProcessingModes(iPortID, &XHV_VOICECHAT_MODE, 1);
        pHeadset->pXHVEngine->UnregisterLocalTalker(iPortID);
        // clear registration
        pHeadset->LocalUsers[iPortID] = 0;
        iResult = 1;
    }

    // return if we were able to set the local talker or not
    return(iResult);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipHeadsetCreate

    \Description
        Create the headset manager.

    \Input iMaxChannels - max number of remote users
    \Input iMaxLocal    - max number of local users
    \Input *pMicDataCb  - pointer to user callback to trigger when mic data is ready
    \Input *pStatusCb   - pointer to user callback to trigger when headset status changes
    \Input *pCbUserData - pointer to user callback data
    \Input iData        - platform-specific - Xbox360 hardware thread to pass to XAudio2Create() (user 0 for default)

    \Output
        VoipHeadsetRefT*   - NULL=failure; else pointer to VoipHeadset ref

    \Notes
        \verbatim
            Valid value range for iProcessor:
            0                           - use default, i.e. XAUDIO2_DEFAULT_PROCESSOR
            XboxThread0                 - value 0x01
            XboxThread1                 - value 0x02
            XboxThread2                 - value 0x04
            XboxThread3                 - value 0x08
            XboxThread4                 - value 0x10
            XboxThread5                 - value 0x20
            XAUDIO2_ANY_PROCESSOR       - value 0x3f
            XAUDIO2_DEFAULT_PROCESSOR   - (XboxThread4|XboxThread5)
        \endverbatim

    \Version 11/10/2009 (jbrookes)
*/
/********************************************************************************F*/
VoipHeadsetRefT *VoipHeadsetCreate(int32_t iMaxChannels, int32_t iMaxLocal, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetStatusCbT *pStatusCb, void *pCbUserData, int32_t iData)
{
    VoipHeadsetRefT *pHeadset;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // make sure we don't exceed maxchannels
    if (iMaxChannels > VOIP_MAXCONNECT)
    {
        NetPrintf(("voipheadsetxenon: request for %d channels exceeds max\n", iMaxChannels));
        return(NULL);
    }

    // allocate and clear module state
    if ((pHeadset = (VoipHeadsetRefT *)DirtyMemAlloc(sizeof(*pHeadset), VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipheadsetxenon: not enough memory to allocate module state\n"));
        return(NULL);
    }
    memset(pHeadset, 0, sizeof(*pHeadset));

    // save info
    pHeadset->pMicDataCb = pMicDataCb;
    pHeadset->pStatusCb = pStatusCb;
    pHeadset->pCbUserData = pCbUserData;
    pHeadset->iMaxChannels = iMaxChannels;

    // start up XHV
    if (_VoipHeadsetXHVCreate(pHeadset, iMaxChannels, iMaxLocal, iData) < 0)
    {
        VoipHeadsetDestroy(pHeadset);
        return(NULL);
    }

    // return module ref to caller
    return(pHeadset);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetDestroy

    \Description
        Destroy the headset manager.

    \Input *pHeadset    - pointer to headset state

    \Version 11/10/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetDestroy(VoipHeadsetRefT *pHeadset)
{
    int32_t iLocalIdx, iMemGroup;
    void *pMemGroupUserData;

    // unregister local talkers
    for (iLocalIdx = 0; iLocalIdx < 4; iLocalIdx += 1)
    {
        if (pHeadset->LocalUsers[iLocalIdx] != 0)
        {
            pHeadset->pXHVEngine->UnregisterLocalTalker(iLocalIdx);
        }
    }

    // release XHV Engine
    if (pHeadset->pXHVEngine != NULL)
    {
        pHeadset->pXHVEngine->Release();
    }
    // release xaudio2 ref
    if (pHeadset->pXAudio2 != NULL)
    {
        pHeadset->pXAudio2->Release();
    }

    // dispose of module memory
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    DirtyMemFree(pHeadset, VOIP_MEMID, iMemGroup, pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetReceiveVoiceDataCb

    \Description
        Connectionlist callback to handle receiving a voice packet from a remote peer.

    \Input *pRemoteUser  - user we're receiving the voice data from
    \Input *pMicrPacket  - micr packet we're receiving
    \Input *pUserData    - VoipHeadsetT ref

    \Version 11/10/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetReceiveVoiceDataCb(VoipUserT *pRemoteUser, VoipMicrPacketT *pMicrPacket, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;
    uint32_t uMicrPkt;
    const BYTE *pDataPacket;

    // submit voice sub-packets
    for (uMicrPkt = 0; uMicrPkt < pMicrPacket->MicrInfo.uNumSubPackets; uMicrPkt++)
    {
        // get pointer to data subpacket
        pDataPacket = (const BYTE *)pMicrPacket->aData + (uMicrPkt * pMicrPacket->MicrInfo.uSubPacketSize);

        // submit the voice data
        _VoipHeadsetSubmitVoiceData(pHeadset, &pRemoteUser[pMicrPacket->MicrInfo.uUserIndex], 1, pDataPacket);
    }
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetRegisterUserCb

    \Description
        Connectionlist callback to register a new user with the XHV Engine

    \Input *pRemoteUser - user to register
    \Input iLocalIdx    - local index to register to
    \Input bRegister    - true=register, false=unregister
    \Input *pUserData   - voipheadset module ref

    \Version 11/10/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetRegisterUserCb(VoipUserT *pRemoteUser, int32_t iLocalIdx, uint32_t bRegister, void *pUserData)
{
    VoipHeadsetRefT *pHeadset = (VoipHeadsetRefT *)pUserData;
    _VoipHeadsetRegisterRemoteTalker(pHeadset, iLocalIdx, pRemoteUser, bRegister);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetProcess

    \Description
        Headset process function.

    \Input *pHeadset    - pointer to headset state
    \Input uFrameCount  - current frame count

    \Version 11/10/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetProcess(VoipHeadsetRefT *pHeadset, uint32_t uFrameCount)
{
    _VoipHeadsetUpdateStatus(pHeadset);

    _VoipHeadsetPollForVoiceData(pHeadset);
}

/*F********************************************************************************/
/*!
    \Function VoipHeadsetSetMicStatus

    \Description
        Enable or disable the mic.

    \Input *pHeadset    - pointer to headset state
    \Input bMicOn       - TRUE to enable mic, else FALSE to disable it.

    \Version 11/10/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipHeadsetSetMicStatus(VoipHeadsetRefT *pHeadset, uint32_t bMicOn)
{
}

/*F*************************************************************************************************/
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
            'mrdr' - set raw data ready callback (must be called before VoipStartup()) - iValue=pCallback
            'prio' - set playback priority for user; pValue=VoipUserT, iValue=port, iValue2=enable/disable
            'rloc' - un/register local talker; iValue=port, iValue2=enable/disable, pValue=VoipUserT (not required for unregistration)
        \endverbatim

    \Version 11/10/2009 (jbrookes)
*/
/*************************************************************************************************F*/
int32_t VoipHeadsetControl(VoipHeadsetRefT *pHeadset, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iSelect == 'mrdr')
    {
        gpRawDataReadyCb = (PFNMICRAWDATAREADY)iValue;
        return(0);
    }
    if (iSelect == 'prio')
    {
        DWORD dwPriority = iValue2 ? XHV_PLAYBACK_PRIORITY_MAX : XHV_PLAYBACK_PRIORITY_NEVER;
        XUID Xuid;
        VoipXuidFromUser(&Xuid, (VoipUserT *)pValue);
        NetPrintf(("voipheasetxenon: %s playback of voice from remote user '%s' for local user %d\n", iValue2?"enabling":"disabling", ((VoipUserT *)pValue)->strUserId, iValue));
        pHeadset->pXHVEngine->SetPlaybackPriority(Xuid, iValue, dwPriority);
        return(0);
    }
    if (iSelect == 'rloc')
    {
        return(_VoipHeadsetRegisterLocalTalker(pHeadset, iValue, iValue2, (VoipUserT *)pValue));
    }
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function VoipHeadsetStatus

    \Description
        Status function.

    \Input *pHeadset    - headset module state
    \Input iSelect      - control selector
    \Input iValue       - selector specific
    \Input pBuf         - buffer pointer
    \Input iBufSize     - buffer size

    \Output
        int32_t         - selector specific, or -1 if no such selector

    \Notes
        iSelect can be one of the following:

        \verbatim
        \endverbatim

    \Version 11/10/2009 (jbrookes)
*/
/*************************************************************************************************F*/
int32_t VoipHeadsetStatus(VoipHeadsetRefT *pHeadset, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function VoipHeadsetSpkrCallback

    \Description
        Set speaker output callback.

    \Input *pHeadset    - headset module state
    \Input *pCallback   - what to call when output data is available
    \Input *pUserData   - user data for callback

    \Version 11/10/2009 (jbrookes)
*/
/*************************************************************************************************F*/
void VoipHeadsetSpkrCallback(VoipHeadsetRefT *pHeadset, VoipSpkrCallbackT *pCallback, void *pUserData)
{
}

/*H*************************************************************************************************/
/*!

    \File    voip.h

    \Description
        Main include for Voice Over IP module.

    \Notes
        MLU:
           Every Local User should be registered with the voip subsytem in order to get headset status info with VoipSetLocalUser().
           In order to get fully functional voip a user must be further activated which enrolls them as a participating user with VoipActivateLocalUser().

           Basically any users that are being pulled into the game should be a participating user.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002-2004.    ALL RIGHTS RESERVED.

    \Version    1.0        11/19/02 (IRS) First Version
    \Version    2.0        05/13/03 (GWS) Rewrite to fix misc bugs and improve network connection.
    \Version    3.0        11/03/03 (JLB) Implemented unloadable support, major code cleanup and reorganization.
    \Version    3.5        03/02/04 (JLB) VoIP 2.0 - API changes for future multiple channel support.
    \Version    3.6        11/19/08 (mclouatre) Modified synopsis of VoipStatus()
*/
/*************************************************************************************************H*/

#ifndef _voip_h
#define _voip_h

/*!
    \Moduledef Voip Voip

    \Description
        The Voip module provides ...

        <b>Overview</b>

        Overview TBD
*/
//@{

/*** Include files ********************************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/voip/voipdef.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// prepare voip for use
DIRTYCODE_API VoipRefT *VoipStartup(int32_t iMaxPeers, int32_t iData);

// return pointer to current module state
DIRTYCODE_API VoipRefT *VoipGetRef(void);

// release all voip resources
DIRTYCODE_API void VoipShutdown(VoipRefT *pVoip, uint32_t uShutdownFlags);

// register/unregister specified local user with the voip sub-system  (allows for local headset status query, but not voice acquisition/playback)
DIRTYCODE_API void VoipSetLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint32_t bRegister);

// activate/deactivate specified local user (when activated, the user becomes a "participating" user and voice acquisition/playback is enabled for that user)
DIRTYCODE_API void VoipActivateLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint8_t bActivate);

// set event notification callback
DIRTYCODE_API void VoipSetEventCallback(VoipRefT *pVoip, VoipCallbackT *pCallback, void *pUserData);

// set transcrived text received notification callback
DIRTYCODE_API void VoipSetTranscribedTextReceivedCallback(VoipRefT *pVoip, VoipTranscribedTextReceivedCallbackT *pCallback, void *pUserData);

// connect to a peer
DIRTYCODE_API int32_t VoipConnect(VoipRefT *pVoip, int32_t iConnID, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionId);

// disconnect from peer
DIRTYCODE_API void VoipDisconnect(VoipRefT *pVoip, int32_t iConnID, uint32_t uAddress);

// disconnect from peer 2
DIRTYCODE_API void VoipDisconnect2(VoipRefT *pVoip, int32_t iConnID, uint32_t uAddress, int32_t bSendDiscMsg);

// remove a peer from connectionlist (does not contract list)
DIRTYCODE_API void VoipRemove(VoipRefT *pVoip, int32_t iConnID);

// return information about peer connection
DIRTYCODE_API int32_t VoipConnStatus(VoipRefT *pVoip, int32_t iConnID);

// return information about remote peer
DIRTYCODE_API int32_t VoipRemoteUserStatus(VoipRefT *pVoip, int32_t iConnID, int32_t iRemoteUserIndex);

// return information about local hardware state on a per-user basis
DIRTYCODE_API int32_t VoipLocalUserStatus(VoipRefT *pVoip, int32_t iLocalUserIndex);

// select which peers to accept voice from
DIRTYCODE_API void VoipSpeaker(VoipRefT *pVoip, uint32_t uConnMask);

// select which peers to send voice to
DIRTYCODE_API void VoipMicrophone(VoipRefT *pVoip, uint32_t uConnMask);

// return status
DIRTYCODE_API int32_t VoipStatus(VoipRefT *pVoip, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);   //do any of the controls need user index?

// set control options
DIRTYCODE_API int32_t VoipControl(VoipRefT *pVoip, int32_t iControl, int32_t iValue, void *pValue);                 //do any of the controls need user index?

// get Voip profile data (see voipdef.h for possible profile statistics to get)
DIRTYCODE_API int32_t VoipGetProfileStat(VoipRefT *pVoip, VoipProfStatE eProfStat);

// get amount of time elapsed in last profiling cycle
DIRTYCODE_API int32_t VoipGetProfileTime(VoipRefT *pVoip);

// set speaker output callback (only available on some platforms)
DIRTYCODE_API void VoipSpkrCallback(VoipRefT *pVoip, VoipSpkrCallbackT *pCallback, void *pUserData);

// pick a channel
DIRTYCODE_API int32_t VoipSelectChannel(VoipRefT *pVoip, int32_t iUserIndex, int32_t iChannel, VoipChanModeE eMode);

// go back to not using channels
DIRTYCODE_API void VoipResetChannels(VoipRefT *pVoip, int32_t iUserIndex);

#ifdef __cplusplus
}
#endif

//@}

#endif // _voip_h


/*H*************************************************************************************************/
/*!

    \File    voip.h

    \Description
        Main include for Voice Over IP module.

    \Notes
        None.

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

/*** Include files ********************************************************************************/

#include "voipdef.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// prepare voip for use
VoipRefT *VoipStartup(int32_t iMaxPeers, int32_t iMaxLocal);

// return pointer to current module state
VoipRefT *VoipGetRef(void);

// release all voip resources //$$ DEPRECATE in V9
void VoipShutdown(VoipRefT *pVoip);

// release all voip resources
void VoipShutdown2(VoipRefT *pVoip, uint32_t uShutdownFlags);

// set local user (must be called before connect)
void VoipSetLocalUser(VoipRefT *pVoip, const char *pUserID, uint32_t bRegister);

// set event notification callback
void VoipSetEventCallback(VoipRefT *pVoip, VoipCallbackT *pCallback, void *pUserData);

// connect to a peer
int32_t VoipConnect(VoipRefT *pVoip, int32_t iConnID, const char *pUserID, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionId);

// disconnect from peer
void VoipDisconnect(VoipRefT *pVoip, int32_t iConnID, uint32_t uAddress);

// remove a peer from connectionlist (does not contract list)
void VoipRemove(VoipRefT *pVoip, int32_t iConnID);

// return information about remote peer
int32_t VoipRemote(VoipRefT *pVoip, int32_t iConnID);

// return information about local hardware state
int32_t VoipLocal(VoipRefT *pVoip);

// select which peers to accept voice from
void VoipSpeaker(VoipRefT *pVoip, uint32_t uConnMask);

// select which peers to send voice to
void VoipMicrophone(VoipRefT *pVoip, uint32_t uConnMask);

// return status
int32_t VoipStatus(VoipRefT *pVoip, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// set control options
int32_t VoipControl(VoipRefT *pVoip, int32_t iControl, int32_t iValue, void *pValue);

// get Voip profile data (see voipdef.h for possible profile statistics to get)
int32_t VoipGetProfileStat(VoipRefT *pVoip, VoipProfStatE eProfStat);

// get amount of time elapsed in last profiling cycle
int32_t VoipGetProfileTime(VoipRefT *pVoip);

// set speaker output callback (only available on some platforms)
void VoipSpkrCallback(VoipRefT *pVoip, VoipSpkrCallbackT *pCallback, void *pUserData);

// update function used to drive optional callbacks
void VoipUpdate(VoipRefT *pVoip);

#ifdef __cplusplus
}
#endif

#endif // _voip_h


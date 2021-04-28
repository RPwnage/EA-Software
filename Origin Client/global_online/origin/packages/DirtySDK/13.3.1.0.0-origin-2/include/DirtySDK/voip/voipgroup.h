/*H*************************************************************************************************/
/*!

    \File    voipgroup.h

    \Description
        A module that handles logical grouping of voip connections. Each module that wants to deal
        with a block of voip connections uses a voip group. This reduces the singleton nature
        of voip.h

    \Notes
        None.

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 12/18/07 (jrainy) First Version
*/
/*************************************************************************************************H*/

#ifndef _voip_group_h
#define _voip_group_h

/*** Include files ********************************************************************************/
#include "DirtySDK/voip/voip.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

//! callback event types
typedef enum ConnSharingCbTypeE
{
    VOIPGROUP_CBTYPE_CONNSUSPEND,    //<! it is time to suspend the connection with VoipGroupSuspend()
    VOIPGROUP_CBTYPE_CONNRESUME      //<! it is time to resume the connection with VoipGroupResume()
} ConnSharingCbTypeE;

//! opaque module ref
typedef struct VoipGroupRefT VoipGroupRefT;

//! event callback function prototype
typedef void (ConnSharingCallbackT)(VoipGroupRefT *pVoip, ConnSharingCbTypeE eCbType, int32_t iConnId, void *pUserData);

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//! allocates a VoipGroup
VoipGroupRefT *VoipGroupCreate(void);

//! deallocates a VoipGroup
void VoipGroupDestroy(VoipGroupRefT *pRef);

// set event notification callback
void VoipGroupSetEventCallback(VoipGroupRefT *pVoipGroup, VoipCallbackT *pCallback, void *pUserData);

// set connection sharing event notification callback - only use if for scenarios involving several voip groups concurrently
void VoipGroupSetConnSharingEventCallback(VoipGroupRefT *pVoipGroup, ConnSharingCallbackT *pCallback, void *pUserData);

//! VoipConnect, through a VoipGroup
int32_t VoipGroupConnect(VoipGroupRefT *pVoipGroup, int32_t iConnID, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientID, uint32_t uSessionID);

//! resume a connection that is in supended state at the voip group level
void VoipGroupResume(VoipGroupRefT *pVoipGroup, int32_t iConnId, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionID);

//! VoipDisconnect, through a VoipGroup
void VoipGroupDisconnect(VoipGroupRefT *pVoipGroup, int32_t iConnID);

//! suspend an active connection while we rely on another low-level voip connection belonging to another voip collection
void VoipGroupSuspend(VoipGroupRefT *pVoipGroup, int32_t iConnId);

//! mute/unmute a client, specified by ConnId
int32_t VoipGroupMuteByConnId(VoipGroupRefT *pVoipGroup, uint32_t iConnId, uint8_t uMute);

//! mute/unmute a client, specified by ClientId
int32_t VoipGroupMuteByClientId(VoipGroupRefT *pVoipGroup, uint32_t iClientId, uint8_t uMute);

//! whether a client is muted, by ConnId
uint8_t VoipGroupIsMutedByConnId(VoipGroupRefT *pVoipGroup, uint32_t iConnId);

//! whether a client is muted, by ClientId
uint8_t VoipGroupIsMutedByClientId(VoipGroupRefT *pVoipGroup, uint32_t iClientId);

//! VoipRemote, through a VoipGroup
int32_t VoipGroupRemote(VoipGroupRefT *pVoipGroup, int32_t iConnID);

//! VoipLocal, through a VoipGroup
int32_t VoipGroupLocal(VoipGroupRefT *pVoipGroup);

//! return status
int32_t VoipGroupStatus(VoipGroupRefT *pVoipGroup, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

//! set control options
int32_t VoipGroupControl(VoipGroupRefT *pVoipGroup, int32_t iControl, int32_t iValue, void *pValue);

#ifdef __cplusplus
}
#endif

#endif // _voip_h


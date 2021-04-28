/*H********************************************************************************/
/*!
    \File voipcommon.h

    \Description
        Cross-platform voip data types and private functions.

    \Copyright
        Copyright (c) 2009 Electronic Arts Inc.

    \Version 12/02/2009 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _voipcommon_h
#define _voipcommon_h

/*** Include files ****************************************************************/

#include "voipconnection.h"
#include "voipheadset.h"
#include "DirtySDK/voip/voip.h"


/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/


typedef struct VoipCommonRefT
{
    VoipConnectionlistT Connectionlist;     //!< virtual connection list
    VoipHeadsetRefT     *pHeadset;          //!< voip headset manager

    VoipCallbackT       *pCallback;         //!< voip event callback (optional)
    void                *pUserData;         //!< user data for voip event callback

    NetCritT            ThreadCrit;         //!< critical section used for thread synchronization
    uint8_t             bApplyChannelConfig; //!< apply channel configs on the next voip thread pass
    
    uint32_t            uLastFrom;          //!< most recent from status, used for callback tracking
    uint32_t            uLastHsetStatus;    //!< most recent headset status, used for callback tracking
    uint32_t            uLastLocalStatus[VOIP_MAXLOCALUSERS];   //!< most recent local status, used for callback tracking
    uint32_t            uPortHeadsetStatus; //!< bitfield indicating which ports have headsets
    
    uint8_t             bUseDefaultSharedChannelConfig; //!< whether we use the default shared config or not (only supported on xbox one)
    uint8_t             bPrivileged;                    //!< whether we are communication-privileged or not
    uint8_t             _pad[2];

    uint32_t            uRemoteChannelSelection[VOIP_MAX_LOW_LEVEL_CONNS][VOIP_MAXLOCALUSERS_EXTENDED]; //<! coded, per remote user, channel selection
    uint32_t            uLocalChannelSelection[VOIP_MAXLOCALUSERS_EXTENDED];     //<! coded, per local user, channel selection

    int32_t             iDefaultSharedChannels[VOIP_MAX_CONCURRENT_CHANNEL];   //<! use to store the default shared channel config
    uint32_t            uDefaultSharedModes[VOIP_MAX_CONCURRENT_CHANNEL];      //<! use to store the default shared channel mode
    int32_t             iCustomSharedChannels[VOIP_MAX_CONCURRENT_CHANNEL];    //<! use to store the custom shared channel config
    uint32_t            uCustomSharedModes[VOIP_MAX_CONCURRENT_CHANNEL];       //<! use to store the custom shared channel mode
    int32_t             iLocalChannels[VOIP_MAXLOCALUSERS_EXTENDED][VOIP_MAX_CONCURRENT_CHANNEL];         //<! channels selected locally
    uint32_t            uLocalModes[VOIP_MAXLOCALUSERS_EXTENDED][VOIP_MAX_CONCURRENT_CHANNEL];            //<! channel modes selected locally

    uint32_t            uChanRecvMask;  //<! the recv mask associated with the channel selection
    uint32_t            uChanSendMask;  //<! the send mask associated with the channel selection

    uint32_t            uUserSpkrValue;
    uint32_t            uUserMicrValue;

} VoipCommonRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// get voip ref
VoipRefT *VoipCommonGetRef(void);

// handle common startup functionality
VoipRefT *VoipCommonStartup(int32_t iMaxPeers, int32_t iVoipRefSize, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetStatusCbT *pStatusCb, int32_t iData);

// handle common shutdown functionality
void VoipCommonShutdown(VoipCommonRefT *pVoipCommon);

// update status flags
void VoipCommonUpdateRemoteStatus(VoipCommonRefT *pVoipCommon);

// handle common status selectors
int32_t VoipCommonStatus(VoipCommonRefT *pVoipCommon, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// handle common control selectors
int32_t VoipCommonControl(VoipCommonRefT *pVoipCommon, int32_t iControl, int32_t iValue, void *pValue);

// add (OR) uAddMask into *pMask
void VoipCommonAddMask(uint32_t *pMask, uint32_t uAddMask, const char *pMaskName);

// del (&~) uDelMask from *pMask
void VoipCommonDelMask(uint32_t *pMask, uint32_t uDelMask, const char *pMaskName);

// set *pMask = uNewMask
void VoipCommonSetMask(uint32_t *pMask, uint32_t uNewMask, const char *pMaskName);

int32_t VoipCommonSelectChannel(VoipCommonRefT *pVoip, int32_t iUserIndex, int32_t iChannel, VoipChanModeE eMode);

void VoipCommonResetChannels(VoipCommonRefT *pVoip, int32_t iUserIndex);

void VoipCommonApplyChannelConfig(VoipCommonRefT *pVoip);

void VoipCommonProcessChannelChange(VoipCommonRefT *pVoipCommon, int32_t iConnID);


#ifdef __cplusplus
}
#endif

#endif // _voipcommon_h


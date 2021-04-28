/*H********************************************************************************/
/*!
    \File voipheadset.h

    \Description
        VoIP headset manager.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/31/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _voipheadset_h
#define _voipheadset_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! voip headset update status
typedef enum VoipHeadsetStatusUpdateE
{
    VOIP_HEADSET_STATUS_INPUT = 1,
    VOIP_HEADSET_STATUS_OUTPUT,
    VOIP_HEADSET_STATUS_INOUT
} VoipHeadsetStatusUpdateE;

//! opaque headset module ref
typedef struct VoipHeadsetRefT VoipHeadsetRefT;

//! mic data ready callback function prototype
typedef void (VoipHeadsetMicDataCbT)(const void *pVoiceData, int32_t iDataSize, const void *pMetaData, int32_t iMetaDataSize, uint32_t uLocalIdx, uint8_t uSendSeqn, void *pUserData);

//! transcribed text ready callback function prototype
typedef void (VoipHeadsetTextDataCbT)(const wchar_t *pTranscribedText, uint32_t uLocalIdx, void *pUserData);

//! headset status change callback
typedef void (VoipHeadsetStatusCbT)(int32_t iLocalUserIndex, uint32_t bStatus, VoipHeadsetStatusUpdateE eUpdate, void *pUserData);


/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create headset module
VoipHeadsetRefT *VoipHeadsetCreate(int32_t iMaxChannels, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetTextDataCbT *pTextDataCb, VoipHeadsetStatusCbT *pStatusCb, void *pCbUserData, int32_t iData);

// destroy headset module
void VoipHeadsetDestroy(VoipHeadsetRefT *pHeadset);

// callback to handle receiving voice data
void VoipHeadsetReceiveVoiceDataCb(VoipUserT *pRemoteUsers, VoipMicrInfoT *pMicrInfo, uint8_t *pPacketData, void *pUserData);

// callback to handle registering of a new user
void VoipHeadsetRegisterUserCb(VoipUserT *pRemoteUser, uint32_t bRegister, void *pUserData);

// process function to update headset(s)
void VoipHeadsetProcess(VoipHeadsetRefT *pHeadset, uint32_t uFrameCount);

// set play/rec volumes (-1 to keep current setting)
void VoipHeadsetSetVolume(VoipHeadsetRefT *pHeadset, int32_t iPlayVol, uint32_t iRecVol);

// control function
int32_t VoipHeadsetControl(VoipHeadsetRefT *pHeadset, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// status function
int32_t VoipHeadsetStatus(VoipHeadsetRefT *pHeadset, int32_t iSelect, int32_t iData, void *pBuf, int32_t iBufSize);

// set speaker output callback (only available on some platforms)
void VoipHeadsetSpkrCallback(VoipHeadsetRefT *pHeadset, VoipSpkrCallbackT *pCallback, void *pUserData);

// display transcribed text in system UI
int32_t VoipHeadsetDisplayTranscribedText(VoipHeadsetRefT *pHeadset, VoipUserT *pOriginator, const wchar_t *pText);

#ifdef __cplusplus
};
#endif

#endif // _voipheadset_h


/*H********************************************************************************/
/*!
    \File voipconduit.h

    \Description
        VoIP data packet definitions.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 07/29/2004 (jbrookes) Split from voipheadset
    \Version 12/01/2009 (jbrookes) voipchannel->voipconduit; avoid name clash with new API
*/
/********************************************************************************H*/

#ifndef _voipconduit_h
#define _voipconduit_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! conduit manager module state
typedef struct VoipConduitRefT VoipConduitRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

// create conduit manager
VoipConduitRefT *VoipConduitCreate(int32_t iMaxConduits);

// set conduit mixer
void VoipConduitSetMixer(VoipConduitRefT *pConduitRef, VoipMixerRefT *pMixerRef);

// destroy conduit manager
void VoipConduitDestroy(VoipConduitRefT *pConduitRef);

// receive voice data
void VoipConduitReceiveVoiceData(VoipConduitRefT *pConduitRef, VoipUserT *pRemoteUser, const uint8_t *pData, int32_t iDataSize);

// register a remote user
void VoipConduitRegisterUser(VoipConduitRefT *pConduitRef, VoipUserT *pRemoteUser, uint32_t bRegister);

#endif // _voipconduit_h


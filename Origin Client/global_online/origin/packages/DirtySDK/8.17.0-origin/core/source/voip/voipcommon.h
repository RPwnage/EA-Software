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

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct VoipCommonRefT
{
    VoipConnectionlistT Connectionlist;     //!< virtual connection list
    VoipHeadsetRefT     *pHeadset;          //!< voip headset manager
    
    VoipCallbackT       *pCallback;         //!< voip event callback (optional)
    void                *pUserData;         //!< user data for event callback

    NetCritT            ThreadCrit;         //!< critical section used for thread synchronization
    
    uint32_t            uLastFrom;          //!< most recent from status, used for callback tracking
    uint32_t            uLastHsetStatus;    //!< most recent headset status, used for callback tracking
    uint32_t            uLastLocalStatus;   //!< most recent local status, used for callback tracking
    uint32_t            uPortHeadsetStatus; //!< bitfield indicating which ports have headsets
    
    #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
    int32_t             iActivePort;        //!< which port has the active headset
    int32_t             iLockedPort;        //!< locked port, or negative if port locking not active
    BOOL                bPrivileged;        //!< whether we are communication-privileged or not
    BOOL                bFriendsPrivileged; //!< whether we are communication-priveleged to friends or not
    #else
    uint8_t             bPrivileged;        //!< whether we are communication-privileged or not
    uint8_t             _pad[3];
    #endif
} VoipCommonRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// get voip ref
VoipRefT *VoipCommonGetRef(void);

// handle common startup functionality
VoipRefT *VoipCommonStartup(int32_t iMaxPeers, int32_t iMaxLocal, int32_t iVoipRefSize, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetStatusCbT *pStatusCb);

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


#ifdef __cplusplus
}
#endif

#endif // _voipcommon_h


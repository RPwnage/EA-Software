/*H********************************************************************************/
/*!
    \File voippriv.h

    \Description
        Header for private VoIP functions.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/17/2004 (jbrookes) First Version
    \Version 1.1 11/18/2008 (mclouatre) Adding user data concept to mem group support
*/
/********************************************************************************H*/

#ifndef _voippriv_h
#define _voippriv_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#if DIRTYCODE_PLATFORM == DIRTYCODE_PC
#define VOIP_PC_OUTPUT_DEVICE_ACTIVE    (0)
#define VOIP_PC_OUTPUT_DEVICE_INACTIVE  (1)
#define VOIP_PC_INPUT_DEVICE_ACTIVE     (2)
#define VOIP_PC_INPUT_DEVICE_INACTIVE   (3)
#endif

/*** Macros ***********************************************************************/

//! copy VoipUserTs
#define VOIP_CopyUser(_pUser1, _pUser2) (strnzcpy((_pUser1)->strUserId, (_pUser2)->strUserId, sizeof((_pUser1)->strUserId)))

//! clear VoipUserT
#define VOIP_ClearUser(_pUser1)         (memset((_pUser1)->strUserId, 0, sizeof((_pUser1)->strUserId)))

//! return if VoipUserT is NULL or not
#define VOIP_NullUser(_pUser1)          ((_pUser1)->strUserId[0] == '\0')

//! compare VoipUserTs
#define VOIP_SameUser(_pUser1, _pUser2) (!strcmp((_pUser1)->strUserId, (_pUser2)->strUserId))

/*** Type Definitions *************************************************************/

typedef struct VoipTimerT
{
    uint32_t uHi;
    uint32_t uLo;
} VoipTimerT;

typedef struct VoipUserT
{
    char strUserId[20];
} VoipUserT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// high-resolution timer
uint32_t VoipTimer(VoipTimerT *pTimer);

// difference two timers (a-b)
int32_t VoipTimerDiff(const VoipTimerT *pTimerA, const VoipTimerT *pTimerB);

// get current memgroup
void VoipMemGroupQuery(int32_t *pMemGroup, void **ppMemGroupUserData);

// set current memgroup
void VoipMemGroupSet(int32_t iMemGroup, void *pMemGroupUserData);

#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON // xenon-specific functions in voipprivxenon.c
// convert from textual XUID to binary form
void VoipXuidFromUser(XUID *pXuid, VoipUserT *pVoipUser);
#endif // XENON

#ifdef __cplusplus
};
#endif

#endif // _voippriv_h


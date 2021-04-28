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

#if defined(DIRTYCODE_PC)
#define VOIP_THREAD_SLEEP_DURATION      (20) //! transmission interval in miliseconds
#endif

/*** Macros ***********************************************************************/

//! copy VoipUserTs
#define VOIP_CopyUser(_pUser1, _pUser2) (ds_memcpy_s(_pUser1, sizeof(*_pUser1), _pUser2, sizeof(*_pUser2)))

//! clear VoipUserT
#define VOIP_ClearUser(_pUser1)         (memset(_pUser1, 0, sizeof(*_pUser1)))

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

#ifdef DIRTYCODE_XBOXONE
typedef struct VoipUserExT
{
    VoipUserT user;                       // important:  must remain first field in this struct.
    uint8_t aSerializedUserBlob[48];      // data blob that can be sent over the network for recreating the user object on other consoles
} VoipUserExT;
#endif

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

#if defined(DIRTYCODE_XENON)// xenon-specific functions in voipprivxenon.c
// convert from textual XUID to binary form
void VoipXuidFromUser(XUID *pXuid, VoipUserT *pVoipUser);
#endif // XENON

#if defined(DIRTYCODE_XBOXONE)// xboxone-specific functions in voipprivxboxone.c
// convert from textual XUID to binary form
void VoipXuidFromUser(uint64_t *pXuid, VoipUserT *pVoipUser);
#endif // XBOXONE

#ifdef __cplusplus
};
#endif

#endif // _voippriv_h


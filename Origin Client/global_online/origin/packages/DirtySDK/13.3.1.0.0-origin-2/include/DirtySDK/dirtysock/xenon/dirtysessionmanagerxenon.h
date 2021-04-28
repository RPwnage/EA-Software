/*H*************************************************************************************/
/*!
    \File dirtysessionmanagerxenon.h

    \Description
        DirtySessionManager handles the session creation on 360. Used to create and join 
        a session. Offers mechanism to encode and decode the session, and does some 
        session flags management

    \Notes

    \Copyright
        Copyright (c) Electronic Arts 2003-2007.  ALL RIGHTS RESERVED.

    \Version 1.0 11/03/2003 (jbrookes) First Version
    \Version 2.0 11/04/2007 (jbrookes) Removed from ProtoMangle namespace, cleanup
    \Version 2.2 10/26/2009 (mclouatre) Renamed from core/include/dirtysessionmanager.h to core/include/xenon/dirtysessionmanagerxenon.h
*/
/*************************************************************************************H*/

#ifndef _dirtysessionmanager_h
#define _dirtysessionmanager_h

/*!
\Module DirtySock
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/dirtysock/dirtyaddr.h"

/*** Defines ***************************************************************************/
#define DIRTYSESSIONMANAGER_FLAG_PUBLICSLOT          (1)
#define DIRTYSESSIONMANAGER_FLAG_PRIVATESLOT         (2)


/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct DirtySessionManagerRefT DirtySessionManagerRefT;
#if DIRTYCODE_DEBUG
//! true skill values struct
typedef struct DirtySessionManagerTrueSkillRefT      //!< true skill struct used for debugging
{
    double dMu;
    double dSigma;
} DirtySessionManagerTrueSkillRefT;
#endif



/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// allocate module state and prepare for use
DirtySessionManagerRefT *DirtySessionManagerCreate(DirtyAddrT *pHostAddr, DirtyAddrT *pLocalAddr);

// destroy the module and release its state
void DirtySessionManagerDestroy(DirtySessionManagerRefT *pRef);

// give time to module to do its thing (should be called periodically to allow module to perform work)
void DirtySessionManagerUpdate(DirtySessionManagerRefT *pRef);

// $$ TODO: V9, change the DirtySessionManagerConnect, DirtySessionManagerComplete names.
// We currently do complete first (resolve address) and then connect (add to session)
// Join one (or many) remote player(s) by specifying the session and type of slot to use
void DirtySessionManagerConnect(DirtySessionManagerRefT *pRef, const char **pSessID, uint32_t *uSlot, uint32_t uCount);

// get result
int32_t DirtySessionManagerComplete(DirtySessionManagerRefT *pRef, int32_t *pAddr, const char* pMachineAddress);

// dirtysessionmanager control
int32_t DirtySessionManagerControl(DirtySessionManagerRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue);

// get module status based on selector
int32_t DirtySessionManagerStatus(DirtySessionManagerRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize);

// encode XSESSION_INFO
void DirtySessionManagerEncodeSession(char *pBuffer, int32_t iBufSize, const void *pSessionInfo);

// decode XSESSION_INFO
void DirtySessionManagerDecodeSession(void *pSessionInfo, const char *pBuffer);

// create the session (previously the 'sess' control selector)
int32_t DirtySessionManagerCreateSess(DirtySessionManagerRefT *pRef, uint32_t bRanked, uint32_t *uUserFlags, const char *pSession, DirtyAddrT *pLocalAddrs);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtysessionmanager_h



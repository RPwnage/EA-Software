/*H********************************************************************************/
/*!
    \File dirtyauthxenon.h

    \Description
        Xenon XAuth wrapper module

    \Copyright
        Copyright 2012 Electronic Arts Inc.

    \Version 04/05/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _dirtyauthxenon_h
#define _dirtyauthxenon_h

/*!
\Moduledef DirtyAuthXenon DirtyAuthXenon
\Modulemember DirtySock
*/
//@{

/*** Include files *********************************************************************/

#include <xtl.h>

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#define DIRTYAUTH_SUCCESS       (0)     //!< success result
#define DIRTYAUTH_AVAILABLE     (0)     //!< alternative name for SUCCESS
#define DIRTYAUTH_PENDING       (-1)    //!< operation pending
#define DIRTYAUTH_ERROR         (-2)    //!< error
#define DIRTYAUTH_MEMORY        (-3)    //!< memory error
#define DIRTYAUTH_NOTSTARTED    (-4)    //!< not started
#define DIRTYAUTH_NOTFOUND      (-5)    //!< not found
#define DIRTYAUTH_XAUTHERR      (-6)    //!< xauth error code

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! module state ref
typedef struct DirtyAuthRefT DirtyAuthRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
DirtyAuthRefT *DirtyAuthCreate(uint8_t bSecureBypass);

// destroy the module
void DirtyAuthDestroy(DirtyAuthRefT *pDirtyAuth);

// get a token
int32_t DirtyAuthGetToken(int32_t iUserIndex, const char *pHost, uint8_t bForceNew);

// check for token availability
int32_t DirtyAuthCheckToken(const char *pHost,int32_t *pTokenLen, uint8_t *pToken);

// DirtyAuthStatus
int32_t DirtyAuthStatus(DirtyAuthRefT* pRef, int32_t iKind, int32_t iData, void *pBuf, int32_t iBufSize, const char *pHost);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtyauthxenon_h


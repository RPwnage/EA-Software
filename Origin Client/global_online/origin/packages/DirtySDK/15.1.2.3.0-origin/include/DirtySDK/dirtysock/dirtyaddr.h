/*H********************************************************************************/
/*!
    \File dirtyaddr.h

    \Description
        Definition for portable address type.

    \Copyright
        Copyright (c) Electronic Arts 2004

    \Version 1.0 04/07/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _dirtyaddr_h
#define _dirtyaddr_h

/*!
\Moduledef DirtyAddr DirtyAddr
\Modulemember DirtySock
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#define DIRTYADDR_MACHINEADDR_MAXLEN    (127)
#define DIRTYADDR_MACHINEADDR_MAXSIZE   (DIRTYADDR_MACHINEADDR_MAXLEN + 1)

/*** Macros ***********************************************************************/

//! compare two opaque addresses for equality  (same=TRUE, different=FALSE)
#define DirtyAddrCompare(_pAddr1, _pAddr2)  (!strcmp((_pAddr1)->strMachineAddr, (_pAddr2)->strMachineAddr))

/*** Type Definitions *************************************************************/

//! opaque address type
typedef struct DirtyAddrT
{
    char strMachineAddr[DIRTYADDR_MACHINEADDR_MAXSIZE];
} DirtyAddrT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//! convert a DirtyAddrT to native format
DIRTYCODE_API uint32_t DirtyAddrToHostAddr(void *pOutput, int32_t iBufLen, const DirtyAddrT *pAddr);

//! convert a host-format address to native format
DIRTYCODE_API uint32_t DirtyAddrFromHostAddr(DirtyAddrT *pAddr, const void *pInput);

//! get local address in DirtyAddr form
DIRTYCODE_API uint32_t DirtyAddrGetLocalAddr(DirtyAddrT *pAddr);

#if defined(DIRTYCODE_XBOXONE)
//! get Xbox One extended info into dirtyaddr
DIRTYCODE_API void DirtyAddrGetInfoXboxOne(const DirtyAddrT *pDirtyAddr, void *pXuid, void *pSecureDeviceAddressBlob, int32_t *pBlobSize);

//! set Xbox One extended info into dirtyaddr
DIRTYCODE_API void DirtyAddrSetInfoXboxOne(DirtyAddrT *pDirtyAddr, const void *pXuid, const void *pSecureDeviceAddressBlob, int32_t iBlobSize);

#endif

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtyaddr_h


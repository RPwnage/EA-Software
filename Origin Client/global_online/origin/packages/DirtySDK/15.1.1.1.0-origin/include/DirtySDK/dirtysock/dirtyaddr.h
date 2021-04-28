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

#if defined(DIRTYCODE_PS3)
#include <np.h>
#endif

/*** Defines **********************************************************************/

#define DIRTYADDR_MACHINEADDR_MAXLEN    (127)
#define DIRTYADDR_MACHINEADDR_MAXSIZE   (DIRTYADDR_MACHINEADDR_MAXLEN + 1)

/*** Macros ***********************************************************************/

//! compare two opaque addresses for equality  (same=TRUE, different=FALSE)
#if !defined(DIRTYCODE_PS3)
#define DirtyAddrCompare(_pAddr1, _pAddr2)  (!strcmp((_pAddr1)->strMachineAddr, (_pAddr2)->strMachineAddr))
#endif

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

#if defined(DIRTYCODE_PS3)
//! get PS3 info from dirtyaddr
DIRTYCODE_API void DirtyAddrGetInfoPS3(const DirtyAddrT *pDirtyAddr, SceNpId *pId, uint32_t *pLocalAddr, uint32_t *pExtAddr);

//! set PS3 info into dirtyaddr
DIRTYCODE_API void DirtyAddrSetInfoPS3(DirtyAddrT *pAddr, const SceNpId *pId, uint32_t uLocalAddr, uint32_t uExtAddr);

//! compare two dirtyaddrs (only compare external addresses)
DIRTYCODE_API int32_t DirtyAddrCompare(DirtyAddrT *pAddr1, DirtyAddrT *pAddr2);
#endif

#if defined(DIRTYCODE_XENON)
//! get Xenon extended info into dirtyaddr
void DirtyAddrGetInfoXenon(const DirtyAddrT *pDirtyAddr, void *pXuid, void *pXnAddr);

//! set Xenon extended info into dirtyaddr
void DirtyAddrSetInfoXenon(DirtyAddrT *pDirtyAddr, const void *pXuid, const void *pXnAddr);
#elif defined(DIRTYCODE_XBOXONE)
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


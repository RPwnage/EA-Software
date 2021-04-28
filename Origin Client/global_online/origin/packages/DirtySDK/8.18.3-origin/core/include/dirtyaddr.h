/*H********************************************************************************/
/*!
    \File dirtyaddr.h

    \Description
        Definition for portable address type.

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 04/07/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _dirtyaddr_h
#define _dirtyaddr_h

/*** Include files ****************************************************************/

#if defined(DIRTYCODE_PS3) || defined(DIRTYCODE_PSP2)
#include <np.h>
#endif

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

//! compare two opaque addresses for equality  (same=TRUE, different=FALSE)
#if !defined(DIRTYCODE_PS3) && !defined(DIRTYCODE_PSP2)
#define DirtyAddrCompare(_pAddr1, _pAddr2)  (!strcmp((_pAddr1)->strMachineAddr, (_pAddr2)->strMachineAddr))
#endif

/*** Type Definitions *************************************************************/

//! opaque address type
typedef struct DirtyAddrT
{
    char strMachineAddr[64];
} DirtyAddrT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//! convert a DirtyAddrT to native format
uint32_t DirtyAddrToHostAddr(void *pOutput, int32_t iBufLen, const DirtyAddrT *pAddr);

//! convert a host-format address to native format
uint32_t DirtyAddrFromHostAddr(DirtyAddrT *pAddr, const void *pInput);

//! get local address in DirtyAddr form
uint32_t DirtyAddrGetLocalAddr(DirtyAddrT *pAddr);

#if defined(DIRTYCODE_PS3) || defined(DIRTYCODE_PSP2)
//! get PS3 info from dirtyaddr
void DirtyAddrGetInfoPS3(const DirtyAddrT *pDirtyAddr, SceNpId *pId, uint32_t *pLocalAddr, uint32_t *pExtAddr);

//! set PS3 info into dirtyaddr
void DirtyAddrSetInfoPS3(DirtyAddrT *pAddr, const SceNpId *pId, uint32_t uLocalAddr, uint32_t uExtAddr);

//! compare two dirtyaddrs (only compare external addresses)
int32_t DirtyAddrCompare(DirtyAddrT *pAddr1, DirtyAddrT *pAddr2);
#endif

#if defined(DIRTYCODE_XENON)
//! get Xenon extended info into dirtyaddr
void DirtyAddrGetInfoXenon(const DirtyAddrT *pDirtyAddr, void *pXuid, void *pXnAddr);

//! set Xenon extended info into dirtyaddr
void DirtyAddrSetInfoXenon(DirtyAddrT *pDirtyAddr, const void *pXuid, const void *pXnAddr);
#endif


#ifdef __cplusplus
}
#endif

#endif // _dirtyaddr_h


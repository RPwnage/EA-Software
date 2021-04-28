/*H********************************************************************************/
/*!
    \File dirtyaddrps3.c

    \Description
        Opaque address functions.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Notes
        A PS3 DirtyAddrT contains the internal and external addresses, as well
        as the NpID.
        
        NpID-internal-external-opt-reserved

    \Version 1.0 09/01/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dirtysock.h"
#include "dirtyaddr.h"
#include "netconn.h"
#include "lobbytagfield.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _DirtyAddrSetBinary

    \Description
        Wrapper to use tagfield to set binary or binary7 data

    \Input *pBuf        - [out] 
    \Input iBufSize     - size of output buffer
    \Input *pInput      - input data to encode
    \Input uInputSize   - size of input data
    \Input bBinary7     - true=use binary7 encoding
    
    \Output
        int32_t        - length of encoded string data

    \Version 1.0 09/01/2006 (jbrookes) First Version
*/
/********************************************************************************F*/
static int32_t _DirtyAddrSetBinary(char *pBuf, int32_t iBufSize, const uint8_t *pInput, uint32_t uInputSize, uint32_t bBinary7)
{
    char strTemp[64];
    TagFieldClear(strTemp, sizeof(strTemp));
    if (bBinary7)
    {
        TagFieldSetBinary7(strTemp, sizeof(strTemp), "T", pInput, uInputSize);
    }
    else
    {
        TagFieldSetBinary(strTemp, sizeof(strTemp), "T", pInput, uInputSize);
    }
    TagFieldGetString(TagFieldFind(strTemp, "T"), pBuf, iBufSize, "");
    return(strlen(pBuf));
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function DirtyAddrToHostAddr

    \Description
        Convert from DirtyAddrT to native format.

    \Input *pOutput - [out] storage for native format address
    \Input iBufLen  - length of output buffer
    \Input *pAddr   - source address to convert
    
    \Output
        uint32_t    - number of characters consumed from input, or zero on failure

    \Notes
        On common platforms, a DirtyAddrT is a string-encoded network-order address
        field.  This function converts that input to a binary-encoded host-order
        address.

    \Version 1.0 04/09/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
uint32_t DirtyAddrToHostAddr(void *pOutput, int32_t iBufLen, const DirtyAddrT *pAddr)
{
    uint32_t uAddress;
    
    // make sure our buffer is big enough
    if (iBufLen < (signed)sizeof(uAddress))
    {
        NetPrintf(("dirtyaddr: output buffer too small\n"));
        return(0);
    }

    // extract external address from DirtyAddr    
    DirtyAddrGetInfoPS3(pAddr, NULL, NULL, &uAddress);

    // convert from network order to host order
    uAddress = SocketNtohl(uAddress);

    // copy to output
    memcpy(pOutput, &uAddress, sizeof(uAddress));
    return(9);
}

/*F********************************************************************************/
/*!
    \Function DirtyAddrFromHostAddr

    \Description
        Convert from native format to DirtyAddrT.

    \Input *pAddr       - [out] storage for output DirtyAddrT
    \Input *pInput      - pointer to native format address

    \Output
        uint32_t        - TRUE if successful, else FALSE

    \Notes
        On common platforms, a DirtyAddrT is a string-encoded network-order address
        field.  This function converts a binary-encoded host-order address to that
        format.

    \Version 1.0 04/09/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
uint32_t DirtyAddrFromHostAddr(DirtyAddrT *pAddr, const void *pInput)
{
    uint32_t uExtAddr;
    memcpy(&uExtAddr, pInput, sizeof(uExtAddr));
    DirtyAddrSetInfoPS3(pAddr, NULL, 0, uExtAddr);
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function DirtyAddrGetLocalAddr

    \Description
        Get the local address in DirtyAddr form.

    \Input *pAddr   - [out] storage for output DirtyAddrT
    
    \Output
        uint32_t    - TRUE if successful, else FALSE

    \Version 1.0 09/29/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
uint32_t DirtyAddrGetLocalAddr(DirtyAddrT *pAddr)
{
    uint32_t uLocalAddr = NetConnStatus('addr', 0, NULL, 0);
    DirtyAddrFromHostAddr(pAddr, &uLocalAddr);
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function DirtyAddrGetInfoPS3

    \Description
        Extract PS3-specific fields from the given DirtyAddr

    \Input *pDirtyAddr  - dirtyaddr to decode
    \Input *pId         - [out] storage for decoded NpId
    \Input *pIntAddr    - [out] storage for decoded internal address
    \Input *pExtAddr    - [out] storage for decoded external address
    
    \Output
        None.

    \Notes
        Any undesired outputs may be specified as NULL

    \Version 1.0 09/01/2006 (jbrookes) First Version
*/
/********************************************************************************F*/
void DirtyAddrGetInfoPS3(const DirtyAddrT *pDirtyAddr, SceNpId *pId, uint32_t *pIntAddr, uint32_t *pExtAddr)
{
    const char *pAddr = pDirtyAddr->strMachineAddr;
    int32_t iIndex;
    uint32_t uTemp;
    SceNpId NpId;
    
    // allow NULL inputs
    if (pId == NULL) pId = &NpId;
    if (pIntAddr == NULL) pIntAddr = &uTemp;
    if (pExtAddr == NULL) pExtAddr = &uTemp;

    // clear output data
    memset(pId, 0, sizeof(*pId));
    
    // extract NpOnlineId
    for (iIndex = 0; pAddr[iIndex] != '\0' && pAddr[iIndex] != '$'; iIndex++)
    {
        pId->handle.data[iIndex] = pAddr[iIndex];
    }
    // extract local address
    TagFieldGetBinary(pAddr+iIndex, pIntAddr, sizeof(*pIntAddr));
    iIndex += 9;    
    // extract external address
    TagFieldGetBinary(pAddr+iIndex, pExtAddr, sizeof(*pExtAddr));
    iIndex += 9;
    // extract id.opt
    TagFieldGetBinary(pAddr+iIndex, pId->opt, sizeof(pId->opt));
    iIndex += 11;
    // extract id.reserved
    TagFieldGetBinary(pAddr+iIndex, pId->reserved, sizeof(pId->reserved));
}

/*F********************************************************************************/
/*!
    \Function DirtyAddrSetInfoPS3

    \Description
        Set PS3-specific fields into the given DirtyAddr

    \Input *pDirtyAddr  - [out] storage for dirtyaddr
    \Input *pId         - NpId to encode
    \Input uIntAddr     - internal address to encode
    \Input uExtAddr     - external address to encode
    
    \Output
        None.

    \Version 1.0 09/01/2006 (jbrookes) First Version
*/
/********************************************************************************F*/
void DirtyAddrSetInfoPS3(DirtyAddrT *pDirtyAddr, const SceNpId *pId, uint32_t uIntAddr, uint32_t uExtAddr)
{
    char *pAddr = pDirtyAddr->strMachineAddr;
    int32_t iIndex;
    SceNpId NpId;

    // allow NULL NpId input
    if (pId == NULL)
    {
        memset(&NpId, 0, sizeof(NpId));
        pId = &NpId;
    }

    // clear output data
    memset(pDirtyAddr, 0, sizeof(*pDirtyAddr));
    
    // set NpOnlineId
    strnzcpy(pAddr, pId->handle.data, sizeof(*pDirtyAddr));
    iIndex = strlen(pAddr);
    // set local address
    iIndex += _DirtyAddrSetBinary(pAddr+iIndex, sizeof(*pDirtyAddr)-iIndex, (uint8_t *)&uIntAddr, sizeof(uIntAddr), FALSE);
    // set external address
    iIndex += _DirtyAddrSetBinary(pAddr+iIndex, sizeof(*pDirtyAddr)-iIndex, (uint8_t *)&uExtAddr, sizeof(uExtAddr), FALSE);
    // set id.opt
    iIndex += _DirtyAddrSetBinary(pAddr+iIndex, sizeof(*pDirtyAddr)-iIndex, pId->opt, sizeof(pId->opt), TRUE);
    // set id.reserved
    iIndex += _DirtyAddrSetBinary(pAddr+iIndex, sizeof(*pDirtyAddr)-iIndex, pId->reserved, sizeof(pId->reserved), TRUE);
}

/*F********************************************************************************/
/*!
    \Function DirtyAddrCompare

    \Description
        Compare two DirtyAddrTs (external address component only)

    \Input *pAddr1  - first address
    \Input *pAddr2  - second address
    
    \Output
        int32_t     - one=same, zero=different

    \Version 1.0 09/05/2006 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t DirtyAddrCompare(DirtyAddrT *pAddr1, DirtyAddrT *pAddr2)
{
    uint32_t iAddr1, iAddr2;
    
    DirtyAddrGetInfoPS3(pAddr1, NULL, NULL, &iAddr1);
    DirtyAddrGetInfoPS3(pAddr2, NULL, NULL, &iAddr2);

    return(!(iAddr1-iAddr2));
}


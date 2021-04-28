/*H********************************************************************************/
/*!
    \File dirtyaddrxenon.c

    \Description
        Platform-specific address functions.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 04/09/2004 (jbrookes) first version
    \Version 1.1 10/26/2009 (mclouatre) renamed from xbox/dirtyaddrxbox.c to xenon/dirtyaddrxenon.c
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <xtl.h>
#include <xonline.h>
#include <string.h>

#include "dirtysock.h"
#include "dirtyaddr.h"
#include "netconn.h"
#include "lobbytagfield.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

static unsigned char _DirtyAddr_aHexMap[16] = "0123456789abcdef";

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ishex

    \Description
        Determine if the given character is a hex digit.

    \Input c    - character to check
    
    \Output
        int32_t     - TRUE if c is a hex digit, else FALSE

    \Version 1.0 05/13/2005 (jbrookes) First Version
*/
/********************************************************************************F*/
static int32_t _ishex(int32_t c)
{
    if (((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')) || ((c >= '0') && (c <= '9')))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

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

    \Input *pOutput     - [out] storage for native format address
    \Input iBufLen      - length of output buffer
    \Input *pAddr       - source address to convert
    
    \Output
        uint32_t    - TRUE if successful, else FALSE

    \Notes
        On Xbox, a DirtyAddrT is a string-encoded network-order XUID.qwUserID
        field.  This function converts that input to a binary-encoded host-order
        XUID.qwUserID.

    \Version 1.0 04/09/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
uint32_t DirtyAddrToHostAddr(void *pOutput, int32_t iBufLen, const DirtyAddrT *pAddr)
{
    const char *pAddrText;
    ULONGLONG qwUserID;
    int32_t iChar;
    unsigned char cNum;
    
    // make sure our buffer is big enough
    if (iBufLen < sizeof(qwUserID))
    {
        NetPrintf(("dirtyaddr: output buffer too small\n"));
        return(FALSE);
    }
    
    // convert from string to qword
    for (iChar = 0, qwUserID = 0, pAddrText = &pAddr->strMachineAddr[1]; _ishex(pAddrText[iChar]); iChar++)
    {
        // make room for next entry
        qwUserID <<= 4;

        // add in character
        cNum = pAddrText[iChar];
        cNum -= ((cNum >= 0x30) && (cNum < 0x3a)) ? '0' : 0x57;
        qwUserID |= (ULONGLONG)cNum;
    }

    // copy to output
    memcpy(pOutput, &qwUserID, sizeof(qwUserID));
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function DirtyAddrFromHostAddr

    \Description
        Convert from native format to DirtyAddrT.

    \Input *pAddr       - [out] storage for output DirtyAddrT
    \Input *pInput      - pointer to native format address

    \Output
        uint32_t    - TRUE if successful, else FALSE

    \Notes
        On Xbox, a DirtyAddrT is a string-encoded network-order XUID.qwUserID
        field.  This function converts that abinary-encoded host-order
        XUID.qwUserID to a DirtyAddrT.

    \Version 1.0 04/09/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
uint32_t DirtyAddrFromHostAddr(DirtyAddrT *pAddr, const void *pInput)
{
    ULONGLONG qwUserID;
    int32_t iChar;
    
    // make sure output buffer is okay
    if ((pInput == NULL) || (((uint32_t)pInput & 0x3) != 0))
    {
        return(FALSE);
    }

    // get userID
    qwUserID = *(ULONGLONG *)pInput;

    // set up address
    memset(pAddr, 0, sizeof(*pAddr));
    for (iChar = (sizeof(qwUserID) * 2); iChar >= 1; iChar--)
    {
        pAddr->strMachineAddr[iChar] = _DirtyAddr_aHexMap[qwUserID & 0xf];
        qwUserID >>= 4;
    }
    pAddr->strMachineAddr[0] = '$';
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function DirtyAddrGetLocalAddr

    \Description
        Get the local address in DirtyAddr form.

    \Input *pAddr       - [out] storage for output DirtyAddrT
    
    \Output
        uint32_t    - TRUE if successful, else FALSE

    \Version 1.0 09/29/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
uint32_t DirtyAddrGetLocalAddr(DirtyAddrT *pAddr)
{
    XUID Xuid;
    if (NetConnStatus('xuid', 0, &Xuid, sizeof(Xuid)) < 0)
    {
        return(FALSE);
    }
    DirtyAddrFromHostAddr(pAddr, &Xuid);
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function DirtyAddrGetInfoXenon

    \Description
        Extract Xenon-specific fields from the given DirtyAddr

    \Input *pDirtyAddr  - dirtyaddr to decode
    \Input *pXuid       - [out] storage for decoded XUID
    \Input *pXnAddr     - [out] storage for decoded XNADDR
    
    \Output
        None.

    \Notes
        Any undesired outputs may be specified as NULL

    \Version 1.0 06/20/2008 (jbrookes) First Version
*/
/********************************************************************************F*/
void DirtyAddrGetInfoXenon(const DirtyAddrT *pDirtyAddr, void *pXuid, void *pXnAddr)
{
    const char *pAddr = pDirtyAddr->strMachineAddr;
    XUID *_pXuid = (XUID *)pXuid, TempXuid;
    XNADDR *_pXnAddr = (XNADDR *)pXnAddr, TempXnAddr;
    int32_t iIndex;
    
    // allow NULL inputs
    if (_pXuid == NULL) _pXuid = &TempXuid;
    if (_pXnAddr == NULL) _pXnAddr = &TempXnAddr;

    // extract XUID
    DirtyAddrToHostAddr(_pXuid, sizeof(*_pXuid), pDirtyAddr);
    iIndex = 17;
        
    // extract XNADDR
    TagFieldGetBinary(pAddr+iIndex, _pXnAddr, sizeof(*_pXnAddr));
}

/*F********************************************************************************/
/*!
    \Function DirtyAddrSetInfoXenon

    \Description
        Set Xenon-specific fields into the given DirtyAddr

    \Input *pDirtyAddr  - [out] storage for dirtyaddr
    \Input *pXuid       - XUID to encode
    \Input *pXnAddr     - XNADDR to encode
    
    \Output
        None.

    \Version 1.0 06/20/2008 (jbrookes) First Version
*/
/********************************************************************************F*/
void DirtyAddrSetInfoXenon(DirtyAddrT *pDirtyAddr, const void *pXuid, const void *pXnAddr)
{
    char *pAddr = pDirtyAddr->strMachineAddr;
    XUID *_pXuid = (XUID *)pXuid;
    XNADDR *_pXnAddr = (XNADDR *)pXnAddr;
    int32_t iIndex;

    // clear output data
    memset(pDirtyAddr, 0, sizeof(*pDirtyAddr));
    
    // set XUID
    DirtyAddrFromHostAddr(pDirtyAddr, _pXuid);
    
    iIndex = strlen(pAddr);
    // append XNADDR
    iIndex += _DirtyAddrSetBinary(pAddr+iIndex, sizeof(*pDirtyAddr)-iIndex, (uint8_t *)_pXnAddr, sizeof(*_pXnAddr), TRUE);
}


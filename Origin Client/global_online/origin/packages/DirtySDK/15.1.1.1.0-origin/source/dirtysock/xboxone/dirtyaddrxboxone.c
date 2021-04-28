/*H********************************************************************************/
/*!
    \File dirtyaddrxboxone.c

    \Description
        Platform-specific address functions.

    \Copyright
        Copyright (c) Electronic Arts 2013. ALL RIGHTS RESERVED.

    \Version 04/17/2013 (mclouatre)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtysock/dirtyaddr.h"
#include "DirtySDK/util/tagfield.h"

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

    \Version 05/13/2005 (jbrookes)
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

    \Input *pBuf        - [out] output buffer
    \Input iBufSize     - size of output buffer
    \Input *pInput      - input data to encode
    \Input uInputSize   - size of input data
    \Input bBinary7     - true=use binary7 encoding

    \Output
        int32_t        - length of encoded string data

    \Version 09/01/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DirtyAddrSetBinary(char *pBuf, int32_t iBufSize, const uint8_t *pInput, uint32_t uInputSize, uint32_t bBinary7)
{
    char strTemp[128];
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
    return((signed)strlen(pBuf));
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
        On Xbox One, a DirtyAddrT is a string-encoded network-order XUID.qwUserID
        field.  This function converts that input to a binary-encoded host-order
        XUID.qwUserID.

    \Version 04/09/2004 (jbrookes)
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
    ds_memcpy(pOutput, &qwUserID, sizeof(qwUserID));
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
        On Xbox One, a DirtyAddrT is a string-encoded network-order XUID.qwUserID
        field.  This function converts that abinary-encoded host-order
        XUID.qwUserID to a DirtyAddrT.

    \Version 04/09/2004 (jbrookes)
*/
/********************************************************************************F*/
uint32_t DirtyAddrFromHostAddr(DirtyAddrT *pAddr, const void *pInput)
{
    ULONGLONG qwUserID;
    int32_t iChar;

    // make sure output buffer is okay
    if ((pInput == NULL) || (((uint64_t)pInput & 0x3) != 0))
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

    \Version 09/29/2004 (jbrookes)
*/
/********************************************************************************F*/
uint32_t DirtyAddrGetLocalAddr(DirtyAddrT *pAddr)
{
    ULONGLONG Xuid;
    if (NetConnStatus('xuid', 0, &Xuid, sizeof(Xuid)) < 0)
    {
        return(FALSE);
    }
    DirtyAddrFromHostAddr(pAddr, &Xuid);
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function DirtyAddrGetInfoXboxOne

    \Description
        Extract Xbox One-specific fields from the given DirtyAddr

    \Input *pDirtyAddr               - dirtyaddr to decode
    \Input *pXuid                    - [out] storage for decoded XUID
    \Input *pSecureDeviceAddressBlob - [out] storage for decode SecureDeviceAddress blob
    \Input *pBlobSize                - [out] size of decoded SecureDeviceAddress blob (in bytes)

    \Notes
        Any undesired outputs may be specified as NULL

    \Version 04/17/2013 (mclouatre)
*/
/********************************************************************************F*/
void DirtyAddrGetInfoXboxOne(const DirtyAddrT *pDirtyAddr, void *pXuid, void *pSecureDeviceAddressBlob, int32_t *pBlobSize)
{
    const char *pAddr = pDirtyAddr->strMachineAddr;
    ULONGLONG *_pXuid = (ULONGLONG *)pXuid, TempXuid;
    uint8_t *_pSecureDeviceAddressBlob = (uint8_t *)pSecureDeviceAddressBlob;
    uint8_t aTempBlob[512];
    int32_t *_pBlobSize = (int32_t *)pBlobSize, iBlobSize;
    int32_t iIndex;

    // allow NULL inputs
    if (_pXuid == NULL) _pXuid = &TempXuid;
    if (_pSecureDeviceAddressBlob == NULL) _pSecureDeviceAddressBlob = aTempBlob;
    if (_pBlobSize == NULL) _pBlobSize = &iBlobSize;

    // extract XUID
    DirtyAddrToHostAddr(_pXuid, sizeof(*_pXuid), pDirtyAddr);
    iIndex = 17;

    // extract SecureDeviceAddress blob size
    TagFieldGetBinary(pAddr+iIndex, _pBlobSize, sizeof(*_pBlobSize));
    iIndex += 9;

    // extract SecureDeviceAddress blob
    TagFieldGetBinary(pAddr+iIndex, _pSecureDeviceAddressBlob, *_pBlobSize);
}

/*F********************************************************************************/
/*!
    \Function DirtyAddrSetInfoXboxOne

    \Description
        Set Xbox One-specific fields into the given DirtyAddr

    \Input *pDirtyAddr               - [out] storage for dirtyaddr
    \Input *pXuid                    - XUID to encode
    \Input *pSecureDeviceAddressBlob - SecureDeviceAddress blob to encode
    \Input iBlobSize                 - size of Secure device address code (in bytes)

    \Version 04/17/2013 (mclouatre)
*/
/********************************************************************************F*/
void DirtyAddrSetInfoXboxOne(DirtyAddrT *pDirtyAddr, const void *pXuid, const void *pSecureDeviceAddressBlob, int32_t iBlobSize)
{
    char *pAddr = pDirtyAddr->strMachineAddr;
    ULONGLONG *_pXuid = (ULONGLONG *)pXuid;
    int32_t iIndex;

    // clear output data
    memset(pDirtyAddr, 0, sizeof(*pDirtyAddr));

    // set XUID
    DirtyAddrFromHostAddr(pDirtyAddr, _pXuid);

    iIndex = (signed)strlen(pAddr);

    // append SecureDeviceAddres blob size
    iIndex += _DirtyAddrSetBinary(pAddr+iIndex, sizeof(*pDirtyAddr)-iIndex, (uint8_t *)&iBlobSize, sizeof(iBlobSize), FALSE);

    // append SecureDeviceAddres blob
    iIndex += _DirtyAddrSetBinary(pAddr+iIndex, sizeof(*pDirtyAddr)-iIndex, (uint8_t *)pSecureDeviceAddressBlob, iBlobSize, TRUE);
}


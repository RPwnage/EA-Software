/*H*************************************************************************************/
/*!

    \File    pcisp.c

    \Description
        High-level module for reading the PC account login info from the registry.

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2005.  ALL RIGHTS RESERVED.

    \Version 1.0 09/15/05 (dclark) First Version
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/util/tagfield.h"
#include "pcisp.h"

/*** Defines ***************************************************************************/

#define PCISP_PC_ACCOUNT_NAME_LEN   16
#define PCISP_PC_PASSWORD_LEN       12
#define PCISP_PASSWORD_BUFFER       128

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! private module data
struct PCIspT
{
    //! buffer to format tagfield info for return to caller
    char                strRetnBuf[128];

    //! buffer to save logon account
    char                strAccountName[PCISP_PC_ACCOUNT_NAME_LEN + 1];

    //! buffer to save password 
    char                strPassword[PCISP_PC_PASSWORD_LEN + 1];

    //! alert list specified by application
    const PCIspAlertT   *pAlertList;

    //! module memory group
    int32_t              iMemGroup;
    void                 *pMemGroupUserData;

    //! current screen context
    PCIspContextE       eCurContext;

    //! current module status
    PCIspStatusE        eCurStatus;

    //! current alert, if it is an alert condition
    PCIspAlertE         eCurAlert;

    //! last error returned when reading registry entry
    LONG                iError;                 
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

/*** Private Functions *****************************************************************/

/*F*************************************************************************************************/
/*!
    \Function    _PCIspHandleError

    \Description
        Handles the alert error conditions.

    \Input iError - the error result from reading the account info from the registry

    \Output
        None.

    \Version 1.0 09/16/05 (dclark) First Version
*/
/*************************************************************************************************F*/
void _PCIspHandleError(PCIspT *pIsp, LONG iError)
{
    // print the error code for debugging
    NetPrintf(("pcisp: handling alert; error code: %d", iError));

    // set status to alert
    pIsp->eCurStatus    = PCISP_STATUS_ALERT;
    pIsp->iError        = iError;

    if (ERROR_FILE_NOT_FOUND == iError)
    {
        pIsp->eCurAlert = PCISP_ALERT_MISSING;
    }
    else if (ERROR_ACCESS_DENIED == iError)
    {
        pIsp->eCurAlert = PCISP_ALERT_ACCESS;
    }
    else
    {
        // all other errors are currently registry issues
        pIsp->eCurAlert = PCISP_ALERT_REGERROR;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _PCIspDecryptPassword

    \Description
        Unencrypts an encrypted password

    \Input pCryptPass - the encrypted password
    \Input dwUserSize - the length of the encrypted data
    \Input strBuf     - [Out] the decrypted password
    \Input iBufSize   - Size of strBuf

    \Output
        None.

    \Notes
        Assuming the passed-in string is null-terminated.

    \Version 1.0 09/16/05 (dclark) First Version
*/
/*************************************************************************************************F*/
void _PCIspDecryptPassword(int32_t * pCryptPass, DWORD dwUserSize, char * strBuf, int32_t iBufSize)
{
    char strUnEncryptedPass[PCISP_PASSWORD_BUFFER];
    uint32_t iPos = 0;

    for(iPos=0; iPos < dwUserSize - 1; ++iPos) 
    { 
        int32_t iChar = pCryptPass[iPos];
        if(iChar == 0)
            break;
        iChar -= iPos * 2;
        strUnEncryptedPass[iPos] = iChar;
    }
    
    // make sure password string ends with a NULL
    strUnEncryptedPass[iPos] = 0;

    // save the decrypted password in the out buffer
    strncpy(strBuf, strUnEncryptedPass, iBufSize);
}

/*F*************************************************************************************************/
/*!
    \Function    _PCIspReadRegKey

    \Description
        Attempt to read a value from the registry.

    \Input *pIsp        - pointer to module state from PCIspCreate()
    \Input hRegKey      - the registry key to read 
    \Input strSubKey    - the registry subkey to read 
    \Input strValue     - the registry value to read
    \Input strBuf       - [out] character buffer to hold key value
    \Input iBufSize     - size of strBuf

    \Output
        LONG     - result of registry operation.  O == success; non-zero == error

    \Version 1.0 09/16/05 (dclark) First Version
*/
/*************************************************************************************************F*/
LONG _PCIspReadRegKey(PCIspT *pIsp, HKEY hRegKey, const char * strSubKey, const char *strValue,  char * strBuf, int32_t iBufSize)
{
    HKEY    hKey;
    LONG    iErrorCode;
    DWORD   dwUserSize;
    LPBYTE  pBuffer;
    LONG    iResult = ERROR_SUCCESS;

    iErrorCode = RegOpenKeyEx(hRegKey,strSubKey, 0, KEY_READ, &hKey);

    if (ERROR_SUCCESS == iErrorCode)
    {
        // find out how large a buffer we need
        BYTE cBuffer;      // buffer can't be NULL...
        dwUserSize = 0;
        iErrorCode = RegQueryValueEx( hKey, strValue, NULL, NULL, &cBuffer, &dwUserSize );

        pBuffer = NULL;

        if (ERROR_MORE_DATA == iErrorCode)
        {
            // allocate the new buffer size
            pBuffer = DirtyMemAlloc(dwUserSize, PCISP_MEMID, pIsp->iMemGroup, pIsp->pMemGroupUserData);

            // read the value again
            iErrorCode = RegQueryValueEx( hKey, strValue, NULL, NULL, pBuffer, &dwUserSize );      
        }

        if (ERROR_SUCCESS == iErrorCode)
        {
            if (strncmp(strValue, "pass", sizeof(strlen(strValue))) == 0)
            {
                // decrypt the password and store it in the out param
                _PCIspDecryptPassword((int32_t *) pBuffer, dwUserSize/ sizeof(int32_t) , strBuf, iBufSize);
            }
            else
            {
                // copy value from registry to our buffer
                memset(strBuf, 0, iBufSize);
                strncpy(strBuf, (const char *)pBuffer, iBufSize);
            }
        }
        else
        {
            iResult = iErrorCode;
        }

        // free pBuffer if necessary
        if (NULL != pBuffer)
        {
            DirtyMemFree(pBuffer, PCISP_MEMID, pIsp->iMemGroup, pIsp->pMemGroupUserData);
            pBuffer = NULL;
        }

        // close the registry key
        RegCloseKey(hKey);
    }
    else
    {
        iResult = iErrorCode;
    }

    return(iResult);
}

/*F*************************************************************************************************/
/*!
    \Function    _PCIspLoadAccountInfo

    \Description
        Attempt to read the account info out of the registry.

    \Input *pIsp - pointer to module state from PCIspCreate()

    \Output
        None.

    \Version 1.0 09/15/05 (dclark) First Version
*/
/*************************************************************************************************F*/
void _PCIspLoadAccountInfo(PCIspT *pIsp)
{
    LONG iResult;
    char strBuf[80];

    // attempt to load the account name
    iResult = _PCIspReadRegKey(pIsp, HKEY_LOCAL_MACHINE, "SOFTWARE\\Electronic Arts\\Shared", "user", strBuf, sizeof(strBuf));

    if (ERROR_SUCCESS == iResult)
    {
        // save account name in PCIsp ref
        strncpy(pIsp->strAccountName, strBuf, sizeof(pIsp->strAccountName)-1);
    }
    else
    {
        _PCIspHandleError(pIsp, iResult);
        return;
    }

    // attempt to load the password
    iResult = _PCIspReadRegKey(pIsp, HKEY_LOCAL_MACHINE, "SOFTWARE\\Electronic arts\\Shared", "pass", strBuf, sizeof(strBuf));

    if (ERROR_SUCCESS == iResult)
    {
        // save account name in PCIsp ref
        strncpy(pIsp->strPassword, strBuf, sizeof(pIsp->strPassword)-1);
    }
    else
    {
        _PCIspHandleError(pIsp, iResult);
        return;
    }

    // We've read account data successfully, so GOTO DONE context
    pIsp->eCurStatus = PCISP_STATUS_GOTO;
    pIsp->eCurContext = PCISP_CONTEXT_DONE;

}

/*** Public Functions ******************************************************************/

/*F*************************************************************************************************/
/*!
    \Function    PCIspCreate

    \Description
        Create the module state.

    \Input *pAlertList - pointer to an alert table

    \Output
        PCIspT *       - module reference pointer that must be passed as input to all other functions

    \Version 1.0 09/15/05 (dclark) First Version
*/
/*************************************************************************************************F*/
PCIspT *PCIspCreate(const PCIspAlertT *pAlertList)
{
    PCIspT *pIsp;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // alloc ref
    if ( (pIsp = DirtyMemAlloc(sizeof(*pIsp), PCISP_MEMID, iMemGroup, pMemGroupUserData)) == NULL )
    {
        return(NULL);
    }

    // initialize ref
    memset(pIsp, 0, sizeof(*pIsp));
    pIsp->iMemGroup = iMemGroup;
    pIsp->pMemGroupUserData = pMemGroupUserData;
    pIsp->pAlertList  = pAlertList;
    pIsp->eCurContext = PCISP_CONTEXT_INACTIVE;
    pIsp->eCurStatus  = PCISP_STATUS_IDLE;

    // Initialize the NetConn connection
    NetConnConnect(NULL, NULL, 0);

    // return ref to caller
    return(pIsp);
}

/*F*************************************************************************************************/
/*!
    \Function    PCIspDestroy

    \Description
        Destroy the module state.

    \Input *pIsp - pointer to module state from PCIspCreate()

    \Output
        None.

    \Version 1.0 09/15/05 (dclark) First Version
*/
/*************************************************************************************************F*/
void PCIspDestroy(PCIspT *pIsp)
{
    // destroy module memory
    DirtyMemFree(pIsp, PCISP_MEMID, pIsp->iMemGroup, pIsp->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function    PCIspParams

    \Description
        Get parameters for current screen.

    \Input *pIsp        - pointer to module state from PCIspCreate()
    \Input eContext     - screen context

    \Output
        const char *    - pointer to TagField encoded data record
        
    \Notes
       Params may be requested in the following contexts:
       
    \verbatim
       PCISP_CONTEXT_DONE
          NAME=<Account name>             (string)
          PASSWORD=<Account password>     (string)
    \endverbatim
      
    \Version 1.0 09/15/05 (dclark) First Version
*/
/*************************************************************************************************F*/
const char *PCIspParams(PCIspT *pIsp, PCIspContextE eContext)
{
    // clear return buffer
    TagFieldClear(pIsp->strRetnBuf, sizeof(pIsp->strRetnBuf));

    // make sure the context is correct
    if(eContext != pIsp->eCurContext)
    {
        NetPrintf(("pcisp: attempt to get params in context %d, when current context is %d\n", eContext, pIsp->eCurContext));
        return(pIsp->strRetnBuf);
    }

    // handle param return in DONE context
    if (eContext == PCISP_CONTEXT_DONE)
    {
        // format tagfield with account name and password
        TagFieldSetString(pIsp->strRetnBuf, sizeof(pIsp->strRetnBuf), "NAME", pIsp->strAccountName);
        TagFieldSetString(pIsp->strRetnBuf, sizeof(pIsp->strRetnBuf), "PASSWORD", pIsp->strPassword);
    }

    return(pIsp->strRetnBuf);
}

/*F*************************************************************************************************/
/*!
    \Function    PCIspAlert

    \Description
        Return current alert message reference (call in response to PCISP_STATUS_ALERT)

    \Input *pIsp        - pointer to module state from PCIspCreate()
    \Input eContext     - screen context

    \Output
        PCIspAlertT *  - pointer to alert record

    \Version 1.0 09/15/05 (dclark) First Version
*/
/*************************************************************************************************F*/
const PCIspAlertT *PCIspAlert(PCIspT *pIsp, PCIspContextE eContext)
{
    const PCIspAlertT *pAlert = NULL;

    // make sure the screen is in the right context
    if (eContext != pIsp->eCurContext)
    {
        NetPrintf(("pcisp: attempt to get alert in context %d, when current context is %d\n", eContext, pIsp->eCurContext));
        return(pAlert);
    }

    // handle alert in loadaccount context
    if (eContext == PCISP_CONTEXT_LOADACCOUNT)
    {
        pAlert = &pIsp->pAlertList[pIsp->eCurAlert];
    }

    // clear the status
    pIsp->eCurStatus = PCISP_STATUS_IDLE;

    // return alert to caller
    return(pAlert);
}

/*F*************************************************************************************************/
/*!
    \Function    PCIspGetAlertEnum

    \Description
        Return the current alert message enum.

    \Input *pIsp        - pointer to module state from PCIspCreate()
    \Input *pAlert      - pointer to alert to get enum for

    \Output
        PCIspAlertE   - enum of the given alert

    \Version 1.0 09/15/05 (dclark) First Version
*/
/*************************************************************************************************F*/
PCIspAlertE PCIspGetAlertEnum(PCIspT *pIsp, const PCIspAlertT *pAlert)
{
    PCIspAlertE eAlert;

    eAlert = (PCIspAlertE)(pAlert - pIsp->pAlertList);
    return(eAlert);
}

/*F*************************************************************************************************/
/*!
    \Function    PcIspGetStatus

    \Description
        Get the current status.

    \Input *pIsp        - pointer to module state from PCIspCreate()
    \Input eContext     - screen context

    \Output
        PCIspStatusE    - current module status

    \Version 1.0 09/15/05 (dclark) First Version
*/
/*************************************************************************************************F*/
PCIspStatusE PCIspGetStatus(PCIspT * pIsp, PCIspContextE eContext)
{
    PCIspStatusE eStatus;

    eStatus = (pIsp->eCurContext == eContext) ? pIsp->eCurStatus : PCISP_STATUS_GOTO;
    return(eStatus);
}

/*F*************************************************************************************************/
/*!
    \Function    PCIspSetContext

    \Description
        Set a different context.

    \Input *pIsp        - pointer to module state from PCIspCreate()
    \Input eContext     - screen context

    \Output
        None.

    \Version 1.0 09/15/05 (dclark) First Version
*/
/*************************************************************************************************F*/
void PCIspSetContext(PCIspT *pIsp, PCIspContextE eContext)
{
    if (eContext >= PCISP_NUMCONTEXTS)
    {
        NetPrintf(("pcisp: ignoring unknown context"));
        return;
    }

    // set current context
    pIsp->eCurContext = eContext;

    // if the new context is PCISP_CONTEXT_LOADACCOUNT, try to load the account
    if (PCISP_CONTEXT_LOADACCOUNT == eContext)
    {
        _PCIspLoadAccountInfo(pIsp);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    PCIspGetContext

    \Description
        Get the current context.

    \Input *pIsp      - pointer to module state from PCIspCreate()

    \Output
        PCIspContextE - current module context

    \Version 1.0 09/15/05 (dclark) First Version
*/
/*************************************************************************************************F*/
PCIspContextE PCIspGetContext(PCIspT *pIsp)
{
    return(pIsp->eCurContext);
}

/*F*************************************************************************************************/
/*!
    \Function    PCIspGetError

    \Description
        Returns the last registry error code.

    \Input *pIsp      - pointer to module state from PCIspCreate()

    \Output
        LONG - last error code; 0 = no error;

    \Version 1.0 09/19/05 (dclark) First Version
*/
/*************************************************************************************************F*/
int32_t PCIspGetError(PCIspT *pIsp)
{
    return(pIsp->iError);
}
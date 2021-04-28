/*H********************************************************************************/
/*!
    \File dirtyauthxenon.c

    \Description
        Xenon XAuth wrapper module

    \Copyright
        Copyright 2012 Electronic Arts Inc.

    \Version 04/05/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#ifdef DIRTYSDK_XHTTP_ENABLED

#include <xtl.h>
#include <winsockx.h>
#include <xauth.h>
#include <xhttp.h>

#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/dirtysock/xenon/dirtyauthxenon.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

//! token request structure
typedef struct DirtyAuthTokenT
{
    struct DirtyAuthTokenT *pNext;  //!< pointer to next token in token list
    XOVERLAPPED XAuthOverlapped;    //!< overlapped structure used while async process is pending
    RELYING_PARTY_TOKEN *pToken;    //!< pointer to xauth token
    char strHost[255];              //!< hostname token is for
    uint8_t bValid;                 //!< token is valid (async process is complete and successful)
    DWORD dwAuthStat;              //!< First Party Auth Token Error Code
} DirtyAuthTokenT;

// module state
struct DirtyAuthRefT
{
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group

    DirtyAuthTokenT *pTokenList;    //!< list of tokens
    NetCritT TokenCrit;             //!< token list critical section

    uint8_t bXAuthStarted;          //!< has xauth been started?
};

/*** Variables ********************************************************************/

//! dirtyauth module ref
static DirtyAuthRefT *_DirtyAuth_pRef = NULL;


/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _DirtyAuthNewToken

    \Description
        Allocate a new token

    \Input *pDirtyAuth      - module state
    \Input *pHost           - pointer to hostname

    \Output
        DirtyAuthTokenT *   - pointer to token, or NULL if not found
        
    \Version 04/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static DirtyAuthTokenT *_DirtyAuthNewToken(DirtyAuthRefT *pDirtyAuth, const char *pHost)
{
    DirtyAuthTokenT *pDirtyToken;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // token entries use netconn's memgroup and memgroup pointer as the memory can exceed the lifespan of the DirtyAuth module
    iMemGroup = NetConnStatus('mgrp', 0, &pMemGroupUserData, sizeof(pMemGroupUserData));

    if ((pDirtyToken = (DirtyAuthTokenT *)DirtyMemAlloc(sizeof(*pDirtyToken), DIRTYAUTH_MEMID, iMemGroup, pMemGroupUserData)) != NULL)
    {
        memset(pDirtyToken, 0, sizeof(*pDirtyToken));
        ds_strnzcpy(pDirtyToken->strHost, pHost, sizeof(pDirtyToken->strHost));
    }

    return(pDirtyToken);
}

/*F********************************************************************************/
/*!
    \Function _DirtyAuthDelToken

    \Description
        Delete specified token

    \Input *pDirtyToken - token to delete from the token list

    \Version 04/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyAuthDelToken(DirtyAuthTokenT *pDirtyToken)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // token entries use netconn's memgroup and memgroup pointer as the memory can exceed the lifespan of the DirtyAuth module
    iMemGroup = NetConnStatus('mgrp', 0, &pMemGroupUserData, sizeof(pMemGroupUserData));

    if (pDirtyToken->pToken != NULL)
    {
        XAuthFreeToken(pDirtyToken->pToken);
    }
    DirtyMemFree(pDirtyToken, DIRTYAUTH_MEMID, iMemGroup, pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function _DirtyAuthAddToken

    \Description
        Add token to token list

    \Input *pDirtyAuth  - module state
    \Input *pDirtyToken - token to add to the token list

    \Output
        int32_t         - 0=not available, 1=available
        
    \Version 04/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyAuthAddToken(DirtyAuthRefT *pDirtyAuth, DirtyAuthTokenT *pDirtyToken)
{
    // add token at front of list
    pDirtyToken->pNext = pDirtyAuth->pTokenList;
    pDirtyAuth->pTokenList = pDirtyToken;
}

/*F********************************************************************************/
/*!
    \Function _DirtyAuthRemToken

    \Description
        Remove token from token list

    \Input *pDirtyAuth      - module state
    \Input *pHost           - pointer to hostname

    \Output
        DirtyAuthTokenT *   - pointer to token, or NULL if not found
        
    \Version 04/11/2013 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyAuthRemToken(DirtyAuthRefT *pDirtyAuth, DirtyAuthTokenT *pDirtyToken)
{
    DirtyAuthTokenT **ppFindToken;
    for (ppFindToken = &pDirtyAuth->pTokenList; *ppFindToken != NULL; ppFindToken = &((*ppFindToken)->pNext))
    {
        if (*ppFindToken == pDirtyToken)
        {
            *ppFindToken = pDirtyToken->pNext;
            break;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyAuthFindToken

    \Description
        See if we have a token for the specified host

    \Input *pDirtyAuth      - module state
    \Input *pHost           - pointer to hostname

    \Output
        DirtyAuthTokenT *   - pointer to token, or NULL if not found
        
    \Version 04/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static DirtyAuthTokenT *_DirtyAuthFindToken(DirtyAuthRefT *pDirtyAuth, const char *pHost)
{
    DirtyAuthTokenT *pDirtyToken;
    for (pDirtyToken = pDirtyAuth->pTokenList; pDirtyToken != NULL; pDirtyToken = pDirtyToken->pNext)
    {
        if (!ds_stricmp(pDirtyToken->strHost, pHost))
        {
            return(pDirtyToken);
        }
    }
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _DirtyAuthCheckToken

    \Description
        Check to see if a token has completed processing

    \Input *pDirtyToken - pointer to dirty token entry
    \Input *pTokenLen   - filled with token length when return code is DIRTYAUTH_SUCCESS (caller can pass NULL here)
    \Input *pToken      - filled with token when return code is DIRTYAUTH_SUCCESS (caller can pass NULL here)

    \Output
        int32_t         - DIRTYAUTH_* response code

    \Version 04/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DirtyAuthCheckToken(DirtyAuthTokenT *pDirtyToken, int32_t *pTokenLen, uint8_t *pToken)
{
    int32_t iRetCode = DIRTYAUTH_SUCCESS;

    // see if we've already resolved this token
    if (!pDirtyToken->bValid)
    {
        // fast check to see if async operation has completed
        if (XHasOverlappedIoCompleted(&pDirtyToken->XAuthOverlapped) == FALSE)
        {
            iRetCode = DIRTYAUTH_PENDING;
        }
        else
        {
            // get the result
            if ((pDirtyToken->dwAuthStat = XGetOverlappedExtendedError(&pDirtyToken->XAuthOverlapped)) == ERROR_SUCCESS)
            {
                NetPrintf(("dirtyauthxenon: acquired token for host '%s'\n", pDirtyToken->strHost));
                pDirtyToken->bValid = TRUE;
            }
            else
            {
                NetPrintf(("dirtyauthxenon: failed to acquire token for host '%s' (err=%s)\n", pDirtyToken->strHost, DirtyErrGetName(pDirtyToken->dwAuthStat)));
                iRetCode = DIRTYAUTH_XAUTHERR;
            }
        }
    }

    if (iRetCode == DIRTYAUTH_SUCCESS)
    {
        if (pTokenLen)
        {
            *pTokenLen = pDirtyToken->pToken->Length;
        }

        /* 
            From the MS doc:
            RELYING_PARTY_TOKEN. The contents of the token. This should be treated as a byte array,
            even though it may appear as text. The data is not guaranteed to be null-terminated.
        */
        if (pToken)
        {
            ds_memcpy(pToken, pDirtyToken->pToken->pToken, pDirtyToken->pToken->Length);
        }
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function _DirtyAuthTokenFreeCallback

    \Description
        Attempt to free a dirty token entry (requires token acquisition to be complete)

    \Input *pTokenMem   - pointer to token

    \Output
        int32_t         - zero=success; -1=try again; other negative=error

    \Version 02/25/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _DirtyAuthTokenFreeCallback(void *pTokenMem)
{
    DirtyAuthTokenT *pDirtyToken = (DirtyAuthTokenT *)pTokenMem;

    if (_DirtyAuthCheckToken(pDirtyToken, NULL, NULL) != DIRTYAUTH_PENDING)
    {
        _DirtyAuthDelToken(pDirtyToken);
        return(0);
    }
    return(-1);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function DirtyAuthCreate

    \Description
        Create the DirtyAuth module state.

    \Input bSecureBypass    - TRUE if secure bypass is enabled, else FALSE (default)

    \Output
        DirtyAuthRefT *     - pointer to module state, or NULL

    \Version 04/05/2012 (jbrookes)
*/
/********************************************************************************F*/
DirtyAuthRefT *DirtyAuthCreate(uint8_t bSecureBypass)
{
    DirtyAuthRefT *pDirtyAuth;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    XAUTH_SETTINGS XAuthSettings;
    HRESULT hResult;

    // if already created return null
    if (_DirtyAuth_pRef != NULL)
    {
        NetPrintf(("dirtyauthxenon: dirtyauth already created\n"));
        return(NULL);
    }

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pDirtyAuth = DirtyMemAlloc(sizeof(*pDirtyAuth), DIRTYAUTH_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtyauthxenon: could not allocate module state\n"));
        return(NULL);
    }
    memset(pDirtyAuth, 0, sizeof(*pDirtyAuth));
    pDirtyAuth->iMemGroup = iMemGroup;
    pDirtyAuth->pMemGroupUserData = pMemGroupUserData;

    // create token list critical section
    NetCritInit(&pDirtyAuth->TokenCrit, "dirtyauth");

    // save module ref pointer (singleton module)
    _DirtyAuth_pRef = pDirtyAuth;

    // start xauth  
    XAuthSettings.SizeOfStruct = sizeof(XAuthSettings);
    XAuthSettings.Flags = bSecureBypass ? XAUTH_FLAG_BYPASS_SECURITY : 0;
    if ((hResult = XAuthStartup(&XAuthSettings)) < 0)
    {
        NetPrintf(("dirtyauthxenon: could not start XAuth (err=%s)\n", DirtyErrGetName(hResult)));
        DirtyAuthDestroy(pDirtyAuth);
        return(NULL);
    }
    pDirtyAuth->bXAuthStarted = TRUE;

    // return module pointer to caller
    return(pDirtyAuth);
}

/*F********************************************************************************/
/*!
    \Function DirtyAuthDestroy

    \Description
        Destroy the DirtyAuth module.

    \Input *pDirtyAuth   - pointer to module state

    \Version 04/05/2012 (jbrookes)
*/
/********************************************************************************F*/
void DirtyAuthDestroy(DirtyAuthRefT *pDirtyAuth)
{
    DirtyAuthTokenT *pDirtyToken, *pTokenToFree;

    if (_DirtyAuth_pRef == NULL)
    {
        NetPrintf(("dirtyauthxenon: dirtyauth not created and cannot be destroyed\n"));
        return;
    }

    // clean up token list
    for (pDirtyToken = pDirtyAuth->pTokenList; pDirtyToken != NULL; )
    {
        pTokenToFree = pDirtyToken;
        pDirtyToken = pDirtyToken->pNext;

        if (_DirtyAuthCheckToken(pTokenToFree, NULL, NULL) == DIRTYAUTH_PENDING)
        {
            // token acquisition is in progress, so transfer token destruction responsibility to NetConn
            NetPrintf(("dirtyauthxenon: [%08x] token free on module shutdown deferred to netconn due to pending token acquisition\n", (uintptr_t)pDirtyToken));
            NetConnControl('recu', 0, 0, (void *)_DirtyAuthTokenFreeCallback, pTokenToFree);
        }
        else
        {
            _DirtyAuthDelToken(pTokenToFree);
        }
    }

    // shut down xauth
    if (pDirtyAuth->bXAuthStarted)
    {
        XAuthShutdown();
    }

    // release critical section
    NetCritKill(&pDirtyAuth->TokenCrit);

    // release module memory
    DirtyMemFree(pDirtyAuth, DIRTYAUTH_MEMID, pDirtyAuth->iMemGroup, pDirtyAuth->pMemGroupUserData);

    // clear singleton ref pointer
    _DirtyAuth_pRef = NULL;
}

/*F********************************************************************************/
/*!
    \Function DirtyAuthGetToken

    \Description
        Get an XAuth token

    \Input iUserIndex   - user index
    \Input *pHost       - pointer to hostname to get token for
    \Input bForceNew    - TRUE=force acquisition of new token

    \Output
        int32_t         - DIRTYAUTH_* response code

    \Version 04/10/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyAuthGetToken(int32_t iUserIndex, const char *pHost, uint8_t bForceNew)
{
    DirtyAuthRefT *pDirtyAuth = _DirtyAuth_pRef;
    DirtyAuthTokenT *pDirtyToken;
    HRESULT hResult;
    int32_t iResult;
    char strUrl[300];

    // make sure module is started
    if (pDirtyAuth == NULL)
    {
        NetPrintf(("dirtyauthxenon: attempt to use dirtyauth when the module is not started\n"));
        return(DIRTYAUTH_NOTSTARTED);
    }

    // acquire token critical section
    NetCritEnter(&pDirtyAuth->TokenCrit);

    // see if token is already in our cache
    if ((pDirtyToken = _DirtyAuthFindToken(pDirtyAuth, pHost)) != NULL)
    {
        // get token status
        iResult = _DirtyAuthCheckToken(pDirtyToken, NULL, NULL);

        // reuse token if caller doesn't want a new token OR we're in a PENDING state
        if ((bForceNew == FALSE) || (iResult == DIRTYAUTH_PENDING))
        {
            NetCritLeave(&pDirtyAuth->TokenCrit);
            NetPrintf(("dirtyauthxenon: reusing token for host '%s' (user index = %d, result = %d)\n", pHost, iUserIndex, iResult));
            return(iResult);
        }

        NetPrintf(("dirtyauthxenon: forcing refresh of token for host '%s' (user index = %d, result = %d)\n", pHost, iUserIndex));
        // remove token from token list
        _DirtyAuthRemToken(pDirtyAuth, pDirtyToken);
        // delete token
        _DirtyAuthDelToken(pDirtyToken);
    }

    // allocate a token cache structure
    if ((pDirtyToken = _DirtyAuthNewToken(pDirtyAuth, pHost)) == NULL)
    {
        NetCritLeave(&pDirtyAuth->TokenCrit);
        return(DIRTYAUTH_MEMORY);
    }

    /* request a token for the user for the target URL.  if the call succeeds, the
       memory for a pToken pointer will be allocated. */
    ds_snzprintf(strUrl, sizeof(strUrl), "https://%s/", pHost);
    NetPrintf(("dirtyauthxenon: getting token for host '%s' (user index = %d)\n", pHost, iUserIndex));
    if (((hResult = XAuthGetToken(iUserIndex, strUrl, strlen(strUrl), &pDirtyToken->pToken, &pDirtyToken->XAuthOverlapped)) & 0xffff) != ERROR_IO_PENDING)
    {
        NetPrintf(("dirtyauthxenon: could not get token for host=%s (err=%s)\n", pHost, DirtyErrGetName(hResult)));
        DirtyMemFree(pDirtyToken, DIRTYAUTH_MEMID, pDirtyAuth->iMemGroup, pDirtyAuth->pMemGroupUserData);
        NetCritLeave(&pDirtyAuth->TokenCrit);
        return(DIRTYAUTH_XAUTHERR);
    }

    // insert into token list
    _DirtyAuthAddToken(pDirtyAuth, pDirtyToken);

    // get token status
    iResult = _DirtyAuthCheckToken(pDirtyToken, NULL, NULL);

    // release tokenlist critical section
    NetCritLeave(&pDirtyAuth->TokenCrit);

    // return token status to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function DirtyAuthCheckToken

    \Description
        Check to see if an auth token is available

    \Input *pHost       - pointer to host to check token availability for
    \Input *pTokenLen   - filled with token length when return code is DIRTYAUTH_SUCCESS (caller can pass NULL here)
    \Input *pToken      - filled with token when return code is DIRTYAUTH_SUCCESS (caller can pass NULL here)

    \Output
        int32_t     - DIRTYAUTH_* response code

    \Version 04/10/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyAuthCheckToken(const char *pHost, int32_t *pTokenLen, uint8_t *pToken)
{
    DirtyAuthRefT *pDirtyAuth = _DirtyAuth_pRef;
    DirtyAuthTokenT *pDirtyToken;
    int32_t iResult;

    // make sure module is started
    if (pDirtyAuth == NULL)
    {
        NetPrintf(("dirtyauthxenon: attempt to use dirtyauth when the module is not started\n"));
        return(DIRTYAUTH_NOTSTARTED);
    }

    // acquire token critical section
    NetCritEnter(&pDirtyAuth->TokenCrit);

    // see if token is already in our cache
    if ((pDirtyToken = _DirtyAuthFindToken(pDirtyAuth, pHost)) != NULL)
    {
        iResult = _DirtyAuthCheckToken(pDirtyToken, pTokenLen, pToken);
    }
    else
    {
        iResult = DIRTYAUTH_NOTFOUND;
    }

    // release tokenlist critical section
    NetCritLeave(&pDirtyAuth->TokenCrit);

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function DirtyAuthGetTokenError

    \Description
        Check general DirtyAuth status. Different selectors return
        different status attributes.

    \Input pRef     - DirtyAuthRefT
    \Input iKind    - input selector
    \Input iData    - (optional) selector specific
    \Input *pBuf    -(optional) pointer to output buffer
    \Input iBufSize  (optional) size of output buffer\
    \Input pHost     (optional) Auth Host String

    \Output
        int32_t     - selector result

    \Notes
        iKind can be one of the following:

        \verbatim
        'stat' - Returns DirtyAuthToken Status/Error in pBuf using pHost to find the Auth Token
    
    \Version 27/1/2014 (tcho)
*/
/********************************************************************************F*/
int32_t DirtyAuthStatus(DirtyAuthRefT *pRef, int32_t iKind, int32_t iData, void *pBuf, int32_t iBufSize, const char *pHost)
{
    if (iKind == 'stat')
    {
        DirtyAuthTokenT *pDirtyToken;
        int32_t iResult = DIRTYAUTH_ERROR;

        //Check if Module is started or not
        if (pRef == NULL)
        {
            NetPrintf(("dirtyauthxenon: DirtyAuthGrtTokenError: DirtyAuthRefT is not initialized.\n"));
            return(DIRTYAUTH_NOTSTARTED);
        }

        //Get Token
        NetCritEnter(&pRef->TokenCrit);

        if ((pDirtyToken = _DirtyAuthFindToken(pRef, pHost)) != NULL)
        {
            if (iBufSize >= sizeof(pDirtyToken->dwAuthStat))
            {
                ds_memcpy(pBuf, &(pDirtyToken->dwAuthStat), sizeof(pDirtyToken->dwAuthStat));
                NetPrintf(("dirtyauthxenon: First Party Error Code 0x%08x\n", pDirtyToken->dwAuthStat));
                iResult = DIRTYAUTH_SUCCESS;
            }
            else
            {
                NetPrintf(("dirtyauthxenon: Insufficient Buffer Size "));
                iResult = DIRTYAUTH_MEMORY;
            }
        }
        else
        {
            NetPrintf(("dirtyauthxenon: Token not found.\n"));
            iResult = DIRTYAUTH_NOTFOUND;
        }

        NetCritLeave(&pRef->TokenCrit);

        return(iResult);
    }

    return(-1);
}

#else

// module state
typedef struct DirtyAuthRefT
{
    int iNothing;              //!< module mem group id
} DirtyAuthRefT;

// create the module
DirtyAuthRefT *DirtyAuthCreate(unsigned char bSecureBypass) {return (void *)0;}
// destroy the module
void DirtyAuthDestroy(DirtyAuthRefT *pDirtyAuth) {}

#endif // DIRTYSDK_XHTTP_ENABLED

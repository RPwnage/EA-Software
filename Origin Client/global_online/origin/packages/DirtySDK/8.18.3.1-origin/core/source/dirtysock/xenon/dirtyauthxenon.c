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

#include "dirtyerr.h"
#include "dirtylib.h"
#include "dirtymem.h"

#include "dirtyauthxenon.h"

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
} DirtyAuthTokenT;

// module state
struct DirtyAuthRefT
{
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group

    DirtyAuthTokenT *pTokenList;    //!< list of tokens
    NetCritT TokenCrit;             //!< token list critical section

    uint8_t bXHttpStarted;          //!< has xhttp been started?
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

    if ((pDirtyToken = (DirtyAuthTokenT *)DirtyMemAlloc(sizeof(*pDirtyToken), DIRTYAUTH_MEMID, pDirtyAuth->iMemGroup, pDirtyAuth->pMemGroupUserData)) != NULL)
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

    \Input *pDirtyAuth  - module state
    \Input *pDirtyToken - token to add to token list

    \Version 04/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyAuthDelToken(DirtyAuthRefT *pDirtyAuth, DirtyAuthTokenT *pDirtyToken)
{
    if (pDirtyToken->pToken != NULL)
    {
        XAuthFreeToken(pDirtyToken->pToken);
    }
    DirtyMemFree(pDirtyToken, DIRTYAUTH_MEMID, pDirtyAuth->iMemGroup, pDirtyAuth->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function _DirtyAuthAddToken

    \Description
        Add token to token list

    \Input *pDirtyAuth  - module state
    \Input *pDirtyToken - token to add to token list

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

    \Input *pDirtyToken - pointer to hostname to check token availability for

    \Output
        int32_t         - 0=not available, 1=available
        
    \Version 04/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DirtyAuthCheckToken(DirtyAuthTokenT *pDirtyToken)
{
    DWORD dwResult;

    // see if we've already resolved this token
    if (pDirtyToken->bValid)
    {
        return(DIRTYAUTH_SUCCESS);
    }

    // fast check to see if async operation has completed
    if (XHasOverlappedIoCompleted(&pDirtyToken->XAuthOverlapped) == FALSE)
    {
        return(DIRTYAUTH_PENDING);
    }

    // get the result
    if ((dwResult = XGetOverlappedExtendedError(&pDirtyToken->XAuthOverlapped)) == ERROR_SUCCESS)
    {
        NetPrintf(("dirtyauthxenon: acquired token for host '%s'\n", pDirtyToken->strHost));
        pDirtyToken->bValid = TRUE;
        return(DIRTYAUTH_SUCCESS);
    }
    else
    {
        NetPrintf(("dirtyauthxenon: failed to acquire token for host '%s' (err=%s)\n", pDirtyToken->strHost, DirtyErrGetName(dwResult)));
        return(DIRTYAUTH_XAUTHERR);
    }
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

    // start xhttp (must come before xauth)
    if (!XHttpStartup(0, NULL))
    {
        NetPrintf(("dirtyauthxenon: could not start XHttp\n"));
        DirtyAuthDestroy(pDirtyAuth);
        return(NULL);
    }
    pDirtyAuth->bXHttpStarted = TRUE;

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
        _DirtyAuthDelToken(pDirtyAuth, pDirtyToken);
    }

    // shut down xauth
    if (pDirtyAuth->bXAuthStarted)
    {
        XAuthShutdown();
    }
    // shut down xhttp
    if (pDirtyAuth->bXHttpStarted)
    {
        XHttpShutdown();
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

    \Input *pHost       - pointer to hostname to get token for
    \Input bForceNew    - TRUE=force acquisition of new token

    \Output
        int32_t         - DIRTYAUTH_* response code
        
    \Version 04/10/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyAuthGetToken(const char *pHost, uint8_t bForceNew)
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
        if (!bForceNew)
        {
            return(_DirtyAuthCheckToken(pDirtyToken));
        }
        _DirtyAuthDelToken(pDirtyAuth, pDirtyToken);
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
    NetPrintf(("dirtyauthxenon: getting token for host '%s'\n", pHost));
    if (((hResult = XAuthGetToken(0, strUrl, strlen(strUrl), &pDirtyToken->pToken, &pDirtyToken->XAuthOverlapped)) & 0xffff) != ERROR_IO_PENDING)
    {
        NetPrintf(("protohttpxenon: could not get token for host=%s (err=%s)\n", pHost, DirtyErrGetName(hResult)));
        DirtyMemFree(pDirtyToken, DIRTYAUTH_MEMID, pDirtyAuth->iMemGroup, pDirtyAuth->pMemGroupUserData);
        NetCritLeave(&pDirtyAuth->TokenCrit);
        return(DIRTYAUTH_XAUTHERR);
    }

    // insert into token list
    _DirtyAuthAddToken(pDirtyAuth, pDirtyToken);

    // get token status
    iResult = _DirtyAuthCheckToken(pDirtyToken);

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

    \Input *pHost   - pointer to host to check token availability for

    \Output
        int32_t     - DIRTYAUTH_* response code
        
    \Version 04/10/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyAuthCheckToken(const char *pHost)
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
        iResult = _DirtyAuthCheckToken(pDirtyToken);
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
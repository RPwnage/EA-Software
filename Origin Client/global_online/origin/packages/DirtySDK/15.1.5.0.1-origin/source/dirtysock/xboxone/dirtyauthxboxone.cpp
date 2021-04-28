/*H********************************************************************************/
/*!
    \File dirtyauthxboxone.cpp

    \Description
        Xbox One Auth wrapper module

    \Copyright
        Copyright 2012 Electronic Arts Inc.

    \Version 09/04/2013 (amakoukji) First Version ported from Xenon
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <xdk.h>
#include <string.h>
#include <stdlib.h>

#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/dirtysock/xboxone/dirtyauthxboxone.h"


using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Xbox::System;

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

//! token request structure
typedef struct DirtyAuthTokenT
{
    struct DirtyAuthTokenT *pNext;                                  //!< pointer to next token in token list
    char *pToken;                                                   //!< pointer to auth token
    int32_t iTokenBufSize;                                          //!< size of the auth token
    char strHost[255];                                              //!< hostname token is for
    int32_t iUserIndex;                                             //!< user index for this token
    int32_t iTimeout;                                               //!< token expiration interval
    int32_t iTokenAcqTime;                                          //!< time when token was acquired
    uint8_t bValid;                                                 //!< token is valid (async process is incomplete or failed)
    uint8_t bFailed;                                                //!< token acquisition failed (async process is complete and failed)
    IAsyncOperation<GetTokenAndSignatureResult^>^ asyncOperation;   //!< async operation for fetching auth
} DirtyAuthTokenT;

// module state
struct DirtyAuthRefT
{
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group

    DirtyAuthTokenT *pTokenList;    //!< list of tokens
    NetCritT TokenCrit;             //!< token list critical section

    bool bAuthStarted;          //!< has xauth been started?
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
    \Input iUserIndex       - user index
    \Input iTimeout         - token expiration interval  (0 for no expiration)

    \Output
        DirtyAuthTokenT *   - pointer to token, or NULL if not found

    \Version 04/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static DirtyAuthTokenT *_DirtyAuthNewToken(DirtyAuthRefT *pDirtyAuth, const char *pHost, int32_t iUserIndex, int32_t iTimeout)
{
    DirtyAuthTokenT *pDirtyToken;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // token entries use netconn's memgroup and memgroup pointer as the memory can exceed the lifespan of the DirtyAuth module
    iMemGroup = NetConnStatus('mgrp', 0, &pMemGroupUserData, sizeof(pMemGroupUserData));

    if ((pDirtyToken = (DirtyAuthTokenT *)DirtyMemAlloc(sizeof(*pDirtyToken), DIRTYAUTH_MEMID, iMemGroup, pMemGroupUserData)) != NULL)
    {
        ds_memclr(pDirtyToken, sizeof(*pDirtyToken));
        ds_strnzcpy(pDirtyToken->strHost, pHost, sizeof(pDirtyToken->strHost));
        pDirtyToken->iUserIndex = iUserIndex;
        pDirtyToken->iTimeout = iTimeout;
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
    \Version 09/04/2013 (amakoukji) Adapted for Xbox One
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
        DirtyMemFree(pDirtyToken->pToken, DIRTYAUTH_MEMID, iMemGroup, pMemGroupUserData);
    }

    pDirtyToken->asyncOperation = nullptr;

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

    \Input *pDirtyAuth  - module state
    \Input *pDirtyToken - token to add to the token list

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
    \Input iUserIndex       - User Index

    \Output
        DirtyAuthTokenT *   - pointer to token, or NULL if not found
        
    \Version 04/10/2012 (jbrookes)
*/
/********************************************************************************F*/
static DirtyAuthTokenT *_DirtyAuthFindToken(DirtyAuthRefT *pDirtyAuth, const char *pHost, int32_t iUserIndex)
{
    DirtyAuthTokenT *pDirtyToken;
    for (pDirtyToken = pDirtyAuth->pTokenList; pDirtyToken != NULL; pDirtyToken = pDirtyToken->pNext)
    {
        if (!ds_stricmp(pDirtyToken->strHost, pHost))
        {
            if (iUserIndex == -1 || iUserIndex == pDirtyToken->iUserIndex)
            {
                return(pDirtyToken);
            }
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
    \Version 09/04/2013 (amakoukji) Adapted for Xbox One
*/
/********************************************************************************F*/
static int32_t _DirtyAuthCheckToken(DirtyAuthTokenT *pDirtyToken, int32_t *pTokenLen, char *pToken)
{
    int32_t iRetCode = DIRTYAUTH_SUCCESS;

    // see if we've already resolved this token
    if (pDirtyToken->bValid)
    {
        // check for timeout, if timeout is enabled (iTimeout value is non zero when disabled)
        if ((pDirtyToken->iTimeout != 0) && (NetTickDiff(NetTick(), pDirtyToken->iTokenAcqTime) >= pDirtyToken->iTimeout) && pDirtyToken->bValid)
        {
            iRetCode = DIRTYAUTH_EXPIRED;
        }
    }
    else
    {
        // fast check to see if async operation has completed
        if (!pDirtyToken->bFailed)
        {
            iRetCode = DIRTYAUTH_PENDING;
        }
        else
        {
            NetPrintf(("dirtyauthxboxone: failed to acquire token for host '%s'\n", pDirtyToken->strHost));
            iRetCode = DIRTYAUTH_AUTHERR;
        }
    }

    if (iRetCode == DIRTYAUTH_SUCCESS)
    {
        if (pTokenLen)
        {
            *pTokenLen = pDirtyToken->iTokenBufSize;
        }

        if (pToken)
        {
            ds_memcpy(pToken, pDirtyToken->pToken, pDirtyToken->iTokenBufSize);
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

    \Input bSecureBypass    - ignored, this is present to keep the same Api as Xenon

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

    // if already created return null
    if (_DirtyAuth_pRef != NULL)
    {
        NetPrintf(("dirtyauthxboxone: dirtyauth already created\n"));
        return(NULL);
    }

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pDirtyAuth = (DirtyAuthRefT*)DirtyMemAlloc(sizeof(*pDirtyAuth), DIRTYAUTH_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtyauthxboxone: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pDirtyAuth, sizeof(*pDirtyAuth));
    pDirtyAuth->iMemGroup = iMemGroup;
    pDirtyAuth->pMemGroupUserData = pMemGroupUserData;

    // create token list critical section
    NetCritInit(&pDirtyAuth->TokenCrit, "dirtyauth");

    // save module ref pointer (singleton module)
    _DirtyAuth_pRef = pDirtyAuth;

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
        NetPrintf(("dirtyauthxboxone: dirtyauth not created and cannot be destroyed\n"));
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
            NetPrintf(("dirtyauthxboxone: [%08x] token free on module shutdown deferred to netconn due to pending token acquisition\n", (uintptr_t)pDirtyToken));
            NetConnControl('recu', 0, 0, (void *)_DirtyAuthTokenFreeCallback, pTokenToFree);
        }
        else
        {
            _DirtyAuthDelToken(pTokenToFree);
        }
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
    \Function DirtyAuthGetToken2

    \Description
        Get an XAuth token

    \Input iUserIndex   - user index
    \Input *pHost       - pointer to hostname to get token for
    \Input bForceNew    - TRUE=force acquisition of new token
    \Input iTimeout     - expiration time interval   (0 for no expiration)

    \Output
        int32_t         - DIRTYAUTH_* response code

    \Version 21/10/2015 (tcho)
*/
/********************************************************************************F*/
int32_t DirtyAuthGetToken2(int32_t iUserIndex, const char *pHost, uint8_t bForceNew, int32_t iTimeout)
{
    DirtyAuthRefT *pDirtyAuth = _DirtyAuth_pRef;
    DirtyAuthTokenT *pDirtyToken;
    int32_t iResult;
    wchar_t wstrUrl[300];
    wchar_t wstrHost[300];

    // make sure module is started
    if (pDirtyAuth == NULL)
    {
        NetPrintf(("dirtyauthxboxone: attempt to use dirtyauth when the module is not started\n"));
        return(DIRTYAUTH_NOTSTARTED);
    }

    // acquire token critical section
    NetCritEnter(&pDirtyAuth->TokenCrit);

    // see if token is already in our cache
    if ((pDirtyToken = _DirtyAuthFindToken(pDirtyAuth, pHost, iUserIndex)) != NULL)
    {
        // get token status
        iResult = _DirtyAuthCheckToken(pDirtyToken, NULL, NULL);

        // reuse token if caller doesn't want a new token OR we're in a PENDING state
        if ((bForceNew == FALSE) || (iResult == DIRTYAUTH_PENDING))
        {
            NetCritLeave(&pDirtyAuth->TokenCrit);
            NetPrintf(("dirtyauthxboxone: reusing token for host '%s' (user index = %d, result = %d)\n", pHost, iUserIndex, iResult));
            return(iResult);
        }

        NetPrintf(("dirtyauthxboxone: forcing refresh of token for host '%s' (user index = %d, result = %d)\n", pHost, iUserIndex));
        // remove token from token list
        _DirtyAuthRemToken(pDirtyAuth, pDirtyToken);
        // delete token
        _DirtyAuthDelToken(pDirtyToken);
    }

    // allocate a token cache structure
    if ((pDirtyToken = _DirtyAuthNewToken(pDirtyAuth, pHost, iUserIndex, iTimeout)) == NULL)
    {
        NetCritLeave(&pDirtyAuth->TokenCrit);
        return(DIRTYAUTH_MEMORY);
    }

    wcscpy_s(wstrUrl, sizeof(wstrUrl) / 2, L"https://");
    mbstowcs(wstrHost, pHost, sizeof(wstrUrl) / 2);
    wcscat_s(wstrUrl, sizeof(wstrUrl) / 2, wstrHost);

    Platform::String ^ pHttpMethod = ref new Platform::String(L"GET");
    Platform::String ^ pStrUrl = ref new Platform::String( wstrUrl );
    Platform::Array<unsigned char>^ strBody = ref new Platform::Array<unsigned char>(1);
    strBody[0] = 0;
    
    if (NetConnQuery(NULL, NULL, 0) < 0)
    {
        NetPrintf(("dirtyauthxboxone: At least 1 user must be signed in to fetch XSTS token.\n"));
        NetCritLeave(&pDirtyAuth->TokenCrit);
        return(DIRTYAUTH_ERROR);
    }

    User ^refUser = nullptr;
    if (NetConnStatus('xusr', iUserIndex, &refUser, sizeof(refUser)) < 0)
    {
        NetPrintf(("dirtyauthxboxone: No user at index %d.\n", iUserIndex));
        NetCritLeave(&pDirtyAuth->TokenCrit);
        return(DIRTYAUTH_ERROR);
    }

    NetPrintf(("dirtyauthxboxone: getting token for host '%s' (user index = %d)\n", pHost, iUserIndex));
    try
    {
        pDirtyToken->asyncOperation = refUser->GetTokenAndSignatureAsync(pHttpMethod, pStrUrl, nullptr, strBody);
    }
    catch (Exception ^ e)
    {
        NetPrintf(("dirtyauthxboxone: exception caught while trying to get token (%S/0x%08x)\n",
            e->Message->Data(), e->HResult));
        NetCritLeave(&pDirtyAuth->TokenCrit);
        return(DIRTYAUTH_ERROR);
    }

    /* request a token for the user for the target URL.  if the call succeeds, the
       memory for a pToken pointer will be allocated. */
    pDirtyToken->asyncOperation->Completed = ref new AsyncOperationCompletedHandler<GetTokenAndSignatureResult ^>(
        [pDirtyToken, pHost, pDirtyAuth, iUserIndex](IAsyncOperation<GetTokenAndSignatureResult ^>^ refAsyncOp, Windows::Foundation::AsyncStatus status){
        if (status == Windows::Foundation::AsyncStatus::Completed)
        {
            // Acquired the token 
            pDirtyToken->pToken = (char *) DirtyMemAlloc(refAsyncOp->GetResults()->Token->Length() + 1, DIRTYAUTH_MEMID, pDirtyAuth->iMemGroup, pDirtyAuth->pMemGroupUserData);
            if (pDirtyToken->pToken != NULL)
            {
                pDirtyToken->iTokenBufSize = refAsyncOp->GetResults()->Token->Length() + 1;
                ds_memclr(pDirtyToken->pToken, pDirtyToken->iTokenBufSize);
                
                // convert wchar string to char string
                wcstombs(pDirtyToken->pToken, refAsyncOp->GetResults()->Token->Data(), pDirtyToken->iTokenBufSize);
                pDirtyToken->iTokenAcqTime = NetTick();
                pDirtyToken->bValid = true;
                pDirtyToken->bFailed = false;
            }
            else
            {    
                pDirtyToken->bValid = false;
                pDirtyToken->bFailed = true;
                NetPrintf(("dirtyauthxboxone: token acquisition for user %d, host %s failed - memory error\n", iUserIndex, pDirtyToken->strHost));
                return;
            }
        }
        else
        {
            pDirtyToken->bValid = false;
            pDirtyToken->bFailed = true;
            NetPrintf(("dirtyauthxboxone: token acquisition for user %d, host %s failed - auth error: %S/0x%x\n", iUserIndex, pDirtyToken->strHost,
                refAsyncOp->ErrorCode.ToString()->Data(), refAsyncOp->ErrorCode.Value));
            return;
        }
     });

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
    return(DirtyAuthGetToken2(iUserIndex, pHost, bForceNew, 0));
}

/*F********************************************************************************/
/*!
    \Function DirtyAuthCheckToken

    \Description
        Check to see if an auth token is available
    
    \Input iUserIndex   - User Index when -1 this will return the first valid ticket
    \Input *pHost       - pointer to host to check token availability for
    \Input *pTokenLen   - filled with token length when return code is DIRTYAUTH_SUCCESS (caller can pass NULL here)
    \Input *pToken      - filled with token when return code is DIRTYAUTH_SUCCESS (caller can pass NULL here)

    \Output
        int32_t     - DIRTYAUTH_* response code

    \Version 04/10/2012 (jbrookes)
    \Version 09/04/2013 (amakoukji) Adapted for Xbox One
*/
/********************************************************************************F*/
int32_t DirtyAuthCheckToken(int32_t iUserIndex, const char *pHost, int32_t *pTokenLen, char *pToken)
{
    DirtyAuthRefT *pDirtyAuth = _DirtyAuth_pRef;
    DirtyAuthTokenT *pDirtyToken;
    int32_t iResult;

    // make sure module is started
    if (pDirtyAuth == NULL)
    {
        NetPrintf(("dirtyauthxboxone: attempt to use dirtyauth when the module is not started\n"));
        return(DIRTYAUTH_NOTSTARTED);
    }

    // acquire token critical section
    NetCritEnter(&pDirtyAuth->TokenCrit);

    // see if token is already in our cache
    if ((pDirtyToken = _DirtyAuthFindToken(pDirtyAuth, pHost, iUserIndex)) != NULL)
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

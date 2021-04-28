/*H*************************************************************************************************/
/*!

    \File    xmltrans.c

    \Description
        This is an XML transaction module. It provides the structure in order to perform
        either a polled or callback based transaction with a server. The individual
        transaction handlers are code independently.

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version    1.0        01/30/2002 (GWS) First Version

*/
/*************************************************************************************************H*/


/*** Include files ********************************************************************************/

#include <stdio.h>
#include <string.h>

#include "dirtydefs.h"
#include "xmltrans.h"


/*** Type Definitions *****************************************************************************/

//! XML idle process structure
typedef struct XmlIdleT
{
    XmlProcessT *pProc;     //!< user-specified idle process
    void *pState;           //!< user-specified data
} XmlIdleT;


/*** Variables ************************************************************************************/

static XmlTransT *_Xml_pTransList = NULL;
static XmlIdleT _Xml_IdleList[16];


/*** Private Functions ****************************************************************************/

/*F************************************************************************************************/
/*!
    \Function _NullCallback
 
    \Description
        A null callback so there is always something to call
 
    \Input pRequest - transaction request record
    
    \Output None

    \Version 1.0 01/30/2002 (GWS) First version	
*/
/************************************************************************************************F*/
static void _NullCallback(void *pRequest)
{
}


/*** Public functions *****************************************************************************/

/*F************************************************************************************************/
/*!
    \Function XmlTransStartup

    \Description
        Startup the transaction module

    \Version 1.0 01/30/2002 (GWS) First version	
*/
/************************************************************************************************F*/
void XmlTransStartup(void)
{
	_Xml_pTransList = NULL;
    memset(_Xml_IdleList, 0, sizeof(_Xml_IdleList));
}


/*F************************************************************************************************/
/*!
    \Function XmlTransShutdown

    \Description
        Shutdown the transaction module

    \Version 1.0 01/30/2002 (GWS) First version	
*/
/************************************************************************************************F*/
void XmlTransShutdown(void)
{
    uint32_t iIndex;

	// cancel any remaining requests
	while (_Xml_pTransList != NULL)
	{
		XmlTransCancel(_Xml_pTransList);
	}

    // let idle procs know about shutdown
    for (iIndex = 0; iIndex < sizeof(_Xml_IdleList)/sizeof(_Xml_IdleList[0]); ++iIndex)
    {
        if (_Xml_IdleList[iIndex].pProc != NULL)
        {
            (_Xml_IdleList[iIndex].pProc)(0, _Xml_IdleList[iIndex].pState);
            _Xml_IdleList[iIndex].pProc = NULL;
        }
    }
}


/*F************************************************************************************************/
/*!
    \Function XmlTransIdleAdd
 
    \Description
        Add an idle function
 
    \Input pProc    - idle pointer
    \Input pState   - state reference
    
    \Output None

    \Version 1.0 01/30/2002 (GWS) First version	
*/
/************************************************************************************************F*/
void XmlTransIdleAdd(XmlProcessT *pProc, void *pState)
{
    uint32_t iIndex;

    // walk the idle list
    for (iIndex = 0; iIndex < sizeof(_Xml_IdleList)/sizeof(_Xml_IdleList[0]); ++iIndex)
    {
        if (_Xml_IdleList[iIndex].pProc == NULL)
        {
            // add idle proc to empty slot
            _Xml_IdleList[iIndex].pProc = pProc;
            _Xml_IdleList[iIndex].pState = pState;
            break;
        }
    }
}


/*F************************************************************************************************/
/*!
    \Function XmlTransIdleDel
 
    \Description
        Delete an idle function
 
    \Input pProc = idle pointer
    \Input pState = state reference
    
    \Output None

    \Version 1.0 01/30/2002 (GWS) First version	
*/
/************************************************************************************************F*/
void XmlTransIdleDel(XmlProcessT *pProc, void *pState)
{
    uint32_t iIndex;

    // walk the idle list
    for (iIndex = 0; iIndex < sizeof(_Xml_IdleList)/sizeof(_Xml_IdleList[0]); ++iIndex)
    {
        if ((_Xml_IdleList[iIndex].pProc == pProc) && (_Xml_IdleList[iIndex].pState == pState))
        {
            // add idle proc to empty slot
            _Xml_IdleList[iIndex].pProc = NULL;
            _Xml_IdleList[iIndex].pState = NULL;
            break;
        }
    }
}


/*F************************************************************************************************/
/*!
    \Function XmlTransComplete
 
    \Description
        Call this function to see if a transaction is complete
 
    \Input pRequest = transaction record pointer
    
    \Output Negative=error, zero=still pending, positive=complete

    \Version 1.0 01/30/2002 (GWS) First version	
*/
/************************************************************************************************F*/
int32_t XmlTransComplete(XmlTransT *pRequest)
{
	XmlTransT *pTrans;
	int32_t iState;
    
    iState = XML_TRANS_FAILED;
    if( pRequest->iResult != NULL )
    {
        iState = *pRequest->iResult;

        // dont allow result to be pending
        if (iState == XML_TRANS_PENDING)
        {
            iState = XML_TRANS_FAILED;
        }

	    // walk the list and see if we can find it
	    for (pTrans = _Xml_pTransList; pTrans != NULL; pTrans = pTrans->pNext)
	    {
		    if (pTrans == pRequest)
		    {
			    iState = XML_TRANS_PENDING;
			    break;
		    }
	    }
    }

	// return the state
	return(iState);
}


/*F************************************************************************************************/
/*!
    \Function XmlTransCancel
 
    \Description
        Cancel a transaction that is in progress
 
    \Input pRequest = transaction record pointer
    
    \Output Negative=error, zero=success

    \Version 1.0 01/30/2002 (GWS) First version	
*/
/************************************************************************************************F*/
int32_t XmlTransCancel(XmlTransT *pRequest)
{
	int32_t iErr = -1;
	XmlTransT *pTrans;
	XmlTransT **pLink;

	// locate the link to this block
	for (pLink = &_Xml_pTransList; *pLink != NULL; pLink = &(*pLink)->pNext)
	{
		if (*pLink == pRequest)
		{
			// remove block from list
			pTrans = *pLink;
			*pLink = pTrans->pNext;

            // only use callback if transaction has started
            if (*pTrans->iResult != XML_TRANS_STARTUP)
            {
                // call the action function
                (pTrans->pAction)(pTrans, XML_TRANS_CANCEL);
            }
            // force the result to cancel
            *pTrans->iResult = XML_TRANS_CANCEL;
            // call the user callback
            (pTrans->pCallback)(pTrans);

			// cancel is complete
			iErr = 0;
			break;
		}
	}

	// return the error code
	return(iErr);
}


/*F************************************************************************************************/
/*!
    \Function XmlTransAppend
 
    \Description
        Append a transaction to the "to do" list
 
    \Input pTrans       - transaction record pointer
    \Input pResult      - pointer to result value
    \Input iChannel     - channel identifier for overlap control
    \Input iTimeout     - timeout in millseconds
    \Input pCallback    - user callback completion function
    \Input pAction      - the action handler for this transaction
    
    \Output Negative=error, zero=added to queue

    \Version 1.0 01/30/2002 (GWS) First version	
    \Version 1.1 05/03/2002 (GWS) Added channel support
*/
/************************************************************************************************F*/
int32_t XmlTransAppend(XmlTransT *pTrans, int32_t *pResult, int32_t iChannel, int32_t iTimeout, XmlCallbackT *pCallback, XmlActionT *pAction)
{
	XmlTransT **pLink;

    // if a result variable is not provided, use the trans record
    if (pResult == NULL)
    {
        pResult = &pTrans->iState;
    }

	// make sure not already in list / find append point
	for (pLink = &_Xml_pTransList; *pLink != NULL; pLink = &(*pLink)->pNext)
	{
		if (*pLink == pTrans)
		{
			// already in list-- return error
			return(-1);
		}
	}

	// setup private data
	memset(pTrans, 0, sizeof(*pTrans));
	pTrans->pCallback = pCallback;
    pTrans->pAction = pAction;
    pTrans->iChannel = iChannel;
    pTrans->uTimeout = (iTimeout < 0 ? 0xffffffff : (uint32_t)iTimeout);
    pTrans->iResult = pResult;
    *pTrans->iResult = XML_TRANS_STARTUP;

    // clamp maximum timeout to one hour
    if (pTrans->uTimeout > 60*60*1000)
    {
        pTrans->uTimeout = 60*60*1000;
    }
    // make sure callback is valid
    if (pTrans->pCallback == NULL)
    {
        pTrans->pCallback = _NullCallback;
    }

	// append to list
	*pLink = pTrans;
	return(0);
}


/*F************************************************************************************************/
/*!
    \Function XmlTransProcess
 
    \Description
        Give "life" to module (all callbacks performed from this thread)
 
    \Input uTick = tick counter in milliseconds
    
    \Output None

    \Version 1.0 01/30/2002 (GWS) First version	
*/
/************************************************************************************************F*/
#if DIRTYCODE_DEBUG
    static int32_t     _XmlTrans_iTransCount = 0;
    static int32_t     _XmlTrans_aResultLst[64];
    static XmlTransT _XmlTrans_aTransLst[64];
#endif

void XmlTransProcess(uint32_t uTick)
{
    uint32_t iIndex;
	XmlTransT *pTrans;
    int32_t ChanList[16];
    uint32_t iChanAdd = 0;
    XmlTransT **pRemove = NULL;

    // don't allow tick to be zero (since that is shutdown flag)
    if (uTick == 0)
    {
        uTick = 1;
    }

    // walk the idle list
    for (iIndex = 0; iIndex < sizeof(_Xml_IdleList)/sizeof(_Xml_IdleList[0]); ++iIndex)
    {
        if (_Xml_IdleList[iIndex].pProc != NULL)
        {
            (_Xml_IdleList[iIndex].pProc)(uTick, _Xml_IdleList[iIndex].pState);
        }
    }
    
    #if DIRTYCODE_DEBUG
        _XmlTrans_iTransCount = 0;
        memset( _XmlTrans_aResultLst, 0, sizeof(_XmlTrans_aResultLst) );
        memset( _XmlTrans_aTransLst, 0, sizeof(_XmlTrans_aTransLst) );
    #endif

    // always scan the entire list
    for (pTrans = _Xml_pTransList; (pTrans != NULL) && (iChanAdd < sizeof(ChanList)/sizeof(ChanList[0])); pTrans = pTrans->pNext)
    {
        #if DIRTYCODE_DEBUG
            _XmlTrans_aTransLst[_XmlTrans_iTransCount] = *pTrans;
            _XmlTrans_aResultLst[_XmlTrans_iTransCount] = *pTrans->iResult;
            _XmlTrans_iTransCount++;
        #endif

        // see if we have already done a transaction of this type
        for (iIndex = 0; (iIndex < iChanAdd) && (pTrans->iChannel != ChanList[iIndex]); ++iIndex)
            ;
        // skip if some other transaction is already on this channel
        if (iIndex < iChanAdd)
        {
            continue;
        }
        // add to channel list
        ChanList[iChanAdd++] = pTrans->iChannel;

        // check for startup mode
        if (*pTrans->iResult == XML_TRANS_STARTUP)
        {
            // translate timeout to expire time
            pTrans->uTimeout += uTick;
            // call the action handler
            *pTrans->iResult = (pTrans->pAction)(pTrans, XML_TRANS_STARTUP);
            // convert startup code to pending
            if (*pTrans->iResult == XML_TRANS_STARTUP)
            {
                *pTrans->iResult = XML_TRANS_PENDING;
            }
        }

        // see if transaction is still pending
        if (*pTrans->iResult == XML_TRANS_PENDING)
        {
            *pTrans->iResult = (pTrans->pAction)(pTrans, XML_TRANS_PENDING);
        }

        // check for timeout
        if ((*pTrans->iResult == XML_TRANS_PENDING) && (NetTickDiff(uTick, pTrans->uTimeout) >= 0))
        {
            // call the action function
            *pTrans->iResult = (pTrans->pAction)(pTrans, XML_TRANS_TIMEOUT);
        }

        // see if transaction is complete
        if (*pTrans->iResult != XML_TRANS_PENDING)
        {
            // point to the list start to signal remove is needed
            pRemove = &_Xml_pTransList;
        }
    }

    // see if we need to remove anything
    if (pRemove != NULL)
    {
        // walk the list and perform removals
        while ((pTrans = *pRemove) != NULL)
        {
            if ((*pTrans->iResult != XML_TRANS_STARTUP) && (*pTrans->iResult != XML_TRANS_PENDING))
            {
                // remove from list
                *pRemove = pTrans->pNext;
                // do the callback
                (pTrans->pCallback)(pTrans);
            }
            else
            {
                // advance to next
                pRemove = &(*pRemove)->pNext;
            }
        }
    }
}


/*F************************************************************************************************/
/*!
    \Function XmlTransActive
 
    \Description
        Determine if transaction of a particular type is active
 
    \Input iChannel = transaction channel to check
    
    \Output zero=inactive, positive=active

    \Version 1.0 05/10/2002 (GWS) First version	
*/
/************************************************************************************************F*/
int32_t XmlTransActive(int32_t iChannel)
{
    int32_t iActive = 0;
	XmlTransT *pTrans;

    // always scan the entire list
    for (pTrans = _Xml_pTransList; (pTrans != NULL); pTrans = pTrans->pNext)
    {
        if (pTrans->iChannel == iChannel)
        {
            iActive = (pTrans->iState == XML_TRANS_PENDING);
            break;
        }
    }
    return(iActive);
}


#if WIN32
#include <windows.h>
#endif

/*F************************************************************************************************/
/*!
    \Function XmlTransPrintf
 
    \Description
        Print diagnostic output
 
    \Input pFormat = printf compatible format string
    
    \Output None

    \Version 1.0 03/25/2002 (GWS) First version	
*/
/************************************************************************************************F*/
void XmlTransPrintf(const char *pFormat, ...)
{
    #if WIN32
    va_list arg;
    char buffer[4096];

	va_start(arg, pFormat);
	vsprintf(buffer, (char *)pFormat, arg);
	va_end(arg);

    OutputDebugStringA(buffer);
    #endif
}




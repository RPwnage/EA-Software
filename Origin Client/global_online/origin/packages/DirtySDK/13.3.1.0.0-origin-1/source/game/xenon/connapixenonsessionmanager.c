/*H*************************************************************************************/
/*!

    \File connapixenonsessionmanager.c

    \Description

    \Notes

    \Copyright
        Copyright (c) / Electronic Arts 2008-2009.  ALL RIGHTS RESERVED.

    \Version 1.0 04/07/2009 (jrainy) First version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/
#include <xtl.h>
#include <xonline.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyaddr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/xenon/dirtysessionmanagerxenon.h"
#include "DirtySDK/game/xenon/connapixenonsessionmanager.h"
#include "DirtySDK/dirtysock/netconn.h"

/*** Defines ***************************************************************************/


/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! internal module state
struct ConnApiXenonSessionManagerRefT      //!< module state
{
    // module memory group
    void *pMemGroupUserData;   //!< user data associated with mem group
    int32_t iMemGroup;         //!< module mem group id
    int32_t iCombinedSessionFlags;
    int32_t iRanked;
    uint32_t uSlotType[NETCONN_MAXLOCALUSERS];
    DirtyAddrT aLocalAddrs[NETCONN_MAXLOCALUSERS];
    int32_t iAddrPrimary;
    int32_t iAddrSecondary;

    DirtySessionManagerRefT *pPrimarySessionManager;
    DirtySessionManagerRefT *pSecondarySessionManager;

    uint8_t uCompletedPrimary;
    uint8_t uCompletedSecondary;
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

/*** Private Functions *****************************************************************/

void _ConnApiXenonSessionManagerEncodeLength(char *strSession, int16_t iLength1, int16_t iLength2)
{
    strSession[0] = (iLength1 % 64) + ' ';
    strSession[1] = (iLength1 / 64) + ' ';
}

void _ConnApiXenonSessionManagerDecodeLength(const char *strSession, int16_t *pLength1, int16_t *pLength2)
{
    if (strSession != NULL)
    {
        if (pLength1 != NULL)
        {
            *pLength1 = (((char*)strSession)[0] - ' ') + 64 * (((char*)strSession)[1] - ' ');
            if (pLength2 != NULL)
            {
                *pLength2 = strlen((char*)strSession) - (*pLength1) - 2;
            }        
        }
    }

    if (((pLength1 != NULL) && (*pLength1 < 0)) ||
        ((pLength2 != NULL) && (*pLength2 < 0)))
    {
        if (pLength1 != NULL)
            *pLength1 = 0;
        if (pLength2 != NULL)
            *pLength2 = 0;
    }
}

/*** Public Functions *****************************************************************/

/*F*************************************************************************************************/
/*!
    \Function    ConnApiXenonSessionManagerCreate
    
    \Description
        Allocate module state and prepare for use.
    
    \Input *pHostAddr   - host dirtyaddr, or NULL if not available
    \Input *pLocalAddr  - local dirtyaddr, or NULL if not available
    
    \Output
        ConnApiXenonSessionManagerRefT * - reference pointer (must be passed to all other functions)

    \Version 04/07/2009 (jrainy)
*/
/*************************************************************************************************F*/
ConnApiXenonSessionManagerRefT *ConnApiXenonSessionManagerCreate(DirtyAddrT *pHostAddr, DirtyAddrT *pLocalAddr)
{
    ConnApiXenonSessionManagerRefT *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), DIRTYSESSMGR_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        return(NULL);
    }
    memset(pRef, 0, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;

    pRef->pSecondarySessionManager = DirtySessionManagerCreate(pHostAddr, pLocalAddr);
    pRef->pPrimarySessionManager = DirtySessionManagerCreate(pHostAddr, pLocalAddr);

    NetPrintf(("connapixenonsessionmanager: [0x%08x] Session Managers: [0x%08x, 0x%08x]\n", pRef, pRef->pPrimarySessionManager, pRef->pSecondarySessionManager));

    pRef->iCombinedSessionFlags = DirtySessionManagerStatus(pRef->pSecondarySessionManager, 'sflg', NULL, 0);

    DirtySessionManagerControl(pRef->pSecondarySessionManager, 'sflg', pRef->iCombinedSessionFlags & (~(XSESSION_CREATE_USES_PEER_NETWORK)), 0, NULL);

    // remove USES_STATS|USES_MATCHMAKING|USES_PRESENCE flags from the primary session default flags
    // also remove all modifiers
    DirtySessionManagerControl(pRef->pPrimarySessionManager, 'sflg', pRef->iCombinedSessionFlags & (~(XSESSION_CREATE_USES_STATS|XSESSION_CREATE_USES_PRESENCE|XSESSION_CREATE_MODIFIERS_MASK)), 0, NULL);

    // return ref to caller
    return(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    ConnApiXenonSessionManagerDestroy

    \Description
        Destroy the module and release its state

    \Input *pRef    - reference pointer

    \Output
        None.

    \Version 04/07/2009 (jrainy)
*/
/*************************************************************************************************F*/
void ConnApiXenonSessionManagerDestroy(ConnApiXenonSessionManagerRefT *pRef)
{
    DirtySessionManagerDestroy(pRef->pSecondarySessionManager);
    DirtySessionManagerDestroy(pRef->pPrimarySessionManager);

    DirtyMemFree(pRef, DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function    ConnApiXenonSessionManagerUpdate
    
    \Description
        Update module state
    
    \Input *pRef    - module state
    
    \Output
        None.
            
    \Version 04/07/2009 (jrainy)
*/
/*************************************************************************************************F*/
void ConnApiXenonSessionManagerUpdate(ConnApiXenonSessionManagerRefT *pRef)
{
    DirtySessionManagerUpdate(pRef->pSecondarySessionManager);
    DirtySessionManagerUpdate(pRef->pPrimarySessionManager);
}

/*F*************************************************************************************************/
/*!
    \Function    ConnApiXenonSessionManagerConnect
    
    \Description
        Initiate transaction with demangler service
    
    \Input *pRef        - module state
    \Input **pSessID    - for Xbox, this is a pointer to the peer's DirtyAddrT
    \Input *uSlot       - slot to use, i.e. DIRTYSESSIONMANAGER_FLAG_PUBLICSLOT
    \Input uCount       - number of players to add to the session
        None.
            
    \Version 04/07/2009 (jrainy)
*/
/*************************************************************************************************F*/
void ConnApiXenonSessionManagerConnect(ConnApiXenonSessionManagerRefT *pRef, const char **pSessID, uint32_t *uSlot, uint32_t uCount)
{
    DirtySessionManagerConnect(pRef->pSecondarySessionManager, pSessID, uSlot, uCount);
    DirtySessionManagerConnect(pRef->pPrimarySessionManager, pSessID, uSlot, uCount);
    pRef->uCompletedPrimary = FALSE;
    pRef->uCompletedSecondary = FALSE;
}

/*F*************************************************************************************************/
/*!
    \Function    ConnApiXenonSessionManagerConnect2
    
    \Description
        Adds the player to the secondary session manager only. Used for rematch
    
    \Input *pRef        - module state
    \Input **pSessID    - for Xbox, this is a pointer to the peer's DirtyAddrT
    \Input *uSlot       - slot to use, i.e. DIRTYSESSIONMANAGER_FLAG_PUBLICSLOT
    \Input uCount       - number of players to add to the session
        None.
            
    \Version 04/27/2009 (jrainy)
*/
/*************************************************************************************************F*/
void ConnApiXenonSessionManagerConnect2(ConnApiXenonSessionManagerRefT *pRef, const char **pSessID, uint32_t *uSlot, uint32_t uCount)
{
    DirtySessionManagerConnect(pRef->pSecondarySessionManager, pSessID, uSlot, uCount);
}


/*F*************************************************************************************************/
/*!
    \Function    ConnApiXenonSessionManagerComplete

    \Description
        Returns TRUE and fills in secure address address if session manager has finished operation
        successfully.

    \Input *pRef            - module state
    \Input *pAddr           - storage for secure address (if result code is positive)
    \Input *pMachineAddress - machine address

    \Output
        int32_t             - positive=success, zero=in progress, negative=failure

    \Version 04/07/2009 (jrainy)
*/
/*************************************************************************************************F*/
int32_t ConnApiXenonSessionManagerComplete(ConnApiXenonSessionManagerRefT *pRef, int32_t *pAddr, const char* pMachineAddress)
{
    int32_t iRet1;
    int32_t iRet2;

    if (!pRef->uCompletedPrimary)
    {
        iRet1 = DirtySessionManagerComplete(pRef->pPrimarySessionManager, &pRef->iAddrPrimary, pMachineAddress);

        if (iRet1 > 0)
        {
            pRef->uCompletedPrimary = TRUE;
        }
        if (iRet1 < 0) 
        {
            pRef->uCompletedPrimary = FALSE;
            pRef->uCompletedSecondary = FALSE;
            return(iRet1);
        }
    }

    if (!pRef->uCompletedSecondary)
    {
        iRet2 = DirtySessionManagerComplete(pRef->pSecondarySessionManager, &pRef->iAddrSecondary, pMachineAddress);

        if (iRet2 > 0)
        {
            pRef->uCompletedSecondary = TRUE;
        }
        if (iRet2 < 0) 
        {
            pRef->uCompletedPrimary = FALSE;
            pRef->uCompletedSecondary = FALSE;
            return(iRet2);
        }
    }

    if (pRef->uCompletedPrimary && pRef->uCompletedSecondary)
    {
        //we want the pAddr from the primary session manager as the secondary session does not include networking
        *pAddr = pRef->iAddrPrimary;
        return(1);
    }

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    ConnApiXenonSessionManagerCreateSess

    \Description
        Passes the request to create a session to the underlying DirtySessionManager

    \Input *pRef            - module state
    \Input bRanked          - whether the session is ranked
    \Input *uUserFlags      - array of flags to pass to the DirtySessionManager
    \Input *pSession        - encoded session string, for existing session or NULL for new session
    \Input *pLocalAddrs     - array of local address of players to join in locally

    \Output
        int32_t             - zero=success, negative=failure


    \Version 04/07/2009 (jrainy)
*/
/*************************************************************************************************F*/
int32_t ConnApiXenonSessionManagerCreateSess(ConnApiXenonSessionManagerRefT *pRef, uint32_t bRanked, uint32_t *uUserFlags, const char *pSession, DirtyAddrT *pLocalAddrs)
{
    char aBuffer1[128]={0};
    char aBuffer2[128]={0};
    int16_t iLength1;
    int16_t iLength2;

    pRef->iRanked = bRanked;
    memcpy(pRef->uSlotType, uUserFlags, sizeof(pRef->uSlotType));
    memcpy(pRef->aLocalAddrs, pLocalAddrs, sizeof(pRef->aLocalAddrs));

    if (pSession == NULL)
    {
        // remove ranked flags from the primary session
        DirtySessionManagerCreateSess(pRef->pSecondarySessionManager, bRanked, uUserFlags, pSession, pLocalAddrs);
        return(DirtySessionManagerCreateSess(pRef->pPrimarySessionManager, FALSE, uUserFlags, pSession, pLocalAddrs));
    }

    _ConnApiXenonSessionManagerDecodeLength(pSession, &iLength1, &iLength2);

    if ((iLength1 < sizeof(aBuffer1)) && (iLength2 < sizeof(aBuffer2)))
    {
        memcpy(aBuffer1, pSession + 2, iLength1);
        memcpy(aBuffer2, pSession + 2 + iLength1, iLength2);

        aBuffer1[iLength1] = 0;
        aBuffer2[iLength2] = 0;
    }
    else
    {
        NetPrintf(("connapixenonsessionmanager: [0x%08x] attempt to create session with invalid session strings\n", pRef));
        return(-1);
    }


    // remove ranked flags from the primary session
    DirtySessionManagerCreateSess(pRef->pSecondarySessionManager, bRanked, uUserFlags, aBuffer2, pLocalAddrs);
    return(DirtySessionManagerCreateSess(pRef->pPrimarySessionManager, FALSE, uUserFlags, aBuffer1, pLocalAddrs));
}

/*F*************************************************************************************************/
/*!
    \Function    ConnApiXenonSessionManagerControl
    
    \Description
        Pass-through to DirtySessionManager control function.
    
    \Input *pRef    - mangle ref
    \Input iControl - control selector
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t     - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'pres' - add/remove presence to/from secondary session creation flags
        \endverbatim

    \Version 04/07/2009 (jrainy)
*/
/*************************************************************************************************F*/
int32_t ConnApiXenonSessionManagerControl(ConnApiXenonSessionManagerRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue)
{
    if (iControl == 'clrs')
    {
        return(DirtySessionManagerControl(pRef->pSecondarySessionManager, iControl, iValue, iValue2, pValue));
    }

    if (iControl == 'conn')
    {
        pRef->uCompletedPrimary = FALSE;
        pRef->uCompletedSecondary = FALSE;

        DirtySessionManagerControl(pRef->pSecondarySessionManager, iControl, iValue, iValue2, pValue);
        return(DirtySessionManagerControl(pRef->pPrimarySessionManager, iControl, iValue, iValue2, pValue));
    }

    if (iControl == 'migr')
    {
        if ((pValue != NULL) && (((char*)pValue)[0] != 0))
        {
            int16_t iLength1;

            _ConnApiXenonSessionManagerDecodeLength(pValue, &iLength1, NULL);

            // don't migrate the primary session "that is not registered with Live" as the WRN[XGI] says
            DirtySessionManagerControl(pRef->pSecondarySessionManager, iControl, iValue, iValue2, (char*)pValue + 2 + iLength1);

            return(0);
        }
        else
        {
            // don't migrate the primary session "that is not registered with Live" as the WRN[XGI] says
            DirtySessionManagerControl(pRef->pSecondarySessionManager, iControl, iValue, iValue2, pValue);
            return(0);
        }
    }

    // add/remove presence to/from secondary session creation flags
    if (iControl == 'pres')
    {
        DirtySessionManagerControl(pRef->pSecondarySessionManager, iControl, iValue, iValue2, pValue);
    }

    if (iControl == 'rest')
    {
        DirtySessionManagerControl(pRef->pSecondarySessionManager, 'dele', 0, 0, NULL);
        if (DirtySessionManagerStatus(pRef->pSecondarySessionManager, 'host', NULL, 0))
        {
            DirtySessionManagerCreateSess(pRef->pSecondarySessionManager, pRef->iRanked, pRef->uSlotType, NULL, pRef->aLocalAddrs);
        }
        else
        {
            int16_t iLength1;
            char* aBuffer2;

            _ConnApiXenonSessionManagerDecodeLength(pValue, &iLength1, NULL);

            aBuffer2 = (char*)pValue + 2 + iLength1;

            // use it to kick off session creation
            DirtySessionManagerCreateSess(pRef->pSecondarySessionManager, pRef->iRanked, pRef->uSlotType, aBuffer2, pRef->aLocalAddrs);
        }

        return(0);
    }
    if (iControl == 'sflg')
    {
        pRef->iCombinedSessionFlags = iValue;

        // remove USES_PRESENCE flags from the primary session
        // also remove modifiers
        DirtySessionManagerControl(pRef->pSecondarySessionManager, iControl, pRef->iCombinedSessionFlags & (~(XSESSION_CREATE_USES_PEER_NETWORK)), iValue2, pValue);
        return(DirtySessionManagerControl(pRef->pPrimarySessionManager, iControl, pRef->iCombinedSessionFlags & (~(XSESSION_CREATE_USES_STATS|XSESSION_CREATE_USES_PRESENCE|XSESSION_CREATE_MODIFIERS_MASK)), iValue2, pValue));
    }
    if ((iControl == 'skil') || (iControl == 'stat') || (iControl == 'strt') || (iControl == 'stop') || (iControl == 'invt'))
    {
        return(DirtySessionManagerControl(pRef->pSecondarySessionManager, iControl, iValue, iValue2, pValue));
    }

    DirtySessionManagerControl(pRef->pSecondarySessionManager, iControl, iValue, iValue2, pValue);
    return(DirtySessionManagerControl(pRef->pPrimarySessionManager, iControl, iValue, iValue2, pValue));
}

/*F*************************************************************************************************/
/*!
    \Function    ConnApiXenonSessionManagerStatus

    \Description
        Return module status based on input selector.

    \Input *pRef    - mangle ref
    \Input iSelect  - status selector
    \Input *pBuf    - buffer pointer
    \Input iBufSize - buffer size

    \Output
        int32_t     - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'pres' - returns whether sessions creation flags have the presence flag set
        \endverbatim

    \Version 04/07/2009 (jrainy)
*/
/*************************************************************************************************F*/
int32_t ConnApiXenonSessionManagerStatus(ConnApiXenonSessionManagerRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'fail')
    {
        uint8_t uFail1 = DirtySessionManagerStatus(pRef->pPrimarySessionManager, iSelect, pBuf, iBufSize);
        uint8_t uFail2 = DirtySessionManagerStatus(pRef->pSecondarySessionManager, iSelect, pBuf, iBufSize);

        return(uFail1 || uFail2);
    }
    if (iSelect == 'idle')
    {
        uint8_t uIdle1 = DirtySessionManagerStatus(pRef->pPrimarySessionManager, iSelect, pBuf, iBufSize);
        uint8_t uIdle2 = DirtySessionManagerStatus(pRef->pSecondarySessionManager, iSelect, pBuf, iBufSize);

        return(uIdle1 && uIdle2);
    }
    if (iSelect == 'pres')
    {
        return(DirtySessionManagerStatus(pRef->pSecondarySessionManager, iSelect, pBuf, iBufSize));
    }
    if (iSelect == 'sess')
    {
        char aBuffer1[128]={0};
        char aBuffer2[128]={0};
        int16_t iLength1;
        int16_t iLength2;
        int32_t iRet1, iRet2;
        char* pOutput = pBuf;

        iRet1 = DirtySessionManagerStatus(pRef->pPrimarySessionManager, iSelect, aBuffer1, sizeof(aBuffer1));
        iRet2 = DirtySessionManagerStatus(pRef->pSecondarySessionManager, iSelect, aBuffer2, sizeof(aBuffer2));

        if ((iRet1 == -1) || (iRet2 == -1))
        {
            return(-1);
        }
        if ((iRet1 == 0) || (iRet2 == 0))
        {
            return(0);
        }

        iLength1 = strlen(aBuffer1);
        iLength2 = strlen(aBuffer2);

        if (iBufSize < (iLength1 + iLength2 + 4))
        {
            return(-1);
        }

        _ConnApiXenonSessionManagerEncodeLength(pOutput, iLength1, iLength2);

        memcpy(pOutput + 2, aBuffer1, iLength1);
        memcpy(pOutput + 2 + iLength1, aBuffer2, iLength2);
        pOutput[2 + iLength1 + iLength2] = 0;

        return(1);
    }
    if (iSelect == 'sflg')
    {
        return(pRef->iCombinedSessionFlags);
    }
 
    //using the secondary manager for 'nonc', 'srch' and any future additions.
    return(DirtySessionManagerStatus(pRef->pSecondarySessionManager, iSelect, pBuf, iBufSize));
}
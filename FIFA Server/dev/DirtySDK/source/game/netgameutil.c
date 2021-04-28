/*H*************************************************************************************************/
/*!

    \File    netgameutil.c

    \Description
        This module provides the setup required to bring peer-peer networking
        online.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2001-2002.  ALL RIGHTS RESERVED.

    \Version    1.0        01/09/01 (GWS) First Version

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/comm/commall.h"
#include "DirtySDK/comm/commudp.h"
#include "DirtySDK/comm/commsrp.h"
#include "DirtySDK/proto/protoadvt.h"
#include "DirtySDK/game/netgameutil.h"
#include "DirtySDK/game/netgamepkt.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! netgameutil internal state
struct NetGameUtilRefT
{
    //! module memory group
    int32_t iMemGroup;
    void *pMemGroupUserData;

    //! class (unique to app)
    char strKind[32];
    //! service address list (64-->128 7/25/05 to fix ConnApi overrun GWS)
    char strAddr[128];

    //! advert ref, for broadcasting hosting info
    ProtoAdvtRef *pAdvt;
    //! advertising ref, for connecting
    ProtoAdvtRef *pFind;

    //! hosting status: 0=hosting, 1=joining
    int32_t iHosting;

    //! host ip
    uint32_t uHostIp;
    //! host port
    uint32_t uHostPort;
    //! peer ip
    uint32_t uPeerIp;
    //! peer port
    uint32_t uPeerPort;

    //! socket ref
    SocketT *pSocket;

    //! max packet width
    int32_t iMaxWid;

    //! size of send buffer in packets
    int32_t iMaxOut;

    //! size of receive buffer in packets
    int32_t iMaxInp;
    
    //! unacknowledged packet window
    int32_t iUnackLimit;

    //! advertising frequency, in seconds
    int32_t iAdvtFreq;
    
    //! client identifier (zero == none)
    int32_t iClientId;
    
    //! remote client identifier
    int32_t iRemoteClientId;
    
    //! metatype
    int32_t iMetatype;

    //! construct function (used for AUTO mode)
    CommAllConstructT *pConstruct;
    
    //! commref of connection, or NULL if no connection
    CommRef *pComm;

    uint8_t uLocalAdvt;
};


/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Constants

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************/
/*!
    \Function _NetGameUtilAdvtConstruct

    \Description
        Create the ProtoAdvt module.

    \Input *pRef        - pointer to module state
    \Input iSize        - ProtoAdvtConstruct() parameter
    \Input bConnect     - TRUE if constructing advt ref for connecting, FALSE if constructing advt ref for broadcasting hosting info

    \Output
        ProtoAdvtRefT * - pointer to new advertising module

    \Version 10/13/2005 (jbrookes)
*/
/*************************************************************************************F*/
static ProtoAdvtRef *_NetGameUtilAdvtConstruct(NetGameUtilRefT *pRef, int32_t iSize, uint32_t bConnect)
{
    ProtoAdvtRef **ppAdvt = bConnect ? &pRef->pFind : &pRef->pAdvt;

    if (*ppAdvt == NULL)
    {
        DirtyMemGroupEnter(pRef->iMemGroup, pRef->pMemGroupUserData);
        *ppAdvt = ProtoAdvtConstruct(iSize);
        DirtyMemGroupLeave();
    }
    return(*ppAdvt);
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function   NetGameUtilCreate

    \Description
        Construct the game setup module

    \Output
        NetGameUtilRefT *   - reference pointer

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
NetGameUtilRefT *NetGameUtilCreate(void)
{
    NetGameUtilRefT *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), NETGAMEUTIL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netgameutil: unable to allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pRef, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;

    // set default comm buffer parameters
    NetGameUtilControl(pRef, 'mwid', NETGAME_DATAPKT_DEFSIZE);
    NetGameUtilControl(pRef, 'minp', NETGAME_DATABUF_MAXSIZE);
    NetGameUtilControl(pRef, 'mout', NETGAME_DATABUF_MAXSIZE);
    
    // return state
    return(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilDestroy

    \Description
        Destroy the game setup module

    \Input *pRef    - reference pointer

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
void NetGameUtilDestroy(NetGameUtilRefT *pRef)
{
    // reset
    NetGameUtilReset(pRef);
    // done with local state
    DirtyMemFree(pRef, NETGAMEUTIL_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilReset

    \Description
        Reset the game setup module

    \Input *pRef    - reference pointer

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
void NetGameUtilReset(NetGameUtilRefT *pRef)
{
    // release comm module
    if (pRef->pComm != NULL)
    {
        pRef->pComm->Destroy(pRef->pComm);
        pRef->pComm = NULL;
    }
    // kill advertisements
    if (pRef->pFind != NULL)
    {
        ProtoAdvtDestroy(pRef->pFind);
        pRef->pFind = NULL;
    }
    if (pRef->pAdvt != NULL)
    {
        ProtoAdvtDestroy(pRef->pAdvt);
        pRef->pAdvt = NULL;
    }
    
    // clear construct ref, if any
    pRef->pConstruct = NULL;
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilControl

    \Description
        Set internal GameUtil parameters.

    \Input *pRef    - reference pointer
    \Input iKind    - selector
    \Input iValue   - value to set

    \Notes
        Selectors:

        \verbatim
            'advf': set advertising frequency, in seconds (call before calling NetGameUtilAdvert)
            'clid': set client identifier* (for use with gameservers)
            'locl': enable or disable query of local adverts
            'meta': set metatype (default=0)
            'minp': set receive buffer size, in packets*
            'mout': set send buffer size, in packets*
            'mwid': set maximum packet width (must be <= NETGAME_DATAPKT_MAXSIZE)*
                * must be set before NetGameLinkConnect() is called to be effective.
        \endverbatim

    \Version    11/12/03 (JLB)
*/
/*************************************************************************************************F*/
void NetGameUtilControl(NetGameUtilRefT *pRef, int32_t iKind, int32_t iValue)
{
    if (iKind == 'clid')
    {
        pRef->iClientId = iValue;
        NetPrintf(("netgameutil: setting clid=0x%08x\n", pRef->iClientId));
    }
    if (iKind == 'rcid')
    {
        pRef->iRemoteClientId = iValue;
        NetPrintf(("netgameutil: setting rcid=0x%08x\n", pRef->iRemoteClientId));
    }
    if (iKind == 'locl')
    {
        pRef->uLocalAdvt = iValue;
        NetPrintf(("netgameutil: setting uLocalAdvt=%d\n", pRef->uLocalAdvt));
    }
    if (iKind == 'meta')
    {
        pRef->iMetatype = iValue;
        NetPrintf(("netgameutil: setting meta=0x%08x\n", pRef->iMetatype));
    }
    if (iKind == 'mwid')
    {
        if (iValue <= NETGAME_DATAPKT_MAXSIZE)
        {
            pRef->iMaxWid = iValue+NETGAME_DATAPKT_MAXTAIL;
            NetPrintf(("netgameutil: setting mwid=%d\n", pRef->iMaxWid));
        }
        else
        {
            NetPrintf(("netgameutil: mwid value of %d is too large\n", iValue));
        }
    }
    if (iKind == 'minp')
    {
        pRef->iMaxInp = iValue;
        NetPrintf(("netgameutil: setting minp=%d\n", pRef->iMaxInp));
    }
    if (iKind == 'mout')
    {
        pRef->iMaxOut = iValue;
        NetPrintf(("netgameutil: setting mout=%d\n", pRef->iMaxOut));
    }
    if (iKind == 'ulmt')
    {
        pRef->iUnackLimit = iValue;
        NetPrintf(("netgameutil: setting iUnackLimit=%d\n", pRef->iUnackLimit));
    }

    if (iKind == 'advf')
    {
        pRef->iAdvtFreq = iValue;
        NetPrintf(("netgameutil: setting advf=%d\n", pRef->iAdvtFreq));
    }
    
    // if selector is unhandled, and a comm func is available, pass it on down
    if ((pRef->pComm != NULL) && (pRef->pComm->Control != NULL))
    {
        pRef->pComm->Control(pRef->pComm, iKind, iValue, NULL);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilConnect

    \Description
        Establish a connection (connect/listen)

    \Input *pRef        - reference pointer
    \Input iConn        - connect mode (NETGAME_CONN_*) | comm type (NETGAME_CONN_*)
    \Input *pAddr       - service address list
    \Input *pConstruct  - comm construct function

    \Output
        int32_t         - zero=success, negative=failure 

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameUtilConnect(NetGameUtilRefT *pRef, int32_t iConn, const char *pAddr, CommAllConstructT *pConstruct)
{
    int32_t iErr = 0;

    // make sure user specified connect and/or listen, and a valid protocol
    if (((iConn & NETGAME_CONN_AUTO) == 0) || (pConstruct == NULL))
    {
        NetPrintf(("netgameutil: invalid conn param\n"));
        return(-100);   // using hundred range to not conflic with COMM_* error codes that iErr can be assigned with.
    }

    // save the address for later
    ds_strnzcpy(pRef->strAddr, pAddr, sizeof(pRef->strAddr));
    // save host/join mode
    pRef->iHosting = ((iConn & NETGAME_CONN_CONNECT) ? 1 : 0);

    NetPrintf(("netgameutil: connect %d %s\n", iConn, pAddr));

    // release previous comm module
    if (pRef->pComm != NULL)
    {
        pRef->pComm->Destroy(pRef->pComm);
    }

    // handle auto mode
    if ((iConn & NETGAME_CONN_AUTO) == NETGAME_CONN_AUTO)
    {
        // save construct ref
        pRef->pConstruct = pConstruct;
        // make sure advertising is running
        if (_NetGameUtilAdvtConstruct(pRef, 8, TRUE) == NULL)
        {
            NetPrintf(("netgameutil: NetGameUtilConnect() failed to create the ProtoAdvt module.\n"));
            return(-101);  // using hundred range to not conflic with COMM_* error codes that iErr can be assigned with.
        }
        ProtoAdvtAnnounce(pRef->pFind, "GmUtil", pAddr, "", "TCP:~1:1024\tUDP:~1:1024", 0);
        return(0);
    }

    // mark modules as created by us with our memgroup
    DirtyMemGroupEnter(pRef->iMemGroup, pRef->pMemGroupUserData);

    // create comm module
    pRef->pComm = pConstruct(pRef->iMaxWid, pRef->iMaxInp, pRef->iMaxOut);

    // start connect/listen
    if (pRef->pComm != NULL)
    {
        if (pRef->pComm->Control != NULL)
        {
            pRef->pComm->Control(pRef->pComm, 'clid', pRef->iClientId, NULL);
            pRef->pComm->Control(pRef->pComm, 'rcid', pRef->iRemoteClientId, NULL);
            pRef->pComm->Control(pRef->pComm, 'meta', pRef->iMetatype, NULL);

            if (pRef->iUnackLimit != 0)
            {
                pRef->pComm->Control(pRef->pComm, 'ulmt', pRef->iUnackLimit, NULL);
            }
        }
        if (iConn & NETGAME_CONN_CONNECT)
        {
            iErr = pRef->pComm->Connect(pRef->pComm, pAddr);
        }
        else if (iConn & NETGAME_CONN_LISTEN)
        {
            iErr = pRef->pComm->Listen(pRef->pComm, pAddr);
        }
        pRef->pSocket = pRef->pComm->sockptr;
        // get host ip/port info from commref
        pRef->uHostIp = pRef->pComm->hostip;
        pRef->uHostPort = pRef->pComm->hostport;
    }

    DirtyMemGroupLeave();
    return(iErr);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilComplete

    \Description
        Check for connection complete

    \Input *pRef    - reference pointer

    \Output
        void *      - connection pointer (NULL is no connection)

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
void *NetGameUtilComplete(NetGameUtilRefT *pRef)
{
    // see if we are in find mode
    if (pRef->pFind != NULL)
    {
        // if not connecting, see if we can locate someone
        if (pRef->pComm == NULL)
        {
            char strText[256];
            uint32_t uPeer, uHost;
            uPeer = ProtoAdvtLocate(pRef->pFind, "GmUtil", pRef->strAddr, &uHost, 0);
            if (uPeer != 0)
            {
                NetPrintf(("netgameutil: located peer=%08x, host=%08x\n", uPeer, uHost));
                if (uPeer > uHost)
                {
                    pRef->uHostIp = uPeer;
                    pRef->uPeerIp = uHost;
                    ds_snzprintf(strText, sizeof(strText), "%d.%d.%d.%d%s",
                        (unsigned char)(uPeer>>24), (unsigned char)(uPeer>>16),
                        (unsigned char)(uPeer>> 8), (unsigned char)(uPeer>>0),
                        pRef->strAddr);
                    NetGameUtilConnect(pRef, NETGAME_CONN_CONNECT, strText, pRef->pConstruct);
                }
                else
                {
                    pRef->uHostIp = uHost;
                    pRef->uPeerIp = uPeer;
                    ds_strnzcpy(strText, pRef->strAddr, sizeof(strText));
                    NetGameUtilConnect(pRef, NETGAME_CONN_LISTEN, strText, pRef->pConstruct);
                }
            }
        }
    }

    // check for a connect
    if ((pRef->pComm != NULL) && (pRef->pComm->Status(pRef->pComm) == COMM_ONLINE))
    {
        // get peer ip/port info from commref
        pRef->uPeerIp = pRef->pComm->peerip;
        pRef->uPeerPort = pRef->pComm->peerport;

        // stop any advertising
        if (pRef->pAdvt != NULL)
        {
            ProtoAdvtDestroy(pRef->pAdvt);
            pRef->pAdvt = NULL;
        }
        if (pRef->pFind != NULL)
        {
            ProtoAdvtDestroy(pRef->pFind);
            pRef->pFind = NULL;
        }
        
        NetPrintf(("netgameutil: connection complete\n"));
        return(pRef->pComm);
    }
    else
    {
        return(NULL);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilStatus

    \Description
        Return status info

    \Input *pRef    - reference pointer
    \Input iSelect  - info selector
    \Input *pBuf    - [out] output buffer
    \Input iBufSize - size of output buffer

    \Output
        int32_t     - status info

    \Notes
        iSelect can be one of the following:

        \verbatim
            'host' - TRUE if hosting, else FALSE
            'join' - TRUE if joining, else FALSE
            'hoip' - host ip
            'hprt' - host port
            'peip' - peer ip
            'pprt' - peer port
            'pkrc' - packet received
            'sock' - SocketT socket pointer (in pBuf)
        \endverbatim

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameUtilStatus(NetGameUtilRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    // return host status
    if (iSelect == 'host')
    {
        return(pRef->iHosting == 0);
    }
    if (iSelect == 'join')
    {
        return(pRef->iHosting == 1);
    }
    if (iSelect == 'hoip')
    {
        return(pRef->uHostIp);
    }
    if (iSelect == 'hprt')
    {
        return(pRef->uHostPort);
    }
    if (iSelect == 'pkrc')
    {
        return(pRef->pComm->bpackrcvd);
    }
    if (iSelect == 'peip')
    {
        return(pRef->uPeerIp);
    }
    if (iSelect == 'pprt')
    {
        return(pRef->uPeerPort);
    }
    if ((iSelect == 'sock') && (iBufSize == (signed)sizeof(pRef->pSocket)))
    {
        ds_memcpy(pBuf, &pRef->pSocket, sizeof(pRef->pSocket));
        return(sizeof(pRef->pSocket));
    }
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilAdvert

    \Description
        Send out an advertisement

    \Input *pRef    - reference pointer
    \Input *pKind   - class (unique to app)
    \Input *pName   - name to broadcast
    \Input *pNote   - notes

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
void NetGameUtilAdvert(NetGameUtilRefT *pRef, const char *pKind, const char *pName, const char *pNote)
{
    // see if we need to create module
    if (_NetGameUtilAdvtConstruct(pRef, 16, FALSE) == NULL)
    {
        NetPrintf(("netgameutil: NetGameUtilAdvert() failed to create the ProtoAdvt module.\n"));
        return;
    }

    // save the kind for future queries
    ds_strnzcpy(pRef->strKind, pKind, sizeof(pRef->strKind));

    // start advertising
    ProtoAdvtAnnounce(pRef->pAdvt, pKind, pName, pNote, "TCP:~1:1024\tUDP:~1:1024", pRef->iAdvtFreq);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilWithdraw

    \Description
        Withdraw given advertisement

    \Input *pRef    - reference pointer
    \Input *pKind   - advert kind
    \Input *pName   - advert name

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
void NetGameUtilWithdraw(NetGameUtilRefT *pRef, const char *pKind, const char *pName)
{
    if (pRef->pAdvt != NULL)
    {
        ProtoAdvtCancel(pRef->pAdvt, pKind, pName);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilLocate

    \Description
        Find ip address of a specific advertisement

    \Input *pRef    - reference pointer
    \Input *pKind   - class (unique to app)
    \Input *pName   - advertisement to look for

    \Output
        uint32_t    - ip address of advertiser or zero if no match

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
uint32_t NetGameUtilLocate(NetGameUtilRefT *pRef, const char *pKind, const char *pName)
{
    // auto-create the advert module if needed
    if (_NetGameUtilAdvtConstruct(pRef, 16, FALSE) == NULL)
    {
        NetPrintf(("netgameutil: NetGameUtilLocate() failed to create the ProtoAdvt module.\n"));
        return(0);
    }

    // allow use of default kind
    if (pKind == NULL)
    {
        pKind = pRef->strKind;
    }

    // pass to advertising module
    return(ProtoAdvtLocate(pRef->pAdvt, pKind, pName, NULL, 0));
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameUtilQuery

    \Description
        Return a list of all advertisements

    \Input *pRef    - reference pointer
    \Input *pKind   - class (unique to app)
    \Input *pBuf    - target buffer
    \Input iMax     - target buffer length

    \Output
        int32_t         - number of matching ads

    \Version    01/09/01 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameUtilQuery(NetGameUtilRefT *pRef, const char *pKind, char *pBuf, int32_t iMax)
{
    // auto-create the advert module if needed
    if (_NetGameUtilAdvtConstruct(pRef, 16, FALSE) == NULL)
    {
        NetPrintf(("netgameutil: NetGameUtilQuery() failed to create the ProtoAdvt module.\n"));
        return(0);
    }

    // allow use of default kind
    if (pKind == NULL)
    {
        pKind = pRef->strKind;
    }

    // pass to advert module
    return(ProtoAdvtQuery(pRef->pAdvt, pKind, "", pBuf, iMax, pRef->uLocalAdvt));
}

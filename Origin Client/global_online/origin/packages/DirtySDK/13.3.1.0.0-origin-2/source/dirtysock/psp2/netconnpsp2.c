/*H********************************************************************************/
/*!
    \File netconnpsp2.c

    \Description
        Provides network setup and teardown support. Does not actually create any
        kind of network connections.

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 11/02/2010 (jbrookes) Ported from PS3
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

// required to enable usage of sceRtcConvertDateTimeToTime_t()
#define SCE_RTC_USE_LIBC_TIME_H (1)

#include <stdlib.h>
#include <string.h>
#include <libnetctl.h>
#include <libime.h>
#include <np.h>
#include <netcheck_dialog.h>
#include <libhttp.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtycert.h"
#include "DirtySDK/dirtylang.h" // for locality macros and definitions
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/protoupnp.h"
#include "DirtySDK/friend/friendapi.h"

/*** Defines **********************************************************************/

#define SCE_NP_MANAGER_SIGNIN_ID_MAX_LEN (64-1)

#define NETCONN_NPPOOL_SIZE (128)       //!< default np pool size is 128k

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/
typedef SceNpCommunicationId NetConnPsp2NpTitleId;

typedef struct SceNpManagerSigninId {
    char data[SCE_NP_MANAGER_SIGNIN_ID_MAX_LEN + 1];
} SceNpManagerSigninId;

//! private module state
typedef struct NetConnRefT
{
    // module memory group
    int32_t         iMemGroup;          //!< module mem group id
    void            *pMemGroupUserData; //!< user data associated with mem group

    enum
    {
        ST_INIT,                        //!< initialization
        ST_CONN,                        //!< connecting to internet
        ST_NPCN,                        //!< connecting to np servers
        ST_RTCN,                        //!< connecting to np real-time communication services
        ST_TICK,                        //!< acquiring np ticket
        ST_IDLE,                        //!< active
    } eState;                           //!< internal connection state

    SceCommonDialogStatus eNetCheckDlgStatus; //!< status of connection to real-time communication services
    SceNpServiceState eNpServiceState;  //!< latest np service state
    SceNpAuthRequestId iTktReqId;       //!< only valid when iNpTicketStatus is 0 (pending)

    uint32_t        uNpErrResult;       //!< error result from last np event

    int32_t         iHttpPoolSize;      //!< size mem pool used by libhttp
    int32_t         iSslPoolSize;       //!< size mem pool used by libssl
    uint32_t        uConnStatus;        //!< connection status (surfaced to user)

    #if DIRTYCODE_DEBUG
    int32_t         iNetAuditTimer;     //!< recurring net audit timer
    int32_t         iNetAuditInterval;  //!< recurring net audit interval; 0=disabled, else audit rate in seconds
    #endif

    SceNpId         NpId;               //!< local np id
    SceNpManagerSigninId NpSigninId;    //!< signin id
    SceNpCommunicationSignature NpCommSignature; // NP Communication Signature
    SceNpCommunicationPassphrase NpCommPassphrase; // NP Communication Passphrase

    uint64_t        uNpAccountId;       //!< NP account ID retrieved from ticket

    uint8_t         *pNpTicket;         //!< pointer to currently cached np ticket blob
    int32_t         iNpTicketSize;      //!< size of currently cached np ticket blob
    int32_t         iNpTicketReqResult; //!< result code passed by the system when invoking the NP ticket request callback
    SceNpTime       ticketExpiryTime;   //!< expiry time of currently cached np ticket blob; this is in time_t format, epoch 1970/1/1 00:00:00

    ProtoUpnpRefT   *pProtoUpnp;        //!< protoupnp module state
    FriendApiRefT   *pFriendApiRef;     //!< friend api module state

    int32_t         iPeerPort;          //!< peer port to be opened by upnp; if zero, still find upnp router but don't open a port

    char            strTitleId[16];     //!< sce title id
    char            strCommId[16];      //!< sce np communications id
    char            strSpid[16];        //!< sce service provider id
    char            strServiceId[32];   //!< sce service id (combination of spid and title id)

    // ticket user info
    uint32_t        uLocale;            //!< user country/language code
    uint8_t         uAge;               //!< user age
    uint8_t         bSuspended;         //!< TRUE if user is suspended
    uint8_t         bChatDisabled;      //!< TRUE if chat is disabled
    uint8_t         bContentRating;     //!< content rating

    int8_t          iNpTicketStatus;    //!< ticket status (<0=failed, 0=pending, 1=available)
    uint8_t         bNpTicketRequested; //!< whether a ticket has been requested as part of the current connect flow or not
    uint8_t         bNpTicketCbkDone;   //!< whether the system has invoked the NP ticket request callback for the pending ticket request
    uint8_t         bNpBypassInit;      //!< TRUE if NP was already started (do not need to shut down)
    uint8_t         bNpAuthBypassInit;  //!< TRUE if NP auth was already started (do not need to shut down)
    uint8_t         bHttpLibInitByUs;   //!< TRUE if libhttp utility was initialized by us
    uint8_t         bSslLibInitByUs;    //!< TRUE if libssl utility was initialized by us
    uint8_t         bNetCheckDlgStarted;//!< TRUE if Network Check Dialog is started
    uint8_t         bSilent;            //!< true/false to prevent/enable Network Check Dialog from displaying.
    uint8_t         bNoTicket;          //!< do not try to acquire ticket at startup
    uint8_t         uPlatEnv;           //!< platform environment (NETCONN_PLATENV_*) retrieved from ticket
    uint8_t         bNetCheckDlgCmptl;  //!< whether Network Check Dialog is complete or not
} NetConnRefT;

/*** Function Prototypes **********************************************************/

int sceNpManagerGetSigninId(struct SceNpManagerSigninId *id);

/*** Variables ********************************************************************/

#if DIRTYCODE_LOGGING
static const char *_NpServiceState[] =
{
    "SCE_NP_SERVICE_STATE_UNKNOWN",
    "SCE_NP_SERVICE_STATE_SIGNED_OUT",
    "SCE_NP_SERVICE_STATE_SIGNED_IN",
    "SCE_NP_SERVICE_STATE_ONLINE"
};
#endif

static NetConnRefT  *_NetConn_pRef = NULL;          //!< module state pointer

//! mapping table to map PSN language defines to DirtySock encodings
static uint64_t _NetConn_PSNLanguageMap[19][2] =
{
    { SCE_IME_LANGUAGE_DANISH, LOBBYAPI_LANGUAGE_DANISH },
    { SCE_IME_LANGUAGE_GERMAN, LOBBYAPI_LANGUAGE_GERMAN },
    { SCE_IME_LANGUAGE_ENGLISH_US, LOBBYAPI_LANGUAGE_ENGLISH },
    { SCE_IME_LANGUAGE_SPANISH, LOBBYAPI_LANGUAGE_SPANISH },
    { SCE_IME_LANGUAGE_FRENCH, LOBBYAPI_LANGUAGE_FRENCH },
    { SCE_IME_LANGUAGE_ITALIAN, LOBBYAPI_LANGUAGE_ITALIAN },
    { SCE_IME_LANGUAGE_DUTCH, LOBBYAPI_LANGUAGE_DUTCH },
    { SCE_IME_LANGUAGE_NORWEGIAN, LOBBYAPI_LANGUAGE_NORWEGIAN },
    { SCE_IME_LANGUAGE_POLISH, LOBBYAPI_LANGUAGE_POLISH },
    { SCE_IME_LANGUAGE_PORTUGUESE_PT, LOBBYAPI_LANGUAGE_PORTUGUESE },
    { SCE_IME_LANGUAGE_RUSSIAN, LOBBYAPI_LANGUAGE_RUSSIAN },
    { SCE_IME_LANGUAGE_FINNISH, LOBBYAPI_LANGUAGE_FINNISH },
    { SCE_IME_LANGUAGE_SWEDISH, LOBBYAPI_LANGUAGE_SWEDISH },
    { SCE_IME_LANGUAGE_JAPANESE, LOBBYAPI_LANGUAGE_JAPANESE },
    { SCE_IME_LANGUAGE_KOREAN, LOBBYAPI_LANGUAGE_KOREAN },
    { SCE_IME_LANGUAGE_SIMPLIFIED_CHINESE, LOBBYAPI_LANGUAGE_CHINESE },
    { SCE_IME_LANGUAGE_TRADITIONAL_CHINESE, LOBBYAPI_LANGUAGE_CHINESE },
    { SCE_IME_LANGUAGE_PORTUGUESE_BR, LOBBYAPI_LANGUAGE_PORTUGUESE },
    { SCE_IME_LANGUAGE_ENGLISH_GB, LOBBYAPI_LANGUAGE_ENGLISH }
};


//! mapping table to map PSN country definitions to DirtySock encodings
static uint16_t _NetConn_PSNCountryMap[58][2] =
{
    { 'jp', LOBBYAPI_COUNTRY_JAPAN },
    { 'us', LOBBYAPI_COUNTRY_UNITED_STATES },
    { 'ca', LOBBYAPI_COUNTRY_CANADA },
    { 'tw', LOBBYAPI_COUNTRY_TAIWAN },
    { 'hk', LOBBYAPI_COUNTRY_HONG_KONG },
    { 'gb', LOBBYAPI_COUNTRY_UNITED_KINGDOM },
    { 'ie', LOBBYAPI_COUNTRY_IRELAND },
    { 'be', LOBBYAPI_COUNTRY_BELGIUM },
    { 'lu', LOBBYAPI_COUNTRY_LUXEMBOURG },
    { 'nl', LOBBYAPI_COUNTRY_NETHERLANDS },

    { 'fr', LOBBYAPI_COUNTRY_FRANCE },
    { 'de', LOBBYAPI_COUNTRY_GERMANY },
    { 'at', LOBBYAPI_COUNTRY_AUSTRIA },
    { 'ch', LOBBYAPI_COUNTRY_SWITZERLAND },
    { 'it', LOBBYAPI_COUNTRY_ITALY },
    { 'pt', LOBBYAPI_COUNTRY_PORTUGAL },
    { 'dk', LOBBYAPI_COUNTRY_DENMARK },
    { 'fi', LOBBYAPI_COUNTRY_FINLAND },
    { 'no', LOBBYAPI_COUNTRY_NORWAY },
    { 'se', LOBBYAPI_COUNTRY_SWEDEN },

    { 'au', LOBBYAPI_COUNTRY_AUSTRALIA },
    { 'nz', LOBBYAPI_COUNTRY_NEW_ZEALAND },
    { 'es', LOBBYAPI_COUNTRY_SPAIN },
    { 'ru', LOBBYAPI_COUNTRY_RUSSIAN_FEDERATION },
    { 'ae', LOBBYAPI_COUNTRY_UNITED_ARAB_EMIRATES },
    { 'za', LOBBYAPI_COUNTRY_SOUTH_AFRICA },
    { 'sg', LOBBYAPI_COUNTRY_SINGAPORE },
    { 'kr', LOBBYAPI_COUNTRY_KOREA_DEMOCRATIC_PEOPLES_REPUBLIC_OF },
    { 'bg', LOBBYAPI_COUNTRY_BULGARIA },
    { 'hr', LOBBYAPI_COUNTRY_CROATIA },

    { 'ro', LOBBYAPI_COUNTRY_ROMANIA },
    { 'si', LOBBYAPI_COUNTRY_SLOVENIA },
    { 'cz', LOBBYAPI_COUNTRY_CZECH_REPUBLIC },
    { 'hu', LOBBYAPI_COUNTRY_HUNGARY },
    { 'pl', LOBBYAPI_COUNTRY_POLAND },
    { 'sk', LOBBYAPI_COUNTRY_SLOVAKIA_ },
    { 'tr', LOBBYAPI_COUNTRY_TURKEY },
    { 'sa', LOBBYAPI_COUNTRY_SAUDI_ARABIA },
    { 'bh', LOBBYAPI_COUNTRY_BAHRAIN },
    { 'kw', LOBBYAPI_COUNTRY_KUWAIT },

    { 'lb', LOBBYAPI_COUNTRY_LEBANON },
    { 'om', LOBBYAPI_COUNTRY_OMAN },
    { 'qa', LOBBYAPI_COUNTRY_QATAR },
    { 'il', LOBBYAPI_COUNTRY_ISRAEL },
    { 'mt', LOBBYAPI_COUNTRY_MALTA },
    { 'is', LOBBYAPI_COUNTRY_ICELAND },
    { 'gr', LOBBYAPI_COUNTRY_GREECE },
    { 'cy', LOBBYAPI_COUNTRY_CYPRUS },
    { 'in', LOBBYAPI_COUNTRY_INDIA },
    { 'mx', LOBBYAPI_COUNTRY_MEXICO },

    { 'cl', LOBBYAPI_COUNTRY_CHILE },
    { 'pe', LOBBYAPI_COUNTRY_PERU },
    { 'ar', LOBBYAPI_COUNTRY_ARGENTINA },
    { 'co', LOBBYAPI_COUNTRY_COLOMBIA },
    { 'br', LOBBYAPI_COUNTRY_BRAZIL },
    { 'my', LOBBYAPI_COUNTRY_MALAYSIA },
    { 'id', LOBBYAPI_COUNTRY_INDONESIA },
    { 'th', LOBBYAPI_COUNTRY_THAILAND }
};


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _NetConnCopyParam

    \Description
        Copy a command-line parameter.

    \Input *pDst        - output buffer
    \Input iDstLen      - output buffer length
    \Input *pParamName  - name of parameter to check for
    \Input *pSrc        - input string to look for parameters in
    \Input *pDefault    - default string to use if paramname not found

    \Version 07/18/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetConnCopyParam(char *pDst, int32_t iDstLen, const char *pParamName, const char *pSrc, const char *pDefault)
{
    int32_t iIndex;

    // find parameter
    if ((pSrc = strstr(pSrc, pParamName)) == NULL)
    {
        // copy in default
        ds_strnzcpy(pDst, pDefault, iDstLen);
        return(0);
    }

    // skip parameter name
    pSrc += strlen(pParamName);

    // make sure buffer has enough room
    if (--iDstLen < 0)
    {
        return(0);
    }

    // copy the string
    for (iIndex = 0; (iIndex < iDstLen) && (pSrc[iIndex] != '\0') && (pSrc[iIndex] != ' '); iIndex++)
    {
        pDst[iIndex] = pSrc[iIndex];
    }

    // write null terminator and return number of bytes written
    pDst[iIndex] = '\0';
    return(iIndex);
}

/*F********************************************************************************/
/*!
    \Function    _NetConnParseParams

    \Description
        Parse and handle input parameters to NetConnStartup().

    \Input *pRef    - module state
    \Input *pParams - startup parameters

    \Version 02/23/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnParseParams(NetConnRefT *pRef, const char *pParams)
{
    char strTemp[32];

    // set titleid
    _NetConnCopyParam(pRef->strTitleId, sizeof(pRef->strTitleId), "-titleid=", pParams, "NPXX00245_00");

    NetPrintf(("netconnpsp2: setting titleid=%s\n", pRef->strTitleId));

    // set commid
    _NetConnCopyParam(pRef->strCommId, sizeof(pRef->strCommId), "-commid=", pParams, "NPWR00108");
    NetPrintf(("netconnpsp2: setting commid=%s\n", pRef->strCommId));

    // set spid
    _NetConnCopyParam(pRef->strSpid, sizeof(pRef->strSpid), "-spid=", pParams, "UP0006");
    NetPrintf(("netconnpsp2: setting spid=%s\n", pRef->strSpid));

    // set service id (combination of service provider id and title id)
    ds_strnzcpy(pRef->strServiceId, pRef->strSpid, sizeof(pRef->strServiceId));
    ds_strnzcat(pRef->strServiceId, "-", sizeof(pRef->strServiceId));
    ds_strnzcat(pRef->strServiceId, pRef->strTitleId, sizeof(pRef->strServiceId));
    NetPrintf(("netconnpsp2: setting serviceid=%s\n", pRef->strServiceId));

    // set net poolsize
    _NetConnCopyParam(strTemp, sizeof(strTemp), "-netpool=", pParams, "128");
    SocketControl(NULL, 'pool', strtol(strTemp, NULL, 10) * 1024, NULL, NULL);

    // set psp2 libhttp pool size
    _NetConnCopyParam(strTemp, sizeof(strTemp), "-httppool=", pParams, "40");
    pRef->iHttpPoolSize = strtol(strTemp, NULL, 10) * 1024;
    NetPrintf(("netconnpsp2: setting libhttp pool size=%d bytes\n", pRef->iHttpPoolSize));

    // set psp2 libssl pool size
    _NetConnCopyParam(strTemp, sizeof(strTemp), "-sslpool=", pParams, "300");
    pRef->iSslPoolSize = strtol(strTemp, NULL, 10) * 1024;
    NetPrintf(("netconnpsp2: setting libssl pool size=%d bytes\n", pRef->iSslPoolSize));
}

/*F********************************************************************************/
/*!
    \Function _NetConnGetInterfaceType

    \Description
        Get interface type and return it to caller.

    \Input *pRef    - module state

    \Output
        uint32_t    - interface type bitfield (NETCONN_IFTYPE_*)

    \Version 10/08/2009 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _NetConnGetInterfaceType(NetConnRefT *pRef)
{
    uint32_t uIfType = NETCONN_IFTYPE_ETHER;
    union SceNetCtlInfo Info;
    int32_t iResult;

    // check for pppoe
    if ((iResult = sceNetCtlInetGetInfo(SCE_NET_CTL_INFO_IP_CONFIG, &Info)) == 0)
    {
        if (Info.ip_config == SCE_NET_CTL_IP_PPPOE)
        {
            uIfType |= NETCONN_IFTYPE_PPPOE;
        }
    }
    else
    {
        NetPrintf(("netconnpsp2: sceNetCtlInetGetInfo(SCE_NET_CTL_INFO_IP_CONFIG) failed; err=%s\n", DirtyErrGetName(iResult)));
    }
    // check for wireless
    if ((iResult = sceNetCtlInetGetInfo(SCE_NET_CTL_INFO_DEVICE, &Info)) == 0)
    {
        switch (Info.device)
        {
            case SCE_NET_CTL_DEVICE_WIRELESS:
                uIfType |= NETCONN_IFTYPE_WIRELESS;
                NetPrintf(("netconnpsp2: sceNetCtlInetGetInfo() returned: wireless\n"));
                break;

            case SCE_NET_CTL_DEVICE_WIRED:
                NetPrintf(("netconnpsp2: sceNetCtlInetGetInfo() returned: wired\n"));
                break;

            case SCE_NET_CTL_DEVICE_PHONE:
                /*
                $$todo
                mclouatre  oct 13 2011

                Not sure if the NETCONN_IFTYPE_ETHER flag should be cleared or not in uIfType here.
                The cellular interface is not an "ethernet" connection-less type of interface. It rather is a connection-oriented
                service. However, some implementation may hide the connection-oriented nature of it behind a connection-less wrapper
                to fake "ethernet" activity and to easily integrate with existing networking support of popular OS. For instance, such
                wrapper would return a valid MAC address for the interface even if the interface do really not use that kind of data.

                So, for now, on PSP2 I decide to keep the NETCONN_IFTYPE_ETHER flag set, but that may proved to be
                wrong/unnecessary in the future.
                */
                NetPrintf(("netconnpsp2: sceNetCtlInetGetInfo() returned: phone\n"));
                uIfType |= NETCONN_IFTYPE_CELL;
                break;

            default:
                NetPrintf(("netconnpsp2: warning - unsupported device type (%d) detected\n", Info.device));
                break;
        }
    }
    else
    {
        NetPrintf(("netconnpsp2: sceNetCtlInetGetInfo(SCE_NET_CTL_INFO_DEVICE) failed; err=%s\n", DirtyErrGetName(iResult)));
    }
    return(uIfType);
}

/*F********************************************************************************/
/*!
    \Function _NetConnNpServiceStateCallback

    \Description
        Callback function for receiving the service state

    \Input eNpServiceState  - new service state (SCE_NP_SERVICE_STATE_*)
    \Input iRetCode         - return code
    \Input *pUserData       - netconn state

    \Version 06/16/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnNpServiceStateCallback(SceNpServiceState eNpServiceState, int iRetCode, void *pUserData)
{
    NetConnRefT *pRef = (NetConnRefT *)pUserData;

    NetPrintf(("netconnpsp2: NP service state change (%s -> %s)  [return code = %d] \n", _NpServiceState[pRef->eNpServiceState], _NpServiceState[eNpServiceState], iRetCode));

    pRef->eNpServiceState = eNpServiceState;
}


/*F********************************************************************************/
/*!
    \Function _NetConnNpInit

    \Description
        Initialize Network Platform

    \Input *pRef    - netconn state

    \Output
        int32_t     - result of initialization

    \Version 10/08/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetConnNpInit(NetConnRefT *pRef)
{
    SceNpCommunicationId npCommId;
    SceNpCommunicationConfig npCommConf;
    int32_t iResult;

    // get registered np communications id
    NetConnStatus('npcm', 0, &npCommId, sizeof(npCommId));

    // prepare config
    npCommConf.commId = &npCommId;
    npCommConf.commSignature = &pRef->NpCommSignature;
    npCommConf.commPassphrase = &pRef->NpCommPassphrase;

    // initialize network platform
    if ((iResult = sceNpInit(&npCommConf, NULL)) < 0)
    {
        if (iResult != (signed)SCE_NP_ERROR_ALREADY_INITIALIZED)
        {
            NetPrintf(("netconnpsp2: sceNpInit() failed: err=%s\n", DirtyErrGetName(iResult)));
            return(-1);
        }
        else
        {
            NetPrintf(("netconnpsp2: sceNpInit() returned SCE_NP_ERROR_ALREADY_INITIALIZED... proceeding to next initialization step.\n"));
            pRef->bNpBypassInit = TRUE;
        }
    }
    else
    {
        pRef->bNpBypassInit = FALSE;
    }

    // initialize np auth manager
    if ((iResult = sceNpAuthInit()) < 0)
    {
        if (iResult != (signed)SCE_NP_AUTH_ERROR_ALREADY_INITIALIZED)
        {
            NetPrintf(("netconnpsp2: sceNpAuthInit() failed: err=%s\n", DirtyErrGetName(iResult)));
            sceNpTerm();
            return(-2);
        }
        else
        {
            NetPrintf(("netconnpsp2: sceNpAuthInit() returned SCE_NP_AUTH_ERROR_ALREADY_INITIALIZED... proceeding to next initialization step.\n"));
            pRef->bNpAuthBypassInit = TRUE;
        }
    }
    else
    {
        pRef->bNpAuthBypassInit = FALSE;
    }

    // update the NP Service State once before registering the callback
    if ((iResult = sceNpGetServiceState(&pRef->eNpServiceState)) < 0)
    {
        NetPrintf(("netconnpsp2: sceNpGetServiceState() failed: err=%s\n", DirtyErrGetName(iResult)));
        sceNpAuthTerm();
        sceNpTerm();
        return(-3);
    }
    NetPrintf(("netconnpsp2: initial NP service state detected is %s\n", _NpServiceState[pRef->eNpServiceState]));

    // register the callback function for receiving the service state
    if ((iResult = sceNpRegisterServiceStateCallback(_NetConnNpServiceStateCallback, pRef)) < 0)
    {
        NetPrintf(("netconnpsp2: sceNpRegisterServiceStateCallback() failed: err=%s\n", DirtyErrGetName(iResult)));
        sceNpAuthTerm();
        sceNpTerm();
        return(-4);
    }

    // success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _NetConnNpTerm

    \Description
        Shut down Network Platform

    \Input *pRef    - netconn state

    \Version 10/08/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnNpTerm(NetConnRefT *pRef)
{
    int32_t iResult;

    NetPrintf(("netconnpsp2: shutting down Network Platform\n"));

    if ((iResult = sceNpUnregisterServiceStateCallback()) != 0)
    {
        NetPrintf(("netconnpsp2: sceNpUnregisterServiceStateCallback() failed with err=%s\n", DirtyErrGetName(iResult)));
    }
    pRef->eNpServiceState = SCE_NP_SERVICE_STATE_UNKNOWN;

    if (!pRef->bNpAuthBypassInit)
    {
       if ((iResult = sceNpAuthTerm()) != 0)
        {
            NetPrintf(("netconnpsp2: sceNpAuthTerm() failed with err=%s\n", DirtyErrGetName(iResult)));
        }
    }
    pRef->bNpAuthBypassInit = FALSE;

    if (!pRef->bNpBypassInit)
    {
        if ((iResult = sceNpTerm()) != 0)
        {
            NetPrintf(("netconnpsp2: sceNpTerm() failed with err=%s\n", DirtyErrGetName(iResult)));
        }
    }
    pRef->bNpBypassInit = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _NetConnIsTicketValid

    \Description
        Check whether the ticket we cache internally is valid.

    \Input *pRef        - netconn state
    \Input *pValid      - [OUT] TRUE if ticket exists in cache and is not yet expired; FALSE otherwise

    \Output
        int32_t         - 0=success, negative=failure

    \Notes
        Validity period of a NP ticket is known to be ~ 10 minutes.

        Sony communicated very clear guidelines about usage of NP tickets (https://ps3.scedev.net/forums/thread/121219/):
        game code shall limit ticket requests to minimize stress on NP authentication servers. Good practice
        consists in re-using a previously requested ticket until the PSP2 ticketing API returns that the ticket
        is expired (SCE_NP_ERROR_EXPIRED_TICKET). Sony specifically suggests the following steps:
            1- check size of ticket data in system utility cache: sceNpManagerGetTicket(NULL, &ticketSize);
            2- if ticketSize>0, then a ticket is sitting in the system utility cache; proceed with checking if this
               ticket is expired or not with a call to sceNpManagerGetTicketParam(). Ticket is expired if
               return code is SCE_NP_ERROR_EXPIRED_TICKET
            3- if ticket is expired, then request a new one

        To comply with Sony's guidelines, NetConnPSP2 was improved to no longer request a ticket
        each time NetConnConnect() is invoked. Indeed, the ticket obtained in the previous NetConnConnect()
        call is re-used if not yet expired. However, we use our own caching and expiration detection
        mechanism for that purpose. We cannot use the implementation suggested by Sony because
        the NetConnDisconnect() call that is performed between 2 calls to NetConnConnect() implies usage
        of sceNpTerm() which invalidates the contents of the sytem utility cache.

    \Version 10/21/2011 (mclouatre)
*/
/********************************************************************************F*/
static uint32_t _NetConnIsTicketValid(NetConnRefT *pRef, uint8_t *pValid)
{
    int32_t iResult = 0; // default to success
    SceRtcTick currentTime;

    *pValid = FALSE; // default to not valid

    // make sure we have a ticket in cache
    if (pRef->pNpTicket)
    {
        /*
        Get current time
        From PSP2 doc: You can use the sceRtcGetCurrentNetworkTick() function to obtain the network time that was
        synchronized with the time reported by the server at PlayStation Network sign-in.
        */
        if ((iResult = sceRtcGetCurrentNetworkTick(&currentTime)) == 0)  // current time (UTC) in Tick units (cumulative number of Ticks from 0001/01/01 00:00:00)
        {
            SceDateTime sceDateTime;

            // convert current time from SceRtcTick to SceDateTime
            if ((iResult = sceRtcSetTick(&sceDateTime, &currentTime)) == SCE_OK)
            {
                time_t currentTTime;  // watch out! on PSP2 (unlike PS3), time_t is a 32-bit value

                // convert current time from SceDateTime to time_t
                if ((iResult = sceRtcConvertDateTimeToTime_t(&sceDateTime, &currentTTime)) == SCE_OK)
                {
                    uint64_t u64currentTTime = currentTTime;
                    SceNpTime currentNetworkTime =  u64currentTTime * 1000; // in milliseconds

                    if (currentNetworkTime <= pRef->ticketExpiryTime)
                    {
                        #if DIRTYCODE_LOGGING
                        SceNpTime delta = pRef->ticketExpiryTime - currentNetworkTime;

                        NetPrintf(("netconnpsp2: locally cached ticket is still valid for %lld ms (~ %lld min %lld sec)\n",
                            delta, ((delta/1000)/60), (delta/1000)%60));
                        #endif
                        *pValid = TRUE; // flag ticket as valid
                    }
                    else
                    {
                        NetPrintf(("netconnpsp2: locally cached ticket is expired\n"));
                    }
                }
                else
                {
                    NetPrintf(("netconnpsp2: sceRtcConvertDateTimeToTime_t() failed (err=%s)\n", DirtyErrGetName(iResult)));
                }
            }
            else
            {
                NetPrintf(("netconnpsp2: sceRtcSetTick() failed (err=%s)\n", DirtyErrGetName(iResult)));
            }
        }
        else
        {
            NetPrintf(("netconnpsp2: sceRtcGetCurrentNetworkTick() failed (err=%s)\n", DirtyErrGetName(iResult)));
        }
    }

    return(iResult);
}


/*F********************************************************************************/
/*!
    \Function _NetConnTicketRequest

    \Description
        Request a ticket from NP

    \Input *pRef                - netconn state
    \Input uTicketVersionMajor  - major version component of ticket to request
    \Input uTicketVersionMinor  - minor version component of ticket to request
    \Input *pServiceId          - serviceId (NULL to use default NetConn-registered serviceId)
    \Input *pCookie             - cookie data (optional)
    \Input iCookieSize          - size of cookie data
    \Input pEntitlementId       - id of consumable entitlement (optional)
    \Input uConsumedCount       - number to consume
    \Input *pCallback           - user callback, called when ticket request is complete (or fails)
    \Input *pUserData           - user data for callback

    \Output
        int32_t                 - <0=error, >0=request issued

    \Version 12/08/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetConnTicketRequest(NetConnRefT *pRef, uint32_t uTicketVersionMajor, uint32_t uTicketVersionMinor, const char *pServiceId, const char *pCookie,
    size_t iCookieSize, const char *pEntitlementId, uint32_t uConsumedCount, NetConnTicketCallbackT *pCallback, void *pUserData)
{
    SceNpTicketVersion TicketVersion;
    SceNpAuthRequestParameter TicketRequestParam;
    int32_t iResult;
    int32_t iRetVal;
    const char *pSelectedServiceId;

    // use internally cached serviceId if not specified
    if (pServiceId == NULL)
    {
        pSelectedServiceId = pRef->strServiceId;
    }
    else
    {
        pSelectedServiceId = pServiceId;
    }

    // format ticket version
    TicketVersion.major = uTicketVersionMajor;
    TicketVersion.minor = uTicketVersionMinor;

    // now request a ticket
    NetPrintf(("netconnpsp2: requesting ticket with serviceId=%s, cookie=%s/%d, entitlement=%s, consumedcount=%d\n",
        pSelectedServiceId, pCookie, iCookieSize, pEntitlementId, uConsumedCount));

    memset(&TicketRequestParam, 0, sizeof(TicketRequestParam));
    TicketRequestParam.size = sizeof(TicketRequestParam);
    TicketRequestParam.version.major = SCE_NP_AUTH_LATEST_TICKET_VERSION_MAJOR;
    TicketRequestParam.version.minor = SCE_NP_AUTH_LATEST_TICKET_VERSION_MINOR;
    TicketRequestParam.serviceId = pSelectedServiceId;
    TicketRequestParam.ticketCb = pCallback;
    TicketRequestParam.cbArg = pUserData;
    iResult = sceNpAuthCreateStartRequest(&TicketRequestParam);
    if (iResult >= 0)
    {
        // mark ticket req as pending
        pRef->iNpTicketStatus = 0;
        pRef->bNpTicketCbkDone = FALSE; // reset variable used to track whether ticket request callback has been called
        pRef->iTktReqId = iResult; // save ticket request id

        iRetVal = 1;
    }
    else
    {
        // error
        NetPrintf(("netconnpsp2: sceNpAuthCreateStartRequest() failed (err=%s)\n", DirtyErrGetName(iResult)));
        iRetVal = iResult;
    }

    return(iRetVal);
}

/*F********************************************************************************/
/*!
    \Function _NetConnTicketGetParam

    \Description
        Get a ticket param

    \Input *pTicketData - ticket blob
    \Input iTicketSize  - size of ticket data
    \Input iTicketParam - SCE_NP_TICKET_PARAM_*
    \Input *pTicketParam- [out] storage for ticket param

    \Output
        uint8_t *       - return code from sceNpAuthGetTicketParam()

    \Version 10/06/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetConnTicketGetParam(const uint8_t *pTicketData, int32_t iTicketSize, int32_t iTicketParam, SceNpTicketParam *pTicketParam)
{
    int32_t iResult;

    #if DIRTYCODE_LOGGING
    const char *_strTicketParamNames[SCE_NP_TICKET_PARAM_MAX] =
    {
        "SCE_NP_TICKET_PARAM_SERIAL_ID",            // 0   /* Binary */
        "SCE_NP_TICKET_PARAM_ISSUER_ID",            // 1   /* SceUInt32 */
        "SCE_NP_TICKET_PARAM_ISSUED_DATE",          // 2   /* SceNpTime */
        "SCE_NP_TICKET_PARAM_EXPIRE_DATE",          // 3   /* SceNpTime */
        "SCE_NP_TICKET_PARAM_SUBJECT_ACCOUNT_ID",   // 4   /* SceUInt64 */
        "SCE_NP_TICKET_PARAM_SUBJECT_ONLINE_ID",    // 5   /* String */
        "SCE_NP_TICKET_PARAM_SUBJECT_REGION",       // 6   /* Binary */
        "SCE_NP_TICKET_PARAM_SUBJECT_DOMAIN",       // 7   /* String */
        "SCE_NP_TICKET_PARAM_SERVICE_ID",           // 8   /* String */
        "SCE_NP_TICKET_PARAM_SUBJECT_STATUS",       // 9   /* SceUInt32 */
        "SCE_NP_TICKET_PARAM_STATUS_DURATION",      // 10  /* SceNpTime */
        "SCE_NP_TICKET_PARAM_SUBJECT_DOB"           // 11  /* SceNpDate */
    };
    #endif

    if ((iResult = sceNpAuthGetTicketParam(pTicketData, iTicketSize, iTicketParam, pTicketParam)) < 0)
    {
        NetPrintf(("netconnpsp2: sceNpAuthGetTicketParam(%s) failed; err=%s\n", _strTicketParamNames[iTicketParam], DirtyErrGetName(iResult)));
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetConnTicketGetInfo

    \Description
        Get ticket params from ticket

    \Input *pRef        - netconn state
    \Input *pTicketData - pointer to ticket data
    \Input iTicketSize  - ticket size

    \Output
        uint8_t *       - newly allocated ticket blob, or NULL if there was an error

    \Version 10/06/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnTicketGetInfo(NetConnRefT *pRef, const uint8_t *pTicketData, int32_t iTicketSize)
{
    SceNpTicketParam TicketParam;
    int32_t iResult;

    NetPrintf(("netconnpsp2: processing ticket\n"));

    // init to defaults
    pRef->uNpAccountId = 0;
    pRef->uPlatEnv = NETCONN_PLATENV_PROD;

    // get ticket params
    if ((iResult = _NetConnTicketGetParam(pTicketData, iTicketSize, SCE_NP_TICKET_PARAM_SUBJECT_ACCOUNT_ID, &TicketParam)) == 0)
    {
        pRef->uNpAccountId = TicketParam.u64;
        NetPrintf(("netconnpsp2: accountid=0x%llx\n", TicketParam.u64));
    }
    if ((iResult = _NetConnTicketGetParam(pTicketData, iTicketSize, SCE_NP_TICKET_PARAM_ISSUER_ID, &TicketParam)) == 0)
    {
        uint32_t uIssuerId = TicketParam.u32;

        /*  Reference: https://ps3.scedev.net/support/issue/16396
            IssuerID == 1    --> SPINT Users
            IssuerID == 8    --> PROD_QA Users
            IssuerID == 256  --> NP Users */
        if (uIssuerId == 1)
        {
            pRef->uPlatEnv = NETCONN_PLATENV_TEST;
        }
        else if (uIssuerId == 8)
        {
            pRef->uPlatEnv = NETCONN_PLATENV_CERT;
        }
        else if (uIssuerId != 256)
        {
            NetPrintf(("netconnpsp2: unknown ticket issuer %d\n", uIssuerId));
        }
        NetPrintf(("netconnpsp2: issuerid=%d, env=%d\n", uIssuerId, pRef->uPlatEnv));
    }
    if ((iResult = _NetConnTicketGetParam(pTicketData, iTicketSize, SCE_NP_TICKET_PARAM_SUBJECT_REGION, &TicketParam)) == 0)
    {
        uint32_t uMap, uLanguage, uCountry;
        SceNpCountryCode SceCountryCode;
        uint16_t uSceLanguageCode, uSceCountryCode;

        // get the language code
        uSceLanguageCode = SCE_NP_SUBJECT_REGION_GET_LANGUAGE_CODE(TicketParam.data);
        for (uMap = 0, uLanguage = LOBBYAPI_LANGUAGE_UNKNOWN; uMap < (sizeof(_NetConn_PSNLanguageMap)/sizeof(_NetConn_PSNLanguageMap[0])); uMap += 1)
        {
            if (_NetConn_PSNLanguageMap[uMap][0] == uSceLanguageCode)
            {
                uLanguage = _NetConn_PSNLanguageMap[uMap][1];
                break;
            }
        }
        // get the country code (endian-flipped to match table)
        SCE_NP_SUBJECT_REGION_GET_COUNTRY_CODE(TicketParam.data, &SceCountryCode);
        uSceCountryCode = SceCountryCode.data[0] | (SceCountryCode.data[1] << 8);
        uSceCountryCode = SocketNtohs(uSceCountryCode);
        for (uMap = 0, uCountry = LOBBYAPI_COUNTRY_UNKNOWN; uMap < (sizeof(_NetConn_PSNCountryMap)/sizeof(_NetConn_PSNCountryMap[0])); uMap += 1)
        {
            if (_NetConn_PSNCountryMap[uMap][0] == uSceCountryCode)
            {
                uCountry = _NetConn_PSNCountryMap[uMap][1];
                break;
            }
        }
        // convert to locale
        pRef->uLocale = LOBBYAPI_LocalizerTokenCreate(uLanguage, uCountry);
        NetPrintf(("netconnpsp2: locale= '%c%c%c%c'\n", (uint8_t)(pRef->uLocale >> 24), (uint8_t)(pRef->uLocale >> 16),
            (uint8_t)(pRef->uLocale >> 8), (uint8_t)(pRef->uLocale >> 0)));
    }
    if ((iResult = _NetConnTicketGetParam(pTicketData, iTicketSize, SCE_NP_TICKET_PARAM_SUBJECT_STATUS, &TicketParam)) == 0)
    {
        pRef->uAge = SCE_NP_SUBJECT_STATUS_GET_AGE(TicketParam.u32);
        pRef->bSuspended = SCE_NP_SUBJECT_STATUS_IS_SUSPENDED(TicketParam.u32) ? TRUE : FALSE;
        pRef->bChatDisabled = SCE_NP_SUBJECT_STATUS_IS_CHAT_DISABLED(TicketParam.u32) ? TRUE : FALSE;
        pRef->bContentRating = SCE_NP_SUBJECT_STATUS_CONTENT_RATING(TicketParam.u32) ? TRUE : FALSE;
        NetPrintf(("netconnpsp2: age=%d\n", pRef->uAge));
        NetPrintf(("netconnpsp2: suspended=%s\n", pRef->bSuspended ? "true" : "false"));
        NetPrintf(("netconnpsp2: chat disabled=%s\n", pRef->bChatDisabled ? "true" : "false"));
        NetPrintf(("netconnpsp2: content rating=%s\n", pRef->bContentRating ? "true" : "false"));
    }
    if ((iResult = _NetConnTicketGetParam(pTicketData, iTicketSize, SCE_NP_TICKET_PARAM_SUBJECT_DOB, &TicketParam)) == 0)
    {
        NetPrintf(("netconnpsp2: dob=%02d.%02d.%d\n", TicketParam.date.month, TicketParam.date.day, TicketParam.date.year));
    }
    if ((iResult = _NetConnTicketGetParam(pTicketData, iTicketSize, SCE_NP_TICKET_PARAM_EXPIRE_DATE, &TicketParam)) == 0)
    {
        pRef->ticketExpiryTime = TicketParam.i64;
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnTicketGet

    \Description
        Get ticket data from NP.

    \Input *pRef        - netconn state
    \Input iId          - auth request id
    \Input iTicketSize  - ticket size

    \Output
        uint8_t *       - newly allocated ticket blob, or NULL if there was an error

    \Version 10/06/2009 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t *_NetConnTicketGet(NetConnRefT *pRef, SceNpAuthRequestId iId, int32_t iTicketSize)
{
    uint8_t *pTicketBuf;
    int32_t iResult;

    // allocate memory for the ticket
    if ((pTicketBuf = DirtyMemAlloc(iTicketSize, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconnpsp2: failed to allocate %d bytes for NP ticket\n", iTicketSize));
        return(NULL);
    }

    // get ticket blob
    if ((iResult = sceNpAuthGetTicket(iId, pTicketBuf, iTicketSize)) == 0)
    {
        NetPrintf(("netconnpsp2: np ticket was queried successfully\n"));
        #if DIRTYCODE_LOGGING
        NetPrintMem(pTicketBuf, iTicketSize, "np-ticket");
        #endif

        // get info from ticket
        _NetConnTicketGetInfo(pRef, pTicketBuf, iTicketSize);
    }
    else
    {
        // save result
        pRef->uNpErrResult = (uint32_t)iResult;

        NetPrintf(("netconnpsp2: sceNpManagerGetTicket() failed (err=%s)\n", DirtyErrGetName(iResult)));
        DirtyMemFree(pTicketBuf, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        pTicketBuf = NULL;
    }

    // return ticket to caller
    return(pTicketBuf);
}

/*F********************************************************************************/
/*!
    \Function _NetConnTicketCallback

    \Description
        Get ticket data from NP.

    \Input iId          - auth request id
    \Input iResult      - result code from ticket event
    \Input *pUserData   - netconn ref

    \Notes
        Current assumption: this callback is always called in the same thread as the
        rest of the netconnpsp2 code because sceNpCheckCallback() is invoked from
        _NetConnUpdate()

    \Version 12/08/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnTicketCallback(SceNpAuthRequestId iId, int32_t iResult, void *pUserData)
{
    NetConnRefT *pRef = (NetConnRefT *)pUserData;

    #if DIRTYCODE_LOGGING
    if (pRef->iTktReqId != iId)
    {
        NetPrintf(("netconnpsp2: warning! got notification for a ticket request id (%d) that does not match ours (%d)\n", iId, pRef->iTktReqId));
    }

    if (pRef->bNpTicketCbkDone)
    {
        NetPrintf(("netconnpsp2: warning - got notification for a ticket request before previous one was processed in _NetConnUpdate()\n"));
    }
    #endif

    pRef->bNpTicketCbkDone = TRUE;  // signal that ticket request callback has been invoked by the system
    pRef->iNpTicketReqResult = iResult;  // save result code
}

/*F********************************************************************************/
/*!
    \Function _NetConnDelayedTicketCallback

    \Description
        Delayed processing of ticket request completion. Allows for calling
        sceNpAuthDestroyRequest() from the idler instead of direclty in the context
        of the ticket request callback

    \Input *pRef    - module state
    \Input iResult  - result code

    \Version 02/16/2012 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnDelayedTicketCallback(NetConnRefT *pRef, int32_t iResult)
{
    if (iResult > 0)
    {
        NetPrintf(("netconnpsp2: ticket is available\n"));

        // if we have a previous ticket buffer, dispose of it
        if (pRef->pNpTicket != NULL)
        {
            DirtyMemFree(pRef->pNpTicket, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
            pRef->iNpTicketSize = 0;
            pRef->ticketExpiryTime = 0;
        }
        // get the ticket
        pRef->pNpTicket = _NetConnTicketGet(pRef, pRef->iTktReqId, iResult);
        pRef->iNpTicketSize = iResult;
        // set ticket state to successfully completed
        if (pRef->pNpTicket)
        {
            pRef->iNpTicketStatus = 1;
        }
        else
        {
            pRef->iNpTicketStatus = -1;
        }
    }
    else
    {
        NetPrintf(("netconnpsp2: error %s acquiring ticket\n", DirtyErrGetName(iResult)));
        pRef->iNpTicketStatus = iResult;
    }

    // destroy ticket request
    if ((iResult = sceNpAuthDestroyRequest(pRef->iTktReqId)) != 0)
    {
        NetPrintf(("netconnpsp2: sceNpAuthDestroyRequest(%d) failed with err=%s!\n", pRef->iTktReqId, DirtyErrGetName(iResult)));
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnProcessDelayedNpEvents

    \Description
        Process delayed NP events

    \Input *pRef    - netconn state

    \Version 02/16/2012 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnProcessDelayedNpEvents(NetConnRefT *pRef)
{
    // is there a ticket request completion event to process
    if (!pRef->iNpTicketStatus && pRef->bNpTicketCbkDone)
    {
        _NetConnDelayedTicketCallback(pRef, pRef->iNpTicketReqResult);
        pRef->bNpTicketCbkDone = FALSE;
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnGetNpInfo

    \Description
        Get Network Platform info

    \Input *pRef    - netconn state

    \Output
        int32_t     - result of ticket request operation

    \Version 07/18/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetConnGetNpInfo(NetConnRefT *pRef)
{
    int32_t iResult = 0;

    // get the npid
    if ((iResult = sceNpManagerGetNpId(&pRef->NpId)) == 0)
    {
        NetPrintf(("netconnpsp2: NpId.handle.data=%s\n", pRef->NpId.handle.data));
        NetPrintf(("netconnpsp2: NpId.opt=0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
            pRef->NpId.opt[0], pRef->NpId.opt[1], pRef->NpId.opt[2], pRef->NpId.opt[3],
            pRef->NpId.opt[4], pRef->NpId.opt[5], pRef->NpId.opt[6], pRef->NpId.opt[7]));
        NetPrintf(("netconnpsp2: NpId.reserved=0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
            pRef->NpId.reserved[0], pRef->NpId.reserved[1], pRef->NpId.reserved[2], pRef->NpId.reserved[3],
            pRef->NpId.reserved[4], pRef->NpId.reserved[5], pRef->NpId.reserved[6], pRef->NpId.reserved[7]));
    }
    else
    {
        NetPrintf(("netconnpsp2: sceNpManagerGetNpId() failed: err=%s\n", DirtyErrGetName(iResult)));
    }

    // get signin id
    if ((iResult = sceNpManagerGetSigninId(&pRef->NpSigninId)) == 0)
    {
        NetPrintf(("netconnpsp2: NpSigninId=%s\n", pRef->NpSigninId.data));
    }
    else
    {
        NetPrintf(("netconnpsp2: sceNpManagerGetSigninId() failed: err=%s\n", DirtyErrGetName(iResult)));
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetConnStartNetCheckDialog

    \Description
        Start PSP2 Network Check Dialog

    \Input *pRef    - netconn state

    \Output
        int32_t     - 1 for success, 0 for "try again", negative for failure.

    \Version 04/20/2011 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _NetConnStartNetCheckDialog(NetConnRefT *pRef)
{
    int32_t iResult;
    SceNetCheckDialogParam netCheckDialogParam;

    NetPrintf(("netconnpsp2: starting Network Check Dialog while NP service state is %s\n", _NpServiceState[pRef->eNpServiceState]));

    // initialize libhttp utility (required for the Network Dialog Check)
    if ((iResult = sceHttpInit(pRef->iHttpPoolSize)) < 0)
    {
        if (iResult != (signed)SCE_HTTP_ERROR_ALREADY_INITED)
        {
            NetPrintf(("netconnpsp2: sceHttpInit() failed: err=%s\n", DirtyErrGetName(iResult)));
            return(-1);
        }
        else
        {
            NetPrintf(("netconnpsp2: sceHttpInit() returned SCE_HTTP_ERROR_ALREADY_INITED... proceeding to next initialization step.\n"));
        }
    }
    else
    {
        pRef->bHttpLibInitByUs = TRUE;
    }

    // initialize libssl utility (required for the Network Dialog Check)
    if ((iResult = sceSslInit(pRef->iSslPoolSize)) < 0)
    {
        if (iResult != (signed)SCE_SSL_ERROR_ALREADY_INITED)
        {
            NetPrintf(("netconnpsp2: sceSslInit() failed: err=%s\n", DirtyErrGetName(iResult)));
            return(-2);
        }
        else
        {
            NetPrintf(("netconnpsp2: sceSslInit() returned SCE_SSL_ERROR_ALREADY_INITED... proceeding to next initialization step.\n"));
        }
    }
    else
    {
        pRef->bSslLibInitByUs = TRUE;
    }

    sceNetCheckDialogParamInit(&netCheckDialogParam);
    netCheckDialogParam.mode = SCE_NETCHECK_DIALOG_MODE_PSN_ONLINE;
    if ((iResult = sceNetCheckDialogInit(&netCheckDialogParam)) != 0)
    {
        if (iResult == SCE_COMMON_DIALOG_ERROR_BUSY)
        {
            // PSP2 Net Check Dialog can't be started because there is another dialog displayed on screen
            // try again later
            return(0);
        }
        else
        {
            NetPrintf(("netconnpsp2: sceNetCheckDialogInit() failed with %s\n", DirtyErrGetName(iResult)));
            return(-3);
        }
    }
    else
    {
        pRef->bNetCheckDlgStarted = TRUE;
    }

    return(1);  // success
}

/*F********************************************************************************/
/*!
    \Function _NetConnTerminateNetCheckDialog

    \Description
        Terminate PSP2 Network Check Dialog

    \Input *pRef    - netconn state

    \Output
        int32_t     - 0 for success, negative for failure.

    \Version 04/20/2011 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _NetConnTerminateNetCheckDialog(NetConnRefT *pRef)
{
    int32_t iResult;
    int32_t iRetCode = 0;

    NetPrintf(("netconnpsp2: terminating Network Check Dialog\n"));

    if (pRef->bNetCheckDlgStarted)
    {
        if ((iResult = sceNetCheckDialogTerm()) < 0)
        {
            NetPrintf(("netconnpsp2: sceNetCheckDialogTerm() failed with err=%s\n", DirtyErrGetName(iResult)));
            iRetCode = -1;
        }
    }
    pRef->bNetCheckDlgStarted = FALSE;

    if (pRef->bSslLibInitByUs)
    {
       if ((iResult = sceSslTerm()) != 0)
        {
            NetPrintf(("netconnpsp2: sceSslTerm() failed with err=%s\n", DirtyErrGetName(iResult)));
            iRetCode = -1;
        }
    }
    pRef->bSslLibInitByUs = FALSE;

    if (pRef->bHttpLibInitByUs)
    {
       if ((iResult = sceHttpTerm()) != 0)
        {
            NetPrintf(("netconnpsp2: sceHttpTerm() failed with err=%s\n", DirtyErrGetName(iResult)));
            iRetCode = -1;
        }
    }
    pRef->bHttpLibInitByUs = FALSE;

    return(iRetCode);
}


/*F********************************************************************************/
/*!
    \Function _NetConnOnlineReached

    \Description
        Processing performed when the service state SCE_NP_SERVICE_STATE_ONLINE is reached.

    \Input *pRef    - netconn state

    \Version 05/20/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnOnlineReached(NetConnRefT *pRef)
{
    pRef->uConnStatus = '+npo';  // NP service state: online

    // start upnp discovery
    if (pRef->pProtoUpnp != NULL)
    {
        if (pRef->iPeerPort != 0)
        {
            ProtoUpnpControl(pRef->pProtoUpnp, 'port', pRef->iPeerPort, 0, NULL);
            ProtoUpnpControl(pRef->pProtoUpnp, 'macr', 'upnp', 0, NULL);
        }
        else
        {
            ProtoUpnpControl(pRef->pProtoUpnp, 'macr', 'dscg', 0, NULL);
        }
    }

    // go to state tracking connection to NP real-time communication services
    pRef->eState = ST_TICK;
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdateNPCN

    \Description
        ST_NPCN state processing

    \Input *pRef    - netconn state

    \Version 04/20/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnUpdateNPCN(NetConnRefT *pRef)
{
    int32_t iResult;
    uint32_t uSocketStatus;

    switch (pRef->eNpServiceState)
    {
        case SCE_NP_SERVICE_STATE_UNKNOWN:
        case SCE_NP_SERVICE_STATE_SIGNED_OUT:
        case SCE_NP_SERVICE_STATE_SIGNED_IN:
            if (pRef->bSilent == FALSE)
            {
                pRef->uConnStatus = '~ncd';  // net check dialog in progress

                // start Network Check Dialog (prerequisite to reach the online state)
                iResult = _NetConnStartNetCheckDialog(pRef);
                if (iResult < 0)
                {
                    // error
                    _NetConnTerminateNetCheckDialog(pRef);
                    pRef->uConnStatus = '-npo';
                }
                else if (iResult > 0)
                {
                    // success; go to state for connecting to NP real-time communication services
                    pRef->eState = ST_RTCN;
                }
            }
            else
            {
                // update connection status if socket layer becomes in error state while waiting
                uSocketStatus = SocketInfo(NULL, 'conn', 0, NULL, 0);
                if ((uSocketStatus >> 24) == '-')
                {
                    NetPrintf(("netconnpsp2: socket layer reported error state while waiting for service state to become SCE_NP_SERVICE_STATE_ONLINE\n"));

                    pRef->uConnStatus = uSocketStatus;
                }
            }

            break;

        case SCE_NP_SERVICE_STATE_ONLINE:
            NetPrintf(("netconnpsp2: skipping ST_RTCN state because service state is already SCE_NP_SERVICE_STATE_ONLINE\n"));
            if (_NetConnGetNpInfo(pRef) == 0)
            {
                _NetConnOnlineReached(pRef);
            }
            else
            {
                NetPrintf(("netconnpsp2: error - failed to get network platform info in _NetConnUpdateRTCN()\n"));
                pRef->uConnStatus = '-npo';
            }
            break;

        default:
            NetPrintf(("netconnpsp2: _NetConnUpdateNPCN() detected an invalid NP service state\n"));
            pRef->uConnStatus = '-err';
            break;
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdateRTCN

    \Description
        ST_RTCN state processing

    \Input *pRef    - netconn state

    \Version 04/20/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnUpdateRTCN(NetConnRefT *pRef)
{
    int32_t iResult;

    if(!pRef->bNetCheckDlgCmptl)
    {
        if ((iResult = sceNetCheckDialogGetStatus()) >= 0)
        {
            pRef->eNetCheckDlgStatus = iResult;
            switch (pRef->eNetCheckDlgStatus)
            {
                case SCE_COMMON_DIALOG_STATUS_NONE:
                    NetPrintf(("netconnpsp2: error - Network Check Dialog status should be in progress\n"));
                    pRef->uConnStatus = '-err';
                    break;

                case SCE_COMMON_DIALOG_STATUS_RUNNING:
                    // keep waiting for completion...
                    break;

                case SCE_COMMON_DIALOG_STATUS_FINISHED:
                    {
                        SceNetCheckDialogResult netCheckDlgResult;

                        if ((iResult = sceNetCheckDialogGetResult(&netCheckDlgResult)) >= 0)
                        {
                            if (netCheckDlgResult.result == SCE_COMMON_DIALOG_RESULT_OK)
                            {
                                NetPrintf(("netconnpsp2: Network Check Dialog terminated normally.\n"));
                                pRef->bNetCheckDlgCmptl = TRUE;
                            }
                            else if (netCheckDlgResult.result == SCE_COMMON_DIALOG_RESULT_USER_CANCELED)
                            {
                                NetPrintf(("netconnpsp2: error - Network Check Dialog was cancelled by the user\n"));
                                pRef->uConnStatus = '-err';
                            }
                            else if (netCheckDlgResult.result == SCE_COMMON_DIALOG_RESULT_ABORTED)
                            {
                                NetPrintf(("netconnpsp2: error - Network Check Dialog was aborted\n"));
                                pRef->uConnStatus = '-err';
                            }
                            else if (netCheckDlgResult.result < 0)
                            {
                                NetPrintf(("netconnpsp2: error - Network Check Dialog was terminated because of error %s\n", DirtyErrGetName(netCheckDlgResult.result)));
                                pRef->uConnStatus = '-err';
                            }
                            else
                            {
                                NetPrintf(("netconnpsp2: error - Network Check Dialog was terminated for an unknown reason (%d)\n", netCheckDlgResult.result));
                                pRef->uConnStatus = '-err';
                            }

                            if ((iResult = _NetConnTerminateNetCheckDialog(pRef)) < 0)
                            {
                                pRef->uConnStatus = '-err';
                            }
                        }
                        else
                        {
                            NetPrintf(("netconnpsp2: sceNetCheckDialogGetResult() failed with err=%s\n", DirtyErrGetName(iResult)));
                            pRef->uConnStatus = '-err';
                        }
                    }
                    break;

                default:
                    NetPrintf(("netconnpsp2: error - unknown Net Check Dialog status (%d)\n", pRef->eNetCheckDlgStatus));
                    pRef->uConnStatus = '-err';
                    break;
            }
        }
        else
        {
            NetPrintf(("netconnpsp2: sceNetCheckDialogGetStatus() failed: err=%s\n", DirtyErrGetName(iResult)));
            pRef->uConnStatus = '-err';
        }
    }
    else
    {
        switch (pRef->eNpServiceState)
        {
            case SCE_NP_SERVICE_STATE_UNKNOWN:
            case SCE_NP_SERVICE_STATE_SIGNED_OUT:
                NetPrintf(("netconnpsp2: error - service state went back to %s state\n", _NpServiceState[pRef->eNpServiceState]));
                pRef->uConnStatus = '-err';
                break;

            case SCE_NP_SERVICE_STATE_SIGNED_IN:
                // keep waiting for user to go to ONLINE state
                break;

            case SCE_NP_SERVICE_STATE_ONLINE:
                if (_NetConnGetNpInfo(pRef) == 0)
                {
                    _NetConnOnlineReached(pRef);
                }
                else
                {
                    NetPrintf(("netconnpsp2: error - failed to get network platform info in _NetConnUpdateRTCN()\n"));
                    pRef->uConnStatus = '-npo';
                }
                break;

            default:
                NetPrintf(("netconnpsp2: _NetConnUpdateRTCN() detected an invalid NP service state\n"));
                pRef->uConnStatus = '-err';
                break;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdateTICK

    \Description
        ST_TICK state processing

    \Input *pRef    - netconn state

    \Version 04/20/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnUpdateTICK(NetConnRefT *pRef)
{
    int32_t iResult;

    // request ticket if not already requested and not disabled
    if (!pRef->bNpTicketRequested && !pRef->bNoTicket)
    {
        uint8_t bValid = FALSE;

        // make sure a ticket request is really required
        if ((iResult = _NetConnIsTicketValid(pRef, &bValid)) == 0)
        {
            if (bValid)
            {
                NetPrintf(("netconnpsp2: skipping the ticket request because cached ticket is still valid\n"));

                // set ticket state to successfully completed
                pRef->iNpTicketStatus = 1;
            }
            else
            {
                NetPrintf(("netconnpsp2: initiating ticket request because our cached ticket is empty or invalid\n"));

                if ((iResult = NetConnRequestTicket(3, 0, NULL, NULL, 0, NULL, 0, _NetConnTicketCallback, pRef)) == 1)
                {
                    pRef->bNpTicketRequested = TRUE;
                }
                else if (iResult < 0)
                {
                    pRef->uConnStatus = '-tkt';
                    pRef->iNpTicketStatus = iResult;
                }
                else
                {
                    // try again, there is already a ticket request in progress
                    NetPrintf(("netconnpsp2: initial internal ticket request delayed by an already pending ticket request triggered externally\n"));
                }
            }
        }
    }

    // poll for request completion
    if ((pRef->iNpTicketStatus == 1) || pRef->bNoTicket)
    {
        pRef->uConnStatus = '+onl';
        pRef->eState = ST_IDLE;
    }
    else if (pRef->iNpTicketStatus < 0)
    {
        pRef->uConnStatus = '-tkt';
    }
    else if (pRef->iNpTicketStatus == 0)
    {
        uint32_t uSocketStatus;
        uSocketStatus = SocketInfo(NULL, 'conn', 0, NULL, 0);
        if ((uSocketStatus >> 24) == '-')
        {
            pRef->uConnStatus = uSocketStatus;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdateIDLE

    \Description
        ST_IDLE state processing

    \Input *pRef    - netconn state

    \Version 04/20/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnUpdateIDLE(NetConnRefT *pRef)
{
    #if DIRTYCODE_DEBUG
    if (pRef->iNetAuditInterval < 0)
    {
        SocketInfo(NULL, 'audt', 0, NULL, 0);
        pRef->iNetAuditInterval = 0;
    }
    if (pRef->iNetAuditInterval > 0)
    {
        int32_t iTimer = NetTickDiff(NetTick(), pRef->iNetAuditTimer);
        if (iTimer >= pRef->iNetAuditInterval)
        {
            SocketInfo(NULL, 'audt', 0, NULL, 0);
            pRef->iNetAuditTimer = NetTick();
        }
    }
    #endif

    if (pRef->eNpServiceState == SCE_NP_SERVICE_STATE_ONLINE)
    {
        if ((pRef->uConnStatus >> 24) != '-')
        {
            // check for socket-level disconnection
            pRef->uConnStatus = SocketInfo(NULL, 'conn', 0, NULL, 0);
        }
    }
    else
    {
        // we moved out of online state, mark us as disconnected
        pRef->uConnStatus = '-dsc';
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdate

    \Description
        Update status of NetConn module.  This function is called by NetConnIdle.

    \Input *pData   - pointer to NetConn module ref
    \Input uData    - unused

    \Version 07/18/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnUpdate(void *pData, uint32_t uData)
{
    NetConnRefT *pRef = (NetConnRefT *)pData;

    // update friends
    if (pRef->pFriendApiRef)
    {
        FriendApiUpdate(pRef->pFriendApiRef);
    }

    // process delayed NP events
    _NetConnProcessDelayedNpEvents(pRef);

    // wait for network active status
    if (pRef->eState == ST_CONN)
    {
        pRef->uConnStatus = SocketInfo(NULL, 'conn', 0, NULL, 0);
        if (pRef->uConnStatus == '+onl')
        {
            pRef->eState = ST_NPCN;
            pRef->uConnStatus = '~con';
        }
    }

    // waiting for service state to become SCE_NP_SERVICE_STATE_SIGNED_IN
    if (pRef->eState == ST_NPCN)
    {
        _NetConnUpdateNPCN(pRef);
    }

    // waiting for connetion to NP real-time communication services to complete (i.e. waiting for service state to become SCE_NP_SERVICE_STATE_ONLINE)
    if (pRef->eState == ST_RTCN)
    {
        _NetConnUpdateRTCN(pRef);
    }

    // waiting to acquire ticket information
    if (pRef->eState == ST_TICK)
    {
        _NetConnUpdateTICK(pRef);
    }

    // update connection status while idle
    if (pRef->eState == ST_IDLE)
    {
        _NetConnUpdateIDLE(pRef);
    }

    // if error status, go to idle state from any other state
    if ((pRef->eState != ST_IDLE) && (pRef->uConnStatus >> 24 == '-'))
    {
        pRef->eState = ST_IDLE;
    }

    // update callback status
    sceNpCheckCallback();
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function    NetConnStartup

    \Description
        Bring the network connection module to life. Creates connection with IOP
        resources and gets things ready to go. Puts all device drivers into "probe"
        mode so they look for appropriate hardware. Does not actually start any
        network activity.

    \Input *pParams - startup parameters

    \Output
        int32_t     - zero=success, negative=failure

    \Notes
        pParams can contain the following terms:

        \verbatim
            -netpool=poolsize   - specify sys_net memory pool size in kbytes (default=128k)
            -httppool=poolsize  - specify libhttp mem pool size in kbytes (default=40k)
            -sslpool=poolsize   - specify libssl mem pool size in kbytes (default=300k)
            -spid=spidstr       - specify service provider id (default "UP0006")
            -titleid=titleidstr - specify title id (default "NPXX00245_00")
            -commid=commidstr   - specify np communications id (default "NPWR00108")
            -servicename        - set servicename <game-year-platform> required for SSL use
        \endverbatim

        Note that for testing purposes spid, titleid, and commid can be omitted, but
        every shipping title must specify a real spid and unique titleid for their
        title, and the titleid must be registered with Sony to be able to access
        the np servers.  The commid is required for sceNpBasic functionality
        (buddies).

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnStartup(const char *pParams)
{
    NetConnRefT *pRef = _NetConn_pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allow NULL params
    if (pParams == NULL)
    {
        pParams = "";
    }

    // error see if already started
    if (pRef != NULL)
    {
        NetPrintf(("netconnpsp2: NetConnStartup() called while module is already active\n"));
        return(-1);
    }

    // alloc and init ref
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), NETCONN_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconnpsp2: unable to allocate module state\n"));
        return(-2);
    }
    memset(pRef, 0, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;
    pRef->eState = ST_INIT;
    pRef->iNpTicketStatus = 1;
    pRef->eNpServiceState = SCE_NP_SERVICE_STATE_UNKNOWN;

    // parse and handle input parameters
    _NetConnParseParams(pRef, pParams);

    // create network instance
    SocketCreate(0, 0, SCE_KERNEL_CPU_MASK_USER_0);

    // create the upnp module
    if ((pRef->pProtoUpnp = ProtoUpnpCreate()) == NULL)
    {
        SocketDestroy(0);
        DirtyMemFree(pRef, NETCONN_MEMID, iMemGroup, pMemGroupUserData);
        NetPrintf(("netconnpsp2: unable to create protoupnp module\n"));
        return(-3);
    }

    // create friend api module
    if ((pRef->pFriendApiRef = FriendApiCreate()) == NULL)
    {
        SocketDestroy(0);
        DirtyMemFree(pRef, NETCONN_MEMID, iMemGroup, pMemGroupUserData);
        ProtoUpnpDestroy(pRef->pProtoUpnp);
        NetPrintf(("netconnpsp2: unable to create friend api module\n"));
        return(-4);
    }

    // create and configure dirtycert
    if (NetConnDirtyCertCreate(pParams))
    {
        NetConnShutdown(0);
        return(-5);
    }

    // start up protossl
    if (ProtoSSLStartup() < 0)
    {
        NetConnShutdown(0);
        NetPrintf(("netconnpsp2: unable to startup protossl\n"));
        return(-6);
    }

    // add netconn task handle
    NetConnIdleAdd(_NetConnUpdate, pRef);

    // save ref
    _NetConn_pRef = pRef;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    NetConnQuery

    \Description
        Query the list of available connection configurations. This list is loaded
        from the specified device. The list is returned in a simple fixed width
        array with one string per array element. The caller can find the user portion
        of the config name via strchr(item, '#')+1.

    \Input *pDevice - device to scan (mc0:, mc1:, pfs0:, pfs1:)
    \Input *pList   - buffer to store output array in
    \Input iSize    - length of buffer in bytes

    \Output
        int32_t       - negative=error, else number of configurations

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnQuery(const char *pDevice, NetConfigRecT *pList, int32_t iSize)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    NetConnConnect

    \Description
        Used to bring the networking online with a specific configuration. Uses a
        configuration returned by NetConnQuery.

    \Input *pConfig - unused
    \Input *pOption - optionally provide silent=true/false to prevent/enable net check dialog from displaying.
    \Input iData    - platform-specific

    \Output
        int32_t     - negative=error, zero=success

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnConnect(const NetConfigRecT *pConfig, const char *pOption, int32_t iData)
{
    int32_t iResult = 0;
    NetConnRefT *pRef = _NetConn_pRef;

    NetPrintf(("netconnpsp2: connecting...\n"));

    // check connection options, if present
    if (pRef->eState == ST_INIT)
    {
        pRef->bSilent = TRUE;

        if (pOption != NULL)
        {
            const char *pOpt;

            // find out if we're doing a silent login, or if we're going to bring up the sign-in UI if needed
            if (strstr(pOption, "silent=false") != NULL)
            {
                pRef->bSilent = FALSE;
            }
            // check for specification of peer port
            if ((pOpt = strstr(pOption, "peerport=")) != NULL)
            {
                pRef->iPeerPort = strtol(pOpt+9, NULL, 10);
                NetPrintf(("netconnpsp2: peerport=%d\n", pRef->iPeerPort));
            }
            // check for no-ticket option
            if ((pOpt = strstr(pOption, "noticket")) != NULL)
            {
                pRef->bNoTicket = TRUE;
                NetPrintf(("netconnpsp2: noticket enabled\n"));
            }
        }

        // initialize cellNetCtl
        if ((iResult = SocketControl(NULL, 'conn', 0, NULL, NULL)) == 0)
        {
            // initialize Network Platform
            if ((iResult = _NetConnNpInit(pRef)) != 0)
            {
                return(iResult);
            }

            pRef->eState = ST_CONN;
            pRef->uNpErrResult = 0;
        }

        // set up to request a new ticket
        pRef->bNpTicketRequested = FALSE;
    }
    else
    {
        NetPrintf(("netconnpsp2: NetConnConnect() ignored because already connected!\n"));
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function    NetConnDisconnect

    \Description
        Used to bring down the network connection. After calling this, it would
        be necessary to call NetConnConnect to bring the connection back up or
        NetConnShutdown to completely shutdown all network support.

    \Output
        int32_t     - negative=error, zero=success

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnDisconnect(void)
{
    NetConnRefT *pRef = _NetConn_pRef;

    NetPrintf(("netconnpsp2: disconnecting...\n"));

    // shut down networking
    if (pRef->eState != ST_INIT)
    {
        // shut down network platform
        _NetConnNpTerm(pRef);

        // bring down network interface
        SocketControl(NULL, 'disc', 0, NULL, NULL);

        // reset status
        pRef->eState = ST_INIT;
        pRef->uConnStatus = 0;
        pRef->iNpTicketStatus = 1;
        pRef->bNetCheckDlgCmptl = FALSE;
        pRef->eNetCheckDlgStatus = SCE_COMMON_DIALOG_STATUS_NONE;
    }

    // clear cached NP information
    memset(&pRef->NpId, 0, sizeof(pRef->NpId));
    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetConnControl

    \Description
        Set module behavior based on input selector.

    \Input  iControl    - input selector
    \Input  iValue      - selector input
    \Input  iValue2     - selector input
    \Input *pValue      - selector input
    \Input *pValue2     - selector input

    \Output
        int32_t         - selector result

    \Notes
        iControl can be one of the following:

        \verbatim
            audt: set up network audit; iValue=recurring output interval in seconds or negative for a one-shot (DEBUG ONLY)
            csig: let netconnpsp2 know what the NP Communication Signature is; needs to be use before calling NetConnConnect()
            cpas: let netconnpsp2 know what the NP Communication Passphrase is; needs to be use before calling NetConnConnect()
            snam: set DirtyCert service name
            tick: request an NP ticket; <0=error, 0=retry (a request is already pending), >0=request issued
        \endverbatim

        Unhandled selectors are passed through to SocketControl()

    \Version 04/27/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue, void *pValue2)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // make sure module is started before allowing any other control calls
    if (pRef == NULL)
    {
        NetPrintf(("netconnpsp2: warning - calling NetConnControl() while module is not initialized\n"));
        return(-1);
    }
    #if DIRTYCODE_DEBUG
    // set up network audit
    if (iControl == 'audt')
    {
        pRef->iNetAuditInterval = iValue * 1000;
        pRef->iNetAuditTimer = NetTick();
        return(0);
    }
    #endif
    // set communication signature
    if (iControl == 'csig')
    {
        // iValue is supposed to contain the size of the buffer to which pValue points
        if ((iValue == sizeof(pRef->NpCommSignature)) && (pValue != NULL))
        {
            // fill buffer with NP Communication Signature passed in pValue
            memcpy(&pRef->NpCommSignature, pValue, iValue);

            NetPrintMem(&pRef->NpCommSignature, sizeof(pRef->NpCommSignature), "NpCommunicationSignature");
        }
        else
        {
            NetPrintf(("netconnpsp2: 'csig' selector ignored because input buffer is invalid.\n"));
            return(-1);
        }

        return(0);
    }
    // set communication passphrase
    if (iControl == 'cpas')
    {
        // iValue is supposed to contain the size of the buffer to which pValue points
        if ((iValue == sizeof(pRef->NpCommPassphrase)) && (pValue != NULL))
        {
            // fill buffer with NP Communication Passphrase passed in pValue
            memcpy(&pRef->NpCommPassphrase, pValue, iValue);

            NetPrintMem(&pRef->NpCommPassphrase, sizeof(pRef->NpCommPassphrase), "NpCommunicationPassphrase");
        }
        else
        {
            NetPrintf(("netconnpsp2: 'cpas' selector ignored because input buffer is invalid.\n"));
            return(-1);
        }

        return(0);
    }
    // set dirtycert service name
    if (iControl == 'snam')
    {
        return(DirtyCertControl('snam', 0, 0, pValue));
    }
    // request refresh of NP ticket
    if (iControl == 'tick')
    {
        #if DIRTYCODE_DEBUG
        uint8_t bValid;

        // make sure a ticket request is really required
        if (_NetConnIsTicketValid(pRef, &bValid) == 0)
        {
            if (bValid)
            {
                NetPrintf(("netconnpsp2: warning -- initiating a new ticket request while current ticket is not yet expired - not compliant with Sony's ticket usage guidelines\n"));
            }
        }
        #endif

        // mark old ticket as unavailable
        pRef->iNpTicketSize = 0;
        pRef->ticketExpiryTime = 0;

        // request ticket refresh
        return(NetConnRequestTicket(3, 0, NULL, NULL, 0, NULL, 0, _NetConnTicketCallback, pRef));
    }
    // pass through unhandled selectors to SocketControl()
    return(SocketControl(NULL, iControl, iValue, pValue, pValue2));
}

/*F********************************************************************************/
/*!
    \Function    NetConnStatus

    \Description
        Check general network connection status. Different selectors return
        different status attributes.

    \Input iKind    - status selector ('open', 'conn', 'onln')
    \Input iData    - (optional) selector specific
    \Input *pBuf    - (optional) pointer to output buffer
    \Input iBufSize - (optional) size of output buffer

    \Output
        int32_t     - selector specific

    \Notes
        iKind can be one of the following:

        \verbatim
            acct: signed-in gamer's account id (extracted from initial NP ticket)
            addr: ip address of client
            bbnd: TRUE if broadband, else FALSE
            conn: connection status: +onl=online, ~<code>=in progress, -<err>=NETCONN_ERROR_*
            envi: EA back-end environment type in use (-1=not available, 0=pending)
            fref: returns the friend api module reference
            locl: returns locality for logged-in user (region/language)
            macx: mac address of client (returned in pBuf)
            npcm: NP Communications ID (returned in pBuf)
            npid: NpId of logged in user (returned in pBuf)
            npss: returns NP Service State
            npti: NP Title ID (returned in pBuf)
            onln: true/false indication of whether network is operational
            open: true/false indication of whether network code running
            plug: SCE/Ethernet plug status (negative=error/unknown, 0=disconnected, 1=connected)
            spid: Service Provider ID
            svid: Service ID
            tick: Np ticket for logged in user (returned in pBuf); returns ticket size (-1=error, 0=pending)
            type: connection type: NETCONN_IFTYPE_* bitfield
            upnp: return protoupnp external port info, if available
            vers: return DirtySDK version
        \endverbatim

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnStatus(int32_t iKind, int32_t iData, void *pBuf, int32_t iBufSize)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // initialize output buffer
    if (pBuf != NULL)
    {
        memset(pBuf, 0, iBufSize);
    }

    // see if connected to ISP/LAN
    if (iKind == 'onln')
    {
        return(pRef->uConnStatus == '+onl');
    }

    // see if network code is initialized
    if (iKind == 'open')
    {
        return(pRef != NULL);
    }

    // return DirtySDK version
    if (iKind == 'vers')
    {
        return(DIRTYSDK_VERSION);
    }

    // make sure module is started before allowing any other status calls
    if (pRef == NULL)
    {
        NetPrintf(("netconnpsp2: warning - calling NetConnStatus() while module is not initialized\n"));
        return(-1);
    }

    // return signed-in gamer's account id
    if (iKind == 'acct')
    {
        // make sure we have a valid ticket
        if (pRef->iNpTicketStatus < 1)
        {
            NetPrintf(("netconnpsp2: 'acct' selector failure - currently no valid ticket\n"));
            return(pRef->iNpTicketStatus);
        }

        // make sure we have a valid account id
        if (pRef->uNpAccountId == 0)
        {
            NetPrintf(("netconnpsp2: 'acct' selector failure - NP Account ID is not yet valid\n"));
            return(-1);
        }

        // make sure user's provided buffer is big enough to store 64-bit account id
        if (sizeof(uint64_t) > (uint32_t)iBufSize)
        {
            NetPrintf(("netconnpsp2: 'acct' selector failure - user-provided buffer is too small(%d/%d)\n", iBufSize, sizeof(uint64_t)));
            return(-1);
        }

        // copy account id in user-provided buffer
        memcpy(pBuf, &pRef->uNpAccountId, sizeof(uint64_t));
        return(0);

    }
    // return broadband (TRUE/FALSE)
    if (iKind == 'bbnd')
    {
        return(TRUE);
    }
    // return ability of local user to chat
    if (iKind == 'chat')
    {
        return(pRef->bChatDisabled);
    }
    // connection status
    if (iKind == 'conn')
    {
        return(pRef->uConnStatus);
    }
    // EA back-end environment type in use
    if (iKind == 'envi')
    {
        // make sure we have a valid ticket
        if (pRef->iNpTicketStatus < 1)
        {
            return(pRef->iNpTicketStatus);
        }
        return(pRef->uPlatEnv);
    }
    // return locality code
    if (iKind == 'locl')
    {
        return(pRef->uLocale);
    }
    // friend api ref
    if (iKind == 'fref')
    {
        if ((pBuf == NULL) || (iBufSize < (signed)sizeof(pRef->pFriendApiRef)))
        {
            NetPrintf(("netconnpsp2: NetConnStatus('fref') failed!\n"));
            return(-1);
        }

        // copy ref pointer into user-provided buffer
        memcpy(pBuf, &pRef->pFriendApiRef, sizeof(pRef->pFriendApiRef));
        return(0);
    }
    // signin id
    if ((iKind == 'mail') && (pBuf != NULL) && ((unsigned)iBufSize >= (strlen(pRef->NpSigninId.data) + 1)))
    {
        ds_strnzcpy(pBuf, pRef->NpSigninId.data, iBufSize);
        return(0);
    }
    // np commid
    if ((iKind == 'npcm') && (pBuf != NULL) && (iBufSize == sizeof(SceNpCommunicationId)))
    {
        memset(pBuf, 0, sizeof(SceNpCommunicationId));
        memcpy(pBuf, pRef->strCommId, 9);
        return(0);
    }
    // np service state
    if (iKind == 'npss')
    {
        return(pRef->eNpServiceState);
    }
    // np titleid
    if ((iKind == 'npti') && (pBuf != NULL) && (iBufSize == sizeof(NetConnPsp2NpTitleId)))
    {
        NetConnPsp2NpTitleId *pNpTitleId = (NetConnPsp2NpTitleId *)pBuf;
        memset(pNpTitleId, 0, sizeof(*pNpTitleId));
        memcpy(pNpTitleId->data, pRef->strTitleId, sizeof(pNpTitleId->data));
        return(0);
    }
    // np id
    if (iKind == 'npid')
    {
        if ((pRef->NpId.handle.data[0] != '\0') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(pRef->NpId)))
        {
            memcpy(pBuf, &pRef->NpId, sizeof(pRef->NpId));
            return(0);
        }
        return(-1);
    }
    // sce service provider id
    if ((iKind == 'spid') && (pBuf != NULL) && (iBufSize >= 16))
    {
        memcpy(pBuf, pRef->strSpid, 16);
        return(0);
    }
    // sce service id
    if ((iKind == 'svid') && (pBuf != NULL) && (iBufSize >= 32))
    {
        memcpy(pBuf, pRef->strServiceId, 32);
        return(0);
    }
    // return ticket info (if available)
    if (iKind == 'tick')
    {
        if ((pRef->iNpTicketSize == 0) || (pBuf == NULL))
        {
            return(pRef->iNpTicketSize);
        }
        if (pRef->iNpTicketSize > iBufSize)
        {
            NetPrintf(("netconnpsp2: user ticket buffer is too small (%d/%d)\n", iBufSize, pRef->iNpTicketSize));
            return(-1);
        }
        memcpy(pBuf, pRef->pNpTicket, pRef->iNpTicketSize);
        return(pRef->iNpTicketSize);
    }
    // return interface type (more verbose)
    if (iKind == 'type')
    {
        return(_NetConnGetInterfaceType(pRef));
    }
    // return upnp addportmap info, if available
    if (iKind == 'upnp')
    {
        // if protoupnp is available, and we've added a port map, return the external port for the port mapping
        if ((pRef->pProtoUpnp != NULL) && (ProtoUpnpStatus(pRef->pProtoUpnp, 'stat', NULL, 0) & PROTOUPNP_STATUS_ADDPORTMAP))
        {
            return(ProtoUpnpStatus(pRef->pProtoUpnp, 'extp', NULL, 0));
        }
    }
    // pass unrecognized options to SocketInfo
    return(SocketInfo(NULL, iKind, 0, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function    NetConnShutdown

    \Description
        Shutdown the network code and return to idle state.

    \Input  uShutdownFlags  - shutdown configuration flags

    \Output
        int32_t             - zero

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnShutdown(uint32_t uShutdownFlags)
{
    // make sure we've been started
    if (_NetConn_pRef == NULL)
    {
        return(-1);
    }

    // destroy the friend api ref
    if (_NetConn_pRef->pFriendApiRef)
    {
        FriendApiDestroy(_NetConn_pRef->pFriendApiRef);
        _NetConn_pRef->pFriendApiRef = NULL;
    }

    // destroy the upnp ref
    if (_NetConn_pRef->pProtoUpnp != NULL)
    {
        ProtoUpnpDestroy(_NetConn_pRef->pProtoUpnp);
        _NetConn_pRef->pProtoUpnp = NULL;
    }

    // shut down protossl
    ProtoSSLShutdown();

    // destroy the dirtycert module
    DirtyCertDestroy();

    // remove netconn idle task
    NetConnIdleDel(_NetConnUpdate, _NetConn_pRef);

    // shut down Idle handler
    NetConnIdleShutdown();

    // disconnect the interfaces
    NetConnDisconnect();

    // shutdown the network code
    SocketDestroy(0);

    // disposed of any cached ticket we might have
    if (_NetConn_pRef->pNpTicket != NULL)
    {
        DirtyMemFree(_NetConn_pRef->pNpTicket, NETCONN_MEMID, _NetConn_pRef->iMemGroup, _NetConn_pRef->pMemGroupUserData);
        _NetConn_pRef->pNpTicket = NULL;
        _NetConn_pRef->iNpTicketSize = 0;
        _NetConn_pRef->ticketExpiryTime = 0;
    }

    // dispose of ref
    DirtyMemFree(_NetConn_pRef, NETCONN_MEMID, _NetConn_pRef->iMemGroup, _NetConn_pRef->pMemGroupUserData);
    _NetConn_pRef = NULL;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    NetConnSleep

    \Description
        Sleep the application for some number of milliseconds.

    \Input iMilliSecs    - number of milliseconds to block for

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
void NetConnSleep(int32_t iMilliSecs)
{
    sceKernelDelayThread(iMilliSecs*1000);
}


/*F********************************************************************************/
/*!
    \Function NetConnRequestTicket

    \Description
        Request a ticket from NP

    \Input uTicketVersionMajor  - major version component of ticket to request
    \Input uTicketVersionMinor  - minor version component of ticket to request
    \Input *pServiceId          - serviceId (NULL to use default NetConn-registered serviceId)
    \Input *pCookie             - cookie (optional)
    \Input iCookieSize          - size of cookie, if specified
    \Input pEntitlementId       - entitlement id (optional)
    \Input uConsumedCount       - number to consume
    \Input *pCallback           - user callback, called when ticket request is complete (or fails)
    \Input *pUserData           - user data for callback

    \Output
        int32_t                 - <0=error, 0=retry (a request is already pending), >0=request issued

    \Version 12/08/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnRequestTicket(uint32_t uTicketVersionMajor, uint32_t uTicketVersionMinor, const char *pServiceId, const char *pCookie,
    size_t iCookieSize, const char *pEntitlementId, uint32_t uConsumedCount, NetConnTicketCallbackT *pCallback, void *pUserData)
{
    NetConnRefT *pRef = _NetConn_pRef;
    // make sure we're in a state where we can request a ticket
    if (pRef->iNpTicketStatus == 0)
    {
        return(pRef->iNpTicketStatus);
    }
    // request the ticket
    return(_NetConnTicketRequest(pRef, uTicketVersionMajor, uTicketVersionMinor, pServiceId, pCookie, iCookieSize, pEntitlementId, uConsumedCount, pCallback, pUserData));
}

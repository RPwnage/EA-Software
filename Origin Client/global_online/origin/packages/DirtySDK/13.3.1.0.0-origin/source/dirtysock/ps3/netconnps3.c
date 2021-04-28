/*H********************************************************************************/
/*!
    \File netconnps3.c

    \Description
        Provides network setup and teardown support. Does not actually create any
        kind of network connections.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 10/04/2005 (jbrookes) First Version
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <system_types.h>
#include <sys/timer.h>
#include <sysutil/sysutil_common.h>
#include <sdk_version.h>
#include <netex/libnetctl.h>
#include <np.h>

#include <cell/sysmodule.h>

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
#define NETCONN_MAX_SUBUSER (4)

//! initial size of external cleanup list (in number of entries)
#define NETCONN_EXTERNAL_CLEANUP_LIST_INITIAL_CAPACITY (12)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct SceNpManagerSigninId {
    char data[SCE_NP_MANAGER_SIGNIN_ID_MAX_LEN + 1];
} SceNpManagerSigninId;

typedef int32_t (*NetConnExternalCleanupCallbackT)(void *pNetConnExternalCleanupData);

typedef struct NetConnExternalCleanupEntryT
{
    void *pCleanupData;                         //!< pointer to data to be passed to the external cleanup callback
    NetConnExternalCleanupCallbackT  pCleanupCb;//!< external cleanup callback
} NetConnExternalCleanupEntryT;

//! private module state
typedef struct NetConnRefT
{
    // module memory group
    int32_t         iMemGroup;          //!< module mem group id
    void            *pMemGroupUserData; //!< user data associated with mem group

    NetConnExternalCleanupEntryT
                            *pExternalCleanupList; //!< pointer to an array of entries pending external cleanup completion

    int32_t                 iExternalCleanupListMax; //!< maximum number of entries in the array (in number of entries)
    int32_t                 iExternalCleanupListCnt; //!< number of valid entries in the array

    enum
    {
        ST_EXT_CLEANUP,                  //!< cleaning up external instances from previous NetConn connection, can't proceed before all cleaned up
        ST_INIT,                        //!< initialization
        ST_WAIT_SUTL,                   //!< Wait for the sysutil to be finished
        ST_CONN,                        //!< connecting to internet
        ST_NPCN,                        //!< connecting to np servers
        ST_TICK,                        //!< acquiring np ticket
        ST_IDLE,                        //!< active
    } eState;                           //!< internal connection state

    NetCritT        callbackCrit;       //!< critical section used to ensure thread safety in handlers registered with sony apis

    int32_t         iNpPoolSize;        //!< size of np pool
    void            *pNpPoolMem;        //!< np pool memory

    int32_t         iNpStatus;          //!< latest np status
    int32_t         iLastNpStatus;      //!< previous np status
    uint32_t        uNpErrResult;       //!< error result from last np event

    uint32_t        uSysutilCbSlot;     //!< callback slot for dirtySDK to use 0-3 (0 by default)
    uint32_t        uConnStatus;        //!< connection status (surfaced to user)

    #if DIRTYCODE_DEBUG
    int32_t         iNetAuditTimer;     //!< recurring net audit timer
    int32_t         iNetAuditInterval;  //!< recurring net audit interval; 0=disabled, else audit rate in seconds
    #endif

    SceNpId         NpId;               //!< local np id
    SceNpManagerSigninId NpSigninId;    //!< signin id
    uint32_t        uSubUserSigninSlot; //!< slot a subuser is getting signed into at the moment
    uint8_t         uSubUserSigninInProgress; //!< whether a subuser is getting signed in
    NetConnSubSigninCallbackT *pSubUserSigninCallback; //!< the client callback for sub-sign-in status
    void            *pSubUserSigninData; //!< user data for sub-sign-in callback
    uint8_t         aSubUserSignedIn[NETCONN_MAX_SUBUSER]; //!< whether a subuser is signed in at this slot

    uint64_t        aNpAccountId[NETCONN_MAX_SUBUSER];       //!< NP account ID retrieved from ticket

    NetConnTicketCallbackT *pTicketCallback;    //!< ticket response user callback
    void            *pTicketUserData;   //!< user data for ticket callback

    uint8_t         *pNpTicket[NETCONN_MAX_SUBUSER + 1];         //!< pointer to currently cached np ticket blob
    int32_t         iNpTicketSize[NETCONN_MAX_SUBUSER + 1];      //!< size of currently cached np ticket blob
    SceNpTime       ticketExpiryTime[NETCONN_MAX_SUBUSER + 1];   //!< expiry time of currently cached np ticket blob; this is in time_t format, epoch 1970/1/1 00:00:00

    ProtoUpnpRefT   *pProtoUpnp;        //!< protoupnp module state

    FriendApiRefT   *pFriendApiRef;     //!< friend api module state

    int32_t         iPeerPort;          //!< peer port to be opened by upnp; if zero, still find upnp router but don't open a port

    /*
    The following five variables are to be accessed safely within callbackCrit. They are all used in the
    NetConnPS3 code and in the callbacks registered with the NP manager utility and the sysutil library.
    So they need to be protected with a critical section to guarantee thread safety
     */
    int32_t         iTSafeOfflineResult;//!< NP manager conn status offline event result
    int32_t         iTSafeTktReqResult; //!< NP manager got ticket event result
    int8_t          bTSafeOfflineEvent; //!< TRUE if NP manager offline event to be processed
    int8_t          iTSafeTktReqStatus; //!< ticket request async status (-1=idle, 0=pending, 1=completed)
    int8_t          iTSafeStrtDlgStatus;//!< start dialog async status (-1=idle, 0=pending, 1=completed)

    uint8_t         bCellSysutilCheckCb;//!< whether netconn calls cellSysutilCheckCallback()
    uint8_t         bConnectDelayed;    //!< if TRUE, NetConnConnect is being delayed until external cleanup phase completes
    uint8_t         bSilent;            //!< true/false to prevent/enable sysutils from displaying.
    uint8_t         bNoTicket;          //!< do not try to acquire ticket at startup

    uint8_t         bNpTicketRequested; //!< whether a ticket has been requested as part of the current connect flow or not
    int8_t          iNpTicketStatus;    //!< ticket status (<0=failed, 0=pending, 1=available)
    uint8_t         uPlatEnv;           //!< platform environment (NETCONN_PLATENV_*) retrieved from ticket

    char            strTitleId[16];     //!< sce title id
    char            strCommId[16];      //!< sce np communications id
    char            strSpid[16];        //!< sce service provider id
    char            strServiceId[32];   //!< sce service id (combination of spid and title id)
    SceNpId         aSubUsers[NETCONN_MAX_SUBUSER];       //!< NP Ids of signed-in sub-users

} NetConnRefT;

/*** Function Prototypes **********************************************************/

int sceNpManagerGetSigninId(struct SceNpManagerSigninId *id);

/*** Variables ********************************************************************/

#if DIRTYCODE_LOGGING
static const char *_NpStatus[] =
{
    "SCE_NP_MANAGER_STATUS_OFFLINE",            // -1
    "SCE_NP_MANAGER_STATUS_GETTING_TICKET",     // 0
    "SCE_NP_MANAGER_STATUS_GETTING_PROFILE",    // 1
    "SCE_NP_MANAGER_STATUS_LOGGING_IN",         // 2
    "SCE_NP_MANAGER_STATUS_ONLINE",             // 3
};
#endif

static NetConnRefT  *_NetConn_pRef = NULL;          //!< module state pointer

//! mapping table to map PS3 language defines to DirtySock encodings
#if CELL_SDK_VERSION < 0x400001
static uint16_t _NetConn_Ps3LanguageMap[16][2] =
#else
static uint16_t _NetConn_Ps3LanguageMap[18][2] =
#endif
{
    { CELL_SYSUTIL_LANG_JAPANESE, LOBBYAPI_LANGUAGE_JAPANESE },
#if CELL_SDK_VERSION < 0x400001
    { CELL_SYSUTIL_LANG_ENGLISH, LOBBYAPI_LANGUAGE_ENGLISH },
#else
    { CELL_SYSUTIL_LANG_ENGLISH_US, LOBBYAPI_LANGUAGE_ENGLISH },
    { CELL_SYSUTIL_LANG_ENGLISH_GB, LOBBYAPI_LANGUAGE_ENGLISH },
#endif
    { CELL_SYSUTIL_LANG_FRENCH, LOBBYAPI_LANGUAGE_FRENCH },
    { CELL_SYSUTIL_LANG_SPANISH, LOBBYAPI_LANGUAGE_SPANISH },
    { CELL_SYSUTIL_LANG_GERMAN, LOBBYAPI_LANGUAGE_GERMAN },
    { CELL_SYSUTIL_LANG_ITALIAN, LOBBYAPI_LANGUAGE_ITALIAN },
    { CELL_SYSUTIL_LANG_DUTCH, LOBBYAPI_LANGUAGE_DUTCH },
#if CELL_SDK_VERSION < 0x400001
    { CELL_SYSUTIL_LANG_PORTUGUESE, LOBBYAPI_LANGUAGE_PORTUGUESE },
#else
    { CELL_SYSUTIL_LANG_PORTUGUESE_PT, LOBBYAPI_LANGUAGE_PORTUGUESE },
    { CELL_SYSUTIL_LANG_PORTUGUESE_BR, LOBBYAPI_LANGUAGE_PORTUGUESE },
#endif
    { CELL_SYSUTIL_LANG_RUSSIAN, LOBBYAPI_LANGUAGE_RUSSIAN },
    { CELL_SYSUTIL_LANG_KOREAN, LOBBYAPI_LANGUAGE_KOREAN },
    { CELL_SYSUTIL_LANG_CHINESE_T, LOBBYAPI_LANGUAGE_CHINESE },
    { CELL_SYSUTIL_LANG_CHINESE_S, LOBBYAPI_LANGUAGE_CHINESE },
    { CELL_SYSUTIL_LANG_FINNISH, LOBBYAPI_LANGUAGE_FINNISH },
    { CELL_SYSUTIL_LANG_SWEDISH, LOBBYAPI_LANGUAGE_SWEDISH },
    { CELL_SYSUTIL_LANG_DANISH, LOBBYAPI_LANGUAGE_DANISH },
    { CELL_SYSUTIL_LANG_NORWEGIAN, LOBBYAPI_LANGUAGE_NORWEGIAN }
};


//! mapping table to map PS3 country definitions to DirtySock encodings
static uint16_t _NetConn_Ps3CountryMap[58][2] =
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
    \Function _NetConnConvertCountryCode

    \Description
        Convert NP country code into a 2-char code.

    \Input sceCountryCode   - country code

    \Output
        uint32_t            - converted country code

    \Version 02/23/2010 (mclouatre)
*/
/********************************************************************************F*/
static uint32_t _NetConnConvertCountryCode(SceNpCountryCode sceCountryCode)
{
    uint32_t uConvertedCountryCode = 0;

    uConvertedCountryCode = sceCountryCode.data[0] << 8;
    uConvertedCountryCode |= sceCountryCode.data[1];

    return(uConvertedCountryCode);
}


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
    NetPrintf(("netconnps3: setting titleid=%s\n", pRef->strTitleId));

    // set commid
    _NetConnCopyParam(pRef->strCommId, sizeof(pRef->strCommId), "-commid=", pParams, "NPWR00108");
    NetPrintf(("netconnps3: setting commid=%s\n", pRef->strCommId));

    // set spid
    _NetConnCopyParam(pRef->strSpid, sizeof(pRef->strSpid), "-spid=", pParams, "UP0006");
    NetPrintf(("netconnps3: setting spid=%s\n", pRef->strSpid));

    // set service id (combination of service provider id and title id)
    ds_strnzcpy(pRef->strServiceId, pRef->strSpid, sizeof(pRef->strServiceId));
    ds_strnzcat(pRef->strServiceId, "-", sizeof(pRef->strServiceId));
    ds_strnzcat(pRef->strServiceId, pRef->strTitleId, sizeof(pRef->strServiceId));
    NetPrintf(("netconnps3: setting serviceid=%s\n", pRef->strServiceId));

    // set net poolsize
    _NetConnCopyParam(strTemp, sizeof(strTemp), "-netpool=", pParams, "128");
    SocketControl(NULL, 'pool', strtol(strTemp, NULL, 10) * 1024, NULL, NULL);

    // set np poolsize
    _NetConnCopyParam(strTemp, sizeof(strTemp), "-nppool=", pParams, "128");
    pRef->iNpPoolSize = strtol(strTemp, NULL, 10) * 1024;
    NetPrintf(("netconnps3: setting np pool size=%d bytes\n", pRef->iNpPoolSize));
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
    union CellNetCtlInfo Info;
    int32_t iResult;

    // check for pppoe
    if ((iResult = cellNetCtlGetInfo(CELL_NET_CTL_INFO_IP_CONFIG, &Info)) == 0)
    {
        if (Info.ip_config == CELL_NET_CTL_IP_PPPOE)
        {
            uIfType |= NETCONN_IFTYPE_PPPOE;
        }
    }
    else
    {
        NetPrintf(("netconnps3: cellNetCtlGetInfo(IP_CONFIG) failed; err=%s\n", DirtyErrGetName(iResult)));
    }
    // check for wireless
    if ((iResult = cellNetCtlGetInfo(CELL_NET_CTL_INFO_DEVICE, &Info)) == 0)
    {
        if (Info.device == CELL_NET_CTL_DEVICE_WIRELESS)
        {
            uIfType |= NETCONN_IFTYPE_WIRELESS;
        }
    }
    else
    {
        NetPrintf(("netconnps3: cellNetCtlGetInfo(IP_CONFIG) failed; err=%s\n", DirtyErrGetName(iResult)));
    }
    return(uIfType);
}

/*F********************************************************************************/
/*!
    \Function _NetConnGetAccountId

    \Description
        Extracts signed-in user's account id from the NP ticket.

    \Input *pRef    - module state

    \Output
        uint64_t    - positive = success (account id); 0 = not available

    \Version 06/02/2010 (mclouatre)
*/
/********************************************************************************F*/
static uint64_t _NetConnGetAccountId(NetConnRefT *pRef)
{
    union SceNpTicketParam TicketParam;
    uint64_t uNpAccountId = 0; // default to "not available"
    int32_t iResult;

    // get account id
    if ((iResult = sceNpManagerGetTicketParam(SCE_NP_TICKET_PARAM_SUBJECT_ACCOUNT_ID, &TicketParam)) == 0)
    {
        uNpAccountId = TicketParam.u64;
    }
    else
    {
        NetPrintf(("netconnps3: failed to extract Account ID from ticket (error = %s)\n", DirtyErrGetName(iResult)));
    }

    return(uNpAccountId);
}


/*F********************************************************************************/
/*!
    \Function _NetConnGetPlatformEnvironment

    \Description
        Uses ticket issuer to determine platform environment.

    \Input *pRef    - module state

    \Output
        uint32_t    - platform environment (NETCONN_PLATENV_*)

    \Version 10/07/2009 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _NetConnGetPlatformEnvironment(NetConnRefT *pRef)
{
    union SceNpTicketParam TicketParam;
    uint32_t uPlatEnv = NETCONN_PLATENV_PROD;   // default to production environment
    int32_t iResult;

    // get ticket issuer id
    if ((iResult = sceNpManagerGetTicketParam(SCE_NP_TICKET_PARAM_ISSUER_ID, &TicketParam)) == 0)
    {
        uint32_t uIssuerId = TicketParam.u32;

        /*  Reference: https://ps3.scedev.net/support/issue/16396
            IssuerID == 1    --> SPINT Users
            IssuerID == 8    --> PROD_QA Users
            IssuerID == 256  --> NP Users */
        if (uIssuerId == 1)
        {
            uPlatEnv = NETCONN_PLATENV_TEST;
        }
        else if (uIssuerId == 8)
        {
            uPlatEnv = NETCONN_PLATENV_CERT;
        }
        else if (uIssuerId != 256)
        {
            NetPrintf(("netconnps3: unknown ticket issuer %d\n", uIssuerId));
        }
    }
    else
    {
        NetPrintf(("netconnps3: sceNpManagerGetTicketParam(SCE_NP_TICKET_PARAM_ISSUER_ID) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    // return platenv to caller
    return(uPlatEnv);
}

/*F********************************************************************************/
/*!
    \Function _NetConnAddToExternalCleanupList

    \Description
        Add an entry to the list of external module pending successful cleanup.

    \Input *pRef            - netconn module state
    \Input *pCleanupCb      - cleanup callback
    \Input *pCleanupData    - data to be passed to cleanup callback

    \Output
        uint32_t            -  0 for success; -1 for failure.

    \Version 10/01/2011 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _NetConnAddToExternalCleanupList(NetConnRefT *pRef, NetConnExternalCleanupCallbackT pCleanupCb, void *pCleanupData)
{
    // if list if full, double its size.
    if (pRef->iExternalCleanupListCnt == pRef->iExternalCleanupListMax)
    {
        NetConnExternalCleanupEntryT *pNewList;
        NetConnExternalCleanupEntryT *pOldList = pRef->pExternalCleanupList;

        // allocate new external cleanup list
        if ((pNewList = DirtyMemAlloc(2 * pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT), NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) == NULL)
        {
            NetPrintf(("netconnps3: unable to allocate memory for the external cleanup list\n"));
            return(-1);
        }
        memset(pNewList, 0, pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT));

        // copy contents of old list in contents of new list
        memcpy(pNewList, pOldList, pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT));

        // replace old list with new list
        pRef->pExternalCleanupList = pNewList;
        pRef->iExternalCleanupListMax = pRef->iExternalCleanupListMax * 2;

        // free old list
        DirtyMemFree(pOldList, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    }

    // add new entry to the list
    pRef->pExternalCleanupList[pRef->iExternalCleanupListCnt].pCleanupCb = pCleanupCb;
    pRef->pExternalCleanupList[pRef->iExternalCleanupListCnt].pCleanupData = pCleanupData;
    pRef->iExternalCleanupListCnt += 1;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _NetConnProcessExternalCleanupList

    \Description
        Walk external cleanup list and try to destroy each individual entry.

    \Input *pRef     - netconn module state

    \Version 09/30/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnProcessExternalCleanupList(NetConnRefT *pRef)
{
    int32_t iEntryIndex;
    int32_t iEntryIndex2;

    for (iEntryIndex = 0; iEntryIndex < pRef->iExternalCleanupListMax; iEntryIndex++)
    {
        if (pRef->pExternalCleanupList[iEntryIndex].pCleanupCb == NULL)
        {
            // no more entry in list
            break;
        }

        if(pRef->pExternalCleanupList[iEntryIndex].pCleanupCb(pRef->pExternalCleanupList[iEntryIndex].pCleanupData) == 0)
        {
            pRef->iExternalCleanupListCnt -= 1;

            NetPrintf(("netconnps3: successfully destroyed external module (cleanup data ptr = 0x%08x), external cleanup list count decremented to %d\n",
                pRef->pExternalCleanupList[iEntryIndex].pCleanupData, pRef->iExternalCleanupListCnt));

            // move all following entries one cell backward in the array
            for(iEntryIndex2 = iEntryIndex; iEntryIndex2 < pRef->iExternalCleanupListMax; iEntryIndex2++)
            {
                if (iEntryIndex2 == pRef->iExternalCleanupListMax-1)
                {
                    // last entry, reset to NULL
                    pRef->pExternalCleanupList[iEntryIndex2].pCleanupCb = NULL;
                    pRef->pExternalCleanupList[iEntryIndex2].pCleanupData = NULL;
                }
                else
                {
                    pRef->pExternalCleanupList[iEntryIndex2].pCleanupCb = pRef->pExternalCleanupList[iEntryIndex2+1].pCleanupCb;
                    pRef->pExternalCleanupList[iEntryIndex2].pCleanupData = pRef->pExternalCleanupList[iEntryIndex2+1].pCleanupData;
                }
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnNpManagerCallback

    \Description
        Network Platform event callback

    \Input iEvent   - network event (SCE_NP_MANAGER_EVENT_*)
    \Input iResult  - result status code
    \Input *pArg    - netconn state

    \Notes
        This callback is now thread-safe. It uses a critical section to access
        variables that are shared with other functions of NetConnPs3. Thread-safety
        is required because we cannot guarantee that cellSysutilCheckCallback()
        is going to be called from the same thread as DirtySDK... and that function
        is the one that internally invokes the callbacks we register with the PS3
        system.

    \Version 07/18/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnNpManagerCallback(int32_t iEvent, int32_t iResult, void *pArg)
{
    NetConnRefT *pRef = (NetConnRefT *)pArg;

#if DIRTYCODE_LOGGING
    const char *pEventStr = "SCE_NP_MANAGER_EVENT_GOT_TICKET";
    if (iEvent != SCE_NP_MANAGER_EVENT_GOT_TICKET)
    {
        if ((unsigned)(iEvent+1) < (sizeof(_NpStatus)/sizeof(_NpStatus[0])))
        {
            pEventStr = _NpStatus[iEvent+1];
        }
        else
        {
            pEventStr = "is unknown";
        }
    }
    NetPrintf(("netconnps3: np event %s(%d), result %s\n", pEventStr, iEvent, DirtyErrGetName(iResult)));
#endif

    // the following variables can only be accessed in the context of the callbackCrit
    // critical section: iTSafeTktReqStatus, iTSafeTktReqResult, bTSafeOfflineEvent, iTSafeOfflineResult
    NetCritEnter(&pRef->callbackCrit);

    // save ticketing result
    if (iEvent == SCE_NP_MANAGER_EVENT_GOT_TICKET)
    {
#if DIRTYCODE_LOGGING
        if (pRef->iTSafeTktReqStatus == 1)
        {
            NetPrintf(("netconnps3: warning - got ticket event occured before previous one was processed in _NetConnUpdate()\n"));
        }
#endif

        pRef->iTSafeTktReqStatus = 1;         // get ticket completed
        pRef->iTSafeTktReqResult = iResult;   // save result
    }
    else if (iEvent == SCE_NP_MANAGER_STATUS_OFFLINE)
    {
#if DIRTYCODE_LOGGING
        if (pRef->bTSafeOfflineEvent == TRUE)
        {
            NetPrintf(("netconnps3: warning - offline status event occured before previous one was processed in _NetConnUpdate()\n"));
        }
#endif

        pRef->bTSafeOfflineEvent = TRUE;      // flag conn status event
        pRef->iTSafeOfflineResult = iResult;  // save result
    }

    NetCritLeave(&pRef->callbackCrit);
}

/*F********************************************************************************/
/*!
    \Function _NetConnNpManagerDelayedGotTicketEvent

    \Description
        Delayed processing of Network Platform events. Not thread-safe. To be invoked
        from idler.

    \Input *pRef    - module state
    \Input iResult  - result status code

    \Version 03/22/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnNpManagerDelayedGotTicketEvent(NetConnRefT *pRef, int32_t iResult)
{
    NetPrintf(("netconnps3: processing of delayed event SCE_NP_MANAGER_EVENT_GOT_TICKET, result %s\n", DirtyErrGetName(iResult)));

    // call user ticketing callback
    pRef->pTicketCallback(iResult, pRef->pTicketUserData);

    if (iResult > 0)
    {
        // set ticket state to successfully completed
        pRef->iNpTicketStatus = 1;
    }
    else
    {
        pRef->iNpTicketStatus = iResult;
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnNpManagerDelayedOfflineStatusEvent

    \Description
        Delayed processing of Network Platform events. Not thread-safe. To be invoked
        from idler.

    \Input *pRef    - module state
    \Input iResult  - result status code

    \Version 03/22/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnNpManagerDelayedOfflineStatusEvent(NetConnRefT *pRef, int32_t iResult)
{
    NetPrintf(("netconnps3: processing of delayed event SCE_NP_MANAGER_STATUS_OFFLINE, result %s\n", DirtyErrGetName(iResult)));

    // save event result
    pRef->uNpErrResult = (uint32_t)iResult;

    // update connection status
    if ((iResult = sceNpManagerGetStatus(&pRef->iNpStatus)) != 0)
    {
        NetPrintf(("netconnps3: sceNpManagerGetStatus() failed in _NetConnNpManagerDelayedOfflineStatusEvent(): err=%s\n", DirtyErrGetName(iResult)));
        pRef->uConnStatus = '-err';
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnSysUtilCallback

    \Description
        Callback used by Sony's sysutils, for on screen network configuration
        error messages.

    \Input uStatus      - indicates what event the callback is being fired for.
    \Input uParam       - unused
    \Input *pUserData   - unused

    \Notes
        This callback is now thread-safe. It uses a critical section to access
        variables that are shared with other functions of NetConnPs3. Thread-safety
        is required because we cannot guarantee that cellSysutilCheckCallback()
        is going to be called from the same thread as DirtySDK... and that function
        is the one that internally invokes the callbacks we register with the PS3
        system

    \Version 07/11/2008 (cvienneau)
*/
/********************************************************************************F*/
static void _NetConnSysUtilCallback(uint64_t uStatus, uint64_t uParam, void * pUserData)
{
    NetConnRefT *pRef = _NetConn_pRef;

    NetPrintf(("netconnps3: sysutil event, status %d\n", uStatus));

    // iTSafeStrtDlgStatus can only be accessed in the context
    // of the callbackCrit critical section
    NetCritEnter(&pRef->callbackCrit);

    switch (uStatus)
    {
        case CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED:
        {
            NetPrintf(("netconnps3: delaying processing of new sysutil status CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED\n"));

            pRef->iTSafeStrtDlgStatus = 1;   // start dialog completed
            break;
        }
        default:
            break;
    }

    NetCritLeave(&pRef->callbackCrit);
}

/*F********************************************************************************/
/*!
    \Function _NetConnSysutilDlgFinishDelayedEvent

    \Input *pRef    - module state

    \Description
        Process delayed sysutil CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED event.
        Not thread-safe - execute only during idle loop.

    \Version 03/22/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnSysutilDlgFinishDelayedEvent(NetConnRefT *pRef)
{
    int32_t iResult;
    struct CellNetCtlNetStartDialogResult NetStartResult;

    memset(&NetStartResult, 0, sizeof(NetStartResult));
    NetStartResult.size = sizeof(NetStartResult);

    NetPrintf(("netconnps3: processing of delayed event CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED\n"));

    if ((iResult = cellNetCtlNetStartDialogUnloadAsync(&NetStartResult)) != 0)
    {
        NetPrintf(("netconnps3: cellNetCtlNetStartDialogUnloadAsync() failed: err=%s\n", DirtyErrGetName(iResult)));
    }

    if (NetStartResult.result == 0)
    {
        if (pRef->iLastNpStatus == SCE_NP_MANAGER_STATUS_OFFLINE)
        {
            pRef->eState = ST_CONN;
        }
        else
        {
            pRef->eState = ST_IDLE;
        }
    }
    else if ((uint32_t)NetStartResult.result == CELL_NET_CTL_ERROR_NP_NO_ACCOUNT)
    {
        pRef->uConnStatus = '-act'; //the network won't be able to work on NP
        pRef->eState = ST_IDLE;
    }
    else
    {
        pRef->uConnStatus = '-err'; //the network won't be able to work on NP
        pRef->eState = ST_IDLE;
    }

    cellSysutilUnregisterCallback(pRef->uSysutilCbSlot);
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
    int32_t iResult;

    // allocate np pool memory
    if ((pRef->pNpPoolMem = DirtyMemAlloc(pRef->iNpPoolSize, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconnps3: could not allocate %d byte np pool\n", pRef->iNpPoolSize));
        return(-1);
    }
    // initialize network platform
    if ((iResult = sceNpInit(pRef->iNpPoolSize, pRef->pNpPoolMem)) != 0)
    {
        DirtyMemFree(pRef->pNpPoolMem, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        pRef->pNpPoolMem = NULL;

        if (iResult != (signed)SCE_NP_ERROR_ALREADY_INITIALIZED)
        {
            NetPrintf(("netconnps3: sceNpInit() failed: err=%s\n", DirtyErrGetName(iResult)));
            return(-2);
        }
        else
        {
            NetPrintf(("netconnps3: sceNpInit() returned SCE_NP_ERROR_ALREADY_INITIALIZED... proceeding to next initialization step.\n"));
        }
    }
    // add np manager callback
    if ((iResult = sceNpManagerRegisterCallback(_NetConnNpManagerCallback, _NetConn_pRef)) != 0)
    {
        NetPrintf(("netconnps3: sceNpManagerRegisterCallback() failed: err=%s\n", DirtyErrGetName(iResult)));

        if ((iResult = sceNpTerm()) != 0)
        {
            NetPrintf(("netconnps3: sceNpTerm() failed: err=%s\n", DirtyErrGetName(iResult)));
        }

        DirtyMemFree(pRef->pNpPoolMem, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        pRef->pNpPoolMem = NULL;

        return(-3);
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

    NetPrintf(("netconnps3: shutting down Network Platform\n"));

    // unregister npmanager callback
    if ((iResult = sceNpManagerUnregisterCallback()) != 0)
    {
        NetPrintf(("netconnps3: sceNpManagerUnregisterCallback() failed: err=%s\n", DirtyErrGetName(iResult)));
    }
    // shut down network platform if it was successfully started
    if (pRef->pNpPoolMem != NULL)
    {
        if ((iResult = sceNpTerm()) != 0)
        {
            NetPrintf(("netconnps3: sceNpTerm() failed: err=%s\n", DirtyErrGetName(iResult)));
        }
        // dispose of np pool memory
        DirtyMemFree(pRef->pNpPoolMem, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        pRef->pNpPoolMem = NULL;
    }

    // if there is a ticket request in progress, it is no longer valid because NP was just shut down
    if (!pRef->iNpTicketStatus)
    {
        NetPrintf(("netconnps3: NetConnDisconnect() fakes a ticket request failure because NP was just shut down\n"));

        // call user ticketing callback to signal that operation was aborted
        pRef->pTicketCallback(-1, pRef->pTicketUserData);
    }
    pRef->iNpTicketStatus = 1;
    pRef->iTSafeStrtDlgStatus = -1;
    pRef->iTSafeTktReqStatus = -1;
}

/*F********************************************************************************/
/*!
    \Function _NetConnIsTicketValid

    \Description
        Check whether the ticket we cache internally is valid.

    \Input *pRef        - netconn state
    \Input *pValid      - [OUT] TRUE if ticket exists in cache and is not yet expired; FALSE otherwise
    \Input uSigninSlot  - The signin slot, 0 for main, subuser id + 1 otherwise

    \Output
        int32_t         - 0=success, negative=failure

    \Notes
        Validity period of a NP ticket is known to be ~ 10 minutes.

        Sony communicated very clear guidelines about usage of NP tickets (https://ps3.scedev.net/forums/thread/121219/):
        game code shall limit ticket requests to minimize stress on NP authentication servers. Good practice
        consists in re-using a previously requested ticket until the PS3 ticketing API returns that the ticket
        is expired (SCE_NP_ERROR_EXPIRED_TICKET). Sony specifically suggests the following steps:
            1- check size of ticket data in system utility cache: sceNpManagerGetTicket(NULL, &ticketSize);
            2- if ticketSize>0, then a ticket is sitting in the system utility cache; proceed with checking if this
               ticket is expired or not with a call to sceNpManagerGetTicketParam(). Ticket is expired if
               return code is SCE_NP_ERROR_EXPIRED_TICKET
            3- if ticket is expired, then request a new one

        To comply with Sony's guidelines, NetConnPS3 was improved to no longer request a ticket
        each time NetConnConnect() is invoked. Indeed, the ticket obtained in the previous NetConnConnect()
        call is re-used if not yet expired. However, we use our own caching and expiration detection
        mechanism for that purpose. We cannot use the implementation suggested by Sony because
        the NetConnDisconnect() call that is performed between 2 calls to NetConnConnect() implies usage
        of sceNpTerm() which invalidates the contents of the sytem utility cache.

    \Version 09/06/2011 (mclouatre)
*/
/********************************************************************************F*/
static uint32_t _NetConnIsTicketValid(NetConnRefT *pRef, uint8_t *pValid, uint32_t uSigninSlot)
{
    int32_t iResult = 0; // default to success
    CellRtcTick currentTime;

    *pValid = FALSE; // default to not valid

    // make sure we have a ticket in cache
    if (pRef->pNpTicket[uSigninSlot])
    {
        /*
        Get current time
        From PS3 doc: The time obtained with this function is based on the time the user signed into
        PlayStation®Network. For this reason, the UTC time obtained cannot be intentionally modified
        by the user and will be fairly accurate.
        */
        if ((iResult = sceNpManagerGetNetworkTime(&currentTime)) == 0)  // current time (UTC) in Tick units (cumulative number of Ticks from 0001/01/01 00:00:00)
        {
            CellRtcDateTime rtcTime;

            // convert current time from CellRtcTick to CellRtcDateTime
            if ((iResult = cellRtcSetTick(&rtcTime, &currentTime)) == CELL_OK)
            {
                time_t currentTTime;

                // convert current time from CellRtcDateTime to time_t
                if ((iResult = cellRtcConvertDateTimeToTime_t(&rtcTime, &currentTTime)) == CELL_OK)
                {
                    SceNpTime currentNetworkTime = currentTTime * 1000; // in milliseconds

                    if (currentNetworkTime <= pRef->ticketExpiryTime[uSigninSlot])
                    {
                        #if DIRTYCODE_LOGGING
                        SceNpTime delta = pRef->ticketExpiryTime[uSigninSlot] - currentNetworkTime;

                        NetPrintf(("netconnps3: locally cached ticket is still valid for %lld ms (~ %lld min %lld sec)\n",
                            delta, ((delta/1000)/60), (delta/1000)%60));
                        #endif
                        *pValid = TRUE; // flag ticket as valid
                    }
                    else
                    {
                        NetPrintf(("netconnps3: locally cached ticket is expired\n"));
                    }
                }
                else
                {
                    NetPrintf(("netconnps3: cellRtcConvertDateTimeToTime_t() failed (err=%s)\n", DirtyErrGetName(iResult)));
                }
            }
            else
            {
                NetPrintf(("netconnps3: cellRtcSetTick() failed (err=%s)\n", DirtyErrGetName(iResult)));
            }
        }
        else
        {
            NetPrintf(("netconnps3: sceNpManagerGetNetworkTime() failed (err=%s)\n", DirtyErrGetName(iResult)));
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
    int32_t iResult;
    int32_t iRetVal;
    const char *pSelectedServiceId;

    // save completion callback
    pRef->pTicketCallback = pCallback;
    pRef->pTicketUserData = pUserData;

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

    // iTSafeTktReqStatus can only be accessed in the context
    // of the callbackCrit critical section
    NetCritEnter(&pRef->callbackCrit);

    // make sure ticket req is idle
    if (pRef->iTSafeTktReqStatus == -1)
    {
        SceNpId* pNpId = &pRef->NpId;

        if (pRef->uSubUserSigninInProgress)
        {
            NetPrintf(("netconnps3: Will use sub-user NpId for ticketing\n"));
            pNpId = &pRef->aSubUsers[pRef->uSubUserSigninSlot];
        }

        // now request a ticket
        NetPrintf(("netconnps3: requesting ticket with serviceId=%s, cookie=%s/%d, entitlement=%s, consumedcount=%d\n",
            pSelectedServiceId, pCookie, iCookieSize, pEntitlementId, uConsumedCount));

        iResult = sceNpManagerRequestTicket2(
            pNpId,                  // const SceNpId *npId,
            &TicketVersion,         // const SceNpTicketVersion *version,
            pSelectedServiceId,     // const char *serviceId,
            pCookie,                // const void *cookie,
            iCookieSize,            // const size_t cookieSize,
            pEntitlementId,         // const char *entitlementId,
            uConsumedCount);        // const unsigned int consumedCount
        if (iResult == 0)
        {
            // mark ticket req as pending
            pRef->iNpTicketStatus = 0;
            pRef->iTSafeTktReqStatus = 0;
            iRetVal = 1;
        }
        else
        {
            // error
            NetPrintf(("netconnps3: sceNpManagerRequestTicket2() failed (err=%s)\n", DirtyErrGetName(iResult)));
            iRetVal = iResult;
        }
    }
    else
    {
        NetPrintf(("netconnps3: critical error - ticket request already in progress\n"));
        iRetVal = -1;
    }

    NetCritLeave(&pRef->callbackCrit);

    return(iRetVal);
}

/*F********************************************************************************/
/*!
    \Function _NetConnTicketGet

    \Description
        Get ticket data from NP.

    \Input *pRef        - netconn state
    \Input *pTicketSize - [out] storage for output size of ticket
    \Input *pTime       - [out] ticket expiration

    \Output
        uint8_t *       - newly allocated ticket blob, or NULL if there was an error

    \Version 10/06/2009 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t *_NetConnTicketGet(NetConnRefT *pRef, int32_t *pTicketSize, SceNpTime *pTime)
{
    int32_t iResult;
    size_t iTicketSize;
    uint8_t *pTicketBuf;

    // find out what the size of the ticket is
    if ((iResult = sceNpManagerGetTicket(NULL, &iTicketSize)) != 0)
    {
        NetPrintf(("netconnps3: sceNpManagerGetTicket(NULL) failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(NULL);
    }
    // allocate memory for the ticket
    if ((pTicketBuf = DirtyMemAlloc(iTicketSize, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconnps3: failed to allocate %d bytes for NP ticket\n", iTicketSize));
        return(NULL);
    }

    // get ticket blob
    if ((iResult = sceNpManagerGetTicket(pTicketBuf, &iTicketSize)) == 0)
    {
        union SceNpTicketParam TicketParam;

        NetPrintf(("netconnps3: np ticket was queried successfully\n"));
        #if DIRTYCODE_LOGGING
        NetPrintMem(pTicketBuf, iTicketSize, "np-ticket");
        #endif
        *pTicketSize = iTicketSize;

        // find ticket expiry date
        if ((iResult = sceNpManagerGetTicketParam(SCE_NP_TICKET_PARAM_EXPIRE_DATE, &TicketParam)) == 0)
        {
            if (pTime)
            {
                *pTime = TicketParam.i64;
            }
        }
        else
        {
            NetPrintf(("netconnps3: sceNpManagerGetTicketParam(SCE_NP_TICKET_PARAM_EXPIRE_DATE) failed (err=%s)\n", DirtyErrGetName(iResult)));
            DirtyMemFree(pTicketBuf, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
            pTicketBuf = NULL;
        }
    }
    else
    {
        NetPrintf(("netconnps3: sceNpManagerGetTicket() failed (err=%s)\n", DirtyErrGetName(iResult)));
        DirtyMemFree(pTicketBuf, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        pTicketBuf = NULL;
    }

    return(pTicketBuf);
}

/*F********************************************************************************/
/*!
    \Function _NetConnTicketCallback

    \Description
        Get ticket data from NP.

    \Input iResult      - result code from ticket event
    \Input *pUserData   - netconn ref

    \Version 12/08/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnTicketCallback(int32_t iResult, void *pUserData)
{
    NetConnRefT *pRef = (NetConnRefT *)pUserData;

    if (iResult > 0)
    {
        int32_t iIndex = 0;
        int32_t iSlot;
        NetPrintf(("netconnps3: ticket is available\n"));

        // get platform environment from ticket
        pRef->uPlatEnv = _NetConnGetPlatformEnvironment(pRef);

        // get gamer's account id from ticket

        iSlot = pRef->uSubUserSigninInProgress ? pRef->uSubUserSigninSlot : 0;

        pRef->aNpAccountId[iSlot] = _NetConnGetAccountId(pRef);

        if (pRef->uSubUserSigninInProgress)
        {
            iIndex = pRef->uSubUserSigninSlot;
            pRef->uSubUserSigninSlot = 0;
            pRef->uSubUserSigninInProgress = FALSE;
        }

        // if we have a previous ticket buffer, dispose of it
        if (pRef->pNpTicket[iIndex] != NULL)
        {
            DirtyMemFree(pRef->pNpTicket[iIndex], NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
            pRef->iNpTicketSize[iIndex] = 0;
            pRef->ticketExpiryTime[iIndex] = 0;
        }

        // get the ticket
        pRef->pNpTicket[iIndex] = _NetConnTicketGet(pRef, &pRef->iNpTicketSize[iIndex], &pRef->ticketExpiryTime[iIndex]);

        NetPrintMem(&(pRef->aNpAccountId[iSlot]), sizeof(pRef->aNpAccountId[iSlot]), "uAccountId");
    }
    else if (iResult < 0)
    {
        NetPrintf(("netconnps3: error %s acquiring ticket\n", DirtyErrGetName(iResult)));
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnSubSigninCallback

    \Description
        Callback from the system after requesting a sub-user to be signed in

    \Input iResult  - SCE return code (error code)
    \Input *pNpId   - SCE NP Id of the signed in user.
    \Input *pData   - module state pointer, passed as void* userdata

    \Version 11/19/2010 (jrainy)
*/
/********************************************************************************F*/
static void _NetConnSubSigninCallback(int32_t iResult, SceNpId *pNpId, void *pData)
{
    NetConnRefT *pRef = (NetConnRefT *)pData;

    if (pRef->pSubUserSigninCallback != NULL)
    {
        pRef->pSubUserSigninCallback(pRef->uSubUserSigninSlot, iResult, pRef->pSubUserSigninData);
    }

    if (iResult == SCE_NP_MANAGER_SUB_SIGNIN_RESULT_OK)
    {
        uint8_t bValid;
        uint32_t uRet;
        uint32_t uValiditySlot;

        memcpy(&pRef->aSubUsers[pRef->uSubUserSigninSlot], pNpId, sizeof(SceNpId));
        NetPrintf(("netconnps3: SubSignin slot %u success\n", pRef->uSubUserSigninSlot));

        pRef->aSubUserSignedIn[pRef->uSubUserSigninSlot] = TRUE;

        // TODO: not sure we need the check below anymore. review.
        uValiditySlot = pRef->uSubUserSigninInProgress ? pRef->uSubUserSigninSlot : 0;

        if ((uRet = _NetConnIsTicketValid(pRef, &bValid, uValiditySlot)) == 0)
        {
            if (!bValid)
            {
                NetConnRequestTicket(3, 0, NULL, NULL, 0, NULL, 0, _NetConnTicketCallback, pRef);
            }
            else
            {
                pRef->uSubUserSigninSlot = 0;
                pRef->uSubUserSigninInProgress = FALSE;
            }
        }
        else
        {
            NetPrintf(("netconnps3: SubSignin ticket error\n"));

            pRef->uSubUserSigninSlot = 0;
            pRef->uSubUserSigninInProgress = FALSE;
        }
    }
    else if (iResult == SCE_NP_MANAGER_SUB_SIGNIN_RESULT_CANCEL)
    {
        NetPrintf(("netconnps3: SubSignin cancelled\n"));

        pRef->uSubUserSigninSlot = 0;
        pRef->uSubUserSigninInProgress = FALSE;
    }
    else
    {
        NetPrintf(("netconnps3: _NetConnSubSigninCallback with error 0x%08x\n", iResult));

        pRef->uSubUserSigninSlot = 0;
        pRef->uSubUserSigninInProgress = FALSE;
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
    int32_t iResult;

    // get the npid
    if ((iResult = sceNpManagerGetNpId(&pRef->NpId)) == 0)
    {
        NetPrintf(("netconnps3: NpId.handle.data=%s\n", pRef->NpId.handle.data));
        NetPrintf(("netconnps3: NpId.opt=0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
            pRef->NpId.opt[0], pRef->NpId.opt[1], pRef->NpId.opt[2], pRef->NpId.opt[3],
            pRef->NpId.opt[4], pRef->NpId.opt[5], pRef->NpId.opt[6], pRef->NpId.opt[7]));
        NetPrintf(("netconnps3: NpId.reserved=0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
            pRef->NpId.reserved[0], pRef->NpId.reserved[1], pRef->NpId.reserved[2], pRef->NpId.reserved[3],
            pRef->NpId.reserved[4], pRef->NpId.reserved[5], pRef->NpId.reserved[6], pRef->NpId.reserved[7]));
    }
    else
    {
        NetPrintf(("netconnps3: sceNpManagerGetNpId() failed: err=%s\n", DirtyErrGetName(iResult)));
    }

    // get signin id
    if ((iResult = sceNpManagerGetSigninId(&pRef->NpSigninId)) == 0)
    {
        NetPrintf(("netconnps3: NpSigninId=%s\n", pRef->NpSigninId.data));
    }
    else
    {
        NetPrintf(("netconnps3: sceNpManagerGetSigninId() failed: err=%s\n", DirtyErrGetName(iResult)));
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetConnSysUtil

    \Description
        Determines if the network is configured properly, and displays 3rd party
        sysutil error displays if its not; called from NetConnConnect.

    \Input *pOnline     - [out] filled with TRUE if network is ready, filled with FALSE if not.

    \Output
        int32_t         - zero=success, negative=failure

    \Version 07/11/2008 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _NetConnSysUtil(uint8_t *pOnline)
{
    int32_t iStateId;
    int32_t iResult;
    NetConnRefT *pRef = _NetConn_pRef;

    if ((iResult = sceNpManagerGetStatus(&iStateId)) == 0)
    {
        if (iStateId != SCE_NP_MANAGER_STATUS_ONLINE)
        {
            if ((iResult = cellSysutilRegisterCallback(pRef->uSysutilCbSlot, _NetConnSysUtilCallback, NULL)) == CELL_OK)
            {

                // iTSafeStrtDlgStatus can only be accessed in the context
                // of the callbackCrit critical section
                NetCritEnter(&pRef->callbackCrit);

                // make sure start dlg is idle
                if (pRef->iTSafeStrtDlgStatus == -1)
                {
                    //setup and display the network start dialog
                    struct CellNetCtlNetStartDialogParam NetStartParam;
                    memset(&NetStartParam, 0, sizeof(NetStartParam));
                    NetStartParam.size = sizeof(NetStartParam);
                    NetStartParam.type = CELL_NET_CTL_NETSTART_TYPE_NP;

                    if ((iResult = cellNetCtlNetStartDialogLoadAsync(&NetStartParam)) != 0)
                    {
                        NetPrintf(("netconnps3: cellNetCtlNetStartDialogLoadAsync() failed: err=%s\n", DirtyErrGetName(iResult)));
                        pRef->uConnStatus = '-dlg';
                    }
                    else
                    {
                        pRef->iTSafeStrtDlgStatus = 0; // mark start dialog as pending
                    }
                }
                else
                {
                    NetPrintf(("netconnps3: critical error - start dialog already in progress\n", DirtyErrGetName(iResult)));
                    pRef->uConnStatus = '-dlg';
                }

                NetCritLeave(&pRef->callbackCrit);
            }
            else
            {
                NetPrintf(("netconnps3: cellSysutilRegisterCallback() failed: err=%s\n", DirtyErrGetName(iResult)));
                pRef->uConnStatus = '-reg';
            }
        }
    }
    else
    {
        NetPrintf(("netconnps3: sceNpManagerGetStatus() failed in _NetConnSysUtil(): err=%s\n", DirtyErrGetName(iResult)));
        pRef->uConnStatus = '-err';
    }

    // fill user-provided variable with TRUE if online or with FALSE otherwise
    *pOnline = (iStateId == SCE_NP_MANAGER_STATUS_ONLINE);

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetConnProcessDelayedNpMgrEvents

    \Description
        Process delayed NP manager events

    \Input *pRef    - netconn state

    \Version 02/22/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnProcessDelayedNpMgrEvents(NetConnRefT *pRef)
{
    NetCritEnter(&pRef->callbackCrit);

    // is there a got ticket event to process
    if (pRef->iTSafeTktReqStatus == 1)
    {
        _NetConnNpManagerDelayedGotTicketEvent(pRef, pRef->iTSafeTktReqResult);
        pRef->iTSafeTktReqStatus = -1;   // back to idle
    }

    // is there a conn status event to process
    if (pRef->bTSafeOfflineEvent)
    {
        _NetConnNpManagerDelayedOfflineStatusEvent(pRef, pRef->iTSafeOfflineResult);
        pRef->bTSafeOfflineEvent = FALSE;
    }

    NetCritLeave(&pRef->callbackCrit);
}

/*F********************************************************************************/
/*!
    \Function _NetConnProcessDelayedSysutilEvents

    \Description
        Process delayed sysutil events

    \Input *pRef    - netconn state

    \Version 02/22/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnProcessDelayedSysutilEvents(NetConnRefT *pRef)
{
    NetCritEnter(&pRef->callbackCrit);

    if (pRef->iTSafeStrtDlgStatus == 1)
    {
        _NetConnSysutilDlgFinishDelayedEvent(pRef);
        pRef->iTSafeStrtDlgStatus = -1;   // back to idle
    }

    NetCritLeave(&pRef->callbackCrit);
}

/*F********************************************************************************/
/*!
    \Function _NetConnConnect

    \Description
        Start connection process

    \Input *pRef     - netconn module state

    \Output
        int32_t     - 0 for success, negative for failure

    \Version 10/01/2011 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _NetConnConnect(NetConnRefT *pRef)
{
    int32_t iResult;
    uint8_t bOnline = TRUE;

    // initialize cellNetCtl
    if ((iResult = SocketControl(NULL, 'conn', 0, NULL, NULL)) == 0)
    {
        // initialize Network Platform
        if ((iResult = _NetConnNpInit(pRef)) != 0)
        {
            NetPrintf(("netconnps3: failed to initalize network platform\n"));
            SocketControl(NULL, 'disc', 0, NULL, NULL);
            pRef->uConnStatus = '-sce';
            return(-1);
        }

        pRef->iLastNpStatus = SCE_NP_MANAGER_STATUS_OFFLINE;
        pRef->uNpErrResult = 0;
    }
    else
    {
        NetPrintf(("netconnps3: failed to initalize network stack\n"));
        pRef->uConnStatus = '-skt';
        return(-2);
    }

    if (pRef->bSilent == FALSE)
    {
        if ((iResult = _NetConnSysUtil(&bOnline)) != 0)
        {
            // assumption: _NetConnSysUtil() has updated pRef->uConnStatus with appropriate error state

            NetPrintf(("netconnps3: failed to initalize libsysutil\n"));
            _NetConnNpTerm(pRef);
            SocketControl(NULL, 'disc', 0, NULL, NULL);
            return(-3);
        }
    }

    // set up to request a new ticket
    pRef->bNpTicketRequested = FALSE;
	
    if (bOnline)
    {
        pRef->eState = ST_CONN;
    }
    else
    {
        // we'll need to wait for the network dialog to be ready
        // lets go to a state where nothing happens so our connection status won't be tampered with
        pRef->eState = ST_WAIT_SUTL;

        // clear connection status, we don't know if the network is fully active for NP yet
        pRef->uConnStatus = 0;
    }

    return(0);
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
    int32_t iResult;

    // update friends
    if (pRef->pFriendApiRef)
    {
        FriendApiUpdate(pRef->pFriendApiRef);
    }

    // process delayed NP manager events
    _NetConnProcessDelayedNpMgrEvents(pRef);

    // try to empty external cleanup list
    _NetConnProcessExternalCleanupList(pRef);

    // waiting for external cleanup list to be empty
    if (pRef->eState == ST_EXT_CLEANUP)
    {
        if (pRef->pExternalCleanupList[0].pCleanupCb == NULL)
        {
            pRef->eState = ST_INIT;
            if (pRef->bConnectDelayed)
            {
                pRef->bConnectDelayed = FALSE;

                // it is only when the external clean up list has been fully cleaned up that sceNpTerm() can be called
                _NetConnNpTerm(pRef);

                if (_NetConnConnect(pRef) < 0)
                {
                    // assumption: _NetConnConnect() has updated pRef->uConnStatus with appropriate error state

                    NetPrintf(("netconnps3: critical error - can't complete connection setup\n"));
                }
            }
        }
    }

    // wait for start dialog close
    if (pRef->eState == ST_WAIT_SUTL)
    {
        _NetConnProcessDelayedSysutilEvents(pRef);
    }

    // wait for network active status
    if (pRef->eState == ST_CONN)
    {
        pRef->uConnStatus = SocketInfo(NULL, 'conn', 0, NULL, 0);
        if (pRef->uConnStatus == '+onl')
        {
            pRef->eState = ST_NPCN;
            pRef->uConnStatus = '+con';
        }
    }

    // waiting for np manager status to go online
    if (pRef->eState == ST_NPCN)
    {
        if ((iResult = sceNpManagerGetStatus(&pRef->iNpStatus)) != 0)
        {
            NetPrintf(("netconnps3: sceNpManagerGetStatus() failed in _NetConnUpdate(): err=%s\n", DirtyErrGetName(iResult)));
            pRef->uConnStatus = '-err';
        }
        if ((pRef->iNpStatus == SCE_NP_MANAGER_STATUS_OFFLINE) && (pRef->iLastNpStatus != SCE_NP_MANAGER_STATUS_OFFLINE))
        {
            pRef->uConnStatus = '-off';
        }
        if (pRef->iLastNpStatus != pRef->iNpStatus)
        {
            NetPrintf(("netconnps3: %s->%s\n", _NpStatus[pRef->iLastNpStatus+1], _NpStatus[pRef->iNpStatus+1]));
            pRef->iLastNpStatus = pRef->iNpStatus;
        }
        if (pRef->iNpStatus == SCE_NP_MANAGER_STATUS_ONLINE)
        {
            if (_NetConnGetNpInfo(pRef) == 0)
            {
                pRef->uConnStatus = '+npo';

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

                // go to ticketing state
                pRef->eState = ST_TICK;
            }
            else
            {
                pRef->uConnStatus = '-npo';
            }
        }
    }

    // waiting to acquire ticket information
    if (pRef->eState == ST_TICK)
    {
        // request ticket if not already requested and not disabled
        if (!pRef->bNpTicketRequested && !pRef->bNoTicket)
        {
            uint8_t bValid = FALSE;

            // make sure a ticket request is really required
            if ((iResult = _NetConnIsTicketValid(pRef, &bValid, 0)) == 0)
            {
                if (bValid)
                {
                    NetPrintf(("netconnps3: skipping the ticket request because cached ticket is still valid\n"));

                    // set ticket state to successfully completed
                    pRef->iNpTicketStatus = 1;
                }
                else
                {
                    NetPrintf(("netconnps3: initiating ticket request because our cached ticket is empty or invalid\n"));

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
                        NetPrintf(("netconnps3: initial internal ticket request delayed by an already pending ticket request triggered externally\n"));
                    }
                }
            }
            else
            {
                pRef->uConnStatus = '-tkt';
                pRef->iNpTicketStatus = iResult;
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

    // update connection status while idle
    if (pRef->eState == ST_IDLE)
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

        // if we got an offline status update in np callback, mark us as disconnected
        if (pRef->iNpStatus == SCE_NP_MANAGER_STATUS_OFFLINE)
        {
            if (pRef->uNpErrResult == SCE_NP_CORE_SERVER_ERROR_CONFLICT)
            {
                pRef->uConnStatus = '-dup';
            }
            else
            {
                pRef->uConnStatus = '-dsc';
            }
        }
        else if ((pRef->uConnStatus >> 24) != '-')
        {
            pRef->uConnStatus = SocketInfo(NULL, 'conn', 0, NULL, 0);
        }
    }

    // if error status, go to idle state from any other state
    if ((pRef->eState != ST_IDLE) && (pRef->uConnStatus >> 24 == '-'))
    {
        pRef->eState = ST_IDLE;
    }

    if (pRef->bCellSysutilCheckCb)
    {
        // update callback status
        cellSysutilCheckCallback();
    }
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
            -nppool=poolsize    - specify np pool size in kbytes (default=128k)
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
        NetPrintf(("netconnps3: NetConnStartup() called while module is already active\n"));
        return(-1);
    }

    // alloc and init ref
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), NETCONN_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconnps3: unable to allocate module state\n"));
        return(-2);
    }
    memset(pRef, 0, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;
    pRef->eState = ST_INIT;
    pRef->iNpTicketStatus = 1;
    pRef->iTSafeStrtDlgStatus = -1;
    pRef->iTSafeTktReqStatus = -1;
    pRef->bCellSysutilCheckCb = TRUE;  // by default NetConnPS3 calls cellSysutilCheckCallback() regularly

    // default to PROD environment, will eventually be overwritten with value queried from NP ticket
    pRef->uPlatEnv = NETCONN_PLATENV_PROD;

    // parse and handle input parameters
    _NetConnParseParams(pRef, pParams);

    // create network instance
    SocketCreate(10, 0, 0);

    // init critical section used to ensure thread safety in handlers registered with sony apis
    NetCritInit(&pRef->callbackCrit, "netconnps3 callback crit");

    // create the upnp module
    if ((pRef->pProtoUpnp = ProtoUpnpCreate()) == NULL)
    {
        SocketDestroy(0);
        DirtyMemFree(pRef, NETCONN_MEMID, iMemGroup, pMemGroupUserData);
        NetPrintf(("netconnps3: unable to create protoupnp module\n"));
        return(-3);
    }

    // allocate external cleanup list
    pRef->iExternalCleanupListMax = NETCONN_EXTERNAL_CLEANUP_LIST_INITIAL_CAPACITY;
    if ((pRef->pExternalCleanupList = DirtyMemAlloc(pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT), NETCONN_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        ProtoUpnpDestroy(pRef->pProtoUpnp);
        SocketDestroy(0);
        DirtyMemFree(pRef, NETCONN_MEMID, iMemGroup, pMemGroupUserData);
        NetPrintf(("netconnps3: unable to allocate memory for initial external cleanup list\n"));
        return(-4);
    }
    memset(pRef->pExternalCleanupList, 0, pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT));

    // create friend api module
    if ((pRef->pFriendApiRef = FriendApiCreate()) == NULL)
    {
        ProtoUpnpDestroy(pRef->pProtoUpnp);
        SocketDestroy(0);
        DirtyMemFree(pRef->pExternalCleanupList, NETCONN_MEMID, iMemGroup, pMemGroupUserData);
        DirtyMemFree(pRef, NETCONN_MEMID, iMemGroup, pMemGroupUserData);
        NetPrintf(("netconnps3: unable to create friend api module\n"));
        return(-5);
    }

    // create and configure dirtycert
    if (NetConnDirtyCertCreate(pParams))
    {
        NetConnShutdown(0);
        return(-6);
    }

    // start up protossl
    if (ProtoSSLStartup() < 0)
    {
        NetConnShutdown(0);
        NetPrintf(("netconnps3: unable to startup protossl\n"));
        return(-7);
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
    \Input *pOption - optionally provide silent=true/false to prevent/enable sysutils from displaying.
    \Input iData    - platform-specific

    \Output
        int32_t     - negative=error, zero=success

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnConnect(const NetConfigRecT *pConfig, const char *pOption, int32_t iData)
{
    NetConnRefT *pRef = _NetConn_pRef;
    int32_t iResult = 0;

    // check connection options, if present
    if ((pRef->eState == ST_INIT) || (pRef->eState == ST_EXT_CLEANUP))
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
                NetPrintf(("netconnps3: peerport=%d\n", pRef->iPeerPort));
            }
            // check for no-ticket option
            if ((pOpt = strstr(pOption, "noticket")) != NULL)
            {
                pRef->bNoTicket = TRUE;
                NetPrintf(("netconnps3: noticket enabled\n"));
            }
        }

        // if needed, delay NetConnConnect until external cleanup phase completes
        if (pRef->eState == ST_EXT_CLEANUP)
        {
            pRef->bConnectDelayed = TRUE;
            NetPrintf(("netconnps3: delaying completion of NetConnConnect() until external cleanup phase is completed\n"));
        }
        else
        {
            iResult = _NetConnConnect(pRef);
        }
    }
    else
    {
        NetPrintf(("netconnps3: NetConnConnect() ignored because already connected!\n"));
    }

    return(iResult); // signal success
}

/*F********************************************************************************/
/*!
    \Function    NetConnConnectSub

    \Description
        Pop the sub-user signin dialog for Sub-User uUserId (from the system)

    \Input uUserId      - user id to sign-in
    \Input uSlot        - one of the sub-users logical slot [1,3]  (cannot be 0, which implicitly means primary user)
    \Input *pCallback   - callback to call with sub-sign-in result (optional, can be NULL)
    \Input *pUserData   - user data to pass to callback

    \Output
        int32_t     - negative=error, zero=success

    \Version 11/20/2010 (jrainy)
*/
/********************************************************************************F*/
int32_t NetConnConnectSub(uint32_t uUserId, uint32_t uSlot, NetConnSubSigninCallbackT *pCallback, void *pUserData)
{
    NetConnRefT *pRef = _NetConn_pRef;
    int32_t iRet;

// $todo, guard agains uSlot = 0

    if (pRef->eState == ST_INIT)
    {
        NetPrintf(("netconnps3: NetConnConnectSub() ignored because not connected!\n"));
        return(-1);
    }

    if (pRef->uSubUserSigninInProgress)
    {
        NetPrintf(("netconnps3: NetConnConnectSub() ignored because another sign-in already in progress!\n"));
        return(-2);
    }

    if (uSlot == 0)
    {
        NetPrintf(("netconnps3: NetConnConnectSub() ignored because sub-user slot (%u) is invalid\n", uSlot));
        return(-3);
    }

    pRef->uSubUserSigninSlot = uSlot;
    pRef->uSubUserSigninInProgress = TRUE;
    pRef->pSubUserSigninCallback = pCallback;
    pRef->pSubUserSigninData = pUserData;

    iRet = sceNpManagerSubSignin(uUserId, _NetConnSubSigninCallback, pRef, 0);
    NetPrintf(("netconnps3: sceNpManagerSubSignin slot %d user id %d returned 0x%08x\n", uSlot, uUserId, iRet));

    if (iRet != 0)
    {
        pRef->uSubUserSigninSlot = 0;
        pRef->uSubUserSigninInProgress = FALSE;
        pRef->pSubUserSigninCallback = NULL;
    }

    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function    NetConnDisconnectSub

    \Description
        Sign-out the sub-user with SCE NP Id pNpId  (from the system)

    \Input *pNpId   - NP Id to sign-out

    \Output
        int32_t     - negative=error, zero=success

    \Version 11/20/2010 (jrainy)
*/
/********************************************************************************F*/
int32_t NetConnDisconnectSub(void *pNpId)
{
    NetConnRefT *pRef = _NetConn_pRef;
    int32_t iRet;
    int32_t iIndex;

    if (pRef->eState == ST_INIT)
    {
        NetPrintf(("netconnps3: NetConnConnectSub() ignored because not connected!\n"));
        return(-1);
    }

    for(iIndex = 0; iIndex < NETCONN_MAX_SUBUSER; iIndex++)
    {
        if (!memcmp(&pRef->aSubUsers[iIndex], pNpId, sizeof(SceNpId)))
        {
            break;
        }
    }

    if (iIndex != NETCONN_MAX_SUBUSER)
    {
        memset(&pRef->aSubUsers[iIndex], 0, sizeof(SceNpId));
    }
    else
    {
        NetPrintf(("netconnps3: sub-user not known by netconn, bailing out\n"));
    }

    iRet = sceNpManagerSubSignout((SceNpId *)pNpId);
    NetPrintf(("netconnps3: sceNpManagerSubSignout returned 0x%08x\n", iRet));

    pRef->aSubUserSignedIn[iIndex] = FALSE;

    return(iRet);
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
    int32_t iIndex, iRet;
    NetConnRefT *pRef = _NetConn_pRef;

    NetPrintf(("netconnps3: disconnecting...\n"));

    for(iIndex = 0; iIndex < NETCONN_MAX_SUBUSER; iIndex++)
    {
        if (pRef->aSubUserSignedIn[iIndex])
        {
            iRet = sceNpManagerSubSignout((SceNpId *)&pRef->aSubUsers[iIndex]);
            NetPrintf(("netconnps3: sceNpManagerSubSignout returned 0x%08x\n", iRet));
            pRef->aSubUserSignedIn[iIndex] = FALSE;
        }
    }

    // early exit if NetConn is already in cleanup phase
    if (pRef->eState == ST_EXT_CLEANUP)
    {
        NetPrintf(("netconnps3: external cleanup already in progress\n"));
        return(0);
    }

    if (pRef->eState == ST_INIT)
    {
        NetPrintf(("netconnps3: already disconnected\n"));
        return(0);
    }

    // bring down network interface
    SocketControl(NULL, 'disc', 0, NULL, NULL);

    // reset status
    pRef->eState = ST_INIT;
    pRef->uConnStatus = 0;

    // clear cached NP information
    memset(&pRef->NpId, 0, sizeof(pRef->NpId));

    // delay call to _NetConnNpTerm() if need be
    if (_NetConn_pRef->iExternalCleanupListCnt != 0)
    {
        /*
        transit to ST_EXT_CLEANUP state
            Upon next call to NetConnConnect(), the NetConn module will be stuck in
            ST_EXT_CLEANUP state until all pending external module destructions
            are completed.
        */
        pRef->eState = ST_EXT_CLEANUP;
    }
    else
    {
        // shut down network platform
        _NetConnNpTerm(pRef);
    }

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
            cscc: turn on/off calls to cellSysutilCheckCallback()
            recu: stands for "register for external cleanup"; add a new entry to the external cleanup list
            scbs: set the sysutil callback slot via iData (value 0-3) - must be done before NetConnConnect
            lprx: load module
            uprx: unload module
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
        NetPrintf(("netconnps3: warning - calling NetConnControl() while module is not initialized\n"));
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
    // turn on/off calling of cellSysutilCallbackCheck
    if (iControl == 'cscc')
    {
        NetPrintf(("netconnps3: internal usage of cellSysutilCallbackCheck is %s\n", (iValue?"enabled":"disabled")));
        pRef->bCellSysutilCheckCb = iValue;
        return(0);
    }
    // register for external cleanup
    if (iControl == 'recu')
    {
        return(_NetConnAddToExternalCleanupList(pRef, (NetConnExternalCleanupCallbackT)pValue, pValue2));
    }
    // set sysutil callback slot
    if (iControl == 'scbs')
    {
        pRef->uSysutilCbSlot = iValue;
        return(0);
    }
    // set dirtycert service name
    if (iControl == 'snam')
    {
        return(DirtyCertControl('snam', 0, 0, pValue));
    }
    // load module
    if (iControl == 'lprx')
    {
        int32_t iResult = cellSysmoduleIsLoaded(iValue);

        if (iResult == CELL_SYSMODULE_LOADED)
        {
            NetPrintf(("netconnps3: Module already loaded\n"));
            return(0);
        }

        if (iResult != CELL_SYSMODULE_ERROR_UNLOADED)
        {
            NetPrintf(("netconnps3: Failed to verify if module is loaded (err=%s)\n", DirtyErrGetName(iResult)));
            return(-1);
        }

        iResult = cellSysmoduleLoadModule(iValue);

        if (iResult != CELL_OK)
        {
            NetPrintf(("netconnps3: Failed to load module (err=%s)\n", DirtyErrGetName(iResult)));
            return(-1);
        }

        return(0);
    }
    // unload module
    if (iControl == 'uprx')
    {
        int32_t iResult = cellSysmoduleIsLoaded(iValue);

        if(iResult == CELL_SYSMODULE_ERROR_UNLOADED)
        {
            NetPrintf(("netconnps3: Module not loaded\n"));
            return(0);
        }

        if(iResult != CELL_SYSMODULE_LOADED)
        {
            NetPrintf(("netconnps3: Failed to verify if module is loaded (err=%s)\n", DirtyErrGetName(iResult)));
            return(-1);
        }

        iResult = cellSysmoduleUnloadModule(iValue);

        if(iResult != CELL_OK)
        {
            NetPrintf(("netconnps3: Failed to load module (err=%s)\n", DirtyErrGetName(iResult)));
            return(-1);
        }

        return(0);
    }
    // request refresh of NP ticket
    if (iControl == 'tick')
    {
        NetPrintf(("netconnps3: NetConnControl 'tick' %d\n", iValue));
        #if DIRTYCODE_DEBUG
        uint8_t bValid;

        // make sure a ticket request is really required
        if (_NetConnIsTicketValid(pRef, &bValid, iValue) == 0)
        {
            if (bValid)
            {
                NetPrintf(("netconnps3: warning -- initiating a new ticket request while current ticket is not yet expired - not compliant with Sony's ticket usage guidelines\n"));
            }
        }
        #endif

        // mark old ticket as unavailable
        pRef->iNpTicketSize[iValue] = 0;
        pRef->ticketExpiryTime[iValue] = 0;

        if (iValue > 0)
        {
            pRef->uSubUserSigninSlot = iValue;
            pRef->uSubUserSigninInProgress = TRUE;
        }

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
            locn: returns location for logged-in user (the same to 'locl' but language part of the return value should be ignored by caller)
            macx: mac address of client (returned in pBuf)
            npcm: NP Communications ID (returned in pBuf)
            npid: NpId of logged in user (returned in pBuf)
            npti: NP Title ID (returned in pBuf)
            onln: true/false indication of whether network is operational
            open: true/false indication of whether network code running
            plug: SCE/Ethernet plug status (negative=error/unknown, 0=disconnected, 1=connected)
            spid: Service Provider ID
            subu: Return the n'th sub-user's SceNpId
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
        NetPrintf(("netconnps3: warning - calling NetConnStatus() while module is not initialized\n"));
        return(-1);
    }

    // return signed-in gamer's account id
    if (iKind == 'acct')
    {
        if ((iData < 0) || (iData >= NETCONN_MAX_SUBUSER))
        {
            NetPrintf(("netconnps3: 'acct' selector failure - invalid user index\n"));
            return(-1);
        }

        // make sure we have a valid account id
        if (pRef->aNpAccountId[iData] == 0)
        {
            NetPrintf(("netconnps3: 'acct' selector failure - NP Account ID is not yet valid\n"));
            return(-1);
        }

        // make sure user's provided buffer is big enough to store 64-bit account id
        if (sizeof(pRef->aNpAccountId[iData]) > (uint32_t)iBufSize)
        {
            NetPrintf(("netconnps3: 'acct' selector failure - user-provided buffer is too small(%d/%d)\n", iBufSize, sizeof(pRef->aNpAccountId[iData])));
            return(-1);
        }

        // copy account id in user-provided buffer
        memcpy(pBuf, &pRef->aNpAccountId[iData], sizeof(pRef->aNpAccountId[iData]));
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
        int32_t bIsRestricted = 1, iResult;
        if ((iResult = sceNpManagerGetChatRestrictionFlag(&bIsRestricted)) != 0)
        {
            NetPrintf(("netconnps3: sceNpManagerGetChatRestrictionFlag() failed (err=%s)\n", DirtyErrGetName(iResult)));
        }
        return(bIsRestricted);
    }
    // connection status
    if (iKind == 'conn')
    {
        return(pRef->uConnStatus);
    }
    // EA back-end environment type in use
    if (iKind == 'envi')
    {
        if (pRef->uConnStatus == '+onl')
        {
            return(pRef->uPlatEnv);
        }
        return(-1);
    }
    // return locality code
    if ((iKind == 'locl') || (iKind == 'locn'))
    {
        int32_t iResult;
        int32_t iSceLanguage;
        SceNpCountryCode sceCountryCode;
        uint32_t uConvertedCountryCode;
        uint16_t uLanguage, uCountry;
        int32_t iMap;

        iResult = sceNpManagerGetAccountRegion(&sceCountryCode, &iSceLanguage);
        if (iResult == 0)
        {
            // get the language code
            for (iMap = 0, uLanguage = LOBBYAPI_LANGUAGE_UNKNOWN; iMap < (signed)(sizeof(_NetConn_Ps3LanguageMap)/sizeof(_NetConn_Ps3LanguageMap[0])); iMap++)
            {
                if (_NetConn_Ps3LanguageMap[iMap][0] == iSceLanguage)
                {
                    uLanguage = _NetConn_Ps3LanguageMap[iMap][1];
                    break;
                }
            }

            // get the country code
            uConvertedCountryCode = _NetConnConvertCountryCode(sceCountryCode);
            for (iMap = 0, uCountry = LOBBYAPI_COUNTRY_UNKNOWN; iMap < (signed)(sizeof(_NetConn_Ps3CountryMap)/sizeof(_NetConn_Ps3CountryMap[0])); iMap++)
            {
                if (_NetConn_Ps3CountryMap[iMap][0] == uConvertedCountryCode)
                {
                    uCountry = _NetConn_Ps3CountryMap[iMap][1];
                    break;
                }
            }

            return(LOBBYAPI_LocalizerTokenCreate(uLanguage, uCountry));
        }
        else
        {
            NetPrintf(("netconnps3: sceNpManagerGetAccountRegion(() failed with error = %s", DirtyErrGetName(iResult)));
            return(LOBBYAPI_LocalizerTokenCreate(LOBBYAPI_LANGUAGE_UNKNOWN, LOBBYAPI_COUNTRY_UNKNOWN));
        }
    }
    // friend api ref
    if (iKind == 'fref')
    {
        if ((pBuf == NULL) || (iBufSize < (signed)sizeof(pRef->pFriendApiRef)))
        {
            NetPrintf(("netconnps3: NetConnStatus('fref') failed!\n"));
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
    // np titleid
    if ((iKind == 'npti') && (pBuf != NULL) && (iBufSize == sizeof(SceNpTitleId)))
    {
        SceNpTitleId *pNpTitleId = (SceNpTitleId *)pBuf;
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
    if ((iKind == 'subu') && (iBufSize >= (int32_t)sizeof(SceNpId)) && (iData < NETCONN_MAX_SUBUSER))
    {
        memcpy(pBuf, &pRef->aSubUsers[iData], sizeof(SceNpId));
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
        NetPrintf(("netconnps3: NetConnStatus 'tick' %d\n", iData));

        if ((iData != 0) && pRef->uSubUserSigninInProgress && (pRef->uSubUserSigninSlot == (uint32_t)iData))
        {
            return(0);
        }

        if ((pRef->iNpTicketSize[iData] == 0) || (pBuf == NULL))
        {
            return(pRef->iNpTicketSize[iData]);
        }

        if (pRef->iNpTicketSize[iData] > iBufSize)
        {
            NetPrintf(("netconnps3: user ticket buffer is too small (%d/%d)\n", iBufSize, pRef->iNpTicketSize[iData]));
            return(-1);
        }
        memcpy(pBuf, pRef->pNpTicket[iData], pRef->iNpTicketSize[iData]);
        return(pRef->iNpTicketSize[iData]);
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
        int32_t             - 0 if success, -1 try again, other negative values: error

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnShutdown(uint32_t uShutdownFlags)
{
    int32_t iIndex;

    // make sure we've been started
    if (_NetConn_pRef == NULL)
    {
        NetPrintf(("netconnps3: NetConnShutdown() called while module is not active\n"));
        return(NETCONN_ERROR_NOTACTIVE);
    }

    /* If NET_SHUTDOWN_THREADSTARVE is passed in, instead of shutting down we simply
       pass the shutdown notification on to the socket module and exit */
    if (uShutdownFlags & NET_SHUTDOWN_THREADSTARVE)
    {
        NetPrintf(("netconnps3: executing threadstarve shutdown\n"));
        SocketDestroy(uShutdownFlags);
        return(0);
    }

    // try to empty external cleanup list
    _NetConnProcessExternalCleanupList(_NetConn_pRef);

    // make sure external cleanup list is empty before proceeding
    if (_NetConn_pRef->iExternalCleanupListCnt != 0)
    {
        // signal "try again"
        NetPrintf(("netconnps3: deferring shutdown while external cleanup list is not empty (%d items)\n", _NetConn_pRef->iExternalCleanupListCnt));
        return(NETCONN_ERROR_ISACTIVE);
    }

    NetPrintf(("netconnps3: shutting down...\n"));

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

    // check if a call to NetConnDisconnect() triggered the external cleanup mechanism before we reached this point
    if (_NetConn_pRef->eState == ST_EXT_CLEANUP)
    {
        // note: it is only when the external clean up list has been fully cleaned up that sceNpTerm() can be called
        _NetConnNpTerm(_NetConn_pRef);
    }
    else
    {
        // disconnect the interfaces
        NetConnDisconnect();
    }

    // destroy critical section used to ensure thread safety in handlers registered with sony apis
    NetCritKill(&_NetConn_pRef->callbackCrit);

    // shutdown the network code
    SocketDestroy(uShutdownFlags);

    for(iIndex = 0; iIndex < NETCONN_MAX_SUBUSER; iIndex++)
    {
        // disposed of any cached ticket we might have
        if (_NetConn_pRef->pNpTicket[iIndex] != NULL)
        {
            DirtyMemFree(_NetConn_pRef->pNpTicket[iIndex], NETCONN_MEMID, _NetConn_pRef->iMemGroup, _NetConn_pRef->pMemGroupUserData);
            _NetConn_pRef->pNpTicket[iIndex] = NULL;
            _NetConn_pRef->iNpTicketSize[iIndex] = 0;
        }
    }

    // free external cleanup list
    if (_NetConn_pRef->pExternalCleanupList != NULL)
    {
        DirtyMemFree(_NetConn_pRef->pExternalCleanupList, NETCONN_MEMID, _NetConn_pRef->iMemGroup, _NetConn_pRef->pMemGroupUserData);
        _NetConn_pRef->pExternalCleanupList = NULL;
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
    sys_timer_usleep(iMilliSecs*1000);
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
    int32_t iResult;
    NetConnRefT *pRef = _NetConn_pRef;

    // guarantee atomic access to iNpTicketStatus in case NetConnRequestTicket is called concurrently from different threads
    NetCritEnter(NULL);

    // make sure we're in a state where we can request a ticket
    if (pRef->iNpTicketStatus == 0)
    {
        if (pRef->uSubUserSigninInProgress)
        {
            pRef->uSubUserSigninSlot = 0;
            pRef->uSubUserSigninInProgress = FALSE;
        }

        NetPrintf(("netconnps3: NetConnRequestTicket() cannot request ticket!\n"));
        NetCritLeave(NULL);
        return(pRef->iNpTicketStatus);
    }

    // request the ticket
    iResult = _NetConnTicketRequest(pRef, uTicketVersionMajor, uTicketVersionMinor, pServiceId, pCookie, iCookieSize, pEntitlementId, uConsumedCount, pCallback, pUserData);

    if (iResult < 0)
    {
        if (pRef->uSubUserSigninInProgress)
        {
            pRef->uSubUserSigninSlot = 0;
            pRef->uSubUserSigninInProgress = FALSE;
        }
    }

    NetCritLeave(NULL);

    return(iResult);

}

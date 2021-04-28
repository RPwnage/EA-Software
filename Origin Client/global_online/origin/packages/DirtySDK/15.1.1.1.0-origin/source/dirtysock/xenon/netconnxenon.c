/*H********************************************************************************/
/*!
    \File netconnxenon.c

    \Description
        Provides network setup and teardown support. Does not actually create any
        kind of network connections.

    \Copyright
        Copyright (c) Electronic Arts 2001-2004. ALL RIGHTS RESERVED.

    \Version 03/01/2005 (jbrookes)  Initial Xenon version.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <xtl.h>
#include <xonline.h>
#include <stdio.h>
#ifdef DIRTYSDK_XHTTP_ENABLED
#include <xhttp.h>
#endif

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtylang.h" // for locality macros and definitions
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtyaddr.h"
#include "DirtySDK/dirtysock/dirtycert.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynames.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/dirtysock/xenon/dirtyauthxenon.h"
#include "DirtySDK/dirtysock/xenon/dirtylspxenon.h"
#include "DirtySDK/friend/friendapi.h"

#if DIRTYCODE_LOGGING
#include "DirtySDK/xml/xmlparse.h"     // for XmlPrintFmt()
#endif


/*** Defines **********************************************************************/

//! maximum number of Microsoft services
#define NETCONN_MAXSERVICES                 (4)

//! used internally to initialize variables tracking a controller index.
#define NETCONN_INVALID_CONTROLLER_INDEX    (0xFF)

//! maximum time to wait for ISP to come up
#define NETCONN_MAX_WAIT_TIME_FOR_ISP       (2000)

//! time interval to update the XLSP connection so that it doesn't time out
#define NETCONN_XLSP_UPDATE_TIME            (60000)

//! title server service id
#define NETCONN_TITLESERVER_SERVICEID       (0x45410805)

//! initial size of external cleanup list (in number of entries)
#define NETCONN_EXTERNAL_CLEANUP_LIST_INITIAL_CAPACITY (12)

// token validity period
#define NETCONNXENON_TOKEN_TIMEOUT        (60 * 1000)   // 1 minute

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef enum NetConnTokenStateE
{
    ST_TOKEN_INVALID = 0,    // token has not yet been requested
    ST_TOKEN_FAILED,         // token acquisition failed
    ST_TOKEN_INPROG,         // token acquisition in progress
    ST_TOKEN_VALID           // token valid
} NetConnTokenStateE;

typedef enum NetConnUserInfoStateE
{
    ST_USERINFO_FAIL = -1,
    ST_USERINFO_IDLE = 0,
    ST_USERINFO_STRT,
    ST_USERINFO_PROC,
    ST_USERINFO_DONE
} NetConnUserInfoStateE;

typedef struct NetConnTokenT
{
    NetConnTokenStateE  eTokenStatus;       //!< token acquisition status
    int32_t             iTokenAcquTick;     //!< token acquisition time
    uint8_t             bFailureReported;   //!< whether token acquisition failure was at least reported once to the user
} NetConnTokenT;

typedef struct NetConnUserT
{
    char                    strGamertag[16];
    XUID                    Xuid;
    NetConnUserInfoStateE   eInfoState;
    GET_USER_INFO_RESPONSE  *pUserInfo;
    NetConnTokenT           token;
} NetConnUserT;

typedef int32_t (*NetConnExternalCleanupCallbackT)(void *pNetConnExternalCleanupData);

typedef struct NetConnExternalCleanupEntryT
{
    void *pCleanupData;                         //!< pointer to data to be passed to the external cleanup callback
    NetConnExternalCleanupCallbackT  pCleanupCb;//!< external cleanup callback
} NetConnExternalCleanupEntryT;

//! must be the same as xenonapi_cwrapper::XShowSigninUI_Ptr
typedef DWORD   (*NetConnXShowSigninUIT)(DWORD cPanes, DWORD dwFlags);

//! private module state
typedef struct NetConnRefT
{
    // module memory group
    int32_t                 iMemGroup;          //!< module mem group id
    void                    *pMemGroupUserData; //!< user data associated with mem group

    NetConnExternalCleanupEntryT
                            *pExternalCleanupList; //!< pointer to an array of entries pending external cleanup completion

    int32_t                 iExternalCleanupListMax; //!< maximum number of entries in the array (in number of entries)
    int32_t                 iExternalCleanupListCnt; //!< number of valid entries in the array

    int32_t                 iThreadStackSize;   //!< stack size used at thread creation (in bytes)

    NetConnXShowSigninUIT   pShowSigninUI;      //!< the function pointer to show the signin UI

    int32_t                 iNetActiv;
    int32_t                 iNetEstab;

    int32_t                 iXbMemUnitState;    //$$ deprecate - these are holdovers from Xbox 1 and no longer used
    int32_t                 iXbMemUnitUpdate;

    uint32_t                uNumServices;

    NetConnUserT            aUsers[NETCONN_MAXLOCALUSERS];
    uint32_t                uNumUsers;
    XOVERLAPPED             XOverlapped;
    XOVERLAPPED             XLookUpOverlapped;
    FIND_USER_INFO          FindUserInfo;
    FIND_USERS_RESPONSE     *pFindUserResponse;
    uint32_t                uFindUserResponseSize;

    NetConnUserDataT        UserLookedUp;
    uint8_t                 uLookUpInProgress;
    UserInfoCallbackT       *UserLookUpCallback;
    void                    *pUserLookedUpData;

    HANDLE                  hListener;

    XNQOS                   *pXnQos;
    XNADDR                  XnAddr;

    XNKID                   XnKid;
    XNKEY                   XnKey;

    enum
    {
        ST_IDLE,                    //!< idle
        ST_EXT_CLEANUP,             //!< cleaning up external instances from previous NetConn connection, can't proceed before all cleaned up
        ST_STAR,                    //!< starting up
        ST_START_LOG,               //!< start login of xbl gamer tag
        ST_END_LOG,                 //!< finish login of xbl gamer tag
        ST_CONN,                    //!< connecting
        ST_INFO,                    //!< getting user info
        ST_XLSP_GATE,               //!< resolving XLSP gateway
    } eState;                       //!< current state

    int32_t                 iNetStatus;
    uint32_t                uConnStatus;
    uint32_t                uIspTimer;

    uint32_t                uHostAddress;      //!< secure gateway IP
    int32_t                 iUpBitsPerSec;     //!< QoS-measured bps, up
    int32_t                 iDnBitsPerSec;     //!< QoS-measured bps, down
    int32_t                 iMedRttTime;       //!< QoS-measured median round-trip time

    SocketAddrMapT          SocketMap[2];

    uint8_t                 bSyslink;         //!< whether we are in syslink mode or not
    uint8_t                 bSyskey;          //!< whether or not we've registered our system link key
    uint8_t                 bUseXLSP;         //!< whether to connect to an XLSP on connect
    uint8_t                 bUseXhttp;        //!< whether to use xhttp or not
    uint8_t                 bXHttpStarted;    //!< has xhttp been started?
    uint8_t                 bXhttpNoSecure;   //!< whether to use xhttp
    uint8_t                 bRedirect;        //!< whether we use xmap and dns, disabled with -noredirect
    uint8_t                 bIspConnected;    //!< TRUE/FALSE indicator of ISP connection status
    uint8_t                 bXblConnected;    //!< TRUE/FALSE indicator of XBL connection status
    uint8_t                 bSGConnected;     //!< TRUE/FALSE indicator of Security Gateway connection status
    uint8_t                 bGotMsgErr;       //!< got the optional message error while signing in
    uint8_t                 bInitiateQos;     //!< if TRUE, initiate a QoS probe
    uint8_t                 bUserInfoQuery;   //!< if TRUE, a user info query is in progress
    uint8_t                 bSilent;          //!< if TRUE, 1st party login huds will be suppressed.
    uint8_t                 bDoConnect;       //!< if TRUE, we should transition out of the ST_STAR state

    enum
    {
        GO_DEFAULT,         //!< default, no guest option related flag will be used while the show signin ui function is called
        GO_ENABLE,          //!< causes the guest option to always appear on the list ("-guest=true", XSSUI_FLAGS_ENABLE_GUEST)
        GO_DISALLOW,        //!< prevents the guest option from appearing on the list ("-guest=false", XSSUI_FLAGS_DISALLOW_GUEST)
    } eGuestOption;

    uint32_t                uSecureSetting;     //!< 0 for "-unsecure", 1 for "-mixedsecure", 2 for "" (secure)

    FriendApiRefT           *pFriendApiRef;     //!< friend api module state
    uint8_t                 bUseFriendModule;              

    uint32_t                uThreadId;
    ULONG                   ulInvitePendingCtrl;//!< used to store controller index for which an invite accept notification is pending
    DirtyAuthRefT           *pDirtyAuth;        //!< dirtyauth ref
    DirtyLSPRefT            *pDirtyLSP;         //!< ref to the DirtyLSP module used for XLSP resolution
    DirtyLSPQueryRecT       *pXlspRec;          //!< pointer to the DirtyLSP query used for XLSP resolution
    uint32_t                uXlspAddress;       //!< secure address resolved for the XLSP gateway
    uint32_t                uLastUpdateTime;    //!< last update time for the XLSP connection
    char                    strXlspSitename[128];   //!< sitename to use for XLSP resolution
    char                    strXAuthUrn[128];   //!< URN used with DirtyAuth
    uint32_t                uTitleServerServiceId;  //!< Service Id use for XLSP resolution

    uint8_t                 bProcessingCleanupList;
} NetConnRefT;

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

//! mapping table to map Xenon language defines to DirtySock encodings
static uint16_t _NetConn_XnLanguageMap[18][2] =
{
    { XC_LANGUAGE_ENGLISH,          LOBBYAPI_LANGUAGE_ENGLISH },
    { XC_LANGUAGE_JAPANESE,         LOBBYAPI_LANGUAGE_JAPANESE },
    { XC_LANGUAGE_GERMAN,           LOBBYAPI_LANGUAGE_GERMAN },
    { XC_LANGUAGE_FRENCH,           LOBBYAPI_LANGUAGE_FRENCH },
    { XC_LANGUAGE_SPANISH,          LOBBYAPI_LANGUAGE_SPANISH },
    { XC_LANGUAGE_ITALIAN,          LOBBYAPI_LANGUAGE_ITALIAN },
    { XC_LANGUAGE_KOREAN,           LOBBYAPI_LANGUAGE_KOREAN },
    { XC_LANGUAGE_TCHINESE,         LOBBYAPI_LANGUAGE_CHINESE },
    { XC_LANGUAGE_PORTUGUESE,       LOBBYAPI_LANGUAGE_PORTUGUESE },
    { XC_LANGUAGE_SCHINESE_DEPRECATED, LOBBYAPI_LANGUAGE_UNKNOWN },
    { XC_LANGUAGE_POLISH,           LOBBYAPI_LANGUAGE_POLISH },
    { XC_LANGUAGE_RUSSIAN,          LOBBYAPI_LANGUAGE_RUSSIAN },
    { XC_LANGUAGE_SWEDISH,          LOBBYAPI_LANGUAGE_SWEDISH },
    { XC_LANGUAGE_TURKISH,          LOBBYAPI_LANGUAGE_TURKISH },
    { XC_LANGUAGE_BNORWEGIAN,       LOBBYAPI_LANGUAGE_NORWEGIAN_BOKMAL },
    { XC_LANGUAGE_DUTCH,            LOBBYAPI_LANGUAGE_DUTCH },
    { XC_LANGUAGE_SCHINESE,         LOBBYAPI_LANGUAGE_CHINESE },
    { 0xffff,                       0xffff  } // terminator
};

//! mapping table to map Xenon locale defines to DirtySock encodings
static uint16_t _NetConn_XnLocaleMap[44][2] =
{
    { XC_LOCALE_AUSTRALIA,          LOBBYAPI_COUNTRY_AUSTRALIA },
    { XC_LOCALE_AUSTRIA,            LOBBYAPI_COUNTRY_AUSTRIA },
    { XC_LOCALE_BELGIUM,            LOBBYAPI_COUNTRY_BELGIUM },
    { XC_LOCALE_BRAZIL,             LOBBYAPI_COUNTRY_BRAZIL },
    { XC_LOCALE_CANADA,             LOBBYAPI_COUNTRY_CANADA },
    { XC_LOCALE_CHILE,              LOBBYAPI_COUNTRY_CHILE },
    { XC_LOCALE_CHINA,              LOBBYAPI_COUNTRY_CHINA },
    { XC_LOCALE_COLOMBIA,           LOBBYAPI_COUNTRY_COLOMBIA },
    { XC_LOCALE_CZECH_REPUBLIC,     LOBBYAPI_COUNTRY_CZECH_REPUBLIC },
    { XC_LOCALE_DENMARK,            LOBBYAPI_COUNTRY_DENMARK },
    { XC_LOCALE_FINLAND,            LOBBYAPI_COUNTRY_FINLAND },
    { XC_LOCALE_FRANCE,             LOBBYAPI_COUNTRY_FRANCE },
    { XC_LOCALE_GERMANY,            LOBBYAPI_COUNTRY_GERMANY },
    { XC_LOCALE_GREECE,             LOBBYAPI_COUNTRY_GREECE },
    { XC_LOCALE_HONG_KONG,          LOBBYAPI_COUNTRY_HONG_KONG },
    { XC_LOCALE_HUNGARY,            LOBBYAPI_COUNTRY_HUNGARY },
    { XC_LOCALE_INDIA,              LOBBYAPI_COUNTRY_INDIA },
    { XC_LOCALE_IRELAND,            LOBBYAPI_COUNTRY_IRELAND },
    { XC_LOCALE_ITALY,              LOBBYAPI_COUNTRY_ITALY },
    { XC_LOCALE_JAPAN,              LOBBYAPI_COUNTRY_JAPAN },
    { XC_LOCALE_KOREA,              LOBBYAPI_COUNTRY_KOREA_REPUBLIC_OF },
    { XC_LOCALE_MEXICO,             LOBBYAPI_COUNTRY_MEXICO },
    { XC_LOCALE_NETHERLANDS,        LOBBYAPI_COUNTRY_NETHERLANDS },
    { XC_LOCALE_NEW_ZEALAND,        LOBBYAPI_COUNTRY_NEW_ZEALAND },
    { XC_LOCALE_NORWAY,             LOBBYAPI_COUNTRY_NORWAY },
    { XC_LOCALE_POLAND,             LOBBYAPI_COUNTRY_POLAND },
    { XC_LOCALE_PORTUGAL,           LOBBYAPI_COUNTRY_PORTUGAL },
    { XC_LOCALE_SINGAPORE,          LOBBYAPI_COUNTRY_SINGAPORE },
    { XC_LOCALE_SLOVAK_REPUBLIC,    LOBBYAPI_COUNTRY_SLOVAKIA_ },
    { XC_LOCALE_SOUTH_AFRICA,       LOBBYAPI_COUNTRY_SOUTH_AFRICA },
    { XC_LOCALE_SPAIN,              LOBBYAPI_COUNTRY_SPAIN },
    { XC_LOCALE_SWEDEN,             LOBBYAPI_COUNTRY_SWEDEN },
    { XC_LOCALE_SWITZERLAND,        LOBBYAPI_COUNTRY_SWITZERLAND },
    { XC_LOCALE_TAIWAN,             LOBBYAPI_COUNTRY_TAIWAN },
    { XC_LOCALE_GREAT_BRITAIN,      LOBBYAPI_COUNTRY_UNITED_KINGDOM },
    { XC_LOCALE_UNITED_STATES,      LOBBYAPI_COUNTRY_UNITED_STATES },
    { XC_LOCALE_RUSSIAN_FEDERATION, LOBBYAPI_COUNTRY_RUSSIAN_FEDERATION },
    { XC_LOCALE_WORLD_WIDE,         LOBBYAPI_COUNTRY_UNKNOWN },
    { XC_LOCALE_TURKEY,             LOBBYAPI_COUNTRY_TURKEY },
    { XC_LOCALE_ARGENTINA,          LOBBYAPI_COUNTRY_ARGENTINA },
    { XC_LOCALE_SAUDI_ARABIA,       LOBBYAPI_COUNTRY_SAUDI_ARABIA },
    { XC_LOCALE_ISRAEL,             LOBBYAPI_COUNTRY_ISRAEL },
    { XC_LOCALE_UNITED_ARAB_EMIRATES, LOBBYAPI_COUNTRY_UNITED_ARAB_EMIRATES },
    { 0xffff,                       0xffff  } // terminator
};

//! mapping table to map Xenon country definitions to DirtySock encodings
static uint16_t _NetConn_XnCountryMap[111][2] =
{
    { XONLINE_COUNTRY_UNITED_ARAB_EMIRATES, LOBBYAPI_COUNTRY_UNITED_ARAB_EMIRATES },
    { XONLINE_COUNTRY_ALBANIA,      LOBBYAPI_COUNTRY_ALBANIA },
    { XONLINE_COUNTRY_ARMENIA,      LOBBYAPI_COUNTRY_ARMENIA },
    { XONLINE_COUNTRY_ARGENTINA,    LOBBYAPI_COUNTRY_ARGENTINA },
    { XONLINE_COUNTRY_AUSTRIA,      LOBBYAPI_COUNTRY_AUSTRIA },
    { XONLINE_COUNTRY_AUSTRALIA,    LOBBYAPI_COUNTRY_AUSTRALIA },
    { XONLINE_COUNTRY_AZERBAIJAN,   LOBBYAPI_COUNTRY_AZERBAIJAN },
    { XONLINE_COUNTRY_BELGIUM,      LOBBYAPI_COUNTRY_BELGIUM },
    { XONLINE_COUNTRY_BULGARIA,     LOBBYAPI_COUNTRY_BULGARIA },
    { XONLINE_COUNTRY_BAHRAIN,      LOBBYAPI_COUNTRY_BAHRAIN },
    { XONLINE_COUNTRY_BRUNEI_DARUSSALAM, LOBBYAPI_COUNTRY_BRUNEI_DARUSSALAM },
    { XONLINE_COUNTRY_BOLIVIA,      LOBBYAPI_COUNTRY_BOLIVIA },
    { XONLINE_COUNTRY_BRAZIL,       LOBBYAPI_COUNTRY_BRAZIL },
    { XONLINE_COUNTRY_BELARUS,      LOBBYAPI_COUNTRY_BELARUS },
    { XONLINE_COUNTRY_BELIZE,       LOBBYAPI_COUNTRY_BELIZE },
    { XONLINE_COUNTRY_CANADA,       LOBBYAPI_COUNTRY_CANADA },
    { XONLINE_COUNTRY_SWITZERLAND,  LOBBYAPI_COUNTRY_SWITZERLAND },
    { XONLINE_COUNTRY_CHILE,        LOBBYAPI_COUNTRY_CHILE },
    { XONLINE_COUNTRY_CHINA,        LOBBYAPI_COUNTRY_CHINA },
    { XONLINE_COUNTRY_COLOMBIA,     LOBBYAPI_COUNTRY_COLOMBIA },
    { XONLINE_COUNTRY_COSTA_RICA,   LOBBYAPI_COUNTRY_COSTA_RICA },
    { XONLINE_COUNTRY_CZECH_REPUBLIC, LOBBYAPI_COUNTRY_CZECH_REPUBLIC },
    { XONLINE_COUNTRY_GERMANY,      LOBBYAPI_COUNTRY_GERMANY },
    { XONLINE_COUNTRY_DENMARK,      LOBBYAPI_COUNTRY_DENMARK },
    { XONLINE_COUNTRY_DOMINICAN_REPUBLIC, LOBBYAPI_COUNTRY_DOMINICAN_REPUBLIC },
    { XONLINE_COUNTRY_ALGERIA,      LOBBYAPI_COUNTRY_ALGERIA },
    { XONLINE_COUNTRY_ECUADOR,      LOBBYAPI_COUNTRY_ECUADOR },
    { XONLINE_COUNTRY_ESTONIA,      LOBBYAPI_COUNTRY_ESTONIA },
    { XONLINE_COUNTRY_EGYPT,        LOBBYAPI_COUNTRY_EGYPT },
    { XONLINE_COUNTRY_SPAIN,        LOBBYAPI_COUNTRY_SPAIN },
    { XONLINE_COUNTRY_FINLAND,      LOBBYAPI_COUNTRY_FINLAND },
    { XONLINE_COUNTRY_FAROE_ISLANDS, LOBBYAPI_COUNTRY_FAEROE_ISLANDS },
    { XONLINE_COUNTRY_FRANCE,       LOBBYAPI_COUNTRY_FRANCE },
    { XONLINE_COUNTRY_GREAT_BRITAIN, LOBBYAPI_COUNTRY_UNITED_KINGDOM },
    { XONLINE_COUNTRY_GEORGIA,      LOBBYAPI_COUNTRY_GEORGIA },
    { XONLINE_COUNTRY_GREECE,       LOBBYAPI_COUNTRY_GREECE },
    { XONLINE_COUNTRY_GUATEMALA,    LOBBYAPI_COUNTRY_GUATEMALA },
    { XONLINE_COUNTRY_HONG_KONG,    LOBBYAPI_COUNTRY_HONG_KONG },
    { XONLINE_COUNTRY_HONDURAS,     LOBBYAPI_COUNTRY_HONDURAS },
    { XONLINE_COUNTRY_CROATIA,      LOBBYAPI_COUNTRY_CROATIA },
    { XONLINE_COUNTRY_HUNGARY,      LOBBYAPI_COUNTRY_HUNGARY },
    { XONLINE_COUNTRY_INDONESIA,    LOBBYAPI_COUNTRY_INDONESIA },
    { XONLINE_COUNTRY_IRELAND,      LOBBYAPI_COUNTRY_IRELAND },
    { XONLINE_COUNTRY_ISRAEL,       LOBBYAPI_COUNTRY_ISRAEL },
    { XONLINE_COUNTRY_INDIA,        LOBBYAPI_COUNTRY_INDIA },
    { XONLINE_COUNTRY_IRAQ,         LOBBYAPI_COUNTRY_IRAQ },
    { XONLINE_COUNTRY_IRAN,         LOBBYAPI_COUNTRY_IRAN },
    { XONLINE_COUNTRY_ICELAND,      LOBBYAPI_COUNTRY_ICELAND },
    { XONLINE_COUNTRY_ITALY,        LOBBYAPI_COUNTRY_ITALY },
    { XONLINE_COUNTRY_JAMAICA,      LOBBYAPI_COUNTRY_JAMAICA },
    { XONLINE_COUNTRY_JORDAN,       LOBBYAPI_COUNTRY_JORDAN },
    { XONLINE_COUNTRY_JAPAN,        LOBBYAPI_COUNTRY_JAPAN },
    { XONLINE_COUNTRY_KENYA,        LOBBYAPI_COUNTRY_KENYA },
    { XONLINE_COUNTRY_KYRGYZSTAN,   LOBBYAPI_COUNTRY_KYRGYZSTAN },
    { XONLINE_COUNTRY_KOREA,        LOBBYAPI_COUNTRY_KOREA_REPUBLIC_OF },
    { XONLINE_COUNTRY_KUWAIT,       LOBBYAPI_COUNTRY_KUWAIT },
    { XONLINE_COUNTRY_KAZAKHSTAN,   LOBBYAPI_COUNTRY_KAZAKHSTAN },
    { XONLINE_COUNTRY_LEBANON,      LOBBYAPI_COUNTRY_LEBANON },
    { XONLINE_COUNTRY_LIECHTENSTEIN, LOBBYAPI_COUNTRY_LIECHTENSTEIN },
    { XONLINE_COUNTRY_LITHUANIA,    LOBBYAPI_COUNTRY_LITHUANIA },
    { XONLINE_COUNTRY_LUXEMBOURG,   LOBBYAPI_COUNTRY_LUXEMBOURG },
    { XONLINE_COUNTRY_LATVIA,       LOBBYAPI_COUNTRY_LATVIA },
    { XONLINE_COUNTRY_LIBYA,        LOBBYAPI_COUNTRY_LIBYAN_ARAB_JAMAHIRIYA },
    { XONLINE_COUNTRY_MOROCCO,      LOBBYAPI_COUNTRY_MOROCCO },
    { XONLINE_COUNTRY_MONACO,       LOBBYAPI_COUNTRY_MONACO },
    { XONLINE_COUNTRY_MACEDONIA,    LOBBYAPI_COUNTRY_MACEDONIA_THE_FORMER_YUGOSLAV_REPUBLIC_OF },
    { XONLINE_COUNTRY_MONGOLIA,     LOBBYAPI_COUNTRY_MONGOLIA },
    { XONLINE_COUNTRY_MACAU,        LOBBYAPI_COUNTRY_MACAU },
    { XONLINE_COUNTRY_MALDIVES,     LOBBYAPI_COUNTRY_MALDIVES },
    { XONLINE_COUNTRY_MEXICO,       LOBBYAPI_COUNTRY_MEXICO },
    { XONLINE_COUNTRY_MALAYSIA,     LOBBYAPI_COUNTRY_MALAYSIA },
    { XONLINE_COUNTRY_NICARAGUA,    LOBBYAPI_COUNTRY_NICARAGUA },
    { XONLINE_COUNTRY_NETHERLANDS,  LOBBYAPI_COUNTRY_NETHERLANDS },
    { XONLINE_COUNTRY_NORWAY,       LOBBYAPI_COUNTRY_NORWAY },
    { XONLINE_COUNTRY_NEW_ZEALAND,  LOBBYAPI_COUNTRY_NEW_ZEALAND },
    { XONLINE_COUNTRY_OMAN,         LOBBYAPI_COUNTRY_OMAN },
    { XONLINE_COUNTRY_PANAMA,       LOBBYAPI_COUNTRY_PANAMA },
    { XONLINE_COUNTRY_PERU,         LOBBYAPI_COUNTRY_PERU },
    { XONLINE_COUNTRY_PHILIPPINES,  LOBBYAPI_COUNTRY_PHILIPPINES },
    { XONLINE_COUNTRY_PAKISTAN,     LOBBYAPI_COUNTRY_PAKISTAN },
    { XONLINE_COUNTRY_POLAND,       LOBBYAPI_COUNTRY_POLAND },
    { XONLINE_COUNTRY_PUERTO_RICO,  LOBBYAPI_COUNTRY_PUERTO_RICO },
    { XONLINE_COUNTRY_PORTUGAL,     LOBBYAPI_COUNTRY_PORTUGAL },
    { XONLINE_COUNTRY_PARAGUAY,     LOBBYAPI_COUNTRY_PARAGUAY },
    { XONLINE_COUNTRY_QATAR,        LOBBYAPI_COUNTRY_QATAR },
    { XONLINE_COUNTRY_ROMANIA,      LOBBYAPI_COUNTRY_ROMANIA },
    { XONLINE_COUNTRY_RUSSIAN_FEDERATION, LOBBYAPI_COUNTRY_RUSSIAN_FEDERATION },
    { XONLINE_COUNTRY_SAUDI_ARABIA, LOBBYAPI_COUNTRY_SAUDI_ARABIA },
    { XONLINE_COUNTRY_SWEDEN,       LOBBYAPI_COUNTRY_SWEDEN },
    { XONLINE_COUNTRY_SINGAPORE,    LOBBYAPI_COUNTRY_SINGAPORE },
    { XONLINE_COUNTRY_SLOVENIA,     LOBBYAPI_COUNTRY_SLOVENIA },
    { XONLINE_COUNTRY_SLOVAK_REPUBLIC, LOBBYAPI_COUNTRY_SLOVAKIA_ },
    { XONLINE_COUNTRY_EL_SALVADOR,  LOBBYAPI_COUNTRY_EL_SALVADOR },
    { XONLINE_COUNTRY_SYRIA,        LOBBYAPI_COUNTRY_SYRIAN_ARAB_REPUBLIC },
    { XONLINE_COUNTRY_THAILAND,     LOBBYAPI_COUNTRY_THAILAND },
    { XONLINE_COUNTRY_TUNISIA,      LOBBYAPI_COUNTRY_TUNISIA },
    { XONLINE_COUNTRY_TURKEY,       LOBBYAPI_COUNTRY_TURKEY },
    { XONLINE_COUNTRY_TRINIDAD_AND_TOBAGO, LOBBYAPI_COUNTRY_TRINIDAD_AND_TOBAGO },
    { XONLINE_COUNTRY_TAIWAN,       LOBBYAPI_COUNTRY_TAIWAN },
    { XONLINE_COUNTRY_UKRAINE,      LOBBYAPI_COUNTRY_UKRAINE },
    { XONLINE_COUNTRY_UNITED_STATES, LOBBYAPI_COUNTRY_UNITED_STATES },
    { XONLINE_COUNTRY_URUGUAY,      LOBBYAPI_COUNTRY_URUGUAY },
    { XONLINE_COUNTRY_UZBEKISTAN,   LOBBYAPI_COUNTRY_UZBEKISTAN },
    { XONLINE_COUNTRY_VENEZUELA,    LOBBYAPI_COUNTRY_VENEZUELA },
    { XONLINE_COUNTRY_VIET_NAM,     LOBBYAPI_COUNTRY_VIETNAM },
    { XONLINE_COUNTRY_YEMEN,        LOBBYAPI_COUNTRY_YEMEN },
    { XONLINE_COUNTRY_SOUTH_AFRICA, LOBBYAPI_COUNTRY_SOUTH_AFRICA },
    { XONLINE_COUNTRY_ZIMBABWE,     LOBBYAPI_COUNTRY_ZIMBABWE },
    { 0xffff,                       0xffff  } // terminator
};

static NetConnRefT  *_NetConn_pRef = NULL;

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function   _NetConnUpdateConnStatus

    \Description
        Update the Connection Status and provide logging of status changes

    \Input *pRef             - pointer to net NetConn module ref
    \Input *uNewConnStatus   - the new conn status

    \Version 01/19/2015 (tcho)
*/
/********************************************************************************F*/
static void _NetConnUpdateConnStatus(NetConnRefT *pRef, uint32_t uNewConnStatus)
{
    char strConnStatus[5];

    pRef->uConnStatus = uNewConnStatus;
    ds_memcpy_s(strConnStatus, 4, (char *)&uNewConnStatus, sizeof(uint32_t)); 
    strConnStatus[4] = 0;

    NetPrintf(("netconnxenon: netconn status changed to %s\n", strConnStatus));
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
    \Function _NetConnExtractSitename

    \Description
        Extract the XLSP sitename from a parameter list

    \Input *pRef    - netconn module ref
    \Input *pParams - parameter list for example: "-option=value -xlspsitename=blah -secure"

    \Version 10/19/2010 (jrainy)
*/
/********************************************************************************F*/
static void _NetConnExtractSitename(NetConnRefT *pRef, const char *pParams)
{
    // early exit if parameter string is NULL
    if (!pParams)
    {
        return;
    }

    _NetConnCopyParam(pRef->strXlspSitename, sizeof(pRef->strXlspSitename), "-xlspsitename=", pParams, pRef->strXlspSitename);

    NetPrintf(("netconnxenon: setting xlspsitename=%s\n", pRef->strXlspSitename));
}

/*F********************************************************************************/
/*!
    \Function    _NetConnParseParams

    \Description
        Parse and handle input parameters to NetConnStartup().

    \Input *pRef    - module state
    \Input *pParams - startup parameters

    \Version 09/14/2012 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnParseParams(NetConnRefT *pRef, const char *pParams)
{
    char strTemp[32];
    const char *pTemp;

    // set if we are using the friends module
    memset(strTemp, 0, sizeof(strTemp));
    _NetConnCopyParam(strTemp, sizeof(strTemp), "-friend=", pParams, "true");
    pRef->bUseFriendModule = TRUE;
    NetPrintf(("netconnxenon: setting friend=%s\n", strTemp));
    if (ds_strnicmp(strTemp, "false", 4) == 0)
    {
        pRef->bUseFriendModule = FALSE;
    }

    // set thread stack size
    memset(strTemp, 0, sizeof(strTemp));
    _NetConnCopyParam(strTemp, sizeof(strTemp), "-threadstacksize=", pParams, "0");
    pRef->iThreadStackSize = strtol(strTemp, NULL, 10);
    NetPrintf(("netconnxenon: setting threadstacksize=%s\n", (pRef->iThreadStackSize?strTemp:"default")));

    // set xlsp service id
    pRef->uTitleServerServiceId = NETCONN_TITLESERVER_SERVICEID;
    if ((pTemp = strstr(pParams, "-xlspserviceid=")) != NULL)
    {
        pRef->uTitleServerServiceId = strtol(pTemp + 15, NULL, 16);
    }
    NetPrintf(("netconnxenon: setting xlspserviceid=0x%08x\n", pRef->uTitleServerServiceId));

    // set xlsp sitename
    ds_strnzcpy(pRef->strXlspSitename, "gos-core", sizeof(pRef->strXlspSitename));
    _NetConnExtractSitename(pRef, pParams);
}

/*F********************************************************************************/
/*!
    \Function _NetConnXnetInit

    \Description
        InitializesXbox network stack.

    \Input *pRef        - netconn module ref
    \Input iKeyRegMax   - max number of simultaneous registered keys
    \Input bInsecure    - TRUE if security should be disabled, else FALSE.

    \Output
        HRESULT         - S_OK on success, E_FAIL on failure.

    \Version 03/20/2004 (jbrookes)
*/
/********************************************************************************F*/
static HRESULT _NetConnXnetInit(NetConnRefT *pRef, int32_t iKeyRegMax, uint32_t bInsecure)
{
    XNetStartupParams XNetParams;
    int32_t iResult;

    // set up XNet parameters
    ZeroMemory(&XNetParams, sizeof(XNetParams));
    XNetParams.cfgSizeOfStruct = sizeof(XNetParams);
    XNetParams.cfgKeyRegMax = iKeyRegMax;
    if (bInsecure == TRUE)
    {
        XNetParams.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
        NetPrintf(("netconnxenon: starting up in insecure mode\n"));
    }

    // start XNet
    if ((iResult = XNetStartup(&XNetParams)) != NO_ERROR)
    {
        NetPrintf(("netconnxenon: unable to start xnet (err=%s)\n", DirtyErrGetName(iResult)));
        return(E_FAIL);
    }

    return(S_OK);
}

/*F********************************************************************************/
/*!
    \Function _NetConnLocaleTranslate

    \Description
        Translate Xbox 360 native Country/Locale/Language to LOBBYAPI_* defines
        representing ISO standards.

    \Input aMap         - lookup table to convert Xbox 360 to ISO
    \Input uValue       - native Xbox 360 value to translate
    \Input uDefault     - default value to use if source value not found

    \Output
        uint16_t        - translated value, or default if not found

    \Version 12/19/2011 (jbrookes)
*/
/********************************************************************************F*/
static uint16_t _NetConnLocaleTranslate(uint16_t aMap[][2], uint16_t uValue, uint16_t uDefault)
{
    int32_t iMap;
    uint16_t uResult;

    for (iMap = 0, uResult = uDefault; aMap[iMap][0] != 0xffff; iMap += 1)
    {
        if (aMap[iMap][0] == uValue)
        {
            uResult = aMap[iMap][1];
            break;
        }
    }

    return(uResult);
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdateIspStatus

    \Description
        Update status of connection to ISP.

    \Input *pRef        - pointer to NetConn module ref
    \Input iNetStatus   - current network status as returned by XNGetTitleXnAddr

    \Output
        uint32_t    - four-byte status code

    \Version 03/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _NetConnUpdateIspStatus(NetConnRefT *pRef, int32_t iNetStatus)
{
    // if we're system link, all we need is an ethernet cable plugged in
    if ((pRef->bSyslink) && (XNetGetEthernetLinkStatus() != 0))
    {
        pRef->bIspConnected = TRUE;
        return('+isp');
    }

    // check for initial condition
    if (iNetStatus == 0)
    {
        return('~ini');
    }

    // check first for an error condition
    if (iNetStatus & XNET_GET_XNADDR_NONE)
    {
        return('-ini');
    }
    if (iNetStatus & XNET_GET_XNADDR_TROUBLESHOOT)
    {
        return('-trb');
    }

    // is the system interface up and ready yet?
    if (iNetStatus & (XNET_GET_XNADDR_GATEWAY|XNET_GET_XNADDR_DNS))
    {
        pRef->bIspConnected = TRUE;
        return('+isp');
    }

    // check progress
    if (iNetStatus & XNET_GET_XNADDR_PENDING)
    {
        return('~pnd');
    }
    if (iNetStatus & XNET_GET_XNADDR_ETHERNET)
    {
        return('~eth');
    }
    if (iNetStatus & XNET_GET_XNADDR_DNS)
    {
        return('~dns');
    }
    if (iNetStatus & XNET_GET_XNADDR_STATIC)
    {
        return('~sta');
    }
    if (iNetStatus & XNET_GET_XNADDR_DHCP)
    {
        return('~dhp');
    }
    if (iNetStatus & XNET_GET_XNADDR_PPPOE)
    {
        return('~ppp');
    }

    return('-\?\?\?');
}

/*F********************************************************************************/
/*!
    \Function _NetConnSetRedirect

    \Description
        Set default redirect to some address

    \Input *pRef        - pointer to module state
    \Input uAddr        - address to redirect to

    \Version 07/11/2010 (jrainy)
*/
/********************************************************************************F*/
static void _NetConnSetRedirect(NetConnRefT *pRef, uint32_t uAddr)
{
    NetPrintf(("netconnxenon: setting default sg redirection behaviour\n"));
    // set up initial address mapping to redirect all connection attempts to the SG
    memset(pRef->SocketMap, 0, sizeof(pRef->SocketMap));
    pRef->SocketMap[0].uAddr = 0;
    pRef->SocketMap[0].uMask = 0;
    pRef->SocketMap[0].uRemap = ntohl(uAddr);
    SocketControl(NULL, 'xmap', 1, &pRef->SocketMap, NULL);
    SocketControl(NULL, 'xdns', 0x7f000001, NULL, NULL);
}

/*F********************************************************************************/
/*!
    \Function _NetConnGetAccountInfoStart

    \Description
        Initiate an account information query.

    \Input *pRef    - state ref
    \Input *pUser   - user to get info for
    \Input iUser    - index of user

    \Version 10/15/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnGetAccountInfoStart(NetConnRefT *pRef, NetConnUserT *pUser, int32_t iUser)
{
    int32_t iUserInfoSize = XAccountGetUserInfoResponseSize();
    int32_t iResult;

    // allocate user info if we haven't yet
    if (pUser->pUserInfo == NULL)
    {
        pUser->pUserInfo = (GET_USER_INFO_RESPONSE *) DirtyMemAlloc(iUserInfoSize, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        if (pUser->pUserInfo == NULL)
        {
            NetPrintf(("netconnxenon: could not allocate user info buffer\n"));
            pUser->eInfoState = ST_USERINFO_FAIL;
            return;
        }
    }

    // get user info
    memset(&pRef->XOverlapped, 0, sizeof(pRef->XOverlapped));
    memset(pUser->pUserInfo, 0, iUserInfoSize);
    if ((iResult = _XAccountGetUserInfo(iUser, iUserInfoSize, pUser->pUserInfo, &pRef->XOverlapped)) == ERROR_IO_PENDING)
    {
        pUser->eInfoState = ST_USERINFO_PROC;
        pRef->bUserInfoQuery = TRUE;
    }
    else
    {
        NetPrintf(("netconnxenon: _XAccountGetUserInfo() failed for user %d: error=%s\n", iUser,
            DirtyErrGetName(XGetOverlappedExtendedError(&pRef->XOverlapped))));
        DirtyMemFree(pUser->pUserInfo, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        pUser->pUserInfo = NULL;
        pUser->eInfoState = ST_USERINFO_FAIL;
        pRef->bUserInfoQuery = FALSE;
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnGetAccountInfoProcess

    \Description
        Update an ongoing account information query.

    \Input *pRef    - state ref
    \Input *pUser   - user info is being queried for
    \Input iUser    - index of user

    \Version 10/15/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnGetAccountInfoProcess(NetConnRefT *pRef, NetConnUserT *pUser, int32_t iUser)
{
    int32_t iResult;

    // fast check to see if operation has completed
    if (XHasOverlappedIoCompleted(&pRef->XOverlapped) == FALSE)
    {
        return;
    }
    pRef->bUserInfoQuery = FALSE;

    // get the result
    if ((iResult = XGetOverlappedExtendedError(&pRef->XOverlapped)) == ERROR_SUCCESS)
    {
        NetPrintf(("netconnxenon: acquired user info for user [%d] %s\n", iUser, pUser->strGamertag));
        NetPrintf(("   firstName:    %S\n", pUser->pUserInfo->wszFirstName));
        NetPrintf(("   lastName:     %S\n", pUser->pUserInfo->wszLastName));
        NetPrintf(("   street1:      %S\n", pUser->pUserInfo->addressInfo.wszStreet1));
        NetPrintf(("   city:         %S\n", pUser->pUserInfo->addressInfo.wszCity));
        NetPrintf(("   state:        %S\n", pUser->pUserInfo->addressInfo.wszState));
        NetPrintf(("   postalCode:   %S\n", pUser->pUserInfo->addressInfo.wszPostalCode));
        NetPrintf(("   email:        %S\n", pUser->pUserInfo->wszEmail));
        NetPrintf(("   wLanguageId:  %d\n", pUser->pUserInfo->wLanguageId));
        NetPrintf(("   bCountryId:   %d\n", pUser->pUserInfo->bCountryId));
        NetPrintf(("   bParterOptIn: %d\n", pUser->pUserInfo->bParterOptIn));
        NetPrintf(("   bAge:         %d\n", pUser->pUserInfo->bAge));
        pUser->eInfoState = ST_USERINFO_DONE;
    }
    else
    {
        NetPrintf(("netconnxenon: _XAccountGetUserInfo() failed: error=%s\n", DirtyErrGetName(iResult)));
        DirtyMemFree(pUser->pUserInfo, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        pUser->pUserInfo = NULL;
        pUser->eInfoState = ST_USERINFO_FAIL;
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdateAccountInfo

    \Description
        Get account information if required.

    \Input *pRef    - state ref

    \Output
        int32_t     - TRUE if all account info processing is complete, else FALSE

    \Version 10/15/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetConnUpdateAccountInfo(NetConnRefT *pRef)
{
    NetConnUserT *pUser;
    int32_t iUser;

    // get user info for all logged in users
    for (iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
    {
        pUser = &pRef->aUsers[iUser];
        if ((pUser->eInfoState == ST_USERINFO_STRT) && (pRef->bUserInfoQuery == FALSE))
        {
            _NetConnGetAccountInfoStart(pRef, pUser, iUser);
            break;
        }
        else if (pUser->eInfoState == ST_USERINFO_PROC)
        {
            _NetConnGetAccountInfoProcess(pRef, pUser, iUser);
            break;
        }
    }

    // return if we are done or not
    return(iUser == NETCONN_MAXLOCALUSERS);
}

/*F********************************************************************************/
/*!
    \Function _NetConnGetUserInfo

    \Description
        Get user info for user at given index.

    \Input *pUser           - [out] storage for user info
    \Input iUser            - index of user to get info for

    \Output
        int32_t             - zero=success, negative=failure

    \Version 07/12/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnGetUserInfo(NetConnUserT *pUser, int32_t iUser)
{
    char strTempGamertag[16];
    XUID Xuid;

    // get XUID and name for given user index
    XUserGetXUID(iUser, &Xuid);
    XUserGetName(iUser, strTempGamertag, sizeof(strTempGamertag));

    // if they haven't changed, skip update
    if ((pUser->Xuid == Xuid) && (DirtyUsernameCompare(pUser->strGamertag, strTempGamertag) == 0))
    {
        return;
    }
    NetPrintf(("netconnxenon: user index %d has changed\n", iUser));

    // save updated xuid and/or gamertag
    pUser->Xuid = Xuid;
    ds_strnzcpy(pUser->strGamertag, strTempGamertag, sizeof(pUser->strGamertag));

    // print out xuid and name
    #if DIRTYCODE_LOGGING
    {
        DirtyAddrT DirtyAddr;
        DirtyAddrFromHostAddr(&DirtyAddr, &pUser->Xuid);
        NetPrintf(("netconnxenon: %s=%s\n", pUser->strGamertag, DirtyAddr.strMachineAddr));
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function _NetConnGetLogonUsers

    \Description
        Get logged-on users.

    \Input *pRef        - pointer to module state

    \Output
        uint32_t        - four-byte status code

    \Version 04/08/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _NetConnGetLogonUsers(NetConnRefT *pRef)
{
    int32_t iUser;

    // get user info for all logged in users
    for (iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
    {
        NetConnUserT *pUser = &pRef->aUsers[iUser];

        if (XUserGetSigninState(iUser) == eXUserSigninState_SignedInToLive)
        {
            _NetConnGetUserInfo(pUser, iUser);
        }
        else
        {
            if (pUser->strGamertag[0] != '\0')
            {
                NetPrintf(("netconnxenon: user %s at index %d signed out\n", pUser->strGamertag, iUser));
            }
            if (pUser->pUserInfo != NULL)
            {
                DirtyMemFree(pUser->pUserInfo, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
            }
            memset(pUser, 0, sizeof(*pUser));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnGetUserAccountInfo

    \Description
        Return pointer to user info for user at specified index

    \Input *pRef        - pointer to module state
    \Input iIndex       - index of user to get info for

    \Output
        GET_USER_INFO_RESPONSE * - pointer to user info, or NULL if it is not available

    \Version 01/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static GET_USER_INFO_RESPONSE *_NetConnGetUserAccountInfo(NetConnRefT *pRef, int32_t iIndex)
{
    // validate index range
    if ((iIndex < 0) || (iIndex >= NETCONN_MAXLOCALUSERS))
    {
        NetPrintf(("netconnxenon: user info requested from invalid user index %d\n", iIndex));
        return(NULL);
    }

    // if we have info, return it
    if ((pRef->aUsers[iIndex].pUserInfo != NULL) && (pRef->aUsers[iIndex].eInfoState == ST_USERINFO_DONE))
    {
        return(pRef->aUsers[iIndex].pUserInfo);
    }

    // not found/available
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _NetConnXblUserMask

    \Description
        return the bitmask of logged-in users.

    \Output
        uint32_t    - number of users logged into xbox live (0-4)

    \Version 07/15/2008 (cvienneau)
*/
/********************************************************************************F*/
static uint32_t _NetConnXblUserMask(void)
{
    uint32_t iIndex;
    uint32_t uUserMask = 0;

    for (iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        if (XUserGetSigninState(iIndex) == eXUserSigninState_SignedInToLive)
        {
            uUserMask |= (1 << iIndex);
        }
    }
    return(uUserMask);
}

/*F********************************************************************************/
/*!
    \Function _NetConnNumXblUsers

    \Description
        Count the number of users currently logged into xbox live from this console.

    \Output
        uint32_t    - number of users logged into xbox live (0-4)

    \Version 07/15/2008 (cvienneau)
*/
/********************************************************************************F*/
static uint32_t _NetConnNumXblUsers(void)
{
    uint32_t iIndex;
    uint32_t uNumUsers = 0;

    for (iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        if (XUserGetSigninState(iIndex) == eXUserSigninState_SignedInToLive)
        {
            ++uNumUsers;
        }
    }
    return(uNumUsers);
}

#ifdef DIRTYSDK_XHTTP_ENABLED
/*F********************************************************************************/
/*!
    \Function _NetConnRequestAuthToken

    \Description
        Initiate async op for acquisition of Security Token from MS STS.

    \Input *pRef            - state ref
    \Input iUserIndex       - user index

    \Version 02/18/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnRequestAuthToken(NetConnRefT *pRef, int32_t iUserIndex)
{
    int32_t iResult;
    NetConnTokenT *pTokenSpace = &pRef->aUsers[iUserIndex].token;

    NetPrintf(("netconnxenon: acquiring XSTS token for user %d\n", iUserIndex));

    // make token request
    if ((iResult = DirtyAuthGetToken(iUserIndex, pRef->strXAuthUrn, TRUE)) == DIRTYAUTH_PENDING)
    {
        pTokenSpace->eTokenStatus = ST_TOKEN_INPROG;
    }
    else if (iResult == DIRTYAUTH_SUCCESS)
    {
        NetPrintf(("netconnxenon: token acquisition for user %d completed (was cached)\n", iUserIndex));

        pTokenSpace->eTokenStatus = ST_TOKEN_VALID;
    }
    else
    {
        NetPrintf(("netconnxenon: failed to initiate token acquisition for user %d\n", iUserIndex));

        pTokenSpace->eTokenStatus = ST_TOKEN_FAILED;
    }
}

/*F********************************************************************************/
/*!
    \Function _NetConnCheckAuthToken

    \Description
        Check if any pending async op for acquisition of Security Token from MS STS
        is completed.

    \Input *pRef            - state ref

    \Version 02/18/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnCheckAuthToken(NetConnRefT *pRef)
{
    int32_t iUserIndex, iResult;
    NetConnTokenT *pTokenSpace;

    for (iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; iUserIndex++)
    {
        pTokenSpace = &pRef->aUsers[iUserIndex].token;

        if (pTokenSpace->eTokenStatus == ST_TOKEN_INPROG)
        {
            int32_t iTokenLen;

            // make token request
            if ((iResult = DirtyAuthCheckToken(pRef->strXAuthUrn, &iTokenLen, NULL)) == DIRTYAUTH_SUCCESS)
            {
                NetPrintf(("netconnxenon: token acquisition for user %d completed - token length = %d\n", iUserIndex, iTokenLen));

                // save token acquisition tick
                pTokenSpace->iTokenAcquTick = NetTick();

                pTokenSpace->eTokenStatus = ST_TOKEN_VALID;
            }
            else if (iResult != DIRTYAUTH_PENDING)
            {
                NetPrintf(("netconnxenon: token acquisition failed for user %d\n", iUserIndex));

                pTokenSpace->eTokenStatus = ST_TOKEN_FAILED;
            }
        }
    }
}
#endif // DIRTYSDK_XHTTP_ENABLED

/*F********************************************************************************/
/*!
    \Function _NetConnShowSigninUI

    \Description
        Show the signin UI

    \Input *pRef    - pointer to NetConn module ref
    \Input uPanes   - The maximum number of players that can sign in.
    \Input uFlags   - Indicates the profile display and sign-in options.

    \Output
        uint32_t    - ERROR_SUCCESS if the function succeeds; otherwise, an error code.

    \Version 08/22/2011 (szhu)
*/
/********************************************************************************F*/
static uint32_t _NetConnShowSigninUI(NetConnRefT *pRef, uint32_t uPanes, uint32_t uFlags)
{
    if (pRef->pShowSigninUI != NULL)
    {
        return(pRef->pShowSigninUI(uPanes, uFlags));
    }
    return(XShowSigninUI(uPanes, uFlags));
}

/*F********************************************************************************/
/*!
    \Function _NetConnXblLogin

    \Description
        Prompt a user to login an xbox live enabled accout, if there are none already

    \Output
        uint32_t    - number of users logged into xbox live (0-4) at time of call.

    \Version 07/15/2008 (cvienneau)
*/
/********************************************************************************F*/
static uint32_t _NetConnXblLogin(NetConnRefT *pRef)
{
    uint32_t uUserMask = _NetConnXblUserMask();
    uint32_t uNumUsers = _NetConnNumXblUsers();
    uint32_t uSigninFlags = XSSUI_FLAGS_SHOWONLYONLINEENABLED;

    // if guest login is allowed
    if (pRef->eGuestOption == GO_ENABLE)
    {
        uSigninFlags |= XSSUI_FLAGS_ENABLE_GUEST;
    }
    else if (pRef->eGuestOption == GO_DISALLOW)
    {
        uSigninFlags |= XSSUI_FLAGS_DISALLOW_GUEST;
    }
    else
    {
        if (pRef->eGuestOption != GO_DEFAULT)
        {
            NetPrintf(("netconnxenon: unrecognized eGuestOption (%d) used in _NetConnXblLogin -- assumed to GO_DEFAULT\n", (int32_t)pRef->eGuestOption));
        }
        // default guest option, nothing to do here
    }

    // if noone is logged in
    if (uNumUsers < 1)
    {
        _NetConnShowSigninUI(pRef, 4, uSigninFlags);
    }

    return(uUserMask);
}

/*F********************************************************************************/
/*!
    \Function _NetConnSetupIsp

    \Description
        Called by _NetConnWaitingForIsp to bring up the ISP

    \Input
        *pRef       - pointer to NetConn module ref

    \Output
        uint32_t        - current connection status.

    \Version 07/15/2008 (cvienneau)
*/
/********************************************************************************F*/
static uint32_t _NetConnSetupIsp(NetConnRefT *pRef)
{
    // update ISP connection status
    _NetConnUpdateConnStatus(pRef, _NetConnUpdateIspStatus(pRef, pRef->iNetStatus));


    // register system link key?
    if ((pRef->bSyslink == TRUE) && (pRef->uConnStatus == '+isp') && (pRef->bSyskey == FALSE))
    {
        XNetCreateKey(&pRef->XnKid, &pRef->XnKey);
        XNetRegisterKey(&pRef->XnKid, &pRef->XnKey);
        pRef->bSyskey = TRUE;
    }
    return(pRef->uConnStatus);
}

/*F********************************************************************************/
/*!
    \Function _NetConnWaitingForIsp

    \Description
        Called by NetConnUpdate to bring up the ISP

    \Input
        *pRef      - pointer to NetConn module ref

    \Output
        uint8_t    - true if its still waiting

    \Version 10/03/2011 (cvienneau)
*/
/********************************************************************************F*/
static uint8_t _NetConnWaitingForIsp(NetConnRefT *pRef)
{
    uint32_t uCurrentTime;

    //if the isp isn't up yet, try bring it up, we'll wait if we must
    //first attempt
    if (!pRef->bIspConnected && pRef->uIspTimer == 0)
    {
        pRef->uIspTimer = NetTick();
        _NetConnSetupIsp(pRef);
    }
    //continued attempts
    else if (!pRef->bIspConnected)
    {
        uCurrentTime = NetTick();

        //if too much time has passed, or we're in an error state give up
        if ((NetTickDiff(uCurrentTime, pRef->uIspTimer) > NETCONN_MAX_WAIT_TIME_FOR_ISP) || ((pRef->uConnStatus >> 24) == '-'))
        {
            _NetConnUpdateConnStatus(pRef, '-svc');
            pRef->uIspTimer = 0;
            pRef->eState = ST_IDLE;
            NetPrintf(("netconnxenon: bring up isp failed\n"));
            return(FALSE);
        }
        _NetConnSetupIsp(pRef);
    }
    //succeeded
    if (pRef->bIspConnected)
    {
        pRef->uIspTimer = 0;
        return(FALSE);
    }
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdateXblStatus

    \Description
        Update status of connection to Xbox Live.

    \Input *pRef        - pointer to NetConn module ref
    \Input iNetStatus   - current network status as returned by XNGetTitleXnAddr

    \Output
        uint32_t        - four-byte status code

    \Version 03/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _NetConnUpdateXblStatus(NetConnRefT *pRef, int32_t iNetStatus)
{
    // check link status - takes ~2-3 milliseconds according to microsoft
    if ((XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE) == 0)
    {
        return('-svc');
    }

    /* get initial state of logged on users.  we should be able to rely
       on the notification listener for this, but if multiple listeners
       are created only the first listener gets the initial sign-in
       message, and netconn could therefore miss the event */
    _NetConnGetLogonUsers(pRef);

    // mark as connected
    pRef->bXblConnected = TRUE;
    return('+xbl');
}

/*F********************************************************************************/
/*!
    \Function _NetConnXblConnected

    \Description
        Returns whether we are connected to XBL or not.

    \Input uConnStatus  - current connection status

    \Output
        uint32_t        - TRUE if connected to XBL, else FALSE

    \Version 03/15/2004 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _NetConnXblConnected(uint32_t uConnStatus)
{
    return((uConnStatus == '+xbl') ? TRUE : FALSE);
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

    \Version 12/07/2009 (mclouatre)
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
            NetPrintf(("netconnxenon: unable to allocate memory for the external cleanup list\n"));
            return(-1);
        }
        memset(pNewList, 0, 2 * pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT));

        // copy contents of old list in contents of new list
        ds_memcpy(pNewList, pOldList, pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT));

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

    \Version 12/07/2009 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnProcessExternalCleanupList(NetConnRefT *pRef)
{
    int32_t iEntryIndex;
    int32_t iEntryIndex2;

    // make sure we don't re-enter this code; some modules may call NetConnStatus() during their cleanup
    if (pRef->bProcessingCleanupList)
    {
        return;
    }
    pRef->bProcessingCleanupList = TRUE;

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

            NetPrintf(("netconnxenon: successfully destroyed external module (cleanup data ptr = 0x%08x), external cleanup list count decremented to %d\n",
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

    // clear re-enter block
    pRef->bProcessingCleanupList = FALSE;
}

/*F*************************************************************************************/
/*!
    \Function _NetConnLSPCacheCallback

    \Description
        Handle DirtyLSP cache events

    \Input uAddress     - address for cache event
    \Input eCacheEvent  - kind of cache event
    \Input *pUserRef    - socket module state

    \Version 07/11/2010 (jrainy)
*/
/************************************************************************************F*/
static void _NetConnLSPCacheCallback(uint32_t uAddress, DirtyLSPCacheEventE eCacheEvent, void *pUserRef)
{
    NetConnRefT *pRef = (NetConnRefT *)pUserRef;

    // if DirtyLSP has expired our XLSP address, forget about it
    if (eCacheEvent == DIRTYLSP_CACHEEVENT_EXPIRED)
    {
        if (uAddress == pRef->uXlspAddress)
        {
            pRef->uXlspAddress = 0;
            pRef->uLastUpdateTime = 0;
        }
    }
}

/*F*************************************************************************************/
/*!
    \Function _NetConnStartXLSPResolution

    \Description
        Kick off the resolution of the XLSP gateway

    \Input *pRef     - netconn module state

    \Version 07/11/2010 (jrainy)
*/
/************************************************************************************F*/
static void _NetConnStartXLSPResolution(NetConnRefT *pRef)
{
    pRef->uXlspAddress = 0;
    pRef->uLastUpdateTime = NetTick();

    // clear xlsp global mapping table
    DirtyLSPControl(pRef->pDirtyLSP, 'xclr', 0, 0, NULL);

    pRef->pXlspRec = DirtyLSPQueryStart(pRef->pDirtyLSP, pRef->strXlspSitename);
}

/*F*************************************************************************************/
/*!
    \Function _NetConnUpdateXLSPResolution

    \Description
        Update/poll the resolution of the XLSP gateway

    \Input *pRef    - netconn module state

    \Output
        int32_t     - 0=in progress, positive=success, negative=failure

    \Version 07/11/2010 (jrainy)
*/
/************************************************************************************F*/
static int32_t _NetConnUpdateXLSPResolution(NetConnRefT *pRef)
{
    int32_t iRet;

    if ((iRet = DirtyLSPQueryDone(pRef->pDirtyLSP, pRef->pXlspRec)) != 0)
    {
        if (iRet > 0)
        {
            pRef->uXlspAddress = pRef->pXlspRec->uAddress;
            pRef->uLastUpdateTime = NetTick();
            NetPrintf(("netconnxenon: resolved XLSP %s to %a\n", pRef->pXlspRec->strName, pRef->pXlspRec->uAddress));

            if (pRef->bRedirect)
            {
                _NetConnSetRedirect(pRef, pRef->pXlspRec->uAddress);
            }
        }
        else
        {
            NetPrintf(("netconnxenon: failed to resolved XLSP with error %d\n", iRet));
            pRef->uIspTimer = 0;
        }

        DirtyLSPQueryFree(pRef->pDirtyLSP, pRef->pXlspRec);
        pRef->pXlspRec = NULL;
    }
    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function _NetConnConnect

    \Description
        Start connection process

    \Input *pRef     - netconn module state

    \Version 05/13/2010 (mclouatre)
*/
/********************************************************************************F*/
static void _NetConnConnect(NetConnRefT *pRef)
{
    NetPrintf(("netconnxenon: connecting...\n"));

    // pass connect message to socket module
    SocketControl(NULL, 'conn', 0, NULL, NULL);
}

/*F********************************************************************************/
/*!
    \Function _NetConnUpdate

    \Description
        Update status of NetConn module.  This function is called by NetConnIdle.

    \Input *pData   - pointer to NetConn module ref
    \Input uCurTick - current millisecond tick counter

    \Version 1.0 03/15/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static void _NetConnUpdate(void *pData, uint32_t uCurTick)
{
    NetConnRefT *pRef = (NetConnRefT *)pData;
    DWORD dwNotify;
    ULONG ulParam;
    int32_t iResult;

    // get XNet status
    pRef->iNetStatus = XNetGetTitleXnAddr(&pRef->XnAddr);

    // make sure there is not already an invite accept notification pending
    if (pRef->ulInvitePendingCtrl == NETCONN_INVALID_CONTROLLER_INDEX)
    {
        // poll for notification - these can come while offline, so always poll regardless of network state
        if (XNotifyGetNext(pRef->hListener, XN_LIVE_INVITE_ACCEPTED, &dwNotify, &ulParam) != 0)
        {
            NetPrintf(("netconnxenon: invite accept notification pending on controller %d\n", ulParam));
            pRef->ulInvitePendingCtrl =  ulParam;
        }
    }

    // update friends
    if (_NetConn_pRef->pFriendApiRef)
    {
        FriendApiUpdate(pRef->pFriendApiRef);
    }

    // try to empty external cleanup list
    _NetConnProcessExternalCleanupList(pRef);

#ifdef DIRTYSDK_XHTTP_ENABLED
    // check for completin of pending token acquisition operation
    _NetConnCheckAuthToken(pRef);
#endif

    // keep-alive gos-core connection
    if ((pRef->uXlspAddress != 0) && (NetTickDiff(uCurTick, pRef->uLastUpdateTime) >= NETCONN_XLSP_UPDATE_TIME))
    {
        DirtyLSPConnectionUpdate(pRef->uXlspAddress);
        pRef->uLastUpdateTime = uCurTick;
    }

    // process gamertag lookup operation
    if (pRef->uLookUpInProgress)
    {
        if (XHasOverlappedIoCompleted(&pRef->XLookUpOverlapped))
        {
            pRef->uLookUpInProgress = FALSE;
            NetPrintf(("netconnxenon: Gamertag is %s\n", pRef->pFindUserResponse->pUsers->szGamerTag));

            strncpy(pRef->UserLookedUp.strName, pRef->pFindUserResponse->pUsers->szGamerTag, sizeof(pRef->UserLookedUp.strName));
            pRef->UserLookedUp.pRawData1 = pRef->pFindUserResponse->pUsers;

            if (pRef->UserLookUpCallback != NULL)
            {
                pRef->UserLookUpCallback(&pRef->UserLookedUp, pRef->pUserLookedUpData);
            }
        }
    }

    // state-based update
    switch(pRef->eState)
    {
        // waiting for external cleanup list to be empty
        case ST_EXT_CLEANUP:
        {
            if (pRef->pExternalCleanupList[0].pCleanupCb == NULL)
            {
                pRef->eState = ST_STAR;
            }
        }
        break;

        // connecting to ISP
        case ST_STAR:
        {
            if ((_NetConnWaitingForIsp(pRef) == FALSE) && (pRef->bDoConnect == TRUE))
            {
                pRef->bDoConnect = FALSE;
                _NetConnConnect(pRef);
                if (pRef->bSilent == TRUE)
                {
                    pRef->eState = ST_CONN;
                }
                else
                {
                    pRef->eState = ST_START_LOG;
                }
            }
        }
        break;

        // prepare for connecting to xbl (user login), and ensure isp is up
        case ST_START_LOG:
        {
            //consume any outstanding UI events so ST_END_LOG doesn't see it
            while (XNotifyGetNext(pRef->hListener, XN_SYS_UI, &dwNotify, &ulParam))
                ;

            _NetConnXblLogin(pRef);
            pRef->eState = ST_END_LOG;
        }
        break;

        // complete login/connection attempt
        case ST_END_LOG:
        {
            //continue with connection if we have anyone logged in
            if (_NetConnXblUserMask() != 0)
            {
                pRef->eState = ST_CONN;
                break;
            }

            //if they closed the UI but we don't have enough logged in users go back to idle state, we can't connect
            if (XNotifyGetNext(pRef->hListener, XN_SYS_UI, &dwNotify, &ulParam) != 0)
            {
                // Documentation BOOL fShow Set to TRUE if the UI is about to be displayed and FALSE if it is about to be hidden.
                if (ulParam == FALSE)
                {
                    NetPrintf(("netconnxenon: No signed in users. Bailing\n"));
                    _NetConnUpdateConnStatus(pRef, '-svc');
                    pRef->eState = ST_IDLE;
                }
            }
        }
        break;

        // connecting to xbox live
        case ST_CONN:
        {
            // only update status if we're not in an error state
            if ((pRef->uConnStatus >> 24) != '-')
            {
                _NetConnUpdateConnStatus(pRef, _NetConnUpdateXblStatus(pRef, pRef->iNetStatus));
            }

            // are we connected to XBL?
            if (_NetConnXblConnected(pRef->uConnStatus))
            {
                if (pRef->bUseXLSP)
                {
                    _NetConnStartXLSPResolution(pRef);
                }
                pRef->eState = ST_XLSP_GATE;
            }
        }
        break;

        // connecting to XLSP
        case ST_XLSP_GATE:
        {
            if ((!pRef->bUseXLSP) || (iResult = _NetConnUpdateXLSPResolution(pRef)) > 0)
            {
                // should xauth be started?
                if (pRef->bUseXhttp)
                {
                    if (_NetConnNumXblUsers() > 0)
                    {
                        // now that we know that at least one user is signed in, we can start xhttp/xauth if need be
                        // from MS doc: "Before calling XAuthStartup, you must make sure there is at least one user connected to Xbox LIVE, by calling XUserGetSigninState."
                        if ((pRef->pDirtyAuth = DirtyAuthCreate(pRef->bXhttpNoSecure)) == NULL)
                        {
                            NetPrintf(("netconnxenon: error creating DirtyAuth\n"));
                            _NetConnUpdateConnStatus(pRef, '-xht');
                        }
                        else
                        {
                            NetPrintf(("netconnxenon: xhttp use enabled\n"));
                        }
                    }
                    else
                    {
                        NetPrintf(("netconnxenon: error - creating DirtyAuth requires at least one user to be signed in\n"));
                        _NetConnUpdateConnStatus(pRef, '-xht');
                    }
                }

                if ((pRef->uConnStatus >> 24) != '-')
                {
                    _NetConnUpdateConnStatus(pRef, '+onl');
                }

                pRef->eState = ST_IDLE;
            }
            else if (iResult < 0)
            {
                _NetConnUpdateConnStatus(pRef, '-svc');
                pRef->eState = ST_IDLE;
            }
        }
        break;

        case ST_IDLE:
        {
            // don't do anything if we're not connected
            if (pRef->bXblConnected == FALSE)
            {
                break;
            }

            // poll for notifications
            if (XNotifyGetNext(pRef->hListener, XN_LIVE_CONNECTIONCHANGED, &dwNotify, &ulParam) != 0)
            {
                #if DIRTYCODE_LOGGING
                NetPrintf(("netconnxenon: connection change->%s\n", DirtyErrGetName(ulParam)));
                #endif
                if (ulParam != XONLINE_S_LOGON_CONNECTION_ESTABLISHED)
                {
                    if (ulParam == XONLINE_E_LOGON_NO_NETWORK_CONNECTION)
                    {
                        _NetConnUpdateConnStatus(pRef, '-net');
                    }
                    else if ((ulParam == XONLINE_E_LOGON_CANNOT_ACCESS_SERVICE) ||
                        (ulParam == XONLINE_E_LOGON_SERVICE_NOT_REQUESTED) ||
                        (ulParam == XONLINE_E_LOGON_SERVICE_NOT_AUTHORIZED) ||
                        (ulParam == XONLINE_E_LOGON_SERVICE_TEMPORARILY_UNAVAILABLE))
                    {
                        _NetConnUpdateConnStatus(pRef, '-svc');
                    }
                    else if ((ulParam == XONLINE_E_LOGON_UPDATE_REQUIRED) ||
                        (ulParam == XONLINE_E_LOGON_FLASH_UPDATE_REQUIRED))
                    {
                        _NetConnUpdateConnStatus(pRef, '-upd');
                    }
                    else if (ulParam == XONLINE_E_LOGON_SERVERS_TOO_BUSY)
                    {
                        _NetConnUpdateConnStatus(pRef, '-bsy');
                    }
                    else if (ulParam == XONLINE_E_LOGON_CONNECTION_LOST)
                    {
                        _NetConnUpdateConnStatus(pRef, '-lst');
                    }
                    else if (ulParam == XONLINE_E_LOGON_KICKED_BY_DUPLICATE_LOGON)
                    {
                        _NetConnUpdateConnStatus(pRef, '-dup');
                    }
                    else if (ulParam == XONLINE_E_LOGON_INVALID_USER)
                    {
                        _NetConnUpdateConnStatus(pRef, '-inv');
                    }
                    else if (ulParam == XONLINE_S_LOGON_DISCONNECTED)
                    {
                        _NetConnUpdateConnStatus(pRef, '-lof');
                    }
                    else
                    {
                        _NetConnUpdateConnStatus(pRef, '-xbl');
                    }

                    // disconnected from xbox live
                    pRef->bXblConnected = FALSE;

                    //we may have lost our connection to the network too, if so update bIspConnected
                    _NetConnUpdateIspStatus(pRef, pRef->iNetStatus);
                }
            }

            // poll for signin notifications
            if (XNotifyGetNext(pRef->hListener, XN_SYS_SIGNINCHANGED, &dwNotify, &ulParam) != 0)
            {
                #if DIRTYCODE_LOGGING
                NetPrintf(("netconnxenon: signin change->0x%02x\n", ulParam));
                #endif

                // update info for logged on users
                _NetConnGetLogonUsers(pRef);
            }

            // poll for profile change notifications
            if (XNotifyGetNext(pRef->hListener, XN_SYS_PROFILESETTINGCHANGED, &dwNotify, &ulParam) != 0)
            {
                #if DIRTYCODE_LOGGING
                NetPrintf(("netconnxenon: profile change->0x%02x\n", ulParam));
                #endif

                // update info for logged on users
                _NetConnGetLogonUsers(pRef);
            }

            // update user account info requests
            _NetConnUpdateAccountInfo(pRef);

            // update any outstanding QoS requests
            if (pRef->pXnQos != NULL)
            {
                // have we got any data?
                if (pRef->pXnQos->axnqosinfo[0].bFlags & (XNET_XNQOSINFO_DATA_RECEIVED|XNET_XNQOSINFO_COMPLETE))
                {
                    // update QoS info
                    pRef->iDnBitsPerSec = pRef->pXnQos->axnqosinfo[0].dwDnBitsPerSec;
                    pRef->iUpBitsPerSec = pRef->pXnQos->axnqosinfo[0].dwUpBitsPerSec;
                    pRef->iMedRttTime = pRef->pXnQos->axnqosinfo[0].wRttMedInMsecs;

                    NetPrintf(("netconnxenon: Qos update (up=%d, dn=%d, mrtt=%d)\n",
                        pRef->iUpBitsPerSec/1024, pRef->iDnBitsPerSec/1024, pRef->iMedRttTime));
                }

                // is the QoS request complete?
                if (pRef->pXnQos->axnqosinfo[0].bFlags & XNET_XNQOSINFO_COMPLETE)
                {
                    XNetQosRelease(pRef->pXnQos);
                    pRef->pXnQos = NULL;
                }
            }
            else if (pRef->bInitiateQos == TRUE)
            {
                // if requested, issue a QoS probe
                iResult = XNetQosServiceLookup(0, NULL, &pRef->pXnQos);
                NetPrintf(("netconnxenon: XNetQosServiceLookup() result=%d\n", iResult));
                pRef->bInitiateQos = FALSE;
            }
        }
        break;

        default:
        break;
    }
}


/*** Public functions *****************************************************************************/


/*F********************************************************************************/
/*!
    \Function NetConnStartup

    \Description
        Bring the network connection module to life.

    \Input  *pParams    - startup parameters

    \Output
        int32_t         - negative=error, zero=success

    \Notes
        pParams can contain the following terms:

        \verbatim
            -nosecure                   : bypass security do not use gateway, allow DNS
            -maxkey=                    : specify maximum key registrations allowed
            -mixedsecure                : bypass security, use gateway, no DNS
            -servicename                : set servicename <game-year-platform> required for SSL use
            -syslink                    : start up in syslink mode
            -threadstacksize=<bytecnt>  : size of stack (in bytes) used at thread creation (default=0)
            -xhttp                      : startup xhttp and xauth
            -xhttp_nosecure             : enable xhttp secure bypass
        \endverbatim

    \Version 02/23/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnStartup(const char *pParams)
{
    NetConnRefT *pRef = _NetConn_pRef;
    unsigned char bSyslink = FALSE, bInsecure = FALSE, bUseXLSP = TRUE, bRedirect = TRUE;
    uint32_t uSecureSetting = 2; //Set to "secure" by default
    int32_t iKeyRegMax = 8;
    const char *pTemp;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allow NULL params
    if (pParams == NULL)
    {
        pParams = "";
    }

    // debug display of input params
    NetPrintf(("netconnxenon: startup params='%s'\n", pParams));

    // syslink mode?
    if (strstr(pParams, "-syslink") != NULL)
    {
        NetPrintf(("netconnxenon: starting up in syslink mode\n"));
        bSyslink = TRUE;
    }

    // insecure mode?
    if (strstr(pParams, "-nosecure") != NULL)
    {
        bInsecure = TRUE;
        bUseXLSP = FALSE;
        uSecureSetting = 0;
    }
    // mixed mode?
    else if (strstr(pParams, "-mixedsecure") != NULL)
    {
        bInsecure = TRUE;
        bRedirect = FALSE;
        uSecureSetting = 1;
    }

    // set max key registrations?
    if ((pTemp = strstr(pParams, "-maxkey=")) != NULL)
    {
        iKeyRegMax = strtol(pTemp + 8, NULL, 10);
        NetPrintf(("netconnxenon: setting maxkey=%d\n", iKeyRegMax));
    }

    // error see if already started
    if (pRef != NULL)
    {
        NetPrintf(("netconnxenon: NetConnStartup() called while module is already active\n"));
        return(-1);
    }

    // alloc and init ref
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), NETCONN_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconnxenon: unable to allocate module state\n"));
        return(-2);
    }
    memset(pRef, 0, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;

    // parse and handle input parameters
    _NetConnParseParams(pRef, pParams);

    // save configuration state
    pRef->bUseXLSP = bUseXLSP;
    pRef->bRedirect = bRedirect;
    pRef->bSyslink = bSyslink;
    pRef->uSecureSetting = uSecureSetting;

    // start up Xbox networking
    if (_NetConnXnetInit(pRef, iKeyRegMax, bInsecure) != S_OK)
    {
        DirtyMemFree(pRef, NETCONN_MEMID, iMemGroup, pMemGroupUserData);
        NetPrintf(("netconnxenon: unable to initialize network stack\n"));
        return(-3);
    }

    // start up dirtysock
    if (SocketCreate(THREAD_PRIORITY_HIGHEST, pRef->iThreadStackSize, 0) != 0)
    {
        XNetCleanup(); // cleanup for _NetConnXnetInit
        DirtyMemFree(pRef, NETCONN_MEMID, iMemGroup, pMemGroupUserData);
        NetPrintf(("netconnxenon: unable to start up dirtysock\n"));
        return(-4);
    }

    // start XOnline
    if (pRef->bSyslink == FALSE)
    {
        if (XOnlineStartup() !=  ERROR_SUCCESS)
        {
            SocketDestroy(0);
            XNetCleanup(); // cleanup for _NetConnXnetInit
            DirtyMemFree(pRef, NETCONN_MEMID, iMemGroup, pMemGroupUserData);
            NetPrintf(("netconnxenon: unable to start up XOnline\n"));
            return(-5);
        }
    }
    // the network has been fully initialized.
    // from now on we can use NetConnShutdown to do cleanup upon failures.

    // save global module ref, NetConnShutdown will use & free _NetConn_pRef
    _NetConn_pRef = pRef;

    // set initial state to starting up
    pRef->eState = ST_STAR;

    // set initial memunit state to invalid
    pRef->iXbMemUnitState = -1;

    // set initial number of services
    pRef->uNumServices = 1;

    // initialize variable used to track which controller has a pending invite accept notification
    pRef->ulInvitePendingCtrl = NETCONN_INVALID_CONTROLLER_INDEX;

    // add netconn task handle
    if (NetConnIdleAdd(_NetConnUpdate, pRef) < 0)
    {
        NetPrintf(("netconnxenon: unable to add netconn task handler\n"));
        NetConnShutdown(0);
        return(-6);
    }

    // create listener for live events
    if ((pRef->hListener = XNotifyCreateListener(XNOTIFY_ALL)) == NULL)
    {
        NetPrintf(("netconnxenon: unable to create live event listener\n"));
        NetConnShutdown(0);
        return(-7);
    }

    // allocate external cleanup list
    pRef->iExternalCleanupListMax = NETCONN_EXTERNAL_CLEANUP_LIST_INITIAL_CAPACITY;
    if ((pRef->pExternalCleanupList = DirtyMemAlloc(pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT), NETCONN_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconnxenon: unable to allocate memory for initial external cleanup list\n"));
        NetConnShutdown(0);
        return(-8);
    }
    memset(pRef->pExternalCleanupList, 0, pRef->iExternalCleanupListMax * sizeof(NetConnExternalCleanupEntryT));

    // create friend api module
    if (pRef->bUseFriendModule == TRUE)
    {
        if ((pRef->pFriendApiRef = FriendApiCreate()) == NULL)
        {
            NetPrintf(("netconnxenon: unable to create friend api module\n"));
            NetConnShutdown(0);
            return(-9);
        }
    }

    pRef->uThreadId = GetCurrentThreadId();

    // create dirtylsp ref used for gos-core connection
    if ((pRef->pDirtyLSP = DirtyLSPCreate(100)) == NULL)
    {
        NetPrintf(("netconnxenon: error creating DirtyLSP\n"));
        NetConnShutdown(0);
        return(-10);
    }

    // startup XHttp (and XAuth) if requested
    if (strstr(pParams, "-xhttp"))
    {
        pRef->bUseXhttp = TRUE;
        if (strstr(pParams, "-xhttp_nosecure"))
        {
            pRef->bXhttpNoSecure = TRUE;
        }
        #ifdef DIRTYSDK_XHTTP_ENABLED
        // start xhttp (must come before xauth)
        if (!XHttpStartup(0, NULL))
        {
            NetPrintf(("netconnxenon: error, could not start XHttp\n"));
            NetConnShutdown(0);
            return(-11);
        }
        pRef->bXHttpStarted = TRUE;
        #endif
    }

    // create and configure dirtycert
    if (NetConnDirtyCertCreate(pParams))
    {
        NetConnShutdown(0);
        return(-12);
    }

    // start up protossl
    if (ProtoSSLStartup() < 0)
    {
        NetConnShutdown(0);
        NetPrintf(("netconnxenon: unable to startup protossl\n"));
        return(-13);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetConnShutdown

    \Description
        Shutdown the network code and return to idle state.

    \Input  uShutdownFlags  - shutdown configuration flags

    \Output
        int32_t             - 0 if success, -1 try again, other negative values: error

    \Notes
        If launching to the dashboard/account management,
        pass NETCONN_SHUTDOWN_NETACTIVE in uShutdownFlags.

    \Version 02/23/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnShutdown(uint32_t uShutdownFlags)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // make sure we've been started
    if (pRef == NULL)
    {
        NetPrintf(("netconnxenon: NetConnShutdown() called while module is not active\n"));
        return(NETCONN_ERROR_NOTACTIVE);
    }

    // if we're not leaving the network active, make sure we're disconnected
    if ((uShutdownFlags & NETCONN_SHUTDOWN_NETACTIVE) == 0)
    {
        //$$ TODO -- review this -- should probably be unconditional, not dependent on NETCONN_SHUTDOWN_NETACTIVE
        NetConnDisconnect();
    }

    // destroy the friend api ref
    if (pRef->pFriendApiRef)
    {
        FriendApiDestroy(pRef->pFriendApiRef);
        pRef->pFriendApiRef = NULL;
    }

    // try to empty external cleanup list
    _NetConnProcessExternalCleanupList(pRef);

    // make sure external cleanup list is empty before proceeding
    if (pRef->iExternalCleanupListCnt != 0)
    {
#if DIRTYCODE_LOGGING
        static uint32_t uLastTime = 0;
        if (!uLastTime || NetTickDiff(NetTick(), uLastTime) > 1000)
        {
            NetPrintf(("netconnxenon: deferring shutdown while external cleanup list is not empty (%d items)\n", _NetConn_pRef->iExternalCleanupListCnt));
            uLastTime = NetTick();
        }
#endif

        // signal "try again"
        NetPrintf(("netconnxenon: deferring shutdown while external cleanup list is not empty (%d items)\n", pRef->iExternalCleanupListCnt));
        return(NETCONN_ERROR_ISACTIVE);
    }

    // make sure gamertag lookup by xuid has completed before proceeding
    if (pRef->uLookUpInProgress)
    {
        if (!XHasOverlappedIoCompleted(&pRef->XLookUpOverlapped))
        {
            return(NETCONN_ERROR_ISACTIVE);
        }
        pRef->uLookUpInProgress = FALSE;
    }

    NetPrintf(("netconnxenon: shutting down...\n"));
    
    // shut down xhttp
    #ifdef DIRTYSDK_XHTTP_ENABLED
    if (pRef->bXHttpStarted)
    {
        XHttpShutdown();
    }
    #endif
    
    // dispose of DirtyLSP
    if (pRef->pDirtyLSP != NULL)
    {
        DirtyLSPDestroy(pRef->pDirtyLSP);
        pRef->pDirtyLSP = NULL;
    }

    // destroy listener for live events
    if (pRef->hListener != NULL)
    {
        CloseHandle(pRef->hListener);
        pRef->hListener = NULL;
    }

    // shut down protossl
    ProtoSSLShutdown();

    // destroy the dirtycert module
    DirtyCertDestroy();

    // remove netconn idle task
    NetConnIdleDel(_NetConnUpdate, pRef);

    // shut down idle handler
    NetConnIdleShutdown();

    // if we're not leaving the network active, take it down
    if ((uShutdownFlags & NETCONN_SHUTDOWN_NETACTIVE) == 0)
    {
        // clean up online stuff, only do the cleanup when we aren't using syslink
        if (pRef->bSyslink == FALSE)
        {
            XOnlineCleanup();
        }

        // shut down XNet
        XNetCleanup();

        // shut down dirtysock
        SocketDestroy(0);
    }

    // free external cleanup list
    if (pRef->pExternalCleanupList != NULL)
    {
        DirtyMemFree(pRef->pExternalCleanupList, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        pRef->pExternalCleanupList = NULL;
    }

    if (pRef->pFindUserResponse)
    {
        DirtyMemFree(pRef->pFindUserResponse, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        pRef->pFindUserResponse = NULL;
    }

    // dispose of ref
    DirtyMemFree(pRef, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    // null global reference
    _NetConn_pRef = NULL;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetConnQuery

    \Description
        Returns the index of the given gamertag.

    \Input *pGamertag   - gamer tag
    \Input *pList       - XUID to get index of
    \Input iListSize    - unused

    \Output
        int32_t         - negative=gamertag not logged in, else index of user

    \Notes
        The gamertags are returned in the strDev field of the NetConfigRecT, and
        the dwUserOptions field is made available in the iType field.

    \Version 02/23/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnQuery(const char *pGamertag, NetConfigRecT *pList, int32_t iListSize)
{
    NetConnRefT *pRef = _NetConn_pRef;
    int32_t iUser;

    // make sure everything is up to date
    _NetConnUpdate(pRef, NetTick());

    // find user based on gamertag?
    if (pGamertag != NULL)
    {
        for (iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
        {
            if (!DirtyUsernameCompare(pGamertag, pRef->aUsers[iUser].strGamertag))
            {
                return(iUser);
            }
        }
    }
    else if (pList != NULL)
    {
        // find user based on XUID
        for (iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
        {
            XUID *pXuid = &pRef->aUsers[iUser].Xuid;
            if (!memcmp(pList, pXuid, sizeof(*pXuid)) && (*pXuid != 0))
            {
                return(iUser);
            }
        }
    }
    else
    {
        // find first signed-in user (XUID/pName are not specified)
        for (iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
        {
            if (!IsEqualXUID(pRef->aUsers[iUser].Xuid, INVALID_XUID) && (XUserGetSigninState(iUser) == eXUserSigninState_SignedInToLive))
            {
                return(iUser);
            }
        }
    }

    // could not resolve user index
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function NetConnConnect

    \Description
        Initiates a connection to Xbox Live.  The ISP connection should already be
        established previous to calling this function.

        Uses the configuration returned by NetConnQuery.

    \Input *pConfig - config entry to use
    \Input *pParms  - "silent=true/false" to specify to use xbl using login flow.
                      "-xlspsitename=<sitename>" to identify XLSP to be used in secure mode
                      "-guest=true/false" to specify to always show/hide the guest option on the signin dialog

    \Input iData    - platform-specific

    \Output
        int32_t     - negative=error, zero=success

    \Version 02/23/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnConnect(const NetConfigRecT *pConfig, const char *pParms, int32_t iData)
{
    NetConnRefT *pRef = _NetConn_pRef;

    pRef->bSilent = TRUE;
    pRef->eGuestOption = GO_DEFAULT;

    if ((pRef->eState == ST_STAR) || (pRef->eState == ST_EXT_CLEANUP))
    {
        // clear connection status
        pRef->uConnStatus = 0;
        pRef->bSGConnected = FALSE;

        // clear QoS measurements & set to request QoS measurement
        pRef->iDnBitsPerSec = pRef->iUpBitsPerSec = pRef->iMedRttTime -1;
        pRef->bInitiateQos = TRUE;

        if (pParms != NULL)
        {
            // find out if we're doing a silent login, or if we're going to bring up the signin UI if needed
            if (strstr(pParms, "silent=false") != NULL)
            {
                pRef->bSilent = FALSE;
            }

            // find out whether the guest option should be displayed
            if (strstr(pParms, "-guest=true") != NULL)
            {
                pRef->eGuestOption = GO_ENABLE;
            }
            else
            {
                pRef->eGuestOption = GO_DISALLOW;
            }
        }

        pRef->bDoConnect = TRUE;  //connection is started in ST_STAR
        _NetConnExtractSitename(pRef, pParms);

        #if DIRTYCODE_LOGGING
        // if needed, delay NetConnConnect until external cleanup phase completes
        if (pRef->eState == ST_EXT_CLEANUP)
        {
            NetPrintf(("netconnxenon: delaying completion of NetConnConnect() until external cleanup phase is completed\n"));
        }
        #endif
        }
    else
    {
        NetPrintf(("netconnxenon: NetConnConnect() ignored because already connected!\n"));
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetConnDisconnect

    \Description
        Used to bring down the network connection.  After calling this, it would
        be necessary to call NetConnConnect to bring the connection back up or
        NetConnShutdown to completely shutdown all network support.

    \Output
        int32_t             - negative=error, zero=success

    \Notes
        The game code using NetConnXenon needs to guarantee that
        DirtySessionManagerDestroy() and DirtyLSPDestroy() calls have been invoked
        on all gamecode-owned DirtySessionManagers/DirtyLSP objects before calling
        NetConnDisconnect().  Failure to do so may lead to erroneous behavior on
        next call to NetConnConnect().  This is in general a good procedure anyway
        but is particularly important for these two modules.

    \Version 02/23/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnDisconnect(void)
{
    NetConnRefT *pRef = _NetConn_pRef;
    int32_t iUser;

    NetPrintf(("netconnxenon: disconnecting...\n"));

    // pass disconnect message to socket module
    SocketControl(NULL, 'disc', 0, NULL, NULL);

    // dispose of QoS probe if it is still outstanding
    if (pRef->pXnQos != NULL)
    {
        XNetQosRelease(pRef->pXnQos);
        pRef->pXnQos = NULL;
    }

    // cancel user info query if one is ongoing
    if (pRef->bUserInfoQuery == TRUE)
    {
        XCancelOverlapped(&pRef->XOverlapped);
        pRef->bUserInfoQuery = FALSE;
    }

    // clean up user info
    for (iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
    {
        if (pRef->aUsers[iUser].pUserInfo != NULL)
        {
            DirtyMemFree(pRef->aUsers[iUser].pUserInfo, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        }
        memset(&pRef->aUsers[iUser], 0, sizeof(pRef->aUsers[0]));
    }

    // unregister key
    if (pRef->bSyslink == TRUE)
    {
        XNetUnregisterKey(&pRef->XnKid);
    }

    // clear connection and memunit status
    pRef->uConnStatus = 0;
    pRef->iXbMemUnitState = -1;

    // shut down xhttp/xauth if started
    if (pRef->pDirtyAuth != NULL)
    {
        DirtyAuthDestroy(pRef->pDirtyAuth);
        pRef->pDirtyAuth = NULL;
    }

    /*
    transit to ST_EXT_CLEANUP state
        Upon next call to NetConnConnect(), the NetConn module will be stuck in
        ST_EXT_CLEANUP state until all pending external module destructions
        are completed.
    */
    pRef->eState = ST_EXT_CLEANUP;

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
            'aurn' - stands for "Authenticaiton URN"; sets the URN to be used when acquiring an XSTS token
            'recu' - stands for "register for external cleanup"; add a new entry to the external cleanup list
            'show' - set the function pointer to show the singin UI
            'snam' - set DirtyCert service name
            'tick' - initiate a new XSTS token acquisition for the user index specified in iValue; <0=error, 0=retry (a request is already pending), >0=request issued
            'xblu' - stands for "xbl update"; force an immediate update of the different users' signin state
            'xtag' - retrieve gamertag from xuid. pValue: XUID, pValue2: user-provided pData
            'xtcb' - set pointer(pValue) to callback for retrieving gamertag from XUID
        \endverbatim

        Unhandled selectors are passed through to SocketControl()

    \Version 1.0 02/23/2004 (jbrookes) first version
*/
/********************************************************************************F*/
int32_t NetConnControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue, void *pValue2)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // make sure module is started before allowing any other control calls
    if (pRef == NULL)
    {
        NetPrintf(("netconnxenon: warning - calling NetConnControl() while module is not initialized\n"));
        return(-1);
    }

    // sets the URN
    if (iControl == 'aurn')
    {
        ds_strnzcpy(pRef->strXAuthUrn, pValue, sizeof(pRef->strXAuthUrn));
        NetPrintf(("netconnxenon: authentication URN set to %s\n", pRef->strXAuthUrn));
        return(0);
    }

    // register for external cleanup
    if (iControl == 'recu')
    {
        return(_NetConnAddToExternalCleanupList(pRef, (NetConnExternalCleanupCallbackT)pValue, pValue2));
    }

    // set the show signin UI func, and return the old one
    if (iControl == 'show')
    {
        NetConnXShowSigninUIT pOld = pRef->pShowSigninUI;
        pRef->pShowSigninUI = (NetConnXShowSigninUIT)pValue;
        NetPrintf(("netconnxenon: changing XShowSigninUI from (0x%08x) to (0x%08x)\n", pOld, pRef->pShowSigninUI));
        return((int32_t)pOld);
    }

    // set dirtycert service name
    if (iControl == 'snam')
    {
        return(DirtyCertControl('snam', 0, 0, pValue));
    }

    // request refresh of XSTS token
    if (iControl == 'tick')
    {
#ifdef DIRTYSDK_XHTTP_ENABLED
        int32_t iUserIndex = iValue;
        int32_t iRetCode = 0;  // default to "try again"

        if (iUserIndex < NETCONN_MAXLOCALUSERS)
        {
            NetPrintf(("netconnxenon: NetConnControl 'tick' for user index %d  (URN = %s)\n", iUserIndex, pRef->strXAuthUrn));

            switch (pRef->aUsers[iUserIndex].token.eTokenStatus)
            {
                // token acquisition is currently in progress
                case ST_TOKEN_INPROG:
                    break;

                default:
                    _NetConnRequestAuthToken(pRef, iUserIndex);
                    iRetCode = 1;  // signal success
                    break;
            }
        }
        else
        {
            NetPrintf(("netconnxenon: NetConnControl 'tick' for invalid user index %d\n", iUserIndex));
            iRetCode = -1;  // signal failure
        }

        return(iRetCode);
#else
        return(-1);
#endif
    }

    // force an immediate update of the different users' signin state
    if (iControl == 'xblu')
    {
        _NetConnGetLogonUsers(pRef);
        return(0);
    }

    // retrieve gamertag from xuid
    if (iControl == 'xtag')
    {
        uint32_t uRet,uSize;

        if (pRef->uLookUpInProgress)
        {
            NetPrintf(("netconnxenon: Already one lookup in progress\n"));
            return(-1);
        }

        memset(&pRef->XLookUpOverlapped, 0, sizeof(pRef->XLookUpOverlapped));
        memset(&pRef->FindUserInfo, 0, sizeof(pRef->FindUserInfo));

        uSize = XUserFindUsersResponseSize(1);

        if (uSize > pRef->uFindUserResponseSize)
        {
            if (pRef->uFindUserResponseSize)
            {
                DirtyMemFree(pRef->pFindUserResponse, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
            }

            pRef->pFindUserResponse = DirtyMemAlloc(uSize, NETCONN_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
            pRef->uFindUserResponseSize = uSize;
        }

        pRef->FindUserInfo.qwUserId = *((XUID*)pValue);
        uRet = XUserFindUsers(pRef->FindUserInfo.qwUserId, 1, &pRef->FindUserInfo, uSize, pRef->pFindUserResponse, &pRef->XLookUpOverlapped);
        pRef->pUserLookedUpData = pValue2;

        if (uRet != ERROR_IO_PENDING)
        {
            NetPrintf(("netconnxenon: Error %d/(%s) getting gamertag from XUID\n", uRet, DirtyErrGetName(uRet)));
            uRet = XGetOverlappedExtendedError(&pRef->XLookUpOverlapped);
            NetPrintf(("netconnxenon: Extended Error %d/(%s)\n", uRet, DirtyErrGetName(uRet)));
            return(-1);
        }

        pRef->uLookUpInProgress = TRUE;
        memset(&pRef->UserLookedUp, 0, sizeof(pRef->UserLookedUp));

        return(0);
    }

    // register callback for retrieving gamertag from XUID
    if (iControl == 'xtcb')
    {
        pRef->UserLookUpCallback = (UserInfoCallbackT*)pValue;
        return(0);
    }

    // pass through unhandled selectors to SocketControl()
    return(SocketControl(NULL, iControl, iValue, pValue, pValue2));
}

/*F********************************************************************************/
/*!
    \Function NetConnStatus

    \Description
        Check general network connection status. Different selectors return
        different status attributes.

    \Input iKind    - input selector
    \Input iData    - (optional) selector specific
    \Input *pBuf    - (optional) pointer to output buffer
    \Input iBufSize - (optional) size of output buffer

    \Output
        int32_t     - selector result

    \Notes
        iKind can be one of the following:

        \verbatim
            'addr' - ip address of client
            'bbnd' - returns TRUE
            'bpsd' - returns bits per second down, as measured by QoS at login
            'bpsu' - returns bits per second up, as measured by QoS at login
            'conn'/'star' - connection status: +onl=online, ~<code>=in progress, -<err>=NETCONN_ERROR_*
            'ethr' - mac address of client (returned in pBuf), 0=success, negative=error
            'fref' - returns the friend api module reference
            'gate' - returns TRUE if connected to SG, else FALSE
            'gtag' - zero=gamertag returned in pBuf, negative=error
            'host' - returns security gateway address, or zero
            'invt' - positive=controller index (0-3), invite session info copied into pBuf; negative=no invite accept notification available, or error
            'locl' - returns "local" locality (set in the dashboard)
            'locu' - returns locality for the specified logged-in user account; user data must be previously acquired using 'xnfo' selector (ploc=platform native locality)
            'macx' - mac address of client (returned in pBuf), 0=success, negative=error
            'mask' - returns the mask of requested user that are logged in.
            'mgrp' - returns memgroup id, and copies memgroup user pointer to pBuf if pBuf is not NULL
            'mrtt' - returns median round trip time, as measured by QoS at login
            'open' - true/false indication of whether network code running
            'onln' - true/false indication of whether we are connected to Xbox Live
            'ploc' - returns platform-specific locality for the specified logged-in user account; user data must be previously acquired using 'xnfo' selector
            'plug' - true/false indication of ethernet link status
            'secu' - true/false indication whether default security gateway is available
            'sset' - returns the XBL security setting - 0 for "unsecure", 1 for "mixedsecure", 2 for "secure"
            'slnk' - true/false indication if we are in syslink mode or not
            'tick' - fills pBuf with XSTS Token if available. return value: 0:"try again"; <0:error; >0:number of bytes copied into pBuf
            'tsta' - reutrn DirtyAuth Token Errors in pBuf where pBuf is a byte array. (pBuf Must be aleast the size of DWORD). Return value is positive if success, negative if failed.
            'type' - connection type: NETCONN_IFTYPE_* bitfield
            'vers' - return DirtySDK version
            'xadd' - zero=xnaddr of machine returned in pBuf, negative=error
            'xkey' - zero=xnkey returned in pBuf, negative=error (system link only)
            'xkid' - zero=xnkid returned in pBuf, negative=error (system link only)
            'xlsp' - returns title server service id
            'xnfo' - zero=user info pointer returned in pBuf, negative=error
            'xuid' - zero=xuid of client returned in pBuf, negative=error
        \endverbatim

    \Version 02/23/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetConnStatus(int32_t iKind, int32_t iData, void *pBuf, int32_t iBufSize)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // see if connected to Xbox Live
    if (iKind == 'onln')
    {
        return((pRef->bXblConnected == TRUE) && (pRef->eState == ST_IDLE));
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
        NetPrintf(("netconnxenon: warning - calling NetConnStatus() while module is not initialized\n"));
        return(-1);
    }

    // returns friend api ref
    //$$note - this is before the call to _NetConnUpdate() to prevent unwanted recursion
    if (iKind == 'fref')
    {
        if ((pBuf == NULL) || (iBufSize < (signed)sizeof(pRef->pFriendApiRef)))
        {
            NetPrintf(("netconnxenon: NetConnStatus('fref') failed!\n"));
            return(-1);
        }

        // copy ref pointer into user-provided buffer
        ds_memcpy(pBuf, &pRef->pFriendApiRef, sizeof(pRef->pFriendApiRef));
        return(0);
    }

    // return memory group and user data pointer
    //$$note - this is before the call to _NetConnUpdate() to prevent unwanted recursion
    if (iKind == 'mgrp')
    {
        if ((pBuf != NULL) && (iBufSize == sizeof(void *)))
        {
            *((void **)pBuf) = pRef->pMemGroupUserData;
        }
        return(pRef->iMemGroup);
    }

    // return physical connection status
    if (iKind == 'plug')
    {
        // check link status - takes ~2-3 milliseconds according to microsoft
        return((XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE) ? TRUE : FALSE);
    }

    // return titleserver service ID
    //$$note - this is before the call to _NetConnUpdate() to prevent unwanted recursion
    if (iKind == 'xlsp')
    {
        return(pRef->uTitleServerServiceId);
    }

    if (GetCurrentThreadId() == pRef->uThreadId)
    {
        // make sure everything is up to date
        _NetConnUpdate(pRef, NetTick());
    }

    // return client ip address
    if (iKind == 'addr')
    {
        return(SocketGetLocalAddr());
    }

    // return broadband
    if (iKind == 'bbnd')
    {
        return(TRUE);
    }

    // return measured bandwidth in bits per second
    if (iKind == 'bpsd')
    {
        return(pRef->iDnBitsPerSec);
    }
    if (iKind == 'bpsu')
    {
        return(pRef->iUpBitsPerSec);
    }

    // return isp connection status
    if ((iKind == 'conn') || (iKind == 'star'))
    {
        return(pRef->uConnStatus);
    }

    // return gateway connection status
    if (iKind == 'gate')
    {
        return(pRef->bSGConnected);
    }

    // return logged in gamertag
    if (iKind == 'gtag')
    {
        if (iData < 0 || iData >= NETCONN_MAXLOCALUSERS)
        {
            // index out of range
            return(-1);
        }

        if ((pBuf == NULL) || (iBufSize < sizeof(pRef->aUsers[iData].strGamertag)))
        {
            // invalid buffer or buffer too small
            return(-2);
        }

        if (pRef->aUsers[iData].Xuid == 0)
        {
            // no valid gamertag at the specified user index
            return(-3);
        }

        // fill user-provided buffer with gamertag
        ds_memcpy(pBuf, pRef->aUsers[iData].strGamertag, sizeof(pRef->aUsers[iData].strGamertag));

        return(0);
    }

    // return host address
    if (iKind == 'host')
    {
        if (pRef->bXblConnected && pRef->bUseXLSP)
        {
            return(SocketNtohl(pRef->uXlspAddress));
        }
        else
        {
            return(0);
        }
    }

    // return any pending invite info
    if (iKind == 'invt')
    {
        XINVITE_INFO XInviteInfo;
        int32_t iRetCode = -1;

        // is there an invite accept notification pending and does it match the controller requested by the caller?
        // iData contains the controller index specified by the user, can be -1 for "any controller"
        if ( (pRef->ulInvitePendingCtrl != NETCONN_INVALID_CONTROLLER_INDEX) &&
             (( iData == -1) || (iData == (int32_t)pRef->ulInvitePendingCtrl)) )
        {
            if (XInviteGetAcceptedInfo(pRef->ulInvitePendingCtrl, &XInviteInfo) == ERROR_SUCCESS)
            {
                NetPrintf(("netconnxenon: consumed invite accept notification on controller %d\n", pRef->ulInvitePendingCtrl));

                if (pBuf != NULL)
                {
                    ds_memcpy(pBuf, &XInviteInfo, iBufSize);
                }

                iRetCode = pRef->ulInvitePendingCtrl;
            }
            else
            {
                NetPrintf(("netconnxenon: XInviteGetAcceptedInfo() failed on controller %d\n", pRef->ulInvitePendingCtrl));
            }

            // now that the notification is consumed, reset the variable tracking the controller index on which it was pending
            pRef->ulInvitePendingCtrl = NETCONN_INVALID_CONTROLLER_INDEX;
        }

        return(iRetCode);
    }

    // return locality code created from dashboard language and locale
    if ((iKind == 'locl') || (iKind == 'locn'))
    {
        uint32_t uCountry, uLanguage;
        uLanguage = _NetConnLocaleTranslate(_NetConn_XnLanguageMap, (uint16_t)XGetLanguage(), LOBBYAPI_LANGUAGE_UNKNOWN);
        // get the country code
        uCountry = _NetConnLocaleTranslate(_NetConn_XnLocaleMap, (uint16_t)XGetLocale(), LOBBYAPI_COUNTRY_UNKNOWN);
        // convert to localizertoken and return
        return(LOBBYAPI_LocalizerTokenCreate(uLanguage, uCountry));
    }

    // return locality code; with locale sourced from user info instead of dashboard
    if ((iKind == 'locu') || (iKind == 'ploc'))
    {
        GET_USER_INFO_RESPONSE *pUserInfo;
        uint16_t uLanguage, uCountry;
        uint32_t uLocale;

        // if no info, return unknown
        if ((pUserInfo = _NetConnGetUserAccountInfo(pRef, iData)) == NULL)
        {
            NetPrintf(("netconnxenon: user info for index %d not available\n", iData));
            return(LOBBYAPI_LOCALITY_UNKNOWN);
        }

        // create locale from user account language and user account country identifier
        if (iKind == 'locu')
        {
            // get the language code
            uLanguage = _NetConnLocaleTranslate(_NetConn_XnLanguageMap, pUserInfo->wLanguageId, LOBBYAPI_LANGUAGE_UNKNOWN);
            // get the country code
            uCountry = _NetConnLocaleTranslate(_NetConn_XnCountryMap, pUserInfo->bCountryId, LOBBYAPI_COUNTRY_UNKNOWN);
            // convert to localizertoken and return
            uLocale = LOBBYAPI_LocalizerTokenCreate(uLanguage, uCountry);
        }
        else
        {
            uLocale = pUserInfo->bCountryId;
        }
        return(uLocale);
    }

    // return mac address
    if ((iKind == 'ethr') || (iKind == 'macx'))
    {
        if (pRef->bIspConnected == TRUE)
        {
            if ((pBuf != NULL) && (iBufSize >= sizeof(pRef->XnAddr.abEnet)))
            {
                ds_memcpy(pBuf, pRef->XnAddr.abEnet, sizeof(pRef->XnAddr.abEnet));
                return(0);
            }
        }

        return(-1);
    }

    if (iKind == 'mask')
    {
        return(_NetConnXblUserMask());
    }

    // return median round-trip time
    if (iKind == 'mrtt')
    {
        return(pRef->iMedRttTime);
    }

    // return default security gateway status (TRUE=available, FALSE=not available)
    if (iKind == 'secu')
    {
        return(pRef->bUseXLSP);
    }

    // return the security setting (0 for unsecure, 1 for mixed secure, 2 for secure)
    if (iKind == 'sset')
    {
        return(pRef->uSecureSetting);
    }

    // return if we are in syslink mode or not
    if (iKind == 'slnk')
    {
        return(pRef->bSyslink);
    }

    // return ticket info (if available)
    if (iKind == 'tick')
    {
#ifdef DIRTYSDK_XHTTP_ENABLED
        uint8_t bNeedRefresh = FALSE;   // default to "no need to refresh ticket"
        uint8_t bCacheValid = FALSE;    // default to "ticket cache content is not valid"
        int32_t iResult = 0;            // default to "try again"
        int32_t iUserIndex = iData;

        if (iUserIndex >= NETCONN_MAXLOCALUSERS)
        {
            NetPrintf(("netconnxenon: NetConnStatus 'tick' for invalid user index %d\n", iUserIndex));
            return(-1);
        }

        NetPrintf(("netconnxenon: NetConnStatus 'tick' for user index %d  (URN = %s)\n", iUserIndex, pRef->strXAuthUrn));

        switch (pRef->aUsers[iUserIndex].token.eTokenStatus)
        {
            // token has not been requested yet
            case ST_TOKEN_INVALID:
                bNeedRefresh = TRUE;
                break;

            // token aquisition failed
            case ST_TOKEN_FAILED:
                if (pRef->aUsers[iUserIndex].token.bFailureReported == TRUE)
                {
                    // attempt a new token request
                    pRef->aUsers[iUserIndex].token.bFailureReported = FALSE;
                    pRef->aUsers[iUserIndex].token.eTokenStatus = ST_TOKEN_INVALID;
                    bNeedRefresh = TRUE;
                }
                else
                {
                    // report the error to the xsts token server
                    pRef->aUsers[iUserIndex].token.bFailureReported = TRUE;
                    iResult = -1;
                }
                break;

            // token acquisition is currently in progress
            case ST_TOKEN_INPROG:
                break;

            // token is valid
            case ST_TOKEN_VALID:
                if (NetTickDiff(NetTick(), pRef->aUsers[iUserIndex].token.iTokenAcquTick) > NETCONNXENON_TOKEN_TIMEOUT)
                {
                    NetPrintf(("netconnxenon: ticket cache for user %d expired, refreshing now...\n", iUserIndex));
                    bNeedRefresh = TRUE;
                }
                else
                {
                    bCacheValid = TRUE;
                }
                break;

            default:
                NetPrintf(("netconnxenon: critical error - invalid token status \n"));
                iResult = -1;
                break;
        }

        // return cached ticket to caller if possible
        if (bCacheValid)
        {
            int32_t iTokenLen;

            // retrieve cached token length
            DirtyAuthCheckToken(pRef->strXAuthUrn, &iTokenLen, NULL);

            // add one to the token to guarantee null termination
            iResult = iTokenLen + 1;

            if (pBuf)
            {
                char *pTokenBuf = (char *)pBuf;

                if (iBufSize >= (iTokenLen + 1))
                {
                    // retrieve cached token
                    DirtyAuthCheckToken(pRef->strXAuthUrn, NULL, (uint8_t *)pTokenBuf);

                    // null terminate
                    pTokenBuf[iTokenLen] = '\0';

                    #if DIRTYCODE_LOGGING
                    XmlPrintFmt(((const char *)pTokenBuf, ""));
                    #endif
                }
                else
                {
                    NetPrintf(("netconnxenon: user-provided buffer is too small (%d/%d)\n", iBufSize, (iTokenLen + 1)));
                    iResult = -1;
                }
            }
        }

        // initiate a ticket cache refresh if needed
        if (bNeedRefresh)
        {
            _NetConnRequestAuthToken(pRef, iUserIndex);
        }

        return(iResult);
#else
        return(-1);
#endif
    }

    //Return DirtyAuth Status
    if (iKind == 'tsta')
    {
        int32_t iReturn = -1;

#ifdef DIRTYSDK_XHTTP_ENABLED

        if (pBuf != NULL)
        {
            iReturn = DirtyAuthStatus(pRef->pDirtyAuth, 'stat', 0, pBuf, iBufSize, pRef->strXAuthUrn);
        }
#endif 
       return iReturn;

    }

    // return connection type
    if (iKind == 'type')
    {
        int32_t iReturn = -1;

        if (pRef->bIspConnected == TRUE)
        {
            iReturn = NETCONN_IFTYPE_ETHER;
            if (pRef->iNetStatus & XNET_GET_XNADDR_PPPOE)
            {
                iReturn |= NETCONN_IFTYPE_PPPOE;
            }
        }

        return(iReturn);
    }

    // return xnaddr
    if (iKind == 'xadd')
    {
        if ((pBuf != NULL) && (iBufSize >= sizeof(pRef->XnAddr)))
        {
            ds_memcpy(pBuf, &pRef->XnAddr, sizeof(pRef->XnAddr));
            return(0);
        }

        return(-1);
    }

    // return user info
    if (iKind == 'xnfo')
    {
        NetConnUserT *pUser = &pRef->aUsers[iData];
        int32_t iResult = -1;

        // request complete?
        if ((pUser->eInfoState == ST_USERINFO_DONE) && (pBuf != NULL) && (pUser->pUserInfo != NULL) && (iBufSize == sizeof(pRef->aUsers[iData].pUserInfo)))
        {
            ds_memcpy(pBuf, &pRef->aUsers[iData].pUserInfo, sizeof(pRef->aUsers[iData].pUserInfo));
            iResult = 0;
        }
        else if (pUser->eInfoState == ST_USERINFO_IDLE) // initiate request?
        {
            pUser->eInfoState = ST_USERINFO_STRT;
            iResult = '~inp';
        }
        // processing
        else if (pUser->eInfoState == ST_USERINFO_PROC || pUser->eInfoState == ST_USERINFO_STRT)
        {
            iResult = '~inp';
        }

        return(iResult);
    }

    // return xnkey (system link only)
    if ((iKind == 'xkey') && (pRef->bSyslink == TRUE))
    {
        if ((pBuf != NULL) && (iBufSize >= sizeof(pRef->XnKey)))
        {
            ds_memcpy(pBuf, &pRef->XnKey, sizeof(pRef->XnKey));
            return(0);
        }

        return(-1);
    }

    // return xnkid (system link only)
    if ((iKind == 'xkid') && (pRef->bSyslink == TRUE))
    {
        if ((pBuf != NULL) && (iBufSize >= sizeof(pRef->XnKid)))
        {
            ds_memcpy(pBuf, &pRef->XnKid, sizeof(pRef->XnKid));
            return(0);
        }

        return(-1);
    }

    // return xuid
    if (iKind == 'xuid')
    {
        if ((pBuf != NULL) && (iBufSize >= sizeof(pRef->aUsers[iData].Xuid)))
        {
            ds_memcpy(pBuf, &pRef->aUsers[iData].Xuid, sizeof(pRef->aUsers[iData].Xuid));
            return(0);
        }

        return(-1);
    }
    // pass through unhandled selectors to SocketInfo()
    return(SocketInfo(NULL, iKind, iData, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function NetConnSleep

    \Description
        Sleep the application (yield thread) for some number of milliseconds.

    \Input  iMilliSecs  - number of milliseconds to se

    \Output
        uint32_t        - current microsecond counter value

    \Version 03/10/2001 (gschaefer)
*/
/********************************************************************************F*/
void NetConnSleep(int32_t iMilliSecs)
{
    Sleep(iMilliSecs);
}

/*H*************************************************************************************************

    \File    netconnwinrt.cpp

    \Description
        Provides network setup and teardown support for Windows8 and WindowsPhone8

    Copyright (c) 2012 Electronic Arts Inc.

    \Version 18/10/2012 (mclouatre) First version

*************************************************************************************************H*/

/*** Include files ********************************************************************************/
#include <windows.h>
#include <cstdlib>
#include <stdlib.h>          // for strtol()

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtycert.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtylang.h"      // for locality macros and definitions
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protoupnp.h"

#if defined(DIRTYCODE_WINRT_ONLY)
using namespace Windows::Globalization;
using namespace Windows::System::UserProfile;
#endif
using namespace Windows::Networking::Connectivity;

/*** Defines **************************************************************************************/

// GetGeoInfo returns XX for unknown country
#define NETCONNWIN_COUNTRY_UNKNOWN_STR  ("XX")

//! UPNP port
#define NETCONN_DEFAULT_UPNP_PORT       (3659)

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

//! private module state
typedef struct NetConnRefT
{
    // module memory group
    int32_t iMemGroup;          //!< module mem group id
    void *pMemGroupUserData;    //!< user data associated with mem group

    enum
    {
        ST_INIT,
        ST_CONN,
        ST_IDLE,
    } eState;

    int32_t iPeerPort;          //!< peer port to be opened by upnp; if zero, still find upnp router but don't open a port
    ProtoUpnpRefT *pProtoUpnp;  //!< protoupnp module state
} NetConnRefT;

//! language system-ds pair
typedef struct NetConnLangPairT
{
    const wchar_t * pSystemLang;   //!< pointer to system language string
    uint16_t        uDsLang;       //!< matching DS language code
} NetConnLangPairT;


/*** Function Prototypes **************************************************************************/

/*** Variables ************************************************************************************/

static NetConnRefT *_NetConn_pRef = NULL;        //!< module state pointer

//! mapping table to map WinRt language defines to DirtySock encodings
static NetConnLangPairT _NetConn_WinRtLanguageMap[203] =
{
    { L"af"   ,         LOBBYAPI_LANGUAGE_AFRIKAANS },
    { L"sq"   ,         LOBBYAPI_LANGUAGE_ALBANIAN },
    { L"am"   ,         LOBBYAPI_LANGUAGE_AMHARIC },
    { L"ar-DZ",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-BH",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-EG",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-IQ",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-JO",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-KW",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-LB",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-LY",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-MA",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-OM",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-QA",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-SA",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-SY",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-TN",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-AE",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"ar-YE",         LOBBYAPI_LANGUAGE_ARABIC },
    { L"hy",            LOBBYAPI_LANGUAGE_ARMENIAN },
    { L"as",            LOBBYAPI_LANGUAGE_ASSAMESE },
    { L"az-Cyrl",       LOBBYAPI_LANGUAGE_AZERBAIJANI },
    { L"az-Latn",       LOBBYAPI_LANGUAGE_AZERBAIJANI },
    { L"bn-BD",         LOBBYAPI_LANGUAGE_BENGALI },
    { L"bn-IN",         LOBBYAPI_LANGUAGE_BENGALI },
    { L"ba",            LOBBYAPI_LANGUAGE_BASHKIR },
    { L"eu",            LOBBYAPI_LANGUAGE_BASQUE },
    { L"bs-Cyrl",       LOBBYAPI_LANGUAGE_BOSNIAN },
    { L"bs-Latn",       LOBBYAPI_LANGUAGE_BOSNIAN },
    { L"br",            LOBBYAPI_LANGUAGE_BRETON },
    { L"bg",            LOBBYAPI_LANGUAGE_BULGARIAN },
    { L"km",            LOBBYAPI_LANGUAGE_CAMBODIAN }, // Khmer
    { L"ca",            LOBBYAPI_LANGUAGE_CATALAN },
    { L"ca-ES-valencia",LOBBYAPI_LANGUAGE_CATALAN }, // Valencian
    { L"zh-Hans-CN",    LOBBYAPI_LANGUAGE_CHINESE }, // Chinese, simplified, China
    { L"zh-Hans-SG",    LOBBYAPI_LANGUAGE_CHINESE }, // Chinese, simplified, Singapore
    { L"zh-Hant-HK",    LOBBYAPI_LANGUAGE_CHINESE }, // Chinese, traditional, Hong Kong
    { L"zh-Hant-MO",    LOBBYAPI_LANGUAGE_CHINESE }, // Chinese, traditional, Macao
    { L"zh-Hant-TW",    LOBBYAPI_LANGUAGE_CHINESE }, // Chinese, traditional, Taiwan
    { L"co",            LOBBYAPI_LANGUAGE_CORSICAN },
    { L"hr-BA",         LOBBYAPI_LANGUAGE_CROATIAN },
    { L"hr-HR",         LOBBYAPI_LANGUAGE_CROATIAN },
    { L"cs",            LOBBYAPI_LANGUAGE_CZECH },
    { L"da",            LOBBYAPI_LANGUAGE_DANISH },
    { L"dv",            LOBBYAPI_LANGUAGE_DIVEHI },
    { L"nl-BE",         LOBBYAPI_LANGUAGE_DUTCH },
    { L"nl-NL",         LOBBYAPI_LANGUAGE_DUTCH },
    { L"en-AU",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-BZ",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-CA",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-029",        LOBBYAPI_LANGUAGE_ENGLISH },  // English, Caribbean
    { L"en-IN",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-IE",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-JM",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-MY",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-NZ",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-PH",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-SG",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-ZA",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-TT",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-GB",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-US",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"en-ZW",         LOBBYAPI_LANGUAGE_ENGLISH },
    { L"et",            LOBBYAPI_LANGUAGE_ESTONIAN },
    { L"fo",            LOBBYAPI_LANGUAGE_FAEROESE },
    { L"fi",            LOBBYAPI_LANGUAGE_FINNISH },
    { L"fr-BE",         LOBBYAPI_LANGUAGE_FRENCH },
    { L"fr-CA",         LOBBYAPI_LANGUAGE_FRENCH },
    { L"fr-CH",         LOBBYAPI_LANGUAGE_FRENCH },
    { L"fr-FR",         LOBBYAPI_LANGUAGE_FRENCH },
    { L"fy",            LOBBYAPI_LANGUAGE_FRISIAN },
    { L"gl",            LOBBYAPI_LANGUAGE_GALICIAN },
    { L"ka",            LOBBYAPI_LANGUAGE_GEORGIAN },
    { L"de-AT",         LOBBYAPI_LANGUAGE_GERMAN },
    { L"de-DE",         LOBBYAPI_LANGUAGE_GERMAN },
    { L"de-LI",         LOBBYAPI_LANGUAGE_GERMAN },
    { L"de-LU",         LOBBYAPI_LANGUAGE_GERMAN },
    { L"de-CH",         LOBBYAPI_LANGUAGE_GERMAN },
    { L"el",            LOBBYAPI_LANGUAGE_GREEK },
    { L"kl",            LOBBYAPI_LANGUAGE_GREENLANDIC },
    { L"gu",            LOBBYAPI_LANGUAGE_GUJARATI },
    { L"ha",            LOBBYAPI_LANGUAGE_HAUSA },
    { L"he",            LOBBYAPI_LANGUAGE_HEBREW },
    { L"hi",            LOBBYAPI_LANGUAGE_HINDI },
    { L"hu",            LOBBYAPI_LANGUAGE_HUNGARIAN },
    { L"is",            LOBBYAPI_LANGUAGE_ICELANDIC },
    { L"ig",            LOBBYAPI_LANGUAGE_IGBO },
    { L"id",            LOBBYAPI_LANGUAGE_INDONESIAN },
    { L"iu-Cans",       LOBBYAPI_LANGUAGE_INUKTITUT },
    { L"iu-Latn",       LOBBYAPI_LANGUAGE_INUKTITUT },
    { L"ga",            LOBBYAPI_LANGUAGE_IRISH },
    { L"it-CH",         LOBBYAPI_LANGUAGE_ITALIAN },
    { L"it-IT",         LOBBYAPI_LANGUAGE_ITALIAN },
    { L"ja",            LOBBYAPI_LANGUAGE_JAPANESE },
    { L"kn",            LOBBYAPI_LANGUAGE_KANNADA },
    { L"kk",            LOBBYAPI_LANGUAGE_KAZAKH },
    { L"rw",            LOBBYAPI_LANGUAGE_KINYARWANDA },
    { L"ko",            LOBBYAPI_LANGUAGE_KOREAN },
    { L"ky",            LOBBYAPI_LANGUAGE_KIRGHIZ },
    { L"ku-Arab",       LOBBYAPI_LANGUAGE_KURDISH },
    { L"lo",            LOBBYAPI_LANGUAGE_LAOTHIAN },
    { L"lv",            LOBBYAPI_LANGUAGE_LATVIAN_LETTISH },
    { L"lt",            LOBBYAPI_LANGUAGE_LITHUANIAN },
    { L"lb",            LOBBYAPI_LANGUAGE_LUXEMBOURGISH },
    { L"mk",            LOBBYAPI_LANGUAGE_MACEDONIAN },
    { L"ms",            LOBBYAPI_LANGUAGE_MALAY },
    { L"ml",            LOBBYAPI_LANGUAGE_MALAYALAM },
    { L"mt",            LOBBYAPI_LANGUAGE_MALTESE },
    { L"mi",            LOBBYAPI_LANGUAGE_MAORI },
    { L"mr",            LOBBYAPI_LANGUAGE_MARATHI },
    { L"mn-Cyrl",       LOBBYAPI_LANGUAGE_MONGOLIAN },
    { L"mn-Mong",       LOBBYAPI_LANGUAGE_MONGOLIAN },
    { L"ne",            LOBBYAPI_LANGUAGE_NEPALI },
    { L"nb",            LOBBYAPI_LANGUAGE_NORWEGIAN_BOKMAL },
    { L"nn",            LOBBYAPI_LANGUAGE_NORWEGIAN_NYNORSK },
    { L"oc",            LOBBYAPI_LANGUAGE_OCCITAN },
    { L"or",            LOBBYAPI_LANGUAGE_ORIYA },
    { L"ps",            LOBBYAPI_LANGUAGE_PASHTO_PUSHTO },
    { L"fa",            LOBBYAPI_LANGUAGE_PERSIAN },
    { L"pl",            LOBBYAPI_LANGUAGE_POLISH },
    { L"pt-BR",         LOBBYAPI_LANGUAGE_PORTUGUESE },
    { L"pt-PT",         LOBBYAPI_LANGUAGE_PORTUGUESE },
    { L"pa-Arab",       LOBBYAPI_LANGUAGE_PUNJABI },
    { L"pa",            LOBBYAPI_LANGUAGE_PUNJABI },
    { L"qu-BO",         LOBBYAPI_LANGUAGE_QUECHUA },
    { L"qu-EC",         LOBBYAPI_LANGUAGE_QUECHUA },
    { L"qu-PE",         LOBBYAPI_LANGUAGE_QUECHUA },
    { L"rm",            LOBBYAPI_LANGUAGE_RHAETO_ROMANCE },
    { L"ro",            LOBBYAPI_LANGUAGE_ROMANIAN },
    { L"ru",            LOBBYAPI_LANGUAGE_RUSSIAN },
    { L"sa",            LOBBYAPI_LANGUAGE_SANSKRIT },
    { L"gd",            LOBBYAPI_LANGUAGE_SCOTS_GAELIC },
    { L"sr-Cyrl-BA",    LOBBYAPI_LANGUAGE_SERBIAN },
    { L"sr-Cyrl-ME",    LOBBYAPI_LANGUAGE_SERBIAN },
    { L"sr-Cyrl-RS",    LOBBYAPI_LANGUAGE_SERBIAN },
    { L"sr-Latn-BA",    LOBBYAPI_LANGUAGE_SERBIAN },
    { L"sr-Latn-ME",    LOBBYAPI_LANGUAGE_SERBIAN },
    { L"sr-Latn-RS",    LOBBYAPI_LANGUAGE_SERBIAN },
    { L"st",            LOBBYAPI_LANGUAGE_SESOTHO },
    { L"tn-BW",         LOBBYAPI_LANGUAGE_SETSWANA },
    { L"tn-ZA",         LOBBYAPI_LANGUAGE_SETSWANA },
    { L"sd",            LOBBYAPI_LANGUAGE_SINDHI },
    { L"si",            LOBBYAPI_LANGUAGE_SINGHALESE },
    { L"sk",            LOBBYAPI_LANGUAGE_SLOVAK },
    { L"sl",            LOBBYAPI_LANGUAGE_SLOVENIAN },
    { L"es-AR",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-BO",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-CL",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-CO",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-CR",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-DO",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-EC",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-SV",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-GT",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-HN",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-419",        LOBBYAPI_LANGUAGE_SPANISH },   // Latin America
    { L"es-MX",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-NI",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-PA",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-PY",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-PE",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-PR",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-ES",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-US",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-UY",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"es-VE",         LOBBYAPI_LANGUAGE_SPANISH },
    { L"sw",            LOBBYAPI_LANGUAGE_SWAHILI }, // Kiswahili
    { L"sv-FI",         LOBBYAPI_LANGUAGE_SWEDISH },
    { L"sv-SE",         LOBBYAPI_LANGUAGE_SWEDISH },
    { L"tg-Cyrl",       LOBBYAPI_LANGUAGE_TAJIK },
    { L"ta-IN",         LOBBYAPI_LANGUAGE_TAMIL },
    { L"ta-LK",         LOBBYAPI_LANGUAGE_TAMIL },
    { L"tt-Cyrl",       LOBBYAPI_LANGUAGE_TATAR },
    { L"te",            LOBBYAPI_LANGUAGE_TEGULU },
    { L"th",            LOBBYAPI_LANGUAGE_THAI },
    { L"bo",            LOBBYAPI_LANGUAGE_TIBETAN },
    { L"ti-ER",         LOBBYAPI_LANGUAGE_TIGRINYA },
    { L"ti-ET",         LOBBYAPI_LANGUAGE_TIGRINYA },
    { L"tr",            LOBBYAPI_LANGUAGE_TURKISH },
    { L"tk-Latn",       LOBBYAPI_LANGUAGE_TURKMEN },
    { L"ug-Arab",       LOBBYAPI_LANGUAGE_UIGHUR }, // Uyghur
    { L"uk",            LOBBYAPI_LANGUAGE_UKRAINIAN },
    { L"ur",            LOBBYAPI_LANGUAGE_URDU },
    { L"uz-Cyrl",       LOBBYAPI_LANGUAGE_UZBEK },
    { L"uz-Latn",       LOBBYAPI_LANGUAGE_UZBEK },
    { L"vi",            LOBBYAPI_LANGUAGE_VIETNAMESE },
    { L"cy",            LOBBYAPI_LANGUAGE_WELCH },
    { L"wo",            LOBBYAPI_LANGUAGE_WOLOF },
    { L"yo",            LOBBYAPI_LANGUAGE_YORUBA },
    { L"ff-Latn",       LOBBYAPI_LANGUAGE_UNKNOWN }, // Fulah
    { L"fil-Latn",      LOBBYAPI_LANGUAGE_UNKNOWN }, // Filipino
    { L"haw-Latn",      LOBBYAPI_LANGUAGE_UNKNOWN }, // Hawaiian
    { L"hsb",           LOBBYAPI_LANGUAGE_UNKNOWN }, // Upper Sorbian
    { L"ii-Yiii",       LOBBYAPI_LANGUAGE_UNKNOWN }, // Yi
    { L"kok",           LOBBYAPI_LANGUAGE_UNKNOWN }, // Konkani
    { L"quc-Latn",      LOBBYAPI_LANGUAGE_UNKNOWN }, // K'iche'
    { L"sah-Cyrl",      LOBBYAPI_LANGUAGE_UNKNOWN }, // Sakha
    { L"sma-Latn-NO",   LOBBYAPI_LANGUAGE_UNKNOWN }, // Southern Sami (Norway)
    { L"sma-Latn-SE",   LOBBYAPI_LANGUAGE_UNKNOWN }, // Southern Sami (Sweden)
    { L"sms-Latn",      LOBBYAPI_LANGUAGE_UNKNOWN }, // Skolt Sami
    { L"syr-Syrc",      LOBBYAPI_LANGUAGE_UNKNOWN }, // Syriac
    { L"tzm-Latn",      LOBBYAPI_LANGUAGE_UNKNOWN }, // Tamazight, Latin
    { L"tzm-Tfng",      LOBBYAPI_LANGUAGE_UNKNOWN }  // Tamazight, Tifinagh
};

/*** Private Functions ****************************************************************************/

/*** Public functions *****************************************************************************/

/*F********************************************************************************/
/*!
    \Function NetConnStartup

    \Description
        Bring the network connection module to life. Does not actually start any
        network activity.

    \Input *pParams - startup parameters

    \Output
        int32_t     - zero=success, negative=failure

    \Notes
        pParams can contain the following terms:

        \verbatim
            -noupnp                             : disable UPNP
            -servicename=<game-year-platform>   : set servicename required for SSL use
        \endverbatim

    \Version 10/24/2012 (mclouatre)
*/
/********************************************************************************F*/
int32_t NetConnStartup(const char *pParams)
{
    NetConnRefT *pRef = _NetConn_pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allow NULL params
    if (pParams == NULL)
    {
        pParams = "";
    }

    // error out if already started
    if (pRef != NULL)
    {
        NetPrintf(("netconnwinrt: NetConnStartup() called with params='%s' while the module is already started\n", pParams));
        return(NETCONN_ERROR_ALREADY_STARTED);
    }

    // debug display of input params
    NetPrintf(("netconnwinrt: startup params='%s'\n", pParams));

    // allow and init ref
    if ((pRef = (NetConnRefT *)DirtyMemAlloc(sizeof(*pRef), NETCONN_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netconnwinrt: unable to allocate module state\n"));
        return(NETCONN_ERROR_NO_MEMORY);
    }
    memset(pRef, 0, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;
    pRef->eState = NetConnRefT::ST_INIT;
    pRef->iPeerPort = NETCONN_DEFAULT_UPNP_PORT;

    // start up dirtysock
    if (SocketCreate(THREAD_PRIORITY_HIGHEST, 0, 0) != 0)
    {
        DirtyMemFree((void*)pRef, NETCONN_MEMID, iMemGroup, pMemGroupUserData);
        NetPrintf(("netconnwinrt: unable to start up dirtysock\n"));
        return(NETCONN_ERROR_SOCKET_CREATE);
    }

    // save ref
    _NetConn_pRef = pRef;

    // create and configure dirtycert
    if (NetConnDirtyCertCreate(pParams))
    {
        NetConnShutdown(0);
        return(NETCONN_ERROR_DIRTYCERT_CREATE);
    }

    // start up protossl
    if (ProtoSSLStartup() < 0)
    {
        NetConnShutdown(0);
        NetPrintf(("netconnwinrt: unable to start up protossl\n"));
        return(NETCONN_ERROR_PROTOSSL_CREATE);
    }

    if (strstr(pParams, "-noupnp") == NULL)
    {
        pRef->pProtoUpnp = ProtoUpnpCreate();
        if (pRef->pProtoUpnp == NULL)
        {
            NetConnShutdown(0);
            NetPrintf(("netconnwinrt: unable to start up protoupnp\n"));
            return(NETCONN_ERROR_PROTOUPNP_CREATE);
        }
    }

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnQuery

    \Description
        No-op

    \Input *pDevice - device to scan (mc0:, mc1:, pfs0:, pfs1:)
    \Input *pList   - buffer to store output array in
    \Input iSize    - length of buffer in bytes

    \Output
        int32_t       - negative=error, else number of configurations

    \Version 10/24/2012 (mclouatre)
*/
/*************************************************************************************************F*/
int32_t NetConnQuery(const char *pDevice, NetConfigRecT *pList, int32_t iSize)
{
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnConnect

    \Description
        Used to bring the networking online with a specific configuration. Uses the
        configuration returned by NetConnQuery.

    \Input *pConfig - the configuration entry from NetConnQuery
    \Input *pOption - asciiz list of config parameters
                      "peerport=<port>" to specify peer port to be opened by upnp.
                      "capreload=<gamename-gameyear-platform>" to specify servicename to identify CA preload set
    \Input iData    - platform-specific

    \Output
        int32_t     - negative=error, zero=success

    \Version 10/24/2012 (mclouatre)
*/
/*************************************************************************************************F*/
int32_t NetConnConnect(const NetConfigRecT *pConfig, const char *pOption, int32_t iData)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // check connection options, if present
    if (pOption != NULL)
    {
        const char *pOpt;
        // check for specification of peer port
        if ((pOpt = strstr(pOption, "peerport=")) != NULL)
        {
            pRef->iPeerPort = strtol(pOpt+9, NULL, 10);
        }
    }
    NetPrintf(("netconnwinrt: upnp peerport=%d %s\n",
        pRef->iPeerPort, (pRef->iPeerPort == NETCONN_DEFAULT_UPNP_PORT ? "(default)" : "(selected via netconnconnect param)")));

    // make sure we aren't already connected
    if (pRef->eState == NetConnRefT::ST_INIT)
    {
        // connecting
        pRef->eState = NetConnRefT::ST_CONN;

        // discover upnp router information
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
    }
    else
    {
        NetPrintf(("netconnwinrt: NetConnConnect() ignored because already connected!\n"));
    }
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnDisconnect

    \Description
        Used to bring down the network connection. After calling this, it would
        be necessary to call NetConnConnect to bring the connection back up or
        NetConnShutdown to completely shutdown all network support.

    \Output
        int32_t     - negative=error, zero=success

    \Version 10/24/2012 (mclouatre)
*/
/*************************************************************************************************F*/
int32_t NetConnDisconnect(void)
{
    if (_NetConn_pRef->eState == NetConnRefT::ST_CONN)
    {
        // shutdown the interfaces (but leave API running)
        _NetConn_pRef->eState = NetConnRefT::ST_INIT;
    }

    // abort upnp operations
    if (_NetConn_pRef->pProtoUpnp != NULL)
    {
        ProtoUpnpControl(_NetConn_pRef->pProtoUpnp, 'abrt', 0, 0, NULL);
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
            snam: set DirtyCert service name
        \endverbatim

        Unhandled selectors are passed through to SocketControl()

    \Version 10/24/2012 (mclouatre)
*/
/********************************************************************************F*/
int32_t NetConnControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue, void *pValue2)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // make sure module is started before allowing any other control calls
    if (pRef == NULL)
    {
        NetPrintf(("netconnwinrt: warning - calling NetConnControl() while module is not initialized\n"));
        return(-1);
    }
    // set dirtycert service name
    if (iControl == 'snam')
    {
        return(DirtyCertControl('snam', 0, 0, pValue));
    }
    // pass through unhandled selectors to SocketControl()
    return(SocketControl(NULL, iControl, iValue, pValue, pValue2));
}

/*F*************************************************************************************************/
/*!
    \Function NetConnStatus

    \Description
        Check general network connection status. Different selectors return
        different status attributes.

    \Input iKind    - status selector
    \Input iData    - (optional) selector specific
    \Input *pBuf    - (optional) pointer to output buffer
    \Input iBufSize - (optional) size of output buffer

    \Output
        int32_t     - selector specific

    \Notes
        iKind can be one of the following:

        \verbatim
            addr: ip address of client
            bbnd: broadband true or false
            conn: true/false indication of whether connection in progress
            ethr: mac address of primary adapter (returned in pBuf), 0=success, negative=error
            locl: return locality (Windows locale) for local system, ex. 'enUS'
                  For unrecognized locale, 'zzZZ' will be returned.
            locn: return location (Windows location) for local system, ex. 'zzCA'
                  The language part of the return value should be ignored by the caller.
                  If local system has 'no location' set, 'zzZZ' will be returned.
            macx: mac address of primary adapter (returned in pBuf), 0=success, negative=error
            onln: true/false indication of whether network is operational
            type: NETCONN_IFTYPE_*
            open: true/false indication of whether network code running
            upnp: return protoupnp port info, if available
            vers: return DirtySDK version
        \endverbatim

    \Version 10/24/2012 (mclouatre)
*/
/*************************************************************************************************F*/
int32_t NetConnStatus(int32_t iKind, int32_t iData, void *pBuf, int32_t iBufSize)
{
    NetConnRefT *pRef = _NetConn_pRef;

    // see if network code is initialized
    if (iKind == 'open')
    {
        return(pRef != NULL);
    }
    // return DirtySDK version
    if (iKind == 'vers')
    {
        return(DIRTYVERS);
    }

    // make sure module is started before allowing any other status calls
    if (pRef == NULL)
    {
        NetPrintf(("netconnwinrt: warning - calling NetConnStatus() while module is not initialized\n"));
        return(-1);
    }

    // return client ip address
    if (iKind == 'addr')
    {
        return((int32_t)SocketGetLocalAddr());
    }

    // return whether we are broadband or not
    if (iKind == 'bbnd')
    {
        // assume broadband
        return(TRUE);
    }

    // return connection status
    if (iKind == 'conn')
    {
        return((pRef->eState == NetConnRefT::ST_CONN) ? '+onl' : '-dsc');
    }

    // return host mac address
    if ((iKind == 'ethr') || (iKind == 'macx'))
    {
        /* not supported on winrt
           Windows App API are sandboxed and are pretty restrictive on the info
           you can get about the user, mainly because of privacy concerns
           (use 'hwid' if you need a unique HW identifier)
        */
        NetPrintf(("netconnwinrt: 'ethr'/'macx' not supported on WinRT\n"));
        return(-1);
    }

    // see if connected to ISP/LAN
    if (iKind == 'onln')
    {
        return(pRef->eState == NetConnRefT::ST_CONN);
    }

    // return what type of interface we are connected with
    if (iKind == 'type')
    {
        // $todo review the value returned here for winprt

        // assume broadband
        return(NETCONN_IFTYPE_ETHER);
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

    // returns the unique identifier of the first network adapter found on the system
    if (iKind == 'hwid')
    {
#if defined(DIRTYCODE_WINRT_ONLY)
        if (iBufSize < sizeof(GUID))
        {
            NetPrintf(("netconnwinrt: buffer used with 'hwid' is not big enough (%d/%d)\n", iBufSize, sizeof(GUID)));
            return(-1);
        }

        if (pBuf == NULL)
        {
            NetPrintf(("netconnwinrt: 'hwid' used with an invalid buffer pointer\n"));
            return(-1);
        }

        auto hostNames = NetworkInformation::GetHostNames();
        auto it = hostNames->First();
        while (it->HasCurrent)
        {
            if (it->Current->IPInformation != nullptr)
            {
                GUID guid = it->Current->IPInformation->NetworkAdapter->NetworkAdapterId;
                memcpy(pBuf, &guid, sizeof(GUID));
                return(0);
            }
            it->MoveNext();
        }

        NetPrintf(("netconnwinrt: 'hwid' failed (no adapter found)\n"));
#endif
#if defined(DIRTYCODE_WINPRT_ONLY)
        // experimentation showed that it->Current->IPInformation->NetworkAdapter->NetworkAdapterId
        // throws a Platform::NotImplementedException
        NetPrintf(("netconnwinrt: 'hwid' not supported on WinPRT\n"));
#endif
        return(-1);
    }

    // return Windows locale information
    if (iKind == 'locl')
    {
#if defined(DIRTYCODE_WINRT_ONLY)
        char strCountry[3];
        uint16_t uLanguage, uCountry;
        int32_t iMap;

        // get the language code from the mapping table
        for (iMap = 0, uLanguage = LOBBYAPI_LANGUAGE_UNKNOWN; iMap < (signed)(sizeof(_NetConn_WinRtLanguageMap)/sizeof(_NetConn_WinRtLanguageMap[0])); iMap++)
        {
            if (wcscmp(_NetConn_WinRtLanguageMap[iMap].pSystemLang, GlobalizationPreferences::Languages->GetAt(0)->Data()) == 0)
            {
                uLanguage = _NetConn_WinRtLanguageMap[iMap].uDsLang;
                break;
            }
        }

        // transform the 2-char country code into a 2-byte code
        // we assume system returns country code equivalent to DS country code, so no need for a mapping table
        std::wcstombs(strCountry, GlobalizationPreferences::HomeGeographicRegion->Data(), 2);
        strCountry[2] = '\0';  // null-terminate
        uCountry = (unsigned)((strCountry[0] << 8 ) | strCountry[1]);

        return(LOBBYAPI_LocalizerTokenCreate(uLanguage, uCountry));
#endif
#if defined(DIRTYCODE_WINPRT_ONLY)
        return(LOBBYAPI_LOCALITY_UNKNOWN);
#endif
    }

    // return Windows location information
    if (iKind == 'locn')
    {
        return(LOBBYAPI_LOCALITY_UNKNOWN);
    }

    // pass unrecognized options to SocketInfo
    return(SocketInfo(NULL, iKind, iData, pBuf, iBufSize));
}

/*F*************************************************************************************************/
/*!
    \Function     NetConnShutdown

    \Description
        Shutdown the network code and return to idle state.

    \Input bStopEE      - ignored in PC implementation

    \Output
        int32_t         - negative=error, zero=success

    \Version 10/24/2012 (mclouatre)
*/
/*************************************************************************************************F*/
int32_t NetConnShutdown(uint32_t bStopEE)
{
    // error see if already stopped
    if (_NetConn_pRef == NULL)
    {
        return(NETCONN_ERROR_NOTACTIVE);
    }

    // destroy the upnp ref
    if (_NetConn_pRef->pProtoUpnp != NULL)
    {
        ProtoUpnpDestroy(_NetConn_pRef->pProtoUpnp);
        _NetConn_pRef->pProtoUpnp = NULL;
    }

    // shut down protossl
    ProtoSSLShutdown();

    // shut down dirtycert
    DirtyCertDestroy();

    // shut down Idle handler
    NetConnIdleShutdown();

    // disconnect the interfaces
    NetConnDisconnect();

    // shut down dirtysock
    SocketDestroy(0);

    // dispose of ref
    DirtyMemFree(_NetConn_pRef, NETCONN_MEMID, _NetConn_pRef->iMemGroup, _NetConn_pRef->pMemGroupUserData);
    _NetConn_pRef = NULL;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    NetConnSleep

    \Description
        Sleep the application (yield thread) for some number of milliseconds.

    \Input iMilliSecs    - number of milliseconds to block for

    \Version 10/24/2012 (mclouatre)
*/
/*************************************************************************************************F*/
void NetConnSleep(int32_t iMilliSecs)
{
    NetSleep(iMilliSecs);
}

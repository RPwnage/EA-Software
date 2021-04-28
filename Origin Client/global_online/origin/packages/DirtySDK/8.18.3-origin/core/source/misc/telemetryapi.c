/*H*************************************************************************************************/
/*!

    \File telemetryapi.c

    \Description
        This module implements the client API for the telemetry server.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2006.  ALL RIGHTS RESERVED.

    \Version 1.0 04/06/2006 (tdills) First Version

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#if defined(_XBOX)
#include <xtl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "protoaries.h"
#include "telemetryapi.h"
#include "lobbytagfield.h"
#include "cryptmd5.h"

#include "dirtyvers.h"

#include "netconn.h"


/*** Defines ***************************************************************************/

#if DIRTYCODE_DEBUG
    #define TELEMETRY_VERBOSE                 (1)     //!< for less debugging information at the console
#endif

//! reserved space in the send buffer for telemetry 3.0 tags
#define TELEMETRY3_RESERVE_SIZE               (5)

//! reserved space in the send buffer for telemetry 2.0 tags
#if DIRTYCODE_DEBUG
#define TELEMETRY_RESERVE_SIZE                (58) 
#else
#define TELEMETRY_RESERVE_SIZE                (47) 
#endif

#define TELEMETRY_EVENT_MAX_SIZE              (4096)

#define TELEMETRY_SENDBUFFERSIZE              (1024)  //!< The size of the lobby send buffer
#define TELEMETRY_SENDBUFFER_EVENTSIZEMAX     ((TELEMETRY_SENDBUFFERSIZE * 4)/5 - TELEMETRY_RESERVE_SIZE) //!< This is how much space can be used by events

#define TELEMETRY_ENCRYPTION_BUFFER_OVERHEAD_SIZE (8 + 1) //!< The size of an encrypted buffer

#define TELEMETRY_TAG_FIELD_SIZE (((((TELEMETRY_SENDBUFFERSIZE + (2 * TELEMETRY_ENCRYPTION_BUFFER_OVERHEAD_SIZE))*8) + 6) /7) + 1)

//! Event setup information
#define TELEMETRY_NUMEVENTS_DEFAULT           (64)     //!< Default # of events to create

#define TELEMETRY_AUTOMATIC_RETRY_MAX         (10)      //!< Hardcoded retry maximum, in case of a faulty config

//! If connected, the logger will send data to the server
//!  at specific intervals if there's an event in the buffer
#define TELEMETRY_SENDBUFFER_EVENTTIMEOUT_DEFAULT         (15*1000)   //! Default value for event sending (in milliseconds)

//! If connected, the logger will send data to the server
//!  if the buffer fills beyond this point (in percentage of the buffer size) 
#define TELEMETRY_SENDBUFFER_EVENTTHRESHOLDPCT_DEFAULT   (60)         //! Default value for event sending (in buffer full %)

//! Filter macros
#define TELEMETRY_FILTERRULES_MAX             (32)        //! maximum number of filter rules to apply
#define TELEMETRY_FILTERRULES_WILDCARD        ('****')    //!< Filter rule wildcard token

//! TelemetryApi Event Buffer State macros
#define TELEMETRY_EVENTBUFFERSTATE_EMPTY          (1<<0)  //!< signifies that the buffer is empty
#define TELEMETRY_EVENTBUFFERSTATE_FULL           (1<<1)  //!< signifies that the buffer is full
#define TELEMETRY_EVENTBUFFERSTATE_SENDHALTED     (1<<2)  //!< temporarily halt sending to the server

//! This macro checks incoming event characters for validity.
#define TELEMETRY_EVENT_TOKENCHAR_VALID(cChar)  ( ((cChar >= '0') && (cChar <= '9')) || \
                                                    ((cChar >= 'A') && (cChar <= 'Z')) || \
                                                    ((cChar >= 'a') && (cChar <= 'z')) )
//! Check an entire event token
#define TELEMETRY_EVENT_TOKEN_VALID(uToken) \
        (TELEMETRY_EVENT_TOKENCHAR_VALID((char)(((uToken)>>24)&0xFF)) && \
         TELEMETRY_EVENT_TOKENCHAR_VALID((char)(((uToken)>>16)&0xFF)) && \
         TELEMETRY_EVENT_TOKENCHAR_VALID((char)(((uToken)>>8)&0xFF))  && \
         TELEMETRY_EVENT_TOKENCHAR_VALID((char)((uToken)&0xFF)))

//! This macro checks FILTER rules, which differ because they can use a wildcard.
#define TELEMETRY_FILTER_TOKENCHAR_VALID(cChar)  \
        ( (cChar == '*') || TELEMETRY_EVENT_TOKENCHAR_VALID(cChar) )

//! Check an entire filter token (can include wildcard token)
#define TELEMETRY_FILTER_TOKEN_VALID(uToken) \
        (TELEMETRY_FILTER_TOKENCHAR_VALID((char)(((uToken)>>24)&0xFF)) && \
         TELEMETRY_FILTER_TOKENCHAR_VALID((char)(((uToken)>>16)&0xFF)) && \
         TELEMETRY_FILTER_TOKENCHAR_VALID((char)(((uToken)>>8)&0xFF))  && \
         TELEMETRY_FILTER_TOKENCHAR_VALID((char)((uToken)&0xFF)))

//! we allow some other characters as prepend characters in the server log
#define TELEMETRY_SERVERLOGPREPENDCHAR_VALID(cChar)  \
    ( (cChar == '+') || (cChar == '-') || (cChar == '~') || (cChar == '=') || TELEMETRY_EVENT_TOKENCHAR_VALID(cChar) )

//! we allow some non-alpha-numerics in the string - this is used by TelemetryApiEventStringFormat
#define TELEMETRY_EVENTSTRINGCHAR_VALID(cChar) \
            ( TELEMETRY_EVENT_TOKENCHAR_VALID(cChar) || \
                (cChar == '.') || \
                (cChar == '/') || \
                (cChar == '+') || \
                (cChar == '-') || \
                (cChar == '=') || \
                (cChar == '_') || \
                (cChar == '~') )

//! for telemetry 3 we need to reserve a different set of characters
#define TELEMETRY3_EVENTSTRINGCHAR_VALID(cChar) \
            ( TELEMETRY_EVENT_TOKENCHAR_VALID(cChar) || \
                (cChar == '.') || \
                (cChar == '+') || \
                (cChar == '-') || \
                (cChar == '_') || \
                (cChar == '~') || \
                (cChar == '$'))

//! define the event string format we'll use to transfer the data to the server
#define TELEMETRY_EVENTSTRING_FORMAT                  ("%08X/%c;%08X/%c%c%c%c/%c%c%c%c/%s")
#define TELEMETRY3_EVENTSTRING_FORMAT                 ("%08X/%c;%08X/%c%c%c%c/%c%c%c%c/%c%c%c%c/%s")

//! Each event entry is a maximum of this long, and will be truncated if necessary
//!                                     timestamp / prependchar ; step / token / token / 16 byte string (NULL)
#define TELEMETRY_EVENTSTRING_LENGTH       (8    +1     +1     +1  +8 +1  +4  +1  +4  +1  +16            +1)

//! This calculates the number of entries we can fit into the buffer, once we know the string size
#define TELEMETRY_ENTRIESTOSEND_MAX       (TELEMETRY_SENDBUFFER_EVENTSIZEMAX / TELEMETRY_EVENTSTRING_LENGTH)

//! this is the string used when the event string (sent to TelemetryApiEvent) is NULL or 0 length
#define TELEMETRY_EVENTSTRING_EMPTY                   ("-")

//! this is the character used to denote the telemetry buffer type (character prepender)
#define TELEMETRY_EVENTSTRING_BUFFERTYPECHARACTER     ('-')

//! when an illegal character is encountered in a log message, this character will replace it
#define TELEMETRY_ILLEGALEVENTSTRINGCHAR_REPLACEMENT  ('_')

#define TELEMETRY_ANONAUTH_SECRET   ("The truth is back in style.")
#define TELEMETRY_AUTHKEY_LEN       (512)

#define TELEMETRY_SUBDIR_LEN        (5)

#define TELEMETRY_MAXIMUM_ANON_DOMAIN_LEN (64)

//! minimum size of a telemetry 3 event
//!                               timestamp / prependchar ; step / token / token / token / (NULL)
#define TELEMETRY3_EVENT_MINSIZE     (8    +1     +1     +1  +8 +1  +4  +1  +4  +1  +4  +1   +1)

/*** Macros ****************************************************************************/

//! Macro to provide and easy way to display the token in character format
#define TELEMETRY_LocalizerTokenPrintCharArray(uToken)  \
    (char)(((uToken)>>24)&0xFF), (char)(((uToken)>>16)&0xFF), (char)(((uToken)>>8)&0xFF), (char)((uToken)&0xFF)

/*** Type Definitions ******************************************************************/

//! filter rule definition
typedef struct TelemetryApiFilterRuleT
{
    int32_t  iEnable;           //!< 1=add, -1=subtract, 0=empty rule
    uint32_t uModuleID;         //!< moduleID or wildcard '****' (no partial wildcards)
    uint32_t uGroupID;          //!< groupID or wildcard  '****' (no partial wildcards)
    uint32_t uStringID;            //!< stringID or wildcard '****' (no partial wildcards)
    char     uStrEvent[TELEMETRY3_EVENTSTRINGSIZE_DEFAULT];
} TelemetryApiFilterRuleT;

//! callback definition
typedef struct TelemetryApiCallbackDataT
{
    TelemetryApiCallbackT   *pCallback;     //!< the callback to use
    void                    *pUserData;     //!< the data to send with
} TelemetryApiCallbackDataT;

//! current module state
struct TelemetryApiRefT
{
    // Generic TelemetryApi data members
    ProtoAriesRefT  *pAries;    //!< network session reference
    
    // module memory group
    int32_t iMemGroup;      //!< module mem group id
    void *pMemGroupUserData;//!< user data associated with mem group
    
    // Custom IP address
    char    strCustomIP[65];                    //!< custom IP address, typically for encrypting an IP in the server logs
    int32_t iTruncateIP;                        //!< Boolean to indicate last octet of IP must be truncated on the server side

    char strAuth[TELEMETRY_AUTHKEY_LEN];      //! authorization string
    char strServerName[256];    //! server name, replaces the old server address field
    int32_t iPort;          //! server port
    int32_t bCanConnect;    //! flag to determine if the client has allowed us to connect
    int32_t bFullSendPending; //! when ProtoAries has emptied its buffer we should send the FullSend event

    char strUserLocale[4];  //! user's locale from EX-Telemetry string
    char strUserLocation[4]; //! user's location from their OS
    char strDisabledCountries[1024]; //! list of disabled countries from lobby config
    int32_t bCountryDisabled;      //! is telemetry disabled based on country code?
    int32_t bUserDisabled;         //! is telemetry disabled based on user settings?

    char strSubDir[TELEMETRY_SUBDIR_LEN];        //! telemetry subdirectory

    // BeginTransaction data members
    int32_t bTransactionBegPending;     //!< are we currently awaiting a &beg response?
    TelemetryApiBeginTransactionCBT *pTransactionBegCB;          //!< a pointer to the BeginTransaction response callback
    void    *pTransactionBegUserData;   //!< client-provided data to return in the BeginTransaction callback

    // Buffer transmission data members
    int32_t                 iTickStartTime;                 //!< abs. time (in (s)) when authentication occurred
    uint32_t                uLastServerSendTime;            //!< last time data was sent to svr

    char                   *pEvent3BufferHead;              //!< start of buffer for storing telemetry 3 events
    char                   *pEvent3BufferTail;              //!< tail of buffer for storing telemetry 3 events
    uint32_t                uEvent3BufferSize;              //!< size of the buffer
    uint32_t                uNumEvent3Submitted;            //!< number of events submitted

    char                   *pServerString;                  //!< server string used to format data for sending
    uint32_t                uServerStringSize;              //!< size of the server string data buffer
    char                   *pTagString;                     //!< tagfield string for sending the server data
    uint32_t                uTagStringSize;                 //!< size of the tagfield server string data buffer
    char                    cEntryPrepender;                //!< the character to prepend each entry in the log with

    char                   *pEncryptionBuffer;              //!< optional encryption buffer, allocated if encryption is turned on
    uint32_t                uEncryptionBufferSize;          //!< size of the encryption buffer

    // Allow authentication bypass for clients with no user information
    uint8_t                 bDisableAuthentication;

    // Enable anonymous telemetry 
    uint8_t                    bAnonymous;

    // Step counter for events
    uint32_t                uStep;

    // Guard against NetTick wraparound
    int32_t                 iWrapTime;
    int32_t                 iLastTick;

    // Filter rules
    uint32_t                uNumFilters;                            //!< current number of rules to process
    TelemetryApiFilterRuleT Filters[TELEMETRY_FILTERRULES_MAX];   //!< the filter structs

    // Callbacks
    TelemetryApiCallbackDataT callbacks[TELEMETRY_CBTYPE_TOTAL]; //!< callbacks to use for particular events

    // Event handling data members
    TelemetryApiBufferTypeE eBufferType;        //!< buffer type
    uint32_t                uEventBufSize;      //!< capacity of the event buffer
    uint32_t                uEventHead;         //!< event buffer head counter
    uint32_t                uEventTail;         //!< event buffer tail counter
    uint32_t                uEventState;        //!< maintains internal buffer state (empty, full, etc.)
    uint32_t                uEventThreshold;    //!< the event sending threshold value - anything beyond this will trigger immediate send
    uint32_t                uEventTimeout;      //!< if there's an event, it'll wait this long (in milliseconds) to send (0=immediate send)
    TelemetryApiEventT      *Events;            //!< event buffer, variable size based on Create call

    // Session ID
    char strSessionID[TELEMETRY_SESSION_ID_LEN];

    int32_t                 iAuthTick;          //!< tick value when authentication occurred

    char failedLastSend;                        //!< indicates last send failed, so send AUTH regardless of connection status

    uint8_t                    bEncrypt;        //!< indicates whether telemetry is set to encrypt its data
    uint8_t                    bUseServerTime;  //!< indicates whether telemetry should log events using the server's time, or client's time
    TelemetryServerTimeStampOverrideTypeE bUseServerTimeOveride; //!< indicates whether the server overides bUseServerTime
    uint32_t                   uSubmit3MaxRetry;//!< maximum number of retries to send data when a full buffer is encountered

    uint8_t                    bEvent3BufferBufferTruncated;    //!< indicated whether the telemetry 3 buffer was artificially shortened to help fit into the ProtoAries send buffer
    char                       *pEvent3BufferUntruncatedTail;   //!< the pointer to the tail of the telemetry buffer before alteration
    char                       *pEvent3TruncationHead;          //!< point to the start of events which have been truncated

    TelemetryApiCallback2T      *pLoggerCallback;
    TelemetryApiCallback2T      *pTransactionEndCallback;
    TelemetryApiCallback2T      *pTransactionDataCallback;
};

//! header structure for a snapshot to persistently store events
typedef struct TelemetryApiSnapshotHeaderT
{
    uint32_t                uBufferSize;        //!< size of the entire buffer
    uint32_t                uEventsChecksum;    //!< checksum of the stored events
    uint32_t                uNumEvents;         //!< number of events
} TelemetryApiSnapshotHeaderT;

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

static const unsigned char hex_encode[16] =
    "0123456789abcdef";

// Public variables

/*** Private Functions *****************************************************************/

static int32_t _TelemetryApiTrySendToServer(TelemetryApiRefT *pTelemetryRef);

static int32_t _TelemetryApiSetupAttribute(uint32_t uKey, char *pTag);

static int32_t _TelemetryApiRelativeTick(TelemetryApiRefT *pTelemetryRef);

static void _TelemetryApiScrubString(char *pStr);

static void _TelemetryApiObfuscateAuth(char *pDst, char *pSrc);

static uint32_t _TelemetryApiObfuscateNPayload(char *pDst, char *pSrc, uint32_t len, uint32_t pDstLen);

static uint32_t _TelemetryApiObfuscatePayload(char *pDst, char *pSrc, uint32_t pDstLen);

static TelemetryApiEventT* _TelemetryApiGetFromFront(TelemetryApiRefT *pTelemetryRef, uint32_t uIndex);

static void _TelemetryApiRemoveFromFront(TelemetryApiRefT *pTelemetryRef, int32_t iNumEvents);

static uint32_t  _TelemetryApiFilterAttribute(const char *pEventAttribute, const char *pAttributeFilter);

// encode attributes directly into a stream
int32_t _TelemetryApiStreamEncAttributeChar(char *pStart, uint32_t uKey, char iValue, uint32_t uMaxSize);
int32_t _TelemetryApiStreamEncAttributeInt(char *pStart, uint32_t uKey, int32_t iValue, uint32_t uMaxSize);
int32_t _TelemetryApiStreamEncAttributeLong(char *pStart, uint32_t uKey, int64_t iValue, uint32_t uMaxSize);
int32_t _TelemetryApiStreamEncAttributeFloat(char *pStart, uint32_t uKey, float fValue, uint32_t uMaxSize);
int32_t _TelemetryApiStreamEncAttributeString(char *pStart, uint32_t uKey, const char * pValue, uint32_t uMaxSize);

// Truncate events
char* _TelemetryApiTruncEvents3(TelemetryApiRefT *pTelemetryRef, const uint32_t uMaxLength);
char* _TelemetryApiUntruncEvents3(TelemetryApiRefT *pTelemetryRef, uint8_t bSendSuccessful);

// Internal SubmitEvent helper
int32_t _TelemetryApiSubmitEvent3Ex( TelemetryApiRefT *pTelemetryRef, TelemetryApiEvent3T *pEvent, uint32_t uNumAttributes, va_list *marker );

/*F********************************************************************************/
/*!
    \Function _TelemetryApiTriggerCallback

    \Description
        Trigger a callback if possible for the given callback type

    \Input *pTelemetryRef        - module state
    \Input  eType       - callback type to trigger

    \Version 01/25/2006 (jfrank)
*/
/********************************************************************************F*/
static void _TelemetryApiTriggerCallback(TelemetryApiRefT *pTelemetryRef, TelemetryApiCBTypeE eType)
{
    // if we've got a callback for that type, call it
    if(pTelemetryRef->callbacks[eType].pCallback)
    {
        pTelemetryRef->callbacks[eType].pCallback(pTelemetryRef, pTelemetryRef->callbacks[eType].pUserData);
    }
}


/*F********************************************************************************/
/*!
    \Function _TelemetryApiClearEvents

    \Description
        Wipe out all events in the buffer

    \Input  *pTelemetryRef - module to wipe events in

    \Version 01/20/2006 (jfrank)
*/
/********************************************************************************F*/
static void _TelemetryApiClearEvents(TelemetryApiRefT *pTelemetryRef)
{
    if(pTelemetryRef)
    {
        pTelemetryRef->uEventHead     = 0;
        pTelemetryRef->uEventTail     = 0;
        pTelemetryRef->uEventState    |= TELEMETRY_EVENTBUFFERSTATE_EMPTY;
        pTelemetryRef->uEventState    &= (~TELEMETRY_EVENTBUFFERSTATE_FULL);
        _TelemetryApiTriggerCallback(pTelemetryRef, TELEMETRY_CBTYPE_BUFFEREMPTY);
    }
}


/*F********************************************************************************/
/*!
    \Function _TelemetryApiChecksum

    \Description
        Generate a checksum for a chunk of code

    \Input  *pBuf       - the buffer to check
    \Input   uSize      - size of buffer to check
    
    \Output  uint32_t   - checksum

    \Version 02/15/1999 (gschaefer) First Version
    \Version 01/20/2006 (jfrank)    Fix variable names to be standards compliant
    \Version 04/06/2006 (tdills)    Moved to TelemetryApi
*/
/********************************************************************************F*/
static uint32_t _TelemetryApiChecksum(const void *pBuf, uint32_t uSize)
{
    uint32_t uChecksum = 0;
    const char *pCharBuf = (const char *)pBuf;

    // quick calc on data
    while (uSize > 0) 
    {
        uChecksum = (uChecksum * 13) + *pCharBuf;
        --uSize; pCharBuf++;
    }

    // return the value
    return(uChecksum);
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiNumEvents

    \Description
        Return the current number of events in the buffer

    \Input *pTelemetryRef        - module state

    \Output uint32_t    - current number of events in the buffer

    \Version 01/10/2006 (jfrank) First Version
    \Version 04/06/2006 (tdills) Moved to TelemetryApi
*/
/********************************************************************************F*/
static uint32_t _TelemetryApiNumEvents(TelemetryApiRefT *pTelemetryRef)
{
    // check for errors
    if (pTelemetryRef == NULL)
        return(0);

    // first check the easy stuff
    if (pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_EMPTY)
    {
        return(0);
    }
    else if (pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_FULL)
    {
        return(pTelemetryRef->uEventBufSize);
    }

    // now do the more complex checks
    else if (pTelemetryRef->uEventTail >= pTelemetryRef->uEventHead)
    {
        // we haven't wrapped
        return((pTelemetryRef->uEventTail - pTelemetryRef->uEventHead) + 1);
    }
    else
    {
        // we have wrapped around
        return((pTelemetryRef->uEventBufSize - pTelemetryRef->uEventHead) + pTelemetryRef->uEventTail + 1);
    }
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiFilterAttribute

    \Description
        Return 1 if the event can stay based only on the attribute, or 0 if the event has been filtered

    \Input *pEventAttribute         - attribute string of incoming event
    \Input *pAttributeFilter        - string of the filter to be parsed

    \Output uint32_t    - 0 = no match, 1 = match  

    \Version 01/26/2011 (amakoukji) First Version
*/
/********************************************************************************F*/

static uint32_t  _TelemetryApiFilterAttribute(const char *pEventAttribute, const char *pAttributeFilter)
{
    char *pAttributeEnd = NULL;
    const char *pCurToken = NULL;
    const char *pCurTokenEnd = NULL;
    const char *pEventTokenStart = NULL;
    const char *pPlaceholderEqual = NULL, *pPlaceholderAmp = NULL, *pPlaceholderNull;
    char strTmp[256];
    uint32_t bExactMatchMode = 0;
    uint32_t uAttributesTreated = 0, uTotalEventAttributes = 0;

    enum TokenStateE
    {
        TOKEN_STATE_ATTRIBUTE_NAME = 0,
        TOKEN_STATE_ATTRIBUTE_VALUE,
        TOKEN_STATE_ATTRIBUTE_DONE
    };

    enum TokenStateE tokenState = TOKEN_STATE_ATTRIBUTE_NAME;
    enum TokenStateE tokenNextState = TOKEN_STATE_ATTRIBUTE_NAME;

    // If there is no Attribute filter, then it need not be considered, thus the attribute condition is fulfilled
    if (pAttributeFilter == NULL || strlen(pAttributeFilter) < 1)
    {
        return(1);
    }

    // Skip the initial '&'
    pCurToken = pAttributeFilter;
    ++pCurToken;

    // Get the attribute's end, we only need up to the next ',' or '\0'
    pAttributeEnd = strchr(pAttributeFilter, ',');
    if (pAttributeEnd == NULL)
    {
        pAttributeEnd = strchr(pAttributeFilter, '\0');
    }

    if (pAttributeEnd > pAttributeFilter)
    {
        if (*(pAttributeEnd - 1) == '*')
        {
            bExactMatchMode = 0;
            --pAttributeEnd;
        }
        else
        {
            bExactMatchMode = 1;
        }
    }

    // Now start going through the tokens and seeing if they are contained within / are an exact match
    // Get the end of the token, which is either an '=', '&' or NULL

    tokenState = TOKEN_STATE_ATTRIBUTE_NAME;
    pPlaceholderNull = strchr(pCurToken, '\0');
    while (tokenState != TOKEN_STATE_ATTRIBUTE_DONE)
    {
        tokenNextState = TOKEN_STATE_ATTRIBUTE_VALUE;

        // Find the end of the token, either a '=', '&' or NULL;
        pPlaceholderEqual =   strchr(pCurToken, '=');
        pPlaceholderAmp =     strchr(pCurToken, '&');

        if (pPlaceholderAmp == NULL)
        {
            pPlaceholderAmp = pPlaceholderNull;
        }

        if (pPlaceholderEqual == NULL)
        {
            pPlaceholderEqual = pPlaceholderNull;
        }

        // Next token is an '=', it means that the next token to parse is a value, if it's an '&', it is another attribute name, otherwise we will be done
        if (pPlaceholderEqual < pPlaceholderAmp)        
        {
            tokenNextState = TOKEN_STATE_ATTRIBUTE_VALUE; 
            pCurTokenEnd = pPlaceholderEqual;
        }
        else if (pPlaceholderAmp < pPlaceholderEqual)   
        {
            tokenNextState = TOKEN_STATE_ATTRIBUTE_NAME;  
            pCurTokenEnd = pPlaceholderAmp;
        }
        else                                                
        {
            tokenNextState = TOKEN_STATE_ATTRIBUTE_DONE;  
            pCurTokenEnd = pPlaceholderNull;
        }
        
        if (pCurTokenEnd > pAttributeEnd) 
        {
            pCurTokenEnd = pAttributeEnd;
        }

        ds_strnzcpy(strTmp, pCurToken, (pCurTokenEnd - pCurToken) + 1);
        // Put in the equals sign, to be sure you are at the END of a tag.
        // Since attributes are always 4 characters long, if there`s a match it will always be unique (unless the attribute name is repeated, in which case it`ll be ignored)
        ds_strnzcat(strTmp, "=", sizeof(strTmp));

        // Search for the name of the attribute in the outgoing event. If it doesn't exist, this filter doesn't apply
        pEventTokenStart = strstr(pEventAttribute, strTmp);        

        // Check if the attribute name exists
        if(pEventTokenStart == NULL)
        {
            return(0);
        }

        ++uAttributesTreated;
        tokenState = tokenNextState;

        // Now we've determined that the Attribute exists.
        // Next, if we need to compare the value, do so
        if (tokenState == TOKEN_STATE_ATTRIBUTE_DONE)
        {
            continue;
        }
        else if (tokenState == TOKEN_STATE_ATTRIBUTE_VALUE)
        {       
            // Skip to the values
            pEventTokenStart += (strlen(strTmp));
            pCurToken += (strlen(strTmp));

            pPlaceholderEqual =   strchr(pCurToken, '=');
            pPlaceholderAmp =     strchr(pCurToken, '&');

            if (pPlaceholderAmp == NULL)
            {
                pPlaceholderAmp = pPlaceholderNull;
            }

            if (pPlaceholderEqual == NULL)  
            {
                pPlaceholderEqual = pPlaceholderNull;
            }

            // Next token is an '=', it means that the next token to parse is a value, if it's an '&', it is another attribute name, otherwise we will be done
            if (pPlaceholderEqual < pPlaceholderAmp)        
            {
                tokenNextState = TOKEN_STATE_ATTRIBUTE_VALUE;  // Improper format
                pCurTokenEnd = pPlaceholderEqual;
            } 
            else if (pPlaceholderAmp < pPlaceholderEqual)   
            {
                tokenNextState = TOKEN_STATE_ATTRIBUTE_NAME;  
                pCurTokenEnd = pPlaceholderAmp;
            }
            else                                                
            {
                tokenNextState = TOKEN_STATE_ATTRIBUTE_DONE;  
                pCurTokenEnd = pPlaceholderNull;
            }

            if (pCurTokenEnd > pAttributeEnd)
            {
                pCurTokenEnd = pAttributeEnd;
            }

            ds_strnzcpy(strTmp, pCurToken, (pCurTokenEnd - pCurToken) + 1);

            // Check if the data matches, case sensitive
            // We cannot check for length since the attribute string does not have delimiters between the end of a value and the name of the next attribute
            if (strncmp(strTmp, pEventTokenStart, strlen(strTmp)) != 0)
            {
                return(0); // Unmatched data found
            }


            tokenState = tokenNextState;
        }
         // If those matched, move onto the next token

        if (pPlaceholderAmp != pPlaceholderNull)
        {
            pCurToken = strchr(pCurToken, '&');
            ++pCurToken;

            // One last check to see if we are beyond the bounds of this particular filter, and if we are, we are done
            if (pCurToken > pAttributeEnd)
                break;
        }
     }

    // Final check, if we are looking for an exact match, count the number of '=' in the attributes to send.
    // If it is not equal to the number of tokens processed, more tags are present, are therefore they do not match
    // 
    //
    pEventTokenStart = pEventAttribute;
    while (pEventTokenStart != NULL)
    {
        pEventTokenStart = strchr(pEventTokenStart, '=');
        if (pEventTokenStart != NULL)
        {
            ++uTotalEventAttributes;
            ++pEventTokenStart;
        }
        
    }

    if((uTotalEventAttributes != uAttributesTreated) && (bExactMatchMode > 0))
        return(0);

    // All conditions fulfilled
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiFilterEvent

    \Description
        Return 1 if the event can stay, or 0 if the event has been filtered

    \Input *pTelemetryRef        - module state (contains filter rules)
    \Input  uModuleID   - module ID of incoming event
    \Input  uGroupID    - group ID of incoming event

    \Output uint32_t    - 1=event OK, 0=event filtered (should not be recorded)

    \Version 01/10/2006 (jfrank) First Version
    \Version 04/06/2006 (tdills) Moved to TelemetryApi
*/
/********************************************************************************F*/
static uint32_t _TelemetryApiFilterEvent(TelemetryApiRefT *pTelemetryRef, uint32_t uModuleID, uint32_t uGroupID)
{
    TelemetryApiFilterRuleT *pRule;
    int32_t bEnabled = 1;           //!< all events start out as "enabled"
    uint32_t uLoop;

    // run through the rules and determine if we can keep this event
    for(uLoop = 0; uLoop < pTelemetryRef->uNumFilters; uLoop++)
    {
        pRule = &(pTelemetryRef->Filters[uLoop]);
        // if (((the module rule is wild) OR (the module ID matches)) AND ((the group ID is wild) or (the group id matches)))
        if (((pRule->uModuleID == TELEMETRY_FILTERRULES_WILDCARD) || (pRule->uModuleID == uModuleID)) &&
            ((pRule->uGroupID  == TELEMETRY_FILTERRULES_WILDCARD) || (pRule->uGroupID  == uGroupID)))
        {
            // the rule applies, so set the flag
            bEnabled = pRule->iEnable;
        }
    }
    
    // remember that the rules are (+1, -1)
    //  and we need to return (0, 1)
    return((bEnabled == 1) ? 1 : 0);
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiFilterEvent3

    \Description
        Return 1 if the event can stay, or 0 if the event has been filtered

    \Input *pTelemetryRef        - module state (contains filter rules)
    \Input *pEvent               - TelemetryApiEvent3T struct contaning the event's Module, group and string ids

    \Output uint32_t    - 1=event OK, 0=event filtered (should not be recorded)

    \Version 22/07/2010 (geoffreyh) First Version
*/
/********************************************************************************F*/
static uint32_t _TelemetryApiFilterEvent3(TelemetryApiRefT *pTelemetryRef, TelemetryApiEvent3T *pEvent)
{
    TelemetryApiFilterRuleT *pRule;
    int32_t bEnabled = 1;           //!< all events start out as "enabled"
    uint32_t uLoop, uModuleID, uGroupID, uStringID; 

    // Extract the necessary info from the Event
    uModuleID = pEvent->uModuleID;
    uGroupID  = pEvent->uGroupID;
    uStringID = pEvent->uStringID;
   
    // run through the rules and determine if we can keep this event
    for(uLoop = 0; uLoop < pTelemetryRef->uNumFilters; uLoop++)
    {
        pRule = &(pTelemetryRef->Filters[uLoop]);
        // if (((the module rule is wild) OR (the module ID matches)) AND ((the group ID is wild) or (the group id matches)))
        if (((pRule->uModuleID == TELEMETRY_FILTERRULES_WILDCARD) || (pRule->uModuleID == uModuleID)) &&
            ((pRule->uGroupID  == TELEMETRY_FILTERRULES_WILDCARD) || (pRule->uGroupID  == uGroupID)) &&
            ((pRule->uStringID == TELEMETRY_FILTERRULES_WILDCARD) || (pRule->uStringID == uStringID)) &&
            _TelemetryApiFilterAttribute(pEvent->strEvent, pRule->uStrEvent))
        {
            // the rule applies, so set the flag
            bEnabled = pRule->iEnable;
        }
    }
    
    // remember that the rules are (+1, -1)
    //  and we need to return (0, 1)
    return((bEnabled == 1) ? 1 : 0);
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiTruncEvents3

    \Description
        Returns an altered telemetry buffer, whose size is limited in order to not overflow the ProtoAries send buffer.

    \Input *pTelemetryRef        - module state, constains pointer to the telemetry buffer containing the queued events, delimited by ','
    \Input uMaxLength            - Maximum length to be copied to the events buffer

    \Output char*                - Pointer to the telemetry buffer

    \Version 03/03/2011 (amakoukji) First Version
*/
/********************************************************************************F*/
char* _TelemetryApiTruncEvents3(TelemetryApiRefT *pTelemetryRef, const uint32_t uMaxLength)
{
    char *pEventEnd = NULL;
    char *pEventStart = pTelemetryRef->pEvent3BufferHead;

    if (pTelemetryRef->bEvent3BufferBufferTruncated > 0 || pTelemetryRef->pEvent3BufferUntruncatedTail != NULL)
    {
        NetPrintf(("telemetryapi: Attempt to truncate an already truncated event3 buffer"));
        return pTelemetryRef->pEvent3BufferHead;
    }

    if ((uint32_t)(pTelemetryRef->pEvent3BufferTail - pTelemetryRef->pEvent3BufferHead) <= uMaxLength)
    {
        // Buffer is short enough, nothing to be done.
        return pTelemetryRef->pEvent3BufferHead;
    }

    // Parse through the telemetry buffer until you either get to then end, or surpass the maximum
    while (*pEventStart != '\0')
    {
        if (*pEventStart == ',')
        {
            ++pEventStart;
        }

        pEventEnd = strchr(pEventStart, ',');
        if (pEventEnd == NULL)
        {
            pEventEnd = strchr(pEventStart, '\0');
        }

        if (pEventEnd - pTelemetryRef->pEvent3BufferHead > (int32_t)uMaxLength)
        {
            // The end of this event is past the max length, this event and all the events after will NOT 
            // be included in the next ProtoAries send buffer.
            // Alter the string to stop before the first element of this event, and save that it has been altered.

            if (pEventStart == pTelemetryRef->pEvent3BufferHead)
            {
                // Here is the SOMETHING IS VERY WRONG catch.
                // The first event is too big to fit, which should not be possible.
                // Try to send anyway but you may be a ProtoAries buffer overflow
                NetPrintf(("telemetryapi: Telemetry Event too long found in the telemetry buffer"));
                break;
            }
            else
            {
                pTelemetryRef->bEvent3BufferBufferTruncated = 1;
                pTelemetryRef->pEvent3TruncationHead = pEventStart;
                *(--pEventStart) = '\0';
                pTelemetryRef->pEvent3BufferUntruncatedTail = pTelemetryRef->pEvent3BufferTail;
                pTelemetryRef->pEvent3BufferTail = pEventStart + 1;
            }
            
        }
        else
        {
            pEventStart = pEventEnd;
        }
    }
    

    // Return the buffer, created this way in case we want to deep copy only the relevant events to another buffer later on,
    // in which case we will return that instead.
    return(pTelemetryRef->pEvent3BufferHead);
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiUntruncEvents3

    \Description
        Returns a restored telemetry buffer, reverting the temporary changes from _TelemetryApiTruncEvents3().

    \Input *pTelemetryRef        - module state, constains pointer to the telemetry buffer containing the queued events, delimited by ','
    \Input bSendSuccessful       - indicates whether the send was successful

    \Output char*                - Pointer to the telemetry buffer

    \Version 03/03/2011 (amakoukji) First Version
*/
/********************************************************************************F*/
char* _TelemetryApiUntruncEvents3(TelemetryApiRefT *pTelemetryRef, uint8_t bSendSuccessful)
{
    char *pTruncationHead = pTelemetryRef->pEvent3TruncationHead - 1;
    char *pCurrentHead = pTelemetryRef->pEvent3BufferHead;

    if (pTelemetryRef->bEvent3BufferBufferTruncated <= 0 && pTelemetryRef->pEvent3BufferUntruncatedTail == NULL)
    {
       // Buffer is altered, nothing to be done.
        return(pTelemetryRef->pEvent3BufferHead);
    }

    // Replace the delimiter character and restore the buffer tail pointer
    *(pTruncationHead) = ',';
    pTelemetryRef->pEvent3BufferTail = pTelemetryRef->pEvent3BufferUntruncatedTail;

    if (bSendSuccessful > 0)
    {
        // If the send was successful, move all the old truncated things to the start of the buffer and adjust the size accordingly
        ++pTruncationHead;
        while ((*pTruncationHead) != '\0')
        {
            *(pCurrentHead)++ = *pTruncationHead++;
        }

        pTelemetryRef->pEvent3BufferTail = pCurrentHead;
        *pTelemetryRef->pEvent3BufferTail++ = '\0';
    }

    // Switch the flags back to normal
    pTelemetryRef->bEvent3BufferBufferTruncated = 0;
    pTelemetryRef->pEvent3BufferUntruncatedTail = NULL;

    // Return the buffer, created this way in case we want to deep copy only the relevant events to another buffer later on,
    // in which case we will return that instead.
    return(pTelemetryRef->pEvent3BufferHead);
}


/*F********************************************************************************/
/*!
    \Function _TelemetryApiSendData

    \Description
        This sends data to the server reconnecting and adding AUTH data if 
        necessary.

    \Input *pTelemetryRef     - module state
    \Input iMsgType           - the message code

    \Version 1.0 06/27/2006 (tdills) First Version
*/
/********************************************************************************F*/
static int32_t _TelemetryApiSendData(TelemetryApiRefT *pTelemetryRef, int32_t iMsgType)
{
    uint8_t bNeedAuth = FALSE;
#if defined(DIRTYCODE_XENON)
    uint32_t intIP;
    XNADDR XnAddr;
    memset(&XnAddr, 0, sizeof(XnAddr));
#endif
    // if ProtoAries isn't connected
    if (ProtoAriesStatus(pTelemetryRef->pAries, 'stat', NULL, 0) != PROTOARIES_STATE_ONLN || pTelemetryRef->failedLastSend != 0)
    {
        // connect to the server if we aren't already
        ProtoAriesConnect(pTelemetryRef->pAries, pTelemetryRef->strServerName, 0, pTelemetryRef->iPort);

        // we're disconnected so we need to add the AUTH data to the message and reconnect
        if(pTelemetryRef->bAnonymous) 
        {
            TagFieldSetString(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize + pTelemetryRef->uEvent3BufferSize, "ANON", pTelemetryRef->strAuth);
            bNeedAuth = TRUE;
        } 
        else
        {
            TagFieldSetString(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize + pTelemetryRef->uEvent3BufferSize, "AUTH", pTelemetryRef->strAuth);
             bNeedAuth = TRUE;
        }

        // Add the custom IP if necessary
        if (bNeedAuth)
        {
            if (strlen(pTelemetryRef->strCustomIP) > 0)
            {
                TagFieldSetString(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize + pTelemetryRef->uEvent3BufferSize, "IPAD", pTelemetryRef->strCustomIP);
            }
            else if (pTelemetryRef->iTruncateIP > 0)// tcip and ipad cannot both be sent, IPAD superceeds TCIP
            {
                TagFieldSetNumber(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize + pTelemetryRef->uEvent3BufferSize, "TCIP", pTelemetryRef->iTruncateIP);
            }
        }

#if defined(DIRTYCODE_XENON)    
        if (NetConnStatus('xadd', 0, &XnAddr, sizeof(XnAddr)) >= 0)
        {
            memcpy(&intIP, &(XnAddr.inaOnline), sizeof(uint32_t));
            TagFieldSetNumber64(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize, "XADD", intIP);
        }
#endif
    }

    // send the request
    if(ProtoAriesSend(pTelemetryRef->pAries, iMsgType, 0, pTelemetryRef->pTagString, -1) < 0)
    {
        pTelemetryRef->failedLastSend = 1;
        return -1;   
    }
    else
    {
        pTelemetryRef->failedLastSend = 0;
    }

    return(TC_OKAY);
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiTrySendToServer

    \Description
        This is the meat of the sending functionality.  It does all the checks
        to make sure it's OK to send to the telemetry server, packages up all the
        events, and sends them along.

    \Input *pTelemetryRef     - module state

    \Version 01/24/2006 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TelemetryApiTrySendToServer(TelemetryApiRefT *pTelemetryRef)
{
    uint32_t uLoop, uCurrentTime, uCurrentNumOfMessages, uCharsWritten;
    TelemetryApiEventT *pEvent;
    char strTick[9];
    char *pBufTelem3StartPtr = NULL;
    char *pBufPtr;
    char *pSrc;
    char *pTail;
    char oldTail = 0;

    if (pTelemetryRef == NULL)
    {
        return(TELEMETRY3_ERROR_NULLPARAM);
    }
    if ((pTelemetryRef->strAuth[0] == '\0') && (pTelemetryRef->bDisableAuthentication == FALSE))
    {
        NetPrintf(("telemetryapi: authent not set\n"));
        return(TELEMETRY_ERROR_UNKNOWN);
    }

    // get the time
    uCurrentTime = NetTick();
    uCurrentNumOfMessages = _TelemetryApiNumEvents(pTelemetryRef);

    // update the time if there's no data so the first data starts the timer 
    if (uCurrentNumOfMessages == 0 && pTelemetryRef->uNumEvent3Submitted == 0)
    {
        pTelemetryRef->uLastServerSendTime = uCurrentTime;
        return(TC_OKAY);
    }

    // check to make sure we've receieved configuration data
    if ((strlen(pTelemetryRef->strServerName) == 0) || (pTelemetryRef->iPort == 0))
    {
        NetPrintf(("telemetryapi: missing configuration data (server address and port)"));
        return(TELEMETRY_ERROR_UNKNOWN);
    }

    // check to make sure we're allowed to connect
    if ((!pTelemetryRef->bCanConnect) || (pTelemetryRef->pAries == NULL))
    {
        NetPrintf(("telemetryapi: TrySendToServer not allowed to connect; try calling TelemetryApiConnect"));
        return(TELEMETRY_ERROR_NOTCONNECTED);
    }

    // now create the string to send
    pBufPtr = pTelemetryRef->pServerString;
    for(uLoop = 0; uLoop < TELEMETRY_ENTRIESTOSEND_MAX; uLoop++)
    {
        if ((pEvent = _TelemetryApiGetFromFront(pTelemetryRef, uLoop)) == NULL)
        {
            // we're out of entries to send
            break;
        }

        uCharsWritten = snzprintf(
            pBufPtr, 
            TELEMETRY_EVENTSTRING_LENGTH, 
            TELEMETRY_EVENTSTRING_FORMAT,
            pEvent->iTimestamp, 
            (pTelemetryRef->cEntryPrepender == 0) ? TELEMETRY_EVENTSTRING_BUFFERTYPECHARACTER : pTelemetryRef->cEntryPrepender,
            pEvent->uStep,
            TELEMETRY_LocalizerTokenPrintCharArray(pEvent->uModuleID),
            TELEMETRY_LocalizerTokenPrintCharArray(pEvent->uGroupID), 
            (strlen(pEvent->strEvent) == 0) ? TELEMETRY_EVENTSTRING_EMPTY : pEvent->strEvent );

        // increment the pointer we're writing at
        pBufPtr += uCharsWritten;

        // add a delimiter
        *pBufPtr++ = ',';
    }

    // done writing - don't forget to dump the last comma and NULL terminate
    if(pBufPtr > pTelemetryRef->pServerString) 
    {
        *(pBufPtr-1) = '\0';
    } 
    else
    {
        *(pBufPtr) = '\0';    
    }

    // clear the tagfield
    TagFieldClear(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize);

    // Add a tag field for subdirectory
    if(strlen(pTelemetryRef->strSubDir) > 0)
    {
        TagFieldSetString(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize, "SUB", pTelemetryRef->strSubDir);
    }

    // Add a tag field for authentication tick
    ds_snzprintf(strTick,sizeof(strTick),"%08X",pTelemetryRef->iAuthTick);
    TagFieldSetString(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize, "T", strTick);
    
#if DIRTYCODE_DEBUG
     // Add a tag field for the version of DirtySDK
    TagFieldSetNumber64(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize, "V", DIRTYVERS);
#endif

    // encrypt and add telemetry 2 events
    if (pTelemetryRef->bEncrypt > 0)
    {
        memset(pTelemetryRef->pEncryptionBuffer, 0, pTelemetryRef->uEncryptionBufferSize);
        _TelemetryApiObfuscatePayload(pTelemetryRef->pEncryptionBuffer, pTelemetryRef->pServerString, (uint32_t)strlen(pTelemetryRef->pEncryptionBuffer));
        TagFieldSetBinary7(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize, "STR", pTelemetryRef->pEncryptionBuffer, (uint32_t)strlen(pTelemetryRef->pEncryptionBuffer));
    }
    else
    {
        // add telemetry 2 events
        TagFieldSetString(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize, "STR", pTelemetryRef->pServerString);
    }

    // Add telemetry 3 events to the tagfield
    if(pTelemetryRef->uNumEvent3Submitted > 0) 
    {
        oldTail = *(pTelemetryRef->pEvent3BufferTail-1);
        *(pTelemetryRef->pEvent3BufferTail-1) = '\0';  //Null-terminate the event list

        // Set up the tag field
        pBufPtr = pTelemetryRef->pTagString + strlen(pTelemetryRef->pTagString);
        memcpy(pBufPtr,"TLM3=",5);
        pBufPtr += 5;
        pBufTelem3StartPtr = pBufPtr;
        
        // Copy the event buffer into the tag field
        // amakoukji 03-03-2011: new feature: only copy up to 4kb worth of events from the buffer to the tag field
        _TelemetryApiTruncEvents3(pTelemetryRef, TELEMETRY_EVENT_MAX_SIZE);
        pSrc = pTelemetryRef->pEvent3BufferHead;

        if (pTelemetryRef->bEncrypt > 0)
        {
            // Make the source point at the new buffer which will contain the encrypted data
            memset(pTelemetryRef->pEncryptionBuffer, 0, pTelemetryRef->uEncryptionBufferSize);
            pTail = pTelemetryRef->pEncryptionBuffer;

            // Encrypt the TLM3 tag payload and make the source point at the new encrypted string
            pTail += _TelemetryApiObfuscateNPayload(pTelemetryRef->pEncryptionBuffer, pSrc, pTelemetryRef->pEvent3BufferTail - pTelemetryRef->pEvent3BufferHead, (uint32_t)strlen(pTelemetryRef->pEncryptionBuffer));
            pSrc = pTelemetryRef->pEncryptionBuffer;
        }
        else
        {
            pTail = pTelemetryRef->pEvent3BufferTail;
        }

        while(pSrc < pTail)
        {
            *pBufPtr++ = *pSrc++;
        }
        if (*pBufPtr != '\0')
        {
           *pBufPtr = '\0';
        }
    }
    
    #if TELEMETRY_VERBOSE
    NetPrintf(("telemetryapi: sending [%u] telemetry entries to the server.\n", uLoop + pTelemetryRef->uNumEvent3Submitted));
    #endif


    if(_TelemetryApiSendData(pTelemetryRef, '&lgr') == 0)
    {
        // Send successful!
        // Pop telemetry 2 events from queue
        _TelemetryApiRemoveFromFront(pTelemetryRef, uLoop);

        // Reset telemetry 3 pointers
        if(pTelemetryRef->uNumEvent3Submitted > 0) 
        {
            pTelemetryRef->pEvent3BufferTail = pTelemetryRef->pEvent3BufferHead;
            pTelemetryRef->uNumEvent3Submitted = 0;

            _TelemetryApiUntruncEvents3(pTelemetryRef, TRUE); 
        }
    }
    else
    {
        // Send buffer overflow occurred in ProtoAries.
        #if TELEMETRY_VERBOSE
        NetPrintf(("telemetryapi: send failed.\n"));
        #endif

        // Copy telemetry 3 data back to original position    
        if(pTelemetryRef->uNumEvent3Submitted > 0) 
        {
            if (pTelemetryRef->bEncrypt > 0)
            {
                // Decrypt the TLM3 String before moving it back
                _TelemetryApiObfuscatePayload(pBufTelem3StartPtr, pTelemetryRef->pEncryptionBuffer, pTelemetryRef->pEvent3BufferTail - pBufTelem3StartPtr);

                // Move the pBufPtr by the diffrence in length between the encrypted and non encrypted string
                pBufPtr -= (strlen(pTelemetryRef->pEncryptionBuffer) - strlen(pBufTelem3StartPtr)) - 1;
            }

            pSrc = pTelemetryRef->pEvent3BufferTail-1;
            pBufPtr--;
            while(pSrc >= pTelemetryRef->pEvent3BufferHead)
            {
                *(pSrc--) = *(pBufPtr--);
            }

            *(pTelemetryRef->pEvent3BufferTail-1) = oldTail;

            _TelemetryApiUntruncEvents3(pTelemetryRef, FALSE);
        }

        return(TELEMETRY_ERROR_SENDFAILED);
    }

    // set the last send time to the current time
    pTelemetryRef->uLastServerSendTime = uCurrentTime;

    return(TC_OKAY);
}


/*F********************************************************************************/
/*!
    \Function _TelemetryApiGetFilterTokenFromChars

    \Description
        Get a token from a list of four characters.
        Returns (-1) if a NULL is encountered.
        Returns (-2) if an invalid character is encountered. 
            (uses TELEMETRY_TOKEN_VALID macro)
        Returns (-3) if an invalid wildcard is encountered
            (cannot use partial wildcards, like '*BAD' - only complete wildcards
             are acceptable, such as '****')

    \Input *pChars      - character string
    \Input *pDestToken  - destination token slot

    \Output int32_t     - 0=success, error code otherwise

    \Version 1.0 01/06/2006 (jfrank) First Version
    \Version 2.0 04/06/2006 (tdills) Moved to TelemetryApi
*/
/********************************************************************************F*/
static int32_t _TelemetryApiGetFilterTokenFromChars(const char *pChars, uint32_t *pDestToken)
{
    const char *pChar1 = pChars;
    const char *pChar2 = (pChars+1);
    const char *pChar3 = (pChars+2);
    const char *pChar4 = (pChars+3);

    // always set the destination token to 0
    *pDestToken = 0;

    // check for NULL and return error if necessary
    if ((*pChar1 == '\0') || (*pChar2 == '\0') || (*pChar3 == '\0') || (*pChar4 == '\0'))
    {
        NetPrintf(("telemetryapi: filter error on %c%c%c%c; NULL char(s)\n",
                   *pChar1, *pChar2, *pChar3, *pChar4));
        return(-1);
    }

    // only process if there are no invalid chars
    if (TELEMETRY_FILTER_TOKENCHAR_VALID(*pChar1) && TELEMETRY_FILTER_TOKENCHAR_VALID(*pChar2) &&
       TELEMETRY_FILTER_TOKENCHAR_VALID(*pChar3) && TELEMETRY_FILTER_TOKENCHAR_VALID(*pChar4))
    {
        // process just like normal
        *pDestToken = ((uint32_t)(*pChar1) << 24) +
                      ((uint32_t)(*pChar2) << 16) +
                      ((uint32_t)(*pChar3) <<  8) +
                      ((uint32_t)(*pChar4));
        
        // check for invalid (partial) filter wildcards
        // if any of the characters are '*' and the token is not a complete wildcard, fail.
        if ( ((*pChar1 == '*') || (*pChar2 == '*') || (*pChar3 == '*') || (*pChar4 == '*')) 
            && (*pDestToken != '****') )
        {
            NetPrintf(("telemetryapi: filter error on %c%c%c%c; partial wildcards are not supported\n",
                       *pChar1, *pChar2, *pChar3, *pChar4));
            return(-3);
        }

        // otherwise, everything seemingly OK - return success code
        return(0);
    }
    else
    {
        // invalid character encountered - return error
        NetPrintf(("telemetryapi: filter error on %c%c%c%c; invalid char(s)\n",
                   *pChar1, *pChar2, *pChar3, *pChar4));
        return(-2);
    }
}


/*F********************************************************************************/
/*!
    \Function _TelemetryApiSetFilter

    \Description
        Set the filter rules from the filter string.
        For more information about filter rules, 
            see the help under TelemetryApiFilter

    \Input *pTelemetryRef   - module state
    \Input *pFilterRule     - the filter rule to use

    \Output

    \Version 01/12/2006 (jfrank)
*/
/********************************************************************************F*/
static void _TelemetryApiSetFilter(TelemetryApiRefT *pTelemetryRef, const char *pFilterRule)
{
    TelemetryApiFilterRuleT TempRule;
    const char *pRule = pFilterRule;
    uint32_t uToken, uDone=0;

    // define parsing states for the rules
    enum TelemetryApiFilterParseStateE
    {
        TELEMETRY_FILTERPARSE_IDLE = 0,
        TELEMETRY_FILTERPARSE_PLUSMINUS,
        TELEMETRY_FILTERPARSE_MODULEID,
        TELEMETRY_FILTERPARSE_SLASH,
        TELEMETRY_FILTERPARSE_GROUPID,
        TELEMETRY_FILTERPARSE_BUFFERFULL,
        TELEMETRY_FILTERPARSE_STRINGID,
        TELEMETRY_FILTERPARSE_ATTRIBUTES,
        TELEMETRY_FILTERPARSE_DONE_RULE
    };
    enum TelemetryApiFilterParseStateE eState = TELEMETRY_FILTERPARSE_IDLE;

    // check for errors
    if (pTelemetryRef == NULL)
    {
        return;
    }

    // see if we just want to wipe out the filter rules
    if ((pFilterRule == NULL) || (strlen(pFilterRule) == 0))
    {
        pTelemetryRef->uNumFilters = 0;
        memset(pTelemetryRef->Filters, 0, sizeof(pTelemetryRef->Filters));
    }

    // otherwise, process the filters starting at zero.
    pTelemetryRef->uNumFilters = 0;
    while((*pRule != '\0') && (!uDone))
    {
        switch(eState)
        {
        case TELEMETRY_FILTERPARSE_IDLE:
            // keep moving until we find a plus or minus
            while(*pRule != '\0')
            {
                if ((*pRule == '+') || (*pRule == '-'))
                {
                    // check to make sure we haven't run out of rule space
                    if (pTelemetryRef->uNumFilters >= TELEMETRY_FILTERRULES_MAX)
                    {
                        // we do this check here because we want to make sure
                        // there aren't any actual rules left - not just
                        // empty space, etc. at the end of the rule
                        eState = TELEMETRY_FILTERPARSE_BUFFERFULL;
                    }
                    // otherwise, continue processing
                    else
                    {
                        memset(&TempRule, 0, sizeof(TempRule));
                        eState = TELEMETRY_FILTERPARSE_PLUSMINUS;
                    }
                    break;
                }
                pRule++;
            }
            break;
        case TELEMETRY_FILTERPARSE_PLUSMINUS:
            // we've found a plus or minus.
            TempRule.iEnable = (*pRule == '+') ? 1 : -1;
            // now loop until we find a valid char (or the end of the string)
            while(*pRule != '\0')
            {
                if (TELEMETRY_FILTER_TOKENCHAR_VALID(*pRule))
                {
                    eState = TELEMETRY_FILTERPARSE_MODULEID;
                    break;
                }
                pRule++;
            }
            break;
        case TELEMETRY_FILTERPARSE_MODULEID:
            // suck in the moduleID
            if (_TelemetryApiGetFilterTokenFromChars(pRule, &uToken) == 0)
            {
                // if we got a valid token, take this rule.  
                TempRule.uModuleID = uToken;
                pRule+=4;

                // find a slash
                while(*pRule != '\0')
                {
                    if ((*pRule == '/') || (*pRule == '\\'))
                    {
                        eState = TELEMETRY_FILTERPARSE_SLASH;
                        break;
                    }
                    pRule++;
                }
            }
            // otherwise wait for the next valid rule
            else
            {
                eState = TELEMETRY_FILTERPARSE_IDLE;
            }
            break;
        case TELEMETRY_FILTERPARSE_SLASH:
            // we've found a slash.
            // now loop until we find a valid char (or the end of the string)
            while(*pRule != '\0')
            {
                if (TELEMETRY_FILTER_TOKENCHAR_VALID(*pRule))
                {
                    eState = TELEMETRY_FILTERPARSE_GROUPID;
                    break;
                }
                pRule++;
            }
            break;
        case TELEMETRY_FILTERPARSE_GROUPID:
            // suck in the groupID
            if (_TelemetryApiGetFilterTokenFromChars(pRule, &uToken) == 0)
            {
                // if we got a valid token, take this rule.  
                TempRule.uGroupID = uToken;
                pRule+=4;

                // if a slash immediately follows, parse the next token as StringID
                if ((*pRule == '/') || *pRule == '\\')
                {
                    pRule++;
                    eState = TELEMETRY_FILTERPARSE_STRINGID;
                    break;
                }
                else
                {
                    // No StringID in filter rule - use wildcard
                    TempRule.uStringID = TELEMETRY_FILTERRULES_WILDCARD;
    
                    // copy the TempRule to the permanent rules
                    memcpy(&(pTelemetryRef->Filters[pTelemetryRef->uNumFilters]), &TempRule, sizeof(TempRule));
                    pTelemetryRef->uNumFilters++;

                    eState = TELEMETRY_FILTERPARSE_DONE_RULE;
                }
            }
            // otherwise wait for the next valid rule
            else
            {
                eState = TELEMETRY_FILTERPARSE_IDLE;
            }
            break;
        case TELEMETRY_FILTERPARSE_STRINGID:
            // suck in the stringID
            if (_TelemetryApiGetFilterTokenFromChars(pRule, &uToken) == 0)
            {
                // if we got a valid token, take this rule.  
                TempRule.uStringID = uToken;
                pRule+=4;

                // copy the TempRule to the permanent rules
                memcpy(&(pTelemetryRef->Filters[pTelemetryRef->uNumFilters]), &TempRule, sizeof(TempRule));

                // if a slash immediately follows, parse the next token as the Attributes String
                if ((*pRule == '/') || *pRule == '\\')
                    eState = TELEMETRY_FILTERPARSE_ATTRIBUTES;
                else
                {
                    pTelemetryRef->uNumFilters++;
                    eState = TELEMETRY_FILTERPARSE_DONE_RULE;
                }
            }
            // otherwise wait for the next valid rule
            else
            {
                eState = TELEMETRY_FILTERPARSE_IDLE;
            }
            break;

        case TELEMETRY_FILTERPARSE_ATTRIBUTES:

            // Get a pointer to the attribute string in the permanent rules.
            ++pRule;
            if (*pRule == '&')
            {
                // Although the string in pRule does not change, we do not technically "own" the memory it lives in, so copy it, just to be safe
                ds_strnzcpy(pTelemetryRef->Filters[pTelemetryRef->uNumFilters].uStrEvent, pRule, TELEMETRY3_EVENTSTRINGSIZE_DEFAULT);
            }

            pTelemetryRef->uNumFilters++;
            eState = TELEMETRY_FILTERPARSE_DONE_RULE;
            break;
        case TELEMETRY_FILTERPARSE_DONE_RULE:

            // find a possible next entry (must be separated by a comma)
            while(*pRule != '\0')
            {
                if (*pRule == ',')
                {
                        // move past the comma and start over
                     pRule++;
                     eState = TELEMETRY_FILTERPARSE_IDLE;
                     break;
                }
                pRule++;
             }
            break;

        case TELEMETRY_FILTERPARSE_BUFFERFULL:
            // the filter buffer is full
            NetPrintf(("telemetryapi: too many server rules; remaining rules dropped: [%s]\n", pRule));
            uDone=1;
            break;
        }
    }

    // done
    return;
}


/*F********************************************************************************/
/*!
    \Function _TelemetryApiEventBufferApplyFilter

    \Description
        Apply the current filter rules 
          to all of the entries in the event buffer

    \Input *pTelemetryRef  - module state

    \Version 01/11/2006 (jfrank)
*/
/********************************************************************************F*/
static void _TelemetryApiEventBufferApplyFilter(TelemetryApiRefT *pTelemetryRef)
{
    TelemetryApiEventT *pEvent;
    uint32_t uLoop, uEntry, uCopyToSlot, bKeptAnEntry = 0, bKilledAnEntry = 0;

    // first loop through all the entries and wipe out the ones which don't apply
    uCopyToSlot = pTelemetryRef->uEventBufSize;
    uEntry      = pTelemetryRef->uEventHead;
    for(uLoop = 0; uLoop < pTelemetryRef->uEventBufSize; uLoop++)
    {
        pEvent = &(pTelemetryRef->Events[uEntry]);

        if (_TelemetryApiFilterEvent(pTelemetryRef, pEvent->uModuleID, pEvent->uGroupID) == 0)
        {
            // wipe out this event
            if (uCopyToSlot == pTelemetryRef->uEventBufSize)
                uCopyToSlot = uEntry;
            bKilledAnEntry = 1;
                // this makes it easier to see the wiped out event
                //   but it is not a necessary step, so it's only
                //   enabled in debug mode
                memset(pEvent, 0, sizeof(*pEvent));
        }
        else
        {
            // the event is OK, but we might need to move it
            if (uCopyToSlot != pTelemetryRef->uEventBufSize)
            {
                memcpy(&(pTelemetryRef->Events[uCopyToSlot]), pEvent, sizeof(TelemetryApiEventT));
                uCopyToSlot++;
                if (uCopyToSlot == pTelemetryRef->uEventBufSize)
                    uCopyToSlot = 0;

                    // this makes it easier to see the wiped out event
                    //   but it is not a necessary step, so it's only
                    //   enabled in debug mode
                    memset(pEvent, 0, sizeof(*pEvent));
            }
            bKeptAnEntry = 1;
        }

        // and determine if we are done
        if (uEntry == pTelemetryRef->uEventTail)
        {
            // done
            break;
        }
        else
        {
            //continue processing
            uEntry++;
            // handle the wrap-around case
            if (uEntry == pTelemetryRef->uEventBufSize)    
                uEntry = 0;
        }
    }

    // the head pointer hasn't changed, but the tail pointer might have
    if (bKilledAnEntry)
    {
        // if we killed an entry, we're definitely not full
        pTelemetryRef->uEventState &= (~TELEMETRY_EVENTBUFFERSTATE_FULL);

        // and the tail pointer has changed
        //  (to the entry previous to the "copy to slot")
        if (uCopyToSlot == 0)
            uCopyToSlot = pTelemetryRef->uEventBufSize;
        uCopyToSlot--;
        pTelemetryRef->uEventTail = uCopyToSlot;
    }

    // check for empty
    if (bKeptAnEntry == 0)
    {
        pTelemetryRef->uEventTail = pTelemetryRef->uEventHead = 0;
        pTelemetryRef->uEventState |= TELEMETRY_EVENTBUFFERSTATE_EMPTY;
        _TelemetryApiTriggerCallback(pTelemetryRef, TELEMETRY_CBTYPE_BUFFEREMPTY);
    }
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiSetCountryDisabledFlag

    \Description
        Compare the user's locale with the list of disabled locales to determine
            if the current user's logging should be disabled.

    \Input *pTelemetryRef  - module state

    \Version 04/10/2006 (tdills)
*/
/********************************************************************************F*/
static void _TelemetryApiSetCountryDisabledFlag(TelemetryApiRefT *pTelemetryRef)
{
    char *pCountry = pTelemetryRef->strDisabledCountries;
    char strCountry[3];
    int32_t uLocale;
   

    if (strlen(pTelemetryRef->strUserLocale) == 0)
    {
        // If the local has not been set, try to secondary check.
        uLocale = NetConnStatus('locl', 0, NULL, 0); // Reliable setting, through 1st party APIs from XBOX, PS3, unreliable setting from Windows
        if (uLocale > 0)
        {
            (pTelemetryRef->strUserLocale)[0] = (char)(((uLocale) >> 8) & 0xFF); 
            (pTelemetryRef->strUserLocale)[1] = (char)((uLocale) & 0xFF); 
            (pTelemetryRef->strUserLocale)[2] = '\0';
        }          
    }

    if (strlen(pTelemetryRef->strUserLocation) == 0)
    {
        uLocale = NetConnStatus('locn', 0, NULL, 0);  // Hardware setting, unreliable fallback
        if(uLocale == 'zzZZ')
        {
            uLocale = 0;
        }
        if (uLocale > 0)
        {
            (pTelemetryRef->strUserLocation)[0] = (char)(((uLocale) >> 8) & 0xFF); 
            (pTelemetryRef->strUserLocation)[1] = (char)((uLocale) & 0xFF); 
            (pTelemetryRef->strUserLocation)[2] = '\0';
        } 
    }

    if (strlen(pTelemetryRef->strUserLocale) == 0 && strlen(pTelemetryRef->strUserLocation) == 0)
    {
        // user locale hasn't been set and cannot be verified so there is nothing to check.
        // Default behavior is to send telemetry in this case for piracy reasons.
        NetPrintf(("telemetryapi: user locale has not been set, and cannot be auto-detected.\n"));
        pTelemetryRef->bCountryDisabled = FALSE;
        return;
    }

    // Check if the locale is in the disabled countries list
    if (strlen(pTelemetryRef->strUserLocale) != 0)
    {
        while ((pCountry != NULL) && (*pCountry != 0))
        {
            // get the next country
            TagFieldGetString(pCountry, strCountry, sizeof(strCountry), "");
            // compare to our country
            if (ds_strnicmp(pTelemetryRef->strUserLocale, strCountry, 2) == 0)
            {
                NetPrintf(("telemetryapi: user locale found in list of disabled countries; telemetry is disabled.\n"));
                pTelemetryRef->bCountryDisabled = TRUE;
                return;
            }
            // move to the next country in the list
            pCountry += 3;
        }

    }
   
    // Check again for location
    if (strlen(pTelemetryRef->strUserLocation) != 0)
    {
        pCountry = pTelemetryRef->strDisabledCountries;
        while ((pCountry != NULL) && (*pCountry != 0))
        {
            // get the next country
            TagFieldGetString(pCountry, strCountry, sizeof(strCountry), "");
            // compare to our country
            if (ds_strnicmp(pTelemetryRef->strUserLocation, strCountry, 2) == 0)
            {
                NetPrintf(("telemetryapi: user locale found in list of disabled countries; telemetry is disabled.\n"));
                pTelemetryRef->bCountryDisabled = TRUE;
                return;
            }
            // move to the next country in the list
            pCountry += 3;
        }
    }
   

    // if we made it through the list without returning then set to false.
    NetPrintf(("telemetryapi: user locale is not restricted.\n"));
    pTelemetryRef->bCountryDisabled = FALSE;
    return;
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiCanSendTelemetry

    \Description
        Look at the 'halt' control the disabled countries set with 'cdlb' and the
            user enable flag set with 'unlb' to determine whether or not we
            can send telemetry to the telemetry server.

    \Input *pTelemetryRef  - module state

    \Output int32_t - 0 = we cannot send; 1 = we can send

    \Version 1.0 04/10/2006 (tdills) Moved to TelemetryApi
*/
/********************************************************************************F*/
static int32_t _TelemetryApiCanSendTelemetry(TelemetryApiRefT *pTelemetryRef)
{
    if ((pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_SENDHALTED) ||
        (pTelemetryRef->bCountryDisabled == TRUE) ||
        (pTelemetryRef->bUserDisabled == TRUE))
    {
        return(0);
    }
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiGetMessage

    \Description
        Get message data from ProtoAries if we have a ProtoAries reference.

    \Input *pTelemetryRef  - module state
    \Input *pKind - [out] the message kind from ProtoAries
    \Input *pCode - [out] the message code from ProtoAries
    \Input **pData - [out] the message data from ProtoAries

    \Output int32_t - -1 = if there is an error or no proto aries connect; the return
                           code from ProtoAriesPeek otherwise.

    \Version 1.0 05/09/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _TelemetryApiGetMessage(TelemetryApiRefT *pTelemetryRef, int32_t *pKind, int32_t *pCode, char **pData)
{
    int32_t iRetVal;
    
    if (pTelemetryRef->pAries == NULL)
    {
        return(-1);
    }

    iRetVal = ProtoAriesPeek(pTelemetryRef->pAries, pKind, pCode, pData);

    if (iRetVal >= 0)
    {
        ProtoAriesRecv(pTelemetryRef->pAries, NULL, NULL, NULL, 0);
    }

    return(iRetVal);
}

/*** Public Functions ******************************************************************/

/*F*************************************************************************************************/
/*!
    \Function TelemetryApiCreate

    \Description
        Creates a new instance of the telemetry module

    \Input  uNumEvents  - number of events to use   (0=use default)
    \Input  eBufferType - type of buffer to use

    \Output TelemetryApiRefT * - state pointer (pass to other functions) (NULL=failed)

    \Version 1.0 04/06/2006 (tdills) First Version
    \Version 2.0 04/06/2006 (geoffreyh) Second Version - wraps call to TelemetryApiCreateEx
*/
/*************************************************************************************************F*/
TelemetryApiRefT *TelemetryApiCreate(uint32_t uNumEvents, TelemetryApiBufferTypeE eBufferType)
{
    return(TelemetryApiCreateEx(uNumEvents,eBufferType,0));
}

/*F*************************************************************************************************/
/*!
    \Function TelemetryApiCreateEx

    \Description
        Creates a new instance of the telemetry module, allocating space for telemetry 3 events

    \Input  uNumEvents  - number of events to use   (0=use default)
    \Input  eBufferType - type of buffer to use
    \Input  uEvent3BufSize - size of buffer for telemetry 3 events

    \Output TelemetryApiRefT * - state pointer (pass to other functions) (NULL=failed)

    \Version 1.0 04/06/2006 (tdills) First Version
*/
/*************************************************************************************************F*/
TelemetryApiRefT *TelemetryApiCreateEx(uint32_t uNumEvents, TelemetryApiBufferTypeE eBufferType, uint32_t uEvent3BufSize)
{
    TelemetryApiRefT *pTelemetryRef;

    uint32_t uMemAllocSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    uint32_t uEvent3ReserveSize = 0;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);    

    #ifdef TELEMETRY_VERBOSE
    NetPrintf(("telemetryapi: create %d events buffertype %d\n", uNumEvents, eBufferType));
    #endif

    // calculate the size for the contiguous block of memory to allocate, and allocate
    uNumEvents = ((uNumEvents==0) ? TELEMETRY_NUMEVENTS_DEFAULT : uNumEvents);

    // calculate size for telemetry 3 buffer
    // must be at least enough to reserve space for tag and enclosing quotes
    if(uEvent3BufSize <= TELEMETRY3_RESERVE_SIZE)
    {
        uEvent3BufSize = 0;
    } 
    else
    {
        uEvent3ReserveSize = TELEMETRY3_RESERVE_SIZE;
    }

    uMemAllocSize = sizeof(TelemetryApiRefT);                    //!< for the TelemetryApiRefT control structure
    uMemAllocSize += (uNumEvents * sizeof(TelemetryApiEventT));  //!< for the event buffer
    uMemAllocSize += TELEMETRY_SENDBUFFERSIZE; //!< for the string used to format data
    uMemAllocSize += TELEMETRY_TAG_FIELD_SIZE; //!< for the tagfield string used for comm
    uMemAllocSize += uEvent3BufSize;                          //!< shared by queued events and the tagfield

    pTelemetryRef = DirtyMemAlloc(uMemAllocSize, TELEMETRYAPI_MEMID, iMemGroup, pMemGroupUserData);

    if (pTelemetryRef != NULL)
    {
        memset(pTelemetryRef, 0, uMemAllocSize);
        pTelemetryRef->iMemGroup         = iMemGroup;
        pTelemetryRef->pMemGroupUserData = pMemGroupUserData;

        pTelemetryRef->pServerString     = ((char *)(pTelemetryRef) + sizeof(TelemetryApiRefT));
        pTelemetryRef->uServerStringSize = TELEMETRY_SENDBUFFERSIZE;

        pTelemetryRef->pTagString        = (char *)(pTelemetryRef->pServerString + pTelemetryRef->uServerStringSize);
        pTelemetryRef->uTagStringSize    = TELEMETRY_TAG_FIELD_SIZE;

        pTelemetryRef->pEvent3BufferHead = (char *)(pTelemetryRef->pTagString + pTelemetryRef->uTagStringSize + uEvent3ReserveSize);
        pTelemetryRef->pEvent3BufferTail = pTelemetryRef->pEvent3BufferHead;
        pTelemetryRef->uEvent3BufferSize = uEvent3BufSize;

        pTelemetryRef->Events            = (TelemetryApiEventT *)(pTelemetryRef->pTagString + pTelemetryRef->uTagStringSize + pTelemetryRef->uEvent3BufferSize);
        pTelemetryRef->uEventBufSize     = uNumEvents;

        pTelemetryRef->eBufferType       = eBufferType;
        pTelemetryRef->uEventState       = TELEMETRY_EVENTBUFFERSTATE_EMPTY | TELEMETRY_EVENTBUFFERSTATE_SENDHALTED;
        pTelemetryRef->uEventTimeout     = TELEMETRY_SENDBUFFER_EVENTTIMEOUT_DEFAULT;
        pTelemetryRef->uEventThreshold   = (uNumEvents * TELEMETRY_SENDBUFFER_EVENTTHRESHOLDPCT_DEFAULT) / 100;

        // register the time we created the API handle - all events are
        // logged as a offset of this time. 
        pTelemetryRef->iTickStartTime = (signed)(NetTick()/1000);

        // Encryption buffer starts NULL, it will be allocated if it needs to be
        pTelemetryRef->pEncryptionBuffer = NULL;
        pTelemetryRef->uEncryptionBufferSize = 0;

        // Set the retry count to 1
        pTelemetryRef->uSubmit3MaxRetry = 1;
        
        pTelemetryRef->strCustomIP[0] = '\0';
        pTelemetryRef->iTruncateIP = 0;
    }

    // return reference pointer
    return(pTelemetryRef);
}


/*F*************************************************************************************************/
/*!
    \Function TelemetryApiDestroy

    \Description
        Destroy an existing client

    \Input *pTelemetryRef - module reference pointer

    \Version 04/06/2006 (tdills)
*/
/*************************************************************************************************F*/
void TelemetryApiDestroy(TelemetryApiRefT *pTelemetryRef)
{
    if(pTelemetryRef == NULL) {
        return;
    }

    #ifdef TELEMETRY_VERBOSE
    NetPrintf(("telemetryapi: destroy [0x%08x]\n", pTelemetryRef));
    #endif

    // shutdown the ProtoAries connection if we haven't already
    if (pTelemetryRef->pAries != NULL)
    {
        // we haven't disconnected!  do that now.
        NetPrintf(("telemetryapi: warning - destroy called before disconnecting; calling disconnect automatically.\n"));
        TelemetryApiDisconnect(pTelemetryRef);
    }

    // done with memory
    if (pTelemetryRef->pEncryptionBuffer != NULL)
    {
        DirtyMemFree(pTelemetryRef->pEncryptionBuffer, TELEMETRYAPI_MEMID, pTelemetryRef->iMemGroup, pTelemetryRef->pMemGroupUserData);
        pTelemetryRef->pEncryptionBuffer = NULL;
    }
    
    DirtyMemFree(pTelemetryRef, TELEMETRYAPI_MEMID, pTelemetryRef->iMemGroup, pTelemetryRef->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function TelemetryApiDisconnect

    \Description
        Disconnect from the telemetry server

    \Input *pTelemetryRef       - module reference pointer

    \Version 04/06/2006 (tdills)
*/
/*************************************************************************************************F*/
void TelemetryApiDisconnect(TelemetryApiRefT *pTelemetryRef)
{
    if (pTelemetryRef == NULL)
    {
        return;
    }

    #ifdef TELEMETRY_VERBOSE
    NetPrintf(("telemetryapi: disconnect [0x%08x]\n", pTelemetryRef));
    #endif

    // force an upload of data from the buffer (flush it to the server)
    pTelemetryRef->uLastServerSendTime = 0;
    TelemetryApiUpdate(pTelemetryRef);

    // flag that we are no longer allowed to connect
    pTelemetryRef->bCanConnect = FALSE;

    if (pTelemetryRef->pAries)
    {
        // Disconnect socket
        ProtoAriesUnconnect(pTelemetryRef->pAries);

        // done with protocol
        ProtoAriesDestroy(pTelemetryRef->pAries);
        pTelemetryRef->pAries = NULL;
    }
}

/*F*************************************************************************************************/
/*!
    \Function TelemetryApiAuthent

    \Description
        Setup request to authenticate user with server

    \Input *pTelemetryRef   - reference pointer
    \Input *pAuth           - authorization string

    \Output
        int32_t             - EC_code

    \Version 02/12/2003 (GWS)
*/
/*************************************************************************************************F*/
int32_t TelemetryApiAuthent(TelemetryApiRefT *pTelemetryRef, const char *pAuth)
{
    const char *pDelimiter;
    char *pEnd;

    if (pTelemetryRef == NULL || pAuth == NULL)
    {
        return(TELEMETRY3_ERROR_NULLPARAM );
    }

    #ifdef TELEMETRY_VERBOSE
    NetPrintf(("telemetryapi: authent [0x%08x] (%s)\n", pTelemetryRef, pAuth));
    #endif

    // extract address
    pDelimiter = strchr(pAuth, ',');

    if (pDelimiter == NULL)
    {
        NetPrintf(("telemetryapi: authent data does not contain IP,port delimiter\n"));
        return(TC_MISSING_PARAM);
    }

    // Copy the auth string, and extract the IP/DNS address
    ds_strnzcpy(pTelemetryRef->strServerName, pAuth, pDelimiter - pAuth + 1);

    if(strlen(pTelemetryRef->strServerName) == 0)
    {
        NetPrintf(("telemetryapi: destination IP was not provided.\n"));
        return(TELEMETRY_ERROR_UNKNOWN); 
    }

    // extract port
    pTelemetryRef->iPort = TagFieldGetNumber(pDelimiter+1, 0);

    // extract the user's locality
    pDelimiter = strchr(pDelimiter+1, ',');
    if (pDelimiter == NULL)
    {
        NetPrintf(("telemetryapi: authent data does not contain user locale\n"));
        return(TC_MISSING_PARAM);
    }
    else
    {
        // just get the 3rd and 4th characters of the string as this contains the country code
        TagFieldGetString(pDelimiter+3, pTelemetryRef->strUserLocale, sizeof(pTelemetryRef->strUserLocale), "");
        // end the locale string at the right spot.
        pEnd = strchr(pTelemetryRef->strUserLocale,',');
        if (pEnd != NULL)
        {
            *pEnd = 0;
        }
        // set the country disabled flag appropriately
        _TelemetryApiSetCountryDisabledFlag(pTelemetryRef);
    }

    // relative tick value when authentication occurred
    pTelemetryRef->iAuthTick = _TelemetryApiRelativeTick(pTelemetryRef);

    // save authent
    pDelimiter = strchr(pDelimiter+1, ',');
    if (pDelimiter == NULL)
    {
        NetPrintf(("telemetryapi: authent data does not contain auth string\n"));
        return(TC_MISSING_PARAM);
    }
    else
    {
        if(strlen(pDelimiter+1) > TELEMETRY_AUTHKEY_LEN) {
            NetPrintf(("telemetryapi: authent string is longer than %d and will be truncated \n", TELEMETRY_AUTHKEY_LEN));
            return(TC_INV_PARAM);
        }

        TagFieldGetString(pDelimiter+1, pTelemetryRef->strAuth, sizeof(pTelemetryRef->strAuth), "");
    }

    // return local result
    return(TC_OKAY);
}

/*F********************************************************************************/
/*!
    \Function TelemetryApiAuthentAnonymous

    \Description
        Set up the AUTH tag field for anonymous telemetry.  
        Can be used in offline game modes where user information is unavailable.

    \Input  *pTelemetryRef  - Telemetry API handle
    \Input  pDestIP         - Telemetry server IP address or DNS address
    \Input  iDestPort       - Telemetry server port
    \Input  uLocale         - The 4-character locale, eg. 'enUS'
    \Input  *pDomain        - Telemetry log directory (convention is "PLATFORM/TITLE")
    \Input  uTime           - The current UTC time (in seconds)
    \Input  *pPlayerID      - Anonymous player ID (optional, set to NULL if not required)

    \Output int32_t         - Returns TC_OKAY for sucess.  Any other value is an error.

    \Notes
        Passing a DNS address instead of an IP address implies the extra cost of a DNS lookup when this function is executed.

    \Version 04/21/2010 (geoffreyh)
*/
/********************************************************************************F*/
int32_t TelemetryApiAuthentAnonymous(TelemetryApiRefT *pTelemetryRef, const char *pDestIP, int32_t iDestPort, uint32_t uLocale,
                                     const char *pDomain, uint32_t uTime, const char *pPlayerID)
{
    return TelemetryApiAuthentAnonymousEx(pTelemetryRef, pDestIP, iDestPort, uLocale, pDomain, uTime, pPlayerID, NULL, 0);
}

/*F********************************************************************************/
/*!
    \Function TelemetryApiAuthentAnonymousEx

    \Description
        Set up the AUTH tag field for anonymous telemetry.  
        Can be used in offline game modes where user information is unavailable.
        This call is similar to TelemetryApiAuthentAnonymous but it permits the client to provide a MAC address and a Nucleus Persona ID.
        There is no guarantee the client is who they say they are, so there is a HUGE disclaimer on data collected using this method.

    \Input  *pTelemetryRef      - Telemetry API handle
    \Input  pDestIP             - Telemetry server IP address or DNS address
    \Input  iDestPort           - Telemetry server port
    \Input  uLocale             - The 4-character locale, eg. 'enUS'
    \Input  *pDomain            - Telemetry log directory (convention is "PLATFORM/TITLE")
    \Input  uTime               - The current UTC time (in seconds)
    \Input  *pPlayerID,         - Optional gamertag or user id hash
    \Input  *pMAC,              - Optional MAC address or machine hash 
    \Input  uNucleusPersonaID   - Optional Nucleus Persona ID

    \Output
        int32_t                 - Returns TC_OKAY for sucess.  Any other value is an error.

    \Notes
        Passing a DNS address instead of an IP address implies the extra cost of a DNS lookup when this function is executed.

    \Version 08/13/2010 (geoffreyh)
*/
/********************************************************************************F*/
int32_t TelemetryApiAuthentAnonymousEx(TelemetryApiRefT *pTelemetryRef, const char *pDestIP, int32_t iDestPort, uint32_t uLocale,
                                     const char *pDomain, uint32_t uTime, const char *pPlayerID, const char *pMAC, int64_t uNucleusPersonaID)
{
    char strPlayerID[64], strMAC[64], strNucleusPersonaID[32], strTmp[TELEMETRY_AUTHKEY_LEN], strDomain[TELEMETRY_MAXIMUM_ANON_DOMAIN_LEN];

    if(pDestIP == NULL || pDomain == NULL || pTelemetryRef == NULL)
    {
        return(TC_MISSING_PARAM);
    }

    if ((uint32_t)strlen(pDomain) > TELEMETRY_MAXIMUM_ANON_DOMAIN_LEN)
    {
       NetPrintf(("telemetryapi: domain is too long, value truncated to %d characters.", TELEMETRY_MAXIMUM_ANON_DOMAIN_LEN));
    }

    if(strlen(pDomain) > 0)
    {
        ds_strnzcpy(strDomain, pDomain, sizeof(strDomain));
    }
    

    if(pPlayerID != NULL && strlen(pPlayerID) > 0)
    {
        ds_snzprintf2(strPlayerID,sizeof(strPlayerID),pPlayerID);
    }
    else
    {
        ds_snzprintf2(strPlayerID,sizeof(strPlayerID),"xxxx");
    }

    if(pMAC != NULL && strlen(pMAC) > 0)
    {
        ds_snzprintf2(strMAC,sizeof(strMAC),pMAC);
    }
    else
    {
        ds_snzprintf2(strMAC,sizeof(strMAC),"xxxx");
    }

    if(uNucleusPersonaID != 0)
    {
        ds_snzprintf(strNucleusPersonaID,sizeof(strNucleusPersonaID),"%lld",uNucleusPersonaID);
    }
    else
    {
        ds_snzprintf(strNucleusPersonaID,sizeof(strNucleusPersonaID),"xxxx");
        ds_snzprintf(pTelemetryRef->strSessionID,sizeof(pTelemetryRef->strSessionID),"%x%s",uTime,pPlayerID);
    }

    if(uLocale == 0)
    {
        // set the country disabled flag appropriately
        _TelemetryApiSetCountryDisabledFlag(pTelemetryRef);
        uLocale = 'xxxx';
    }
    else
    {
        // just get the 3rd and 4th characters of the string as this contains the country code
        pTelemetryRef->strUserLocale[0] = (char)(((uLocale)>>8)&0xFF);
        pTelemetryRef->strUserLocale[1] = (char)((uLocale)&0xFF);
        pTelemetryRef->strUserLocale[2] = '\0';

        // set the country disabled flag appropriately
        _TelemetryApiSetCountryDisabledFlag(pTelemetryRef);
    }

    // Scrub the strings for invalid characters, replacing with '_'
    _TelemetryApiScrubString(strMAC);
    _TelemetryApiScrubString(strPlayerID);

    // extract the address
    ds_strnzcpy(pTelemetryRef->strServerName, pDestIP, sizeof(pTelemetryRef->strServerName));
    pTelemetryRef->iPort = iDestPort;

    if(strlen(pTelemetryRef->strServerName) == 0)
    {
        NetPrintf(("telemetryapi: destination IP was not provided."));
        return(TELEMETRY_ERROR_UNKNOWN);
    }
    
    // relative tick when authentication occurred
    pTelemetryRef->iAuthTick = _TelemetryApiRelativeTick(pTelemetryRef);

    #ifdef TELEMETRY_VERBOSE
    NetPrintf(("telemetryapi: authent [0x%08x] (%s,%d,%c%c%c%c,%s:%s:%u:%s:%s)\n", pTelemetryRef,
        pDestIP,iDestPort,TELEMETRY_LocalizerTokenPrintCharArray(uLocale),strDomain,strPlayerID,uTime,strMAC,strNucleusPersonaID));
    #endif

    // Write out colon-delimited fields
    ds_snzprintf(strTmp,sizeof(strTmp),"%s:%s:%u:%s:%s:%c%c%c%c",strDomain,strPlayerID,uTime,
        strMAC,strNucleusPersonaID,TELEMETRY_LocalizerTokenPrintCharArray(uLocale));

    // Generate an MD5 checksum
    _TelemetryApiObfuscateAuth(pTelemetryRef->strAuth,strTmp);

    // switch to anonymous mode
    TelemetryApiControl(pTelemetryRef,'anon',1,0);

    // return local result
    return(TC_OKAY);
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiObfuscateAuth

    \Description
        Generate a checksum for the authentication string.
        This is weak since the client knows the encrypt secret (different than the standard Blaze encyrpt secret), but better than nothing.

    \Input  *pDst   - Destination string buffer
    \Input  pSrc    - Source string buffer

    \Output int32_t - Returns TC_OKAY for sucess.  Any other value is an error.

    \Version 08/13/2010 (geoffreyh)
*/
/********************************************************************************F*/
void _TelemetryApiObfuscateAuth(char *pDst, char *pSrc)
{
    CryptMD5T md5;
    char strTmp[TELEMETRY_AUTHKEY_LEN];
    int32_t iTmpLen;

    CryptMD5Init(&md5);
    CryptMD5Update(&md5, TELEMETRY_ANONAUTH_SECRET, -1);
    CryptMD5Update(&md5, pSrc, -1);
    CryptMD5Final(&md5, strTmp, MD5_BINARY_OUT);

    iTmpLen = TagFieldPrintf(strTmp + MD5_BINARY_OUT,
            sizeof(strTmp) - MD5_BINARY_OUT, "%s", pSrc);

    pDst[0] = '$';
    TagFieldSetBinary7(pDst+1, TELEMETRY_AUTHKEY_LEN-1, NULL, strTmp, MD5_BINARY_OUT + iTmpLen + 1);
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiObfuscateNPayload

    \Description
        XOR the bits of the payload with the SECRET, which is repeated as necessary.
        This is weak since the client knows the encrypt secret (different than the standard Blaze encyrpt secret), but better than nothing.
        Also this will generate an output of the same length, simplifying payload manipulations.
        Sending an encrypted string to this method will decrypt it.

    \Input  *pDst   - Destination string buffer
    \Input  pSrc    - Source string buffer
    \Input  len        - The amount of characters to encrypt, ignored when decrypting
    \Input  pDstLen - Size of destination, not to be exceeded

    \Output int32_t - The length of the output buffer in pDst

    \Version 01/11/2011 (amakoukji)
*/
/********************************************************************************F*/
uint32_t _TelemetryApiObfuscateNPayload(char *pDst, char *pSrc, uint32_t len, uint32_t pDstLen)
{
    uint32_t i = 0, payloadSizeLen;
    uint32_t payloadLen;
    const uint32_t secretLen = (uint32_t)strlen(TELEMETRY_ANONAUTH_SECRET);
    char strSecret[256];
    unsigned char* pTmp; // a temporary pointer to a string buffer

    pDst[0] = 0;
    strncpy(strSecret, TELEMETRY_ANONAUTH_SECRET, sizeof(TELEMETRY_ANONAUTH_SECRET));

    // By convention if the first character of pSrc is a '@', the string is encrypted already and we wish to decrypt
    if (pSrc[0] == '@')
    {
        // DECRYPT
        pTmp = (unsigned char*)pSrc;

        // Ignore the leading '@'
        ++pTmp;

        // Check the size, which is encoded next
        sscanf((char*)pTmp, "%x", &payloadLen);

        // scan to the actual data payload after the size info
        pTmp = (unsigned char*)strchr((char*)pTmp, '-');

        // Check for an improperly formatted input
        if (pTmp == NULL)
            return 0;

        ++pTmp;

        // The rest of the source buffer contains the actual payload
        i = 0;
        while ( i <  payloadLen )
        {
            // Overflow check
            if (i > pDstLen)
            {
                return 0;
            }

            // Decode special characters 
            if ((pTmp[i] ^ strSecret[i % secretLen]) > 0x80)
                pDst[i] = (pTmp[i] - 0x80) ^ strSecret[i % secretLen];
            else
            // Decode normally
                pDst[i] = pTmp[i] ^ strSecret[i % secretLen];

            ++i;
        }

        //Ensure the last chartacter is a NULL termination
        pDst[i-1] = '\0';
    }
    else
    {
        // ENCRYPT
        pTmp = (unsigned char*)pDst;
        // First put the leading dollar sign
        pTmp[0] = '@';
        ++pTmp;

        // Encode the payload length
        payloadLen = len + 1;
        payloadSizeLen = sprintf((char*)pTmp, "%x", payloadLen);
        pTmp += strlen((char*)pTmp);

        // add a delimiter
        *pTmp++ = '-'; 
        

        // Encode the payload
        i = 0;
        while ( i < payloadLen )
        {
            // Overflow check
            // Check if i is greater than the length minus what has been written already
            if( i > (pDstLen - 2 - payloadSizeLen))
            {
                return 0;
            }

            pTmp[i] = pSrc[i] ^ strSecret[i % secretLen];

            // NULLs check: Since the send / receive tag system does not support NULLs in the payload, replace all NULL (0x00) by 0x80.
            // 0x80 is an impossible character giving neither the secret or the message is allowed to have ASCII characters greater than 0x79
            // '=' check: the equal sign also gets some special treatment, so shift them
            // Also, special ASCII control character (those under 0x016) can give us problems, so we'll avoid those as well.

            if ((pSrc[i] ^ strSecret[i % secretLen]) < 0x16 || pTmp[i] == '=')
                pTmp[i] = ((pSrc[i] + 0x80) ^ strSecret[i % secretLen]);
            ++i;
        }

        pTmp[i-1] = '\0';
    }

    // Return the length of the encrypted buffer
    return((uint32_t)strlen(pDst));
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiObfuscatePayload

    \Description
        XOR the bits of the payload with the SECRET, which is repeated as necessary.
        This is weak since the client knows the encrypt secret (different than the standard Blaze encyrpt secret), but better than nothing.
        Also this will generate an output of the same length, simplifying payload manipulations.
        Sending an encrypted string to this method will decrypt it.
        When encypting, this function will encrypt character until the end of pSrc.

    \Input  *pDst   - Destination string buffer
    \Input  pSrc    - Source string buffer
    \Input  pDstLen - Size of destination, not to be exceeded

    \Output int32_t - The length of the output buffer in pDst

    \Version 01/11/2011 (amakoukji)
*/
/********************************************************************************F*/
uint32_t _TelemetryApiObfuscatePayload(char *pDst, char *pSrc, uint32_t pDstLen)
{
    return(_TelemetryApiObfuscateNPayload(pDst, pSrc, (uint32_t)strlen(pSrc) + 1, pDstLen));
}

/*F*************************************************************************************************/
/*!
    \Function TelemetryApiUpdate

    \Description
        Perform needed background processing

    \Input *pTelemetryRef - module reference pointer

    \Output int32_t - Returns TC_OKAY for sucess.  -2

    \Version 04/06/2006 (tdills)
*/
/*************************************************************************************************F*/
int32_t TelemetryApiUpdate(TelemetryApiRefT *pTelemetryRef)
{
    int32_t iKind;
    int32_t iCode;
    char *pData;
    uint32_t uCurrentTime, uCurrentNumOfMessages;

    if (pTelemetryRef == NULL)
    {
        return(TELEMETRY3_ERROR_NULLPARAM );
    }

    if ((!pTelemetryRef->bCanConnect) || (pTelemetryRef->pAries == NULL))
    {
        // we can't do anything right now
        return(TELEMETRY_ERROR_NOTCONNECTED);
    }

    // while there are messages in the queue
    while (_TelemetryApiGetMessage(pTelemetryRef, &iKind, &iCode, &pData) >= 0)
    {
        switch (iKind)
        {
            case '&lgr':
                {
                    if (pTelemetryRef->pLoggerCallback != NULL)
                    {
                        pTelemetryRef->pLoggerCallback(pTelemetryRef, iCode);
                    }
                    if(iCode == 0)
                    {   
                        #if TELEMETRY_VERBOSE
                        // the send was successful
                        NetPrintf(("telemetryapi: successfully completed send of telemetry data to server.\n"));
                        #endif
                    }
                    else
                    {   
                        #if TELEMETRY_VERBOSE
                        // failed send - error recovery
                        NetPrintf(("telemetryapi: failed to send data to the telemetry server; result code '%c%c%c%c'\n", 
                            (char)(iCode >> 24), (char)(iCode >> 16), (char)(iCode >> 8), (char)iCode));
                        #endif
                    }
                }
                break;
            case '&beg':
                {
                    int32_t iTransactionId;
                    // read the transaction id out of the response
                    iTransactionId = TagFieldGetNumber(TagFieldFind(pData, "IDENT"), -1);

                    // make sure the callback is valid
                    if ((pTelemetryRef->bTransactionBegPending) && (pTelemetryRef->pTransactionBegCB != NULL))
                    {
                        // return the transaction id to the caller
                        pTelemetryRef->bTransactionBegPending = FALSE; 
                        pTelemetryRef->pTransactionBegCB(pTelemetryRef, pTelemetryRef->pTransactionBegUserData, iTransactionId);
                    }
                }
                break;
            case '&dat':
                {
                    if (pTelemetryRef->pTransactionDataCallback != NULL)
                    {
                        pTelemetryRef->pTransactionDataCallback(pTelemetryRef, iCode);
                    }
                }
                break;
            case '&end':
                {
                    if (pTelemetryRef->pTransactionEndCallback != NULL)
                    {
                        pTelemetryRef->pTransactionEndCallback(pTelemetryRef, iCode);
                    }
                }
                break;
        }
    }

    // give time to aries
    ProtoAriesUpdate(pTelemetryRef->pAries);

    // determine if we've just completed a full send
    if ((pTelemetryRef->bFullSendPending) &&
        (_TelemetryApiNumEvents(pTelemetryRef) == 0) &&
        (ProtoAriesStatus(pTelemetryRef->pAries, 'obuf', NULL, 0) == 0))
    {
        pTelemetryRef->bFullSendPending = FALSE;
        // if we're done sending everything, trigger the "I sent everything" callback
        _TelemetryApiTriggerCallback(pTelemetryRef, TELEMETRY_CBTYPE_FULLSENDCOMPLETE);
    }

    // be sure the send isn't disabled
    if (!_TelemetryApiCanSendTelemetry(pTelemetryRef))
    {
        // note: we check this here (and not in the generic send function, or in the
        // update callback) because the HALT control should not take effect if
        // a send is in progress.
        return(TC_OKAY);
    }
    
    // get the time
    uCurrentTime = NetTick();
    uCurrentNumOfMessages = _TelemetryApiNumEvents(pTelemetryRef);

    // check sending conditions -
    // see if the buffer is 3/4 full
    // or the data in it is older than the timeout value.
    if ( (uCurrentNumOfMessages < pTelemetryRef->uEventThreshold) &&
        ((uCurrentTime - pTelemetryRef->uLastServerSendTime) <= pTelemetryRef->uEventTimeout) 
        && pTelemetryRef->uNumEvent3Submitted == 0)
    {
        // no send just yet
    }
    else
    {
        // then check to see if we should send to the server
        return _TelemetryApiTrySendToServer(pTelemetryRef);
    }

    return(TC_OKAY);
}

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function TelemetryApiEventPrint

    \Description
        Print a telemetry event

    \Input  *pEvent          - event reference
    \Input   uEventNUm       - event number to print

    \Version 01/10/2006 (jfrank)
*/
/********************************************************************************F*/
void TelemetryApiEventPrint(TelemetryApiEventT *pEvent, uint32_t uEventNum)
{
    if (pEvent == NULL)
    {
        NetPrintf(("\t[----][    /    ][ts:        ][ %16s ]\n", "<NULL PTR>"));
    }
    else if ((pEvent->uGroupID == 0) && (pEvent->uModuleID == 0))
    {
        NetPrintf(("\t[%04u][    /    ][ts:        ][ %16s ]\n", uEventNum, "<no entry>"));
    }
    else if (pEvent->strEvent[0] == '\0')
    {
        NetPrintf(("\t[%04u][%c%c%c%c/%c%c%c%c][ts:%08X][ %16s ]\n", uEventNum,
            TELEMETRY_LocalizerTokenPrintCharArray(pEvent->uModuleID),
            TELEMETRY_LocalizerTokenPrintCharArray(pEvent->uGroupID),
            pEvent->iTimestamp, "<empty string>"));
    }
    else
    {
        NetPrintf(("\t[%04u][%c%c%c%c/%c%c%c%c][ts:%08X][ %16s ]\n", uEventNum,
            TELEMETRY_LocalizerTokenPrintCharArray(pEvent->uModuleID),
            TELEMETRY_LocalizerTokenPrintCharArray(pEvent->uGroupID),
            pEvent->iTimestamp, pEvent->strEvent));
    }
}
#endif


/*F********************************************************************************/
/*!
    \Function TelemetryApiEventsPrint

    \Description
        Print all telemtry events

    \Input  *pTelemetryRef - module reference

    \Version 01/10/2006 (jfrank)
*/
/********************************************************************************F*/
#if DIRTYCODE_LOGGING
void TelemetryApiEventsPrint(TelemetryApiRefT *pTelemetryRef)
{
    uint32_t uLoop;
    if (pTelemetryRef)
    {
        for(uLoop = 0; uLoop < pTelemetryRef->uEventBufSize; uLoop++)
        {
            TelemetryApiEventPrint(&(pTelemetryRef->Events[uLoop]), uLoop);
        }
    }

}
#endif


/*F********************************************************************************/
/*!
    \Function TelemetryApiFiltersPrint

    \Description
        Print a telemetry filter

    \Input  *pTelemetryRef - module reference

    \Notes
        Buffer size should be >= (24 bytes per filter * Number of Filters)

    \Version 01/10/2006 (jfrank)
*/
/********************************************************************************F*/
#if DIRTYCODE_LOGGING
void TelemetryApiFiltersPrint(TelemetryApiRefT *pTelemetryRef)
{
    uint32_t uLoop;
    char cEnable;
    const TelemetryApiFilterRuleT *pRule;

    if (pTelemetryRef)
    {
        // print all the filter rules
        for(uLoop = 0; uLoop < pTelemetryRef->uNumFilters; uLoop++)
        {
            pRule = &(pTelemetryRef->Filters[uLoop]); 
            switch(pRule->iEnable)
            {
            case  1:    cEnable = '+';  break;
            case -1:    cEnable = '-';  break;
            default:    cEnable = '?';
            }

            NetPrintf(("\t[%02u][ %c %c%c%c%c / %c%c%c%c / %c%c%c%c ]\n", uLoop, cEnable,
                TELEMETRY_LocalizerTokenPrintCharArray(pRule->uModuleID),
                TELEMETRY_LocalizerTokenPrintCharArray(pRule->uGroupID),
                TELEMETRY_LocalizerTokenPrintCharArray(pRule->uStringID)));
        }
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function TelemetryApiSetCallback

    \Description
        Register a callback.  Only one callback per type, new callback requests
        will overwrite old requests.  Register a type with a NULL function pointer
        to cancel the callback.

    \Input *pTelemetryRef   - module state
    \Input eType            - the event type we're registering a callback for
    \Input *pCallback       - callback pointer, NULL to cancel a callback for a type
    \Input *pUserData       - user data field to send with the callback

    \Version 1.0 01/25/2006 (jfrank) First Version
*/
/********************************************************************************F*/
void TelemetryApiSetCallback(TelemetryApiRefT *pTelemetryRef, TelemetryApiCBTypeE eType, TelemetryApiCallbackT *pCallback, void *pUserData)
{
    #ifdef TELEMETRY_VERBOSE
    NetPrintf(("telemetryapi: setcallback [0x%08x] type=%d callback=[0x%08x]\n", pTelemetryRef, eType, pCallback));
    #endif

    // check for errors
    if(pTelemetryRef == NULL)
        return;

    // assign the callback if possible
    if(eType < TELEMETRY_CBTYPE_TOTAL)
    {
        pTelemetryRef->callbacks[eType].pCallback = pCallback;
        pTelemetryRef->callbacks[eType].pUserData = pUserData;
    }
    else
    {
        NetPrintf(("telemetryapi: unhandled telemtry callback type [%d]\n", (int32_t)eType));
    }
}


/*F********************************************************************************/
/*!
    \Function TelemetryApiConnect

    \Description
        This function “connects” the API to the telemetry server so data can be 
        uploaded.

    \Input *pTelemetryRef     - module state

    \Output int32_t - TRUE if we can now connect; FALSE if not.

    \Version 1.0 10/11/2004 (jfrank) First Version
*/
/********************************************************************************F*/
int32_t TelemetryApiConnect(TelemetryApiRefT *pTelemetryRef)
{
    if (pTelemetryRef == NULL)
    {
        return(TELEMETRY3_ERROR_NULLPARAM );
    }

    #ifdef TELEMETRY_VERBOSE
    NetPrintf(("telemetryapi: connect [0x%08x]\n", pTelemetryRef));
    #endif

    // create virtual session handler
    DirtyMemGroupEnter(pTelemetryRef->iMemGroup, pTelemetryRef->pMemGroupUserData);
    pTelemetryRef->pAries = ProtoAriesCreate(8192);
    DirtyMemGroupLeave();

    // if Aries creation fails, bail
    if (pTelemetryRef->pAries == NULL)
    {
        return(FALSE);
    }

    // note that it is now okay to connect
    pTelemetryRef->bCanConnect = TRUE;
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function TelemetryApiStatus

    \Description
        Return telemetry information.

    \Input *pTelemetryRef - module state
    \Input  iSelect       - output selector
    \Input *pBuf          - [out] pointer to output buffer
    \Input  iBufSize      - size of output buffer

    \Output
        int32_t - selector specific

    \Notes
        iSelect can be one of the following:
    
        \verbatim
                'ctry' - return if telemetry is enabled based on locale
                'locl' - negative=error, else copies user locale into provided buffer
                'evth' - return the event buffer head pointer value
                'evtt' - return the event buffer tail pointer value
                'full' - return 1 if full,  0 otherwise
                'halt' - return 1 if sending is halted, 0 otherwise
                'maxe' - return the maximum number of events which can be stored
                'mflt' - return the maximum number of filter rules
                'mpty' - return 1 if empty, 0 otherwise
                'nabl' - can telemetry be sent to the server?
                'nflt' - return the current number of filter rules
                'nume' - return the current number of events stored
                'ssiz' - returns the maximum size the snapshot buffer can be
                'time' - return the timeout for sending data (milliseconds)
                'isv3' - returns 1 if the API was intialized for telemetry 3.0
                'num3' - number of telemetry 3.0 events submitted (and buffered)
                'cryp' - return if telemetry data is encrypted
                'stim' - return if telemetry events will be logger with the server's time.
                'rtry' - return the number of automatic retries to flush telemetry on a full buffer
                'cdbl' - copies the list of disabled countries into the provided buffer, return -1 if the buffer is too small, 0 otherwise.
        \endverbatim

    \Version 1.0 01/11/2006 (jfrank) First Version
    \Version 2.0 04/06/2006 (tdills) Moved to LobbyLoggerInfoInt to TelemetryApiStatus
*/
/********************************************************************************F*/
int32_t TelemetryApiStatus(TelemetryApiRefT *pTelemetryRef, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    // there are no states which allow the pTelemetryRef to be set to NULL
    // so return an error condition
    if (pTelemetryRef == NULL)
    {
        return(-1);
    }

    switch(iSelect)
    {
        case 'maxe':    return((int32_t)pTelemetryRef->uEventBufSize);
        case 'nume':    return((int32_t)_TelemetryApiNumEvents(pTelemetryRef));
        case 'full':    return((int32_t)(pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_FULL)  ? 1 : 0);
        case 'mpty':    return((int32_t)(pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_EMPTY) ? 1 : 0);
        case 'halt':    return((int32_t)(pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_SENDHALTED) ? 1 : 0);
        case 'nflt':    return((int32_t)pTelemetryRef->uNumFilters);
        case 'mflt':    return((int32_t)TELEMETRY_FILTERRULES_MAX);
        case 'evth':    return((int32_t)pTelemetryRef->uEventHead);
        case 'evtt':    return((int32_t)pTelemetryRef->uEventTail);
        case 'time':    return((int32_t)pTelemetryRef->uEventTimeout);
        case 'nabl':    return(_TelemetryApiCanSendTelemetry(pTelemetryRef));
        case 'ssiz':    return(sizeof(TelemetryApiSnapshotHeaderT) + (sizeof(TelemetryApiEventT) * pTelemetryRef->uEventBufSize));
        case 'ctry':    return(!pTelemetryRef->bCountryDisabled);
        case 'isv3':    return((int32_t)(pTelemetryRef->uEvent3BufferSize > 0) ? 1 : 0);
        case 'num3':    return((int32_t)pTelemetryRef->uNumEvent3Submitted);
        case 'cryp':    return((int32_t)(pTelemetryRef->bEncrypt > 0) ? 1 : 0);
        case 'stim':    return((int32_t)(pTelemetryRef->bUseServerTime > 0) ? 1 : 0);
        case 'rtry':    return((int32_t)pTelemetryRef->uSubmit3MaxRetry);
        case 'cdbl':
        {
            if (pBuf != NULL && iBufSize > 0)
            {
                strnzcpy(pBuf, pTelemetryRef->strDisabledCountries, iBufSize);
                if (iBufSize <= (int32_t)strlen(pTelemetryRef->strDisabledCountries))
                {
                    return -1; // Buffer not large enough
                }
                else
                {
                    return(0);
                }
            }
            break;
        }
        case 'locl':
        {
            if ((pBuf != NULL) && (pTelemetryRef->strUserLocale[0] != '\0'))
            {
                strnzcpy(pBuf, pTelemetryRef->strUserLocale, iBufSize);
                return(0);
            }
            break;
        }
    }

    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function    TelemetryApiControl

    \Description
        TelemetryApi control function.  Different selectors control different behaviors.

    \Input  pTelemetryRef   - module ref
    \Input  iKind           - control selector
    \Input  iValue          - selector-specific data
    \Input  pValue          - selector-specific data

    \Output int32_t         - selector-specific

    \Notes
        iKind can be one of the following:

        \verbatim
            'auth' - whether authentication bypass is enabled (0=disabled; 1=enabled)
            'cdbl' - list of countries for which telemetry is disabled
            'flsh' - try to send even telemetry data regardless of how full the buffer is
            'halt' - temporarily halt sending to the server (0=not halted, ok to send)
            'logc' - prepend all entries in the server log with the character (char)(iValue)
            'thrs' - set the THReShold for sending data (in percentage, 0..100)
            'time' - set the timeout for sending data (milliseconds)
            'unbl' - whether or not the user is enabled for telemetry (0=disabled; 1=enabled)
            'anon' - whether anonymous telemetry tag field is sent (0=disabled, 1=enabled)
            'subd' - select a subdirectory of the telemetry domain (maximum 4 chars)
            'cryp' - set whether to encrypt telemetry data
            'rtry' - set the number of automatic retries when a full telemetry 3 buffer is encountered.
            'ipad' - set the IP address to report in telemetry logs. IP will be reported exactly as written, use pValue = NULL to reset. (maximum 64 characters)
            'tcip' - set a flag to indicate to the server to truncate the last octet of the user's IP for data privacy reasons.
        \endverbatim

    \Version 04/05/2004 (jbrookes)
    \Version 04/06/2006 (tdills) Moved to TelemetryApi added 'dslb'
*/
/********************************************************************************F*/
int32_t TelemetryApiControl(TelemetryApiRefT *pTelemetryRef, int32_t iKind, int32_t iValue, void* pValue)
{
    int32_t iReturn = 0;
    int32_t i;
    char *pSrc;
    uint8_t ipOk = TRUE;

    // there's nothing we can set without a logger reference,
    //  so this is an error
    if (pTelemetryRef == NULL)
    {
        return(iReturn);
    }

    #ifdef TELEMETRY_VERBOSE
    NetPrintf(("telemetryapi: control [0x%08x] %c%c%c%c %d\n", pTelemetryRef,                             
        (char)(iKind >> 24), (char)(iKind >> 16), (char)(iKind >> 8), (char)iKind, iValue));
    #endif

    switch(iKind)
    {
        case 'auth':
            NetPrintf(("telemetryapi: %s authentication bypass\n", iValue ? "enabling" : "disabling"));
            pTelemetryRef->bDisableAuthentication = (uint8_t)iValue;
            break;
        case 'anon':
            NetPrintf(("telemetryapi: %s anonymous telemetry\n", iValue ? "enabling" : "disabling"));
            pTelemetryRef->bAnonymous = (uint8_t)iValue;
            break;
        case 'cdbl':
            if (pValue != NULL)
            {
                // store the list of countries
                strncpy(pTelemetryRef->strDisabledCountries, pValue, sizeof(pTelemetryRef->strDisabledCountries));
                // compute the value of the disabled flag
                _TelemetryApiSetCountryDisabledFlag(pTelemetryRef);
            }
            break;
        case 'flsh': 
            if (_TelemetryApiCanSendTelemetry(pTelemetryRef)) 
            { 
                return _TelemetryApiTrySendToServer(pTelemetryRef);
            } 
            else 
            {
                NetPrintf(("telemetryapi: unable to send telemetry\n"));
                iReturn = -1;
            }
            break; 
        case 'halt':
            if(iValue == 0)
            {
                NetPrintf(("telemetryapi: telemetry has been un-halted.\n"));
                pTelemetryRef->uEventState &= (~TELEMETRY_EVENTBUFFERSTATE_SENDHALTED);
            }
            else
            {
                NetPrintf(("telemetryapi: telemetry has been halted.\n"));
                pTelemetryRef->uEventState |= TELEMETRY_EVENTBUFFERSTATE_SENDHALTED;
            }
            break;
        case 'logc':
            if(iValue == 0)
            {
                pTelemetryRef->cEntryPrepender = 0;
            }
            else if(TELEMETRY_SERVERLOGPREPENDCHAR_VALID((char)iValue))
            {
                pTelemetryRef->cEntryPrepender = (char)(iValue);
            }
            else
            {
                NetPrintf(("telemetryapi: illegal prepend character [%c]\n", (char)(iValue)));
                iReturn = -1;
            }
            break;
        case 'subd':
            if(pValue != NULL) 
            {
                pSrc = (char *)pValue;
                i = 0;
                while(TELEMETRY_EVENT_TOKENCHAR_VALID(*pSrc) && i < (TELEMETRY_SUBDIR_LEN-1))
                {
                    pTelemetryRef->strSubDir[i++] = *pSrc++;
                }
                pTelemetryRef->strSubDir[i] = '\0';
            }
            else
            {
                pTelemetryRef->strSubDir[0] = '\0';
            }
            break;
        case 'time':    
            if (iValue >= 0)
            {
                pTelemetryRef->uEventTimeout = (uint32_t)iValue;         
            }
            break;
        case 'thrs':    
            if ((iValue <= 100) && (iValue >= 0))
            {
                pTelemetryRef->uEventThreshold = (pTelemetryRef->uEventBufSize * ((uint32_t)(iValue))) / 100;
            }
            // we've gotta have at least one event to send to the server
            if (pTelemetryRef->uEventThreshold == 0)
            {
                pTelemetryRef->uEventThreshold++;
            }
            break;
        case 'unbl':
            // store the user setting
            if (iValue > 0)
            {
                NetPrintf(("telemetryapi: telemetry user-setting set to enabled.\n"));
                pTelemetryRef->bUserDisabled = FALSE;
            }
            else
            {
                NetPrintf(("telemetryapi: telemetry user-setting set to disabled; telemetry data is suspended.\n"));
                pTelemetryRef->bUserDisabled = TRUE;
            }
            break;
        case 'cryp':  
            {
                if (iValue > 0)
                {
                    pTelemetryRef->bEncrypt = 1;
                    // Allocate encryption buffer space if needed
                    if (pTelemetryRef->pEncryptionBuffer == NULL)
                    {
                        // Query current mem group data
                        pTelemetryRef->pEncryptionBuffer = DirtyMemAlloc(pTelemetryRef->uEvent3BufferSize + TELEMETRY_ENCRYPTION_BUFFER_OVERHEAD_SIZE, TELEMETRYAPI_MEMID, pTelemetryRef->iMemGroup, pTelemetryRef->pMemGroupUserData);
                        pTelemetryRef->uEncryptionBufferSize = pTelemetryRef->uEvent3BufferSize;
                    }
                }
                else
                {
                    pTelemetryRef->bEncrypt = 0;

                }
            }
            
            break;
        case 'stim':
            {
                if (iValue > 0)
                    pTelemetryRef->bUseServerTime = 1;
                else
                    pTelemetryRef->bUseServerTime = 0;
            }
            break;
        case 'stio':
            {
                pSrc = (char *)pValue;
                if (ds_stricmp("True", pSrc) == 0)
                    pTelemetryRef->bUseServerTimeOveride = TELEMETRY_OVERRIDE_TRUE;
                else if (ds_stricmp("False", pSrc) == 0)
                    pTelemetryRef->bUseServerTimeOveride = TELEMETRY_OVERRIDE_FALSE;
                else // should be "Default" but it will take anything else
                    pTelemetryRef->bUseServerTimeOveride = TELEMETRY_OVERRIDE_DEFAULT;
            }
            break;
        case 'rtry':    
            if (iValue >= 0)
            {
                pTelemetryRef->uSubmit3MaxRetry = (uint32_t)iValue;         
            }
            break;
        case 'ipad':
            if (pValue != NULL)
            {
                // check that all characters in the IP are allowable
                ds_strnzcpy(pTelemetryRef->strCustomIP, (char*)pValue, sizeof(pTelemetryRef->strCustomIP));
                for (i=0; i<(int32_t)strlen((char*)pTelemetryRef->strCustomIP);++i)
                {
                    if (!TELEMETRY_EVENTSTRINGCHAR_VALID(((char*)pTelemetryRef->strCustomIP)[i]))
                    {
                        ipOk = FALSE;
                        ((char*)pTelemetryRef->strCustomIP)[i] = TELEMETRY_ILLEGALEVENTSTRINGCHAR_REPLACEMENT;
                    }
                }

                if (!ipOk)
                {
                    iReturn = -1;
                }
            }
            break;
        case 'tcip':
            {
                if (iValue > 0)
                {
                    pTelemetryRef->iTruncateIP = iValue;
                }
                else
                {
                    pTelemetryRef->iTruncateIP = 0;
                }
            }
            break;
            
    }
    return(iReturn);
}


/*F********************************************************************************/
/*!
    \Function TelemetryApiFilter

    \Description
        Set up the filtering rule, typically coming in from the news file.
        Send a NULL string to remove all filters.  Will apply to the current 
        items already in the buffer, as well as any new items.

    \Input *pTelemetryRef        - module state
    \Input *pFilterRule - the filter rule to use

    \Notes
        Filter rules should be listed in the following format:

    \verbatim
        <+/-><moduleID>/<groupID>,

        Either a + or - must be present.
        The module/group's full ID must be listed - all four characters.
        The module and group IDs are separated by a forward or backward slash.
        Each entry should be separated by a comma.
        Module and group ID must use [0..9, A..Z, a..z, *] - 
            !! no other characters are permitted !!
        Full wildcards are OK ('****') but NOT partial wildcards, 
            such as ('*BAD') or ('*N*o').
        Wildcards are only acceptable as filter rules, not events.

        +XXXX/XXXX, -XXXX/XXXX, -XXXX/XXXX, +XXXX/XXXX
    \endverbatim

    \Version 01/06/2006 (jfrank)
*/
/********************************************************************************F*/
void TelemetryApiFilter(TelemetryApiRefT *pTelemetryRef, const char *pFilterRule)
{

    // check for errors
    if (pTelemetryRef == NULL)
        return;

    // set the filter rule
    _TelemetryApiSetFilter(pTelemetryRef, pFilterRule);

    // finally apply the rule to all the entries already in the buffer
    _TelemetryApiEventBufferApplyFilter(pTelemetryRef);

}


/*F********************************************************************************/
/*!
    \Function TelemetryApiEvent

    \Description
        This function logs an event string to a local buffer to later be sent 
        up to the telemetry server.  Empty or NULL strings are ignored.

    \Input *pTelemetryRef        - module state
    \Input  uModuleID   - the module ID in which the event occurred
    \Input  uGroupID    - group within the module where the event occurred
    \Input *pEvent      - action string to be logged - null terminated

    \Version 10/11/2004 (jfrank) First Version
*/
/********************************************************************************F*/
int32_t TelemetryApiEvent(TelemetryApiRefT *pTelemetryRef, uint32_t uModuleID, uint32_t uGroupID, const char *pEvent)
{
    #ifdef TELEMETRY_VERBOSE
    NetPrintf(("telemetryapi: event [0x%08x] %c%c%c%c/%c%c%c%c %s\n", pTelemetryRef,                             
        (char)(uModuleID >> 24), (char)(uModuleID >> 16), (char)(uModuleID >> 8), (char)uModuleID, 
        (char)(uGroupID >> 24), (char)(uGroupID >> 16), (char)(uGroupID >> 8), (char)uGroupID, 
        pEvent));
    #endif

    // check to make sure we have a valid telemetry pointer
    if (pTelemetryRef == NULL)
    {
        // print an error message and return.
        NetPrintf(("telemetryapi: warning - TelemetryApiEvent() - must provide a valid telemetry pointer with the event; no event logged.\n"));
        return(TELEMETRY_ERROR_UNKNOWN);
    }

    // to be successfully added to the internal buffer, three conditions must be met
    //    1. must have a valid module ID
    //    2. must have a valid group ID
    //    3. must not be currently disabled by any filter rules
    if (TELEMETRY_EVENT_TOKEN_VALID(uModuleID) && 
        TELEMETRY_EVENT_TOKEN_VALID(uGroupID) && 
        (_TelemetryApiFilterEvent(pTelemetryRef, uModuleID, uGroupID) == 1))
    {
        TelemetryApiEventT NewEvent;
        memset(&NewEvent, 0, sizeof(NewEvent));

        // if we have a legal event, log it
        if (pEvent)
        {
            uint32_t uLoop;
            char cTemp;

            // copy all the valid characters, replacing the invalid ones with the
            // specified replacement character, and leaving room for a NULL terminator
            for(uLoop = 0; uLoop < (TELEMETRY_EVENTSTRINGSIZE_DEFAULT - 1); uLoop++)
            {
                cTemp = *pEvent++;
                if(cTemp == '\0')
                {
                    // first do the easy check - are we out?
                    break;
                }
                else if(!TELEMETRY_EVENTSTRINGCHAR_VALID(cTemp))
                {
                    // if it's illegal, replace it
                    cTemp = TELEMETRY_ILLEGALEVENTSTRINGCHAR_REPLACEMENT;
                }

                // now copy it to the new buffer
                NewEvent.strEvent[uLoop] = cTemp;
            }
        }

        // copy in the module and group
        NewEvent.uModuleID = uModuleID;
        NewEvent.uGroupID  = uGroupID;

        // Increment step counter
        NewEvent.uStep = pTelemetryRef->uStep++;

        // Set the relative tick 
        NewEvent.iTimestamp = _TelemetryApiRelativeTick(pTelemetryRef);

        // finally add the new event to the buffer
        return TelemetryApiPushBack(pTelemetryRef, &NewEvent);
    }

    return(TELEMETRY_ERROR_UNKNOWN);
}

/*F********************************************************************************/
/*!
    \Function TelemetryApiEventBDID

    \Description
        Log BDID event(s).        

    \Input *pTelemetryRef   - module state
    \Input *pIdent          - BDID ident (binary data)
    \Input iLength          - length of ident

    \Version 03/02/2010 (jbrookes)
*/
/********************************************************************************F*/
void TelemetryApiEventBDID(TelemetryApiRefT *pTelemetryRef, const uint8_t *pIdent, int32_t iLength)
{
    int32_t iEvent, iNumEvents = iLength/4; // four bytes per event
    char strEvent[9]; // eight chars per event

    // validate input length    
    #if defined(DIRTYCODE_PS3)
    if (iLength != 16)
    #else
    if (((iLength & 0x3) != 0) || (iNumEvents > 9))
    #endif
    {
        NetPrintf(("telemetryapi: error; BDID of length %d is invalid and will not be reported\n", iLength));
        return;
    }
    
    // submit an event for every four bytes in ident data
    for (iEvent = 0; iEvent < iNumEvents; iEvent += 1, pIdent += 4)
    {
        snzprintf(strEvent, sizeof(strEvent), "%2X%2X%2X%2X", pIdent[0], pIdent[1], pIdent[2], pIdent[3]);
        TelemetryApiEvent(pTelemetryRef, 'BDID', 'STR0' + iEvent, strEvent);
    }
}


/*F********************************************************************************/
/*!
    \Function TelemetryApiPopFront

    \Description
        Remove an entry from the front of the buffer, if possible.  This entry is
        typically the oldest entry in the buffer.  Provide a destination buffer
        for the popped entry, or pass NULL if no returned entry is desired.

    \Input *pTelemetryRef        - module state
    \Input *pEvent      - buffer to store the popped event, NULL if not needed

    \Output TelemetryApiEventT* - pointer to the entry popped, NULL if no entry was
        popped or no target buffer was provided.  Typically this will have the same
        value as the target buffer (pEvent) passed in, if one was provided.

    \Version 1.0 01/10/2006 (jfrank) First Version
    \Version 2.0 04/06/2006 (tdills) Moved to TelemetryApi
*/
/********************************************************************************F*/
TelemetryApiEventT *TelemetryApiPopFront(TelemetryApiRefT *pTelemetryRef, TelemetryApiEventT *pEvent)
{
    TelemetryApiEventT *pPoppedEvent;
    // this is an externally-exposed function, so check for errors
    if (pTelemetryRef == NULL)
        return(NULL);

    // see if we're already empty
    if (pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_EMPTY)
        return(NULL);

    // mark the buffer as "not full"
    pTelemetryRef->uEventState &= (~TELEMETRY_EVENTBUFFERSTATE_FULL);
    
    // remove something from the buffer
    pPoppedEvent = &(pTelemetryRef->Events[pTelemetryRef->uEventHead]);
    if (pEvent)
    {
        // copy the popped entry into the destination buffer if one was provided
        memcpy(pEvent, pPoppedEvent, sizeof(*pEvent));
    }

#if TELEMETRY_DEBUG
    // clear the memory slot for that entry
    memset(pPoppedEvent, 0, sizeof(*pPoppedEvent));
#endif

    // see if the buffer is empty
    if (pTelemetryRef->uEventTail == pTelemetryRef->uEventHead)
    {
        pTelemetryRef->uEventState |= TELEMETRY_EVENTBUFFERSTATE_EMPTY;
        _TelemetryApiTriggerCallback(pTelemetryRef, TELEMETRY_CBTYPE_BUFFEREMPTY);
    }
    else
    {
        // adjust the head pointer
        pTelemetryRef->uEventHead++;
        if (pTelemetryRef->uEventHead == pTelemetryRef->uEventBufSize)
            pTelemetryRef->uEventHead = 0;
    }

    // return the event pointer, NULL or not
    return(pEvent);
}


/*F********************************************************************************/
/*!
    \Function TelemetryApiPopBack

    \Description
        Remove an entry from the back of the buffer, if possible.  This entry is
        typically the newest entry in the buffer.  Provide a destination buffer
        for the popped entry, or pass NULL if no returned entry is desired.

    \Input *pTelemetryRef        - module state
    \Input *pEvent      - buffer to store the popped event, NULL if not needed

    \Output TelemetryApiEventT* - pointer to the entry popped, NULL if no entry was
        popped or no target buffer was provided.  Typically this will have the same
        value as the target buffer (pEvent) passed in, if one was provided.

    \Version 1.0 01/10/2006 (jfrank) First Version
    \Version 2.0 04/06/2006 (tdills) Moved to TelemetryApi
*/
/********************************************************************************F*/
TelemetryApiEventT *TelemetryApiPopBack(TelemetryApiRefT *pTelemetryRef, TelemetryApiEventT *pEvent)
{
    TelemetryApiEventT *pPoppedEvent;
    // this is an externally-exposed function, so check for errors
    if (pTelemetryRef == NULL)
        return(NULL);

    // see if we're already empty
    if (pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_EMPTY)
        return(NULL);

    // mark the buffer as "not full"
    pTelemetryRef->uEventState &= (~TELEMETRY_EVENTBUFFERSTATE_FULL);
    
    // remove something from the buffer
    pPoppedEvent = &(pTelemetryRef->Events[pTelemetryRef->uEventTail]);
    if (pEvent)
    {
        // copy the popped entry into the destination buffer if one was provided
        memcpy(pEvent, pPoppedEvent, sizeof(*pEvent));
    }

#if TELEMETRY_DEBUG
    // clear the memory slot for that entry
    memset(pPoppedEvent, 0, sizeof(*pPoppedEvent));
#endif

    // see if the buffer is empty
    if (pTelemetryRef->uEventTail == pTelemetryRef->uEventHead)
    {
        pTelemetryRef->uEventState |= TELEMETRY_EVENTBUFFERSTATE_EMPTY;
        _TelemetryApiTriggerCallback(pTelemetryRef, TELEMETRY_CBTYPE_BUFFEREMPTY);
    }
    else
    {
        // adjust the tail pointer
        if (pTelemetryRef->uEventTail == 0)
            pTelemetryRef->uEventTail = pTelemetryRef->uEventBufSize;
        pTelemetryRef->uEventTail--;
    }


    // return the event pointer, NULL or not
    return(pEvent);
}


/*F********************************************************************************/
/*!
    \Function TelemetryApiPushFront

    \Description
        Push an entry onto the front of the buffer, if possible.  This is typically
        where the oldest entries live.  No entry will be added if this is a
        "fill and stop" buffer and the buffer is already full.

    \Input *pTelemetryRef        - module state
    \Input *pEvent      - buffer to push

    \Output int32_t     - 0=success, error code otherwise

    \Version 1.0 01/10/2006 (jfrank) First Version
    \Version 2.0 04/06/2006 (tdills) Moved to TelemetryApi
*/
/********************************************************************************F*/
int32_t TelemetryApiPushFront(TelemetryApiRefT *pTelemetryRef, const TelemetryApiEventT *pEvent)
{
    // this is an externally-exposed function, so check for errors
    if ((pTelemetryRef == NULL) || (pEvent == NULL))
        return(TELEMETRY_ERROR_UNKNOWN);

    // check for full
    if (pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_FULL)
    {
        // if we're a "fill and then stop" buffer, don't do anything
        if (pTelemetryRef->eBufferType == TELEMETRY_BUFFERTYPE_FILLANDSTOP)
        {
            return(TELEMETRY_ERROR_BUFFERFULL);
        }
        // if we're a circular overwrite butter and we're full, 
        //  decrement the tail pointer to make room for a new entry
        else
        {
            if (pTelemetryRef->uEventTail == 0)
                pTelemetryRef->uEventTail = pTelemetryRef->uEventBufSize;
            pTelemetryRef->uEventTail--;
        }
    }

    // at this point, we're only dealing with circular buffers
    // check for non-empty
    if (pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_EMPTY)
    {
        pTelemetryRef->uEventTail = 0;
        pTelemetryRef->uEventHead = 0;
        pTelemetryRef->uEventState &= (~TELEMETRY_EVENTBUFFERSTATE_EMPTY);
        memcpy(&(pTelemetryRef->Events[pTelemetryRef->uEventHead]), pEvent, sizeof(*pEvent));
    }
    // if not empty, do a normal buffer insertion
    else
    {
        // decrement the head pointer
        if (pTelemetryRef->uEventHead == 0)
            pTelemetryRef->uEventHead = pTelemetryRef->uEventBufSize;
        pTelemetryRef->uEventHead--;

        // copy the event
        memcpy(&(pTelemetryRef->Events[pTelemetryRef->uEventHead]), pEvent, sizeof(*pEvent));
    }

    // we have new data so we will eventually send the client a FULLSEND event
    pTelemetryRef->bFullSendPending = TRUE;

    // check for full and take action if necessary
    if (((pTelemetryRef->uEventTail+1) % pTelemetryRef->uEventBufSize) == pTelemetryRef->uEventHead)
    {
        // set the state to FULL and call the callback, if it exists
        pTelemetryRef->uEventState |= TELEMETRY_EVENTBUFFERSTATE_FULL;
        _TelemetryApiTriggerCallback(pTelemetryRef, TELEMETRY_CBTYPE_BUFFERFULL);
    }

    // success
    return(0);
}


/*F********************************************************************************/
/*!
    \Function TelemetryApiPushBack

    \Description
        Push an entry onto the front of the buffer, if possible.  This is typically
        where the oldest entries live.  No entry will be added if this is a
        "fill and stop" buffer and the buffer is already full.

    \Input *pTelemetryRef        - module state
    \Input *pEvent      - buffer to push

    \Output int32_t     - 0=success, error code otherwise

    \Version 1.0 01/10/2006 (jfrank) First Version
    \Version 2.0 04/06/2006 (tdills) Moved to TelemetryApi
*/
/********************************************************************************F*/
int32_t TelemetryApiPushBack(TelemetryApiRefT *pTelemetryRef, const TelemetryApiEventT *pEvent)
{
    // this is an externally-exposed function, so check for errors
    if ((pTelemetryRef == NULL) || (pEvent == NULL))
        return(TELEMETRY_ERROR_UNKNOWN);
    
    // check for full
    if (pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_FULL)
    {
        // if we're a "fill and then stop" buffer, don't do anything
        if (pTelemetryRef->eBufferType == TELEMETRY_BUFFERTYPE_FILLANDSTOP)
        {
            return(TELEMETRY_ERROR_BUFFERFULL);
        }
        // if we're a circular overwrite butter and we're full, 
        //  advance the head pointer to make room for a new entry
        else
        {
            pTelemetryRef->uEventHead++;
            if (pTelemetryRef->uEventHead == pTelemetryRef->uEventBufSize)
                pTelemetryRef->uEventHead = 0;
        }
    }
    
    // at this point, we're only dealing with circular buffers
    // check for non-empty
    if (pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_EMPTY)
    {
        pTelemetryRef->uEventTail = 0;
        pTelemetryRef->uEventHead = 0;
        pTelemetryRef->uEventState &= (~TELEMETRY_EVENTBUFFERSTATE_EMPTY);
        memcpy(&(pTelemetryRef->Events[pTelemetryRef->uEventTail]), pEvent, sizeof(*pEvent));
    }
    // if not empty, do a normal buffer insertion
    else
    {
        // increment the tail pointer
        pTelemetryRef->uEventTail++;
        if (pTelemetryRef->uEventTail == pTelemetryRef->uEventBufSize)
            pTelemetryRef->uEventTail = 0;

        // copy the event
        memcpy(&(pTelemetryRef->Events[pTelemetryRef->uEventTail]), pEvent, sizeof(*pEvent));
    }

    // we have new data so we will eventually send the client a FULLSEND event
    pTelemetryRef->bFullSendPending = TRUE;

    // check for full and take action if necessary
    if (((pTelemetryRef->uEventTail+1) % pTelemetryRef->uEventBufSize) == pTelemetryRef->uEventHead)
    {
        // set the state to FULL and call the callback, if it exists
        pTelemetryRef->uEventState |= TELEMETRY_EVENTBUFFERSTATE_FULL;
        _TelemetryApiTriggerCallback(pTelemetryRef, TELEMETRY_CBTYPE_BUFFERFULL);
    }

    // success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function TelemetryApiSnapshotSave

    \Description
        Use the provided buffer to store the telemetry snapshot. 
        Send NULL as a destination buffer to get the req'd buf size.

    \Input *pTelemetryRef        - module state
    \Input *pDestBuf    - buffer to save to (or NULL for size)
    \Input  uDestSize   - size of the buffer to save to

    \Output uint32_t    - Number of bytes saved, 0 if nothing saved (or error)

    \Notes
        Saves data in the following format:

        \verbatim
            (4 bytes)   length of buffer
            (4 bytes)   checksum
            (N bytes)   events (N = sizeof(event) * number of events)
        \endverbatim


    \Version 1.0 01/20/2006 (jfrank) First Version
    \Version 2.0 04/06/2006 (tdills) Moved to TelemetryApi
*/
/********************************************************************************F*/
uint32_t TelemetryApiSnapshotSave(TelemetryApiRefT *pTelemetryRef, void *pDestBuf, uint32_t uDestSize)
{
    uint32_t uBufferSaveSize, uNumEvents, uAmtToCopy;
    TelemetryApiSnapshotHeaderT *pHeader;
    TelemetryApiEventT *pEvents;

    // check for error
    if(pTelemetryRef == NULL)
    {
        return(0);
    }

    // calculate the size of the buffer needed
    uNumEvents      = _TelemetryApiNumEvents(pTelemetryRef);
    uBufferSaveSize = sizeof(TelemetryApiSnapshotHeaderT) + (sizeof(TelemetryApiEventT) * uNumEvents);

    if(uNumEvents == 0)
    {
        // no events to save
        return(0);
    }
    else if(pDestBuf == NULL)
    {
        // simply return the size required
        return(uBufferSaveSize);
    }
    else if(uDestSize < uBufferSaveSize)
    {
        // the buffer isn't big enough - return nothing
        return(0);
    }

    // determine where to put the events
    pHeader = (TelemetryApiSnapshotHeaderT *)pDestBuf;
    pEvents = (TelemetryApiEventT *)(pHeader+1);

    // save the events
    if(pTelemetryRef->uEventHead <= pTelemetryRef->uEventTail)
    {
        uAmtToCopy = (pTelemetryRef->uEventTail - pTelemetryRef->uEventHead + 1);
        memcpy(pEvents, &(pTelemetryRef->Events[pTelemetryRef->uEventHead]), uAmtToCopy * sizeof(TelemetryApiEventT));
    }
    else
    {
        TelemetryApiEventT *pEventsPtr = pEvents;
        uAmtToCopy = (pTelemetryRef->uEventBufSize - pTelemetryRef->uEventHead);
        memcpy(pEventsPtr, &(pTelemetryRef->Events[pTelemetryRef->uEventHead]), uAmtToCopy * sizeof(TelemetryApiEventT));
        pEventsPtr += uAmtToCopy;
        uAmtToCopy = (pTelemetryRef->uEventTail) + 1;
        memcpy(pEventsPtr, &(pTelemetryRef->Events[0]), uAmtToCopy * sizeof(TelemetryApiEventT));
    }

    // calculate the header data and store it
    pHeader->uBufferSize = uBufferSaveSize;
    pHeader->uNumEvents  = uNumEvents;
    pHeader->uEventsChecksum = _TelemetryApiChecksum((void *)pEvents, uNumEvents * sizeof(TelemetryApiEventT));

    // return the size of the buffer we saved
    return(uBufferSaveSize);
}

/*F********************************************************************************/
/*!
    \Function TelemetryApiSnapshotSize

    \Description
        Determine the number of events contained in a previously saved buffer

    \Input *pSourceBuf     - buffer to load from
    \Input  uSourceSize    - size of the buffer to load from

    \Output uint32_t     - number of entries in buffer

    \Version 1.0 06/21/2006 (tdills) First Version
*/
/********************************************************************************F*/
uint32_t TelemetryApiSnapshotSize(const void *pSourceBuf, uint32_t uSourceSize)
{
    const TelemetryApiSnapshotHeaderT *pHeader;
    uint32_t uHeaderMemSize, uEventMemSize;
    
    if(pSourceBuf == NULL)
    {
        return(0);
    }

    // calculate the convenience pointers
    pHeader = pSourceBuf;

    // make sure the events haven't grown or shrunk
    uEventMemSize  = pHeader->uNumEvents * sizeof(TelemetryApiEventT);
    uHeaderMemSize = sizeof(TelemetryApiSnapshotHeaderT);
    if ((uHeaderMemSize + uEventMemSize != pHeader->uBufferSize) || (pHeader->uBufferSize > uSourceSize))
    {
        NetPrintf(("telemetryapi: snapshot data is incorrect size - incoming [%u] expected [%u] source [%u] - cannot parse.\n", pHeader->uBufferSize, uEventMemSize, uSourceSize));
        return(0);
    }

    return(pHeader->uNumEvents);
}

/*F********************************************************************************/
/*!
    \Function TelemetryApiSnapshotLoad

    \Description
        Load data from the provided buffer into the current telemetry module.

    \Input *pTelemetryRef - module state
    \Input *pSourceBuf    - buffer to load from
    \Input  uSourceSize   - size of the buffer to load from

    \Output uint32_t     - number of entries imported

    \Notes
        If successful, Will delete any entries currently in the event buffer and 
            replace them with new entries from the source buffer.

    \Version 1.0 01/20/2006 (jfrank) First Version
    \Version 2.0 04/06/2006 (tdills) Moved to TelemetryApi
*/
/********************************************************************************F*/
uint32_t TelemetryApiSnapshotLoad(TelemetryApiRefT *pTelemetryRef, const void *pSourceBuf, uint32_t uSourceSize)
{
    const TelemetryApiSnapshotHeaderT *pHeader;
    const TelemetryApiEventT *pEvents;
    uint32_t uHeaderMemSize, uEventMemSize, uEventChecksum, uNumEventsToCopy;
    
    if ((pTelemetryRef == NULL) || (pSourceBuf == NULL))
    {
        return(0);
    }

    // calculate the convenience pointers
    pHeader = pSourceBuf;
    pEvents = (TelemetryApiEventT *)(pHeader+1);

    // make sure the events haven't grown or shrunk
    uEventMemSize  = pHeader->uNumEvents * sizeof(TelemetryApiEventT);
    uHeaderMemSize = sizeof(TelemetryApiSnapshotHeaderT);
    if ((uHeaderMemSize + uEventMemSize != pHeader->uBufferSize) || (pHeader->uBufferSize > uSourceSize))
    {
        NetPrintf(("telemetryapi: snapshot data is incorrect size - incoming [%u] expected [%u] source [%u] - cannot parse.\n", pHeader->uBufferSize, uEventMemSize, uSourceSize));
        return(0);
    }

    // suck the current events into the buffer
    // first check to make sure the checksum is OK
    uEventChecksum = _TelemetryApiChecksum(pEvents, uEventMemSize);
    if(pHeader->uEventsChecksum != uEventChecksum)
    {
        NetPrintf(("telemetryapi: snapshot data checksum does not match - incoming [0x%08X] expected [0x%08X] - cannot parse.\n", 
            uEventChecksum, pHeader->uEventsChecksum));
        return(0);
    }

    // determine how many events to copy in (we might have too many!)
    uNumEventsToCopy = pHeader->uNumEvents;
    if(uNumEventsToCopy > pTelemetryRef->uEventBufSize)
    {
        NetPrintf(("telemetryapi: incoming snapshot buffer [%u] is larger than the destination buffer [%u] - taking the oldest entries.\n",
            pHeader->uNumEvents, pTelemetryRef->uEventBufSize));
        uNumEventsToCopy = pTelemetryRef->uEventBufSize;
    }

    // and copy the new stuff in
    if(uNumEventsToCopy > 0)
    {
        // wipe out any entries in the current buffer
        _TelemetryApiClearEvents(pTelemetryRef);

        memcpy(pTelemetryRef->Events, pEvents, uNumEventsToCopy * sizeof(TelemetryApiEventT));
        pTelemetryRef->uEventHead = 0;
        pTelemetryRef->uEventTail = uNumEventsToCopy - 1;
        
        // we're not empty - remove the flag
        pTelemetryRef->uEventState &= (~TELEMETRY_EVENTBUFFERSTATE_EMPTY);
        // we have new data so we will eventually send the client a FULLSEND event
        pTelemetryRef->bFullSendPending = TRUE;
        // check for full and call the callback if necessary
        if(uNumEventsToCopy == pTelemetryRef->uEventBufSize)
        {
            pTelemetryRef->uEventState |= TELEMETRY_EVENTBUFFERSTATE_FULL;
            _TelemetryApiTriggerCallback(pTelemetryRef, TELEMETRY_CBTYPE_BUFFERFULL);
        }
    }

    return(uNumEventsToCopy);
}


/*F********************************************************************************/
/*!
    \Function TelemetryApiEventStringFormat

    \Description
        This takes an arbitrary-length string and copies it into a string buffer
        TELEMETRY_EVENTSTRINGSIZE_DEFAULT in length, leaving out any characters
        which won't display in the telemetry log.

    \Input *pDest   - where to copy 
    \Input *pSource - where to copy from (assume NULL terminator)

    \Output char *  - pDest, so this can be used inline with TelemetryApiEvent().

    \Version 1.0 01/26/2006 (jfrank) First Version
    \Version 2.0 04/06/2006 (tdills) Moved to TelemetryApi
*/
/********************************************************************************F*/
char *TelemetryApiEventStringFormat(char *pDest, const char *pSource)
{
    char *pDestPtr = pDest;
    uint32_t uNumCharsCopied = 0;
    
    // walk through the source until we hit a NULL or we run out of space
    // copying valid characters along the way
    while(*pSource != '\0')
    {
        if(TELEMETRY_EVENTSTRINGCHAR_VALID(*pSource))
        {
            *pDestPtr++ = *pSource;
            uNumCharsCopied++;
            if(uNumCharsCopied == (TELEMETRY_EVENTSTRINGSIZE_DEFAULT-1))
            {
                // we've copied enough - quit now
                break;
            }
        }
        pSource++;
    }

    // don't forget to null terminate
    *pDestPtr = '\0';
    return(pDest);
}


/*F********************************************************************************/
/*!
    \Function TelemetryApiStringRight

    \Description
        Returns a pointer to the rightmost TELEMETRY_EVENTSTRINGSIZE_DEFAULT 
        bytes of a string, or just the pointer to the string if less than
        TELEMETRY_EVENTSTRINGSIZE_DEFAULT bytes.

    \Input *pString      - string to use

    \Output const char * - the pointer to the rightmost N bytes of the string, or
                            the string itself if less than N bytes.

    \Version 1.0 01/27/2006 (jfrank) First Version
    \Version 2.0 04/06/2006 (tdills) Moved to TelemetryApi
*/
/********************************************************************************F*/
const char *TelemetryApiStringRight(const char *pString)
{
    uint32_t uLength;

    // check for null
    if(pString == NULL)
        return(NULL);

    // see if we need to do anything (leave room for a terminator)
    uLength = (uint32_t)strlen(pString);
    if(uLength < (TELEMETRY_EVENTSTRINGSIZE_DEFAULT-1))
        return(pString); // string fits without adjustment

    // otherwise, return a pointer to a shorter string
    return(pString + uLength - TELEMETRY_EVENTSTRINGSIZE_DEFAULT + 1);
}



/*F********************************************************************************/
/*!
    \Function TelemetryApiBeginTransaction

    \Description
        Starts a transaction for transferring large data streams to the 
        Telemetry server.

    \Input *pTelemetryRef   - Module state
    \Input *pCallback       - Callback function to notify
    \Input *pUserData       - data that will be returned in the call to the callback

    \Output int32_t         - TC_INV_PARAM if pTelemetryRef is NULL;
                              TC_BUSY if another begin transaction is currently in progress;
                              TC_NOT_CONNECTED if you are not currently connected to the host;
                              TC_MISSING_PARAM if Authent has not been called;
                              TC_OKAY if succeeded.

    \Version 1.0 04/17/2006 (tdills) First Version
*/
/********************************************************************************F*/
int32_t TelemetryApiBeginTransaction(TelemetryApiRefT *pTelemetryRef, TelemetryApiBeginTransactionCBT *pCallback, void *pUserData)
{
    if (pTelemetryRef == NULL)
    {
        return(TC_INV_PARAM);
    }
    if (pTelemetryRef->bTransactionBegPending)
    {
        return(TC_BUSY);
    }
    if ((pTelemetryRef->strAuth[0] == '\0') && (pTelemetryRef->bDisableAuthentication == FALSE))
    {
        NetPrintf(("telemetryapi: auth string not set, try calling TelemetryApiAuthent\n"));
        return(TC_MISSING_PARAM);
    }
    if ((!pTelemetryRef->bCanConnect) || (pTelemetryRef->pAries == NULL))
    {
        return(TC_NOT_CONNECTED);
    }

    // make sure we only have 1 outstanding &beg request at a time
    pTelemetryRef->bTransactionBegPending = TRUE;

    // store the user data and callback pointer
    pTelemetryRef->pTransactionBegCB = pCallback;
    pTelemetryRef->pTransactionBegUserData = pUserData;

    // format the &beg request data
    TagFieldClear(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize);

    // send the data to the server with reconnect/auth logic
    _TelemetryApiSendData(pTelemetryRef, '&beg');

    return(TC_OKAY);
}


/*F********************************************************************************/
/*!
    \Function TelemetryApiSendTransactionData

    \Description
        Send data for a transaction that has been started using 
        TelemetryApiBeginTransaction.

    \Input *pTelemetryRef   - Module state
    \Input iTransactionId   - the transaction id returned from BeginTransaction
    \Input *strData         - data to send
    \Input iDataLength      - amount of data to send

    \Output int32_t         - returns number of bytes sent; 0 if failed.

    \Version 1.0 04/17/2006 (tdills) First Version
*/
/********************************************************************************F*/
int32_t TelemetryApiSendTransactionData(TelemetryApiRefT *pTelemetryRef, int32_t iTransactionId, const char *strData, int32_t iDataLength)
{
    int32_t iSendLength;

    NetPrintf(("telemetryapi: start TelemetryApiSendTransactionData\n"));
    if (pTelemetryRef == NULL)
    {
        return(0);
    }
    if ((!pTelemetryRef->bCanConnect) || (pTelemetryRef->pAries == NULL))
    {
        return(TC_NOT_CONNECTED);
    }

    // determine how much data we will send - 7k max
    iSendLength = (iDataLength > 7168) ? 7168 : iDataLength;
    // send the request
    if (0 == ProtoAriesSend(pTelemetryRef->pAries, '&dat', iTransactionId, strData, iSendLength))
    {
        return(iSendLength);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function TelemetryApiCancelTransaction

    \Description
        Finish the telemetry transaction.

    \Input *pTelemetryRef   - Module state
    \Input iTransactionId   - the transaction id returned from BeginTransaction

    \Output int32_t         - TC_INV_PARAM if pTelemetryRef is NULL; 
                              TC_NOT_CONNECTED if you are not connect to the host;
                              TC_OKAY if succeeded.

    \Version 1.0 06/07/2006 (tdills) First Version
*/
/********************************************************************************F*/
int32_t TelemetryApiCancelTransaction(TelemetryApiRefT *pTelemetryRef, int32_t iTransactionId)
{
    if (pTelemetryRef == NULL)
    {
        return(TC_INV_PARAM);
    }
    if ((!pTelemetryRef->bCanConnect) || (pTelemetryRef->pAries == NULL))
    {
        return(TC_NOT_CONNECTED);
    }

    // use the string buffer now as a tagfield
    TagFieldClear(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize);
    TagFieldSetNumber(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize, "IDENT", iTransactionId);
    TagFieldSetNumber(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize, "DISCARD", 1);

    // send the request
    ProtoAriesSend(pTelemetryRef->pAries, '&end', 0, pTelemetryRef->pTagString, -1);

    return(TC_OKAY);
}


/*F********************************************************************************/
/*!
    \Function TelemetryApiEndTransaction

    \Description
        Finish the telemetry transaction.

    \Input *pTelemetryRef   - Module state
    \Input iTransactionId   - the transaction id returned from BeginTransaction

    \Output int32_t         - TC_INV_PARAM if pTelemetryRef is NULL; 
                              TC_NOT_CONNECTED if you are not connect to the host;
                              TC_OKAY if succeeded.

    \Version 1.0 04/17/2006 (tdills) First Version
*/
/********************************************************************************F*/
int32_t TelemetryApiEndTransaction(TelemetryApiRefT *pTelemetryRef, int32_t iTransactionId)
{
    if (pTelemetryRef == NULL)
    {
        return(TC_INV_PARAM);
    }
    if ((!pTelemetryRef->bCanConnect) || (pTelemetryRef->pAries == NULL))
    {
        return(TC_NOT_CONNECTED);
    }

    // use the string buffer now as a tagfield
    TagFieldClear(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize);
    TagFieldSetNumber(pTelemetryRef->pTagString, pTelemetryRef->uTagStringSize, "IDENT", iTransactionId);

    // send the request
    ProtoAriesSend(pTelemetryRef->pAries, '&end', 0, pTelemetryRef->pTagString, -1);

    return(TC_OKAY);
}

/*F*************************************************************************************************/
/*!
    \Function TelemetryApiInitEvent3

    \Description
        Clear data and set the fields of a TelemetryApiEvent3T object

    \Input  *pEvent - Pointer to the event
    \Input  uModuleID - Category token (4-chars)
    \Input  uGroupID - Sub-category token (4-chars)
    \Input  uStringID - Sub-sub-category token (4-chars)

    \Output int32_t - 0 for success.  A negative result is an error.

    \Version 1.0 04/17/2010 (geoffreyh) First Version
*/
/*************************************************************************************************F*/
int32_t TelemetryApiInitEvent3(TelemetryApiEvent3T *pEvent, uint32_t uModuleID, uint32_t uGroupID, uint32_t uStringID)
{
    if(pEvent == NULL) 
    {
        return(TELEMETRY3_ERROR_NULLPARAM);
    }

    // Clear the structure
    memset(pEvent, 0, sizeof(TelemetryApiEvent3T));

    // Set the Module, Group, and String category
    pEvent->uModuleID = uModuleID;
    pEvent->uGroupID = uGroupID;
    pEvent->uStringID = uStringID;

    return(TC_OKAY);
}

/*F*************************************************************************************************/
/*!
    \Function _TelemetryApiSetupAttribute

    \Description
        Validate fields and create the tag string for an attribute

    \Input  uKey - Attribute name (four-char tag)
    \Input  pTag - Tag string

    \Output int32_t - 0 for success.  A negative result is an error.

    \Version 1.0 04/17/2010 (geoffreyh) First Version
*/
/*************************************************************************************************F*/
int32_t _TelemetryApiSetupAttribute(uint32_t uKey, char *pTag)
{
    if(pTag == NULL) 
    {
        return(TELEMETRY3_ERROR_NULLPARAM);
    }
    if(!TELEMETRY_EVENT_TOKEN_VALID(uKey)) 
    {
        return(TELEMETRY3_ERROR_INVALIDTOKEN);
    }

    pTag[0] = (char)((uKey>>24)&0xFF);
    pTag[1] = (char)((uKey>>16)&0xFF);
    pTag[2] = (char)((uKey>>8)&0xFF);
    pTag[3] = (char)((uKey)&0xFF);
    pTag[4] = '\0';

    return(TC_OKAY);
}

/*F*************************************************************************************************/
/*!
    \Function TelemetryApiEncAttributeChar

    \Description
        Add a single byte attribute to the passed event.

    \Input  *pEvent - Pointer to the event
    \Input  uKey - Attribute name (four-char tag)
    \Input  iValue - Attribute value

    \Output int32_t - 0 for success.  A negative result is an error.

    \Version 1.0 04/17/2010 (geoffreyh) First Version
*/
/*************************************************************************************************F*/
int32_t TelemetryApiEncAttributeChar(TelemetryApiEvent3T *pEvent, uint32_t uKey, char iValue)
{
    char tag[5];
    char val[2];
    int32_t iResult;

    iResult = _TelemetryApiSetupAttribute(uKey,tag);
    if(iResult != TC_OKAY)
    {
        return(iResult);
    }

    if(!TELEMETRY3_EVENTSTRINGCHAR_VALID(iValue)) 
    {
        return(TELEMETRY3_ERROR_INVALIDTOKEN);
    }

    val[0] = iValue;
    val[1] = '\0';

    if(TagFieldSetRaw(pEvent->strEvent,
        TELEMETRY3_EVENTSTRINGSIZE_DEFAULT,
        tag,
        val) <= 0) 
    {
        return(TELEMETRY3_ERROR_FULL);
    }

    return(TC_OKAY);
}

/*F*************************************************************************************************/
/*!
    \Function TelemetryApiEncAttributeLong

    \Description
        Add a 32-bit integer attribute to the passed event.

    \Input  *pEvent - Pointer to the event
    \Input  uKey - Attribute name (four-char tag)
    \Input  iValue - Attribute value

    \Output int32_t - 0 for success.  A negative result is an error.

    \Version 1.0 04/17/2010 (geoffreyh) First Version
*/
/*************************************************************************************************F*/
int32_t TelemetryApiEncAttributeInt(TelemetryApiEvent3T *pEvent, uint32_t uKey, int32_t iValue)
{
    char tag[5];
    int32_t iResult;

    iResult = _TelemetryApiSetupAttribute(uKey,tag);
    if(iResult != TC_OKAY)
    {
        return(iResult);
    }

    if(TagFieldSetNumber(pEvent->strEvent,
        TELEMETRY3_EVENTSTRINGSIZE_DEFAULT,
        tag,
        iValue) <= 0) 
    {
        return(TELEMETRY3_ERROR_FULL);
    }

    return(TC_OKAY);
}

/*F*************************************************************************************************/
/*!
    \Function TelemetryApiEncAttributeLong

    \Description
        Add a 64-bit integer attribute to the passed event.

    \Input  *pEvent - Pointer to the event
    \Input  uKey - Attribute name (four-char tag)
    \Input  iValue - Attribute value

    \Output int32_t - 0 for success.  A negative result is an error.

    \Version 1.1 04/17/2010 (geoffreyh) Second Version 
        No longer uses TagFieldSetNumber64 since this function will abitrarily choose between hex and dec
*/
/*************************************************************************************************F*/
int32_t TelemetryApiEncAttributeLong(TelemetryApiEvent3T *pEvent, uint32_t uKey, int64_t iValue)
{
    int8_t iNeg = 0;
    unsigned char *pData;
    unsigned char strData[20];
    char tag[5];
    int32_t iResult;

    iResult = _TelemetryApiSetupAttribute(uKey,tag);
    if(iResult != TC_OKAY)
    {
        return(iResult);
    }

    // since length is difficult to calculate and formatting requires shift, just
    // format into local buffer and then shift into final buffer
    pData = strData+sizeof(strData);
    *--pData = 0;

    // save the sign
    if (iValue < 0)
    {
        iValue = -iValue;
        iNeg = 1;
    }

    // insert the digits
    for (; iValue > 0; iValue >>= 4)
    {
        *--pData = hex_encode[iValue&15];
    }

    // handle zero case
    if (*pData == 0)
    {
        *--pData = '0';
    }

    // insert the hex indicator
    *--pData = '$';

    // insert sign if needed
    if (iNeg)
    {
        *--pData = '-';
    }

    if(TagFieldSetRaw(pEvent->strEvent,
        TELEMETRY3_EVENTSTRINGSIZE_DEFAULT,
        tag,
        (char *)pData) <= 0) 
    {
        return(TELEMETRY3_ERROR_FULL);
    }

    return(TC_OKAY);
}

/*F*************************************************************************************************/
/*!
    \Function TelemetryApiEncAttributeFloat

    \Description
        Add a floating point attribute to the passed event.

    \Input  *pEvent - Pointer to the event
    \Input  uKey - Attribute name (four-char tag)
    \Input  fValue - Attribute value

    \Output int32_t - 0 for success.  A negative result is an error.

    \Version 1.0 04/17/2010 (geoffreyh) First Version
*/
/*************************************************************************************************F*/
int32_t TelemetryApiEncAttributeFloat(TelemetryApiEvent3T *pEvent, uint32_t uKey, float fValue)
{
    char tag[5];
    int32_t iResult;

    iResult = _TelemetryApiSetupAttribute(uKey,tag);
    if(iResult != TC_OKAY)
    {
        return(iResult);
    }

    if(TagFieldSetFloat(pEvent->strEvent,
        TELEMETRY3_EVENTSTRINGSIZE_DEFAULT,
        tag,
        fValue) <= 0) 
    {
        return(TELEMETRY3_ERROR_FULL);
    }

    return(TC_OKAY);
}

/*F*************************************************************************************************/
/*!
    \Function TelemetryApiEncAttributeString

    \Description
        Add a string attribute to the passed event.

    \Input  *pEvent - Pointer to the event
    \Input  uKey - Attribute name (four-char tag)
    \Input  *pValue - Attribute value

    \Output int32_t - 0 for success.  A negative result is an error.

    \Version 1.0 04/17/2010 (geoffreyh) First Version
*/
/*************************************************************************************************F*/
int32_t TelemetryApiEncAttributeString(TelemetryApiEvent3T *pEvent, uint32_t uKey, const char * pValue)
{
    char *pDest;
    const char *pSrc;
    const char *pEnd;
    char tag[5];
    int32_t iResult;

    if (pValue == NULL)
    {
        return(TELEMETRY3_ERROR_NULLPARAM);
    }

    iResult = _TelemetryApiSetupAttribute(uKey,tag);
    if (iResult != TC_OKAY)
    {
        return(iResult);
    }

    if(TagFieldFind(pEvent->strEvent,tag)) 
    {
        TagFieldDelete(pEvent->strEvent,tag);
    }

    if(TagFieldSetRaw(pEvent->strEvent,
        TELEMETRY3_EVENTSTRINGSIZE_DEFAULT,
        tag,
        "") <= 0) 
    {
        return(TELEMETRY3_ERROR_FULL);
    }

    pDest = pEvent->strEvent + strlen(pEvent->strEvent) - 1;
    pEnd = pEvent->strEvent + TELEMETRY3_EVENTSTRINGSIZE_DEFAULT - 2;
    pSrc = pValue;

    while(*pSrc != '\0' && pDest < pEnd) 
    {
        if(TELEMETRY3_EVENTSTRINGCHAR_VALID(*pSrc))
        {
            *pDest++ = *pSrc++;
        } 
        else 
        {
            // if it's illegal, replace it
            *pDest++ = TELEMETRY_ILLEGALEVENTSTRINGCHAR_REPLACEMENT; 
            pSrc++;
        }
    }
    *pDest++ = '\n';
    *pDest = '\0';

    if(*pSrc != '\0') {
        TagFieldDelete(pEvent->strEvent,tag);  // didn't make it to the end of the string
        return(TELEMETRY3_ERROR_FULL);
    }

    return(TC_OKAY);
}

/*F*************************************************************************************************/
/*!
    \Function _TelemetryApiStreamEncAttributeChar

    \Description
        Add a single byte attribute to the passed event.

    \Input  *pStart - Start of the string
    \Input  uKey - Attribute name (four-char tag)
    \Input  cValue - Attribute value
    \Input  uMaxSize - Maximum remaining length in the buffer

    \Output int32_t - 0 for success.  A negative result is an error.

    \Version 1.0 04/17/2010 (amakoukji) First Version
*/
/*************************************************************************************************F*/
int32_t _TelemetryApiStreamEncAttributeChar(char *pStart, uint32_t uKey, char cValue, uint32_t uMaxSize)
{
    char tag[5];
    char val[2];
    int32_t iResult;

    iResult = _TelemetryApiSetupAttribute(uKey, tag);
    if(iResult != TC_OKAY)
    {
        return(iResult);
    }

    if(!TELEMETRY3_EVENTSTRINGCHAR_VALID(cValue)) 
    {
        return(TELEMETRY3_ERROR_INVALIDTOKEN);
    }

    val[0] = cValue;
    val[1] = '\0';

    if(TagFieldSetRaw(pStart,
        uMaxSize,
        tag,
        val) <= 0) 
    {
        return(TELEMETRY3_ERROR_FULL);
    }

    return(TC_OKAY);
}

/*F*************************************************************************************************/
/*!
    \Function _TelemetryApiStreamEncAttributeInt

    \Description
        Add a 32-bit integer attribute to the passed event.

    \Input  *pStart - Start of the string
    \Input  uKey - Attribute name (four-char tag)
    \Input  iValue - Attribute value
    \Input  uMaxSize - Maximum remaining length in the buffer

    \Output int32_t - 0 for success.  A negative result is an error.

    \Version 1.0 04/17/2010 (amakoukji) First Version
*/
/*************************************************************************************************F*/
int32_t _TelemetryApiStreamEncAttributeInt(char *pStart, uint32_t uKey, int32_t iValue, uint32_t uMaxSize)
{
    char tag[5];
    int32_t iResult;

    iResult = _TelemetryApiSetupAttribute(uKey,tag);
    if(iResult != TC_OKAY)
    {
        return(iResult);
    }

    if(TagFieldSetNumber(pStart,
        uMaxSize,
        tag,
        iValue) <= 0) 
    {
        return(TELEMETRY3_ERROR_FULL);
    }

    return(TC_OKAY);
}

/*F*************************************************************************************************/
/*!
    \Function _TelemetryApiStreamEncAttributeLong

    \Description
        Add a 64-bit integer attribute to the passed event.

    \Input  *pStart - Start of the string
    \Input  uKey - Attribute name (four-char tag)
    \Input  lValue - Attribute value
    \Input  uMaxSize - Maximum remaining length in the buffer

    \Output int32_t - 0 for success.  A negative result is an error.

    \Version 1.1 04/17/2010 (amakoukji) Second Version 
        No longer uses TagFieldSetNumber64 since this function will abitrarily choose between hex and dec
*/
/*************************************************************************************************F*/
int32_t _TelemetryApiStreamEncAttributeLong(char *pStart, uint32_t uKey, int64_t lValue, uint32_t uMaxSize)
{
    int8_t iNeg = 0;
    unsigned char *pData;
    unsigned char strData[20];
    char tag[5];
    int32_t iResult;

    iResult = _TelemetryApiSetupAttribute(uKey,tag);
    if(iResult != TC_OKAY)
    {
        return(iResult);
    }

    // since length is difficult to calculate and formatting requires shift, just
    // format into local buffer and then shift into final buffer
    pData = strData+sizeof(strData);
    *--pData = 0;

    // save the sign
    if (lValue < 0)
    {
        lValue = -lValue;
        iNeg = 1;
    }

    // insert the digits
    for (; lValue > 0; lValue >>= 4)
    {
        *--pData = hex_encode[lValue&15];
    }

    // handle zero case
    if (*pData == 0)
    {
        *--pData = '0';
    }

    // insert the hex indicator
    *--pData = '$';

    // insert sign if needed
    if (iNeg)
    {
        *--pData = '-';
    }

    if(TagFieldSetRaw(pStart,
        uMaxSize,
        tag,
        (char *)pData) <= 0) 
    {
        return(TELEMETRY3_ERROR_FULL);
    }

    return(TC_OKAY);
}

/*F*************************************************************************************************/
/*!
    \Function _TelemetryApiStreamEncAttributeFloat

    \Description
        Add a floating point attribute to the passed event.

    \Input  *pStart - Start of the string
    \Input  uKey - Attribute name (four-char tag)
    \Input  fValue - Attribute value
    \Input  uMaxSize - Maximum remaining length in the buffer

    \Output int32_t - 0 for success.  A negative result is an error.

    \Version 1.0 04/17/2010 (amakoukji) First Version
*/
/*************************************************************************************************F*/
int32_t _TelemetryApiStreamEncAttributeFloat(char *pStart, uint32_t uKey, float fValue, uint32_t uMaxSize)
{
    char tag[5];
    int32_t iResult;

    iResult = _TelemetryApiSetupAttribute(uKey,tag);
    if(iResult != TC_OKAY)
    {
        return(iResult);
    }

    if(TagFieldSetFloat(pStart,
        uMaxSize,
        tag,
        fValue) <= 0) 
    {
        return(TELEMETRY3_ERROR_FULL);
    }

    return(TC_OKAY);
}

/*F*************************************************************************************************/
/*!
    \Function _TelemetryApiStreamEncAttributeString

    \Description
        Add a string attribute to the passed event.

    \Input  *pStart - Start of the string
    \Input  uKey - Attribute name (four-char tag)
    \Input  *pValue - Attribute value
    \Input  uMaxSize - Maximum remaining length in the buffer

    \Output int32_t - 0 for success.  A negative result is an error.

    \Version 1.0 04/17/2010 (amakoukji) First Version
*/
/*************************************************************************************************F*/
int32_t _TelemetryApiStreamEncAttributeString(char *pStart, uint32_t uKey, const char * pValue, uint32_t uMaxSize)
{
    char *pDest;
    const char *pSrc;
    const char *pEnd;
    char tag[5];
    int32_t iResult;

    if (pValue == NULL)
    {
        return(TELEMETRY3_ERROR_NULLPARAM);
    }

    iResult = _TelemetryApiSetupAttribute(uKey,tag);
    if(iResult != TC_OKAY)
    {
        return(iResult);
    }

    if(TagFieldFind(pStart,tag)) 
    {
        TagFieldDelete(pStart,tag);
    }

    if(TagFieldSetRaw(pStart,
        uMaxSize,
        tag,
        "") <= 0) 
    {
        return(TELEMETRY3_ERROR_FULL);
    }

    pDest = pStart + strlen(pStart) - 1;
    pEnd = pStart + uMaxSize - 2;
    pSrc = pValue;

    while(*pSrc != '\0' && pDest < pEnd) 
    {
        if(TELEMETRY3_EVENTSTRINGCHAR_VALID(*pSrc))
        {
            *pDest++ = *pSrc++;
        } 
        else 
        {
            // if it's illegal, replace it
            *pDest++ = TELEMETRY_ILLEGALEVENTSTRINGCHAR_REPLACEMENT; 
            pSrc++;
        }
    }
    *pDest++ = '\n';
    *pDest = '\0';

    if(*pSrc != '\0') {
        TagFieldDelete(pStart,tag);  // didn't make it to the end of the string
        return(TELEMETRY3_ERROR_FULL);
    }

    return(TC_OKAY);
}

/*F*************************************************************************************************/
/*!
    \Function TelemetryApiSubmitEvent3

    \Description
        Submit a telemetry 3 event.  Deep-copies the event into the buffer.

    \Input  *pTelemetryRef  - Telemetry API handle
    \Input  *pEvent - Pointer to the event

    \Output int32_t - Number of bytes written to the buffer.  A negative result is an error.

    \Version 1.0 04/17/2010 (geoffreyh) First Version
*/
/*************************************************************************************************F*/
int32_t TelemetryApiSubmitEvent3(TelemetryApiRefT *pTelemetryRef, TelemetryApiEvent3T *pEvent)
{
    int32_t iEventSize;
    int32_t iEventStrLen;
    int32_t iBufAvail;
    int32_t iTick;
    uint32_t uCharsWritten;

    int8_t bUseServerTime = 0;

    if(pTelemetryRef == NULL || pEvent == NULL) 
    {
        return(TELEMETRY3_ERROR_NULLPARAM);
    }
    if(pTelemetryRef->uEvent3BufferSize == 0) 
    {
        return(TELEMETRY3_ERROR_NOTVER3);
    }
    if(!TELEMETRY_EVENT_TOKEN_VALID(pEvent->uModuleID) || 
        !TELEMETRY_EVENT_TOKEN_VALID(pEvent->uGroupID) ||
        !TELEMETRY_EVENT_TOKEN_VALID(pEvent->uStringID)) 
    {
        return(TELEMETRY3_ERROR_INVALIDTOKEN);
    }

    // Do not submit filtered events
    if(_TelemetryApiFilterEvent3(pTelemetryRef, pEvent) != 1)
    {
#if TELEMETRY_VERBOSE
    NetPrintf(("telemetryapi: Telemetry 3 event filtered out.\n"));
#endif
        return(TELEMETRY3_ERROR_FILTERED);
    }

    // Compute the size of the event
    iEventStrLen = (int32_t)strlen(pEvent->strEvent);
    if(iEventStrLen == 0)
    {
        // No attributes - use a placeholder string
        iEventStrLen = ds_snzprintf(pEvent->strEvent,
            TELEMETRY3_EVENTSTRINGSIZE_DEFAULT,
            "%s,",
            TELEMETRY_EVENTSTRING_EMPTY);
    }

    iEventSize = TELEMETRY3_EVENT_MINSIZE + iEventStrLen;

     // Check if there is enough space in the buffer
    iBufAvail = TelemetryApiEvent3BufferAvailable(pTelemetryRef);
    #if TELEMETRY_VERBOSE
    NetPrintf(("telemetryapi: submit event [%d] bytes available, using additional [%d]\n", iBufAvail, iEventSize));
    #endif
    if(iBufAvail < iEventSize) 
    {
        return(TELEMETRY3_ERROR_FULL);
    }

    // Replace non-printable delimiters with '&'
    TagFieldFormat(pEvent->strEvent,'&');

    // Replace last field terminator with a ',' delimiter
    pEvent->strEvent[iEventStrLen-1] = ',';

    // Compute the event timestamp
    iTick = _TelemetryApiRelativeTick(pTelemetryRef);

    // Make sure the buffer is properly terminated
    if (pTelemetryRef->pEvent3BufferTail > pTelemetryRef->pEvent3BufferHead)
    {
        if (*(pTelemetryRef->pEvent3BufferTail - 1) != ',')
        {
            *(pTelemetryRef->pEvent3BufferTail - 1) = ',';
        }
    }

    // Check whether we should use the server's time
    if (pTelemetryRef->bUseServerTimeOveride >= TELEMETRY_OVERRIDE_TRUE)
    { 
        bUseServerTime = 1;
    }
    else if (pTelemetryRef->bUseServerTimeOveride <= TELEMETRY_OVERRIDE_FALSE)
    {
        bUseServerTime = -1;
    }
    else
    {
        if (pTelemetryRef->bUseServerTime > 0)
        {
            bUseServerTime = 1;
        }
        else
        {
            bUseServerTime = -1;
        }
    }

    // Copy into buffer
    uCharsWritten = ds_snzprintf(
        pTelemetryRef->pEvent3BufferTail, 
        iEventSize, 
        TELEMETRY3_EVENTSTRING_FORMAT,
        bUseServerTime > 0 ? (uint32_t)0xFFFFFFFF : (uint32_t)iTick,  
        (pTelemetryRef->cEntryPrepender == 0) ? TELEMETRY_EVENTSTRING_BUFFERTYPECHARACTER : pTelemetryRef->cEntryPrepender,
        pTelemetryRef->uStep++,
        TELEMETRY_LocalizerTokenPrintCharArray(pEvent->uModuleID),
        TELEMETRY_LocalizerTokenPrintCharArray(pEvent->uGroupID), 
        TELEMETRY_LocalizerTokenPrintCharArray(pEvent->uStringID),
        pEvent->strEvent );

    pTelemetryRef->pEvent3BufferTail += uCharsWritten;

    pTelemetryRef->uNumEvent3Submitted++;

    return((int32_t)uCharsWritten);
}

/*F*************************************************************************************************/
/*!
    \Function _TelemetryApiSubmitEvent3Ex

    \Description
        Submit a telemetry 3 event.  Deep-copies the event into the buffer, then adds additional attributes.
        This is the internal helper function that does the heavy lifting.

    \Input  *pTelemetryRef  - Telemetry API handle
    \Input  *pEvent - Pointer to the event
    \Input  uNumAttributes - The number of attributes to add to the event before submitting
    \Input  *marker  - varargs, always in multiples of 3: the key, the type then the value

    \Output int32_t - Number of bytes written to the buffer.  A negative result is an error.

    \Version 1.0 03/01/2011 (amakoukji) First Version
*/
/*************************************************************************************************F*/
int32_t _TelemetryApiSubmitEvent3Ex( TelemetryApiRefT *pTelemetryRef, TelemetryApiEvent3T *pEvent, uint32_t uNumAttributes, va_list *marker )
{
    int32_t i = (int32_t)uNumAttributes, iError;
    uint32_t uKey, uType;

    char cValue;
    int32_t iValue;
    int64_t lValue;
    float   fValue;
    const char *pValue;

    int32_t iEventSize;
    int32_t iEventStrLen;
    int32_t iBufAvail;
    int32_t iTick;
    uint32_t uCharsWritten, uAttrLength;
    char *pVargAttrStart ,*oldBufferTail = pTelemetryRef->pEvent3BufferTail;
    int32_t bUseServerTime = 0;

    if(pTelemetryRef == NULL || pEvent == NULL) 
    {
        return(TELEMETRY3_ERROR_NULLPARAM);
    }
    if(pTelemetryRef->uEvent3BufferSize == 0) 
    {
        return(TELEMETRY3_ERROR_NOTVER3);
    }
    if(!TELEMETRY_EVENT_TOKEN_VALID(pEvent->uModuleID) || 
        !TELEMETRY_EVENT_TOKEN_VALID(pEvent->uGroupID) ||
        !TELEMETRY_EVENT_TOKEN_VALID(pEvent->uStringID)) 
    {
        return(TELEMETRY3_ERROR_INVALIDTOKEN);
    }

    // Do not submit filtered events
    if(_TelemetryApiFilterEvent3(pTelemetryRef, pEvent) != 1)
    {
#if TELEMETRY_VERBOSE
        NetPrintf(("telemetryapi: Telemetry 3 event filtered out.\n"));
#endif
        return(TELEMETRY3_ERROR_FILTERED);
    }

    // Compute the size of the event, BEFORE additional args
    iEventStrLen = (int32_t)strlen(pEvent->strEvent);
    if(iEventStrLen == 0 && uNumAttributes == 0)
    {
        // No attributes - use a placeholder string
        iEventStrLen = ds_snzprintf(pEvent->strEvent,
            TELEMETRY3_EVENTSTRINGSIZE_DEFAULT,
            "%s,",
            TELEMETRY_EVENTSTRING_EMPTY);
    }

    iEventSize = TELEMETRY3_EVENT_MINSIZE + iEventStrLen;

    // Check if there is enough space in the buffer
    iBufAvail = TelemetryApiEvent3BufferAvailable(pTelemetryRef);
#if TELEMETRY_VERBOSE
    NetPrintf(("telemetryapi: submit event [%d] bytes available, using additional [%d]\n", iBufAvail, iEventSize));
#endif
    if(iBufAvail < iEventSize) 
    {
        return(TELEMETRY3_ERROR_FULL);
    }

    // Replace non-printable delimiters with '&'
    TagFieldFormat(pEvent->strEvent,'&');

    // Replace last field terminator with a ',' delimiter
    if (iEventStrLen > 0)
    {
        if (uNumAttributes <= 0)
        {
            pEvent->strEvent[iEventStrLen-1] = ',';
        }
        else
        {
            pEvent->strEvent[iEventStrLen-1] = '&';
        }
    }

    // Compute the event timestamp
    iTick = _TelemetryApiRelativeTick(pTelemetryRef);

    // Make sure the buffer is properly terminated
    if (pTelemetryRef->pEvent3BufferTail > pTelemetryRef->pEvent3BufferHead)
    {
        if (*(pTelemetryRef->pEvent3BufferTail - 1) != ',')
        {
            *(pTelemetryRef->pEvent3BufferTail - 1) = ',';
        }
    }

    // Check whether we should use the server's time
    if (pTelemetryRef->bUseServerTimeOveride > 0)
    { 
        bUseServerTime = 1;
    }
    else if (pTelemetryRef->bUseServerTimeOveride < 0)
    {
        bUseServerTime = -1;
    }
    else
    {
        if (pTelemetryRef->bUseServerTime > 0)
        {
            bUseServerTime = 1;
        }
        else
        {
            bUseServerTime = -1;
        }
    }

    // Copy into buffer
    uCharsWritten = ds_snzprintf(
        pTelemetryRef->pEvent3BufferTail, 
        iEventSize, 
        TELEMETRY3_EVENTSTRING_FORMAT,
        bUseServerTime > 0 ? (uint32_t)0xFFFFFFFF : (uint32_t)iTick,  
        (pTelemetryRef->cEntryPrepender == 0) ? TELEMETRY_EVENTSTRING_BUFFERTYPECHARACTER : pTelemetryRef->cEntryPrepender,
        pTelemetryRef->uStep++,
        TELEMETRY_LocalizerTokenPrintCharArray(pEvent->uModuleID),
        TELEMETRY_LocalizerTokenPrintCharArray(pEvent->uGroupID), 
        TELEMETRY_LocalizerTokenPrintCharArray(pEvent->uStringID),
        pEvent->strEvent );

    pTelemetryRef->pEvent3BufferTail += uCharsWritten;

    // Now add all the varg attributes
    pVargAttrStart = pTelemetryRef->pEvent3BufferTail;
    // Get all the attributes and try to apply them
    while( i > 0 )
    {
        uKey = va_arg( *marker, uint32_t);
        uType = va_arg( *marker, uint32_t);
        iBufAvail = TelemetryApiEvent3BufferAvailable(pTelemetryRef);

        if (uType == TELEMETRY3_ATTRIBUTE_TYPE_CHAR)
        {
            cValue = (char)va_arg( *marker, int32_t);
            iError = _TelemetryApiStreamEncAttributeChar(pTelemetryRef->pEvent3BufferTail, uKey, cValue, iBufAvail);
            uAttrLength = (int32_t)strlen(pTelemetryRef->pEvent3BufferTail);
            uCharsWritten += uAttrLength;
            pTelemetryRef->pEvent3BufferTail += uAttrLength;
        }
        else if (uType == TELEMETRY3_ATTRIBUTE_TYPE_INT)
        {
            iValue = va_arg( *marker, int32_t);
            iError = _TelemetryApiStreamEncAttributeInt(pTelemetryRef->pEvent3BufferTail, uKey, iValue, iBufAvail);
            uAttrLength = (int32_t)strlen(pTelemetryRef->pEvent3BufferTail);
            uCharsWritten += uAttrLength;
            pTelemetryRef->pEvent3BufferTail += uAttrLength;
        }
        else if (uType == TELEMETRY3_ATTRIBUTE_TYPE_LONG)
        {
            lValue = va_arg( *marker, int64_t);
            iError = _TelemetryApiStreamEncAttributeLong(pTelemetryRef->pEvent3BufferTail, uKey, lValue, iBufAvail);
            uAttrLength = (int32_t)strlen(pTelemetryRef->pEvent3BufferTail);
            uCharsWritten += uAttrLength;
            pTelemetryRef->pEvent3BufferTail += uAttrLength;
        }
        else if (uType == TELEMETRY3_ATTRIBUTE_TYPE_FLOAT)
        {
            fValue = (float)va_arg( *marker, double);
            iError = _TelemetryApiStreamEncAttributeFloat(pTelemetryRef->pEvent3BufferTail, uKey, fValue, iBufAvail);
            uAttrLength = (int32_t)strlen(pTelemetryRef->pEvent3BufferTail);
            uCharsWritten += uAttrLength;
            pTelemetryRef->pEvent3BufferTail += uAttrLength;
        }
        else if (uType == TELEMETRY3_ATTRIBUTE_TYPE_STRING)
        {
            pValue = va_arg( *marker, char*);
            iError = _TelemetryApiStreamEncAttributeString(pTelemetryRef->pEvent3BufferTail, uKey, pValue, iBufAvail);
            uAttrLength = (int32_t)strlen(pTelemetryRef->pEvent3BufferTail);
            uCharsWritten += uAttrLength;
            pTelemetryRef->pEvent3BufferTail += uAttrLength;       
        }
        else
        {
            iError = TELEMETRY3_ERROR_INVALIDTOKEN;
        }

        if ( pTelemetryRef->pEvent3BufferTail - oldBufferTail > TELEMETRY_EVENT_MAX_SIZE)
        {
            iError = TELEMETRY3_ERROR_TOOBIG;
        }

        if (iError < 0)
        {
            // Reset the buffer and return the error
            pTelemetryRef->pEvent3BufferTail = oldBufferTail;
            (*pTelemetryRef->pEvent3BufferTail) = '\0';

            return iError;
        }

        --i;
    }

    // Replace non-printable delimiters with '&'
    TagFieldFormat(pVargAttrStart,'&');
    *(pTelemetryRef->pEvent3BufferTail - 1) = ',';

    pTelemetryRef->uNumEvent3Submitted++;

    return((int32_t)uCharsWritten);
}




/*F*************************************************************************************************/
/*!
    \Function TelemetryApiSubmitEvent3Ex

    \Description
        Submit a telemetry 3 event.  Deep-copies the event into the buffer, then adds additional attributes

    \Input  *pTelemetryRef  - Telemetry API handle
    \Input  *pEvent - Pointer to the event
    \Input  uNumAttributes - The number of attributes to add to the event before submitting
    \Input  ...      - varargs, always in multiples of 3: the key, the type then the value

    \Output int32_t - Number of bytes written to the buffer.  A negative result is an error.

    \Version 1.0 03/01/2011 (amakoukji) First Version
*/
/*************************************************************************************************F*/
int32_t TelemetryApiSubmitEvent3Ex( TelemetryApiRefT *pTelemetryRef, TelemetryApiEvent3T *pEvent, uint32_t uNumAttributes, ... )
{
    va_list args;
    int32_t result = TELEMETRY_ERROR_UNKNOWN;
    uint32_t retries = 0;

    va_start(args, uNumAttributes);
    result = _TelemetryApiSubmitEvent3Ex(pTelemetryRef, pEvent, uNumAttributes, &args);
    va_end(args);

    // Retry while the buffer is full, up to the config's maximum, or the hardcoded max, whichever comes first
    while (result == TELEMETRY3_ERROR_FULL && retries < TELEMETRY_AUTOMATIC_RETRY_MAX && retries < pTelemetryRef->uSubmit3MaxRetry) 
    {
        result = TelemetryApiControl(pTelemetryRef, 'flsh', 0, NULL);
        ++retries;

        if (result == TELEMETRY_ERROR_SENDFAILED)
        {
            // Network busy, back off
            break;
        }

        va_start(args, uNumAttributes);
        result = _TelemetryApiSubmitEvent3Ex(pTelemetryRef, pEvent, uNumAttributes, &args);
        va_end(args);
    }

    return result;
}
/*F*************************************************************************************************/
/*!
    \Function TelemetryApiSubmitEvent3Long

    \Description
        Submit a telemetry 3 event. Encapsulated all the functionality on of the event creation/ submission . 
        Deep-copies the event into the buffer.

    \Input  *pTelemetryRef  - Telemetry API handle
    \Input  uModule - Module for the telemetry event
    \Input  uGroup - Module for the telemetry event
    \Input  uString - Module for the telemetry event
    \Input  uNumAttributes - The number of attributes to add to the event before submitting
    \Input  ...      - varargs, always in multiples of 3: the key, the type then the value

    \Output int32_t - Number of bytes written to the buffer.  A negative result is an error.

    \Version 1.0 03/01/2011 (amakoukji) First Version
*/
/*************************************************************************************************F*/
int32_t TelemetryApiSubmitEvent3Long( TelemetryApiRefT *pTelemetryRef, int32_t uModule, int32_t uGroup, int32_t uString, uint32_t uNumAttributes, ... )
{
    TelemetryApiEvent3T event;
    int32_t result;
    va_list args;
    uint32_t retries = 0;

    result = TelemetryApiInitEvent3(&event, uModule, uGroup, uString);

    if (result >= 0)
    {
        va_start(args, uNumAttributes);
        result = _TelemetryApiSubmitEvent3Ex(pTelemetryRef, &event, uNumAttributes, &args);
        va_end(args);

        // Retry while the buffer is full, up to the config's maximum, or the hardcoded max, whichever comes first
        while (result == TELEMETRY3_ERROR_FULL && retries < TELEMETRY_AUTOMATIC_RETRY_MAX && retries < pTelemetryRef->uSubmit3MaxRetry) 
        {
            result = TelemetryApiControl(pTelemetryRef, 'flsh', 0, NULL);
            ++retries;

            if (result == TELEMETRY_ERROR_SENDFAILED)
            {
                // Network busy, back off
                break;
            }

            va_start(args, uNumAttributes);
            result = _TelemetryApiSubmitEvent3Ex(pTelemetryRef, &event, uNumAttributes, &args);
            va_end(args);
        }

    }

    return result;
}


/*F********************************************************************************/
/*!
    \Function TelemetryApiEvent3BufferAvailable

    \Description
        Get the current space available for buffering telemetry 3 events.

    \Input  *pTelemetryRef   - Telemetry API handle

    \Output int32_t         - Number of unused bytes in the buffer.
                              0 if API is not version 3.

    \Version 1.0 04/17/2010 (geoffreyh) First Version
*/
/********************************************************************************F*/
int32_t TelemetryApiEvent3BufferAvailable(TelemetryApiRefT *pTelemetryRef)
{
    if(pTelemetryRef == NULL) 
    {
        return(0);
    }
    if(pTelemetryRef->uEvent3BufferSize == 0) 
    {
        return(0);
    }
    return(pTelemetryRef->uEvent3BufferSize - (pTelemetryRef->pEvent3BufferTail - pTelemetryRef->pEvent3BufferHead) - TELEMETRY3_RESERVE_SIZE);
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiRelativeTick

    \Description
        Computes the relative tick of the event based on the current NetTick and login NetTick
        Checks if NetTick has wrapped around, and adds an offset to compensate.

    \Input  *pTelemetryRef   - Telemetry API handle

    \Output int32_t         - Returns a tick delta to send to the server

    \Version 1.0 07/23/2010 (geoffreyh) First Version
*/
/********************************************************************************F*/
int32_t _TelemetryApiRelativeTick(TelemetryApiRefT *pTelemetryRef)
{
    int32_t iTickDiff, iCurrentTick;

    // Get the current NetTick in seconds
    iCurrentTick = (signed)(NetTick()/1000);

    // Check if less than the previous tick.  This is used to detect wraparound.
    iTickDiff = iCurrentTick - pTelemetryRef->iLastTick;
    if(iTickDiff < 0)
    {
        pTelemetryRef->iWrapTime += (((signed)(0xFFFFFFFF/1000)) - pTelemetryRef->iTickStartTime);
        pTelemetryRef->iTickStartTime = 0;
    }
    iTickDiff = (iCurrentTick - pTelemetryRef->iTickStartTime) + pTelemetryRef->iWrapTime;
    pTelemetryRef->iLastTick = iCurrentTick;
    return iTickDiff;
}

/*F********************************************************************************/
/*!
    \Function _TelemetryApiScrubString

    \Description
        Remove invalid telemetry characters from a string

    \Input  *pStr - string to scrub, must be null-terminated

    \Version 01/20/2006 (geoffreyh)
*/
/********************************************************************************F*/
static void _TelemetryApiScrubString(char *pStr)
{
    if(pStr)
    {
        char *pData = pStr;
        while(*pData != '\0')
        {
            if(!TELEMETRY3_EVENTSTRINGCHAR_VALID(*pData))
            {
                *pData = TELEMETRY_ILLEGALEVENTSTRINGCHAR_REPLACEMENT;
            }
            pData++;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function TelemetryApiGetSessionID

    \Description
        Retrieve the Session ID from the TelemetryApiRefT

    \Input  *pTelemetryRef   - Telemetry API handle

    \Output const char *     - Returns a pointer to the Session ID string.  Returns null if pTelemetryRef is invalid.

    \Version 1.0 07/23/2010 (geoffreyh) First Version
*/
/********************************************************************************F*/
const char *TelemetryApiGetSessionID(TelemetryApiRefT *pTelemetryRef)
{
    if(pTelemetryRef == NULL) 
    {
        return(NULL);
    }

    return pTelemetryRef->strSessionID;
}

/*F********************************************************************************/
/*!
    \Function TelemetryApiSetSessionID

    \Description
        Save the Session ID for the TelemetryApiRefT

    \Input  *pTelemetryRef   - Telemetry API handle
    \Input  *pSessID         - Session ID string

    \Output int32_t          - Returns TC_OKAY if successful.

    \Version 1.0 07/23/2010 (geoffreyh) First Version
*/
/********************************************************************************F*/
int32_t TelemetryApiSetSessionID(TelemetryApiRefT *pTelemetryRef, const char *pSessID)
{
    if(pTelemetryRef == NULL || pSessID == NULL) 
    {
        return(TELEMETRY3_ERROR_NULLPARAM);
    }

    ds_snzprintf(pTelemetryRef->strSessionID,sizeof(pTelemetryRef->strSessionID),pSessID);

    return TC_OKAY;
}


/*F********************************************************************************/
/*!
    \Function TelemetryApiRemoveFromFront

    \Description
        Removes iNumEvents entries from the front of the list.
        Private to the telemetryapi module.

    \Input *pTelemetryRef  - module state
    \Input iNumEvents      - number of entries to remove

    \Version 1.0 12/13/2010 (geoffreyh) First Version
*/
/********************************************************************************F*/
void _TelemetryApiRemoveFromFront(TelemetryApiRefT *pTelemetryRef, int32_t iNumEvents)
{
    // this is an externally-exposed function, so check for errors
    if (pTelemetryRef == NULL)
        return;

    // see if we're already empty
    if (pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_EMPTY)
        return;

    // mark the buffer as "not full"
    pTelemetryRef->uEventState &= (~TELEMETRY_EVENTBUFFERSTATE_FULL);

    // while the buffer is not empty (we know there is at least one event)
    while(iNumEvents > 0)
    {
#if TELEMETRY_DEBUG
        // clear the memory slot for the deleted entry
        memset(&(pTelemetryRef->Events[pTelemetryRef->uEventHead]), 0, sizeof(TelemetryApiEvent));
#endif
        iNumEvents--;

        // If tail == head, we just popped the last event
        if(pTelemetryRef->uEventTail == pTelemetryRef->uEventHead)
        {
            pTelemetryRef->uEventState |= TELEMETRY_EVENTBUFFERSTATE_EMPTY;
            _TelemetryApiTriggerCallback(pTelemetryRef, TELEMETRY_CBTYPE_BUFFEREMPTY);
            break;
        }
        else
        {
            // adjust the head pointer
            pTelemetryRef->uEventHead++;
            if (pTelemetryRef->uEventHead == pTelemetryRef->uEventBufSize)
                pTelemetryRef->uEventHead = 0;
        }
    }
}


/*F********************************************************************************/
/*!
    \Function _TelemetryApiGetFromFront

    \Description
        Use to retrieve a TelemetryApiEvent in the queue without removing it.
        iIndex is the offset from the front of the queue.

    \Input *pTelemetryRef        - module state
    \Input  uIndex   - index of event in the queue

    \Output TelemetryApiEvent*    - Pointer to a TelemetryApiEvent object.  NULL if index is out of bounds or passed ref is NULL.

    \Version 1.0 12/13/2006 (geoffreyh) First Version
*/
/********************************************************************************F*/
TelemetryApiEventT* _TelemetryApiGetFromFront(TelemetryApiRefT *pTelemetryRef, uint32_t uIndex)
{
    TelemetryApiEventT *pEvent;
    uint32_t uTrueIndex;

    // this is an externally-exposed function, so check for errors
    if (pTelemetryRef == NULL)
        return(NULL);

    // see if we're empty
    if (pTelemetryRef->uEventState & TELEMETRY_EVENTBUFFERSTATE_EMPTY)
        return(NULL);

    // get index of event
    uTrueIndex = pTelemetryRef->uEventHead + uIndex;
    if(uTrueIndex >= pTelemetryRef->uEventBufSize)
    {
        // WRAP
        uTrueIndex -= pTelemetryRef->uEventBufSize;
        if(uTrueIndex > pTelemetryRef->uEventTail || uTrueIndex >= pTelemetryRef->uEventHead)
            return(NULL);   //out of bounds
    }
    else
    {
        // NO WRAP
        if(uTrueIndex > pTelemetryRef->uEventTail && pTelemetryRef->uEventTail >= pTelemetryRef->uEventHead)
            return(NULL);   //out of bounds
    }
    
    pEvent = &(pTelemetryRef->Events[uTrueIndex]);

    // return the event pointer, NULL or not
    return(pEvent);
}

void TelemetryApiSetLoggerCallback( TelemetryApiRefT *pTelemetryRef, TelemetryApiCallback2T *pLoggerCallback )
{
    pTelemetryRef->pLoggerCallback = pLoggerCallback;
}

void TelemetryApiSetTransactionEndCallback( TelemetryApiRefT *pTelemetryRef, TelemetryApiCallback2T *pTEndCallback )
{
    pTelemetryRef->pTransactionEndCallback = pTEndCallback;
}

void TelemetryApiSetTransactionDataCallback(TelemetryApiRefT *pTelemetryRef, TelemetryApiCallback2T *pTEndCallback )
{
    pTelemetryRef->pTransactionDataCallback = pTEndCallback;
}

/*H********************************************************************************/
/*!
    \File telemetryapi.h

    \Description
        This module provides games a way to transmit telemetry and game usage
        information to a central server for later research and study by game teams
        and production.

    \Notes
        The idea is to allow a lightweight client interface which collects and stores
        data and then periodically sends it to the server in a compressed format. The 
        server then converts the data into Common Log Format and sends it to the log 
        processor. The log processor will then write it into a local log file which is
        then periodically transferred down to a Dirtysock server where analysis tools 
        can be used to analyze the data. In order to prevent log overflow, only the 
        last 7 days of data will be maintained on a rotating basis.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 11/10/2004 (jfrank) First Version
    \Version 2.0 01/06/2006 (jfrank) Second Version
*/
/********************************************************************************H*/

#ifndef _telemetry_h
#define _telemetry_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*! The event string is limited this year to this many bytes
    and should include a NULL terminator. */
#define TELEMETRY_EVENTSTRINGSIZE_DEFAULT     (16)

// Extended event type
#define TELEMETRY3_EVENTSTRINGSIZE_DEFAULT    (256)

//! Default size for telemetry event 3 buffer
#define TELEMETRY3_BUFFERSIZE                 (4096)

//! Telemetry session ID length
#define TELEMETRY_SESSION_ID_LEN              (32)

// errors
#define TELEMETRY_ERROR_UNKNOWN               (-1)    //!< unknown error
#define TELEMETRY_ERROR_BUFFERFULL            (-2)    //!< an entry was dropped b/c of full buffer
#define TELEMETRY_ERROR_SENDFAILED            (-8)    //!< Indicates send failed due to ProtoAries buffer overflow.  Retry later to send the data.
#define TELEMETRY_ERROR_NOTCONNECTED          (-9)    //!< Indicates send failed because the underlying connection with the server is gone.

// version 3 errors (always negative)
#define TELEMETRY3_ERROR_FULL                 (-3)    //!< TelemetryApiEvent3T is out of storage
#define TELEMETRY3_ERROR_NOTVER3              (-4)    //!< API not initialized with TelemetryApiCreate3, or else uNumEvent3  was set to 0
#define TELEMETRY3_ERROR_INVALIDTOKEN         (-5)    //!< Token contains invalid characters
#define TELEMETRY3_ERROR_NULLPARAM            (-6)    //!< Passed parameter is NULL
#define TELEMETRY3_ERROR_FILTERED             (-7)    //!< The event was filtered by a filter rule
#define TELEMETRY3_ERROR_TOOBIG               (-10)   //!< The event was over 4kb in size

#define TC_OKAY                0
#define TC_MISSING_PARAM       -2       //!< Missing parameter(s)
#define TC_INV_PARAM           -3       //!< Invalid parameter(s)
#define TC_BUSY                -4       //!< Server is busy
#define TC_NOT_CONNECTED       -5       //!< Client not connected

/*** Macros ***********************************************************************/

#define TelemetryApiCreate3(_uNumEvents, _eBufferType)  TelemetryApiCreateEx(_uNumEvents, _eBufferType, TELEMETRY3_BUFFERSIZE)

/*** Type Definitions *************************************************************/

//! reference state passed to each function
typedef struct TelemetryApiRefT TelemetryApiRefT;

//! each event to be logged fits into this structure
typedef struct TelemetryApiEventT
{
    uint32_t uModuleID;             //!< the module this event belongs to
    uint32_t uGroupID;              //!< group, within the module, where the event occurred
    int32_t  iTimestamp;            //!< timestamp when the event occurred. Can be negative as it is in server relative time
    char     strEvent[TELEMETRY_EVENTSTRINGSIZE_DEFAULT];     //!< short descriptive string
	uint32_t uStep;					//!< step or sequence number
} TelemetryApiEventT;

//! the buffer can be one of these types
typedef enum TelemetryApiBufferTypeE
{
    TELEMETRY_BUFFERTYPE_FILLANDSTOP = 0,     //!< original implementation - fill and then stop taking events
    TELEMETRY_BUFFERTYPE_CIRCULAROVERWRITE    //!< wrap around and overwrite old events as necessary
} TelemetryApiBufferTypeE;

//! the different types of callbacks offerred
typedef enum TelemetryApiCBTypeE
{
    TELEMETRY_CBTYPE_BUFFERFULL = 0,          //!< buffer is full (every time an event occurs and the buf is full)
    TELEMETRY_CBTYPE_BUFFEREMPTY,             //!< buffer goes empty (something happens and the buffer goes empty)
    TELEMETRY_CBTYPE_FULLSENDCOMPLETE,        //!< triggered when the buffer is empty after a send to the lobby server
    TELEMETRY_CBTYPE_TOTAL                    //!< do not use - for internal calculations only
} TelemetryApiCBTypeE;

//! telemetry attribute types
typedef enum TelemetryApiAttributeTypeE
{
    TELEMETRY3_ATTRIBUTE_TYPE_CHAR = 1,
    TELEMETRY3_ATTRIBUTE_TYPE_INT,
    TELEMETRY3_ATTRIBUTE_TYPE_LONG,
    TELEMETRY3_ATTRIBUTE_TYPE_FLOAT,
    TELEMETRY3_ATTRIBUTE_TYPE_STRING,
} TelemetryApiAttributeTypeE;

//! telemetry server side timestamp override types
typedef enum TelemetryServerTimeStampOverrideTypeE
{
    TELEMETRY_OVERRIDE_FALSE = -1,
    TELEMETRY_OVERRIDE_DEFAULT = 0,
    TELEMETRY_OVERRIDE_TRUE = 1
} TelemetryServerTimeStampOverrideTypeE;

//! telemetry callback function prototype
typedef void (TelemetryApiCallbackT)(TelemetryApiRefT *pRef, void *pUserData); 

//! telemetry begin transaction callback function prototype
typedef void (TelemetryApiBeginTransactionCBT)(TelemetryApiRefT *pRef, void *pUserData, int32_t iTransactionId);

typedef void (TelemetryApiCallback2T)(TelemetryApiRefT *pRef, int32_t iErrorCode); 

//! each event to be logged fits into this structure
typedef struct TelemetryApiEvent3T
{
    uint32_t uModuleID;                                        //!< the module this event belongs to
    uint32_t uGroupID;                                         //!< sub-category of module
    uint32_t uStringID;                                        //!< sub-category of group
    char     strEvent[TELEMETRY3_EVENTSTRINGSIZE_DEFAULT];     //!< buffer for event attributes, null-terminated
	uint32_t uStep;	                                           //!< step or sequence number
} TelemetryApiEvent3T;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the logger instance
TelemetryApiRefT *TelemetryApiCreate(uint32_t uNumEvents, TelemetryApiBufferTypeE eBufferType);

// extended version for telemetry 3.0
TelemetryApiRefT *TelemetryApiCreateEx(uint32_t uNumEvents, TelemetryApiBufferTypeE eBufferType, uint32_t uEvent3BufSize);

// destroy the logger instance
void TelemetryApiDestroy(TelemetryApiRefT *pRef);

// disconnect from the Lobby - call this before disconnecting from lobby server
void TelemetryApiDisconnect(TelemetryApiRefT *pRef);

// send authentication data to the telemetry api
int32_t TelemetryApiAuthent(TelemetryApiRefT *pRef, const char *pAuth);

// anonymous version (encodes a token with empty PII fields)
int32_t TelemetryApiAuthentAnonymous(TelemetryApiRefT *pTelemetryRef, const char *pDestIP, int32_t iDestPort, 
                                     uint32_t uLocale, const char *pDomain, uint32_t uTime, const char *pPlayerID);

// pseudo-authenticated version ( PII fields can be spoofed by client )
int32_t TelemetryApiAuthentAnonymousEx(TelemetryApiRefT *pTelemetryRef, const char *pDestIP, int32_t iDestPort, uint32_t uLocale,
                                      const char *pDomain, uint32_t uTime, const char *pPlayerID, const char *pMAC, int64_t uNucleusPersonaID);

// Perform needed background processing
int32_t TelemetryApiUpdate(TelemetryApiRefT *pRef);

#if DIRTYCODE_LOGGING
// print a telemetry event to debug output
void TelemetryApiEventPrint  (TelemetryApiEventT *pEvent, uint32_t uEventNum);

// print all telemetry events to debug output
void TelemetryApiEventsPrint (TelemetryApiRefT *pRef);

// print a telemetry filter to debug output
void TelemetryApiFiltersPrint(TelemetryApiRefT *pRef);
#endif

// register a callback (send a NULL callback function pointer to unregister)
void TelemetryApiSetCallback(TelemetryApiRefT *pRef, TelemetryApiCBTypeE eType, TelemetryApiCallbackT *pCallback, void *pUserData);

void TelemetryApiSetLoggerCallback(TelemetryApiRefT *pTelemetryRef, TelemetryApiCallback2T *pLoggerCallback);
void TelemetryApiSetTransactionEndCallback( TelemetryApiRefT *pTelemetryRef, TelemetryApiCallback2T *pTEndCallback );
void TelemetryApiSetTransactionDataCallback(TelemetryApiRefT *pTelemetryRef, TelemetryApiCallback2T *pTEndCallback );

// connect to the Telemetry Server
int32_t TelemetryApiConnect(TelemetryApiRefT *pRef);

// get various status items from the lobby logger
int32_t TelemetryApiStatus(TelemetryApiRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize);

// set various things in the lobby logger
int32_t TelemetryApiControl(TelemetryApiRefT *pRef, int32_t iKind, int32_t iValue, void* pValue);

// set up the filtering rule.  This comes in from the server from the news file.
void TelemetryApiFilter(TelemetryApiRefT *pRef, const char *pFilterRule);

// log some data
int32_t TelemetryApiEvent(TelemetryApiRefT *pRef, uint32_t uModuleID, uint32_t uGroupID, const char *pEvent);

// log BDID
void TelemetryApiEventBDID(TelemetryApiRefT *pTelemetryRef, const uint8_t *pIdent, int32_t iLength);

// buffer accessor:  remove event from front and return the removed event
TelemetryApiEventT *TelemetryApiPopFront (TelemetryApiRefT *pRef, TelemetryApiEventT *pEvent);

// buffer accessor:  remove event from back and return the removed event
TelemetryApiEventT *TelemetryApiPopBack  (TelemetryApiRefT *pRef, TelemetryApiEventT *pEvent);

// buffer accessor:  add event to front
int32_t TelemetryApiPushFront(TelemetryApiRefT *pRef, const TelemetryApiEventT *pEvent);

// buffer accessors:  add event to back
int32_t TelemetryApiPushBack (TelemetryApiRefT *pRef, const TelemetryApiEventT *pEvent);

// use the provided buffer to store the logger snapshot. Send NULL for *pDestBuf to get the req'd buf size.
uint32_t TelemetryApiSnapshotSave(TelemetryApiRefT *pRef, void *pDestBuf, uint32_t uDestSize);

// determine the number of events contained in a previously saved log buffer
uint32_t TelemetryApiSnapshotSize(const void *pSourceBuf, uint32_t uSourceSize);

// load data from the provided buffer into the current logger
uint32_t TelemetryApiSnapshotLoad(TelemetryApiRefT *pRef, const void *pSourceBuf, uint32_t uSourceSize);

// a utility function to format a normal string into an event string, always returns pDest (for use in-line)
char *TelemetryApiEventStringFormat(char *pDest, const char *pSource);

// a utility function to return the rightmost TELEMETRY_EVENTSTRINGSIZE_DEFAULT bytes of a string
const char *TelemetryApiStringRight(const char *pString);

// begin a transaction to send large pieces of data to the telemetry server
int32_t TelemetryApiBeginTransaction(TelemetryApiRefT *pTelemetryRef, TelemetryApiBeginTransactionCBT *pCallback, void *pUserData);

// send the data in packets to the telemetry server
int32_t TelemetryApiSendTransactionData(TelemetryApiRefT *pTelemetryRef, int32_t iTransactionId, const char *strData, int32_t iDataLength);

// cancel a transaction
int32_t TelemetryApiCancelTransaction(TelemetryApiRefT *pTelemetryRef, int32_t iTranactionId);

// end the transaction
int32_t TelemetryApiEndTransaction(TelemetryApiRefT *pTelemetryRef, int32_t iTransactionId);

//// version 3 methods ////

// get buffer space available for telemetry 3 events
int32_t TelemetryApiEvent3BufferAvailable(TelemetryApiRefT *pTelemetryRef);

// set the fields of a telemetry event
int32_t TelemetryApiInitEvent3(TelemetryApiEvent3T *pEvent, uint32_t uModuleID, uint32_t uGroupID, uint32_t uStringID);

// encode attributes into a event
int32_t TelemetryApiEncAttributeChar(TelemetryApiEvent3T *pEvent, uint32_t uKey, char iValue);
int32_t TelemetryApiEncAttributeInt(TelemetryApiEvent3T *pEvent, uint32_t uKey, int32_t iValue);
int32_t TelemetryApiEncAttributeLong(TelemetryApiEvent3T *pEvent, uint32_t uKey, int64_t iValue);
int32_t TelemetryApiEncAttributeFloat(TelemetryApiEvent3T *pEvent, uint32_t uKey, float fValue);
int32_t TelemetryApiEncAttributeString(TelemetryApiEvent3T *pEvent, uint32_t uKey, const char * pValue);

// submit the event
int32_t TelemetryApiSubmitEvent3(TelemetryApiRefT *pRef, TelemetryApiEvent3T *pEvent);
int32_t TelemetryApiSubmitEvent3Ex(TelemetryApiRefT *pRef,  TelemetryApiEvent3T *pEvent, uint32_t uNumAttributes, ...);
int32_t TelemetryApiSubmitEvent3Long(TelemetryApiRefT *pRef, int32_t uModule, int32_t uGroup, int32_t uString, uint32_t uNumAttributes, ...);


// get or set session ID
const char *TelemetryApiGetSessionID(TelemetryApiRefT *pRef);
int32_t TelemetryApiSetSessionID(TelemetryApiRefT *pRef, const char *pBuf);
#ifdef __cplusplus
}
#endif

#endif // _telemetry_h


/*H*******************************************************************/
/*!
    \File tickerapi.h

    \Description
        This module implements the client API for accessing the ticker server.

    \Copyright
        Copyright (c) Electronic Arts 2003-2004. ALL RIGHTS RESERVED.

    \Version 1.0 12/11/2003 (sbevan) First Version
    \Version 2.0 23/03/2004 (sbevan) TickerApiConnect gains pIdentity argument.
*/
/*******************************************************************H*/

#ifndef _tickerapi_h
#define _tickerapi_h

/*** Include files ***************************************************/

#include "tickerfilter.h"

/*** Defines *********************************************************/

#define TICKER_RECS_DEFAULT 30  // default number of records to queue

//
// The maximum length of ticker message data.
// The size was chosen so as to be able to hold an 8-player buddy score
// where each user is assigned a different color.
//
#define TICKER_MESSAGE_MAX  352
#define TICKER_PERSONA_BUF  20
#define TICKER_EA_UID_BUF   36
#define TICKER_MID_BUF      84

//
// Various error codes
//
#define TICKER_ERR_NONE      0  // no error
#define TICKER_ERR_TIME     -1  // timeout
#define TICKER_ERR_OVERFLOW -2  // buffer overflow
#define TICKER_ERR_NOSUCH   -3  // no such message
#define TICKER_ERR_CONN     -4  // could not connect to server
#define TICKER_ERR_BUSY     -5  // busy processing events, try again later
#define TICKER_ERR_INVP     -6  // invalid argument


/*** Macros **********************************************************/

#define TICKER_LANG(a, b) (((a)<<8)|(b))

/*** Type Definitions ************************************************/

typedef struct TickerFilterT
{
    uint32_t uTop;
    uint32_t uBot;
} TickerFilterT;

/*
  Each TickerRecordT logically holds one or more TickerStringT where
  each TickerStringT represents a separate string.  Note the TickerRecordT
  doesn't physically hold TickerStringT records though.  Instead the
  data that makes up each TickerStringT is encoded in the data field and
  it is unpacked by calling TickerApiGetString.  The encoding scheme
  is an implementation detail that is not visible to clients, interested
  parties can read about it in tickerapi.c.
*/
typedef struct TickerRecordT
{
    int32_t iId;                      // unique identifier for this event    
    int16_t iPriority;                // relative priority (1=highest, 100=lowest)
    int16_t iMsToDisplayFor;
    uint16_t uRepeatCount;            // how many times to display the message.
    uint16_t uRepeatTime;             // how many seconds to repeat for.
    int32_t iState;                   // custom state info based on event kind (uKind)
    TickerFilterT Kind;               // the kind of the message.
    unsigned char uSound;             // non-zero value means play sound

    //
    // All the following are private and not for use by clients.
    //

    // number of separate strings encoded in data
    unsigned char uStringCount;

    uint32_t uExpiryTime;
    // The following hold state between successive calls to TickerApiGetString.
    unsigned char uStringsReadCount;
    unsigned char uNextStringPos;

    unsigned char uDataStart;   // offset of first string in strData
    unsigned char uDataLen;     // length in bytes of all strings in strData
    unsigned char strData[TICKER_MESSAGE_MAX];
} TickerRecordT;


//
// RGBA value for specifying colors provided by the ticker server
//
typedef struct TickerColorT
{
    unsigned char uRed;
    unsigned char uGreen;
    unsigned char uBlue;
    unsigned char uAlpha;
} TickerColorT;

//
// A ticker string is a NUL terminated UTF-8 string along with an RGB color and
// alpha value.
//
typedef struct TickerStringT
{
    TickerColorT Color;
    unsigned char strString[TICKER_MESSAGE_MAX];
} TickerStringT;


/*
  A ticker buddy.
*/
typedef struct TickerBuddyT
{
    unsigned char strName[TICKER_PERSONA_BUF];
    unsigned char uState;       // TICKER_FILTER_STATE_XXX
} TickerBuddyT;


typedef struct TickerApiT TickerApiT;

/*
   Various attributes to identify the user logging in along with what
   preferences they want.
 */
typedef struct TickerIdentityT
{
    char strPersona[TICKER_PERSONA_BUF]; //!< user persona
    char strProduct[32];                   //!< product ID (e.g., MADDEN-2005)
    char strPlatform[32];                  //!< platform identifier (i.e., PS2, XBOX, PC)
    char strSlus[32];                      //!< product SLUS code
    char strLKey[TICKER_EA_UID_BUF];     //!< LKEY from lobby login
    char strMid[TICKER_MID_BUF];         //!< NetConnMAC()
    char strFavouriteTeam[32];             //!< favorite team (recommended but not required)
    TickerFilterT Filter;                  //!< the events the client wants (see ticker_filter.h)
    uint32_t uLocality;                    //!< locality (language, country)
} TickerIdentityT;


/*** Variables *******************************************************/

/*** Functions *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Create a ticker client.
TickerApiT *TickerApiCreate(int32_t iRecordCount);

// Attach a logger to aid in debugging.
void TickerApiDebug(TickerApiT *pTicker, void *pUserData, void (*pLogger)(void *pUserData, const char *pFormat, ...));

// Destroy the instance.  
void TickerApiDestroy(TickerApiT *pTicker);

// Flush all entries from the ticker module.
void TickerApiFlush(TickerApiT *pTicker);

// Called when the EX-ticker string from persona login is available
int32_t TickerApiOnline(TickerApiT *pTicker, const char *pServer);

// Connect to the ticker server using the passing the optional identity info.
int32_t TickerApiConnect(TickerApiT *pTicker, const TickerIdentityT *pIdentity);

// Disconnect from ticker server.
void TickerApiDisconnect(TickerApiT *pTicker);

// Update the user's identity information.
int32_t TickerApiIdentity(TickerApiT *pTicker, const TickerIdentityT *pIdentity);

// Returns a non-zero value if the ticker is connected to the ticker server
int32_t TickerApiIsOnline(TickerApiT *pTicker);

// Insert local item into ticker.
int32_t TickerApiInsert(TickerApiT *pTicker, TickerRecordT *pRecord);

// If there is a record available then copy it to the given record.
int32_t TickerApiQuery(TickerApiT *pTicker, TickerRecordT *pRecord);

// Extract a string from the event.
int32_t TickerApiGetString(TickerApiT *pTicker, TickerRecordT *pEvent, TickerStringT *pString);

// add a string element to a ticker message
int32_t TickerApiAddString(TickerApiT *pTicker, TickerRecordT *pEvent, TickerStringT *pString);

// Perform any needed background processing.
void TickerApiUpdate(TickerApiT *pTicker);

// Query the ticker server module for status information.
int32_t TickerApiStatus(TickerApiT *pTicker, int32_t iSelect, void *pBuf, int32_t iBufSize);

//
// Filter manipulation functions
//

// Build a filter disabling all events
TickerFilterT *TickerApiFilterNone(TickerFilterT *pFilter);

// Build a filter enabling all events
TickerFilterT *TickerApiFilterAll(TickerFilterT *pFilter);

// Build a filter enabling all auto racing events
TickerFilterT *TickerApiFilterGameAutoAll(TickerFilterT *pFilter);

// Build a filter enabling all FIFA leagues
TickerFilterT *TickerApiFilterGameFifaAll(TickerFilterT *pFilter);

// Build a filter enabling events for all sports
TickerFilterT *TickerApiFilterGameAll(TickerFilterT *pFilter);

// Build a filter enabling news events for all sports
TickerFilterT *TickerApiFilterNewsAll(TickerFilterT *pFilter);

// Build a filter enabling only local events (buddy, etc)
TickerFilterT *TickerApiFilterLocal(TickerFilterT *pFilter);

// Enable a particular event type in the given filter
void TickerApiFilterSet(TickerFilterT *pFilter, TickerFilterItemE eItem);

// Check if an event type is enabled in the given filter
int32_t TickerApiFilterIsSet(TickerFilterT *pFilter, TickerFilterItemE eItem);

// Clear an event type from the given filter
void TickerApiFilterClear(TickerFilterT *pFilter, TickerFilterItemE eItem);

// Check if the filter has no events enabled
int32_t TickerApiFilterIsZero(const TickerFilterT *pFilter);

// Compare two filters for equality
int32_t TickerApiFilterIsEqual(const TickerFilterT *pFilt0, const TickerFilterT *pFilt1);

// Perform an AND operation on two filters
TickerFilterT *TickerApiFilterAnd(const TickerFilterT *pSrc0, const TickerFilterT *pSrc1, TickerFilterT *pDst);

// Perform an OR operation on two filters
TickerFilterT *TickerApiFilterOr(const TickerFilterT *pSrc0, const TickerFilterT *pSrc1, TickerFilterT *pDst);

// Perform a NOT operation on a filter
TickerFilterT *TickerApiFilterNot(const TickerFilterT *pSrc, TickerFilterT *pDst);

#ifdef __cplusplus
}
#endif

#endif

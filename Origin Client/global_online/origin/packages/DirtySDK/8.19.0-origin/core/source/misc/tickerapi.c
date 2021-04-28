/*H*******************************************************************/
/*!
    \File tickerapi.c

    \Description
        This module implements the client API for accessing the ticker server.

    \Copyright
        Copyright (c) Electronic Arts 2003-2004. ALL RIGHTS RESERVED.

    \Notes
        The ticker uses UDP to communicate with the Ticker server.
        The motivation for this rather than TCP is that :-

          a) it means the means that the Ticker Server doesn't have to
             have N open sockets where N is the number of online users
             instead it can multiplex over one UDP socket
          b) some amount of packet loss can be accepted in the events
             that the server sends out.

        As a result of b) the client does not acknowledge the ticker
        events it receives from the server.  This keeps that part 
        of the protocol nice and simple.

        Typically once connected all the traffic is one way, from the
        server to the client.  Since the client may be behind NAPT there
        is the potential for the NAPT device timing out the connection due
        to lack of outbound traffic.  Therefore a period 'keep-alive' packet
        is sent to the server to keep the port mapping alive.  The server
        also uses this as an indication that the client is still alive --
        something it cannot know once the user is in game since the client
        will have disconnected from the lobby (the ticker server has no
        connection to the buddy system and hence any presence information).
       
    \Version 1.0 12/11/2003 (sbevan) First Version
    \Version 1.1 16/11/2003 (sbevan) Converted to use a UDP carrier
    \Version 2.0 23/03/2004 (sbevan) TickerApiConnect gains pIdentity argument.
*/
/*******************************************************************H*/

/*** Include files ***************************************************/

#include <stdio.h>              // vsnprintf
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>             // UCHAR_MAX
#include <string.h>             // memcpy/memmove
#include <ctype.h>

#include "dirtysock.h"          // struct in_addr
#include "dirtymem.h"
#include "lobbylang.h"
#include "lobbytagfield.h"      // TagFieldXXX
#include "protoudp.h"
#include "tickerapi.h"

/*** Defines *********************************************************/

#define TICKER_PROTOCOL_VERSION 9

/*** Macros **********************************************************/

//!< internal debug printing
#if DIRTYCODE_LOGGING
#define _TickerApiPrintf(x) _TickerApiPrintfCode x
#else
#define _TickerApiPrintf(x)
#endif

/*** Type Definitions ************************************************/

enum
{
    //
    // The min/max queue size that the user can sensibly ask for. 
    //
    TICKER_RECS_MIN = 3,
    TICKER_RECS_MAX = 255,

    // An artificially high priority that ensures that the event
    // is always added to the rear of the queue.
    //
    TICKER_APPEND_PRIORITY = 1000,

    //
    // The maximum size of a server context sent by the server.
    // Deliberately a power of 4 for alignment purposes.
    //
    TICKER_CONTEXT_MAX = 32,

    TICKER_MSG_EVENT      = 'E',
    TICKER_MSG_HELLO      = 'H',
    TICKER_MSG_KEEP_ALIVE = 'K',

    //
    // The TickerApi has its own queue so there is no need to make
    // the UDP layer do much buffering/queuing, instead it only needs
    // to keep a few packets around.
    //
    TICKER_UDP_QUEUE_SIZE = 4,

    // How many seconds to wait before trying to send a 'hello' packet
    // to the server if nothing has been received.
    //
    TICKER_DEFAULT_HELLO_PERIOD = 5,

    // Max length of a ticker sports header string
    TICKER_SPORTS_HEADER_MAX = 32
};


/*!
   MarshallerT aids in unpacking scalars and strings from a byte stream.
   It simply hides some of the book-keeping that is required.
   Instead of checking the result of each extraction which leads to
   a chain of 'if' statements, all extractions are done as if they
   all succeed and then the state is checked once at the end.
*/
typedef struct MarshallerT
{
    unsigned char* pBuffer;
    uint32_t uSize;
    uint32_t uReadPoint;
    uint32_t uWritePoint;
    int32_t bError;
} MarshallerT;


/*!
   An EventsIndexT is used to form a prioritized linked list of
   events.  See below for more details on exactly how it is used.
 */
struct EventsIndexT
{
    unsigned char uIndex;
    unsigned char uNext;
};

typedef struct EventsIndexT EventsIndexT;

/*!
   The guts of the ticker consists of a sequence of prioritized events.
   Logically the TickerRecordT objects only need to be allocated as
   messages are received from the ticker server.  However, since the
   client has to be able to cope with the maximum number of records
   it simplifies error handling (and potentially reduces fragmentation)
   if all the records are created when the TickerApiT is created.
  
   All the TickerRecordT objects are stored contiguously (pEvents).
   The location at which to extract the next event for the user
   (TickerApiQuery) or insert the a new event (TickerApiInsert) is
   governed by a separate index (pEventsIndex).  The index is logically
   a linked list ordered by event priority.  To keep the space used to
   a minimum the linked list is not implemented with pointers, instead
   one byte integer offsets are used (hence a maximum of 256 events)
   which should bring back memories for any ex-FORTRAN programmers.

   For example, here's what things look like after three events have
   been added, with priorities 4, 6 and 7 respectively :-

                    +-----+-----+-----+-----+-----+-----+-----+-----+
     pEvents ------>|  4  |   6 |   7 |     |     |     |     |     |
                    +-----+-----+-----+-----+-----+-----+-----+-----+
                      ^     ^     ^     ^     ^     ^     ^     ^
                      |     |     |     |     |     |     |     |
                      |     |     |     |     |     |     |     |
                    +-----+-----+-----+-----+-----+-----+-----+-----+
     pEventsIndex ->| 0| 1| 1| 2| 2| 3| 3| 4| 4| 5| 5| 6| 6| 7| 7| 8|
                    +-----+-----+-----+-----+-----+-----+-----+-----+
                      ^ |   ^ |   ^     ^  |  ^  |  ^  |  ^  |  ^
                      | +---+ +---+     |  +--+  +--+  +--+  +--+
                      |                 |
                uPendingEvents     uFreeEvents

   uPendingEvents holds the offset of the head of the priority queue
   and in this case it is at position 0.  uFreeEvents holds the head of
   the list of free index slots which currently is at position 3.

   If a new event with priority 5 is inserted then the event is added
   at the first free slot in pEvents (slot 3 according to uFreeEvents)
   and then the priority list pEventsIndex is re-adjusted so that the
   events positions are: 0 -> 3 -> 1 -> 2 :-

                    +-----+-----+-----+-----+-----+-----+-----+-----+
     pEvents ------>|  4  |   6 |   7 |   5 |     |     |     |     |
                    +-----+-----+-----+-----+-----+-----+-----+-----+
                      ^     ^     ^     ^     ^     ^     ^     ^
                      |     |     |     |     |     |     |     |
                      |     |     |     |     |     |     |     |
                    +-----+-----+-----+-----+-----+-----+-----+-----+
     pEventsIndex ->| 0| 3| 1| 2| 2| 3| 3| 1| 4| 5| 5| 6| 6| 7| 7| 8|
                    +-----+-----+-----+-----+-----+-----+-----+-----+
                      ^ |   ^ |   ^     ^  |  ^  |  ^  |  ^  |  ^
                      | |   | +---+     |  |  |  +--+  +--+  +--+
                      | |   +--------------+  |
                      | |               |     |
                      | +---------------+     |
                uPendingEvents            uFreeEvents


   If now TickerApiQuery is called, then the first event is returned
   (priority 4 at position 0) and then added to the free list of events :-

                    +-----+-----+-----+-----+-----+-----+-----+-----+
     pEvents ------>|     |   6 |   7 |   5 |     |     |     |     |
                    +-----+-----+-----+-----+-----+-----+-----+-----+
                      ^     ^     ^     ^     ^     ^     ^     ^
                      |     |     |     |     |     |     |     |
                      |     |     |     |     |     |     |     |
                    +-----+-----+-----+-----+-----+-----+-----+-----+
     pEventsIndex ->| 0| 4| 1| 2| 2| 3| 3| 1| 4| 5| 5| 6| 6| 7| 7| 8|
                    +-----+-----+-----+-----+-----+-----+-----+-----+
                      ^  |  ^ |   ^     ^  |  ^  |  ^  |  ^  |  ^
                      |  |  | +---+     |  |  |  +--+  +--+  +--+
                      |  |  +--------------+  |
                      |  +--------------------+
                      |                 |      
                  uFreeEvents        uPendingEvents  

*/
struct TickerApiT
{
    ProtoUdpT *pUdp;
    //
    // there is no point to pDebugProc being a varargs routing anymore since
    // it is hard to support that an the requirement that it be possible to
    // compile out debug calls in release builds.  Would be simpler just to
    // remove it and replace all calls to NetPrintf.
    //
    void * pDebugData;
    void (* pDebugProc)(void *, const char*, ...);
    TickerRecordT* pEvents;
    EventsIndexT* pEventsIndex;

    // module memory group
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group
    
    uint32_t uKeepAliveTimeout;     //!< when next 'keep-alive' should be sent.
    TickerColorT BackgroundColor;   //!< background color to be used for the ticker display
    uint32_t uHelloTimeout;         //!< when next 'hello' should be sent.
    uint32_t uUid;                  //!< user ID as defined by the server.
    uint32_t uLastServerId;         //!< last event ID that came from server
    uint32_t uCurrentEventTimeout;
    struct sockaddr_in ServerAddr;
    struct sockaddr_in LocalAddr;
    char strContext[TICKER_CONTEXT_MAX];
    TickerIdentityT Identity;
    int16_t iCurrentEventPriority;
    uint16_t uRequestId;
    uint32_t uLocality;            //!< user's chosen locality (language, country)
    unsigned char uEventsCount;         //!< maximum number of events
    unsigned char uPendingEvents;       //!< head of active events list
    unsigned char uFreeEvents;          //!< head of free event list
    unsigned char uPendingEventsCount;  //!< # of active events
    unsigned char uLocalEventsCount;    //!< # of events in queue that are local
    unsigned char uQueueHighWater;      //!< 
    unsigned char uQueueLowWater;       //!< 
    unsigned char uKeepAlivePeriod;     //!< how often in seconds to send "keep-alive"
    unsigned char uHelloPeriod;         //!< how often in seconds to send "hello" if no events are received.
    char strLastSportsHeader[TICKER_SPORTS_HEADER_MAX];
    TickerFilterT ServerFilter;         //!< Filter indicated which events the server will send

};


/*** Private Functions ***********************************************/

#if DIRTYCODE_LOGGING
/*F*******************************************************************/
/*!
    \Function    _TickerApiPrintfCode

    \Description
        Handle debug output

    \Input *pTicker     - reference pointer
    \Input *pFmt        - format string
    \Input ...          - varargs

    \Version 04/04/2004 (sbevan)
*/
/********************************************************************F*/
static void _TickerApiPrintfCode(TickerApiT *pTicker, const char *pFmt, ...)
{
    va_list args;
    char strText[4096];

    va_start(args, pFmt);
    vsnzprintf(strText, sizeof strText, pFmt, args);

    if (pTicker->pDebugProc != NULL)
        pTicker->pDebugProc(pTicker->pDebugData, strText);
    va_end(args);
}
#endif


/*F*******************************************************************/
/*!
    \Function _TickerApiDefaultLogger

    \Description
        This is the default TickerApi logger.
        It just throws away anything that is sent to it.

    \Input *pUserData - ignored.
    \Input *pFormat   - ignored.

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiDefaultLogger(void *pUserData,
                                    const char *pFormat, ...)
{
    // deliberately does nothing.
}


/*F*******************************************************************/
/*!
    \Function _TickerApiMarshallerInit

    \Description
        Initialise the marshaller, must be done before any other
        marshaller functions are used.

    \Input *pMarshaller - the marshaller
    \Input iBufferLen   - the length of the buffer
    \Input *pBuffer     - the buffer from which things will be de-marshalled.

    \Version 12/11/2002 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiMarshallerInit(MarshallerT *pMarshaller,
                                     int32_t iBufferLen, char *pBuffer)
{
    pMarshaller->pBuffer = (unsigned char*)pBuffer;
    pMarshaller->uSize = iBufferLen;
    pMarshaller->uReadPoint = 0;
    pMarshaller->uWritePoint = 0;
    pMarshaller->bError = 0;
}


/*F*******************************************************************/
/*!
    \Function _TickerApiMarshallerGetUChar

    \Description
        Extract an unsigned character

    \Input *pMarshaller - the marshaller

    \Output the unsigned character

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
static unsigned char _TickerApiMarshallerGetUChar(MarshallerT *pMarshaller)
{
    if (!pMarshaller->bError
        && pMarshaller->uReadPoint + sizeof(char) <= pMarshaller->uSize)
    {
        unsigned char c = pMarshaller->pBuffer[pMarshaller->uReadPoint];
        pMarshaller->uReadPoint += sizeof(char);
        return(c);
    }
    else
    {
        pMarshaller->bError = 1;
        return(0);
    }
}


/*F*******************************************************************/
/*!
    \Function _TickerApiMarshallerGetUShort

    \Description
        Extract an uint16_t

    \Input *pMarshaller - the marshaller

    \Output the uint16_t

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
static uint16_t _TickerApiMarshallerGetUShort(MarshallerT *pMarshaller)
{
    if (!pMarshaller->bError 
        && pMarshaller->uReadPoint + sizeof(int16_t) <= pMarshaller->uSize)
    {
        uint32_t c1 = pMarshaller->pBuffer[pMarshaller->uReadPoint+0];
        uint32_t c0 = pMarshaller->pBuffer[pMarshaller->uReadPoint+1];
        pMarshaller->uReadPoint += sizeof(int16_t);
        return((c1<<8)|c0);
    }
    else
    {
        pMarshaller->bError = 1;
        return(0);
    }
}


/*F*******************************************************************/
/*!
    \Function _TickerApiMarshallerGetInt

    \Description
        Extract an integer

    \Input *pMarshaller - the marshaller

    \Output the integer

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
static int32_t _TickerApiMarshallerGetInt(MarshallerT *pMarshaller)
{
    if (!pMarshaller->bError 
        && pMarshaller->uReadPoint + sizeof(int32_t) <= pMarshaller->uSize)
    {
        uint32_t c3 = pMarshaller->pBuffer[pMarshaller->uReadPoint+0];
        uint32_t c2 = pMarshaller->pBuffer[pMarshaller->uReadPoint+1];
        uint32_t c1 = pMarshaller->pBuffer[pMarshaller->uReadPoint+2];
        uint32_t c0 = pMarshaller->pBuffer[pMarshaller->uReadPoint+3];
        pMarshaller->uReadPoint += sizeof(int32_t);
        return((c3<<24)|(c2<<16)|(c1<<8)|c0);
    }
    else
    {
        pMarshaller->bError = 1;
        return(0);
    }
}


/*F*******************************************************************/
/*!
    \Function _TickerApiMarshallerGetString

    \Description
        Extract a string from the marshaller and copy it to the
        provided buffer as long as it does not exceed the maximum
        specified length.

    \Input *pMarshaller - the marshaller
    \Input iMaxLen      - the maximum size of the buffer
    \Input pBuffer      - the buffer to put the extracted string in

    \Notes
       The marshalled string looks like :-

            0         len+1
         +-----+.....+----+
         |     |     |    |
         +-----+.....+----+
         | len |<--len -->|

      Note there is *no* NUL terminator in the input.

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiMarshallerGetString(MarshallerT *pMarshaller,
                                          uint32_t iMaxLen,
                                          char *pBuffer)
{
    uint32_t iStringLen;

    if (pMarshaller->bError 
        || (pMarshaller->uReadPoint + sizeof(char) > pMarshaller->uSize))
    {
        pMarshaller->bError = 1;
        return;
    }
    iStringLen = pMarshaller->pBuffer[pMarshaller->uReadPoint];
    if (iStringLen >= iMaxLen 
        || iStringLen > (pMarshaller->uSize - pMarshaller->uReadPoint-sizeof(char)))
    {
        pMarshaller->bError = 1;
        return;
    }
    memcpy(pBuffer, &pMarshaller->pBuffer[pMarshaller->uReadPoint+sizeof(char)],
           iStringLen);
    pBuffer[iStringLen] = '\0';
    pMarshaller->uReadPoint += iStringLen+sizeof(char);
}


/*F*******************************************************************/
/*!
    \Function _TickerApiMarshallerPutUChar

    \Description
        Add a character

    \Input *pMarshaller - the marshaller
    \Input uChr         - the character to add

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiMarshallerPutUChar(MarshallerT *pMarshaller,
                                         unsigned char uChr)
{
    if (!pMarshaller->bError 
        && pMarshaller->uWritePoint + sizeof(char) <= pMarshaller->uSize)
    {
        pMarshaller->pBuffer[pMarshaller->uWritePoint+0] = uChr;
        pMarshaller->uWritePoint += sizeof(char);
    }
    else
    {
        pMarshaller->bError = 1;
    }
}


/*F*******************************************************************/
/*!
    \Function _TickerApiMarshallerPutUShort

    \Description
        Add an uint16_t

    \Input *pMarshaller - the marshaller
    \Input uShort       - the uint16_t to add.

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiMarshallerPutUShort(MarshallerT *pMarshaller,
                                          uint16_t uShort)
{
    if (!pMarshaller->bError 
        && pMarshaller->uWritePoint + sizeof(int16_t) <= pMarshaller->uSize)
    {
        pMarshaller->pBuffer[pMarshaller->uWritePoint+0] = (uShort>>8)&0xFF;
        pMarshaller->pBuffer[pMarshaller->uWritePoint+1] = (uShort>>0)&0xFF;
        pMarshaller->uWritePoint += sizeof(int16_t);
    }
    else
    {
        pMarshaller->bError = 1;
    }
}


/*F*******************************************************************/
/*!
    \Function _TickerApiMarshallerPutUInt

    \Description
        Add an unsigned integer

    \Input *pMarshaller - the marshaller
    \Input uInt         - the integer to add

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiMarshallerPutUInt(MarshallerT *pMarshaller,
                                        uint32_t uInt)
{
    if (!pMarshaller->bError 
        && pMarshaller->uWritePoint + sizeof(int32_t) <= pMarshaller->uSize)
    {
        pMarshaller->pBuffer[pMarshaller->uWritePoint+0] = (uInt>>24)&0xFF;
        pMarshaller->pBuffer[pMarshaller->uWritePoint+1] = (uInt>>16)&0xFF;
        pMarshaller->pBuffer[pMarshaller->uWritePoint+2] = (uInt>>8)&0xFF;
        pMarshaller->pBuffer[pMarshaller->uWritePoint+3] = (uInt>>0)&0xFF;
        pMarshaller->uWritePoint += sizeof(int32_t);
    }
    else
    {
        pMarshaller->bError = 1;
    }
}


/*F*******************************************************************/
/*!
    \Function _TickerApiMarshallerPutString

    \Description
        Add a string

    \Input *pMarshaller - the marshaller
    \Input *pStr        - the string to add

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiMarshallerPutString(MarshallerT *pMarshaller,
                                          const char *pStr)
{
    int32_t length = (int32_t)strlen(pStr);
    if (!pMarshaller->bError 
        && pMarshaller->uWritePoint + length + 1 <= pMarshaller->uSize)
    {
        pMarshaller->pBuffer[pMarshaller->uWritePoint+0] = length & 0xFF;
        memcpy(&pMarshaller->pBuffer[pMarshaller->uWritePoint+1], pStr, length);
        pMarshaller->uWritePoint += length + 1;
    }
    else
    {
        pMarshaller->bError = 1;
    }
}


/*F*******************************************************************/
/*!
    \Function _TickerApiIsConnected

    \Description
        Determines if TickerApiConnect has been called.

    \Input *pTicker - ticker

    \Output non-zero if it has been called.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
static int32_t _TickerApiIsConnected(TickerApiT *pTicker)
{
    return(pTicker->pUdp != 0);
}


/*F*******************************************************************/
/*!
    \Function _TickerApiEnableKeepAlive

    \Description
        Enable sending periodic 'keep-alive' messages to the ticker server.

    \Input *pTicker - the ticker

    \Version 04/04/2004 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiEnableKeepAlive(TickerApiT *pTicker)
{
    pTicker->uKeepAliveTimeout = NetTick() + pTicker->uKeepAlivePeriod*1000;
}


/*F*******************************************************************/
/*!
    \Function _TickerApiEnableKeepAlive

    \Description
        Disable sending periodic 'keep-alive' messages to the ticker server.

    \Input *pTicker - the ticker

    \Version 04/04/2004 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiDisableKeepAlive(TickerApiT *pTicker)
{
    pTicker->uKeepAliveTimeout = (unsigned)-1;
}


/*F*******************************************************************/
/*!
    \Function _TickerApiEnableHello

    \Description
        Enable sending periodic 'hello' messages to the ticker server.

    \Input *pTicker - the ticker

    \Version 04/04/2004 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiEnableHello(TickerApiT *pTicker)
{
    pTicker->uHelloTimeout = 0;
}


/*F*******************************************************************/
/*!
    \Function _TickerApiUpdateHello

    \Description
        Update the 'hello' timer to mark the next time the 'hello' should be sent.

    \Input *pTicker - the ticker

    \Version 04/04/2004 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiUpdateHello(TickerApiT *pTicker)
{
    pTicker->uHelloTimeout = NetTick() + pTicker->uHelloPeriod*1000;
}


/*F*******************************************************************/
/*!
    \Function _TickerApiDisableHello

    \Description
        Disable sending periodic 'hello' messages to the ticker server.

    \Input *pTicker - the ticker

    \Version 04/04/2004 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiDisableHello(TickerApiT *pTicker)
{
    pTicker->uHelloTimeout = (unsigned)-1;
}


/*F*******************************************************************/
/*!
    \Function _TickerApiInsertIndex

    \Description
        Insert the the index into the correct position based on the
        priority of the event that it points at.

    \Input *pTicker  - the ticker
    \Input iPriority - the priority of the event to be inserted.
    \Input *pNew     - new entry

    \Notes
        This uses the standard approach of inserting an element into 
        a priority queue implemented as a linked list.  The only unusual
        aspect is that a) the lower the numeric value the higher the
        priority and so the test looks the wrong way around and b) the
        linked list is implemented in a Fortran-like fashion using 
        integer (actually unsigned character) indices rather than pointers.

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiInsertIndex(TickerApiT *pTicker, int32_t iPriority, EventsIndexT *pNew)
{
    uint32_t i;
    uint32_t uEventsCount = pTicker->uPendingEventsCount-1;
    unsigned char *pNext = &pTicker->uPendingEvents;
    EventsIndexT *pIndex = &pTicker->pEventsIndex[pTicker->uPendingEvents];

    for (i = 0; i != uEventsCount; i += 1)
    {
        TickerRecordT *pEvent = &pTicker->pEvents[pIndex->uIndex];
        if (iPriority < pEvent->iPriority)
        {
            pNew->uNext = pIndex->uIndex;
            break;
        }
        pNext = &pIndex->uNext;
        pIndex = &pTicker->pEventsIndex[pIndex->uNext];
    }
    *pNext = pNew->uIndex;
}


/*F*******************************************************************/
/*!
    \Function _TickerApiGetNextFreeEvent

    \Description
       Grab the next event.

    \Input *pTicker     - the ticker
    \Input iPriority    - priority

    \Output the event or 0 if there is no space for the event.

    \Notes
       If the queue is not full then the the priority is used to locate
       the correct position in the queue for a new event and the event
       is returned.

       If the queue is full then the last event in the queue is examined
       and if if its priority is lower then the event to be inserted
       it is removed to make way for the new event.

    \Version 1.0 04/04/2004 (sbevan) First Version
*/
/*******************************************************************F*/
static TickerRecordT *_TickerApiGetNextFreeEvent(TickerApiT *pTicker, int32_t iPriority)
{
    EventsIndexT *pFree;
    TickerRecordT *pEvent;

    if (pTicker->uPendingEventsCount == pTicker->uEventsCount)
    {
        uint32_t i;
        uint32_t uEventsCount = pTicker->uPendingEventsCount-1;
        pFree = &pTicker->pEventsIndex[pTicker->uPendingEvents];
        for (i = 0; i != uEventsCount; i += 1)
            pFree = &pTicker->pEventsIndex[pFree->uNext];
        pEvent = &pTicker->pEvents[pFree->uIndex];
        if (iPriority > pEvent->iPriority)
            return(0);
        _TickerApiPrintf((pTicker, "queue full, replacing %u\n", pEvent->iId));
    }
    else
    {
        pFree = &pTicker->pEventsIndex[pTicker->uFreeEvents];
        pTicker->uPendingEventsCount += 1;
        pTicker->uFreeEvents = pFree->uNext;
    }
    _TickerApiInsertIndex(pTicker, iPriority, pFree);
    return(&pTicker->pEvents[pFree->uIndex]);
}


/*F*******************************************************************/
/*!
    \Function _TickerApiReleaseEvent

    \Description
        Release the event/index and put it back in the free event/index
        list.

    \Input *pTicker     - the ticker
    \Input *pIndex      - index

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiReleaseEvent(TickerApiT *pTicker, EventsIndexT *pIndex)
{
    pTicker->uPendingEventsCount -= 1;
    pIndex->uNext = pTicker->uFreeEvents;
    pTicker->uFreeEvents = pIndex->uIndex;
    if (pTicker->uPendingEventsCount == 0)
    {
        // there are no pending events so re-set
        // head of the pending queue to be empty.
        pTicker->uPendingEvents = 0;
    }
}


/*F*******************************************************************/
/*!
    \Function _TickerApiShouldIgnoreEvent

    \Description
        Check if the event should be ignored due to filtering.

    \Input *pTicker - ticker
    \Input *pEvent  - event to check

    \Output non-zero if the event should be ignored.

    \Version 1.0 04/04/2004 (sbevan) First Version
*/
/*******************************************************************F*/
static int32_t _TickerApiShouldIgnoreEvent(TickerApiT *pTicker, TickerRecordT *pEvent)
{
    TickerFilterT Local;
    TickerFilterT Event;
    TickerFilterT Ident;

    TickerApiFilterLocal(&Local);

    // Strip non-local flags off identity and event filters
    TickerApiFilterAnd(&pEvent->Kind, &Local, &Event);
    TickerApiFilterAnd(&pTicker->Identity.Filter, &Local, &Ident);

    // Compare if any bits still match
    TickerApiFilterAnd(&Event, &Ident, &Event);
    return(TickerApiFilterIsZero(&Ident));
}


/*F*******************************************************************/
/*!
    \Function _TickerApiIsServerVisibleFilterChange

    \Description
        Determine if the filter change should affect what the server sends

    \Input of - old filter state
    \Input nf - new filter state

    \Output non-zero if the filter affects the server

    \Notes
        If the user changes the filter then we only want to inform the
        server of the change if it affects what the server will send to 
        the user.  That is, if the user was not getting any server
        generated events and now wants them, or if the user was getting
        server generated events and now does not want them.

    \Version 1.0 05/05/2004 (sbevan) First Version
*/
/*******************************************************************F*/
static int32_t _TickerApiIsServerVisibleFilterChange(const TickerFilterT *of, const TickerFilterT *nf)
{
    TickerFilterT NotLocal;
    TickerFilterT NF;
    TickerFilterT OF;
    TickerApiFilterLocal(&NotLocal);
    TickerApiFilterNot(&NotLocal, &NotLocal);

    TickerApiFilterAnd(of, &NotLocal, &OF);
    TickerApiFilterAnd(nf, &NotLocal, &NF);

    return(!TickerApiFilterIsEqual(&OF, &NF));
}


/*F*******************************************************************/
/*!
    \Function _TickerApiSillWantServerEvents

    \Description
        Checks if the user was getting server events before and still wants
        them after the filter change.

    \Input of - old filter state
    \Input nf - new filter state

    \Output non-zero if the above is true.

    \Version 1.0 05/05/2004 (sbevan) First Version
*/
/*******************************************************************F*/
static int32_t _TickerApiStillWantServerEvents(const TickerFilterT *of, const TickerFilterT *nf)
{
    TickerFilterT NotLocal;
    TickerFilterT NF;
    TickerFilterT OF;
    TickerApiFilterLocal(&NotLocal);
    TickerApiFilterNot(&NotLocal, &NotLocal);

    TickerApiFilterAnd(of, &NotLocal, &OF);
    TickerApiFilterAnd(nf, &NotLocal, &NF);
    return(!TickerApiFilterIsZero(&OF) && !TickerApiFilterIsZero(&NF));
}


/*F*******************************************************************/
/*!
    \Function _TickerApiAppend

    \Description
        Copy the event and insert it at the back of the queue.

    \Input *pTicker - ticker
    \Input *pEvent  - event to add

    \Notes
        Unless it is being filtered then the event is always added, it
        displaces the last item if the queue is full.

    \Version 04/04/2004 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiAppend(TickerApiT *pTicker, TickerRecordT *pEvent)
{
    TickerRecordT *pQueuedEvent;

    if (_TickerApiShouldIgnoreEvent(pTicker, pEvent))
        return;
    pQueuedEvent = _TickerApiGetNextFreeEvent(pTicker, TICKER_APPEND_PRIORITY);
    if (pQueuedEvent == 0)
    {
        _TickerApiPrintf((pTicker, "queue full, dropping %u\n", pEvent->iId));
        return;
    }
    if (pEvent->uRepeatTime != 0 && pEvent->uExpiryTime == 0)
        pEvent->uExpiryTime = pEvent->uRepeatTime*1000 + NetTick();
    if (pQueuedEvent != pEvent)
        memcpy(pQueuedEvent, pEvent, sizeof *pEvent);
    pTicker->uLocalEventsCount += 1;
}

/*F*******************************************************************/
/*!
    \Function _TickerApiDelete

    \Description
        Delete any existing event with the specified id

    \Input *pTicker - ticker
    \Input iId      - id of event to remove

    \Version 05/05/2004 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiDelete(TickerApiT *pTicker, int32_t iId)
{
    uint32_t i;
    uint32_t uEventsCount = pTicker->uPendingEventsCount;
    unsigned char *pNext = &pTicker->uPendingEvents;
    EventsIndexT *pIndex = &pTicker->pEventsIndex[pTicker->uPendingEvents];
    TickerFilterT Local;
    TickerFilterT Filter;

    TickerApiFilterLocal(&Local);

    for (i = 0; i != uEventsCount; i += 1)
    {
        TickerRecordT *pEvent = &pTicker->pEvents[pIndex->uIndex];
        TickerApiFilterAnd(&pEvent->Kind, &Local, &Filter);
        if (iId == pEvent->iId && (TickerApiFilterIsZero(&Filter) != 0))
        {
            *pNext = pIndex->uNext;
            _TickerApiReleaseEvent(pTicker, pIndex);
            pTicker->uLocalEventsCount -= 1;
            break;
        }
        else
        {
            pNext = &pIndex->uNext;
            pIndex = &pTicker->pEventsIndex[pIndex->uNext];
        }
    }
}


/*F*******************************************************************/
/*!
    \Function _TickerApiUnpackEvent

    \Description
        Takes a ticker event in which strData contains the encoded event
        information and unpacks it to populate the event object.

    \Input *pTicker     - the ticker
    \Input *pMarshaller - marshaller that contains the encoded message

    \Notes
    \verbatim
        Ticker events come from the server in a marshalled/serialized form.
        Each ticker event (currently) looks like :-

             0     1     2     3     4     5     6     7     8     9
          +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
          |     |     |     |     |     |     |     |     |     |     |
          +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
          |<--------------------->|  ^  |kind |<--- custom state ---->|
             sequence number         |
                                     |
                                     |
                                  priority


            10    11    12    13
          +-----+-----+-----+-----+
          |     |     |     |     |...strings
          +-----+-----+-----+-----+
          |   ms to   |  ^  |  ^  |
          |display for|  |     |  |
                       sound   |
                               |
                           #strings

        That is there is 32-bit sequence number (network byte order)
        followed by the priority, repeat count and number of strings
        all of which are just one byte each.  The strings then
        immediately follow.  Each string has the form :-

             0     1     2     3      4        len+5
          +-----+-----+-----+-----+-----+.....+-----+
          |     |     |     |     |     |     |     |
          +-----+-----+-----+-----+-----+.....+-----+
          | red |green|blue |alpha| len | <-- len-->|


       Note there is no length in the ticker event header since events
       are framed by the carrier and so the length is already known.

       Note also that the format is not finalised yet, the ticker event
       in particular is likely to gain some additional fields at some point.

       The message will be ignored if :-

          * there is any problem unpacking the event due to fields not
            containing expected values or the message being truncated.

          * there is not enough space to hold the new event.

       If a logger has been registered then either situation will generate
       some log information.

       The expiry value is not sent from the server.  Instead all events
       originating from the server have an expiry time of 0 which indicates
       that the event should expire as soon as it is queried.  This is 
       the client does not deal repeating/expiring server side events,
       that is taken care of by the server.
    \endverbatim

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiUnpackEvent(TickerApiT *pTicker, MarshallerT *pMarshaller)
{
    uint32_t uId, uPriority, uDisplay, uSound, nStrings, uStrlen;
    uint32_t uKind;
    int32_t iState;
    uint32_t uServerEventsCount;
    TickerRecordT *pEvent;

    uId = _TickerApiMarshallerGetInt(pMarshaller);
    uPriority = _TickerApiMarshallerGetUChar(pMarshaller);
    uKind = _TickerApiMarshallerGetUChar(pMarshaller);
    iState = _TickerApiMarshallerGetInt(pMarshaller);
    uDisplay = _TickerApiMarshallerGetUShort(pMarshaller);
    uSound = _TickerApiMarshallerGetUChar(pMarshaller);
    nStrings = _TickerApiMarshallerGetUChar(pMarshaller);
    if (pMarshaller->bError)
    {
        _TickerApiPrintf((pTicker, "marshalling error, ignoring message\n"));
        return;
    }
    _TickerApiPrintf((pTicker, "recv id %d typ %d st %d pri %u snd %u #%d\n",
                     uId, uKind, iState, uPriority, uSound, nStrings));
    pEvent = _TickerApiGetNextFreeEvent(pTicker, uPriority);
    if (pEvent == 0)
    {
        _TickerApiPrintf((pTicker, "queue full, dropping server event %u\n", uId));
        return;
    }
    uStrlen = pMarshaller->uSize-pMarshaller->uReadPoint;
    pEvent->iId = uId;
    pEvent->iPriority = uPriority;
    pEvent->iMsToDisplayFor = uDisplay;
    pEvent->uRepeatCount = 1;
    TickerApiFilterNone(&pEvent->Kind);
    TickerApiFilterSet(&pEvent->Kind, (TickerFilterItemE)uKind);
    pEvent->iState = iState;
    pEvent->uSound = uSound;
    pEvent->uStringCount = nStrings;
    pEvent->uStringsReadCount = 0;
    pEvent->uNextStringPos = 0;
    pEvent->uDataStart = 0;

    // Truncate the message if it is too big for the buffer
    if (uStrlen > sizeof(pEvent->strData))
        uStrlen = sizeof(pEvent->strData);

    pEvent->uDataLen = uStrlen;
    memcpy(pEvent->strData, &pMarshaller->pBuffer[pMarshaller->uReadPoint],
           uStrlen);
    pTicker->uLastServerId = uId;
    uServerEventsCount = pTicker->uPendingEventsCount - pTicker->uLocalEventsCount;
    //
    // If the queue is getting full and at least some of the events
    // are from the server then schedule sending a keep-alive to
    // inform the ticker server that the queue is filling up.
    // The keep-alive is deliberately scheduled with a small delay
    // to avoid spamming the server.
    //
    if ((pTicker->uPendingEventsCount > pTicker->uQueueHighWater)
        && (uServerEventsCount > 0)
        && (pTicker->uKeepAliveTimeout > 1000))
        pTicker->uKeepAliveTimeout = 1000;
    _TickerApiUpdateHello(pTicker);
}


/*F*******************************************************************/
/*!
    \Function _TickerApiUnpackHello

    \Description
        Unpacks the hello message.

    \Input *pTicker     - the ticker
    \Input *pMarshaller - a marshaller containing the encoded update

    \Notes
        The ticker hello reply looks like :-

             0     1     2
          +-----+-----+-----+-----+
          |     |     |     |     |
          +-----+-----+-----+-----+
          |  ^  |  ^  |  ^  |  ^  |
             |     |     |    new queue high water mark   
             |     |     |       
             |     |    new queue low water mark
             |     |
             |    new keep-alive period
             |
            new hello period in seconds
 
        The receipt of a hello response also determines whether any
        more hello or keep-alive packets are sent to the server.

        In order that the ticker server can have a reasonable idea of who
        is online/offline, keep-alive events are still sent in the case where
        the user doesn't actually want any events from the server.

    \Version 1.0 04/04/2004 (sbevan) First Version
*/
/*******************************************************************F*/
static void _TickerApiUnpackHello(TickerApiT *pTicker, MarshallerT *pMarshaller)
{
    unsigned char uHelloPeriod = _TickerApiMarshallerGetUChar(pMarshaller);
    unsigned char uKeepAlivePeriod = _TickerApiMarshallerGetUChar(pMarshaller);
    unsigned char uQueueLowWater = _TickerApiMarshallerGetUChar(pMarshaller);
    unsigned char uQueueHighWater = _TickerApiMarshallerGetUChar(pMarshaller);
    TickerFilterT Filter;

    if (pMarshaller->bError)
    {
        _TickerApiPrintf((pTicker, "marshalling error, ignoring message\n"));
    }
    else
    {
        pTicker->uHelloPeriod = uHelloPeriod;
        pTicker->uKeepAlivePeriod = uKeepAlivePeriod;
        if (uQueueLowWater <= pTicker->uEventsCount)
        {
            pTicker->uQueueLowWater = uQueueLowWater;
        }
        else
        {
            _TickerApiPrintf((pTicker, "low water overflow: %u > %u, ingoring\n",
                              uQueueLowWater, pTicker->uQueueLowWater));
        }
        if (uQueueHighWater <= pTicker->uEventsCount)
        {
            pTicker->uQueueHighWater = uQueueHighWater;
        }
        else
        {
            _TickerApiPrintf((pTicker, "highwater overflow: %u > %u, ingoring\n",
                              uQueueHighWater, pTicker->uQueueHighWater));
        }
    }
    TickerApiFilterLocal(&Filter);
    TickerApiFilterNot(&Filter, &Filter);
    TickerApiFilterAnd(&Filter, &pTicker->Identity.Filter, &Filter);
    if (TickerApiFilterIsZero(&Filter))
    {
        _TickerApiDisableHello(pTicker);
    }
    else
    {
        _TickerApiUpdateHello(pTicker);
    }
    _TickerApiEnableKeepAlive(pTicker);
}


/*F*******************************************************************/
/*!
    \Function _TickerApiAttachMessageHeader

    \Description
        Attach the standard client->server message header.

    \Input *pTicker     - the ticker
    \Input *pMarshaller - the marshaller to write the header into
    \Input msg          - message

    \Notes
        The standard header consists of :-

             0     1     2         len+3 len+4  +5    +6   len+7
          +-----+-----+-----+.....+-----+-----+-----+-----+-----+
          |     |     |     |     |     |     |     |     |     |
          +-----+-----+-----+.....+-----+-----+-----+-----+-----+
          |vers |msg# | len |<-- len -->|<----- user id ------->|
          | ion |     |<--- context --->|
            

           len+8   +9   +10   +11   +12   +13   +14   +15   +16
          +-----+.....+-----+-----+-----+-----+-----+-----+-----+
          |     |     |     |     |     |     |     |     |     |
          +-----+.....+-----+-----+-----+-----+-----+-----+-----+
          | len  <-- data ->|<-------- addr ------->|<-- port ->|
          |<--- persona --->|<---------local address & port --->|
            

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
static void _TickerApiAttachMessageHeader(TickerApiT *pTicker, MarshallerT *pMarshaller, char msg)
{
    _TickerApiMarshallerPutUChar(pMarshaller, TICKER_PROTOCOL_VERSION);
    _TickerApiMarshallerPutUChar(pMarshaller, msg);
    _TickerApiMarshallerPutString(pMarshaller, pTicker->strContext);
    _TickerApiMarshallerPutUInt(pMarshaller, pTicker->uUid);
    _TickerApiMarshallerPutString(pMarshaller, pTicker->Identity.strPersona);
    _TickerApiMarshallerPutUInt(pMarshaller, 
                                pTicker->LocalAddr.sin_addr.s_addr);
    _TickerApiMarshallerPutUShort(pMarshaller, pTicker->LocalAddr.sin_port);
}


/*F*******************************************************************/
/*!
    \Function _TickerApiSendHello

    \Description
        Send a "hello" message to the server to indicate that we want
        and want to receive ticker events.

    \Input *pTicker - the ticker

    \Notes
    \verbatim
        The "hello" message is packed into a UDP packet in the
        following format :-

             0     1         len+2 len+3  +4    +5     +6  len+7
          +-----+-----+-----+.....+-----+-----+-----+-----+-----+
          |     |     |     |     |     |     |     |     |     |
          +-----+-----+-----+.....+-----+-----+-----+-----+-----+
          |vers |hello| len |<-- len -->|<----- user id ------->|
          | ion |     |<--- context --->|
            

           len+8   +9   +10   +11   +12   +13   +14   +15   +16
          +-----+.....+-----+-----+-----+-----+-----+-----+-----+
          |     |     |     |     |     |     |     |     |     |
          +-----+.....+-----+-----+-----+-----+-----+-----+-----+
          | len  <-- data ->|<-------- addr ------->|<-- port ->|
          |<--- persona --->|<---------local address & port --->|

            +17
          +-----+.....+-----+-----+.....+-----+
          |     |     |     |     |     |     |
          +-----+.....+-----+-----+.....+-----+
          | len |<-- data ->| len |<-- data ->|
          |<--- product --->|<--- platform -->|

          +-----+.....+-----+-----+.....+-----+-----+.....+-----+
          |     |     |     |     |     |     |     |     |     |
          +-----+-----+-----+-----+.....+-----+-----+.....+-----+
          | len |<-- data ->| len |<-- data ->| len |<-- data ->|
          |<---- slus ----->|<---- lkey ----->|<---- mid ------>|

          +-----+.....+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
          |     |     |     |     |     |     |     |     |     |     |     |     |     |     |     |
          +-----+.....+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
          | len |<-- data ->|< country >|< language>|<------------------- filt -------------------->|
          |<---- team ----->|


          +-----+-----+-----+-----+-----+-----+-----+
          |     |     |     |     |     |     |     |
          +-----+-----+-----+-----+-----+-----+-----+
          |<- max event id seen ->|  ^  |  ^  |  ^
                                     |     |    # events from server
                                     |     |
                                     | max queue size
                                     |
                            current #events in queue

        This function is typically called from the TickerUpdate and
        only sends the hello if one is required which is determined
        by expiry time of the hello timer.  Since the carrier is UDP
        and hence lossy, once a hello is sent another one is scheduled
        to be sent in another X seconds.  This is disabled if/when
        a hello response is recieved from the server.
    \endverbatim

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiSendHello(TickerApiT *pTicker)
{
    uint32_t uServerEventsCount = 
        pTicker->uPendingEventsCount - pTicker->uLocalEventsCount;
    MarshallerT Marshaller;
    char strHello[256];
    uint16_t uCountry, uLanguage;

    if ((NetTick() < pTicker->uHelloTimeout)
        || (uServerEventsCount > pTicker->uQueueLowWater))
        return;

    // flip the language and country to make this fit into the ticker server
    // and make sure to send lowercase country
    uCountry = LOBBYAPI_LocalizerTokenGetCountry(pTicker->Identity.uLocality);
    uCountry = LOBBYAPI_LocalizerTokenShortToLower(uCountry);
    uLanguage = LOBBYAPI_LocalizerTokenGetLanguage(pTicker->Identity.uLocality);

    _TickerApiMarshallerInit(&Marshaller, sizeof strHello, strHello);
    _TickerApiAttachMessageHeader(pTicker, &Marshaller, TICKER_MSG_HELLO);
    _TickerApiMarshallerPutString(&Marshaller, pTicker->Identity.strProduct);
    _TickerApiMarshallerPutString(&Marshaller, pTicker->Identity.strPlatform);
    _TickerApiMarshallerPutString(&Marshaller, pTicker->Identity.strSlus);
    _TickerApiMarshallerPutString(&Marshaller, pTicker->Identity.strLKey);
    _TickerApiMarshallerPutString(&Marshaller, pTicker->Identity.strMid);
    _TickerApiMarshallerPutString(&Marshaller, pTicker->Identity.strFavouriteTeam);
    _TickerApiMarshallerPutUShort(&Marshaller, uCountry);
    _TickerApiMarshallerPutUShort(&Marshaller, uLanguage);
    _TickerApiMarshallerPutUInt(&Marshaller, pTicker->Identity.Filter.uTop);
    _TickerApiMarshallerPutUInt(&Marshaller, pTicker->Identity.Filter.uBot);
    _TickerApiMarshallerPutUInt(&Marshaller, pTicker->uLastServerId);
    _TickerApiMarshallerPutUChar(&Marshaller, pTicker->uPendingEventsCount);
    _TickerApiMarshallerPutUChar(&Marshaller, pTicker->uEventsCount);
    _TickerApiMarshallerPutUChar(&Marshaller, uServerEventsCount);
    ProtoUdpSend(pTicker->pUdp, strHello, Marshaller.uWritePoint);
    _TickerApiUpdateHello(pTicker);
}


/*F*******************************************************************/
/*!
    \Function _TickerApiSendKeepAlive

    \Description
        Send a "keep-alive" message to the server to indicate that we are
        alive and want to keep receiving ticker events.

    \Input *pTicker - the ticker

    \Notes
    \verbatim
        The "keep-alive" message is packed into a UDP packet in the
        following format :-

             0     1         len+2 len+3  +4    +5     +6  len+7
          +-----+-----+-----+.....+-----+-----+-----+-----+-----+
          |     |     |     |     |     |     |     |     |     |
          +-----+-----+-----+.....+-----+-----+-----+-----+-----+
          |vers |hello| len |<-- len -->|<----- user id ------->|
          | ion |     |<--- context --->|
            

           len+8   +9   +10   +11   +12   +13   +14   +15   +16
          +-----+.....+-----+-----+-----+-----+-----+-----+-----+
          |     |     |     |     |     |     |     |     |     |
          +-----+.....+-----+-----+-----+-----+-----+-----+-----+
          | len  <-- data ->|<-------- addr ------->|<-- port ->|
          |<--- persona --->|<---------local address & port --->|
            

            +14   +15   +16   +17   +18   +19   +20
          +-----+-----+-----+-----+-----+-----+-----+
          |     |     |     |     |     |     |     |
          +-----+-----+-----+-----+-----+-----+-----+
          |<- max event id seen ->|  ^  |  ^  |  ^
                                     |     |    # of events that came from server 
                                     |     |
                                     | max queue size
                                     |
                            current #events in queue
    \endverbatim

    \Version 23/03/2003 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiSendKeepAlive(TickerApiT *pTicker)
{
    uint32_t uNow = NetTick();
    if (uNow >= pTicker->uKeepAliveTimeout)
    {
        uint32_t uServerEventsCount = 
            pTicker->uPendingEventsCount - pTicker->uLocalEventsCount;
        MarshallerT Marshaller;
        char strMsg[128];
        _TickerApiMarshallerInit(&Marshaller, sizeof strMsg, strMsg);
        _TickerApiAttachMessageHeader(pTicker, &Marshaller, TICKER_MSG_KEEP_ALIVE);
        _TickerApiMarshallerPutUInt(&Marshaller, pTicker->uLastServerId);
        _TickerApiMarshallerPutUChar(&Marshaller, pTicker->uPendingEventsCount);
        _TickerApiMarshallerPutUChar(&Marshaller, pTicker->uEventsCount);
        _TickerApiMarshallerPutUChar(&Marshaller, uServerEventsCount);
        ProtoUdpSend(pTicker->pUdp, strMsg, Marshaller.uWritePoint);
        _TickerApiEnableKeepAlive(pTicker);
    }
}


/*F*******************************************************************/
/*!
    \Function _TickerApiRecv

    \Description
        If the the ticker is connected then UDP messages are read off
        the socket until the queue is full or there are no UDP messages
        left to read.  The size of the queue puts a bound on the number
        of messages that can be read in one go.

    \Input *pTicker - the ticker

    \Notes
        May want to consider putting an upper bound on the amount of work
        that can be done in one iteration.  Right now the upper bound is
        determined by how many free slots there are in the client side
        queue.  That's fairly logical for ticker events but it not quite
        right for filter events since in a pathalogical case it is possible
        for a stream of filter events to come in.  However, we'll ignore
        that possibility for now.
        
        The fact that the UDP payload is dumped right into the 
        TickerRecordT strData means that the maximum size of any
        string message is TICKER_MESSAGE_MAX - the message header.

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
static void _TickerApiRecv(TickerApiT *pTicker)
{
    ProtoUdpT *pUdp = pTicker->pUdp;
    int32_t iLen = 1;
    MarshallerT Marshaller;
    uint32_t uCmd;
    // Don't want to do an unbounded amount of work, so don't
    // try and extract a small finite number of packets in one go.
    uint32_t uMaxPackets = TICKER_UDP_QUEUE_SIZE;
    TickerRecordT Record;

    while ((iLen > 0) && (--uMaxPackets != 0))
    {
        ProtoUdpUpdate(pUdp);
        iLen = ProtoUdpRecv(pUdp, (char *)Record.strData, sizeof(Record.strData));
        if (iLen > 0)
        {
            _TickerApiMarshallerInit(&Marshaller, iLen, (char *)Record.strData);
            uCmd =_TickerApiMarshallerGetUChar(&Marshaller);
            switch (uCmd)
            {
            case TICKER_MSG_EVENT:
                _TickerApiUnpackEvent(pTicker, &Marshaller);
                break;
            case TICKER_MSG_HELLO:
                _TickerApiUnpackHello(pTicker, &Marshaller);
                break;
            default:
                _TickerApiPrintf((pTicker, 
                                  "ignoring unknown command %u\n", uCmd));
                break;
            }
        }
    }
}


/*F*******************************************************************/
/*!
    \Function _TickerApiParseServerString

    \Description
        The ticker server encodes a bunch of (connection) information
        in the EX-ticker string that is sent back when persona validation
        is completed.  This parses that string and extracts the information.

    \Input *pTicker - the ticker
    \Input *pServer - string encoding server connection information

    \Output TICKER_ERR_xxx

    \Notes
    \verbatim
       The information is encoded as :-

         <context>:<hello-period>:<bgRed>:<bgGreen>:<bgBlue>:<bgAlpha>:<user-id>:<ip-addr>:<port>

       where <context> is a string <hello-period> is an integer,
             <user-id> is an unsigned integer, <ip-addr> is a dotted-quad,
             <port> is an integer.
    \endverbatim

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
static int32_t _TickerApiParseServerString(TickerApiT *pTicker, const char *pServer)
{
    uint32_t uKeepAlivePeriod;
    uint32_t uColor;
    char strBuf[32];

    TagFieldGetDelim(pServer, strBuf, sizeof(strBuf), "", 0, ',');
    if (strBuf[0] == '\0')
        return(TICKER_ERR_INVP);
    pTicker->uUid = TagFieldGetNumber(strBuf, 0);

    TagFieldGetDelim(pServer, strBuf, sizeof(strBuf), "", 1, ',');
    if (strBuf[0] == '\0')
        return(TICKER_ERR_INVP);
    if (SockaddrInParse((struct sockaddr *)&pTicker->ServerAddr, strBuf) != 3)
        return(TICKER_ERR_INVP);

    TagFieldGetDelim(pServer, pTicker->strContext, sizeof(pTicker->strContext), "", 2, ',');
    if (pTicker->strContext[0] == '\0')
        return(TICKER_ERR_INVP);

    TagFieldGetDelim(pServer, strBuf, sizeof(strBuf), "", 3, ',');
    if (strBuf[0] == '\0')
        return(TICKER_ERR_INVP);
    uKeepAlivePeriod = TagFieldGetNumber(strBuf, 0);
    pTicker->uKeepAlivePeriod = (uKeepAlivePeriod > UCHAR_MAX) ? 30 : uKeepAlivePeriod;

    TagFieldGetDelim(pServer, strBuf, sizeof(strBuf), "", 4, ',');
    if (strBuf[0] == '\0')
        return(TICKER_ERR_INVP);
    uColor = TagFieldGetNumber(strBuf, 0);
    pTicker->BackgroundColor.uRed = (uColor > UCHAR_MAX) ? 0 : uColor;

    TagFieldGetDelim(pServer, strBuf, sizeof(strBuf), "", 5, ',');
    if (strBuf[0] == '\0')
        return(TICKER_ERR_INVP);
    uColor = TagFieldGetNumber(strBuf, 0);
    pTicker->BackgroundColor.uGreen = (uColor > UCHAR_MAX) ? 0 : uColor;

    TagFieldGetDelim(pServer, strBuf, sizeof(strBuf), "", 6, ',');
    if (strBuf[0] == '\0')
        return(TICKER_ERR_INVP);
    uColor = TagFieldGetNumber(strBuf, 0);
    pTicker->BackgroundColor.uBlue = (uColor > UCHAR_MAX) ? 0 : uColor;

    TagFieldGetDelim(pServer, strBuf, sizeof(strBuf), "", 7, ',');
    if (strBuf[0] == '\0')
        return(TICKER_ERR_INVP);
    uColor = TagFieldGetNumber(strBuf, 0);
    pTicker->BackgroundColor.uAlpha = (uColor > UCHAR_MAX) ? 0 : uColor;

    TagFieldGetDelim(pServer, strBuf, sizeof(strBuf), "", 8, ',');
    if (strBuf[0] == '\0')
        return(TICKER_ERR_INVP);
    pTicker->ServerFilter.uTop = TagFieldGetNumber(strBuf, 0xffffffff);
    TagFieldGetDelim(pServer, strBuf, sizeof(strBuf), "", 9, ',');
    if (strBuf[0] == '\0')
        return(TICKER_ERR_INVP);
    pTicker->ServerFilter.uBot = TagFieldGetNumber(strBuf, 0xffffffff);

    return(TICKER_ERR_NONE);
}


/*** Public functions ************************************************/

/*F*******************************************************************/
/*!
    \Function TickerApiCreate

    \Description
        Create a new Ticker client.

    \Input iRecordCount - maximum size of the ticker event queue.

    \Output pointer to new Ticker or 0 on error.

    \Notes
        Since the number of ticker events cannot be changed once the
        TickerApiT is created the following takes the opportunity to
        allocate the TickerApiT and the sequence of TickerRecordT in
        one allocation.  This simplifies the error handing and uses
        marginally less memory.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
TickerApiT *TickerApiCreate(int32_t iRecordCount)
{
    TickerApiT *pTicker;
    uint32_t uTotalSpace;
    char * pMem;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    if (iRecordCount < TICKER_RECS_MIN || iRecordCount > TICKER_RECS_MAX)
        iRecordCount = TICKER_RECS_DEFAULT;

    uTotalSpace = sizeof *pTicker + iRecordCount*(sizeof *pTicker->pEvents + sizeof *pTicker->pEventsIndex);
    pMem = (char *)DirtyMemAlloc(uTotalSpace, TICKERAPI_MEMID, iMemGroup, pMemGroupUserData);
    if (pMem == NULL)
        return(NULL);
    pTicker = (TickerApiT*)pMem;
    pTicker->iMemGroup = iMemGroup;
    pTicker->pMemGroupUserData = pMemGroupUserData;
    pTicker->pUdp = 0;
    pTicker->pDebugData = 0;
    pTicker->pDebugProc = _TickerApiDefaultLogger;
    pTicker->pEvents = (TickerRecordT*)&pMem[sizeof *pTicker];
    pTicker->pEventsIndex = (EventsIndexT*)&pMem[sizeof *pTicker+iRecordCount*sizeof *pTicker->pEvents];
    _TickerApiDisableKeepAlive(pTicker);
    _TickerApiDisableHello(pTicker);
    pTicker->uCurrentEventTimeout = 0;
    pTicker->iCurrentEventPriority = 0;
    TickerApiFilterAll(&pTicker->Identity.Filter);
    pTicker->uUid = 0;
    pTicker->uLastServerId = 0;
    memset(&pTicker->ServerAddr, 0, sizeof pTicker->ServerAddr);
    memset(&pTicker->LocalAddr, 0, sizeof pTicker->LocalAddr);
    pTicker->uRequestId = 0;
    pTicker->uLocality = LOBBYAPI_LOCALITY_EN_US;
    pTicker->uEventsCount = iRecordCount;
    pTicker->uLocalEventsCount = 0;
    pTicker->uQueueLowWater = iRecordCount/4;
    pTicker->uQueueHighWater = (3*iRecordCount)/4;
    pTicker->uHelloPeriod = TICKER_DEFAULT_HELLO_PERIOD;
    memset(pTicker->strLastSportsHeader, 0, sizeof pTicker->strLastSportsHeader);
    TickerApiFlush(pTicker);
    return(pTicker);
}


/*F*******************************************************************/
/*!
    \Function TickerApiDebug

    \Description
        Install a debug function that will log various conditions that
        are internal to the ticker but which may aid in debugging any
        code that uses the TickerApi.

    \Input *pTicker    - ticker
    \Input *pDebugData - data that will be passed to the debug function
    \Input *pDebugProc - function to call with any debug output

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
void TickerApiDebug(TickerApiT *pTicker, void *pDebugData, void (*pDebugProc)(void *pDebugData, const char *pFormat, ...))
{
    pTicker->pDebugData = pDebugData;
    pTicker->pDebugProc = pDebugProc;
}


/*F*******************************************************************/
/*!
    \Function TickerApiDestroy

    \Description
        Destroy the Ticker

    \Input *pTicker    - ticker

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
void TickerApiDestroy(TickerApiT *pTicker)
{
    TickerApiDisconnect(pTicker);
    if (pTicker->pUdp != 0)
        ProtoUdpDestroy(pTicker->pUdp);
    DirtyMemFree(pTicker, TICKERAPI_MEMID, pTicker->iMemGroup, pTicker->pMemGroupUserData);
}


/*F*******************************************************************/
/*!
    \Function TickerApiFlush

    \Description
        Flush any ticker events that may be in the TickerApi.

    \Input *pTicker - ticker

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
void TickerApiFlush(TickerApiT *pTicker)
{
    int32_t i;
    for (i = 0; i != pTicker->uEventsCount; i += 1)
    {
        pTicker->pEventsIndex[i].uIndex = i;
        pTicker->pEventsIndex[i].uNext = i+1;
    }
    pTicker->uPendingEvents = 0;
    pTicker->uFreeEvents = 0;
    pTicker->uPendingEventsCount = 0;
    pTicker->uLocalEventsCount = 0;
}

// Called when the EX-ticker string from persona login is available

/*F********************************************************************/
/*!
    \Function TickerApiOnline

    \Description
         This function should be called when the ticker server
         information string is available.  This string typically
         comes from the EX-ticker attribute in a lobby server
         persona authenticaon response.

    \Input pTicker - ticker reference
    \Input pServer - The value associated with EX-ticker in the lobby server.

    \Output TICKER_ERR_NONE if the info was parsed successfully.
            TICKER_ERR_INVP otherwise.

    \Version 01/18/2006 (doneill)
*/
/********************************************************************F*/
int32_t TickerApiOnline(TickerApiT *pTicker, const char *pServer)
{
    return(_TickerApiParseServerString(pTicker, pServer));
}

/*F*******************************************************************/
/*!
    \Function TickerApiConnect

    \Description
        Connect to the ticker server.

    \Input *pTicker   - ticker
    \Input *pIdentity - user info., or NULL if no info. available.

    \Output TICKER_ERR_NONE if the connection was successful,
            TICKER_ERR_CONN if it was not possible to connect.

    \Version 1.0 12/11/2003 (sbevan) First Version
    \Version 2.0 23/03/2004 (sbevan) Added pIdentity argument.
*/
/*******************************************************************F*/
int32_t TickerApiConnect(TickerApiT *pTicker, const TickerIdentityT *pIdentity)
{
    ProtoUdpT * pUdp;
    TickerApiDisconnect(pTicker);

    if (pTicker->ServerAddr.sin_addr.s_addr == 0)
    {
        return(TICKER_ERR_CONN);
    }
    if (pTicker->pUdp == 0)
    {
        DirtyMemGroupEnter(pTicker->iMemGroup, pTicker->pMemGroupUserData);
        pTicker->pUdp = ProtoUdpCreate(TICKER_MESSAGE_MAX, TICKER_UDP_QUEUE_SIZE);
        DirtyMemGroupLeave();
    }
    else
    {
        ProtoUdpDisconnect(pTicker->pUdp);
    }
    pUdp = pTicker->pUdp;
    if (pUdp == 0)
        return(TICKER_ERR_CONN);

    if (ProtoUdpConnect(pUdp, (struct sockaddr *)&pTicker->ServerAddr) != 0)
    {
        ProtoUdpDestroy(pUdp);
        pTicker->pUdp = 0;
        return(TICKER_ERR_CONN);
    }
    ProtoUdpGetLocalAddr(pUdp, &pTicker->LocalAddr);
    _TickerApiEnableHello(pTicker);
    _TickerApiEnableKeepAlive(pTicker);
    if (pIdentity != 0)
    {
        memcpy(&pTicker->Identity, pIdentity, sizeof *pIdentity);
        // make sure all strings are NUL terminated ...
        pTicker->Identity.strPersona[sizeof pTicker->Identity.strPersona -1] = '\0';
        pTicker->Identity.strProduct[sizeof pTicker->Identity.strProduct -1] = '\0';
        pTicker->Identity.strPlatform[sizeof pTicker->Identity.strPlatform -1] = '\0';
        pTicker->Identity.strSlus[sizeof pTicker->Identity.strSlus -1] = '\0';
        pTicker->Identity.strLKey[sizeof pTicker->Identity.strLKey -1] = '\0';
        pTicker->Identity.strMid[sizeof pTicker->Identity.strMid -1] = '\0';
        pTicker->Identity.strFavouriteTeam[sizeof pTicker->Identity.strFavouriteTeam -1] = '\0';

        // force admin messages on in the filter
        TickerApiFilterSet(&pTicker->Identity.Filter, TICKER_FILT_MESSAGE);

    }
    else
    {
        memset(pTicker->Identity.strPersona, 0, sizeof pTicker->Identity.strPersona);
        memset(pTicker->Identity.strProduct, 0, sizeof pTicker->Identity.strProduct);
        memset(pTicker->Identity.strPlatform, 0, sizeof pTicker->Identity.strPlatform);
        memset(pTicker->Identity.strSlus, 0, sizeof pTicker->Identity.strSlus);
        memset(pTicker->Identity.strLKey, 0, sizeof pTicker->Identity.strLKey);
        memset(pTicker->Identity.strMid, 0, sizeof pTicker->Identity.strMid);
        memset(pTicker->Identity.strFavouriteTeam, 0, sizeof pTicker->Identity.strFavouriteTeam);
        pTicker->Identity.uLocality = LOBBYAPI_LOCALITY_BLANK;
        TickerApiFilterAll(&pTicker->Identity.Filter);
    }
    memset(pTicker->strLastSportsHeader, 0, sizeof pTicker->strLastSportsHeader);
    _TickerApiSendHello(pTicker);
    return(TICKER_ERR_NONE);
}


/*F*******************************************************************/
/*!
    \Function TickerApiDisconnect

    \Description
        Disconnect from the ticker server.

    \Input *pTicker - ticker

    \Version 12/11/2003 (sbevan)
*/
/*******************************************************************F*/
void TickerApiDisconnect(TickerApiT *pTicker)
{
    if (!_TickerApiIsConnected(pTicker))
        return;
    ProtoUdpDisconnect(pTicker->pUdp);
    _TickerApiDisableHello(pTicker);
    _TickerApiDisableKeepAlive(pTicker);
}


/*F*******************************************************************/
/*!
    \Function TickerApiIdentity

    \Description
        Update the user's identity information.

    \Input *pTicker   - ticker
    \Input *pIdentity - user info.

    \Output TICKER_ERR_NONE if the update was initiated.

    \Notes
        This is primarily intended to allow the filter list to be updated.

        A successful return does not mean that the update has been done, only
        that it has been initiated.  The TickerApi will take care of re-trying
        the update until it succeeds.

        This used to return TICKER_ERR_CONN if the ticker was not
        connected to the server which is why it has a return value.

    \Version 1.0 04/04/2004 (sbevan) First Version
*/
/*******************************************************************F*/
int32_t TickerApiIdentity(TickerApiT *pTicker, const TickerIdentityT *pIdentity)
{
    if (_TickerApiIsServerVisibleFilterChange(&pTicker->Identity.Filter, &pIdentity->Filter))
    {
        if (_TickerApiIsConnected(pTicker))
            _TickerApiEnableHello(pTicker);
        if (!_TickerApiStillWantServerEvents(&pTicker->Identity.Filter, &pIdentity->Filter))
            pTicker->strLastSportsHeader[0] = '\0';
    }
    memcpy(&pTicker->Identity, pIdentity, sizeof *pIdentity);
    // make sure all strings are NUL terminated ...
    pTicker->Identity.strPersona[sizeof pTicker->Identity.strPersona -1] = '\0';
    pTicker->Identity.strProduct[sizeof pTicker->Identity.strProduct -1] = '\0';
    pTicker->Identity.strPlatform[sizeof pTicker->Identity.strPlatform -1] = '\0';
    pTicker->Identity.strSlus[sizeof pTicker->Identity.strSlus -1] = '\0';
    pTicker->Identity.strLKey[sizeof pTicker->Identity.strLKey -1] = '\0';
    pTicker->Identity.strMid[sizeof pTicker->Identity.strMid -1] = '\0';
    pTicker->Identity.strFavouriteTeam[sizeof pTicker->Identity.strFavouriteTeam -1] = '\0';

    // force admin messages on in the filter
    TickerApiFilterSet(&pTicker->Identity.Filter, TICKER_FILT_MESSAGE);

    if (_TickerApiIsConnected(pTicker))
        _TickerApiSendHello(pTicker);
    return(TICKER_ERR_NONE);
}


/*F*******************************************************************/
/*!
    \Function TickerApiIsOnline

    \Description
        Determines if the ticker is online and so receiving events
        from the ticker server.

    \Input *pTicker - ticker

    \Output non-zero value if the ticker is online, zero otherwise.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
int32_t TickerApiIsOnline(TickerApiT *pTicker)
{
    return(_TickerApiIsConnected(pTicker));
}


/*F*******************************************************************/
/*!
    \Function TickerApiInsert

    \Description
        Copy the event and insert it into the correct position in the
        client side queue of ticker events.

    \Input *pTicker - ticker
    \Input *pEvent  - event to add

    \Output TICKER_ERR_NONE if the event added or was dropped because 
            it didn't match the current filter.  TICKER_ERR_OVERFLOW if the  
            new event was dropped due to the queue being full.

    \Notes
        The event must have all its fields filled in correctly.
        The event is not sent to the server, it is purely local event.
        If the event doesn't match the current filters (as defined by
        TickerApiConnect or TickerApiIdentity) then it is silently
        dropped.  If the queue is full then the last event in the queue
        is checked and if it has a lower priority than new event then
        the last event is recycled to provide space for the new event.
        If pEvent.iId has the same value as any other event in the queue
        that was added by TickerApiInsert then that other event is deleted
        before attempting to add the new event.

    \Version 1.0 12/11/2003 (sbevan) First Version
    \Version 1.1 04/04/2004 (sbevan) Changed full queue handling
    \Version 1.2 05/05/2004 (sbevan) Now deletes any old event with same id
*/
/*******************************************************************F*/
int32_t TickerApiInsert(TickerApiT *pTicker, TickerRecordT *pEvent)
{
    TickerRecordT *pQueuedEvent;

    if (_TickerApiShouldIgnoreEvent(pTicker, pEvent))
        return(TICKER_ERR_NONE);
    if (pEvent->iId != 0)
        _TickerApiDelete(pTicker, pEvent->iId);
    pQueuedEvent = _TickerApiGetNextFreeEvent(pTicker, pEvent->iPriority);
    if (pQueuedEvent == 0)
    {
        _TickerApiPrintf((pTicker, "queue full, dropping local event %d\n",
                          pEvent->iId));
        return(TICKER_ERR_OVERFLOW);
    }
    if (pEvent->uRepeatTime != 0 && pEvent->uExpiryTime == 0)
        pEvent->uExpiryTime = pEvent->uRepeatTime*1000 + NetTick();
    memcpy(pQueuedEvent, pEvent, sizeof *pEvent);
    pTicker->uLocalEventsCount += 1;
    return(TICKER_ERR_NONE);
}


/*F*******************************************************************/
/*!
    \Function TickerApiQuery

    \Description
        Retrieve the next item in the queue of events.

    \Input *pTicker - ticker
    \Input *pEvent  - where to copy the next item to

    \Output TICKER_ERR_NONE if there is no event or the time for previous
            event has not expired yet.   A positive value if
            pEvent has been filled with a new event.

    \Notes
        You are advised not to add any intra-string spaces to format the
        the strings contained in the record.  All necessary formatting is
        supposed to be done by the server.  The only spaces that should need
        to be added are intra-message spaces to keep one ticker message
        visually separated from another message.

    \Version 1.0 12/11/2003 (sbevan) First Version
    \Version 1.1 04/04/2004 (sbevan) Strips off repeated sports headers
*/
/*******************************************************************F*/
int32_t TickerApiQuery(TickerApiT *pTicker, TickerRecordT *pEvent)
{
    EventsIndexT *pIndex;
    uint32_t uTimeNow = 0;  /* keep the PS2 compiler quiet */
    TickerRecordT *pFirstEvent; 
    int32_t bHaveEvent = 0;
    TickerStringT Header;
    TickerFilterT Filter;

    TickerApiUpdate(pTicker);
    do 
    {
        if (pTicker->uPendingEventsCount == 0)
            return(TICKER_ERR_NONE);
        pIndex = &pTicker->pEventsIndex[pTicker->uPendingEvents];
        pFirstEvent = &pTicker->pEvents[pIndex->uIndex];
        TickerApiFilterAnd(&pFirstEvent->Kind, &pTicker->Identity.Filter, &Filter);
        if (!TickerApiFilterIsZero(&Filter))
        {
            uTimeNow = NetTick();
            if ((pTicker->uCurrentEventTimeout != 0)
                && (uTimeNow < pTicker->uCurrentEventTimeout)
                && (pTicker->iCurrentEventPriority <= pFirstEvent->iPriority))
                return(TICKER_ERR_NONE);
            memcpy(pEvent, pFirstEvent, sizeof *pEvent);
            if (pEvent->iMsToDisplayFor > 0)
                pTicker->uCurrentEventTimeout = uTimeNow + pEvent->iMsToDisplayFor;
            else
                pTicker->uCurrentEventTimeout = 0;
            bHaveEvent = 1;
            TickerApiFilterGameAll(&Filter);
            TickerApiFilterAnd(&Filter, &pFirstEvent->Kind, &Filter);
            if (!TickerApiFilterIsZero(&Filter))
            {
                if ((TickerApiGetString(pTicker, pEvent, &Header) == TICKER_ERR_NONE)
                    && (strcmp((char *)Header.strString, pTicker->strLastSportsHeader) != 0))
                {
                    // put the header back ...
                    pEvent->uNextStringPos = pFirstEvent->uNextStringPos;
                    pEvent->uStringsReadCount = pFirstEvent->uStringsReadCount;
                    strnzcpy(pTicker->strLastSportsHeader, (char *)Header.strString, sizeof(pTicker->strLastSportsHeader));
                }
            }
            else
            {
                pTicker->strLastSportsHeader[0] = '\0';
            }
        }
        pTicker->iCurrentEventPriority = pFirstEvent->iPriority;
        pTicker->uPendingEvents = pIndex->uNext;
        _TickerApiReleaseEvent(pTicker, pIndex);
    } while (!bHaveEvent);
    //
    // We refer to pFirstEvent rather than pEvent here because pEvent
    // may have had its header removed.  Refering to pFirstEvent after
    // _TickerApiReleaseEvent has been called is akin to dereferencing
    // memory after free() but in this case we know that _TickerApiReleaseEvent
    // has not actually freed up the memory or altered in any way pFirstEvent.
    //
    TickerApiFilterLocal(&Filter);
    TickerApiFilterAnd(&Filter, &pFirstEvent->Kind, &Filter);
    if (!TickerApiFilterIsZero(&Filter))
    {
        pTicker->uLocalEventsCount -= 1;
        if (((pFirstEvent->uRepeatCount != 0) && (--pFirstEvent->uRepeatCount > 0))
            || ((pFirstEvent->uExpiryTime != 0) && (uTimeNow < pFirstEvent->uExpiryTime)))
            _TickerApiAppend(pTicker, pFirstEvent);
    }
    return(1);
}


/*F*******************************************************************/
/*!
    \Function TickerApiGetString

    \Description
        Extract a string from a ticker event.  Successive calls 
        extract the next string and so on until all the string have
        been extracted.

    \Input *pTicker - ticker
    \Input *pEvent  - ticker event to extract string from
    \Input *pString - where to copy the extracted string to

    \Output A positive value if a string is extracted, TICKER_ERR_NONE
            if there are no more strings, and TICKER_ERR_OVERFLOW if
            one of the strings is too large or malformed.

    \Notes
        When called more than once on the same pEvent it steps
        sequentially through all the strings contained in pEvent.  When
        the there are no more strings left then a positive value is
        returned and the iteration is reset so that if it is called
        again on the same pEvent it will start from the beginning again.
        For example, to display all the strings and their associated
        colors in a TickerRecordT  :-

     \verbatim
          TickerRecordT Record;
          TickerStringT String;
          if (TickerApiQuery(pTicker, &Record) <= 0)
              return;
          while (TickerApiGetString(pTicker, &Record, &String) > 0)
          {
              printf("red %d green %d blue %d alpha %d %s\n", 
                     String.uRed, String.uGreen, String.uBlue, 
                     String.uAlpha, String.strString);
          }
     \endverbatim   
    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
int32_t TickerApiGetString(TickerApiT *pTicker, TickerRecordT *pEvent, TickerStringT *pString)
{
    MarshallerT Marshaller;
    if (pEvent->uStringsReadCount == pEvent->uStringCount)
    {
        pEvent->uStringsReadCount = 0;
        pEvent->uNextStringPos = pEvent->uDataStart;
        return(1);
    }
    _TickerApiMarshallerInit(&Marshaller,
                             pEvent->uDataLen - pEvent->uNextStringPos, 
                             (char *)&pEvent->strData[pEvent->uNextStringPos]);
    pString->Color.uRed = _TickerApiMarshallerGetUChar(&Marshaller);
    pString->Color.uGreen = _TickerApiMarshallerGetUChar(&Marshaller);
    pString->Color.uBlue = _TickerApiMarshallerGetUChar(&Marshaller);
    pString->Color.uAlpha = _TickerApiMarshallerGetUChar(&Marshaller);
    _TickerApiMarshallerGetString(&Marshaller, TICKER_MESSAGE_MAX,
                                  (char *)pString->strString);
    if (Marshaller.bError)
    {
        _TickerApiPrintf((pTicker, "error unpacking string\n"));
        return(TICKER_ERR_OVERFLOW);
    }
    pEvent->uNextStringPos += Marshaller.uReadPoint;
    pEvent->uStringsReadCount += 1;
    return(TICKER_ERR_NONE);
}



/*F*******************************************************************/
/*!
    \Function TickerApiSetString

    \Description
        Add a string to the ticker event.  

    \Input *pTicker - ticker
    \Input *pEvent  - ticker event to add the string to
    \Input *pString - the string to be copied and added

    \Output TICKER_ERR_NONE if the string is added, TICKER_ERR_OVERFLOW
            if the string cannot be added.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
int32_t TickerApiAddString(TickerApiT *pTicker, TickerRecordT *pEvent, TickerStringT *pString)
{
    MarshallerT Marshaller;
    _TickerApiMarshallerInit(&Marshaller, TICKER_MESSAGE_MAX-pEvent->uDataLen,
                             (char *)&pEvent->strData[pEvent->uDataLen]);
    _TickerApiMarshallerPutUChar(&Marshaller, pString->Color.uRed);
    _TickerApiMarshallerPutUChar(&Marshaller, pString->Color.uGreen);
    _TickerApiMarshallerPutUChar(&Marshaller, pString->Color.uBlue);
    _TickerApiMarshallerPutUChar(&Marshaller, pString->Color.uAlpha);
    _TickerApiMarshallerPutString(&Marshaller, (char *)pString->strString);
    if (Marshaller.bError == 0)
    {
        pEvent->uStringCount += 1;
        pEvent->uDataLen += Marshaller.uWritePoint;
        return(TICKER_ERR_NONE);
    }
    else
    {
        return(TICKER_ERR_OVERFLOW);
    }
}


/*F*******************************************************************/
/*!
    \Function TickerApiUpdate

    \Description
        Do various background tasks.

    \Input *pTicker - the ticker

    \Output None

    \Notes
        There is no need to call this if all you are doing is calling
        TickerApiQuery to pull events since TickerApiQuery takes care
        of calling this if necessary.  However, if you have registered
        any kind of callback (e.g. TickerApiGetFilters, 
        TickerApiUpdateFilter, ... etc.) then you must call TickerApiUpdate
        regularly in order to run the code that will call the callbacks.
        Anywhere in the 10-30Hz range should be sufficient, though you
        can run it outside that range if necessary.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************F*/
void TickerApiUpdate(TickerApiT *pTicker)
{
    if (!_TickerApiIsConnected(pTicker))
        return;
    _TickerApiRecv(pTicker);
    _TickerApiSendHello(pTicker);
    _TickerApiSendKeepAlive(pTicker);
}

/*F********************************************************************************/
/*!
     \Function TickerApiStatus
 
     \Description
          Query the ticker server module for status information.
  
     \Input pTicker - point to module state
     \Input iSelect - status selector
     \Input pBuf - [out] storage for selector-specific output
     \Input iBufSize - size of output buffer
 
     \Output int32_t - selector specific; -1 on error
        
    \Notes
        iSelect can be one of the following:
    
        \verbatim
            'bkgr' - return recommended background color (TickerColorT) in the output buffer.
            'filt' - return filter indicating which events the server is willing to publish
                     to this client (TickerFilterT) in the output buffer.
        \endverbatim
 
     \Version 01/18/2006 (doneill)
*/
/********************************************************************************F*/
int32_t TickerApiStatus(TickerApiT *pTicker, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    int32_t iResult = -1;

    // Make sure that TickerApiOnline or TickerApiConnect has been called first
    if (pTicker->ServerAddr.sin_addr.s_addr != 0)
    {
        switch (iSelect)
        {
            case 'bkgr':
                // Return the background color recommended for display
                if ((pBuf == NULL) || (iBufSize < (int32_t)sizeof(pTicker->BackgroundColor)))
                    return(-1);
                memcpy(pBuf, &pTicker->BackgroundColor, sizeof(pTicker->BackgroundColor));
                iResult = 0;
                break;

            case 'filt':
                // Return the filter indicating which events the server is willing to serve up.
                if ((pBuf == NULL) || (iBufSize < (int32_t)sizeof(pTicker->ServerFilter)))
                    return(-1);
                memcpy(pBuf, &pTicker->ServerFilter, sizeof(pTicker->ServerFilter));
                iResult = 0;
                break;
        }
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterNone
 
     \Description
        Build a filter disabling all events
  
     \Input pFilter - Filter to populate.
 
     \Output TickerFilterT * - Ptr to pFilter.
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
TickerFilterT *TickerApiFilterNone(TickerFilterT *pFilter)
{
    pFilter->uTop = 0;
    pFilter->uBot = 0;
    return(pFilter);
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterAll
 
     \Description
        Build a filter enabling all events
  
     \Input pFilter - Filter to populate.
 
     \Output TickerFilterT * - Ptr to pFilter.
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
TickerFilterT *TickerApiFilterAll(TickerFilterT *pFilter)
{
    pFilter->uTop = 0xffffffff;
    pFilter->uBot = 0xffffffff;
    return(pFilter);
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterGameAutoAll
 
     \Description
        Build a filter enabling all auto racing events
  
     \Input pFilter - Filter to populate.
 
     \Output TickerFilterT * - Ptr to pFilter.
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
TickerFilterT *TickerApiFilterGameAutoAll(TickerFilterT *pFilter)
{
    int32_t iIdx;
    TickerFilterItemE aFilter[] = { TICKER_FILT_GAME_AUTO_ALL, TICKER_FILT_INVALID };
    TickerApiFilterNone(pFilter);
    for(iIdx = 0; iIdx < 64; iIdx++)
    {
        if ((int32_t)aFilter[iIdx] == -1)
            break;
        TickerApiFilterSet(pFilter, aFilter[iIdx]);
    }
    return(pFilter);
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterGameFifaAll
 
     \Description
        Build a filter enabling all FIFA leagues
  
     \Input pFilter - Filter to populate.
 
     \Output TickerFilterT * - Ptr to pFilter.
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
TickerFilterT *TickerApiFilterGameFifaAll(TickerFilterT *pFilter)
{
    int32_t iIdx;
    TickerFilterItemE aFilter[] = { TICKER_FILT_GAME_FIFA_ALL, TICKER_FILT_INVALID };
    TickerApiFilterNone(pFilter);
    for(iIdx = 0; iIdx < 64; iIdx++)
    {
        if ((int32_t)aFilter[iIdx] == -1)
            break;
        TickerApiFilterSet(pFilter, aFilter[iIdx]);
    }
    return(pFilter);
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterGameAll
 
     \Description
        Build a filter enabling events for all sports
  
     \Input pFilter - Filter to populate.
 
     \Output TickerFilterT * - Ptr to pFilter.
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
TickerFilterT *TickerApiFilterGameAll(TickerFilterT *pFilter)
{
    int32_t iIdx;
    TickerFilterItemE aFilter[] = { TICKER_FILT_GAME_ALL, TICKER_FILT_INVALID };
    TickerApiFilterNone(pFilter);
    for(iIdx = 0; iIdx < 64; iIdx++)
    {
        if ((int32_t)aFilter[iIdx] == -1)
            break;
        TickerApiFilterSet(pFilter, aFilter[iIdx]);
    }
    return(pFilter);
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterNewsAll
 
     \Description
        Build a filter enabling news events for all sports
  
     \Input pFilter - Filter to populate.
 
     \Output TickerFilterT * - Ptr to pFilter.
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
TickerFilterT *TickerApiFilterNewsAll(TickerFilterT *pFilter)
{
    int32_t iIdx;
    TickerFilterItemE aFilter[] = { TICKER_FILT_NEWS_ALL, TICKER_FILT_INVALID };
    TickerApiFilterNone(pFilter);
    for(iIdx = 0; iIdx < 64; iIdx++)
    {
        if ((int32_t)aFilter[iIdx] == -1)
            break;
        TickerApiFilterSet(pFilter, aFilter[iIdx]);
    }
    return(pFilter);
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterLocal
 
     \Description
        Build a filter enabling only local events (buddy, etc)
  
     \Input pFilter - Filter to populate.
 
     \Output TickerFilterT * - Ptr to pFilter.
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
TickerFilterT *TickerApiFilterLocal(TickerFilterT *pFilter)
{
    pFilter->uTop = (uint32_t)((1 << (TICKER_FILT_BUDDY - 32)) | (1 << (TICKER_FILT_LOCAL_MESSAGE - 32)));
    pFilter->uBot = 0;
    return(pFilter);
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterSet
 
     \Description
        Enable a particular event type in the given filter.
          
     \Input pFilter - Filter to modify.
     \Input eItem - Event type to enable.
 
     \Output void - 
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
void TickerApiFilterSet(TickerFilterT *pFilter, TickerFilterItemE eItem)
{
    if (eItem < 32)
    {
        pFilter->uBot |= (1 << eItem);
    }
    else
    {
        pFilter->uTop |= (1 << (eItem - 32));
    }
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterIsSet
 
     \Description
        Check if an event type is enabled in the given filter.
  
     \Input pFilter - Filter to check.
     \Input eItem - Event type to check for.
 
     \Output int32_t - != 0 if event type is set; == 0 otherwise
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
int32_t TickerApiFilterIsSet(TickerFilterT *pFilter, TickerFilterItemE eItem)
{
    if (eItem < 32)
    {
        return(pFilter->uBot & (1 << eItem));
    }
    else
    {
        return(pFilter->uTop & (1 << (eItem - 32)));
    }
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterClear
 
     \Description
        Clear an event type from the given filter.
  
     \Input pFilter - Filter to modify.
     \Input eItem - Event type to clear.
 
     \Output None.
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
void TickerApiFilterClear(TickerFilterT *pFilter, TickerFilterItemE eItem)
{
    if (eItem < 32)
    {
        pFilter->uBot &= ~(1 << eItem);
    }
    else
    {
        pFilter->uTop &= ~(1 << (eItem - 32));
    }
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterIsZero
 
     \Description
        Check if the filter has no events enabled.
  
     \Input pFilter - Filter to check.
 
     \Output int32_t - != 0 if no events are enabled; == 0 if events are enabled.
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
int32_t TickerApiFilterIsZero(const TickerFilterT *pFilter)
{
    return((pFilter->uTop == 0) && (pFilter->uBot == 0));
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterIsEqual
 
     \Description
        Compare two filters for equality.
  
     \Input pFilt0 - First filter to compare.
     \Input pFilt1 - Second filter to compare.
 
     \Output int32_t - != 0 if filters match; == 0 if filters don't match.
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
int32_t TickerApiFilterIsEqual(const TickerFilterT *pFilt0, const TickerFilterT *pFilt1)
{
    return((pFilt0->uTop == pFilt1->uTop) && (pFilt0->uBot == pFilt1->uBot));
}

/*F********************************************************************************/
/*!
    \Function    TickerApiFilterAnd

    \Description
       Perform an AND operation on two filters.

    \Input pSrc0        - LHS of AND operation.
    \Input pSrc1        - RHS of AND operation.
    \Input pDst         - Result is placed in this filter.

    \Output 
        TickerFilterT*   - Pts to pDst.

    \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
TickerFilterT *TickerApiFilterAnd(const TickerFilterT *pSrc0, const TickerFilterT *pSrc1, TickerFilterT *pDst)
{
    pDst->uTop = pSrc0->uTop & pSrc1->uTop;
    pDst->uBot = pSrc0->uBot & pSrc1->uBot;
    return(pDst);
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterOr
 
     \Description
        Perform an OR operation on two filters.
  
     \Input pSrc0 - LHS of OR operation.
     \Input pSrc1 - RHS of OR operation.
     \Input pDst - Result is placed in this filter.
 
     \Output TickerFilterT - Pts to pDst.
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
TickerFilterT *TickerApiFilterOr(const TickerFilterT *pSrc0, const TickerFilterT *pSrc1, TickerFilterT *pDst)
{
    pDst->uTop = pSrc0->uTop | pSrc1->uTop;
    pDst->uBot = pSrc0->uBot | pSrc1->uBot;
    return(pDst);
}

/*F********************************************************************************/
/*!
     \Function TickerApiFilterNot
 
     \Description
        Perform a NOT operation on a filter.
  
     \Input pSrc - Filter to apply NOT operation to.
     \Input pDst - Result is placed in this filter.
 
     \Output TickerFilterT - Pts to pDst.
 
     \Version 01/31/2006 (doneill)
*/
/********************************************************************************F*/
TickerFilterT *TickerApiFilterNot(const TickerFilterT *pSrc, TickerFilterT *pDst)
{
    pDst->uTop = ~pSrc->uTop;
    pDst->uBot = ~pSrc->uBot;
    return(pDst);
}



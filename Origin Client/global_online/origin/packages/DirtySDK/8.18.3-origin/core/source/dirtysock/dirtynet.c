/*H*************************************************************************************************/
/*!

    \File    dirtynet.c

    \Description
        Platform-independent network related routines.

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 1999-2003.  ALL RIGHTS RESERVED.

    \Version    1.0        01/02/02 (GWS) First Version
    \Version    1.1        01/27/03 (JLB) Split from dirtynetwin.c

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <string.h>
#include "dirtysock.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables


// Public variables


/*** Private Functions *****************************************************************/

/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    SockaddrCompare

    \Description
        Compare two sockaddr structs and see if address is the same. This is different
        from simple binary compare because only relevent fields are checked.

    \Input *addr1   - address #1
    \Input *addr2   - address to compare with address #1

    \Output
        int32_t         - zero=no error, negative=error

    \Notes
        None.

    \Version    1.0        10/04/99 (GWS) First Version

*/
/*************************************************************************************************F*/
int32_t SockaddrCompare(struct sockaddr *addr1, struct sockaddr *addr2)
{
    int32_t len = sizeof(*addr1)-sizeof(addr1->sa_family);

    // make sure address family matches
    if (addr1->sa_family != addr2->sa_family)
        return(addr1->sa_family-addr2->sa_family);

    // do type specific comparison
    if (addr1->sa_family == AF_INET)
        len = 2+4;

    // fall back to binary compare
    return(memcmp(addr1->sa_data, addr2->sa_data, len));
}

/*F*************************************************************************************************/
/*!
    \Function    SockaddrInSetAddrText

    \Description
        Set Internet address component of sockaddr struct from textual address (a.b.c.d).

    \Input *addr    - sockaddr structure
    \Input *s       - textual address

    \Output
        int32_t         - zero=no error, negative=error

    \Notes
        None.

    \Version    1.0        10/04/99 (GWS) First Version

*/
/*************************************************************************************************F*/
int32_t SockaddrInSetAddrText(struct sockaddr *addr, const char *s)
{
    int32_t i;
    unsigned char *ipaddr = (unsigned char *)(addr->sa_data+2);

    for (i = 0; i < 4; ++i, ++s) {
        ipaddr[i] = 0;
        while ((*s >= '0') && (*s <= '9'))
            ipaddr[i] = (ipaddr[i]*10) + (*s++ & 15);
        if ((i < 3) && (*s != '.')) {
            ipaddr[0] = ipaddr[1] = 0; ipaddr[2] = ipaddr[3] = 0;
            return(-1);
        }
    }

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    SockaddrInGetAddrText

    \Description
        Return Internet address component of sockaddr struct in textual form (a.b.c.d).

    \Input *addr    - sockaddr struct
    \Input *str     - address buffer
    \Input len      - address length

    \Output
        char *		- returns str on success, NULL on failure

    \Notes
        None.

    \Version    1.0        10/04/99 (GWS) First Version

*/
/*************************************************************************************************F*/
char *SockaddrInGetAddrText(struct sockaddr *addr, char *str, int32_t len)
{
    int32_t i;
	char *s = str;

    // make sure buffer has room
    if (len <= 0)
        return(NULL);
    if (len < 16) {
        *s = 0;
        return(NULL);
    }

    // convert to text form
    for (i = 2; i < 6; ++i) {
        int32_t val = (unsigned char)addr->sa_data[i];
        if (val > 99) {
            *s++ = (char)('0'+(val/100));
            val %= 100;
            *s++ = (char)('0'+(val/10));
            val %= 10;
        }
        if (val > 9) {
            *s++ = (char)('0'+(val/10));
            val %= 10;
        }
        *s++ = (char)('0'+val);
        if (i < 5) {
            *s++ = '.';
        }
    }

    *s = 0;
    return(str);
}

/*F*************************************************************************************************/
/*!
    \Function    SocketHtons

    \Description
        Convert int16_t from host to network byte order

    \Input addr         - value to convert

    \Output
        uint16_t  - converted value

    \Notes
        None.

    \Version    1.0        10/04/99 (GWS) First Version

*/
/*************************************************************************************************F*/
uint16_t SocketHtons(uint16_t addr)
{
    unsigned char netw[2];

    memcpy((char *)netw, (char *)&addr, sizeof(addr));

    return((netw[0]<<8)|(netw[1]<<0));
}

/*F*************************************************************************************************/
/*!
    \Function    SocketHtonl

    \Description
        Convert int32_t from host to network byte order.

    \Input addr         - value to convert

    \Output
        uint32_t    - converted value

    \Notes
        None.

    \Version    1.0        10/04/99 (GWS) First Version

*/
/*************************************************************************************************F*/
uint32_t SocketHtonl(uint32_t addr)
{
    unsigned char netw[4];

    memcpy((char *)netw, (char *)&addr, sizeof(addr));

    return((((((netw[0]<<8)|netw[1])<<8)|netw[2])<<8)|netw[3]);
}

/*F*************************************************************************************************/
/*!
    \Function    SocketNtohs

    \Description
        Convert int16_t from network to host byte order.

    \Input addr         - value to convert

    \Output
        uint16_t  - converted value

    \Notes
        None.

    \Version    1.0        10/0/99 (GWS) First Version

*/
/*************************************************************************************************F*/
uint16_t SocketNtohs(uint16_t addr)
{
    unsigned char netw[2];

    netw[1] = (unsigned char)addr;
    addr >>= 8;
    netw[0] = (unsigned char)addr;

    memcpy((char *)&addr, (char *)netw, sizeof(addr));

    return(addr);
}

/*F*************************************************************************************************/
/*!
    \Function    SocketNtohl

    \Description
        Convert int32_t from network to host byte order.

    \Input addr         - value to convert

    \Output
        uint32_t    - converted value

    \Notes
        None.

    \Version    1.0        10/04/99 (GWS) First Version

*/
/*************************************************************************************************F*/
uint32_t SocketNtohl(uint32_t addr)
{
    unsigned char netw[4];

    netw[3] = (unsigned char)addr;
    addr >>= 8;
    netw[2] = (unsigned char)addr;
    addr >>= 8;
    netw[1] = (unsigned char)addr;
    addr >>= 8;
    netw[0] = (unsigned char)addr;

    memcpy((char *)&addr, (char *)netw, sizeof(addr));

    return(addr);
}

/*F*************************************************************************************************/
/*!
    \Function SocketInAddrGetText
    
    \Description
        Convert 32-bit internet address into textual form.
        
    \Input addr     - address
    \Input *str     - [out] address buffer
    \Input len      - address length

    \Output
        char *		- returns str on success, NULL on failure
            
    \Version 06/17/2009 (jbrookes)
*/
/*************************************************************************************************F*/
char *SocketInAddrGetText(uint32_t addr, char *str, int32_t len)
{
    struct sockaddr sa;
    SockaddrInSetAddr(&sa,addr);
    return(SockaddrInGetAddrText(&sa, str, len));
}

/*F*************************************************************************************************/
/*!
    \Function    SocketInTextGetAddr
    
    \Description
        Convert textual internet address into 32-bit integer form
        
    \Input *pAddrText   - textual address
    
    \Output
        int32_t             - integer form
            
    \Notes
        None.
            
    \Version    1.0        11/23/02 (JLB) First Version
    
*/
/*************************************************************************************************F*/
int32_t SocketInTextGetAddr(const char *pAddrText)
{
    struct sockaddr SockAddr;
    int32_t iAddr = 0;

    if (SockaddrInSetAddrText(&SockAddr, pAddrText) == 0)
    {
        iAddr = SockaddrInGetAddr(&SockAddr);
    }
    return(iAddr);
}

/*F*************************************************************************************************/
/*!
    \Function    SockaddrInParse
    
    \Description
        Convert textual internet address:port into sockaddr structure

        If the textual internet address:port is followed by a second :port, the second port
        is optionally parsed into pPort2, if not NULL.
        
    \Input *sa      - sockaddr to fill in
    \Input *pParse  - textual address
    
    \Output
        int32_t         - flags:
            0=parsed nothing
            1=parsed addr
            2=parsed port
            3=parsed addr+port
            
    \Version    1.0        11/23/02 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t SockaddrInParse(struct sockaddr *sa, const char *pParse)
{
    int32_t iReturn = 0, iPort = 0;
    uint32_t uAddr = 0;

    // init the address
    SockaddrInit(sa, AF_INET);

    // parse addr:port
    iReturn = SockaddrInParse2(&uAddr, &iPort, NULL, pParse);

    // set addr:port in sockaddr
    SockaddrInSetAddr(sa, uAddr);
    SockaddrInSetPort(sa, iPort);

    // return parse info
    return(iReturn);
}

/*F*************************************************************************************************/
/*!
    \Function    SockaddrInParse2
    
    \Description
        Convert textual internet address:port into sockaddr structure

        If the textual internet address:port is followed by a second :port, the second port
        is optionally parsed into pPort2, if not NULL.
        
    \Input *pAddr   - address to fill in
    \Input *pPort   - port to fill in
    \Input *pPort2  - second port to fill in         
    \Input *pParse  - textual address
    
    \Output
        int32_t         - flags:
            0=parsed nothing
            1=parsed addr
            2=parsed port
            3=parsed addr+port
            4=parsed port2
            
    \Version    1.0        11/23/02 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t SockaddrInParse2(uint32_t *pAddr, int32_t *pPort, int32_t *pPort2, const char *pParse)
{
    int32_t iReturn = 0;
    uint32_t uVal;

    // skip embedded white-space
    while ((*pParse > 0) && (*pParse <= ' '))
    {
        ++pParse;
    }

	// parse the address (no dns for listen)
	for (uVal = 0; ((*pParse >= '0') && (*pParse <= '9')) || (*pParse == '.'); ++pParse)
    {
		// either add or shift
		if (*pParse != '.')
        {
			uVal = (uVal - (uVal & 255)) + ((uVal & 255) * 10) + (*pParse & 15);
        }
		else
        {
            uVal <<= 8;
        }
	}
    if ((*pAddr = uVal) != 0)
    {
        iReturn |= 1;
    }

	// skip non-port info
	while ((*pParse != ':') && (*pParse != 0))
    {
		++pParse;
    }

    // parse the port
    uVal = 0;
	if (*pParse == ':')
    {
		for (++pParse; (*pParse >= '0') && (*pParse <= '9'); ++pParse)
        {
			uVal = (uVal * 10) + (*pParse & 15);
        }
        iReturn |= 2;
	}
    *pPort = (int32_t)uVal;

    // parse port2 (optional)
    if (pPort2 != NULL)
    {
        uVal = 0;
	    if (*pParse == ':')
        {
		    for (++pParse; (*pParse >= '0') && (*pParse <= '9'); ++pParse)
            {
			    uVal = (uVal * 10) + (*pParse & 15);
            }
            iReturn |= 4;
	    }
        *pPort2 = (int32_t)uVal;
    }

    // return the address
    return(iReturn);
}

/*F*************************************************************************************************/
/*!
    \Function SocketSimulatePacketLoss
    
    \Description
        A very basic packet loss simulation driver.  Returns true if a packet should be considered
        lost else false.  This code should be called by the low-level SocketRecvfrom() functions and
        a true result should cause the packet to be not returned to the caller; a "no data" result
        should be returned instead (typically SOCKERR_NONE).
        
    \Input uPacketLossParam - param for tuning packet loss (see Notes)
    
    \Output
        uint32_t            - TRUE if packet send should be skipped, else FALSE

    \Notes
        Format of the uPacketLossParam field:
        
        \verbatim
                1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
                F E D C B A 9 8 7 6 5 4 3 2 1 0 F E D C B A 9 8 7 6 5 4 3 2 1 0
                | |___________________________| |_____________| |_____________|
                |              |                       |               |
             verbose        unused                 frequency       duration

            -verbose: if set, packet loss events will be printed to diagnostic output
            -frequency: max frequency of events (min zero seconds): [0...255s]
            -duration: max duration of events (min zero milliseconds): [0...255ms]
        \endverbatim
            
    \Version 06/25/2009 (jbrookes)
*/
/*************************************************************************************************F*/
#if DIRTYCODE_DEBUG
uint32_t SocketSimulatePacketLoss(uint32_t uPacketLossParam)
{
    static uint32_t _uRandPacketTimer = 0;
    static uint32_t _bPacketLossEvent = FALSE;
    static uint32_t _uPacketsLost = 0;
    
    // extract params
    uint32_t uDuration = uPacketLossParam & 0xff;
    uint32_t uFrequency = (uPacketLossParam >> 8) & 0xff;
    uint32_t bVerbose = uPacketLossParam >> 31;

    // all zeros means disabled    
    if (uPacketLossParam == 0)
    {
        return(0);
    }
    
    // set up next packet loss event
    if (_uRandPacketTimer == 0)
    {
        // generate number of milliseconds before next packet loss event, 0s..uDuration seconds
        _uRandPacketTimer = NetTick() + NetRand(1000*uFrequency) + 1;
    }
    
    // wait for packet loss event
    if ((_uRandPacketTimer != 0) && (_bPacketLossEvent == FALSE) && (NetTickDiff(NetTick(), _uRandPacketTimer) > 0))
    {
        // generate window length packets will be lost over (simulate burst packet loss), 1ms..100ms
        _uRandPacketTimer = NetRand(uDuration) + 1;
        if (bVerbose)
        {
            NetPrintf(("dirtynet: simulating packet loss over a %dms window\n", _uRandPacketTimer));
        }
        _uRandPacketTimer += NetTick();
        // initiate packet loss event
        _bPacketLossEvent = TRUE;
    }

    // handle packet loss event
    if (_bPacketLossEvent == TRUE)
    {
        // check if packet timer window has elapsed
        if (NetTickDiff(NetTick(), _uRandPacketTimer) > 0)
        {
            if (bVerbose)
            {
                NetPrintf(("dirtynet: %d packets lost\n", _uPacketsLost));
            }
            _bPacketLossEvent = FALSE;
            _uRandPacketTimer = 0;
            _uPacketsLost = 0;
        }
        else
        {
            _uPacketsLost += 1;
        }
    }
    
    return(_bPacketLossEvent);
}
#endif

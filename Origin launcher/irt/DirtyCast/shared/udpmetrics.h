/*H********************************************************************************/
/*!
    \File udpmetrics.h

    \Description
        Implements the ability to get socket diagnostics via AF_NETLINK sockets

    \Copyright
        Copyright (c) 2019 Electronic Arts Inc.

    \Version 04/01/2019 (eesponda)
*/
/********************************************************************************H*/

#ifndef _udpmetrics_h
#define _udpmetrics_h

/*** Include Files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Type Definitions *************************************************************/

//! structure used to collect udp metrics and then pass them to metrics handlers
typedef struct UdpMetricsT
{
    uint32_t uSendQueueLen; //!< the amount of data in the receive queue
    uint32_t uRecvQueueLen; //!< the amount of data in send queue
} UdpMetricsT;

/*** Functions ********************************************************************/

// open the AF_NETLINK socket to query metrics
int32_t UdpMetricsSocketOpen(void);

// send a query for udp metrics to the kernel
void UdpMetricsQuerySend(int32_t iSocket, uint16_t uPort);

// retrieve the results of the query
void UdpMetricsQueryRecv(int32_t iSocket, UdpMetricsT *pUdpMetrics);

// close the AF_NETLINK socket
void UdpMetricsSocketClose(int32_t iSocket);

#endif // _udpmetrics_h

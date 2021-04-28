/*H********************************************************************************/
/*!
    \File tcpmetrics.h

    \Description
        Implements the ability to get socket diagnostics via AF_NETLINK sockets

    \Copyright
        Copyright (c) 2019 Electronic Arts Inc.

    \Version 11/01/2019 (eesponda)
*/
/********************************************************************************H*/

#ifndef _tcpmetrics_h
#define _tcpmetrics_h

/*** Include Files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Type Definitions *************************************************************/

//! structure used to collect udp metrics and then pass them to metrics handlers
typedef struct TcpMetricsT
{
    uint32_t uSendQueueLen; //!< the amount of data in the receive queue
    uint32_t uRecvQueueLen; //!< the amount of data in send queue
} TcpMetricsT;

/*** Functions ********************************************************************/

// open the AF_NETLINK socket to query metrics
int32_t TcpMetricsSocketOpen(void);

// send a query for udp metrics to the kernel
void TcpMetricsQuerySend(int32_t iSocket, uint16_t uPort);

// retrieve the results of the query
void TcpMetricsQueryRecv(int32_t iSocket, TcpMetricsT *pTcpMetrics);

// close the AF_NETLINK socket
void TcpMetricsSocketClose(int32_t iSocket);

#endif // _tcpmetrics_h

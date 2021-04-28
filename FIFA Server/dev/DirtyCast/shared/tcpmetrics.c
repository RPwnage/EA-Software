/*H********************************************************************************/
/*!
    \File tcpmetrics.h

    \Description
        Implements the ability to get socket diagnostics via AF_NETLINK sockets

    \Copyright
        Copyright (c) 2019 Electronic Arts Inc.

    \Todo
        There is overlap between this module and the udpmetrics module but because
        of how the legacy querying for udp sockets work we can't combine them.
        When we remove support for the legacy kernels we should combine the
        modules.

    \Version 11/01/2019 (eesponda)
*/
/********************************************************************************H*/

/*** Include Files ****************************************************************/

#include "DirtySDK/platform.h"

#if defined(DIRTYCODE_LINUX)
#include <linux/version.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/inet_diag.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "DirtySDK/dirtysock/dirtyerr.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
#include <linux/sock_diag.h>
#endif
#endif

#include "dirtycast.h"
#include "tcpmetrics.h"

/*** Defines *********************************************************************/

//! msg type we used based on kernel version
#if defined(DIRTYCODE_LINUX)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
  #define TCPMETRICS_MSGTYPE (SOCK_DIAG_BY_FAMILY)
#else
  #define TCPMETRICS_MSGTYPE (TCPDIAG_GETSOCK)
#endif
#endif

/*** Type Definitions *************************************************************/

#if defined(DIRTYCODE_LINUX)
//! this type wraps the request we send via the AF_NETLINK socket
typedef struct TcpMetricsDiagnosticRequestT
{
    struct nlmsghdr Header;         //!< header required by the netlink request
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
    struct inet_diag_req_v2 Data;   //!< request data which specifies the socket we are after
    #else
    struct inet_diag_req Data;      //!< request data which specifies the socket we are after
    #endif
} TcpMetricsDiagnosticRequestT;
#endif

/*** Variables ********************************************************************/

static TcpMetricsCbT *_TcpMetrics_pCallback = NULL;
static void *_TcpMetrics_pUserData = NULL;

/*** Public Functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function TcpmetricsSocketOpen

    \Description
        Open a non-blocking AF_NETLINK socket to query TCP metrics from

    \Input *pCallback   - callback to be invoked when new metrics samples are ready
    \Input *pUserData   - user data that will be passed to the callback

    \Output
        int32_t - socket descriptor or negative on error

    \Version 11/01/2019 (eesponda)
*/
/********************************************************************************F*/
int32_t TcpMetricsSocketOpen(TcpMetricsCbT *pCallback, void *pUserData)
{
    int32_t iSocket = -1;

    if (_TcpMetrics_pCallback == NULL)
    {
        _TcpMetrics_pCallback = pCallback;
        _TcpMetrics_pUserData = pUserData;
    }
    else
    {
        DirtyCastLogPrintf("tcpmetrics: can't create multiple netlink diagnostic socket\n");
        return(-1);
    }

    #if defined(DIRTYCODE_LINUX)
    /* create netlink socket
       SOCK_RAW is used because we build NETLINK msgs ourselves and we expect the send/recv operation
       to be datagram-based (not bytestream-based) */
    if ((iSocket = socket(AF_NETLINK, SOCK_RAW, NETLINK_INET_DIAG)) < 0)
    {
        DirtyCastLogPrintf("tcpmetrics: creating netlink diagnostic socket failed (%s)\n", DirtyErrGetName(errno));
    }

    if (iSocket >= 0)
    {
        // set NETLINK socket as non-blocking
        if (fcntl(iSocket, F_SETFL, O_NONBLOCK) < 0)
        {
            DirtyCastLogPrintf("tcpmetrics: could not set netlink diagnostic socket as non-blocking (%s)\n", DirtyErrGetName(errno));
            close(iSocket);
            iSocket = -1;
        }
    }

    if (iSocket >= 0)
    {
        // bind netlink socket --> tells netlink layer that we are uniquely identified by our process id
        struct sockaddr_nl SrcNlAddr = { .nl_family = AF_NETLINK, .nl_pid = DirtyCastGetPid(), .nl_groups = 0 };
        if (bind(iSocket, (struct sockaddr*)&SrcNlAddr, sizeof(SrcNlAddr) ) < 0)
        {
            DirtyCastLogPrintf("tcpmetrics: could not bind netlink diagnostic socket\n", DirtyErrGetName(errno));
            close(iSocket);
            iSocket = -1;
        }
    }
    #endif

    return(iSocket);
}

/*F********************************************************************************/
/*!
    \Function TcpMetricsQuerySend

    \Description
        Send a query on the AF_NETLINK socket for metrics

    \Input iSocket  - socket descriptor
    \Input uPort    - port we are querying

    \Notes
        Based on the implementation of DirtySDK, we know that all our sockets
        will be on the IPv6 stack.

    \Version 11/01/2019 (eesponda)
*/
/********************************************************************************F*/
void TcpMetricsQuerySend(int32_t iSocket, uint16_t uPort)
{
    #if defined(DIRTYCODE_LINUX)
    struct sockaddr_nl DestNlAddr = { .nl_family = AF_NETLINK, .nl_pid = 0 };  // pid 0 means: destination is 'kernel'
    TcpMetricsDiagnosticRequestT Request =
    {
        // nlmsghdr
        { .nlmsg_len = sizeof(Request), .nlmsg_type = TCPMETRICS_MSGTYPE, .nlmsg_flags = (NLM_F_REQUEST | NLM_F_DUMP) },
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
        // inet_diag_req_v2
        { .sdiag_family = AF_INET6, .sdiag_protocol = IPPROTO_TCP, .idiag_ext = INET_DIAG_MEMINFO, .idiag_states = -1, .id = { .idiag_sport = htons(uPort) } }
        #else
        // inet_diag_req
        { .idiag_family = AF_INET6, .idiag_ext = INET_DIAG_MEMINFO, .idiag_states = -1, .id = { .idiag_sport = htons(uPort) } }
        #endif
    };

    // send out the request
    if (sendto(iSocket, &Request, sizeof(Request), 0, (const struct sockaddr *)&DestNlAddr, sizeof(DestNlAddr)) < 0)
    {
        DirtyCastLogPrintf("tcpmetrics: could not send diagnostics query via AF_NETLINK socket (%s)\n", DirtyErrGetName(errno));
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function TcpMetricsQueryRecv

    \Description
        Receive the result of a sent query

    \Input iSocket      - identifier of the AF_NETLINK socket

    \Version 11/01/2019 (eesponda)
*/
/********************************************************************************F*/
void TcpMetricsQueryRecv(int32_t iSocket)
{
    #if defined(DIRTYCODE_LINUX)
    int32_t iResult;
    uint32_t uBytesToConsume;
    uint8_t aBuffer[16 * 1024];   // enough to receive 71 x 116-byte responses from kernel 
    struct sockaddr_nl SrcNlAddr; // will be filled in by recvfrom(), should have .nl_pid=0 for all kernel-originated responses
    socklen_t SrcNlAddrSize = sizeof(SrcNlAddr);

    // receive messages until recvfrom() returns an error or the kernel sends us done
    for (;;)
    {
        // postion message header pointer to beginning of recv buffer passed to recvfrom()
        const struct nlmsghdr *pHdr = (const struct nlmsghdr *)aBuffer;
 
        // attempt to receive the message from the kernel, hiding EWOULDBLOCK errors
        if ((iResult = recvfrom(iSocket, &aBuffer[0], sizeof(aBuffer), 0, (struct sockaddr *)&SrcNlAddr, &SrcNlAddrSize)) < 0)
        {
            if (errno != EWOULDBLOCK)
            {
                DirtyCastLogPrintf("tcpmetrics: could not receive diagnostics results, recvmsg returned (%s)\n", DirtyErrGetName(errno));
            }
            return;
        }
 
        /* loop through the messages we received from the kernel. in my experience we only receive one at a time per recvmsg call but in case
           behavior would to change, we loop here. */
        uBytesToConsume = iResult;

        while (1)
        {
            // stop looping if header pointer has hit end of list
            if (pHdr == NULL)
            {
                 break;
            }

            // stop looping if out of bytes to consume
            if (uBytesToConsume <= 0)
            {
                break;
            }

            // stop looping if integrity of msg header is not good
            if (!NLMSG_OK(pHdr, uBytesToConsume))
            {
                break;
            }

            /* inspect message type */

            // stop looping of end-of-sequence message found
            if (pHdr->nlmsg_type == NLMSG_DONE)
            {
                break;
            }
            // report any errors found
            else if ((pHdr->nlmsg_type == NLMSG_ERROR) && (pHdr->nlmsg_len >= NLMSG_LENGTH(sizeof(struct nlmsgerr))))
            {
                const struct nlmsgerr *pError = NLMSG_DATA(pHdr);
                DirtyCastLogPrintf("tcpmetrics: received error from kernel (%s)\n", DirtyErrGetName(-pError->error));
            }
            // is this the specific msg we are looking for?
            else if ((pHdr->nlmsg_type == TCPMETRICS_MSGTYPE) && (pHdr->nlmsg_len >= NLMSG_LENGTH(sizeof(struct inet_diag_msg))))
            {
                const struct inet_diag_msg *pDiag = NLMSG_DATA(pHdr);
                uint32_t uAttributeLen = pHdr->nlmsg_len - NLMSG_LENGTH(sizeof(*pDiag));
                const struct rtattr *pAttribute;

                // look for the attribute which contains the queue information
                for (pAttribute = (const struct rtattr *)(pDiag + 1); RTA_OK(pAttribute, uAttributeLen); pAttribute = RTA_NEXT(pAttribute, uAttributeLen))
                {
                    if (pAttribute->rta_type == INET_DIAG_MEMINFO)
                    {
                        const struct inet_diag_meminfo *pMemInfo = RTA_DATA(pAttribute);
                        TcpMetricsT TcpMetrics;
                        TcpMetrics.uSendQueueLen = pMemInfo->idiag_tmem;
                        TcpMetrics.uRecvQueueLen = pMemInfo->idiag_rmem;
                        TcpMetrics.uSrcPort = ntohs(pDiag->id.idiag_sport);
                        TcpMetrics.uDstPort = ntohs(pDiag->id.idiag_dport);

                        // notify higher-level code about new socket info samples being available
                        _TcpMetrics_pCallback(&TcpMetrics, _TcpMetrics_pUserData);
                    }
                }
            }
            // move to the next message
            pHdr = NLMSG_NEXT(pHdr, uBytesToConsume);
        }
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function TcpMetricsSocketClose

    \Description
        Close the AF_NETLINK socket

    \Input iSocket      - socket descriptor

    \Version 11/01/2019 (eesponda)
*/
/********************************************************************************F*/
void TcpMetricsSocketClose(int32_t iSocket)
{
    #if defined(DIRTYCODE_LINUX)
    close(iSocket);
    #endif
}

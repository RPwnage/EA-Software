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

/*** Public Functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function TcpmetricsSocketOpen

    \Description
        Open a non-blocking AF_NETLINK socket to query TCP metrics from

    \Output
        int32_t - socket descriptor or negative on error

    \Version 11/01/2019 (eesponda)
*/
/********************************************************************************F*/
int32_t TcpMetricsSocketOpen(void)
{
    int32_t iSocket = -1;

    #if defined(DIRTYCODE_LINUX)
    if ((iSocket = socket(AF_NETLINK, SOCK_RAW, NETLINK_INET_DIAG)) < 0)
    {
        DirtyCastLogPrintf("tcpmetrics: creating netlink diagnostic socket failed (%s)\n", DirtyErrGetName(errno));
    }
    else if (fcntl(iSocket, F_SETFL, O_NONBLOCK) < 0)
    {
        DirtyCastLogPrintf("tcpmetrics: could not set netlink diagnostic socket as non-blocking (%s)\n", DirtyErrGetName(errno));
        close(iSocket);
        iSocket = -1;
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
    int32_t iResult;
    struct sockaddr_nl NlAddr = { .nl_family = AF_NETLINK };
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
    struct iovec Iov = { .iov_base = &Request, .iov_len = sizeof(Request) };
    struct msghdr Msg = { .msg_name = (void *)&NlAddr, .msg_namelen = sizeof(NlAddr), .msg_iov = &Iov, .msg_iovlen = 1 };

    // send out the request
    if ((iResult = sendmsg(iSocket, &Msg, 0)) < 0)
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

    \Input iSocket      - socket descriptor
    \Input *pTcpMetrics - [out] result of the query

    \Notes
        The output might not get updated if we don't receive anything from the
        kernel.

    \Version 11/01/2019 (eesponda)
*/
/********************************************************************************F*/
void TcpMetricsQueryRecv(int32_t iSocket, TcpMetricsT *pTcpMetrics)
{
    #if defined(DIRTYCODE_LINUX)
    int32_t iResult;
    uint8_t aBuffer[512];
    struct sockaddr_nl NlAddr = { .nl_family = AF_NETLINK };
    const struct nlmsghdr *pHdr = (const struct nlmsghdr *)aBuffer;
    struct iovec Iov = { .iov_base = aBuffer, .iov_len = sizeof(aBuffer) };
    struct msghdr Msg = { .msg_name = &NlAddr, .msg_namelen = sizeof(NlAddr), .msg_iov = &Iov, .msg_iovlen = 1 };

    // receive messages until recvmsg returns an error or the kernel sends us done
    for (;;)
    {
        // attempt to receive the message from the kernel, hiding EWOULDBLOCK errors
        if ((iResult = recvmsg(iSocket, &Msg, 0)) < 0)
        {
            if (errno != EWOULDBLOCK)
            {
                DirtyCastLogPrintf("tcpmetrics: could not receive diagnostics results, recvmsg returned (%s)\n", DirtyErrGetName(errno));
            }
            return;
        }
        /* loop through the messages we received from the kernel. in my experience we only receive one at a time per recvmsg call but in case
           behavior would to change, we loop here. */
        while (NLMSG_OK(pHdr, (uint32_t)iResult))
        {
            // nothing left to do if the kernel sent the done message
            if (pHdr->nlmsg_type == NLMSG_DONE)
            {
                return;
            }
            // report any errors found
            else if ((pHdr->nlmsg_type == NLMSG_ERROR) && (pHdr->nlmsg_len >= NLMSG_LENGTH(sizeof(struct nlmsgerr))))
            {
                const struct nlmsgerr *pError = NLMSG_DATA(pHdr);
                DirtyCastLogPrintf("tcpmetrics: received error from kernel (%s)\n", DirtyErrGetName(-pError->error));
            }
            // check for the specific msg we are looking for
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
                        pTcpMetrics->uSendQueueLen = pMemInfo->idiag_tmem;
                        pTcpMetrics->uRecvQueueLen = pMemInfo->idiag_rmem;
                    }
                }
            }
            // move to the next message
            pHdr = NLMSG_NEXT(pHdr, iResult);
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

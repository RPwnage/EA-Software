/*************************************************************************************************/
/*!
    \file


    Common file to handle socket-layer differences between Windows and Linux.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef PLATFORMSOCKET_H
#define PLATFORMSOCKET_H

/*** Include files *******************************************************************************/

// Unix/Windows specific includes for sockets.
#ifdef EA_PLATFORM_LINUX
    #include <arpa/inet.h>
    #include <sys/ioctl.h>
    #include <errno.h>
    #include <unistd.h>
#endif

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


#if defined(EA_PLATFORM_LINUX)

typedef int SOCKET;
const SOCKET INVALID_SOCKET = (SOCKET)-1;
#define SOCKET_ERROR (-1)
#define WSAGetLastError() (errno)
#define WSASetLastError(_err) (errno = _err)
#define closesocket close

#define SOCKET_WOULDBLOCK(err) (((err) == EWOULDBLOCK) || ((err) == EAGAIN))
#define SOCKET_INPROGRESS(err) ((err) == EINPROGRESS)
#define SOCKET_NOFDS(err) ((err) == EMFILE)

#define SHUT_HOW_READ SHUT_RD
#define SHUT_HOW_WRITE SHUT_WR
#define SHUT_HOW_READWRITE SHUT_RDWR

#elif defined(EA_PLATFORM_WINDOWS)

typedef int32_t socklen_t;

#define SOCKET_WOULDBLOCK(err) ((err) == WSAEWOULDBLOCK)
#define SOCKET_INPROGRESS(err) ((err) == WSAEINPROGRESS)
#define SOCKET_NOFDS(err) ((err) == WSAEMFILE)

#define MSG_NOSIGNAL 0

#define SHUT_HOW_READ SD_RECEIVE
#define SHUT_HOW_WRITE SD_SEND
#define SHUT_HOW_READWRITE SD_BOTH

#endif

#endif // PLATFORMSOCKET_H



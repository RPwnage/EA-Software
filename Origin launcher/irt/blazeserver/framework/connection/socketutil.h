/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SOCKETUTIL_H
#define BLAZE_SOCKETUTIL_H

/*** Include files *******************************************************************************/
#include "framework/connection/platformsocket.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define NIPQUAD(addr) \
    ((unsigned char*)&(addr))[0], \
    ((unsigned char*)&(addr))[1], \
    ((unsigned char*)&(addr))[2], \
    ((unsigned char*)&(addr))[3]

#if defined(EA_SYSTEM_LITTLE_ENDIAN)
#define HIPQUAD(addr) \
    ((unsigned char*)&(addr))[3], \
    ((unsigned char*)&(addr))[2], \
    ((unsigned char*)&(addr))[1], \
    ((unsigned char*)&(addr))[0]
#elif defined(EA_SYSTEM_BIG_ENDIAN)
#define HIPQUAD NIPQUAD
#endif

namespace Blaze
{

class InetAddress;
class BootConfig;

class SocketUtil
{
    NON_COPYABLE(SocketUtil);

public:
    static SOCKET createSocket(bool enableBlocking, int32_t type = SOCK_STREAM, int32_t protocol = 0);

    static bool setSocketBlockingMode(SOCKET sock, bool enableBlocking);
    static bool setReuseAddr(SOCKET sock, bool reuseAddr);
    static bool setLinger(SOCKET sock, bool enableLinger, uint16_t timeInSec);

    static bool setKeepAlive(SOCKET sock, bool keepAlive);

    static uint32_t getInterface(const InetAddress* target = nullptr);

    static bool initializeNetworking();
    static bool shutdownNetworking();

    static const char8_t* getErrorMsg(char8_t* buf, size_t len, int32_t error);

    static void initialize(const BootConfig& config);

    static bool enableKeepAlive(SOCKET sock); // DEPRECATED - use setKeepAlive instead

    // On the Windows platform only, this function will attempt to exempt this application from any INBOUND firewall rules.
    static bool configureInboundWindowsFirewallRule();

private:
#if defined(EA_PLATFORM_WINDOWS)
    static WSADATA wsaData;
#endif
    static int32_t mKeepAliveInterval;
    static int32_t mKeepAliveCount;

#ifdef EA_PLATFORM_LINUX
    static uint32_t getInterfaceAddress(const char* pIfaceName);
#endif
};

} // Blaze

#endif // BLAZE_SOCKETUTIL_H

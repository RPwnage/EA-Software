/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#ifdef EA_PLATFORM_LINUX
    #include <unistd.h>
    #include <errno.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <net/if.h>
    #include <net/route.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <sys/ioctl.h>
    #include <arpa/inet.h>
#else
    #include <windows.h>
    #include <stdio.h>
    #include <netfw.h>
    #include <comutil.h>
    #include <atlcomcli.h>
    #include <shellapi.h>
    #include <string.h>
    #include "EASTL/string.h"
    #include "EAStdC/EAProcess.h"
    #include "EAIO/EAFileBase.h"
    #include "EAIO/EAFileUtil.h"
    #include "EAIO/PathString.h"
    #pragma comment( lib, "ole32.lib" )
    #pragma comment( lib, "oleaut32.lib" )
#endif
#include "framework/connection/socketutil.h"
#include "framework/connection/inetaddress.h"
#include "framework/util/shared/blazestring.h"
#include "framework/tdf/frameworkconfigtypes_server.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define DEFAULT_KEEP_ALIVE_INTERVAL 15
#define DEFAULT_KEEP_ALIVE_COUNT 4

#if defined(EA_PLATFORM_WINDOWS)
WSADATA SocketUtil::wsaData;
#endif

int32_t SocketUtil::mKeepAliveInterval = DEFAULT_KEEP_ALIVE_INTERVAL;
int32_t SocketUtil::mKeepAliveCount = DEFAULT_KEEP_ALIVE_COUNT;

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \class SocketUtil

    Static helper methods for dealing with sockets.
*/
/*************************************************************************************************/


/*************************************************************************************************/
/*!
    \brief createSocket

    Create a new socket with keep-alive enabled, and set the initial blocking mode.

    \param[in]  enableBlocking - The Selector to register with
    \param[in]  type           - The type of socket to create (SOCK_STREAM, SOCK_DGRAM, etc)
    \param[in]  protocol       - The protocol to be used

    \return - Handle to the new socket, or INVALID_SOCKET if it failed to create, 
              failed to set the blocking mode, or failed to enable keep-alive.
*/
/*************************************************************************************************/
SOCKET SocketUtil::createSocket(bool enableBlocking, int32_t type, int32_t protocol)
{
    SOCKET sock = socket(AF_INET, type, protocol);
    if (sock == INVALID_SOCKET)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SocketUtil].createSocket: Creating socket system call failed.");
        return INVALID_SOCKET;
    }

    if (!setSocketBlockingMode(sock, enableBlocking))
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SocketUtil].createSocket: Setting the blocking mode of the socket failed.");
        closesocket(sock);
        return INVALID_SOCKET;
    }

    if (type == SOCK_STREAM && !setKeepAlive(sock, true))
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SocketUtil].createSocket: : setKeepAlive is failed.");
        closesocket(sock);
        return INVALID_SOCKET;
    }
    return sock;
}

/*************************************************************************************************/
/*!
    \brief setSocketBlockingMode

    Set the blocking mode of the socket

    \param[in]  sock - Socket handle to modify
    \param[in]  enableBlocking - The Selector to register with

    \return - Handle to the new socket, or INVALID_SOCKET if it failed to create, or if the
              blocking mode could not be set.
*/
/*************************************************************************************************/
bool SocketUtil::setSocketBlockingMode(SOCKET sock, bool enableBlocking)
{
#ifdef EA_PLATFORM_WINDOWS
    u_long isNonBlocking = !enableBlocking;
    if (ioctlsocket(sock, FIONBIO, &isNonBlocking) == SOCKET_ERROR)
        return false;
#else
    unsigned long isNonBlocking = !enableBlocking;
    if (ioctl(sock, FIONBIO, (char *)&isNonBlocking) < 0)
        return false;
#endif
    return true;
}

bool SocketUtil::initializeNetworking()
{         
#if defined(EA_PLATFORM_WINDOWS)
    // Initialize Winsock
    int result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (result != 0)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SocketUtil].initializeNetworking: WSAStartup failed: " << result);
        return false;
    }
    return true;
#elif defined(EA_PLATFORM_LINUX)
    return true;
#else
    return false;
#endif
}

bool SocketUtil::shutdownNetworking()
{
#if defined(EA_PLATFORM_WINDOWS)
    // Cleanup Winsock
    WSACleanup();
    return true;
#else
    return true;
#endif
}

/*************************************************************************************************/
/*!
     \brief getInterface
 
      Get the IP address of the interface that would be used if a packet with the given address
      was to be send from this machine.  This code was borrowed from the EAFN lobby server codebase.
  
     \param[in] addr - IP address of target.
 
     \return IP address of interface to be used (nullptr on failure)
*/
/*************************************************************************************************/
#ifdef EA_PLATFORM_WINDOWS
uint32_t SocketUtil::getInterface(const InetAddress* target)
{
    SOCKET sock;
    struct sockaddr_in Host;
    struct sockaddr_in Dest;
    uint32_t uSource = 0;
    uint32_t uTarget = (target == nullptr) ? 0 : target->getIp();

    // create a temp socket (must be datagram)
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock != INVALID_SOCKET)
    {
        int32_t iLen;
        char strHost[256];
        
        // get interface that would be used for this dest
        memset(&Dest, 0, sizeof(Dest));
        Dest.sin_family = AF_INET;
        Dest.sin_addr.s_addr = uTarget;
        memset(&Host, 0, sizeof(Host));
        iLen = sizeof(Host);
        if (WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, (void *)&Dest, sizeof(Dest), &Host, sizeof(Host), (LPDWORD)&iLen, nullptr, nullptr) >= 0)
        {
            uSource = ntohl(Host.sin_addr.s_addr);
        }
        else
        {
            // use classic gethosthame/gethostbyname
            // get machine name
            gethostname(strHost, sizeof(strHost));

            // lookup ip info
            struct addrinfo hints;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            struct addrinfo* list = nullptr;

            BLAZE_PUSH_ALLOC_OVERRIDE(MEM_GROUP_FRAMEWORK_NOLEAK, "getaddrinfo");
            int result = getaddrinfo(strHost, nullptr, &hints, &list);
            BLAZE_POP_ALLOC_OVERRIDE();

            if (result != 0)
            {
                BLAZE_WARN_LOG(Log::CONNECTION, "SocketUtil::getInterface: the hostname(" << strHost << ") resolution failed, result=" << result);                
            }
            else
            {
                uSource = ((sockaddr_in*)list->ai_addr)->sin_addr.s_addr;
            }
            freeaddrinfo(list);
        }
        // close the socket
        closesocket(sock);
    }

    // return source address
    return(htonl(uSource));
}
#else
uint32_t SocketUtil::getInterface(const InetAddress* target)
{
    uint32_t uTarget = (target == nullptr) ? 0 : target->getIp();
    uint32_t uSource = 0;
    char strIface[33];
    uint32_t uDest;
    uint32_t uGateway;
    uint32_t uFlags;
    uint32_t uRefCnt;
    uint32_t uUse;
    uint32_t uMetric;
    uint32_t uMask;
    uint32_t bestMask = 0;
    uint32_t bestMetric = UINT32_MAX;
    char strBuf[256];
    FILE* pFp;

    pFp = fopen("/proc/net/route", "r");
    if (pFp == nullptr)
        return 0;

    // Consume the header line
    fgets(strBuf, sizeof(strBuf), pFp);

    // Read the routing entries
    while (fgets(strBuf, sizeof(strBuf), pFp) != nullptr)
    {
        sscanf(strBuf, "%32s %08X %08X %04d %d %d %d %08X", strIface,
                &uDest, &uGateway, &uFlags, &uRefCnt, &uUse, &uMetric, &uMask);
        // IF the route is up
        if ((uFlags & RTF_UP) != 0)
        {
            // IF the target address matches this destination
            // (if this is true for more than one interface, use the most specific route with the highest priority)
            if ( ( (uTarget & uMask) == (uDest & uMask) ) &&
                 ( (uMask > bestMask) || ((uMask == bestMask) && (uMetric <= bestMetric)) ) )
            {
                bestMask = uMask;
                bestMetric = uMetric;
                uSource = SocketUtil::getInterfaceAddress(strIface);
            }
        }
    }
    fclose(pFp);
    return uSource;
}

uint32_t SocketUtil::getInterfaceAddress(const char* pIfaceName)
{                   
    int sock;   
    uint32_t uSource = 0;
        
    // create a temp socket (must be datagram)
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock != -1)
    {
        int32_t iIndex;
        int32_t iCount;
        struct ifreq EndpRec[16];
        struct ifconf EndpList;
        struct sockaddr_in Host;

        // request list of interfaces
        memset(&EndpList, 0, sizeof(EndpList));
        EndpList.ifc_req = EndpRec;
        EndpList.ifc_len = sizeof(EndpRec);
        if (ioctl(sock, SIOCGIFCONF, &EndpList) >= 0)
        {
            // figure out number and walk the list
            iCount = EndpList.ifc_len / sizeof(EndpRec[0]);
            for (iIndex = 0; iIndex < iCount; ++iIndex)
            {
                memcpy(&Host, &EndpRec[iIndex].ifr_addr, sizeof(Host));
                if (strcmp(EndpRec[iIndex].ifr_name, pIfaceName) == 0)
                {
                    uSource = Host.sin_addr.s_addr;
                    break;
                }
            }
        }
        // close the socket
        close(sock);
    }

    // return source address
    return (uSource);
}

#endif

// static
const char8_t* SocketUtil::getErrorMsg(char8_t* buf, size_t len, int32_t error)
{
#ifdef EA_PLATFORM_WINDOWS
    if (0 != FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPSTR)buf,len,nullptr ))
    { 
        return buf;
    }

    if (strerror_s(buf, len, error) != 0)
    {
        blaze_strnzcpy(buf, "Unknown error", len);
    }       
#else
    buf = strerror_r(error, buf, len);
#endif
    return buf;
}

bool SocketUtil::enableKeepAlive(SOCKET sock)
{
    return setKeepAlive(sock, true);
}

bool SocketUtil::setKeepAlive(SOCKET sock, bool keepAlive)
{
#ifndef EA_PLATFORM_WINDOWS
    if ((mKeepAliveInterval == 0) || (mKeepAliveCount == 0))
    {
        // Keep-alives are disabled at the process level.
        return true;
    }

    int val = 0;
    if (keepAlive)
        val = 1;

    if (::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) != 0)
        return false;

    val = mKeepAliveInterval;
    if (::setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, &val, sizeof(val)) != 0)
        return false;

    val = mKeepAliveInterval;
    if (::setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, &val, sizeof(val)) != 0)
        return false;

    val = mKeepAliveCount;
    if (::setsockopt(sock, SOL_TCP, TCP_KEEPCNT, &val, sizeof(val)) != 0)
        return false;

#endif
    return true;
}

bool SocketUtil::setReuseAddr(SOCKET sock, bool reuseAddr)
{
    int val = 0;
    if (reuseAddr)
        val = 1;

    if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val)) != 0)
        return false;

    return true;
}

bool SocketUtil::setLinger(SOCKET sock, bool enableLinger, uint16_t timeInSec)
{
    struct linger _linger;
    _linger.l_linger = timeInSec;
    _linger.l_onoff = 0;
    if (enableLinger)
        _linger.l_onoff = 1;

    if (::setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&_linger, sizeof(_linger)) != 0)
        return false;

    return true;
}

void SocketUtil::initialize(const BootConfig& config)
{
    mKeepAliveInterval = (uint32_t)config.getSocketKeepAlive().getKeepAliveInterval().getSec();
    mKeepAliveCount = config.getSocketKeepAlive().getKeepAliveCount();

    if (mKeepAliveInterval < 0)
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[SocketUtil].initialize: invalid value specified for keepAliveInterval (" << mKeepAliveInterval << "); using default (" << DEFAULT_KEEP_ALIVE_INTERVAL << ")");
        mKeepAliveInterval = DEFAULT_KEEP_ALIVE_INTERVAL;
    }

    BLAZE_INFO_LOG(Log::CONNECTION, "[SocketUtil].initialize: Initializing TCP keep-alive settings; keepAliveInterval=" << mKeepAliveInterval << " keepAliveCount=" << mKeepAliveCount);
}

bool SocketUtil::configureInboundWindowsFirewallRule()
{
#if defined(EA_PLATFORM_WINDOWS)
    bool result = false;

    wchar_t currentWorkingDir[EA::IO::kMaxPathLength] = {0};
    EA::IO::Directory::GetCurrentWorkingDirectory(currentWorkingDir, sizeof(currentWorkingDir));

    wchar_t currentProcessPath[EA::IO::kMaxPathLength] = {0};
    EA::StdC::GetCurrentProcessPath(currentProcessPath, sizeof(currentProcessPath), 0);

    wchar_t currentRedisServerPath[EA::IO::kMaxPathLength] = {0};
    EA::IO::Path::PathString16 searchPath((const char16_t*)currentWorkingDir);
    for (; !searchPath.empty(); EA::IO::Path::TruncateComponent(searchPath, -1))
    {
        EA::IO::Path::PathString16 redisPath(searchPath);
        EA::IO::Path::Join(redisPath, EA_CHAR16("bin\\redis\\win64\\redis-server.exe"));
        if (EA::IO::File::Exists(redisPath.c_str()))
        {
            wcscpy(currentRedisServerPath, (const wchar_t*)redisPath.c_str());
            break;
        }
    }

    eastl::wstring ruleBlazeName(eastl::wstring::CtorSprintf(), L"Blaze Server (%s)", currentProcessPath);
    eastl::wstring ruleBlazeApplication(currentProcessPath);
    eastl::wstring ruleRedisName(eastl::wstring::CtorSprintf(), L"Redis Server (%s)", currentRedisServerPath);
    eastl::wstring ruleRedisApplication(currentRedisServerPath);
    eastl::wstring ruleRemoteAddresses(L"LocalSubnet,10.10.0.0/255.255.0.0");

    // Initialize COM.
    HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);

    // Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
    // initialized with a different mode. Since we don't care what the mode is,
    // we'll just use the existing mode.
    if (SUCCEEDED(hr) || (hr == RPC_E_CHANGED_MODE))
    {
        // Retrieve INetFwPolicy2
        INetFwPolicy2* pNetFwPolicy2 = nullptr;
        hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&pNetFwPolicy2);
        if (SUCCEEDED(hr))
        {
            VARIANT_BOOL firewallEnabled = TRUE;
            hr = pNetFwPolicy2->get_FirewallEnabled(NET_FW_PROFILE2_DOMAIN, &firewallEnabled);
            if (SUCCEEDED(hr) && (firewallEnabled == VARIANT_TRUE))
            {
                // Retrieve INetFwRules
                INetFwRules* pFwRules = nullptr;
                hr = pNetFwPolicy2->get_Rules(&pFwRules);
                if (SUCCEEDED(hr))
                {
                    enum RuleAction
                    {
                        RULE_EXISTS,
                        RULE_CREATE,
                        RULE_UPDATE
                    };
                    RuleAction blazeRuleAction = RULE_CREATE;
                    RuleAction redisRuleAction = RULE_CREATE;

                    // Iterate through all of the rules in pFwRules
                    IEnumVARIANT* pVariant = nullptr;
                    IUnknown* pEnumerator;
                    pFwRules->get__NewEnum(&pEnumerator);
                    if (pEnumerator != nullptr)
                        hr = pEnumerator->QueryInterface(__uuidof(IEnumVARIANT), (void**)&pVariant);

                    CComVariant var;
                    INetFwRule* existingFwRule = nullptr;
                    while (SUCCEEDED(hr) && (hr != S_FALSE)
                        && ((blazeRuleAction != RULE_EXISTS) || (redisRuleAction != RULE_EXISTS)))
                    {
                        var.Clear();

                        ULONG cFetched = 0;
                        hr = pVariant->Next(1, &var, &cFetched);
                        if (hr != S_FALSE)
                        {
                            if (SUCCEEDED(hr))
                                hr = var.ChangeType(VT_DISPATCH);
                            if (SUCCEEDED(hr))
                                hr = (V_DISPATCH(&var))->QueryInterface(__uuidof(INetFwRule), (void**)(&existingFwRule));

                            if (SUCCEEDED(hr))
                            {
                                BSTR existingRuleApplication;
                                BSTR existingRuleRemoteAddresses;
                                if (SUCCEEDED(existingFwRule->get_ApplicationName(&existingRuleApplication)) && (existingRuleApplication != nullptr) &&
                                    SUCCEEDED(existingFwRule->get_RemoteAddresses(&existingRuleRemoteAddresses)) && (existingRuleRemoteAddresses != nullptr))
                                {
                                    NET_FW_RULE_DIRECTION existingRuleDirection;
                                    if (SUCCEEDED(existingFwRule->get_Direction(&existingRuleDirection)) &&
                                        (existingRuleDirection == NET_FW_RULE_DIR_IN))
                                    {
                                        if (ruleBlazeApplication.comparei(existingRuleApplication) == 0)
                                        {
                                            if (ruleRemoteAddresses.comparei(existingRuleRemoteAddresses) == 0)
                                                blazeRuleAction = RULE_EXISTS;
                                            else
                                                blazeRuleAction = RULE_UPDATE;
                                        }

                                        if (ruleRedisApplication.comparei(existingRuleApplication) == 0)
                                            redisRuleAction = RULE_EXISTS;
                                    }
                                }
                            }
                        }
                    }

                    result = (blazeRuleAction == RULE_EXISTS) && (redisRuleAction == RULE_EXISTS);
                    if (!result) // If a blaze or redis rule needs to be created or updated
                    {
                        bool isAdminUser = false;

                        SID_IDENTIFIER_AUTHORITY sidNtAuthority = SECURITY_NT_AUTHORITY;
                        SID* sid;
                        if (AllocateAndInitializeSid(&sidNtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, (PSID*)&sid))
                        {
                            BOOL isMember;
                            if (CheckTokenMembership(nullptr, sid, &isMember))
                                isAdminUser = (isMember != 0);
                            FreeSid(sid);
                        }

                        if (!isAdminUser)
                        {
                            // The 'runas' verb causes the shell to run NetSH with administrative privileges - required to make changes to the Windows Firewall.
                            int64_t execResult = (int64_t)ShellExecuteW(nullptr, L"runas", currentProcessPath, L"--tool addfwrules", nullptr, SW_HIDE);
                            if (execResult > 32)
                                result = true;
                        }
                        else
                        {
                            if (blazeRuleAction == RULE_CREATE)
                            {
                                eastl::string netshCommandArgs;
                                netshCommandArgs.sprintf(
                                    "advfirewall firewall add rule description=\"Allow network access for the Blaze server\" dir=in remoteip=\"%S\" action=allow protocol=tcp localport=10000-65535 profile=domain enable=yes "
                                    "name=\"%S\" "
                                    "program=\"%S\"",
                                    ruleRemoteAddresses.c_str(),
                                    ruleBlazeName.c_str(),
                                    ruleBlazeApplication.c_str());

                                // The 'runas' verb causes the shell to run NetSH with administrative privileges - required to make changes to the Windows Firewall.
                                int64_t execResult = (int64_t)ShellExecute(nullptr, "runas", "netsh", netshCommandArgs.c_str(), nullptr, SW_HIDE);
                                if (execResult > 32)
                                    blazeRuleAction = RULE_EXISTS;
                            }
                            else if (blazeRuleAction == RULE_UPDATE)
                            {
                                eastl::string netshCommandArgs;
                                netshCommandArgs.sprintf(
                                    "advfirewall firewall set rule name=\"%S\" new description=\"Allow network access for the Blaze server\" dir=in "
                                    "remoteip=\"%S\" action=allow protocol=tcp localport=10000-65535 profile=domain enable=yes "
                                    "program=\"%S\"",
                                    ruleBlazeName.c_str(),
                                    ruleRemoteAddresses.c_str(),
                                    ruleBlazeApplication.c_str());

                                // The 'runas' verb causes the shell to run NetSH with administrative privileges - required to make changes to the Windows Firewall.
                                int64_t execResult = (int64_t)ShellExecute(nullptr, "runas", "netsh", netshCommandArgs.c_str(), nullptr, SW_HIDE);
                                if (execResult > 32)
                                    blazeRuleAction = RULE_EXISTS;
                            }

                            if (redisRuleAction == RULE_CREATE)
                            {
                                eastl::string netshCommandArgs;
                                netshCommandArgs.sprintf(
                                    "advfirewall firewall add rule description=\"Allow network access for the Redis server (required by Blaze)\" dir=in remoteip=localsubnet action=allow protocol=tcp localport=10000-65535 profile=domain enable=yes "
                                    "name=\"%S\" "
                                    "program=\"%S\"",
                                    ruleRedisName.c_str(),
                                    ruleRedisApplication.c_str());

                                // The 'runas' verb causes the shell to run NetSH with administrative privileges - required to make changes to the Windows Firewall.
                                int64_t execResult = (int64_t)ShellExecute(nullptr, "runas", "netsh", netshCommandArgs.c_str(), nullptr, SW_HIDE);
                                if (execResult > 32)
                                    redisRuleAction = RULE_EXISTS;
                            }
                        }
                    }

                    pFwRules->Release();
                }
            }
            pNetFwPolicy2->Release();
        }
        CoUninitialize();
    }

    return result;
#else
    // On all other platforms, just tell the caller things succeeded.
    return true;
#endif
}

} // Blaze

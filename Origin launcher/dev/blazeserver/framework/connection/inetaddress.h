/*************************************************************************************************/ /*!
    \file inetaddress.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_INETADDRESS_H
#define BLAZE_INETADDRESS_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

struct sockaddr_in;

namespace Blaze
{

class IpAddress;

class InetAddress
{
public:
    // Specify byte ordering
    typedef enum { NET, HOST } Order;

    // Constructors/Destructors
    // ---
    InetAddress();
    // Constructor that takes a dotted IP string and optional colon-separated port
    // number (e.g., "10.20.30.40:123")
    InetAddress(const char8_t* hostnameAndPort);
    InetAddress(const char8_t* hostname, uint16_t port);
    InetAddress(uint32_t address, uint16_t port, Order order);
    InetAddress(const sockaddr_in& addr);
    InetAddress(const InetAddress& rhs);
    InetAddress(const IpAddress& ipAddr);

    uint32_t getIp(Order order = NET) const;
    uint16_t getPort(Order order = NET) const;
    const char8_t* getHostname() const { return mHostname; }

    // Address/ports returned in Network Order.
    sockaddr_in& getSockAddr(sockaddr_in& addr) const;

    void setHostname(const char8_t* hostname);
    void setHostnameAndPort(const char8_t* hostnameAndPort);
    void setIp(uint32_t addr, Order order);
    void setPort(uint16_t port, Order order);
    void clearResolved() { mResolved = false; }
    void reset() { mHostname[0] = '\0'; mAddr = 0; mPort = 0; mResolved = false; }

    enum AddressFormat
    {
        HOST_IP_PORT, //HostName(IP):port
        IP_PORT, //IP:port
        HOST_PORT //HostName:port
    };
    const char8_t* const toString(char8_t* buf, uint32_t len, AddressFormat = HOST_IP_PORT) const;
    const char8_t* getIpAsString() const;

    // Convert a dotted decimal string (eg. 10.10.20.30) to a network-byte ordered int
    static bool parseDottedDecimal(const char8_t* str, uint32_t& ip, Order order = NET);

    // Overloaded operators
    // ---
    InetAddress& operator=(const InetAddress& rhs);

    // Less than operator for use with STL set<>
    bool operator<(const InetAddress& address) const;

    // Equals operator (for testing)
    bool operator==(const InetAddress& address) const;

    // Equals operator (for testing)
    bool operator!=(const InetAddress& address) const { return !(operator==(address)); }

    bool isResolved() const { return mResolved; }
    bool hostnameIsIp() const;
    bool isValid() const { return (mAddr != 0) || (mHostname[0] != '\0'); }

    static const uint32_t MAX_HOSTNAME_LEN = 256;
    static const uint32_t MAX_IPADDR_LEN = 46; // Supports IPV6 including nullptr terminator
protected:

private:
    bool isEmpty() const;

    char8_t mHostname[MAX_HOSTNAME_LEN];
    mutable char8_t mIpAddrStr[MAX_IPADDR_LEN]; // Only constructed when getIpAsString is called.
    uint32_t mAddr;
    uint16_t mPort;
    bool mResolved;
};

} // namespace Blaze

#endif // BLAZE_INETADDRESS_H


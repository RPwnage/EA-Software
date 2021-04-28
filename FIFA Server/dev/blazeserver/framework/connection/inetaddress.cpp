/*************************************************************************************************/
/*!
    \file inetaddress.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class InetAddress

    InetAddress is a concrete subclass of Address that represents internet dotted notation
    addresses and ports. (e.g. 192.168.0.1:2700)

    The internal representation of the address (32-bit unsigned integer) and port (16-bit unsigned
    integer) will be stored in HOST order and not in NETWORK order.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "EAStdC/EAString.h"
#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/util/shared/blazestring.h"
#include "framework/connection/inetaddress.h"
#include "framework/connection/socketutil.h"
#include "framework/tdf/networkaddress.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief InetAddress

    String constructor.

    \param[in]  hostnameAndPort - A string representation of a dotted IP string and
                                  colon-separated port number (e.g., "10.20.30.40:123")

    \notes
        If the address is an invalid format, it will log it but will still return an object without
        casting an exception.
*/
/*************************************************************************************************/
InetAddress::InetAddress(const char8_t* hostnameAndPort)
{
    mResolved = false;
    setHostnameAndPort(hostnameAndPort);
}

InetAddress::InetAddress(const char8_t* hostname, uint16_t port)
{
    mResolved = false;
    setHostname(hostname);
    mPort = htons(port);
}

/*************************************************************************************************/
/*!
    \brief InetAddress

    Constructor for InetAddress that takes in a 32-bit address and a 16-bit port.

    \param[in]  address - 32-bit value representing address in host order.
    \param[in]  port - Port number

    \notes
        e.g. Creating an InetAddress for "192.168.0.1:1000" would be:
        InetAddress myAddress(3232235521, 1000);
        3232235521 = 192 * (2^24) + 168 * (2^16) + 1
*/
/*************************************************************************************************/
InetAddress::InetAddress(uint32_t address, uint16_t port, Order order)
{
    mResolved = true;
    if (order == NET)
    {
        mAddr = address;
        mPort = port;
        blaze_snzprintf(mHostname, sizeof(mHostname), "%d.%d.%d.%d", NIPQUAD(address));
    }
    else
    {
        mAddr = htonl(address);
        mPort = htons(port);
        blaze_snzprintf(mHostname, sizeof(mHostname), "%d.%d.%d.%d", HIPQUAD(address));
    }
}

InetAddress::InetAddress(const sockaddr_in& addr)
{   
    mResolved = true;
    mAddr = addr.sin_addr.s_addr;
    mPort = addr.sin_port;
    blaze_snzprintf(mHostname, sizeof(mHostname), "%d.%d.%d.%d", NIPQUAD(mAddr));
}


InetAddress::InetAddress()
{
    reset();
}

InetAddress::InetAddress(const InetAddress& rhs)
{
    strcpy(mHostname, rhs.mHostname);
    mAddr = rhs.mAddr;
    mPort = rhs.mPort;
    mResolved = rhs.mResolved;
}

InetAddress::InetAddress(const IpAddress& ipAddr)
{
    mResolved = true;
    mAddr = ipAddr.getIp();
    mPort = ipAddr.getPort();
    blaze_snzprintf(mHostname, sizeof(mHostname), "%d.%d.%d.%d", NIPQUAD(mAddr));
}

InetAddress& InetAddress::operator=(const InetAddress& rhs)
{
    if (this != &rhs)
    {
        strcpy(mHostname, rhs.mHostname);
        mAddr = rhs.mAddr;
        mPort = rhs.mPort;
        mResolved = rhs.mResolved;
    }
    return *this;
}


/*************************************************************************************************/
/*!
    \brief toString

    Gets the string representation for this address (and optional) port.

    \return - String representation of the address and port.
*/
/*************************************************************************************************/
const char8_t* const InetAddress::toString(char8_t* buf, uint32_t len, InetAddress::AddressFormat format) const
{
    if (format == HOST_PORT)
    {
        blaze_snzprintf(buf, len, "%s:%d", mHostname, ntohs(mPort));
    }
    else
    {
        char8_t dottedDecimal[32];
        blaze_snzprintf(dottedDecimal, sizeof(dottedDecimal), "%d.%d.%d.%d", NIPQUAD(mAddr));
        if (format == IP_PORT|| strcmp(dottedDecimal, mHostname) == 0)
            blaze_snzprintf(buf, len, "%s:%d", dottedDecimal, ntohs(mPort));
        else
            blaze_snzprintf(buf, len, "%s(%s):%d", mHostname, dottedDecimal, ntohs(mPort));
    }

    return buf;
} // toString

/*************************************************************************************************/
/*!
    \brief getIpAsString

    Gets the string representation for the address only.

    \return - String representation of the address.
*/
/*************************************************************************************************/
const char8_t* InetAddress::getIpAsString() const
{
    if (mAddr == 0)
        return "";

    blaze_snzprintf(mIpAddrStr, MAX_IPADDR_LEN, "%d.%d.%d.%d", NIPQUAD(mAddr));
    return mIpAddrStr;
} // toString

/*************************************************************************************************/
/*!
    \brief hostnameIsIp

    Checks if the stored hostname is actually a real hostname or an IP address.

    \return - Returns true is the hostname is real, false if it is an IP address.
*/
/*************************************************************************************************/
bool InetAddress::hostnameIsIp() const
{
    in_addr sin_addr;
    return (inet_pton(AF_INET, mHostname, &sin_addr) > 0);
}

/*************************************************************************************************/
/*!
    \brief setIp

    Sets the IP to the given IP address.
*/
/*************************************************************************************************/
void InetAddress::setIp(uint32_t addr, Order order)
{
    if (order == NET)
        mAddr = addr;
    else
        mAddr = htonl(addr);
    if (mHostname[0] == '\0')
        blaze_snzprintf(mHostname, sizeof(mHostname), "%d.%d.%d.%d", NIPQUAD(mAddr));
    mResolved = true;
}

void InetAddress::setHostname(const char8_t* hostname)
{
    blaze_strnzcpy(mHostname, hostname, sizeof(mHostname));
    char8_t* portStr = strchr(mHostname, ':');
    if (portStr != nullptr)
    {
        *portStr++ = '\0';
    }

    if (!mResolved)
    {
        if (hostnameIsIp())
        {
            mResolved = true;

            in_addr sin_addr;
            inet_pton(AF_INET, mHostname, &sin_addr);
            mAddr = sin_addr.s_addr;           
        }
        else
        {
            mAddr = 0;
        }
    }
}

void InetAddress::setHostnameAndPort(const char8_t* hostnameAndPort)
{
    blaze_strnzcpy(mHostname, hostnameAndPort, sizeof(mHostname));
    char8_t* portStr = strchr(mHostname, ':');
    if (portStr == nullptr)
    {
        mPort = 0;
    }
    else
    {
        *portStr++ = '\0';
        mPort = htons((uint16_t)EA::StdC::AtoU32(portStr));
    }

    if (!mResolved)
    {
        if (hostnameIsIp())
        {
            mResolved = true;
            in_addr sinAddr;
            inet_pton(AF_INET, mHostname, &sinAddr);
            mAddr = sinAddr.s_addr;
        }
        else
        {
            mAddr = 0;
        }
    }
}

/*************************************************************************************************/
/*!
    \brief setPort

    Sets the port to the given value
*/
/*************************************************************************************************/
void InetAddress::setPort(uint16_t port, Order order)
{
    if (order == NET)
        mPort = port;
    else
        mPort = htons(port);
}

/*************************************************************************************************/
/*!
    \brief operator<

    Helps for ordering in maps.

    \param[in]  address - Checking InetAddress.

    \return - True if less than.

*/
/*************************************************************************************************/
bool InetAddress::operator<(const InetAddress& address) const
{
    EA_ASSERT(mResolved && address.mResolved);
    uint64_t thisIpPort = static_cast<uint64_t>(ntohl(mAddr)) << 32 | ntohs(mPort);
    uint64_t rhsIpPort = static_cast<uint64_t>(ntohl(address.mAddr)) << 32 | ntohs(address.mPort);
    return thisIpPort < rhsIpPort;
}

/*************************************************************************************************/
/*!
    \brief operator==

    Equality operator.

    \param[in]  address - Checking InetAddress.

    \return - True if equal than.

*/
/*************************************************************************************************/
bool InetAddress::operator==(const InetAddress& address) const
{
    if (!mResolved && !address.isResolved())
    {
        return ((blaze_strcmp(mHostname, address.mHostname) == 0) && (mPort == address.mPort));
    }
    else if (mResolved && address.isResolved())
    {
        return ((mAddr == address.mAddr) && (mPort == address.mPort));
    }
    else
    {
        if (!isEmpty() && !address.isEmpty())
        {
            // Check for != 0 before the comparison, so that we don't accept 0==0
            if (((mHostname[0] != '\0' && blaze_strcmp(mHostname, address.mHostname) == 0)
                || (mAddr != 0 && mAddr == address.mAddr))
                && (mPort == address.mPort))
                return true;
        }
        BLAZE_ERR_LOG(Log::CONNECTION, "cannot compare resolved and unresolved addresses");
        return false;
    }
}

bool InetAddress::isEmpty() const
{
    return (mHostname[0] == '\0' && mAddr == 0 && mPort == 0);
}

uint32_t InetAddress::getIp(InetAddress::Order order) const
{
    if (order == NET)
        return mAddr;
    return ntohl(mAddr);
}

uint16_t InetAddress::getPort(InetAddress::Order order) const
{
    if (order == NET)
        return mPort;
    return ntohs(mPort);
};

sockaddr_in& InetAddress::getSockAddr(sockaddr_in& addr) const
{    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = mAddr;
    addr.sin_port = mPort;
    return addr;
}

// static
bool InetAddress::parseDottedDecimal(const char8_t* str, uint32_t& ip, InetAddress::Order order)
{
    in_addr sinAddr;

    int result = inet_pton(AF_INET, str, &sinAddr);
    if (result <= 0)
        return false;

    if (order == NET)
        ip = sinAddr.s_addr;
    else
        ip = ntohl(sinAddr.s_addr);

    return true;  
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // Blaze


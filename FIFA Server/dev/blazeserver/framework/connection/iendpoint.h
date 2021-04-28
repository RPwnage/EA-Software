 /*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_IENDPOINT_H
#define BLAZE_IENDPOINT_H

/*** Include files *******************************************************************************/

#include "framework/connection/inetaddress.h"

namespace Blaze
{

class IEndpoint
{
public:
    virtual const char8_t* getName() const = 0;
    virtual uint16_t getPort(InetAddress::Order order) const = 0;
    virtual BindType getBindType() const = 0;
    virtual bool hasInternalNicAccess(const InetAddress& addr) const = 0;
    virtual bool getInternalCommandsAllowed() const = 0;

    virtual const char8_t* getEnvoyResourceName() const = 0;
    virtual bool getEnvoyAccessEnabled() const = 0;

    // A client address is considered "internal" if it is able to connect directly to this server's internal network interface.
    // An internal-only bound endpoint should only ever have internal clients.
    bool isInternalPeerAddress(const InetAddress& addr)
    {
        if (getBindType() == BIND_INTERNAL)
            return true;

        return (addr.isValid() && hasInternalNicAccess(addr));
    }
};

} // Blaze

#endif // BLAZE_IENDPOINT_H

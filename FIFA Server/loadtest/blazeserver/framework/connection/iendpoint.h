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

namespace Blaze
{

class IEndpoint
{
public:
    virtual const char8_t* getName() const = 0;
    virtual uint16_t getPort(InetAddress::Order order) const = 0;
    virtual BindType getBindType() const = 0;
    virtual bool getInternalCommandsAllowed() const = 0;
    virtual bool getEnvoyAccessEnabled() const = 0;
};

} // Blaze

#endif // BLAZE_IENDPOINT_H

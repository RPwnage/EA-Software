/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/endpointhelper.h"

namespace Blaze
{

InetAddress getInetAddress(Blaze::BindType bindType, uint16_t port)
{
    switch (bindType)
    {
    case Blaze::BIND_ALL:
    {
        InetAddress addr(INADDR_ANY, port, InetAddress::HOST);
        return addr;
    }

    case Blaze::BIND_INTERNAL:
    {
        InetAddress addr(NameResolver::getInternalAddress());
        addr.setPort(port, InetAddress::HOST);
        return addr;
    }

    case Blaze::BIND_EXTERNAL:
    {
        InetAddress addr(NameResolver::getExternalAddress());
        addr.setPort(port, InetAddress::HOST);
        return addr;
    }

    default:
        BLAZE_ERR_LOG(Log::CONNECTION, "[EndpointHelper].getInetAddress: Unhandled bind type");
        return InetAddress();
    }
}

} // namespace Blaze


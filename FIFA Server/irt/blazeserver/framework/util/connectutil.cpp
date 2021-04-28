/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/connectutil.h"

namespace Blaze
{

    // if the client doesn't know his external IP, replace it with his blazeServer connection IP
    //    we leave the external port alone, since the blazeServer doesn't know what the title's listen port is
    void ConnectUtil::setExternalAddr(NetworkAddress &destAddr, const InetAddress &sourceAddr, bool force)
    {
        if (destAddr.getActiveMemberIndex() == NetworkAddress::MEMBER_IPPAIRADDRESS)
        {
            IpAddress& externalAddress = destAddr.getIpPairAddress()->getExternalAddress();
            if (externalAddress.getIp() == 0 || force)
            {
                externalAddress.setIp(sourceAddr.getIp(InetAddress::HOST));
            }
            if (externalAddress.getPort() == 0 || force)
            {
                externalAddress.setPort(destAddr.getIpPairAddress()->getInternalAddress().getPort());
            }
        }
        else if (destAddr.getActiveMemberIndex() == NetworkAddress::MEMBER_IPADDRESS)
        {
            IpAddress* address = destAddr.getIpAddress();
            if (address->getIp() == 0 || force)
            {
                address->setIp(sourceAddr.getIp(InetAddress::HOST));
            }
        }
    }


} // Blaze


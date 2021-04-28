/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVERCHANNELFACTORY_H
#define BLAZE_SERVERCHANNELFACTORY_H

/*** Include files *******************************************************************************/
#include "framework/util/shared/blazestring.h"
#include "framework/connection/serversocketchannel.h"
#include "framework/connection/sslserversocketchannel.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class Endpoint;

class ServerChannelFactory
{
    NON_COPYABLE(ServerChannelFactory);
    ServerChannelFactory() {};
    ~ServerChannelFactory() {};

public:

    static ServerSocketChannel* create(Endpoint &endpoint, InetAddress& localAddress);
};

} // Blaze

#endif // BLAZE_SERVERCHANNELFACTORY_H

/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef _BLAZE_SSLSERVERSOCKETCHANNEL_H
#define _BLAZE_SSLSERVERSOCKETCHANNEL_H

/*** Include files *******************************************************************************/

#include "framework/connection/serversocketchannel.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class InetAddress;
class Channel;
class SslSocketChannel;

// Socket channel per non-grpc, ssl inbound endpoint on this blaze server instance.
class SslServerSocketChannel : public ServerSocketChannel
{
    NON_COPYABLE(SslServerSocketChannel);

public:
    SslServerSocketChannel(Endpoint &endpoint, InetAddress& localAddress);

    const char8_t* getCommonName() const override;
    
protected:
    void onRead(Channel& channel) override;

private:
    void finishAccept(SslSocketChannel& channel);

};

} // Blaze

#endif // _BLAZE_SSLSERVERSOCKETCHANNEL_H


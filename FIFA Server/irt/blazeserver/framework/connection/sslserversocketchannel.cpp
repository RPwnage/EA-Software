/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SslServerSocketChannel

    An implementation of ServerSocketChannel that uses OpenSSL for secure communications.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/endpoint.h"
#include "framework/connection/sslserversocketchannel.h"
#include "framework/connection/sslsocketchannel.h"
#include "framework/connection/sslcontext.h"
#include "framework/connection/selector.h"
#include "framework/system/fibermanager.h"
#include "framework/system/fiber.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

SslServerSocketChannel::SslServerSocketChannel(Endpoint& endpoint, InetAddress& localAddress)
    : ServerSocketChannel(endpoint, localAddress)
{
}

const char8_t* SslServerSocketChannel::getCommonName() const
{
    return mEndpoint.getSslContext()->getCommonName();
}

/*** Protected Methods ***************************************************************************/

void SslServerSocketChannel::onRead(Channel& channel)
{
    EA_ASSERT(&channel == this);

    //For efficiency, attempt to accept multiple sockets per read.
    for (uint32_t attempts = 0, max = mEndpoint.getMaxAcceptsPerIdle(); attempts < max; ++attempts)
    {   
        // Accept the new connection and register with the Selector
        struct sockaddr_in addr;
        SOCKET sok = accept(addr);// This is ServerSocketChannel base class level implementation accept and not the global socket accept call.

        if (sok != INVALID_SOCKET)
        {
            SslSocketChannel* newSocket = BLAZE_NEW SslSocketChannel(addr, sok, mEndpoint.getSslContext());
            if (newSocket == nullptr)
            {
                BLAZE_WARN_LOG(Log::CONNECTION, "[SslServerSocketChannel].accept: Unable to create a new "
                        "socket to handle the request.");
                return;
            }

            Fiber::CreateParams fiberParams;
            fiberParams.blocking = true;
            fiberParams.stackSize = Fiber::STACK_SMALL; 
            fiberParams.relativeTimeout = mEndpoint.getSslHandShakeTimeout();
            fiberParams.namedContext = "SslServerSocketChannel::finishAccept";
            gFiberManager->scheduleCall<SslServerSocketChannel, SslSocketChannel&>(this, &SslServerSocketChannel::finishAccept, *newSocket, fiberParams);
        }
        else
        {
            //Any error breaks the read for this time - it will raise again afterwards.  Most likely
            //this was an EWOULDBLOCK
            break;
        }
    }
}

/*** Private Methods *****************************************************************************/

void SslServerSocketChannel::finishAccept(SslSocketChannel& channel)
{
    if (channel.accept() != ERR_OK)
    {
        char8_t namebuf[256];
        BLAZE_WARN_LOG(Log::CONNECTION, "[SslServerSocketChannel].onAccepted: Unable to complete connection handshake, local port "
                       << channel.getAddress().getPort(InetAddress::HOST)
                       << " from peer " << channel.getPeerAddress().toString(namebuf, sizeof(namebuf)));

        delete &channel;

        return;
    }

    if (mHandler != nullptr)
    {
        RatesPerIp *rateLimitCounter = nullptr;
        rateLimitCounter = mEndpoint.getRateLimiter().getCurrentRatesForIp(channel.getPeerAddress().getIp());
        mHandler->onNewChannel(channel, mEndpoint, rateLimitCounter);
    }
}
} // Blaze


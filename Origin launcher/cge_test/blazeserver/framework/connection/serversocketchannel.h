/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVERSOCKETCHANNEL_H
#define BLAZE_SERVERSOCKETCHANNEL_H

/*** Include files *******************************************************************************/
#include "framework/connection/channel.h"
#include "framework/connection/socketchannel.h"
#include "framework/connection/inetaddress.h"
#include "framework/util/alertinfo.h"
#include "EATDF/time.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class Endpoint;
struct RatesPerIp;

// Socket channel per non-grpc, non-ssl inbound endpoint on this blaze server instance.
class ServerSocketChannel : public Channel, protected ChannelHandler
{
    NON_COPYABLE(ServerSocketChannel);
public:

    class Handler
    {
    public:
        virtual void onNewChannel(SocketChannel& channel, Endpoint& endpoint, RatesPerIp *rateLimitCounter) = 0;

        virtual ~Handler() { }
    };

    ServerSocketChannel(Endpoint &endpoint, InetAddress& localAddress);
    ~ServerSocketChannel() override;

    virtual bool initialize(Handler* handler);

    void start();
    void pause();
    void resume();

    ChannelHandle getHandle() override { return static_cast<ChannelHandle>(mSocket); }

protected:
    SOCKET mSocket;
    InetAddress& mLocalAddress;
    Handler* mHandler;
    Endpoint& mEndpoint;
    bool mIsPaused;

    SOCKET accept(sockaddr_in& addr);
    void resumeAfterMaxFdError();

    // ChannelHandler interface
    void onRead(Channel& channel) override;
    void onWrite(Channel& channel) override;
    void onError(Channel& channel) override;
    void onClose(Channel& channel) override;

private:

    AlertInfo mMaxConnectionsAlert;
    AlertInfo mTrustedAddressAlert;
};

} // Blaze

#endif // BLAZE_SERVERSOCKETCHANNEL_H

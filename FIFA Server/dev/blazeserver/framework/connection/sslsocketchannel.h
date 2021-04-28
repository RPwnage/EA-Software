/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SSLSOCKETCHANNEL_H
#define BLAZE_SSLSOCKETCHANNEL_H

/*** Include files *******************************************************************************/

#include "openssl/ssl.h"
#include "framework/connection/socketchannel.h"
#include "framework/connection/sslcontext.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class SslContext;

class SslSocketChannel : public SocketChannel
{
    NON_COPYABLE(SslSocketChannel);
public:
    SslSocketChannel(const InetAddress& peer, SslContextPtr sslContext);
    SslSocketChannel(sockaddr_in& sAddrInPeer, SOCKET socket, SslContextPtr sslContext);
    ~SslSocketChannel() override;

    BlazeRpcError DEFINE_ASYNC_RET(connect() override);
    void disconnect() override;
  
    BlazeRpcError DEFINE_ASYNC_RET(accept());

    bool isTrustedPeer() const override { return mIsTrustedPeer; }

protected:
    int32_t recv(uint8_t* buf, size_t len, int32_t flags) override;
    int32_t send(const uint8_t* buf, size_t len, int32_t flags) override;

private:
    SslContextPtr mSslContext;
    SSL* mSsl;

    enum { INIT, SSL_CONNECT_HANDSHAKE, SSL_ACCEPT_HANDSHAKE, CONNECTED } mSslState;

    bool mIsTrustedPeer;
  
    BlazeRpcError DEFINE_ASYNC_RET(finishConnection() override);

    void onRead(Channel& channel) override;
    void onWrite(Channel& channel) override;

    void onSwitchWith(SocketChannel& connectedChannel) override;

    bool pumpHandshake();
};

} // Blaze

#endif // BLAZE_SSLSOCKETCHANNEL_H


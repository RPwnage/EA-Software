/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/sslsocketchannel.h"
#include "framework/connection/sslcontext.h"
#include "framework/system/fiber.h"

#include "openssl/err.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

SslSocketChannel::SslSocketChannel(const InetAddress& peer, SslContextPtr sslContext)
    : SocketChannel(peer),
      mSslContext(sslContext),
      mSsl(nullptr),
      mSslState(INIT),
      mIsTrustedPeer(false)
{
}

SslSocketChannel::SslSocketChannel(sockaddr_in& sAddrInPeer, SOCKET inSocket, SslContextPtr sslContext)
    : SocketChannel(sAddrInPeer, inSocket),
      mSslContext(sslContext),
      mSsl(sslContext->createSsl(inSocket)),
      mSslState(INIT),
      mIsTrustedPeer(false)
{
}

SslSocketChannel::~SslSocketChannel()
{
    SslSocketChannel::disconnect();
}

BlazeRpcError SslSocketChannel::connect()
{
    return SocketChannel::connect();
}

void SslSocketChannel::disconnect()
{
    mSslState = INIT;
    if (mSsl != nullptr)
    {
        mSslContext->destroySsl(*mSsl);
        mSsl = nullptr;
    }
    SocketChannel::disconnect();
}

BlazeRpcError SslSocketChannel::accept()
{
    mSslState = SSL_ACCEPT_HANDSHAKE;

    if (!registerChannel(Channel::OP_READ) || !setHandler(this))
        return ERR_SYSTEM;

    bool result = pumpHandshake();
    while (result && mSslState == SSL_ACCEPT_HANDSHAKE)
    {
        BlazeRpcError sleepErr = Fiber::getAndWait(mConnectionEvent, "SslSocketChannel::accept");
        if (sleepErr != ERR_OK)
            return sleepErr;
        result = isConnected() && pumpHandshake();
    }

    setInterestOps(Channel::OP_READ);

    return (result ? ERR_OK : ERR_SYSTEM);
}

/*** Protected Methods ***************************************************************************/

int32_t SslSocketChannel::recv(uint8_t* buf, size_t len, int32_t flags)
{
    return SSL_read(mSsl, buf, (int32_t)len);
}

int32_t SslSocketChannel::send(const uint8_t* buf, size_t len, int32_t flags)
{
    return SSL_write(mSsl, buf, (int32_t)len);
}

/*** Private Methods *****************************************************************************/

BlazeRpcError SslSocketChannel::finishConnection()
{
    // Create an SSL object associated with the newly connected socket
    mSsl = mSslContext->createSsl(mSocket);
    mSslState = SSL_CONNECT_HANDSHAKE;

    bool result = pumpHandshake();
    while (result && mSslState == SSL_CONNECT_HANDSHAKE)
    {
        BlazeRpcError sleepErr = Fiber::getAndWait(mConnectionEvent, "SslSocketChannel::finishConnection");
        if (sleepErr != ERR_OK)
            return sleepErr;
        result = pumpHandshake();
    }

    setInterestOps(Channel::OP_READ);

    mConnected = result;
    return (result ? ERR_OK : ERR_SYSTEM);
}


void SslSocketChannel::onRead(Channel& channel)
{
    EA_ASSERT(&channel == this);

    switch (mSslState)
    {
        case INIT:
            SocketChannel::onRead(channel);
            break;

        case SSL_CONNECT_HANDSHAKE:
        case SSL_ACCEPT_HANDSHAKE:
            //We're trying to connect, wake up the connection fiber.
            Fiber::signal(mConnectionEvent, ERR_OK);
            break;

        case CONNECTED:
            setInterestOps(Channel::OP_READ);
            break;
    }
}

void SslSocketChannel::onWrite(Channel& channel)
{
    EA_ASSERT(&channel == this);

    switch (mSslState)
    {
        case INIT:
            SocketChannel::onWrite(channel);
            break;

        case SSL_CONNECT_HANDSHAKE:
        case SSL_ACCEPT_HANDSHAKE:
            //We're trying to connect, wake up the connection fiber.
            Fiber::signal(mConnectionEvent, ERR_OK);
            break;

        case CONNECTED:
            setInterestOps(Channel::OP_READ);
            break;
    }
}

void SslSocketChannel::onSwitchWith(SocketChannel& connectedChannel)
{
    SslSocketChannel& connectedSslChannel = static_cast<SslSocketChannel&>(connectedChannel);

    mSsl = connectedSslChannel.mSsl;
    mSslState = connectedSslChannel.mSslState;

    connectedSslChannel.mSsl = nullptr;
    connectedSslChannel.mSslState = INIT;

    SocketChannel::onSwitchWith(connectedChannel);
}

bool SslSocketChannel::pumpHandshake()
{
    if (mSsl == nullptr)
        return false;

    // Begin the connection handshake
    int32_t ret;
    if (mSslState == SSL_CONNECT_HANDSHAKE)
        ret = SSL_connect(mSsl);
    else
        ret = SSL_accept(mSsl);

    if (ret == 1)
    {
        if (mSslState == SSL_ACCEPT_HANDSHAKE)
            mIsTrustedPeer = mSslContext->verifyPeer(mSsl, getPeerAddress());
        mSslState = CONNECTED;
        setInterestOps(Channel::OP_READ);
        return true;
    }

    // Handshake not complete
    int32_t err = SSL_get_error(mSsl, ret);
    if (err == SSL_ERROR_WANT_READ)
    {
        setInterestOps(Channel::OP_READ);
        return true;
    }
    else if (err == SSL_ERROR_WANT_WRITE)
    {
        setInterestOps(Channel::OP_WRITE);
        return true;
    }

    SslContext::logErrorStack("SslSocketChannel", this,
            mSslState == SSL_CONNECT_HANDSHAKE ? "connect" : "accept");
    return false;
}

} // Blaze


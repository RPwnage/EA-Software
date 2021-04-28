/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SOCKETCHANNEL_H
#define BLAZE_SOCKETCHANNEL_H

/*** Include files *******************************************************************************/
#include "eathread/eathread_storage.h"
#include "framework/connection/platformsocket.h"
#include "framework/connection/channel.h"
#include "framework/connection/channelhandler.h"
#include "framework/connection/inetaddress.h"
#include "framework/connection/nameresolver.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{
class Protocol;
class Fiber;

class SocketChannel : public Channel, protected ChannelHandler
{
    NON_COPYABLE(SocketChannel)
public:
    SocketChannel(const InetAddress& peer);
    SocketChannel(const sockaddr_in& sAddrInPeer, SOCKET socket);
    SocketChannel(const InetAddress& sAddrInPeer, SOCKET socket, bool connected);
    ~SocketChannel() override;

    virtual BlazeRpcError DEFINE_ASYNC_RET(connect());
    virtual void disconnect() { disconnect(false); }

    void setCloseOnFork();

    virtual bool isTrustedPeer() const { return false; }
    const InetAddress& getAddress() const { return mAddress; }
    const InetAddress& getPeerAddress() const { return mPeerAddress; }
    const InetAddress& getRealPeerAddress() const;
    bool isRealPeerAddressResolved() const { return mRealPeerAddress.isResolved(); }
    void setRealPeerAddress(const InetAddress& realAddr) { mRealPeerAddress = realAddr; }
    void setResolveAlways(bool resolveAlways) { mResolveAlways = resolveAlways; }
    void setKeepAlive(bool keepAlive);
    void setReuseAddr(bool reuseAddr);
    void setLinger(bool enableLinger, uint16_t timeInSec);
    void setEnableKeepAlive(bool enableKeepAlive); // DEPRECATED - use setKeepAlive instead

    bool isConnecting() const;
    bool isConnected() const { return mConnected; }

    ChannelHandle getHandle() override { return static_cast<ChannelHandle>(mSocket); }

    int32_t read(RawBuffer& buffer, size_t length = 0);
    int32_t write(RawBuffer& buffer);

    int32_t getLastError() const { return mLastError; }
    void setLastErrorToPendingError(bool ignoreWouldBlock); 

    //  used by connections to trade sockets.
    bool switchWith(SocketChannel& connectedChannel);

    int32_t shutdown(int32_t how);

protected:
    friend class BlockingSocket;

    // Private Members
    SOCKET mSocket;
    InetAddress mAddress;
    InetAddress mPeerAddress;
    InetAddress mRealPeerAddress;

    bool mIsConnecting;
    bool mConnected;
    bool mResolveAlways;

    bool mReuseAddr;
    struct linger mLinger;

    int32_t mLastError;
    Fiber::EventHandle mConnectionEvent;
    NameResolver::LookupJobId mResolverJobId;

    void setLocalAddressFromSocket();

    virtual int32_t recv(uint8_t* buf, size_t len, int32_t flags);
    virtual int32_t send(const uint8_t* buf, size_t len, int32_t flags);

    virtual BlazeRpcError DEFINE_ASYNC_RET(finishConnection()) { mConnected = true; return ERR_OK; }

    // ChannelHandler interface
    void onRead(Channel& channel) override;
    void onWrite(Channel& channel) override;
    void onError(Channel& channel) override;
    void onClose(Channel& channel) override;

    virtual void onSwitchWith(SocketChannel& connectedChannel);

    SOCKET disconnect(bool detachSocket);
};

} // Blaze

#endif // BLAZE_SOCKETCHANNEL_H

/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SocketChannel

    Channel wrapper over a stream/TCP socket.  Provides the ability to register with a Selector,
    and to read/write ByteBuffers.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/inetaddress.h"
#include "framework/connection/socketchannel.h"
#include "framework/connection/socketutil.h"
#include "framework/connection/nameresolver.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/system/fiber.h"

#ifdef EA_PLATFORM_LINUX
#include <unistd.h>
#include <fcntl.h>
#endif

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define SOCKETCHANNEL_LOG_IDENT "[SocketChannel(id=" << getIdent() << "/lport=" << mAddress.getPort(InetAddress::HOST) << "/peer=" << SbFormats::InetAddress(mPeerAddress, InetAddress::IP_PORT) << ")]"

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief Constructor
*/
/*************************************************************************************************/
SocketChannel::SocketChannel(const InetAddress& peer)
:   mSocket(INVALID_SOCKET),
    mAddress(),
    mPeerAddress(peer),
    mRealPeerAddress(),
    mIsConnecting(false),
    mConnected(false),
    mResolveAlways(true),
    mReuseAddr(false),
    mLastError(0),
    mResolverJobId(NameResolver::INVALID_JOB_ID)
{
    mLinger.l_onoff = 1;
    mLinger.l_linger = 5;
}

SocketChannel::SocketChannel(const sockaddr_in& sAddrInPeer, SOCKET inSocket)
:   mSocket(inSocket),
    mAddress(),
    mPeerAddress(sAddrInPeer),
    mRealPeerAddress(),
    mIsConnecting(false),
    mConnected(true),
    mResolveAlways(true),
    mLastError(0),
    mResolverJobId(NameResolver::INVALID_JOB_ID)
{
    SocketUtil::setSocketBlockingMode(mSocket, false);
    SocketUtil::setKeepAlive(mSocket, true);

    setLocalAddressFromSocket();
}


SocketChannel::SocketChannel(const InetAddress& sAddrInPeer, SOCKET inSocket, bool connected)
:   mSocket(inSocket),
    mAddress(),
    mPeerAddress(sAddrInPeer),
    mRealPeerAddress(),
    mIsConnecting(false),
    mConnected(connected),
    mResolveAlways(true),
    mLastError(0),
    mResolverJobId(NameResolver::INVALID_JOB_ID)
{
    SocketUtil::setSocketBlockingMode(mSocket, false);
    SocketUtil::setKeepAlive(mSocket, true);

    if (connected)
    {
        setLocalAddressFromSocket();
    }
}

/*************************************************************************************************/
/*!
    \brief Destructor
*/
/*************************************************************************************************/
SocketChannel::~SocketChannel()
{
    SocketChannel::disconnect();
}

void SocketChannel::setEnableKeepAlive(bool enableKeepAlive)
{
    if (mSocket != INVALID_SOCKET && enableKeepAlive)
        SocketUtil::setKeepAlive(mSocket, true);
}

void SocketChannel::setKeepAlive(bool keepAlive)
{
    if (mSocket != INVALID_SOCKET)
        SocketUtil::setKeepAlive(mSocket, keepAlive);
}

void SocketChannel::setReuseAddr(bool reuseAddr)
{
    mReuseAddr = reuseAddr;
    if (mSocket != INVALID_SOCKET)
        SocketUtil::setReuseAddr(mSocket, reuseAddr);
}

void SocketChannel::setLinger(bool enableLinger, uint16_t timeInSec)
{
    mLinger.l_onoff = (enableLinger ? 1 : 0);
    mLinger.l_linger = timeInSec;
    if (mSocket != INVALID_SOCKET)
        SocketUtil::setLinger(mSocket, enableLinger, timeInSec);
}

bool SocketChannel::isConnecting() const
{
    return mIsConnecting || (mResolverJobId != NameResolver::INVALID_JOB_ID) || mConnectionEvent.isValid();
}

void SocketChannel::setLocalAddressFromSocket()
{
    // Grab our local address
    sockaddr_in addr;
    socklen_t addrLen = (socklen_t)sizeof(addr);
    if (getsockname(mSocket, (struct sockaddr*)&addr, &addrLen) == SOCKET_ERROR)
    {
        BLAZE_WARN_LOG(Log::CONNECTION, SOCKETCHANNEL_LOG_IDENT ".ctor: Failed to retrieve address from socket. err=" << WSAGetLastError());
        mAddress.setIp(0, InetAddress::NET);
        mAddress.setPort(0, InetAddress::NET);
    }
    else
    {
        mAddress.setIp(addr.sin_addr.s_addr, InetAddress::NET);
        mAddress.setPort(addr.sin_port, InetAddress::NET);
    }
}

//  used by connections to trade sockets in highly specialized cases.
//  "disconnects" the passed in channel and "connects" this channel using the passed-in channel's socket.
bool SocketChannel::switchWith(SocketChannel& connectedChannel)
{
    //  must switch a disconnected channel with a connected channel.
    if (connectedChannel.isConnected() && !mConnected)
    {
        //  copy over addresses from connected channel since this channel will now point to them.
        onSwitchWith(connectedChannel);

        //  detach socket from connected channel.
        SOCKET connectedSocket = connectedChannel.disconnect(true);
        
        // During the blocking disconnect call, connectedChannel might become invalid
        // Ensure we don't switch with an invalid socket
        if (connectedSocket != INVALID_SOCKET)
        {
            //  attach socket to this channel.   we're already connected so no need to resolve.
            //  caller is responsible for re-registering the channel, setting handlers.
            mSocket = connectedSocket;

            mConnected = true;
        }
    }
    
    return isConnected();
}

void SocketChannel::onSwitchWith(SocketChannel& connectedChannel)
{
    mAddress = connectedChannel.mAddress;
    mPeerAddress = connectedChannel.mPeerAddress;
    mRealPeerAddress = connectedChannel.mRealPeerAddress;
}

void SocketChannel::setCloseOnFork()
{
#ifdef EA_PLATFORM_LINUX
    SOCKET fd=getHandle();

    int oldflags = fcntl (fd, F_GETFD, 0);

    if (oldflags >= 0)
    {
        //set channel to close on fork
        oldflags |= FD_CLOEXEC;
        fcntl (fd, F_SETFD, oldflags);
    }
#endif
}



/*************************************************************************************************/
/*!
    \brief connect

    Begin the connection process to the channel's remote peer.
*/
/*************************************************************************************************/
BlazeRpcError SocketChannel::connect()
{
    char namebuf[256];

    if (mConnected)
    {
        // If we're already connected, nothing to do
        return ERR_OK;
    }

    mIsConnecting = true;

    BLAZE_TRACE_LOG(Log::CONNECTION, SOCKETCHANNEL_LOG_IDENT ".connect: connecting to " <<  mPeerAddress.toString(namebuf, sizeof(namebuf)));

    BlazeRpcError err = ERR_OK;
    if (mResolveAlways || !mPeerAddress.isResolved())
    {
        // Try to resolve the endpoint first
        err = gNameResolver->resolve(mPeerAddress, mResolverJobId);

        //By the time it returns we won't be canceling the job, so make the job id invalid
        mResolverJobId = NameResolver::INVALID_JOB_ID;

        //Check the result from the name resolve.
        if (err != ERR_OK)
        {
            mIsConnecting = false;
            return err;
        }
    }

    BLAZE_TRACE_LOG(Log::CONNECTION, SOCKETCHANNEL_LOG_IDENT ".connect: connecting to " << mPeerAddress.toString(namebuf, sizeof(namebuf)));

    // Create socket
    if (mSocket == INVALID_SOCKET)
    {
        mSocket = SocketUtil::createSocket(false);
        if (mSocket == INVALID_SOCKET)
        {
            mIsConnecting = false;
            return ERR_SYSTEM;
        }

        SocketUtil::setReuseAddr(mSocket, mReuseAddr);
        SocketUtil::setLinger(mSocket, (mLinger.l_onoff != 0), mLinger.l_linger);
    }

    if (!setHandler(this))
    {
        disconnect();
        mIsConnecting = false;
        return ERR_SYSTEM;
    }
    
    // Call the Socket API to actually connect
    sockaddr_in sAddr;
    mPeerAddress.getSockAddr(sAddr);
    if (::connect(mSocket, (sockaddr*)&sAddr, sizeof(sAddr)) == SOCKET_ERROR)
    {
        mLastError = WSAGetLastError();
        if (!SOCKET_WOULDBLOCK(mLastError) && !SOCKET_INPROGRESS(mLastError))
        {
            BLAZE_WARN_LOG(Log::CONNECTION, SOCKETCHANNEL_LOG_IDENT ".connect: connect() failed; code=" << mLastError);
            disconnect();
            mIsConnecting = false;
            return ERR_SYSTEM;
        }

        // Couldn't connect immediately so wait until we get a write notification on the socket to
        // indicate connection.
        registerChannel(Channel::OP_WRITE);

        //We'll wake up when something happens on the socket
        BlazeRpcError sleepErr = Fiber::getAndWait(mConnectionEvent, "SocketChannel::connect");

        //Check to see if we disconnected by looking at the socket
        if (sleepErr != ERR_OK || mSocket == INVALID_SOCKET)
        {
            disconnect();
            mIsConnecting = false;
            return (sleepErr != ERR_OK ? sleepErr : ERR_SYSTEM);
        }
    }

    // Grab our local address
    sockaddr_in addr;
    socklen_t addrLen = (socklen_t)sizeof(addr);
    if (getsockname(mSocket, (struct sockaddr*)&addr, &addrLen) == SOCKET_ERROR)
    {
        BLAZE_WARN_LOG(Log::CONNECTION, SOCKETCHANNEL_LOG_IDENT ".connect: Failed to retrieve address from socket. err=" << WSAGetLastError());
        mIsConnecting = false;
        return ERR_SYSTEM;
    }

    mAddress.setIp(addr.sin_addr.s_addr, InetAddress::NET);
    mAddress.setPort(addr.sin_port, InetAddress::NET);

    BlazeRpcError result = finishConnection();

    mIsConnecting = false;

    return result;
}


int32_t SocketChannel::shutdown(int32_t how)
{
    int32_t result = ::shutdown(mSocket, how);
    mLastError = WSAGetLastError();

    return result;
}
    
/*************************************************************************************************/
/*!
    \brief disconnect

    Disconnect from the current peer.
*/
/*************************************************************************************************/
SOCKET SocketChannel::disconnect(bool detachSocket)
{
    //Kill our resolve if its still going on
    if (mResolverJobId != NameResolver::INVALID_JOB_ID)
    {
        gNameResolver->cancelResolve(mResolverJobId);
        mResolverJobId = NameResolver::INVALID_JOB_ID;
    }

    SOCKET detachedSocket = INVALID_SOCKET;

    // Nothing to do if not connected - take into account that we could be disconnecting a channel 
    // whose socket has been detached via a switchWith call.
    if (mSocket != INVALID_SOCKET)
    {
        unregisterChannel(detachSocket);
        if (detachSocket)
        {
            detachedSocket = mSocket;
        }
        else
        {
            closesocket(mSocket);
        }

        mConnected = false;
        mSocket = INVALID_SOCKET;

        //Wake any connecting fiber
        Fiber::signal(mConnectionEvent, ERR_DISCONNECTED);
    }

    return detachedSocket;
}


/*************************************************************************************************/
/*!
    \brief read

    Read bytes from the Channel into the given ByteBuffer starting at the given offset.

    \param[in]  buffer - The buffer to read into
    \param[in]  length - The maximum length to read

    \return - The number of bytes read into the buffer or -1 on error
*/
/*************************************************************************************************/
int32_t SocketChannel::read(RawBuffer& buffer, size_t length)
{
    int32_t numToRead;
    size_t bufferRoom = buffer.tailroom();
    if ((length == 0) || (length > bufferRoom))
        numToRead = (int32_t)bufferRoom;
    else
        numToRead = (int32_t)length;
    if (numToRead == 0)
        return 0;

    int32_t numRead = recv(buffer.tail(), numToRead, 0);
    if (numRead == SOCKET_ERROR)
    {
        mLastError = WSAGetLastError();

        if (SOCKET_WOULDBLOCK(mLastError))
        {
            // Read would block so don't error out; just indicate that nothing could be read
            return 0;
        }

        // Fatal socket error
        BLAZE_WARN_LOG(Log::CONNECTION, SOCKETCHANNEL_LOG_IDENT ".read: recv failed; code=" << mLastError);
        disconnect();
        return -1;
    }
    else if (numRead == 0)
    {
        // Lost connection (either the other end has disconnected or gracefully closed)
        BLAZE_TRACE_LOG(Log::CONNECTION, SOCKETCHANNEL_LOG_IDENT ".read: connection closed.");
        disconnect();
        mLastError = 0;
        return -1;
    }

    // Update the buffer so its position is correct
    buffer.put(numRead);

    return numRead;
}

/*************************************************************************************************/
/*!
    \brief write

    Writes bytes from the given ByteBuffer to the Channel starting from the given offset.

    \param[in]  buffer - The buffer to read into
    \param[in]  length - The maximum length to write to the Channel

    \return - The number of bytes written to the Channel or -1 on error
*/
/*************************************************************************************************/
int32_t SocketChannel::write(RawBuffer& buffer)
{
    if (buffer.datasize() == 0)
    {
        BLAZE_WARN_LOG(Log::CONNECTION, SOCKETCHANNEL_LOG_IDENT ".write: no bytes to write.");
        return 0;
    }

    int32_t numWritten = send(buffer.data(), buffer.datasize(), 0);
    if (numWritten == SOCKET_ERROR)
    {
        mLastError = WSAGetLastError();
        if (SOCKET_WOULDBLOCK(mLastError))
        {
            // Write would block; don't error out, just indicate that nothing could be written
            return 0;
        }

        BLAZE_WARN_LOG(Log::CONNECTION, SOCKETCHANNEL_LOG_IDENT ".write: send failed; code=" << getLastError());
        disconnect();
        return -1;
    }
    else if (numWritten == 0)
    {
        // Lost connection
        BLAZE_TRACE_LOG(Log::CONNECTION, SOCKETCHANNEL_LOG_IDENT ".write: connection closed.");
        disconnect();
        mLastError = 0;
        return -1;
    }

    // Update the buffer so its position is correct
    buffer.pull(numWritten);

    return numWritten;
}

/*************************************************************************************************/
/*!
    \brief getRealPeerAddress

    Returns the real peer address for this channel. This can be, for example, the real Xbox 360
    address; in this case, the peer address will be the secure gateway's address. If the real peer
    address hasn't been set by calling setRealPeerAddress(), this method will default to 
    getPeerAddress(). This method should be called instead of getRealPeerAddress() whenever the
    actual physical location of the endpoint is preferred, such as in GeoIP.

    \return - The real peer address, if set; the peer address otherwise.
*/
/*************************************************************************************************/
const InetAddress& SocketChannel::getRealPeerAddress() const 
{ 
    return mRealPeerAddress.isResolved() ? mRealPeerAddress : mPeerAddress; 
}

/*** Protected Methods ***************************************************************************/

int32_t SocketChannel::recv(uint8_t* buf, size_t len, int32_t flags)
{
    return ::recv(mSocket, (char8_t*)buf, (int32_t)len, flags);
}

int32_t SocketChannel::send(const uint8_t* buf, size_t len, int32_t flags)
{
    // MSG_NOSIGNAL needed to disable SIGPIPE on Linux
    return ::send(mSocket, (const char8_t*)buf, (int32_t)len, flags | MSG_NOSIGNAL);
}

/* Sets the last error to the system error stored in the TCP stack.  For use when there's an error during select. */
void SocketChannel::setLastErrorToPendingError(bool ignoreWouldBlock)
{
    int32_t errCode;    
    socklen_t errSize = sizeof(errCode);
    if (::getsockopt(mSocket, SOL_SOCKET, SO_ERROR, (char*) &errCode, &errSize) == 0)
    {
        if (!(SOCKET_WOULDBLOCK(errCode)) || !ignoreWouldBlock)
        {
            mLastError = errCode;
        }
        else
        {
            mLastError = 0;
        }        
    }
    else
    {
        mLastError = WSAGetLastError();
    }
}

void SocketChannel::onRead(Channel& channel)
{
    EA_ASSERT(&channel == this);

    // Don't need to do anything since SocketChannel is only used as a handler when connecting
    // to a remote port.
}

void SocketChannel::onWrite(Channel& channel)
{
    EA_ASSERT(&channel == this);

    setInterestOps(Channel::OP_READ);

    Fiber::signal(mConnectionEvent, ERR_OK);
}

void SocketChannel::onError(Channel& channel)
{
    EA_ASSERT(&channel == this);
    setLastErrorToPendingError(true);
    disconnect();
}

void SocketChannel::onClose(Channel& channel)
{
    EA_ASSERT(&channel == this);
    setLastErrorToPendingError(true);
    disconnect();  
}

/*** Private Methods *****************************************************************************/

} // namespace Blaze


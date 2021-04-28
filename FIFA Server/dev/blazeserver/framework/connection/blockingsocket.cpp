/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class BlockingSocketManager

    This class defines a socket manager which provides blocking socket behavior to a fiber.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <stdio.h>
#include <errno.h>
#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/connection/blockingsocket.h"
#include "framework/connection/channel.h"
#include "framework/connection/socketchannel.h"
#include "framework/connection/selector.h"
#include "framework/connection/socketutil.h"
#include "framework/system/fiber.h"
#include "framework/system/fibermanager.h"
#include "blazerpcerrors.h"
#include "EAStdC/EAString.h"

#ifdef EA_PLATFORM_WINDOWS
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

extern "C" int32_t absl_connect(SOCKET s, const struct sockaddr* name, int32_t namelen)
{
    //Connect, resolve, etc
    Blaze::InetAddress addr(*(sockaddr_in*) name);
    return Blaze::gBlockingSocketManager->connect(s, addr);
}

extern "C" int32_t absl_recv(SOCKET s, char8_t* buffer, size_t bufSize, int32_t flags)
{
    return Blaze::gBlockingSocketManager->recv(s, buffer, bufSize, flags);
}

extern "C" int32_t absl_send(SOCKET s, const char8_t* buffer, size_t bufSize, int32_t flags)
{
    return Blaze::gBlockingSocketManager->send(s, buffer, bufSize, flags);
}

extern "C" int32_t absl_closesocket(SOCKET s)
{
    return Blaze::gBlockingSocketManager->close(s);
}

extern "C" int32_t absl_poll(struct pollfd *fds, uint32_t nfds, int32_t timeout)
{
    return Blaze::gBlockingSocketManager->poll(fds, nfds, timeout);
}

extern "C" int absl_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
    // We call the native getaddrinfo() function directly in 2 cases:
    // 1. If there is no name resolver we can't do anything, just passthrough to the underlying getaddrinfo.
    // 2. If the 'node' that was passed in is already an IP address, then no *resolving* needs to be done.  It is safe
    //    to call the getaddrinfo() function directly because we know it won't block the calling thread.
    //    IMPORTANT:  This distinction also allows calling code to manage the name resolution process first, without having
    //                to worry about the Fiber getting switched out without expecting it.
    if ((Blaze::gNameResolver == nullptr) || !Blaze::Fiber::isCurrentlyBlockable())
    {
        return getaddrinfo(node, service, hints, res);
    }

    if (hints != nullptr)
    {
        //Assert that we're looking for something the resolver can actually return.  Note that 0 is acceptable for 
        //socktype and protocol, and means "take any", or "no hint provided".
        EA_ASSERT((hints->ai_family == AF_INET) || (hints->ai_family == AF_UNSPEC) || (hints->ai_family == AI_NUMERICHOST));
        EA_ASSERT((hints->ai_socktype == 0) || (hints->ai_socktype == SOCK_STREAM) || (hints->ai_socktype == SOCK_DGRAM));
        EA_ASSERT((hints->ai_protocol == 0) || (hints->ai_protocol == IPPROTO_TCP) || (hints->ai_protocol == IPPROTO_UDP));
    }

    Blaze::NameResolver::LookupJobId resolveJobId = Blaze::NameResolver::INVALID_JOB_ID;
    Blaze::InetAddress addr(node, (uint16_t)EA::StdC::AtoU32(service));
    Blaze::BlazeRpcError err = Blaze::gNameResolver->resolve(addr, resolveJobId);
    resolveJobId = Blaze::NameResolver::INVALID_JOB_ID;

    if (err != Blaze::ERR_OK)
        return EAI_NONAME;

    if (res != nullptr)
    {
        *res = BLAZE_NEW addrinfo;
        memcpy((*res), hints, sizeof(**res));
        sockaddr_in* inaddr = BLAZE_NEW sockaddr_in;
        addr.getSockAddr(*inaddr);
        (*res)->ai_family = AF_INET;
        (*res)->ai_addr = (sockaddr*)inaddr;
        (*res)->ai_addrlen = sizeof(*inaddr);
        (*res)->ai_canonname = nullptr;
        (*res)->ai_next = nullptr;
    }
    return 0;
}

extern "C" void absl_freeaddrinfo(struct addrinfo *res)
{
    // If our allocator doesn't own the memory, then it must have been allocated directly by getaddrinfo(), so free it directly as well.
    if ((Blaze::gNameResolver == nullptr) || !Blaze::Fiber::isCurrentlyBlockable())
    {
        freeaddrinfo(res);
        return;
    }

    if (res != nullptr)
    {
        delete res->ai_addr;
        delete res;
    }
}


namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define BLOCKINGSOCKET_LOG_IDENT "[BlockingSocket(id=" << getIdent() << "/lport=" << mChannel.getAddress().getPort(InetAddress::HOST) << "/peer=" << SbFormats::InetAddress(mChannel.getPeerAddress(), InetAddress::IP_PORT) << ")]"

/*** Public Methods ******************************************************************************/



BlockingSocketManager::BlockingSocketManager() : mOpenSockets(BlazeStlAllocator("mOpenSockets"))
{

}

BlockingSocketManager::~BlockingSocketManager()
{
    //Go through our sockets and delete any outstanding ones
    for (SocketChannelMap::iterator itr = mOpenSockets.begin(), end = mOpenSockets.end(); itr != end; ++itr)
    {
        delete itr->second;
    }
}

int32_t BlockingSocketManager::connect(SOCKET s, const InetAddress& peerAddr)
{
    //Verify we don't already have it
    SocketChannelMap::const_iterator itr = mOpenSockets.find(s);
    if (itr != mOpenSockets.end())
    {
        WSASetLastError(EBADF);
        return -1; 
    }

    //No open socket, create our internal channel and connect it
    BlockingSocket *newSock = BLAZE_NEW BlockingSocket(peerAddr, s);
    int32_t result = newSock->connect();

    if (result == 0)
    {
        mOpenSockets[s] = newSock;
    }
    else
    {
        delete newSock;
    }

    return result; 
}

int32_t BlockingSocketManager::recv(SOCKET s, char8_t* buffer, size_t bufSize, int32_t flags)
{
    SocketChannelMap::const_iterator itr = mOpenSockets.find(s);
    if (itr != mOpenSockets.end())
    {
        return itr->second->recv(buffer, bufSize, flags);
    }

    WSASetLastError(EBADF);
    return -1;
}

int32_t BlockingSocketManager::send(SOCKET s, const char8_t* buffer, size_t bufSize, int32_t flags)
{
    SocketChannelMap::const_iterator itr = mOpenSockets.find(s);
    if (itr != mOpenSockets.end())
    {
        return itr->second->send(buffer, bufSize, flags);
    }

    WSASetLastError(EBADF);
    return -1;
}

int32_t BlockingSocketManager::close(SOCKET s)
{
    SocketChannelMap::iterator itr = mOpenSockets.find(s);
    if (itr != mOpenSockets.end())
    {
        itr->second->close();
        delete itr->second;
        mOpenSockets.erase(itr);

        return 0;
    }

    WSASetLastError(EBADF);
    return -1; 
}

int32_t BlockingSocketManager::poll(struct pollfd* fds, uint32_t nfds, int32_t timeout)
{
    // Do an initial poll with no timeout as a quick check in case there are already file
    // descriptors with the requested events on them.  This avoids the more expensive step of
    // registering with the selector.
#ifdef EA_PLATFORM_WINDOWS
    int rc = WSAPoll(fds, nfds, 0);
#else
    int rc = ::poll(fds, nfds, 0);
#endif
    if ((rc != 0) || (timeout == 0))
        return rc;

    BlazeRpcError err;
    if (nfds == 0)
    {
        // No file descriptors so this call is being used as a sleep
        err = gSelector->sleep(timeout * 1000);
        if (err == ERR_OK)
            return 0;
        WSASetLastError(EBADF);
        return -1;
    }

    // The poll() call timed out so now we need to do the more expensive operation to register
    // each file descriptor with the selector and wait for a wakeup.

    const Fiber::EventHandle eventHandle = Fiber::getNextEventHandle(); 
    for(uint32_t idx = 0; idx < nfds; ++idx)
    {
        SocketChannelMap::const_iterator itr = mOpenSockets.find(fds[idx].fd);
        if (itr == mOpenSockets.end())
        {
            WSASetLastError(EBADF);
            return -1;
        }        

        itr->second->setupPoll(fds[idx], eventHandle);
    }

    // Now wait for one or more of the file descriptors to be woken up;    
    BlazeRpcError pollWaitErr = Fiber::wait(eventHandle, "BlockingSocketManager::poll", TimeValue::getTimeOfDay() + (timeout * 1000));
    if (pollWaitErr != ERR_OK && pollWaitErr != ERR_TIMEOUT)
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[BlockingSocketManager].poll: waitOnEvent(" << eventHandle.eventId << ") returned " << ErrorHelp::getErrorName(pollWaitErr));
    }

    int32_t events = 0;
    if (pollWaitErr == ERR_OK)
    {
        for(uint32_t idx = 0; idx < nfds; ++idx)
        {
            SocketChannelMap::const_iterator itr = mOpenSockets.find(fds[idx].fd);
            if (itr == mOpenSockets.end())
            {
                WSASetLastError(EBADF);
                return -1;
            }

            fds[idx].revents = 0;
            events += itr->second->clearPoll(fds[idx]) ? 1 : 0;
        }
    }

    return events;
}

int32_t BlockingSocketManager::getLastError(SOCKET s)
{
    SocketChannelMap::iterator itr = mOpenSockets.find(s);
    return (itr != mOpenSockets.end()) ? itr->second->getLastError() : EBADF;
}

BlockingSocket::BlockingSocket(const InetAddress& peerAddr, SOCKET sockId)
    : mChannel(*new SocketChannel(peerAddr, sockId, false)),
      mHasSetBlockingMode(false),
      mPollOps(0),
      mShuttingDown(false)
{
}

BlockingSocket::BlockingSocket(SocketChannel& existingChannel)
    : mChannel(existingChannel),
      mHasSetBlockingMode(false),
      mPollOps(0),
      mShuttingDown(false)
{
    if (mChannel.isConnected())
    {
        if (!mChannel.setHandler(this))
        {
            close();
        }
    }
}


BlockingSocket::~BlockingSocket()
{
    close();
    SAFE_DELETE_REF(mChannel);
}

int32_t BlockingSocket::connect(bool enableKeepAlive/* = true*/)
{
    //Connect the channel.
    char8_t addrStr[64];
    BLAZE_TRACE_LOG(Log::CONNECTION, BLOCKINGSOCKET_LOG_IDENT ".connect: connecting to " << mChannel.getPeerAddress().toString(addrStr, sizeof(addrStr)));

    // Event though the blocking mode on the socket would have been set in the SocketChannel constructor,
    // we want to reset the blocking state here in case it was, for whatever reason, changed to
    // a blocking mode.  We also set mHasSetBlockingMode=false to make sure we reset the blocking mode
    // again in the first call to send() or recv().  This is all to handle the scenario where the absl_*
    // functions are used, and the blocking mode of a socket is changed outside of the BlockingSocket class.
    SocketUtil::setSocketBlockingMode(mChannel.getHandle(), false);
    mHasSetBlockingMode = false;

    if (mChannel.connect() != ERR_OK)
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, BLOCKINGSOCKET_LOG_IDENT ".connect: failed to connect to " << mChannel.getPeerAddress().toString(addrStr, sizeof(addrStr)));

        //set the error so we have it
        WSASetLastError(mChannel.getLastError());
        close();
        return -1;
    }

    if (!mChannel.setHandler(this))
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, BLOCKINGSOCKET_LOG_IDENT ".onConnected: failed to register handle.");
        close();
        return -1;
    }

    //This seems redundant, but is necessary for cases where we were handed the channel.
    if (!mChannel.registerChannel(mChannel.getInterestOps()))
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, BLOCKINGSOCKET_LOG_IDENT ".onConnected: failed to register channel!");
        close();
        return -1;
    }

    if (enableKeepAlive)
        mChannel.setKeepAlive(true);

    BLAZE_TRACE_LOG(Log::CONNECTION, BLOCKINGSOCKET_LOG_IDENT ".onConnected: connected to " << mChannel.getPeerAddress().toString(addrStr, sizeof(addrStr)));

    //Set the timeouts to now
    mLastReadTime = TimeValue::getTimeOfDay();
    mLastWriteTime = TimeValue::getTimeOfDay();

    return 0;
}

void BlockingSocket::close()
{
    if (mChannel.isConnected())
    {
        mChannel.disconnect();

        //Wake any events
        Fiber::signal(mRecvWaitEvent, ERR_DISCONNECTED);
        Fiber::signal(mSendWaitEvent, ERR_DISCONNECTED);
        
        if (mPollWaitEvent.isValid())
        {
            mPollOps |= Channel::OP_CLOSE;
            Fiber::signal(mPollWaitEvent, ERR_DISCONNECTED);
        }       
    }
}

int32_t BlockingSocket::shutdown(int32_t how)
{    
    mShuttingDown = true;
    int32_t result = mChannel.shutdown(how);
    return result;
}

int32_t BlockingSocket::recv(RawBuffer& buf, size_t len)
{
    if (EA_UNLIKELY(mRecvWaitEvent.isValid()))
    {
        EA_FAIL_MSG("BlockingSocket::recv - attempting to recv while already in recv.  This is most likely a blocking socket being incorrectly used by two fibers.");
        return -1;
    }

    bool neededToWait = false;
    int32_t result = 0;
    while (result == 0 && isConnected())
    {
        result = mChannel.read(buf, len);
        if (result == 0) //SocketChannel returns this on wouldblock
        {
            //Wait for the socket to be ready for reading
            if (waitForRead())
                neededToWait = true;
        }
        else if (result > 0)
        {
            mLastReadTime = TimeValue::getTimeOfDay();
        }

    }
    if (neededToWait)
    {
        // We needed to block waiting for the selector to tell us there was data so yield this
        // fiber to force the selector's onRead handler to unwind out of this object.  This will
        // ensure any attempts to delete this object after returning out of this function won't
        // have any other fiber blocked executing in one of its functions.  We only need to yield
        // if we had to block waiting for a read.
        Fiber::yield();
    }

    return result;
}

int32_t BlockingSocket::recv(char8_t* buffer, size_t bufSize, int32_t flags)
{
    if (EA_UNLIKELY(mRecvWaitEvent.isValid()))
    {
        EA_FAIL_MSG("BlockingSocket::recv - attempting to recv while already in recv.  This is most likely a blocking socket being incorrectly used by two fibers.");
        return -1;
    }

    if (!mHasSetBlockingMode)
    {
        SocketUtil::setSocketBlockingMode(mChannel.getHandle(), false);
        mHasSetBlockingMode = true;
    }

    //Try reading and see what we get
    bool neededToWait = false;
    int32_t result = -1;
    while (result < 0 && mChannel.isConnected())
    {
        result = ::recv(mChannel.getHandle(), buffer, bufSize, flags);
        if (result < 0)
        {
            int32_t sockErr = WSAGetLastError();
            if (SOCKET_WOULDBLOCK(sockErr))
            {
                //Wait for the socket to be ready for reading
                if (waitForRead())
                    neededToWait = true;
            }
            else
            {
                if (mChannel.getLastError() == 0)
                {
                    BLAZE_TRACE_LOG(Log::CONNECTION, BLOCKINGSOCKET_LOG_IDENT ".recv: connection closed, no error.");
                }
                else
                {
                    BLAZE_WARN_LOG(Log::CONNECTION, BLOCKINGSOCKET_LOG_IDENT ".recv: connection closed - error reading: " << sockErr);
                }
                close();
            }
        }
        else
        {
            mLastReadTime = TimeValue::getTimeOfDay();
        }
    }

    if (neededToWait)
    {
        // We needed to block waiting for the selector to tell us there was data so yield this
        // fiber to force the selector's onRead handler to unwind out of this object.  This will
        // ensure any attempts to delete this object after returning out of this function won't
        // have any other fiber blocked executing in one of its functions.  We only need to yield
        // if we had to block waiting for a read.
        Fiber::yield();
    }

    return result;
}

int32_t BlockingSocket::send(const RawBuffer& buf)
{
    size_t totalSent = 0;
    while (totalSent < buf.datasize())
    {
        int32_t result = send((const char8_t*) buf.data() + totalSent, buf.datasize() - totalSent, MSG_NOSIGNAL);
        if (result < 0)
            return -1;

        totalSent += result;
    }

    return totalSent;
}

int32_t BlockingSocket::send(const char8_t* buffer, size_t bufSize, int32_t flags)
{
    if (EA_UNLIKELY(mSendWaitEvent.isValid()))
    {
        EA_FAIL_MSG("BlockingSocket::send - attempting to send while already sending.  This is most likely a blocking socket being incorrectly used by two fibers.");
        return -1;
    }

    if (!mHasSetBlockingMode)
    {
        SocketUtil::setSocketBlockingMode(mChannel.getHandle(), false);
        mHasSetBlockingMode = true;
    }

    //Try writing and see what we get
    bool neededToWait = false;
    int32_t result = -1;
    while (result < 0 && mChannel.isConnected())
    {
        result = mChannel.send((uint8_t*)buffer, bufSize, flags);
        
        if (result < 0)
        {
            int32_t sockErr = WSAGetLastError();
            if (SOCKET_WOULDBLOCK(sockErr))
            {
                //Wait for the socket to be ready for writing
                if (mChannel.addInterestOp(Channel::OP_WRITE))
                {
                    BlazeRpcError err = Fiber::getAndWait(mSendWaitEvent, "BlockingSocket::send");
                    neededToWait = true;
                    if (err != ERR_OK)
                    {
                        close();
                        result = -1;
                    }

                    //There's a problem with tracking write time as a form of activity timeout
                    //As long as the buffer on the kernel has room to fill, we'll write no problem
                    //That means we're saying a connection is "active" even if we're just stuffing the buffer
                    //under that notion if we write a small amount of data to a socket we could keep the conn open for a 
                    //long long time.  For that reason we say that the activity timeout only comes into play for writes
                    //AFTER we've unblocked the socket.  This means that the other side is absolutely still 
                    //reading what we're writing, and is thus active.
                    mLastWriteTime = TimeValue::getTimeOfDay();
                    mChannel.removeInterestOp(Channel::OP_WRITE);
                }
                else
                {
                    close();
                    result = -1;
                }
            }
            else
            {
                if (sockErr == 0)
                {
                    BLAZE_TRACE_LOG(Log::CONNECTION, BLOCKINGSOCKET_LOG_IDENT ".send: connection closed, no error.");
                }
                else
                {
                    BLAZE_WARN_LOG(Log::CONNECTION, BLOCKINGSOCKET_LOG_IDENT ".send: connection closed - error writing: " << sockErr);
                }
                close();
            }
        }
    }

    if (neededToWait)
    {
        // We needed to block waiting for the selector to tell us there was data so yield this
        // fiber to force the selector's onWrite handler to unwind out of this object.  This will
        // ensure any attempts to delete this object after returning out of this function won't
        // have any other fiber blocked executing in one of its functions.  We only need to yield
        // if we had to block waiting for a write.
        Fiber::yield();
    }
 
    return result;
}

void BlockingSocket::setupPoll(struct pollfd& fds, const Fiber::EventHandle& eventHandle)
{
    mPollWaitEvent = eventHandle;
    mPollOps = 0;

    uint32_t interestOps = 0;
    if (fds.events & POLLIN)
        interestOps |= Channel::OP_READ;
    if (fds.events & POLLOUT)
        interestOps |= Channel::OP_WRITE;

    mChannel.setInterestOps(interestOps);
}

bool BlockingSocket::clearPoll(struct pollfd& fds)
{
    bool result =(mPollOps != 0);

    if (mPollOps & Channel::OP_READ)
        fds.revents |= POLLIN;
    if (mPollOps & Channel::OP_WRITE)
        fds.revents |= POLLOUT;
    if (mPollOps & Channel::OP_CLOSE)
        fds.revents |= POLLHUP;
    if (mPollOps & Channel::OP_ERROR)
        fds.revents |= POLLERR;

    if (mPollWaitEvent.isValid())
    {
        mPollWaitEvent.reset();
        mPollOps = 0;
        mChannel.setInterestOps(Channel::OP_NONE);      
    }    

    return result;
}


int32_t BlockingSocket::poll(struct pollfd* fds, int32_t timeout)
{
    if (fds == nullptr || fds->fd != (SOCKET) mChannel.getHandle())
        return 0;

    Fiber::EventHandle eventHandle = Fiber::getNextEventHandle();
    setupPoll(*fds, eventHandle);

    // Now wait for one or more of the file descriptors to be woken up;    
    BlazeRpcError pollWaitErr = Fiber::wait(eventHandle, "BlockingSocketManager::poll", TimeValue::getTimeOfDay() + (timeout * 1000));
    if (pollWaitErr != ERR_OK && pollWaitErr != ERR_TIMEOUT)
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[BlockingSocket].poll: waitOnEvent(" << eventHandle.eventId << ") returned " << ErrorHelp::getErrorName(pollWaitErr));
        return 0;
    }

    bool rc = clearPoll(*fds) ? 1 : 0;

    // Yield this fiber to force the selector's onRead/onWrite/etc handler to unwind out of this
    // object.  This will ensure any attempts to delete this object after returning out of this
    // function won't have any other fiber blocked executing in one of its functions.
    Fiber::yield();

    return rc;
}


bool BlockingSocket::waitForRead()
{
    bool waited = false;

    //note that "addInterestOp" is a no-op if OP_READ is already set on the channel. (should be almost all of the time).
    if (mChannel.addInterestOp(Channel::OP_READ))
    {
        BlazeRpcError err = Fiber::getAndWait(mRecvWaitEvent, "BlockingSocket::recv");
        if (err != ERR_OK)
        {
            close();
        }
        waited = true;
    }
    else
    {
        close();
    }
    return waited;
}

void BlockingSocket::onRead(Channel& channel)
{
    //Wake up our socket
    if (mRecvWaitEvent.isValid())
    {
        Fiber::signal(mRecvWaitEvent, ERR_OK);
    }
    else
    {
        //If we're not waiting for this event, disable us for read for now so this doesn't continually happen.
        //we'll pick up the read and re-register the next time recv is called.
        if (isConnected() && !mChannel.removeInterestOp(Channel::OP_READ))
        {
            close();
        }
    }

    if (mPollWaitEvent.isValid())
    {
        mPollOps |= Channel::OP_READ;
        Fiber::signal(mPollWaitEvent, ERR_OK);
    }
}

void BlockingSocket::onWrite(Channel& channel)
{
    //Wake up the fiber so it can write
    Fiber::signal(mSendWaitEvent, ERR_OK);

    if (mPollWaitEvent.isValid())
    {
        mPollOps |= Channel::OP_WRITE;
        Fiber::signal(mPollWaitEvent, ERR_OK);
    }
}

void BlockingSocket::onError(Channel& channel)
{
    //fetch the pending error on the socket
    mChannel.setLastErrorToPendingError(true);
    if (mChannel.getLastError() != 0)
    {        
        BLAZE_WARN_LOG(Log::CONNECTION, BLOCKINGSOCKET_LOG_IDENT ".onError: connection closed, error " 
                       << mChannel.getLastError() << ".");
    }
    else
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, BLOCKINGSOCKET_LOG_IDENT ".onError: connection closed normally.");
    }

    close();

    Fiber::signal(mRecvWaitEvent, ERR_SYSTEM);
    Fiber::signal(mSendWaitEvent, ERR_SYSTEM);
    
    if (mPollWaitEvent.isValid())
    {
       Fiber::signal(mPollWaitEvent, ERR_SYSTEM);
    }    
}

void BlockingSocket::onClose(Channel& channel)
{
    mChannel.setLastErrorToPendingError(true);
    if (mChannel.getLastError() != 0)
    {        
        BLAZE_WARN_LOG(Log::CONNECTION, "[BlockingSocket().onClose]: connection closed, error " 
            << mChannel.getLastError() << ".");
    }
    else
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, BLOCKINGSOCKET_LOG_IDENT ".onClose: connection closed normally.");
    }

    close();

    Fiber::signal(mRecvWaitEvent, ERR_DISCONNECTED);
    Fiber::signal(mSendWaitEvent, ERR_DISCONNECTED);

    if (mPollWaitEvent.isValid())
    {
        mPollOps |= Channel::OP_CLOSE;
        Fiber::signal(mPollWaitEvent, ERR_DISCONNECTED);
    }
}

/*** Private Methods *****************************************************************************/


} // Blaze


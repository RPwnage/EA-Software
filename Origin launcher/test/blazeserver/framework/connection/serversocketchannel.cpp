/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ServerSocketChannel

    Subclass of SocketChannel that only listens for new connections.  connect() and disconnect()
    have no effect and always return false.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/inetaddress.h"
#include "framework/connection/serverchannelfactory.h"
#include "framework/connection/serversocketchannel.h"
#include "framework/connection/selector.h"
#include "framework/connection/socketutil.h"
#include "framework/connection/endpoint.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

static const uint64_t MAX_FD_TIME_INTERVAL = 100000; //100ms

/*** Public Methods ******************************************************************************/

ServerSocketChannel* ServerChannelFactory::create(Endpoint &endpoint, InetAddress& localAddress)
{
    if (blaze_stricmp(endpoint.getChannelType(), "tcp") == 0)
        return BLAZE_NEW ServerSocketChannel(endpoint, localAddress);
    else if (blaze_stricmp(endpoint.getChannelType(), "ssl") == 0)
    {
        if (endpoint.getSslContext() == nullptr)
        {
            BLAZE_WARN_LOG(Log::CONNECTION, "[ServerChannelFactory].create: unable to create SSL server socket for endpoint '" << endpoint.getName() << "'; no SslContext available.");
            return nullptr;
        }
        return BLAZE_NEW SslServerSocketChannel(endpoint, localAddress);
    }
    else
        return nullptr;
}


/*************************************************************************************************/
/*!
    \brief Constructor
*/
/*************************************************************************************************/
ServerSocketChannel::ServerSocketChannel(Endpoint &endpoint, InetAddress& localAddress)
    : mSocket(INVALID_SOCKET),
      mLocalAddress(localAddress),
      mHandler(nullptr),
      mEndpoint(endpoint),
      mIsPaused(true),
      mMaxConnectionsAlert(10000),
      mTrustedAddressAlert(1000)      
{
}

/*************************************************************************************************/
/*!
    \brief Destructor
*/
/*************************************************************************************************/
ServerSocketChannel::~ServerSocketChannel()
{
    // Nothing to do if not connected
    if (mSocket != INVALID_SOCKET)
    {
        closesocket(mSocket);
        mSocket = INVALID_SOCKET;
    }
}

/*************************************************************************************************/
/*!
    \brief initialize

    Allocate resources and initialize them.

    \return - true if initialization was successful, false if not
*/
/*************************************************************************************************/
bool ServerSocketChannel::initialize(Handler* handler)
{
    uint32_t addr = mLocalAddress.getIp();
    char8_t addrBuf[256];
    blaze_snzprintf(addrBuf, sizeof(addrBuf), "%d.%d.%d.%d", NIPQUAD(addr));

    BLAZE_TRACE_LOG(Log::CONNECTION, "[ServerSocketChannel].initialize: " << addrBuf <<":" << mLocalAddress.getPort(InetAddress::HOST));

    // Create socket
    mSocket = SocketUtil::createSocket(false, SOCK_STREAM, 0);
    if (mSocket == INVALID_SOCKET)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[ServerSocketChannel].initialize: Failed to open socket.");
        return false;
    }

#ifdef EA_PLATFORM_WINDOWS
    // NOTE: Winsock's SO_REUSEADDR is unusable because it enables a second Blaze server to hijack
    // a port used by a running Blaze server. Winsock's SO_EXCLUSIVEADDRUSE is unusable because it
    // prevents a Blaze server from re-binding to its port(on XP) after an unclean shutdown
    // failed to finish the handshake with peer(leaving the port in FIN_WAIT_2 state).
    // Therefore, the *only* option is to use the DEFAULT behavior of Winsock, which is essentially 
    // the same as BSD's SO_REUSEADDR with the one caveat that other processes using SO_REUSEADDR 
    // could steal our port. We will have to take that chance, because we want the following behavior:
    // 1. A Blaze server must *not* bind to a port that is in use by another process (or itself)
    // 2. A Blaze server must re-bind to an uncleanly shut port (common in ARSON tests)
    // For details see: http://msdn.microsoft.com/en-us/library/ms740621%28VS.85%29.aspx
    // Also see: http://twistedmatrix.com/trac/ticket/1151
    // And: http://old.nabble.com/Port-allocation-problem-on-windows-%28incl.-patch%29-td28241079.html
#else
    // Allow socket reuse
    int on = 1;
    ::setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
#endif

    // Bind it to the local address
    sockaddr_in sAddr;
    mLocalAddress.getSockAddr(sAddr);
    if (::bind(mSocket, (sockaddr*)&sAddr, sizeof(sAddr)) == SOCKET_ERROR)
    {
        int32_t error = WSAGetLastError();
        char8_t errBuf[128];
        BLAZE_ERR_LOG(Log::CONNECTION, "[ServerSocketChannel].initialize: Failed to bind to local address. ("<< mLocalAddress.getIpAsString() << ":"<< mLocalAddress.getPort(InetAddress::HOST) <<") Error("
                      << error <<"): " << SocketUtil::getErrorMsg(errBuf, sizeof(errBuf), error));
        return false;
    }

    // Listen for new connections
    if (::listen(mSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[ServerSocketChannel].initialize: Unable to listen to socket.");
        return false;
    }

    mHandler = handler;

    return true;
}

/*************************************************************************************************/
/*!
    \brief start

    Start accepting new connections on this channel.
*/
/*************************************************************************************************/
void ServerSocketChannel::start()
{
    setHandler(this);
    registerChannel(Channel::OP_NONE);
}

/*************************************************************************************************/
/*!
    \brief pause

    Stop accepting connections.
*/
/*************************************************************************************************/
void ServerSocketChannel::pause()
{
    if (!mIsPaused)
    {
        mIsPaused = true;
        setInterestOps(Channel::OP_NONE);
    }
}

/*************************************************************************************************/
/*!
    \brief resume

    Resume accepting connections.
*/
/*************************************************************************************************/
void ServerSocketChannel::resume()
{
    if (mIsPaused)
    {
        mIsPaused = false;
        // use register channel instead of setInterestOps here
        // because for channels that have been registered
        // while mProcessingChannels = true, they would have been put
        // in the mDeferredRegistrationQueue with OP_NONE so if we 
        // setInterestOps now, it would later get overwritten with OP_NONE
        registerChannel(Channel::OP_READ);
    }
}

/*************************************************************************************************/
/*!
    \brief resume

    Resume accepting connections after a max fd error.  If we're paused, don't bother.
*/
/*************************************************************************************************/
void ServerSocketChannel::resumeAfterMaxFdError()
{
    if (!mIsPaused)
    {
        setInterestOps(Channel::OP_READ);
    }
}


/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

void ServerSocketChannel::onRead(Channel& channel)
{
    EA_ASSERT(&channel == this);

    //For efficiency, attempt to accept multiple sockets per read.
    for (uint32_t attempts = 0, max = mEndpoint.getMaxAcceptsPerIdle(); attempts < max; ++attempts)
    {   
        // Accept the new connection and register with the Selector
        struct sockaddr_in addr;
        SOCKET sok = accept(addr); // This is class level implementation accept and not the global socket accept call.
        if (sok != INVALID_SOCKET)
        {
            SocketChannel* newSocket = BLAZE_NEW SocketChannel(addr, sok);
            if (newSocket == nullptr)
            {
                BLAZE_WARN_LOG(Log::CONNECTION, "[ServerSocketChannel].accept: Unable to create a new socket to handle the request.");
                return;
            }

            RatesPerIp *rateLimitCounter = nullptr;
            rateLimitCounter = mEndpoint.getRateLimiter().getCurrentRatesForIp(newSocket->getPeerAddress().getIp());
            mHandler->onNewChannel(*newSocket, mEndpoint, rateLimitCounter);
        }
        else
        {
            //Any error breaks the read for this time - it will raise again afterwards.  Most likely
            //this was an EWOULDBLOCK
            break;
        }
    }
}

void ServerSocketChannel::onWrite(Channel& channel)
{
    EA_ASSERT(&channel == this);

    // Don't need to do anything since an accepting socket will only be read from.
    // This should never get called.
}

void ServerSocketChannel::onError(Channel& channel)
{
    EA_ASSERT(&channel == this);

    // TODO
}

void ServerSocketChannel::onClose(Channel& channel)
{
    EA_ASSERT(&channel == this);

    // TODO
}

/*************************************************************************************************/
/*!
    \brief accept

    Accept a new connection and return a new SocketChannel for it.

    \return - Newly initialized SocketChannel.  Call SocketChannel.getPeerAddress() to find out
              who connected.
*/
/*************************************************************************************************/
SOCKET ServerSocketChannel::accept(sockaddr_in& addr)
{
    socklen_t sockAddrSize = sizeof(addr);
    SOCKET newSocket = ::accept(mSocket, (sockaddr*) &addr, &sockAddrSize);
    char8_t addrBuf[256];
    blaze_snzprintf(addrBuf, sizeof(addrBuf), "%d.%d.%d.%d", NIPQUAD(addr.sin_addr.s_addr));

    if (newSocket == INVALID_SOCKET)
    {
        int32_t error = WSAGetLastError();
        if (SOCKET_NOFDS(error))
        {
            mMaxConnectionsAlert.increment();
            if (mMaxConnectionsAlert.shouldLog())
            {
                BLAZE_ERR_LOG(Log::CONNECTION, "[ServerSocketChannel].accept: Rejecting incoming connection on endpoint '" 
                              << mEndpoint.getName() << "': maximum file descriptor limit reached; " << mMaxConnectionsAlert.getCount() << " others also rejected.");
                mMaxConnectionsAlert.reset();
            }

            //Because this error does not clear the READ op on the server socket FD, we need to remove ourselves
            //from the poll or the server will go into a tight loop of poll->accept->poll->accept->...            
            setInterestOps(Channel::OP_NONE);

            //Schedule a timer to enable the read op at a later time, after some ops have potentially cleared.
            gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + MAX_FD_TIME_INTERVAL, 
                this, &ServerSocketChannel::resumeAfterMaxFdError,
                "ServerSocketChannel::resumeAfterMaxFdError");
        }
        else if (!SOCKET_WOULDBLOCK(error))
        {
            char8_t errBuf[128];
            BLAZE_WARN_LOG(Log::CONNECTION, "[ServerSocketChannel].accept: Incoming connection is invalid: " 
                           << SocketUtil::getErrorMsg(errBuf, sizeof(errBuf), error) << " (" << error << ")");
        }
        return INVALID_SOCKET;
    }

    // Ensure that we're not over the connection limit
    if (mEndpoint.hasMaxConnections())
    {
        mMaxConnectionsAlert.increment();
        if (mMaxConnectionsAlert.shouldLog())
        {
            BLAZE_WARN_LOG(Log::CONNECTION, "[ServerSocketChannel].accept: Rejecting incoming connection from " 
                           << addrBuf << " on endpoint '" << mEndpoint.getName() << "': maximum connection limit (" << mEndpoint.getMaxConnections()
                           << ") reached; " << mMaxConnectionsAlert.getCount() << " others also rejected.");
            mMaxConnectionsAlert.reset();
        }
        mEndpoint.incrementCountTotalRejects();
        closesocket(newSocket);
        return INVALID_SOCKET;
    }

    // Make sure that the client is coming from a trusted address on any endpoint. For externalFire2Secure endpoint, the trust setting
    // are not specified in the config file which actually means all the addresses make it past the isTrustedAddress check here (bizarre but cest la vie). 
    if (!mEndpoint.isTrustedAddress(addr.sin_addr.s_addr))
    {
        mTrustedAddressAlert.increment();
        if (mTrustedAddressAlert.shouldLog())
        {
            BLAZE_WARN_LOG(Log::CONNECTION, "[ServerSocketChannel].accept: dropping untrusted connection from " 
                       << addrBuf << " on endpoint '" << mEndpoint.getName() << "'; " << mTrustedAddressAlert.getCount() << " others also rejected.");
            mTrustedAddressAlert.reset();
        }
        closesocket(newSocket);
        return INVALID_SOCKET;
    }

    // Ensure that we are not going over configured rate limits for this ip
    RatesPerIp* rateLimitCounter = mEndpoint.getRateLimiter().getCurrentRatesForIp(addr.sin_addr.s_addr);
    if (rateLimitCounter != nullptr)
    {
        if (!mEndpoint.getRateLimiter().tickAndCheckConnectionLimit(*rateLimitCounter))
        {
            if (!rateLimitCounter->mLoggedConnectionLimitExceeded)
            {
                rateLimitCounter->mLoggedConnectionLimitExceeded = true;
                BLAZE_WARN_LOG(Log::CONNECTION, "[ServerSocketChannel].accept: dropping over limit connection from "
                    << addrBuf << " on endpoint '" << mEndpoint.getName() << "' because we had " << rateLimitCounter->mConnectionAttemptRate << " attempts in current window that started at " << rateLimitCounter->mCurrentConnectionInterval);
            }
            mEndpoint.incrementCountTotalRejectsMaxRateLimit();
            closesocket(newSocket);
            return INVALID_SOCKET;
        }
    }

    BLAZE_TRACE_LOG(Log::CONNECTION, "[ServerSocketChannel].accept: Accepting a new connection from " << addrBuf << ":" << ntohs(addr.sin_port));

    SocketUtil::setKeepAlive(newSocket, true);
    return newSocket;
}

} // Blaze


/*************************************************************************************************/
/*!
    \file 

    A generic socket interface that works with the selector and fibers.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_BLOCKING_SOCKET_H
#define BLAZE_BLOCKING_SOCKET_H

/*** Include files *******************************************************************************/

#ifdef EA_PLATFORM_WINDOWS
#include <winsock2.h>
#else
#include <poll.h>
#endif

#include "eathread/eathread_storage.h"

#include "framework/connection/channelhandler.h"
#include "framework/connection/socketchannel.h"

#include "framework/blazedefines.h"
#include "EATDF/time.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#ifndef CONNECTION_LOGGER_CATEGORY
    #ifdef LOGGER_CATEGORY
        #define CONNECTION_LOGGER_CATEGORY LOGGER_CATEGORY
    #else
        #define CONNECTION_LOGGER_CATEGORY Log::CONNECTION
    #endif
#endif

namespace Blaze
{
class SocketChannel;

class BlockingSocket : public ChannelHandler
{
    NON_COPYABLE(BlockingSocket);

public:
    BlockingSocket(const InetAddress& peerAddr, SOCKET sockId);
    BlockingSocket(SocketChannel& channel);
    ~BlockingSocket() override;

    int32_t getIdent() const { return mChannel.getIdent(); }

    int32_t connect(bool enableKeepAlive = true);
    int32_t recv(char8_t* buffer, size_t bufSize, int32_t flags);
    int32_t recv(RawBuffer& buf, size_t len);
    int32_t send(const char8_t* buffer, size_t bufSize, int32_t flags);
    int32_t send(const RawBuffer& buf);

    int32_t poll(struct pollfd* fds, int32_t timeout);

    void close();
    int32_t getLastError() { return mChannel.getLastError(); }

    SocketChannel& getChannel() { return mChannel; }
    const SocketChannel& getChannel() const { return mChannel; }

    bool isConnected() const { return mChannel.isConnected(); }

    const EA::TDF::TimeValue& getLastWriteTime() const { return mLastWriteTime; }
    const EA::TDF::TimeValue& getLastReadTime() const { return mLastReadTime; }
    const EA::TDF::TimeValue& getLastActivityTime() const { return mLastReadTime > mLastWriteTime ? mLastReadTime : mLastWriteTime; }

    void setupPoll(struct pollfd& fds, const Fiber::EventHandle& eventHandle);
    bool clearPoll(struct pollfd& fds);

    int32_t shutdown(int32_t how);
    bool isShuttingDown() const { return mShuttingDown; }

private:
    bool waitForRead();

    // ChannelHandler interface    
    void onRead(Channel& channel) override;
    void onWrite(Channel& channel) override;
    void onError(Channel& channel) override;
    void onClose(Channel& channel) override;

private:
    SocketChannel& mChannel;

    //RawBuffer objects to wrap read/write sockets
    Fiber::EventHandle mRecvWaitEvent;
    Fiber::EventHandle mSendWaitEvent;
    Fiber::EventHandle mPollWaitEvent;

    EA::TDF::TimeValue mLastWriteTime;
    EA::TDF::TimeValue mLastReadTime;
    bool mHasSetBlockingMode;

    uint32_t mPollOps;

    bool mShuttingDown;
};

class BlockingSocketManager 
{
public:
    BlockingSocketManager();
    ~BlockingSocketManager();

    int32_t connect(SOCKET s, const InetAddress& peerAddr);
    int32_t recv(SOCKET s, char8_t* buffer, size_t bufSize, int32_t flags);
    int32_t send(SOCKET s, const char8_t* buffer, size_t bufSize, int32_t flags);
    int32_t close(SOCKET s);
    int32_t poll(struct pollfd* fds, uint32_t nfds, int32_t timeout);
    int32_t getLastError(SOCKET s);

private:
    typedef eastl::hash_map<SOCKET, BlockingSocket*> SocketChannelMap;
    SocketChannelMap mOpenSockets;

};

extern EA_THREAD_LOCAL BlockingSocketManager* gBlockingSocketManager;

} // Blaze

#endif // BLAZE_BLOCKING_SOCKET_H


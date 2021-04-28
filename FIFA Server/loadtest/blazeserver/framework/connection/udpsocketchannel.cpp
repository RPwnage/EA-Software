/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UdpSocketChannel

    Channel wrapper over a stream/TCP socket.  Provides the ability to register with a Selector,
    and to read/write ByteBuffers.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/inetaddress.h"
#include "framework/connection/selector.h"
#include "framework/connection/socketutil.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/connection/udpsocketchannel.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief Constructor
*/
/*************************************************************************************************/
UdpSocketChannel::UdpSocketChannel()
    : mSocket(INVALID_SOCKET),
      mLastError(0)
{
}

/*************************************************************************************************/
/*!
    \brief Destructor
*/
/*************************************************************************************************/
UdpSocketChannel::~UdpSocketChannel()
{
    // Nothing to do if socket is invalid
    if (mSocket != INVALID_SOCKET)
    {
        unregisterChannel();
        closesocket(mSocket);
        mSocket = INVALID_SOCKET;
    }
}

/*************************************************************************************************/
/*!
    \brief initialize

    Allocate resources and intialize them.

    \return - true if initialization was successful, false if not
*/
/*************************************************************************************************/
bool UdpSocketChannel::initialize(sockaddr_in& socketAddr)
{
    // Create socket
    mSocket = SocketUtil::createSocket(false, SOCK_DGRAM);
    if (mSocket == INVALID_SOCKET)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[UdpSocketChannel].initialize: Failed to open socket.");
        return false;
    }

    SocketUtil::setSocketBlockingMode(mSocket, false);

    // Allow socket reuse
    int on = 1;
#ifdef EA_PLATFORM_WINDOWS
    // Winsock apparently has different behaviour for SO_REUSEADDR than BSD
    // sockets so we have to use SO_EXCLUSIZEADDRUSE instead to prevent
    // multiple servers starting on the same port.
    ::setsockopt(mSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&on, sizeof(on));
#else
    ::setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
#endif

    // Bind it to the local address
    if (::bind(mSocket, (sockaddr*) &socketAddr, sizeof(socketAddr)) == SOCKET_ERROR)
    {
        int32_t error = WSAGetLastError();
        char8_t errBuf[128];
        BLAZE_ERR_LOG(Log::CONNECTION, "[UdpSocketChannel].initialize: Failed to bind to local address: "
                  << SocketUtil::getErrorMsg(errBuf, sizeof(errBuf), error) << " (" << error << ")" );
        return false;
    }

    return registerChannel(Channel::OP_READ);
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
int32_t UdpSocketChannel::read(RawBuffer& buffer, size_t length, sockaddr_in& fromAddr)
{
    int32_t numToRead;
    socklen_t fromSize = static_cast<socklen_t>(sizeof(fromAddr));
    size_t bufferRoom = buffer.tailroom();
    if ((length == 0) || (length > bufferRoom))
        numToRead = (int32_t)bufferRoom;
    else
        numToRead = (int32_t)length;

    int32_t numRead = ::recvfrom(mSocket, (char8_t *)buffer.tail(), numToRead, 0, (struct sockaddr *) &fromAddr, &fromSize);
    if (numRead == SOCKET_ERROR)
    {
        mLastError = WSAGetLastError();

        if (SOCKET_WOULDBLOCK(mLastError))
        {
            // Read would block so don't error out; just indicate that nothing could be read
            return 0;
        }

        // Fatal socket error
        BLAZE_WARN_LOG(Log::CONNECTION, "[UdpSocketChannel].read: recvfrom failed; code=" << mLastError);
        return -1;
    }
    else if (numRead == 0)
    {
        // Lost connection
        BLAZE_TRACE_LOG(Log::CONNECTION, "[UdpSocketChannel].read: connection closed.");
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
int32_t UdpSocketChannel::write(RawBuffer& buffer, const sockaddr_in& toAddr)
{
    int32_t numWritten = ::sendto(mSocket, (const char8_t *)buffer.data(), (int32_t)buffer.datasize(), 0, (struct sockaddr *) &toAddr, sizeof(toAddr));
    if (numWritten == SOCKET_ERROR)
    {
        mLastError = WSAGetLastError();
        if (SOCKET_WOULDBLOCK(mLastError))
        {
            // Write would block; don't error out, just indicate that nothing could be written
            return 0;
        }

        BLAZE_WARN_LOG(Log::CONNECTION, "[UdpSocketChannel].write: sendto failed; code=" << getLastError());
        return -1;
    }
    else if (numWritten == 0)
    {
        // Lost connection
        BLAZE_TRACE_LOG(Log::CONNECTION, "[UdpSocketChannel].write: connection closed.");
        mLastError = 0;
        return -1;
    }

    // Update the buffer so its position is correct
    buffer.pull(numWritten);

    return numWritten;
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // namespace Blaze


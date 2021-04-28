/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_UDPSOCKETCHANNEL_H
#define BLAZE_UDPSOCKETCHANNEL_H

/*** Include files *******************************************************************************/
#include "eathread/eathread_storage.h"
#include "framework/connection/platformsocket.h"
#include "framework/connection/channel.h"
#include "framework/connection/channelhandler.h"
#include "framework/connection/inetaddress.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{
class Protocol;

class UdpSocketChannel : public Channel
{
    NON_COPYABLE(UdpSocketChannel)
public:
    UdpSocketChannel();
    ~UdpSocketChannel() override;

    virtual bool initialize(sockaddr_in& socketAddr);
    bool isInitialized() const { return (mSocket != INVALID_SOCKET); }

    ChannelHandle getHandle() override { return static_cast<ChannelHandle>(mSocket); }
    virtual SOCKET getSocket() { return mSocket; }

    int32_t read(RawBuffer& buffer, size_t length, sockaddr_in& fromAddr);
    int32_t write(RawBuffer& buffer, const sockaddr_in& toAddr);

    int32_t getLastError() { return mLastError; }

protected:
    // Private Members
    SOCKET mSocket;
    int32_t mLastError;

private:
};

} // Blaze

#endif // BLAZE_UDPSOCKETCHANNEL_H

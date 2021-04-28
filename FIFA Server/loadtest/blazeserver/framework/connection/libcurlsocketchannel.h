/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_LIBCURLSOCKETCHANNEL_H
#define BLAZE_LIBCURLSOCKETCHANNEL_H

/*** Include files *******************************************************************************/
#include "eathread/eathread_storage.h"
#include "EASTL/intrusive_list.h"
#include "framework/connection/platformsocket.h"
#include "framework/connection/channel.h"
#include "framework/connection/channelhandler.h"
#include "framework/connection/inetaddress.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{
class Protocol;
class Fiber;

class LibcurlSocketChannel : public Channel, public eastl::intrusive_list_node
{
    NON_COPYABLE(LibcurlSocketChannel)
public:
    LibcurlSocketChannel(SOCKET socket):mSocket(socket){};
    ~LibcurlSocketChannel() override{};

    ChannelHandle getHandle() override { return static_cast<ChannelHandle>(mSocket); }

protected:
    // Private Members
    SOCKET mSocket;

};

} // Blaze

#endif // BLAZE_LIBCURLSOCKETCHANNEL_H


#include "BlockedUserMessageFilter.h"

#include <chat/BlockList.h>
#include <chat/Message.h>
#include <services/log/LogService.h>

namespace Origin
{
namespace Engine
{
namespace Social
{
    BlockedUserMessageFilter::BlockedUserMessageFilter(Chat::BlockList *blockList) :
        mBlockList(blockList)
    {
    }
        
    bool BlockedUserMessageFilter::filterMessage(const Chat::Message &message)
    {
        const bool isBlocked = mBlockList->jabberIDs().contains(message.from().asBare());

        if (isBlocked)
        {
            // The server should be blocking this
            ORIGIN_LOG_WARNING << "Received message from blocked user " << QString::fromUtf8(message.from().toEncoded());
            return true;
        }

        return false;
    }

}
}
}

#ifndef _BLOCKEDUSERMESSAGEFILTER_H
#define _BLOCKEDUSERMESSAGEFILTER_H

#include "chat/MessageFilter.h"

namespace Origin
{
namespace Chat
{
    class BlockList;
}

namespace Engine
{
namespace Social
{
    ///
    /// Drops messages from blocked users and warns
    ///
    class BlockedUserMessageFilter : public Chat::MessageFilter
    {
    public:
        explicit BlockedUserMessageFilter(Chat::BlockList *blockList);

        bool filterMessage(const Chat::Message &);

    private:
        Chat::BlockList *mBlockList;
    };
}
}
}

#endif


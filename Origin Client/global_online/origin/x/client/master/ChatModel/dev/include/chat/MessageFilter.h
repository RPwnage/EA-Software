#ifndef _CHATMODEL_MESSAGEFILTER_H
#define _CHATMODEL_MESSAGEFILTER_H

namespace Origin
{
namespace Chat
{
    class Message;
    
    ///
    /// Abstract base class for MessageFilters
    ///
    /// MessageFilters can be used to perform actions on or reject incoming messages.
    ///
    /// MessageFilters can be installed with Chat::Connection::installIncomingMessageFilter
    ///
    class MessageFilter
    {
    public:
        virtual ~MessageFilter()
        {
        }

        ///
        /// Filters a message
        ///
        /// \return true to prevent further processing of this message, false to continue
        ///
        virtual bool filterMessage(const Message &) = 0;
    };
}
}

#endif

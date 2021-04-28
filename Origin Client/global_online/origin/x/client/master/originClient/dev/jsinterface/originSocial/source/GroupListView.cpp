#include "GroupListView.h"
#include "ChatGroupProxy.h"
#include "ChatChannelProxy.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            GroupListView::GroupListView(QObject *parent)
                : QObject(parent)
                , m_delayedChangedQueued(false)
            {
            }

            QObjectList GroupListView::groups()
            {
                QObjectList ret; 

                for(QSet<ChatGroupProxy*>::const_iterator it = m_currentChatGroups.constBegin();
                    it != m_currentChatGroups.constEnd();
                    it++)
                {
                    ret.append(*it);
                }

                return ret;
            }

            void GroupListView::markViewChanged()
            {
                // Simple event compression
                if (!m_delayedChangedQueued)
                {
                    m_delayedChangedQueued = true;
                    QMetaObject::invokeMethod(this, "emitDelayedChanged", Qt::QueuedConnection);
                }
            }

            void GroupListView::emitDelayedChanged()
            {
                m_delayedChangedQueued = false;
                emit changed();
            }

            // Functions called from OriginSocialProxy
            void GroupListView::chatGroupAdded(ChatGroupProxy* proxy)
            {
                // Call our filter function
                bool alreadyIncluded = m_currentChatGroups.contains(proxy);

                if (!alreadyIncluded)
                {
                    m_currentChatGroups.insert(proxy);
                    emit addChatGroup(proxy);
                }

                markViewChanged();
            }

            void GroupListView::chatGroupRemoved(ChatGroupProxy *proxy)
            {
                if (m_currentChatGroups.remove(proxy))
                {
                    emit removeChatGroup(proxy);
                    markViewChanged();
                }
            }

            void GroupListView::chatGroupChanged(ChatGroupProxy *proxy)
            {
                emit changeChatGroup(proxy);
                markViewChanged();
            }

            void GroupListView::chatChannelAdded(ChatGroupProxy *group, ChatChannelProxy *channel)
            {
                group->addChatChannelProxy(channel);
                emit addChatChannel(group, channel);
                markViewChanged();
            }

            void GroupListView::chatChannelRemoved(ChatGroupProxy *group, ChatChannelProxy *channel)
            {
                group->removeChatChannelProxy(channel);
                emit removeChatChannel(group, channel);
                markViewChanged();
            }

            void GroupListView::chatChannelChanged(ChatGroupProxy *group, ChatChannelProxy *channel)
            {
                emit changeChatChannel(group, channel);
                markViewChanged();
            }
        }
    }
}
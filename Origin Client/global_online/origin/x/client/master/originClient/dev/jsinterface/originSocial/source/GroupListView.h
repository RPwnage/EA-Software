#ifndef _GROUPLISTVIEW_H
#define _GROUPLISTVIEW_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QSet>

namespace Origin
{

namespace Client
{
namespace JsInterface
{
    class OriginSocialProxy;
    class ChatGroupProxy;
    class ChatChannelProxy;

class GroupListView : public QObject
{
    Q_OBJECT
    friend class OriginSocialProxy;

public:
    GroupListView(QObject *parent = NULL);
    virtual ~GroupListView() {}

    Q_PROPERTY(QObjectList groups READ groups);
    QObjectList groups();

    QSet<ChatGroupProxy*> currentChatGroups() { return m_currentChatGroups; }

signals:
    void changed();

    void addChatGroup(QObject *group);
    void removeChatGroup(QObject *group);
    void changeChatGroup(QObject *group);

    void addChatChannel(QObject *group, QObject *channel);
    void removeChatChannel(QObject *group, QObject *channel);
    void changeChatChannel(QObject *group, QObject *channel);

protected:
    void chatGroupAdded(ChatGroupProxy *group);
    void chatGroupRemoved(ChatGroupProxy *group);
    void chatGroupChanged(ChatGroupProxy *group);

    void chatChannelAdded(ChatGroupProxy *group, ChatChannelProxy *channel);
    void chatChannelRemoved(ChatGroupProxy *group, ChatChannelProxy *channel);
    void chatChannelChanged(ChatGroupProxy *group, ChatChannelProxy *channel);

    Q_INVOKABLE void emitDelayedChanged();
    void markViewChanged();

protected:
    QSet<ChatGroupProxy*> m_currentChatGroups;
    bool m_delayedChangedQueued;
};

}
}
}

#endif

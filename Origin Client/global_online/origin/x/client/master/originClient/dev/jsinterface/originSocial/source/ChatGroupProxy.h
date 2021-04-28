#ifndef _CHATGROUPPROXY_H
#define _CHATGROUPPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QSet>

#include <chat/ChatGroup.h>
#include "chat/JabberID.h"
#include <services/rest/GroupServiceResponse.h>

namespace Origin
{
namespace Client
{
namespace JsInterface
{
    class ChatChannelProxy;
class ChatGroupProxy : public QObject
{
    Q_OBJECT 
public:
    ChatGroupProxy(Chat::ChatGroup *proxied);
    virtual ~ChatGroupProxy();

    Q_INVOKABLE void getGroupChannelInformation();

    Q_INVOKABLE QString getRemoteUserRole(QObject* user);

    Q_INVOKABLE bool isUserInGroup(QObject* user);

    Q_INVOKABLE void inviteUsersToGroup(const QObjectList& users);

    Q_INVOKABLE void acceptGroupInvite();
    Q_INVOKABLE void rejectGroupInvite();

    Q_INVOKABLE void failedToEnterRoom();

    void addChatChannelProxy(ChatChannelProxy* proxy);
    void removeChatChannelProxy(ChatChannelProxy* proxy);

    Q_PROPERTY(QString id READ id);
    QString id();

    Q_PROPERTY (QString name READ name)
    QString name();

    Q_PROPERTY (QString groupGuid READ groupGuid)
    QString groupGuid();

    Q_PROPERTY (QVariant invitedBy READ invitedBy)
    QVariant invitedBy();

    Q_PROPERTY (QObjectList channels READ channels)
    QObjectList channels();

    Q_PROPERTY (QString role READ role)
    QString role();
    
    Q_PROPERTY (QVariantList members READ members)
    QVariantList members();

    Q_PROPERTY (QVariantList admins READ admins)
    QVariantList admins();

    Q_PROPERTY (QVariantList owners READ owners)
    QVariantList owners();

    Q_PROPERTY (bool inviteGroup READ inviteGroup)
    bool inviteGroup();
    
    // Returns our backing ChatGroup
    Chat::ChatGroup* proxied() { return mProxied; }

    static int mNextId;

signals:
    void rejoinMucRoomSuccess();
    void enterMucRoomFailed(int);
    void membersLoadFinished();
    void anyChange();

public slots:
    void onAnyChange(quint64 id);
    void onUsersInvitedToGroup(QList<Origin::Chat::RemoteUser*>);

protected:

private slots:
    void onGroupMemberRemoved(Origin::Chat::JabberID);
    void onMembersLoadFinished();
    void onGroupMembersRoleChange(quint64 userId, const QString& role);
private:
    Chat::ChatGroup *mProxied;

    QVariantList mMembers;
    QVariantList mAdmins;
    QVariantList mOwners;
    QSet<ChatChannelProxy*> mChatChannelProxies;
    qint64 getUserNucleusId(QObject* user);
    int mId;
};

}
}
}

#endif

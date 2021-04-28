#ifndef _CHATCHANNELPROXY_H
#define _CHATCHANNELPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QVariant>

#include <chat/ChatChannel.h>

#include "engine/login/User.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{
class ChatChannelProxy : public QObject
{
    Q_OBJECT 
public:
    ChatChannelProxy(Chat::ChatChannel *proxied, int id, Engine::UserRef user);

    Q_INVOKABLE void joinChannel();
    Q_INVOKABLE void rejoinChannel();
    Q_INVOKABLE void getChannelPresence();

    Q_INVOKABLE void inviteUser(QObject* user);

    Q_INVOKABLE QString getParticipantRole(QObject* user);

    Q_PROPERTY(QString id READ id);
    QString id();

    Q_PROPERTY (QString name READ name)
    QString name();

    Q_PROPERTY (QString channelId READ channelId)
    QString channelId();

    Q_PROPERTY (QString groupGuid READ groupGuid)
    QString groupGuid();

    Q_PROPERTY (bool locked READ locked)
    bool locked();

    Q_PROPERTY (QString status READ status)
    QString status();

    Q_PROPERTY (QString role READ role)
    QString role();

    Q_PROPERTY(QObjectList channelMembers READ channelMembers);
    QObjectList channelMembers();
    
    Q_PROPERTY(bool inChannel READ inChannel);
    bool inChannel();

    // Returns our backing Chat Channel
    Chat::ChatChannel* proxied() { return mProxied; }

    Q_PROPERTY (QString invitedBy READ invitedBy)
    QString invitedBy();

signals:

protected:

private slots:
    void startJoinChannel(const bool& success);
    void inviteUserFinished();

private:
    Chat::ChatChannel *mProxied;
    int mId;
    Engine::UserRef mUser;
};

}
}
}

#endif

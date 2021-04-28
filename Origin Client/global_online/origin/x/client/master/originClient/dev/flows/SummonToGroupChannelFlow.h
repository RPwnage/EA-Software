#ifndef _SUMMONTOGROUPCHANNELFLOW_H
#define _SUMMONTOGROUPCHANNELFLOW_H

#include <QObject>

#include <engine/login/User.h>

#include "chat/ConnectedUser.h" 
#include <chat/ChatChannel.h>
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

///
class ORIGIN_PLUGIN_API SummonToGroupChannelFlow : public QObject
{
    Q_OBJECT
public:
    explicit SummonToGroupChannelFlow(Chat::ChatChannel* channel, Engine::UserRef user, const Chat::JabberID& toJid, const QString& groupId, const QString& channelId);

    ///
    /// Starts the flow
    ///
    void start();

signals:
    void finished();

private slots:
    void onGroupInviteSuccess();

private:
    Engine::UserRef mUser;
    Chat::ChatChannel* mChannel;
    Chat::JabberID mToJid;
    QString mGroupId;
    QString mChannelId;
};

}
}

#endif


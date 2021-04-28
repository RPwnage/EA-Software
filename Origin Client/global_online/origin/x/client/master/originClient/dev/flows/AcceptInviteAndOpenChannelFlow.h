#ifndef _ACCEPTINVITEANDOPENCHANNELFLOW_H
#define _ACCEPTINVITEANDOPENCHANNELFLOW_H

#include <QObject>

#include <engine/login/User.h>

#include "chat/ConnectedUser.h" 
#include "chat/ChatGroup.h"
#include "chat/ChatChannel.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

///
class ORIGIN_PLUGIN_API AcceptInviteAndOpenChannelFlow : public QObject
{
    Q_OBJECT
public:
    explicit AcceptInviteAndOpenChannelFlow(Chat::ChatGroup* group);

    ///
    /// Starts the flow
    ///
    void start();

signals:

private slots:
    void onGroupInviteAcceptSuccess();
    void onChannelAdded(Origin::Chat::ChatChannel* channel);

private:

    Chat::ChatGroup* mGroup;
};


}
}

#endif


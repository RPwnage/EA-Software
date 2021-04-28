#ifndef _INGAMESTATUSWATCHER_H
#define _INGAMESTATUSWATCHER_H

#include <QObject>
#include <QString>

#include <engine/login/User.h>
#include <engine/content/Entitlement.h>
#include <chat/Presence.h>

namespace Origin
{
namespace Engine
{
namespace Social
{

class ConnectedUserPresenceStack;

/// \brief Internal class to automatically set our status for game launches and exists 
class InGameStatusWatcher : public QObject
{
    Q_OBJECT
public:
    InGameStatusWatcher(ConnectedUserPresenceStack *presenceStack, UserRef user, QObject *parent = NULL);

private slots:
    void onPlayStarted(Origin::Engine::Content::EntitlementRef);
    void delayedPlayStarted();

    void onPlayFinished(Origin::Engine::Content::EntitlementRef);

    void onBroadcastStarted(const QString& broadcastUrl);
    void onBroadcastStopped();

private:
    Q_INVOKABLE void connectBroadcastSignals();

    QString mPresenceProductId;
    ConnectedUserPresenceStack *mPresenceStack;
    UserWRef mUser;
};

}
}
}

#endif


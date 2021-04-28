#ifndef _ONLINESTATUSPROXY_H
#define _ONLINESTATUSPROXY_H

#include <QObject>
#include "engine/login/User.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API OnlineStatusProxy : public QObject
{
    Q_OBJECT
public:
    explicit OnlineStatusProxy(bool canRequestState = false);

    Q_PROPERTY(bool onlineState READ onlineState NOTIFY onlineStateChanged);
    bool onlineState();

    Q_INVOKABLE void requestManualOfflineMode();
    Q_INVOKABLE void requestOnlineMode();

private slots:
    void connectUserSignals(Origin::Engine::UserRef);

signals:
    void onlineStateChanged(bool state);

private:
    bool mCanRequestState;
};

}
}
}

#endif

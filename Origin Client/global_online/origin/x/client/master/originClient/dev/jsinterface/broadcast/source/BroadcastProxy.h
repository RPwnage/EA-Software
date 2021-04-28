#ifndef _BROADCASTPROXY_H
#define _BROADCASTPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
    class IGOTwitch;
}
namespace Client
{
namespace JsInterface
{
#ifdef ORIGIN_PC
class ORIGIN_PLUGIN_API BroadcastProxy : public QObject
{
    Q_OBJECT
public:
    BroadcastProxy(QObject* parent = NULL);
    Q_INVOKABLE void showLogin();
    Q_INVOKABLE void disconnectAccount();
    Q_INVOKABLE void connectAccount();
    Q_INVOKABLE bool attemptBroadcastStart();
    Q_INVOKABLE bool attemptBroadcastStop();
    Q_INVOKABLE bool isBroadcastingBlocked(uint32_t id = 0) const;

    Q_PROPERTY(bool enabled READ enabled)
    bool enabled() const;

    Q_PROPERTY(bool isUserConnected READ isUserConnected)
    bool isUserConnected() const;

    Q_PROPERTY(bool isBroadcasting READ isBroadcasting)
    bool isBroadcasting() const;

    Q_PROPERTY(bool isBroadcastingPending READ isBroadcastingPending)
    bool isBroadcastingPending() const;

    Q_PROPERTY(bool isBroadcastTokenValid READ isBroadcastTokenValid)
    bool isBroadcastTokenValid() const;

    Q_PROPERTY(int broadcastDuration READ broadcastDuration)
    int broadcastDuration() const;

    Q_PROPERTY(int broadcastViewers READ broadcastViewers)
    int broadcastViewers() const;

    Q_PROPERTY(QString broadcastUserName READ broadcastUserName)
    const QString broadcastUserName() const;

    Q_PROPERTY(QString broadcastGameName READ broadcastGameName)
    const QString broadcastGameName() const;

    Q_PROPERTY(QString broadcastChannel READ broadcastChannel)
    const QString broadcastChannel() const;

    Q_PROPERTY(bool isBroadcastOptEncoderAvailable READ isBroadcastOptEncoderAvailable)
    bool isBroadcastOptEncoderAvailable() const;

    Q_PROPERTY(QString currentBroadcastedProductId READ currentBroadcastedProductId)
    const QString currentBroadcastedProductId() const;

signals:
    void broadcastStarted();
    void broadcastStopped();
    void broadcastStartStopError();
    void broadcastUserNameChanged(const QString& userName);
    void broadcastGameNameChanged(const QString &gameName);
    void broadcastDurationChanged(const int& duration);
    void broadcastChannelChanged(const QString &channel);
    void broadcastStreamInfoChanged(int viewers);
    void broadcastOptEncoderAvailable(const bool& available);
    void broadcastPermitted();
    void broadcastStartPending();
    void broadcastErrorOccurred(int errorCategory, int errorCode);
    void broadcastStatusChanged(bool status, bool archivingEnabled, const QString &fixArchivingURL, const QString &channelURL);
    void broadcastingStateChanged(bool broadcasting, bool isBroadcastPending);

private:
    Engine::IGOTwitch* mIgoTwitch;
};
#endif
}
}
}

#endif
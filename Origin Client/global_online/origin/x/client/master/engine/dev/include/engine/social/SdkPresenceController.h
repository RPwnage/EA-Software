#ifndef _SDKPRESENCECONTROLLER_H
#define _SDKPRESENCECONTROLLER_H

// Copyright 2013, Electronic Arts
// All rights reserved.

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMutex>

#include "engine/login/User.h"
#include "chat/OriginGameActivity.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{

namespace MultiplayerInvite
{
    class SessionInfo;
}

namespace Social
{
    class ConnectedUserPresenceStack;

    ///
    /// \brief Manages presence being set from SDK games 
    ///
    /// This is called in from the XMPP_ServiceArea to communicate the game's wanted presence state to the rest of
    /// the application.
    ///
    /// This class is thread safe.
    ///
    class ORIGIN_PLUGIN_API SdkPresenceController : public QObject
    {
        Q_OBJECT
    public:
        /// \brief Creates a new SdkPresenceController instance
        SdkPresenceController(ConnectedUserPresenceStack *presenceStack);

        ///
        /// \brief Sets the game presence and multiplayer session
        ///
        /// \param productId     Product ID of the game setting the session
        /// \param gameActivity  Game activity information
        /// \param status        Status string to set with the game activity
        /// \param sessionData   Opaque game-specific multiplayer session information for joinable games
        ///
        Q_INVOKABLE void set(QString productId, Origin::Chat::OriginGameActivity gameActivity, QString status, QByteArray sessionData = QByteArray());

        /// \brief Clears the SDK presence if it was set by the given content ID
        Q_INVOKABLE void clearIfSetBy(QString setterProductId);

        ///
        /// \brief Returns session information about the playing game  usable by the multiplayer invite system
        ///
        /// This will return null if no SDK presence is set
        ///
        MultiplayerInvite::SessionInfo multiplayerSessionInfo() const;

    private slots:
        void onBroadcastStarted(const QString& broadcastUrl);
        void onBroadcastStopped();

    protected:
        Q_INVOKABLE void connectBroadcastSignals();

        ConnectedUserPresenceStack *mPresenceStack;
            
        mutable QMutex mStateLock;

        QString mProductId;
        Chat::OriginGameActivity mGameActivity;
        QByteArray mSessionData;
        QString mStatus;
        QString mBroadcastUrl;
    };
}
}
}

#endif


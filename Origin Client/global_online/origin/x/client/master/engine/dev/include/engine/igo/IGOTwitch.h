// *********************************************************************
//  IGOTwitch.h
//  Copyright (c) 2014 Electronic Arts Inc. All rights reserved.
// *********************************************************************

#ifndef IGOTWITCH_H
#define IGOTWITCH_H

#ifdef ORIGIN_PC

#include "IIGOCommandController.h"

#include <limits>
#include <QDateTime>
#include <QDebug>
#include <QTimer>

#include <stdint.h>

namespace Origin
{
    namespace Services
    {
        // Fwd decls        
        class Variant;
    }
}

namespace Origin
{
    namespace Engine
    {

        // Fwd decls
        class IGOWindowManagerIPC;
        class IGOGameTracker;

        class IGOTwitch : public QObject, public Origin::Engine::IIGOCommandController
        {
            friend class IGOWindowManager;

            Q_OBJECT

        public:
            IGOTwitch();
            virtual ~IGOTwitch();

            void setGameTracker(Origin::Engine::IGOGameTracker *tracker);
            void setIPCServer(Origin::Engine::IGOWindowManagerIPC *ipc);
            void storeTwitchLogin(const QString& token, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);

            bool attemptBroadcastStart(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            bool attemptBroadcastStop(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            bool attemptDisplayWindow(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            bool disconnectAccount();

            bool isBroadcasting() const;
            bool isBroadcastingPending() const;

            bool isBroadcastTokenValid() const;
            bool isBroadcastingBlocked(uint32_t id = 0) const;

            int broadcastDuration() const { return mBroadcastDuration; }
            int broadcastViewers() const { return mBroadcastViewers; }
            const QString broadcastGameName() const;
            const QString & broadcastUserName() const { return mBroadcastUserName; }
            const QString & broadcastChannel() const { return mBroadcastChannel; }
			bool isBroadcastOptEncoderAvailable() const { return mBroadcastOptEncoderAvailable; }

            //for telemetry
            const QString & currentBroadcastedProductId() { return mBroadcastedProductId;  }
            void setBroadcastOriginatedFromSDK(bool flag) { mBroadcastInitiatedFromSDK = flag; }
            void triggerBroadcastAccountLinkedTelemetry();
            QString createBroadcastSettingsStringForTelemetry();

            // queued ?
            void injectBroadcastError(int errorCategory, int errorCode);
            void injectBroadcastStatus(bool status, bool archivingEnabled, const QString &fixArchivingURL, const QString &channelURL);
            void injectBroadcastUserName(const QString &userName);
            void injectBroadcastStreamInfo(int viewers);
			void injectBroadcastOptEncoderAvailable(bool available);

        signals:

            void showBroadcastDialog();

            void broadcastStarted(const QString& broadcastUrl);
            void broadcastStopped();
            void broadcastStoppedError();
            void broadcastPermitted(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void broadcastNetworkError(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void broadcastDialogOpen(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void broadcastDialogClosed();
            void broadcastIntroOpen(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void broadcastIntroClosed();
            void broadcastAccountLinkDialogOpen(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);
            void broadcastAccountDisconnected();
            void broadcastStartPending();
            void broadcastStartStopError();

            void broadcastErrorOccurred(int errorCategory, int errorCode);
            void broadcastErrorOccurred_forSDK();    // just to let Origin SDK know
            void broadcastStatusChanged(bool status, bool archivingEnabled, const QString &fixArchivingURL, const QString &channelURL);
            void broadcastingStateChanged(bool broadcasting, bool isBroadcastPending);
            void broadcastUserNameChanged(const QString &userName);
            void broadcastGameNameChanged(const QString &gameName);
            void broadcastChannelChanged(const QString &channel);
            void broadcastStreamInfoChanged(int viewers);
            void broadcastIndicatorChanged(bool visible);
            void broadcastDurationChanged(int duration);
			void broadcastOptEncoderAvailable(bool available);

        private slots:

            void setAutobroadcastState(bool state);
            void toggleBroadcasting();
            void toggleBroadcasting(Origin::Engine::IIGOCommandController::CallOrigin callOrigin);

            void gameLaunched(uint32_t id);
            void gameFocused(uint32_t id);
            void gameStopped(uint32_t id);
            void settingChanged(const QString& settingKey, const Origin::Services::Variant& value);

            void setBroadcastStatus(bool status, bool archivingEnabled, const QString & fixArchivingURL, const QString & channelURL);
            void setBroadcastError(int errorCategory, int errorCode);
            void updateBroadcastGameName();
            void setBroadcastUserName(const QString &userName);
            void setBroadcastStreamInfo(int viewers);
			void setBroadcastOptEncoderAvailable(bool available);
            void updateBroadcastDuration();
            void stopPendingBroadcast();
            void initiateAutobroadcast();

        private:

            bool igoCommand(int cmd, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);

            QString mBroadcastUserName;
            QString mBroadcastToken;
            QString mBroadcastChannel;


            bool mIsBroadcasting;
            bool mBroadcastingStatsCleared;
            bool mIsAutoBroadcastingEnabled;
            bool mIsAutoBroadcastingTokenMissing;
            uint32_t mCurrentBroadcastConnection;
            QTimer mAutobroadcastTimer;
            QTimer mBroadcastStartPendingTimer;
            bool mIsBroadcastStartPending;
            bool mBroadcastStoppedManually;
            QTimer mBroadcastDurationTimer;
            int mBroadcastDuration; // in seconds
            int mBroadcastViewers;
            bool mBroadcastInitiatedFromSDK;
			bool mBroadcastOptEncoderAvailable;
            QString mBroadcastedProductId;

            IGOWindowManagerIPC* mIPCServer;
            IGOGameTracker* mGameTracker;

            IIGOCommandController::CallOrigin mPendingCallOrigin; // used around settings callback

        };
    }    // Engine
}    // Origin

#endif  // ORIGIN_PC
#endif // IGOTWITCH_H

// *********************************************************************
//  IGOTwitch.cpp
//  Copyright (c) 2014 Electronic Arts Inc. All rights reserved.
// *********************************************************************

#include "engine/igo/IGOTwitch.h"

#ifdef ORIGIN_PC

#include "IGOController.h"
#include "IGOGameTracker.h"
#include "IGOWindowManager.h"
#include "IGOWindowManagerIPC.h"
#include "IGOWinHelpers.h"
#include "IGOIPC/IGOIPC.h"
#include "TwitchManager.h"

#include "services/session/SessionService.h"
#include "services/settings/SettingsManager.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/network/NetworkAccessManagerFactory.h"

#include "TelemetryAPIDLL.h"


using namespace Origin;

namespace Origin
{
namespace Engine
{
    IGOTwitch::IGOTwitch()
        : mIsBroadcasting(false)
        , mBroadcastingStatsCleared(false)
        , mIsAutoBroadcastingEnabled(false)
        , mIsAutoBroadcastingTokenMissing(false)
        , mCurrentBroadcastConnection(0)
        , mIsBroadcastStartPending(false)
        , mBroadcastStoppedManually(false)
        , mBroadcastDuration(0)
        , mBroadcastViewers(0)
        , mBroadcastInitiatedFromSDK(false)
		, mBroadcastOptEncoderAvailable(false)
        , mIPCServer(NULL)
        , mGameTracker(NULL)
        , mPendingCallOrigin(IIGOCommandController::CallOrigin_UNDEFINED)
    {
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(broadcastErrorOccurred(int, int)), this, SLOT(setBroadcastError(int, int)));
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(broadcastStatusChanged(bool, bool, const QString &, const QString &)), this, SLOT(setBroadcastStatus(bool, bool, const QString &, const QString &)));        
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(broadcastUserNameChanged(const QString &)), this, SLOT(setBroadcastUserName(const QString &)));
		ORIGIN_VERIFY_CONNECT(this, SIGNAL(broadcastStreamInfoChanged(int)), this, SLOT(setBroadcastStreamInfo(int)));
		ORIGIN_VERIFY_CONNECT(this, SIGNAL(broadcastOptEncoderAvailable(bool)), this, SLOT(setBroadcastOptEncoderAvailable(bool)));

        ORIGIN_VERIFY_CONNECT(&mAutobroadcastTimer, SIGNAL(timeout()), this, SLOT(initiateAutobroadcast()));
        ORIGIN_VERIFY_CONNECT(&mBroadcastStartPendingTimer, SIGNAL(timeout()), this, SLOT(stopPendingBroadcast()));
        ORIGIN_VERIFY_CONNECT(&mBroadcastDurationTimer, SIGNAL(timeout()), this, SLOT(updateBroadcastDuration()));

        ORIGIN_VERIFY_CONNECT(Origin::Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)), this, SLOT(settingChanged(const QString&, const Origin::Services::Variant&)));
    }

    IGOTwitch::~IGOTwitch()
    {
    }

    void IGOTwitch::setGameTracker(Origin::Engine::IGOGameTracker *tracker)
    {
        mGameTracker = tracker;
        ORIGIN_VERIFY_CONNECT(mGameTracker, SIGNAL(gameAdded(uint32_t)), this, SLOT(gameLaunched(uint32_t)));
        ORIGIN_VERIFY_CONNECT(mGameTracker, SIGNAL(gameRemoved(uint32_t)), this, SLOT(gameStopped(uint32_t)));
        ORIGIN_VERIFY_CONNECT(mGameTracker, SIGNAL(gameFocused(uint32_t)), this, SLOT(gameFocused(uint32_t)));
    }

    void IGOTwitch::setIPCServer(Origin::Engine::IGOWindowManagerIPC *ipc)
    {
        mIPCServer = ipc;
    }


    void IGOTwitch::gameLaunched(uint32_t id)
    {
        // new game launched, activate auto broadcast timer
        mAutobroadcastTimer.start(5000);
    }

    void IGOTwitch::gameStopped(uint32_t id)
    {
        if (mCurrentBroadcastConnection == id && (mIsBroadcasting || mIsBroadcastStartPending))
            setBroadcastError(OriginIGO::TwitchManager::ERROR_CATEGORY_NONE, OriginIGO::TwitchManager::BROADCAST_REQUEST_TIMEOUT);
    }

    void IGOTwitch::gameFocused(uint32_t id)
    {
        updateBroadcastGameName();
    }

    bool IGOTwitch::isBroadcasting() const
    {
        return mIsBroadcasting;
    }

    bool IGOTwitch::isBroadcastingPending() const
    {
        return mIsBroadcastStartPending;
    }

    bool IGOTwitch::isBroadcastTokenValid() const
    {
        return !mBroadcastToken.isEmpty();
    }


    bool IGOTwitch::igoCommand(int cmd, Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
    {
        switch (cmd)
        {
            case IIGOCommandController::CMD_START_AUTOBROADCAST:

                attemptBroadcastStart(IIGOCommandController::CallOrigin_CLIENT);
                return true;

            case IIGOCommandController::CMD_SHOW_BROADCAST:
            {                
                return attemptDisplayWindow(callOrigin);
            }
            
        }
        return false;
    }

    bool IGOTwitch::attemptDisplayWindow(Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
    {
        if (!mIsBroadcasting && !mIsBroadcastStartPending)
            mCurrentBroadcastConnection = mGameTracker->currentGameId();
        {
            const bool dontStart = isBroadcastingBlocked(mCurrentBroadcastConnection);
            if (dontStart)
            {
                // disable the auto broadcast timer until a new game is launched
                mAutobroadcastTimer.stop();
                emit broadcastPermitted(callOrigin);
                return false;
            }
        }

        if (isBroadcastTokenValid())
        {
            // we already have a token, just send it to IGO
            emit broadcastDialogOpen(callOrigin);
            return true;
        }
        // login window!
        else
        {
            // the token is a cloud setting, if that fails, show an error and do not continue the flow!
            if (Services::SettingsManager::instance()->areServerUserSettingsLoaded())
            {
                emit broadcastIntroOpen(callOrigin);
                return true;
            }
            else
            {
                // cloud settings failed, so we have no broadcast token :(
                emit broadcastNetworkError(callOrigin);
                ORIGIN_LOG_ERROR << "Twitch error: " << "BroadcastServerFailure" << " : " << OriginIGO::TwitchManager::BROADCAST_CLOUD_ERROR_NO_TOKEN;
                const QString setting_str = createBroadcastSettingsStringForTelemetry();
                GetTelemetryInterface()->Metric_BROADCAST_STREAM_ERROR(currentBroadcastedProductId().toUtf8().data(), "ServerFailure", QString::number(OriginIGO::TwitchManager::BROADCAST_CLOUD_ERROR_NO_TOKEN).toUtf8().data(), setting_str.toUtf8().data());
                return false;
            }
        }
    }

    void IGOTwitch::setAutobroadcastState(bool state)
    {
        // we hit stop, so stop autobroadcasting until a new game launches
        if (state == false)
            mAutobroadcastTimer.stop();
    }

    void IGOTwitch::toggleBroadcasting()
    {
        toggleBroadcasting(Origin::Engine::IGOController::instance()->twitchStartedFrom());
    }

    void IGOTwitch::toggleBroadcasting(Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
    {
        if (!mIsBroadcastStartPending)
        {
            IGO_ASSERT(mCurrentBroadcastConnection != 0);

            {
                // don't broadcast unreleased or black listed games!!!
                IGOGameTracker::GameInfo info = mGameTracker->gameInfo(mCurrentBroadcastConnection);
                bool dontStart = false;

                if (info.isValid() && info.mBase.mIsUnreleased == true)
                    dontStart = true;

                if (dontStart)
                {
                    emit broadcastPermitted(callOrigin);
                    return;
                }
            }

            bool started = mIsBroadcasting;
            started = !started;

            if (started)
            {
                mBroadcastStartPendingTimer.stop();
                mBroadcastStartPendingTimer.start(20000); //20 seconds server timeout
                mIsBroadcastStartPending = true;
            }
            else
            {
                mBroadcastStartPendingTimer.stop();
                mIsBroadcastStartPending = false;
                // we hit the stop button to stop the broadcasting
                mBroadcastStoppedManually = true;
            }

            emit broadcastingStateChanged(started, mIsBroadcastStartPending);

            int resolution = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_RESOLUTION, Origin::Services::Session::SessionService::currentSession());
            int framerate = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_FRAMERATE, Origin::Services::Session::SessionService::currentSession());
            int quality = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_QUALITY, Origin::Services::Session::SessionService::currentSession());
            int bitrate = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_BITRATE, Origin::Services::Session::SessionService::currentSession());

            int showViewersNum = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_SHOW_VIEWERS_NUM, Origin::Services::Session::SessionService::currentSession());

            bool micVolumeMuted = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_MIC_MUTE, Origin::Services::Session::SessionService::currentSession());
            int micVolume = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_MIC_VOL, Origin::Services::Session::SessionService::currentSession());;

            bool gameVolumeMuted = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_GAME_MUTE, Origin::Services::Session::SessionService::currentSession());
            int gameVolume = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_GAME_VOL, Origin::Services::Session::SessionService::currentSession());;

			bool useOptEncoder = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_USE_OPT_ENCODER, Origin::Services::Session::SessionService::currentSession());;

            // yes I put microphone muted twice. It's the same as not broadcasting your microphone.
            eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgBroadcast(started, resolution, framerate, quality, bitrate, showViewersNum, !micVolumeMuted, micVolumeMuted, micVolume, gameVolumeMuted, gameVolume, useOptEncoder, mBroadcastToken.toStdString().c_str()));

            if (started == true)
            {
                emit broadcastStartPending();
                eastl::shared_ptr<IGOIPCMessage> titleMsg(IGOIPC::instance()->createMsgBroadcastDisplayName(broadcastGameName().toStdString().c_str()));
                mIPCServer->send(titleMsg, mCurrentBroadcastConnection);
                mIPCServer->send(msg, mCurrentBroadcastConnection);   // send this only to one game process, because only one can stream!!!
            }
            else
                if (started == false)   // reset our broadcasting state and just send it to all game processes, just in case one is broadcasting and we do not got that
                {
                    mIPCServer->send(msg);   // send this to all game process
                    setBroadcastStatus(started, false, "", "");
                }
                else
                {
                    ORIGIN_ASSERT(mCurrentBroadcastConnection != 0);
                }
        }
        else
        {
            emit broadcastStartStopError();
        }

    }

    bool IGOTwitch::isBroadcastingBlocked(uint32_t id /* = 0*/) const
    {
        // don't auto broadcast unreleased or blacklisted games!!!
        bool dontStart = false;

        IGOGameTracker::GameInfo info = id == 0 ? mGameTracker->currentGameInfo() : mGameTracker->gameInfo(id);
        if (info.isValid())
            if (info.mBase.mIsUnreleased == true)
                dontStart = true;

        return dontStart;
    }

    const QString IGOTwitch::broadcastGameName() const
    {
        return Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_GAMENAME, Origin::Services::Session::SessionService::currentSession());;
    }

    bool IGOTwitch::attemptBroadcastStart(Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
    {
        if ((Origin::Services::PlatformService::OSMajorVersion() > 5)/*twitch is disabled on XP*/)
        {
            bool success = true;

            // track our broadcasting game!!!
            if (!mIsBroadcasting && !mIsBroadcastStartPending)
                mCurrentBroadcastConnection = mGameTracker->currentGameId();
            {
                bool dontStart = isBroadcastingBlocked(mCurrentBroadcastConnection);

                if (dontStart)
                {
                    // disable the auto broadcast timer until a new game is launched
                    if (callOrigin != IIGOCommandController::CallOrigin_SDK)
                        mAutobroadcastTimer.stop();

                    emit broadcastPermitted(callOrigin);
                    return false;
                }
            }

            ORIGIN_ASSERT(mCurrentBroadcastConnection != 0);

            if (!mBroadcastToken.isEmpty() && mCurrentBroadcastConnection != 0)   // we have everything, just start broadcasting
            {
                if ((mIsAutoBroadcastingEnabled || (callOrigin == IIGOCommandController::CallOrigin_SDK)) && !mIsBroadcasting && !mIsBroadcastStartPending)
                    emit toggleBroadcasting(callOrigin);
                else
                    emit broadcastStartStopError();
            }
            else
                if (mBroadcastToken.isEmpty() && mCurrentBroadcastConnection != 0)    // token is missing so change the flow to login
                {
                    bool autoBroadcastEnabledAndTokenValid = mIsAutoBroadcastingEnabled && !mIsAutoBroadcastingTokenMissing;

                    if ((autoBroadcastEnabledAndTokenValid || (callOrigin == IIGOCommandController::CallOrigin_SDK)) && !mIsBroadcasting && !mIsBroadcastStartPending)
                    {
                        mIsAutoBroadcastingTokenMissing = true;

                        if ((callOrigin == IIGOCommandController::CallOrigin_SDK))
                        {
                            IGOController::instance()->EbisuShowBroadcastUI();
                        }
                        else
                        {
                            igoCommand(IIGOCommandController::CMD_SHOW_BROADCAST, callOrigin);
                        }
                    }
                }
                else
                {
                    emit broadcastStartStopError();
                }

            return success;
        }
        return false;
    }

    bool IGOTwitch::attemptBroadcastStop(Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
    {
        bool success = true;

        if (mIsBroadcasting && !mIsBroadcastStartPending)
        {
            setAutobroadcastState(false);
            emit toggleBroadcasting(callOrigin);
        }
        else
        {
            success = false;
            emit broadcastStartStopError();
        }

        return success;
    }

    bool IGOTwitch::disconnectAccount()
    {
        //TODO clear our browser twitch.tv cookie
        // TODO: Only delete twitch.tv cookies, not all!
        Services::NetworkAccessManagerFactory::instance()->sharedCookieJar()->clear();
        // clear our stored token
        Services::writeSetting(Services::SETTING_BROADCAST_TOKEN, "", Services::Session::SessionService::currentSession());
        // reset archiving - default is true
        Services::writeSetting(Services::SETTING_BROADCAST_SAVE_VIDEO_URL, "", Services::Session::SessionService::currentSession());
        Services::writeSetting(Services::SETTING_BROADCAST_SAVE_VIDEO, true, Services::Session::SessionService::currentSession());
        //Services::writeSetting(Services::SETTING_BROADCAST_CHANNEL_URL, "", Services::Session::SessionService::currentSession());
        mBroadcastUserName = "";
        mBroadcastChannel = "";
        emit broadcastIntroOpen(Engine::IGOController::instance()->twitchStartedFrom());
        return true;
    }

    void IGOTwitch::storeTwitchLogin(const QString& token, Origin::Engine::IIGOCommandController::CallOrigin callOrigin)
    {
        QString tokenCopy = token;
        if(tokenCopy.count() > 0)
        {
            triggerBroadcastAccountLinkedTelemetry();

            const QString tokenParam("access_token=");       
            int endPos = tokenCopy.indexOf(tokenParam);
            if (endPos > 0)
            {
                tokenCopy = tokenCopy.mid(endPos + tokenParam.size(), tokenCopy.size());
                endPos = tokenCopy.indexOf("&");
                if (endPos > 0)
                    tokenCopy = tokenCopy.mid(0, endPos);
            }

            if(tokenCopy.count() > 0)
            {
                mPendingCallOrigin = callOrigin;
                Services::writeSetting(Services::SETTING_BROADCAST_TOKEN, tokenCopy, Services::Session::SessionService::currentSession());
                mPendingCallOrigin = IIGOCommandController::CallOrigin_UNDEFINED;
            }
        }
        attemptDisplayWindow(callOrigin);
    }

    void IGOTwitch::injectBroadcastError(int errorCategory, int errorCode)
    {
        // use a signal, because we are in another thread!
        emit broadcastErrorOccurred(errorCategory, errorCode);
    }

    QString IGOTwitch::createBroadcastSettingsStringForTelemetry()
    {
        // create settings string to pass into telemetry (see TwitchManager::SetBroadcastSettings() for corresponding resolution/framerate/quality values)
        int resolution = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_RESOLUTION, Origin::Services::Session::SessionService::currentSession());
        int framerate = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_FRAMERATE, Origin::Services::Session::SessionService::currentSession());
        int quality = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_QUALITY, Origin::Services::Session::SessionService::currentSession());

        bool useOptEncoder = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_USE_OPT_ENCODER, Origin::Services::Session::SessionService::currentSession());;

        QString settings_str = QString("settings: r:%0 f:%1 q:%2 enc:%3").arg(resolution).arg(framerate).arg(quality).arg((mBroadcastOptEncoderAvailable && useOptEncoder) ? "opt" : "sdk");

        return settings_str;
    }

    void IGOTwitch::setBroadcastError(int errorCategory, int errorCode)
    {
        bool logError = true;

        // don't log the errors for killed/crashed/exited game processes
        // we capture it here, because "emit broadcastStatusChanged" will reset our mCurrentBroadcastConnection variable!!!
        if (errorCode == OriginIGO::TwitchManager::BROADCAST_REQUEST_TIMEOUT && mCurrentBroadcastConnection != 0 /*&& !isGameProcessStillRunning(mCurrentBroadcastConnection)*/)
            logError = false;


        // abort broadcasting and auto broadcasting
        mBroadcastStartPendingTimer.stop();
        setAutobroadcastState(false);
        eastl::shared_ptr<IGOIPCMessage> msg(IGOIPC::instance()->createMsgBroadcast(false));
        mIPCServer->send(msg);
        emit broadcastStatusChanged(false, false, "", "");
        emit broadcastErrorOccurred_forSDK();    // let Origin SDK know an error occurred

        // don't log or show this error, if we time out while a user is registering to twitch!!!
        if (mIsAutoBroadcastingTokenMissing == true && errorCode == OriginIGO::TwitchManager::BROADCAST_REQUEST_TIMEOUT)
            return;

        // don't make IGO visible, it steals focus from the game which is annoying for the gamer
        QString settings_str = createBroadcastSettingsStringForTelemetry();

        // Handle the offline error message separate from the Twitch errors
        if (Origin::Services::Connection::ConnectionStatesService::isUserOnline(Origin::Engine::LoginController::instance()->currentUser()->getSession()) == false)
        {
            ORIGIN_LOG_ERROR << "Twitch error: " << "Offline" << " : " << errorCode;
            GetTelemetryInterface()->Metric_BROADCAST_STREAM_ERROR(mBroadcastedProductId.toUtf8().data(), "Offline", QString::number(errorCode).toUtf8().data(), settings_str.toUtf8().data());
            IGOController::instance()->showError(tr("origin_client_must_be_online_to_broadcast_caps"), tr("origin_client_must_be_online_to_broadcast_text"), tr("ebisu_client_single_login_go_online"), this, SIGNAL(requestGoOnline()));
        }
        else
        {
            // TwitchManager::ERROR_CATEGORY
            // errorCode is usually a TTV_ErrorCode
            // except for "BroadcastGameSettingsFailure" where it can be a glError code or a HRESULT of a directx API
            if (logError)
            {
                switch (errorCategory)
                {
                case OriginIGO::TwitchManager::ERROR_CATEGORY_SERVER:
                    ORIGIN_LOG_ERROR << "Twitch error: " << "BroadcastServerFailure" << " : " << errorCode;
                    GetTelemetryInterface()->Metric_BROADCAST_STREAM_ERROR(mBroadcastedProductId.toUtf8().data(), "ServerFailure", QString::number(errorCode).toUtf8().data(), settings_str.toUtf8().data());
                    IGOController::instance()->showError(tr("origin_client_unable_to_broadcast_caps"), tr("origin_client_unable_to_broadcast_text"), tr("ebisu_client_retry"), this, SLOT(toggleBroadcasting()));
                    break;

                case OriginIGO::TwitchManager::ERROR_CATEGORY_CAPTURE_SETTINGS:
                    ORIGIN_LOG_ERROR << "Twitch error: " << "BroadcastCaptureSettingsFailure" << " : " << errorCode;
                    GetTelemetryInterface()->Metric_BROADCAST_STREAM_ERROR(mBroadcastedProductId.toUtf8().data(), "CaptureSettingsFailure", QString::number(errorCode).toUtf8().data(), settings_str.toUtf8().data());
                    IGOController::instance()->showError(tr("ebisu_client_igo_video_broadcast"), tr("ebisu_client_broadcast_capture_failure"));
                    break;

                case OriginIGO::TwitchManager::ERROR_CATEGORY_CREDENTIALS:
                    ORIGIN_LOG_ERROR << "Twitch error: " << "BroadcastTokenFailure" << " : " << errorCode;
                    GetTelemetryInterface()->Metric_BROADCAST_STREAM_ERROR(mBroadcastedProductId.toUtf8().data(), "TokenFailure", QString::number(errorCode).toUtf8().data(), settings_str.toUtf8().data());
                    IGOController::instance()->showError(tr("ebisu_client_igo_video_broadcast"), tr("ebisu_client_twitch_auth_failure"));
                    break;

                case OriginIGO::TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS:
                    ORIGIN_LOG_ERROR << "Twitch error: " << "BroadcastGameSettingsFailure" << " : " << errorCode;
                    GetTelemetryInterface()->Metric_BROADCAST_STREAM_ERROR(mBroadcastedProductId.toUtf8().data(), "GameSettingsFailure", QString::number(errorCode).toUtf8().data(), settings_str.toUtf8().data());
                    IGOController::instance()->showError(tr("ebisu_client_igo_video_broadcast"), tr("ebisu_client_broadcast_game_failure"));
                    break;

                case OriginIGO::TwitchManager::ERROR_CATEGORY_UNSUPPORTED:
                    ORIGIN_LOG_ERROR << "Twitch error: " << "BroadcastIsNotSupported" << " : " << errorCode;
                    GetTelemetryInterface()->Metric_BROADCAST_STREAM_ERROR(mBroadcastedProductId.toUtf8().data(), "IsNotSupported", QString::number(errorCode).toUtf8().data(), settings_str.toUtf8().data());
                    IGOController::instance()->showError(tr("ebisu_client_igo_video_broadcast"), tr("ebisu_client_broadcast_not_supported"));
                    break;

                case OriginIGO::TwitchManager::ERROR_CATEGORY_SERVER_BANDWIDTH:
                    ORIGIN_LOG_ERROR << "Twitch error: " << "BroadcastServerBandwidthFailure" << " : " << errorCode;
                    GetTelemetryInterface()->Metric_BROADCAST_STREAM_ERROR(mBroadcastedProductId.toUtf8().data(), "ServerBandwidthFailure", QString::number(errorCode).toUtf8().data(), settings_str.toUtf8().data());
                    IGOController::instance()->showError(tr("ebisu_client_igo_broadcast_bandwidth_caps"), tr("ebisu_client_igo_broadcast_bandwidth_text"));
                    break;

                }
            }
        }
    }

    void IGOTwitch::injectBroadcastStreamInfo(int viewers)
    {
        // use a signal, because we are in another thread!
        emit broadcastStreamInfoChanged(viewers);
    }

    void IGOTwitch::injectBroadcastUserName(const QString &userName)
    {
        // use a signal, because we are in another thread!
        emit broadcastUserNameChanged(userName);
    }

    void IGOTwitch::injectBroadcastStatus(bool status, bool archivingEnabled, const QString &fixArchivingURL, const QString &channelURL)
    {
        // use a signal, because we are in another thread!
        emit broadcastStatusChanged(status, archivingEnabled, fixArchivingURL, channelURL);
    }

	void IGOTwitch::injectBroadcastOptEncoderAvailable(bool available)
	{
		// use a signal, because we are in another thread!
		emit broadcastOptEncoderAvailable(available);
	}

    void IGOTwitch::setBroadcastStatus(bool status, bool archivingEnabled, const QString & fixArchivingURL, const QString & channelURL)
    {
        mBroadcastStartPendingTimer.stop();
        mIsBroadcastStartPending = false;   // received a start or a stop from the game, so nothing is pending anymore
        mIsAutoBroadcastingTokenMissing = false;

        IGOGameTracker::GameInfo info = mGameTracker->gameInfo(mCurrentBroadcastConnection);
        ORIGIN_ASSERT(info.isValid());

        // broadcast telemetry
        // start event
        if (mBroadcastingStatsCleared == false && mIsBroadcasting == false && status == true)
        {
            mBroadcastingStatsCleared = true;

            const QString productId = info.mBase.mProductID;
            updateBroadcastGameName();
            setBroadcastUserName(info.mBroadcastingChannel);

            ORIGIN_ASSERT(!mBroadcastUserName.isEmpty());
            ORIGIN_ASSERT(!channelURL.isEmpty());
            
            // If an error occurs, save product ID to send with telemetry
            QString settings_str = createBroadcastSettingsStringForTelemetry();
            mBroadcastedProductId = productId;
            GetTelemetryInterface()->Metric_BROADCAST_STREAM_START(productId.toUtf8().data(), mBroadcastUserName.toUtf8().data(), info.mBroadcastingStreamId, mBroadcastInitiatedFromSDK, settings_str.toUtf8().data());

            Origin::Services::writeSetting(Origin::Services::SETTING_BROADCAST_CHANNEL_URL, channelURL, Origin::Services::Session::SessionService::currentSession());

            if (!archivingEnabled)
            {
                Origin::Services::writeSetting(Origin::Services::SETTING_BROADCAST_SAVE_VIDEO_URL, fixArchivingURL, Origin::Services::Session::SessionService::currentSession());
            }
            else
            {
                // kill the URL, if archiving is enabled!!!
                Origin::Services::writeSetting(Origin::Services::SETTING_BROADCAST_SAVE_VIDEO_URL, "", Origin::Services::Session::SessionService::currentSession());
            }

            // simulate toggle
            Origin::Services::writeSetting(Origin::Services::SETTING_BROADCAST_SAVE_VIDEO, !archivingEnabled, Origin::Services::Session::SessionService::currentSession());
            Origin::Services::writeSetting(Origin::Services::SETTING_BROADCAST_SAVE_VIDEO, archivingEnabled, Origin::Services::Session::SessionService::currentSession());

            // re-enable our auto broadcast timer, in case it was disabled by an error
            if (!mAutobroadcastTimer.isActive())
                mAutobroadcastTimer.start(5000);   // check every 5 seconds to start broadcasting....

            emit(broadcastStarted(channelURL));

        }
        else
            // stop event
            if (mIsBroadcasting == true && status == false)
            {
                bool isBroadcasting = false;
                QString broadcastingChannel;
                uint64_t broadcastingStreamId = 0;
                uint32_t minBroadcastingViewers = 0;
                uint32_t maxBroadcastingViewers = 0;

                IGOGameTracker::GameInfo info = mGameTracker->gameInfo(mCurrentBroadcastConnection);
                ORIGIN_ASSERT(info.isValid());
                if (mBroadcastingStatsCleared == false)    // did we ever receive streaming stats from the game?
                {
                    // Check streaming end stats from the game
                    isBroadcasting = info.mIsBroadcasting;
                    broadcastingChannel = info.mBroadcastingChannel;
                    broadcastingStreamId = info.mBroadcastingStreamId;
                    minBroadcastingViewers = info.mMinBroadcastingViewers;
                    maxBroadcastingViewers = info.mMaxBroadcastingViewers;
                }

                QString productId = info.mBase.mProductID;
                ORIGIN_ASSERT(!broadcastingChannel.isEmpty());
                GetTelemetryInterface()->Metric_BROADCAST_STREAM_STOP(productId.toUtf8().data(),
                    broadcastingChannel.toUtf8().data(),
                    broadcastingStreamId,
                    minBroadcastingViewers,
                    maxBroadcastingViewers,
                    static_cast<unsigned int>(mBroadcastDuration));
                mGameTracker->resetBroadcastStats(mCurrentBroadcastConnection);
                mCurrentBroadcastConnection = 0;
                mBroadcastingStatsCleared = false;

                if (!mBroadcastStoppedManually)
                {
                    emit broadcastStoppedError();
                }
                mBroadcastStoppedManually = false;

                emit broadcastStopped();
            }
            else
                if (status == false)    // no broadcast was running, just reset our internal states
                {
                    mGameTracker->resetBroadcastStats(mCurrentBroadcastConnection);
                    mCurrentBroadcastConnection = 0;
                    mBroadcastingStatsCleared = false;
                }

        mIsBroadcasting = status;

        emit broadcastingStateChanged(mIsBroadcasting, mIsBroadcastStartPending);

        if (mIsBroadcasting)
        {
            bool showViewersNum = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_SHOW_VIEWERS_NUM, Origin::Services::Session::SessionService::currentSession());
            emit broadcastIndicatorChanged(showViewersNum);

            mBroadcastDuration = 0;
            mBroadcastDurationTimer.start(1000);
        }
        else
        {
            mBroadcastDuration = 0;
            mBroadcastDurationTimer.stop();
            emit broadcastIndicatorChanged(false);
        }

        // update the game name, because we might have switched apps while stopping the broadcast
        updateBroadcastGameName();
    }

    void IGOTwitch::setBroadcastUserName(const QString &userName)
    {
        mBroadcastUserName = userName;
    }

    void IGOTwitch::setBroadcastStreamInfo(int viewers)
    {
        mBroadcastViewers = viewers;

        if (mBroadcastingStatsCleared == true && mIsBroadcasting == true)
        {
            mBroadcastingStatsCleared = false;
        }
    }

	void IGOTwitch::setBroadcastOptEncoderAvailable(bool available)
	{
		mBroadcastOptEncoderAvailable = available;
	}

    void IGOTwitch::updateBroadcastGameName()
    {
        if (!mIsBroadcasting && !mIsBroadcastStartPending)   // update our game name & broadcast connection, if no broadcast has been initiated yet
            // and if we have a valid connection!!!
        {
            const QString currentGameName = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_GAMENAME, Origin::Services::Session::SessionService::currentSession());
            QString gameName = "";
            mCurrentBroadcastConnection = mGameTracker->currentGameId();

            const IGOGameTracker::GameInfo info = mGameTracker->gameInfo(mCurrentBroadcastConnection);
            gameName = (info.isValid() && info.mBase.mIsUnreleased) ? tr("ebisu_client_notranslate_embargomode_title") : gameName;

            if (mCurrentBroadcastConnection != 0)
            {
                if (gameName.isEmpty() && info.isValid())
                {
                    gameName = info.mBase.mMasterTitle;

                    //if the masterTitle is empty, probably for NOG's use the titel
                    if (gameName.isEmpty())
                        gameName = info.mBase.mTitle;
                }

                mGameTracker->setMasterTitleOverride(mCurrentBroadcastConnection, gameName);
            }

            if(currentGameName.compare(gameName) != 0)
            {
                Origin::Services::writeSetting(Origin::Services::SETTING_BROADCAST_GAMENAME, gameName, Origin::Services::Session::SessionService::currentSession());
                emit broadcastGameNameChanged(gameName);           
            }
        }
    }


    void IGOTwitch::updateBroadcastDuration()
    {
        mBroadcastDuration++;

        emit broadcastDurationChanged(mBroadcastDuration);
    }


    void IGOTwitch::stopPendingBroadcast()
    {
        setBroadcastError(1, OriginIGO::TwitchManager::BROADCAST_REQUEST_TIMEOUT/*artificial error - timeout!!!*/);
    }

    void IGOTwitch::initiateAutobroadcast()
    {
        // The timer might trip just after logging out.
        Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();
        bool underAge = user && user->isUnderAge();
        if (Origin::Engine::IGOController::instance()->igowm()->isScreenLargeEnough() && Origin::Services::Session::SessionService::hasValidSession() && Origin::Engine::IGOController::instance()->isGameInForeground() && !underAge && (Origin::Services::PlatformService::OSMajorVersion()  > 5)/*twitch is disabled on XP*/)
        {
            mIsAutoBroadcastingEnabled = Origin::Services::readSetting(Origin::Services::SETTING_BROADCAST_AUTOSTART, Origin::Services::Session::SessionService::currentSession());
            // show the broadcast window once we completed registration/login
            if (mBroadcastingStatsCleared == false && mIsBroadcasting == false && mIsAutoBroadcastingEnabled == true)
            {
                igoCommand(IIGOCommandController::CMD_START_AUTOBROADCAST, IIGOCommandController::CallOrigin_CLIENT);
            }
        }
    }

    void IGOTwitch::triggerBroadcastAccountLinkedTelemetry()
    {
        GetTelemetryInterface()->Metric_BROADCAST_ACCOUNT_LINKED(mBroadcastInitiatedFromSDK);
    }


    void IGOTwitch::settingChanged(const QString& settingKey, const Origin::Services::Variant& value)
    {
        if (settingKey == Origin::Services::SETTING_BROADCAST_SHOW_VIEWERS_NUM.name())
        {
            emit broadcastIndicatorChanged(value);
        }
        else if (settingKey == Origin::Services::SETTING_BROADCAST_TOKEN.name())
        {
            if (value.toString().isEmpty())
            {
                mBroadcastUserName.clear();
                mBroadcastToken.clear();
                mBroadcastChannel.clear();
                // EBIBUGS-28425: Don't actually clear the game name. Just emit-out that the names should be cleared. 
                emit broadcastGameNameChanged("");
                emit broadcastChannelChanged(mBroadcastChannel);
                emit broadcastAccountDisconnected();
            }
            else
            {

                mBroadcastToken = value.toString();

                if ((Origin::Services::PlatformService::OSMajorVersion() > 5)/*twitch is disabled on XP*/)
                {
                    HMODULE hIGO = LoadIGODll();
                    if (hIGO)
                    {
                        typedef bool(*GetTwitchBroadcastChannelURLFunc)(const char* authToken, wchar_t **channelName);
                        GetTwitchBroadcastChannelURLFunc GetTwitchBroadcastChannelURL = (GetTwitchBroadcastChannelURLFunc)GetProcAddress(hIGO, "GetTwitchBroadcastChannelURL");
                        if (GetTwitchBroadcastChannelURL)
                        {
                            wchar_t *channelName = NULL;
                            // this call can take some seconds, if that becomes an issue, make GetTwitchBroadcastChannelURL async...
                            if (GetTwitchBroadcastChannelURL(mBroadcastToken.toStdString().c_str(), &channelName))
                            {
                                mBroadcastChannel = QString::fromWCharArray(channelName);
                                mBroadcastUserName = mBroadcastChannel.split("/").last();
                                emit broadcastChannelChanged(mBroadcastChannel);
                            }
                        }
                    }
                }
                updateBroadcastGameName();

                // show the broadcast window once we completed registration/login
                if (!mIsBroadcasting && Origin::Engine::IGOController::instance()->isGameInForeground())
                {
                    IIGOCommandController::CallOrigin callOrigin = Engine::IGOController::instance()->twitchStartedFrom();
                    if (mPendingCallOrigin != IIGOCommandController::CallOrigin_UNDEFINED)
                        callOrigin = mPendingCallOrigin;

                    igoCommand(IIGOCommandController::CMD_SHOW_BROADCAST, callOrigin);
                }
            }
        }
    }
    
} // Engine
} // Origin

#endif // ORIGIN_PC
// Copyright (C) 2013 Electronic Arts. All rights reserved.

#include "engine/voice/VoiceClient.h"
#include "engine/social/SocialController.h"

#include "services/config/OriginConfigService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/InGameHotKey.h"
#include "services/settings/SettingsManager.h"

#include "chat/BlockList.h"
#include "chat/ConnectedUser.h"
#include "chat/OriginConnection.h"

#include "TelemetryAPIDLL.h"

#include <math.h>
#include <cmath>

#if ENABLE_VOICE_CHAT

#include <QMutexLocker>
#include <QStringList>

#include <SonarClient/Client.h>
#include <SonarCommon/Common.h>

#if !defined(ENABLE_VOICECLIENT_TRACING)
#define ENABLE_VOICECLIENT_TRACING 0
#endif

namespace
{
    QMutex mVoiceMutex(QMutex::Recursive);
    const float SONAR_MAX_CAPTURE_GAIN = 8.0f;
    const float SONAR_MAX_PLAYBACK_GAIN = 8.0f;
    const int SONAR_INACTIVITY_TIMEOUT = 3600000; // 1 hour (in milliseconds)
}

namespace Origin
{
    namespace Engine
    { 
        namespace Voice
        {
            void SonarLogger(const char* format, va_list args)
            {
                if (!format)
                    return;

                char buffer[2048];
                int numChars = vsnprintf(buffer, sizeof(buffer), format, args);

                if (numChars > 0 && numChars < sizeof(buffer))
                    ORIGIN_LOG_EVENT << buffer;
                else if (numChars >= sizeof(buffer))
                    ORIGIN_LOG_EVENT << "** Sonar logging buffer overflow";
            }

            inline float scaleVoiceSensitivity(float from)
            {
                 if (from <= 0.0f)
                     return 0.0f;
                 else if (from >= 100.0f)
                     return 100.0f;
                float t = log10(1 + from) * 49.5;
                return t;
            }

            void SonarTelemetry(sonar::TelemetryType stt, va_list vl)
            {
                switch (stt)
                {
                    case sonar::SONAR_TELEMETRY_CLIENT_CONNECTED: {
                        char* channelId = va_arg(vl, char*);
                        char* inputDeviceId = va_arg(vl, char*);
                        char inputDeviceAGC = va_arg(vl, char);
                        short inputDeviceGain = va_arg(vl, short);
                        short inputDeviceThreshold = va_arg(vl, short);
                        char* outputDeviceId = va_arg(vl, char*);
                        short outputDeviceGain = va_arg(vl, short);
						uint32_t voiceServerIP = va_arg(vl, uint32_t);
                        char* captureMode = va_arg(vl, char*);
                        GetTelemetryInterface()->Metric_SONAR_CLIENT_CONNECTED(
                            channelId,
                            inputDeviceId,
                            inputDeviceAGC,
                            inputDeviceGain,
                            inputDeviceThreshold,
                            outputDeviceId,
                            outputDeviceGain,
							voiceServerIP,
                            captureMode);
                        break;
                    }

                    case sonar::SONAR_TELEMETRY_CLIENT_DISCONNECTED: {
                        char* channelId = va_arg(vl, char*);
                        char* reason = va_arg(vl, char*);
                        char* reasonDesc = va_arg(vl, char*);
                        unsigned int messagesSent = va_arg(vl, unsigned int);
                        unsigned int messagesReceived = va_arg(vl, unsigned int);
                        unsigned int messagesLost = va_arg(vl, unsigned int);
                        unsigned int messagesOutOfSequence = va_arg(vl, unsigned int);
                        unsigned int badCid = va_arg(vl, unsigned int);
                        unsigned int messagesTruncated = va_arg(vl, unsigned int);
                        unsigned int audioFramesClippedInMixer = va_arg(vl, unsigned int);
                        unsigned int socketSendError = va_arg(vl, unsigned int);
                        unsigned int socketReceiveError = va_arg(vl, unsigned int);
                        unsigned int receiveToPlaybackLatencyMean = va_arg(vl, unsigned int);
                        unsigned int receiveToPlaybackLatencyMax = va_arg(vl, unsigned int);
                        unsigned int playbackToPlaybackLatencyMean = va_arg(vl, unsigned int);
                        unsigned int playbackToPlaybackLatencyMax = va_arg(vl, unsigned int);
                        int jitterArrivalMean = va_arg(vl, int);
                        int jitterArrivalMax = va_arg(vl, int);
                        int jitterPlaybackMean = va_arg(vl, int);
                        int jitterPlaybackMax = va_arg(vl, int);
                        unsigned int playbackUnderrun = va_arg(vl, unsigned int);
                        unsigned int playbackOverflow = va_arg(vl, unsigned int);
                        unsigned int playbackDeviceLost = va_arg(vl, unsigned int);
                        unsigned int dropNotConnected = va_arg(vl, unsigned int);
                        unsigned int dropReadServerFrameCounter = va_arg(vl, unsigned int);
                        unsigned int dropReadTakeNumber = va_arg(vl, unsigned int);
                        unsigned int dropTakeStopped = va_arg(vl, unsigned int);
                        unsigned int dropReadCid = va_arg(vl, unsigned int);
                        unsigned int dropNullPayload = va_arg(vl, unsigned int);
                        unsigned int dropReadClientFrameCounter = va_arg(vl, unsigned int);
                        unsigned int jitterbufferUnderrun = va_arg(vl, unsigned int);
                        unsigned int networkLoss = va_arg(vl, unsigned int);
                        unsigned int networkJitter = va_arg(vl, unsigned int);
                        unsigned int networkPing = va_arg(vl, unsigned int);
                        unsigned int networkReceived = va_arg(vl, unsigned int);
						unsigned int networkQuality = va_arg(vl, unsigned int);
						uint32_t voiceServerIP = va_arg(vl, uint32_t);
                        char* captureMode = va_arg(vl, char*);
                        int jitterbufferOccupancyMean = va_arg(vl, int);
                        int jitterbufferOccupancyMax = va_arg(vl, int);
                        GetTelemetryInterface()->Metric_SONAR_CLIENT_DISCONNECTED(
                            channelId,
                            reason,
                            reasonDesc,
                            messagesSent,
                            messagesReceived,
                            messagesLost,
                            messagesOutOfSequence,
                            badCid,
                            messagesTruncated,
                            audioFramesClippedInMixer,
                            socketSendError,
                            socketReceiveError,
                            receiveToPlaybackLatencyMean,
                            receiveToPlaybackLatencyMax,
                            playbackToPlaybackLatencyMean,
                            playbackToPlaybackLatencyMax,
                            jitterArrivalMean,
                            jitterArrivalMax,
                            jitterPlaybackMean,
                            jitterPlaybackMax,
                            playbackUnderrun,
                            playbackOverflow,
                            playbackDeviceLost,
                            dropNotConnected,
                            dropReadServerFrameCounter,
                            dropReadTakeNumber,
                            dropTakeStopped,
                            dropReadCid,
                            dropNullPayload,
                            dropReadClientFrameCounter,
                            jitterbufferUnderrun,
                            networkLoss,
                            networkJitter,
                            networkPing,
                            networkReceived,
                            networkQuality,
							voiceServerIP,
                            captureMode,
                            jitterbufferOccupancyMean,
                            jitterbufferOccupancyMax
                        );
                        break;
                    }

                    case sonar::SONAR_TELEMETRY_ERROR: {
                        char* category = va_arg(vl, char*);
                        char* message = va_arg(vl, char*);
                        unsigned int error = va_arg(vl, unsigned int);
                        GetTelemetryInterface()->Metric_SONAR_ERROR(category, message, error);
                        break;
                    }

                    case sonar::SONAR_TELEMETRY_CLIENT_STATS: {
                        char* channelId = va_arg(vl, char*);
                        uint32_t networkLatencyMin = va_arg(vl, uint32_t);
                        uint32_t networkLatencyMax = va_arg(vl, uint32_t);
                        uint32_t networkLatencyMean = va_arg(vl, uint32_t);
                        uint32_t bytesSent = va_arg(vl, uint32_t);
                        uint32_t bytesReceived = va_arg(vl, uint32_t);
                        uint32_t messagesLost = va_arg(vl, uint32_t);
                        uint32_t messagesOutOfSequence = va_arg(vl, uint32_t);
                        uint32_t messagesReceivedJitterMin = va_arg(vl, uint32_t);
                        uint32_t messagesReceivedJitterMax = va_arg(vl, uint32_t);
                        uint32_t messagesReceivedJitterMean = va_arg(vl, uint32_t);
                        uint32_t cpuLoad = va_arg(vl, uint32_t);

                        GetTelemetryInterface()->Metric_SONAR_CLIENT_STATS(
                            channelId,
                            networkLatencyMin,
                            networkLatencyMax,
                            networkLatencyMean,
                            bytesSent,
                            bytesReceived,
                            messagesLost,
                            messagesOutOfSequence,
                            messagesReceivedJitterMin,
                            messagesReceivedJitterMax,
                            messagesReceivedJitterMean,
                            cpuLoad
                            );
                        break;
                    }

                    case sonar::SONAR_TELEMETRY_STAT:
                    default: {
                        // TBD:
                        break;
                    }
                }
            }

            void VoiceUpdateWorker::run()
            {
                sonar::INT64 baseClock = 0;

                while (!mFinished)
                {
                    //sonar::common::sleepHalfFrame();
                    //sonar::common::sleepFrame();
                    sonar::common::sleepFrameDriftAdjusted(baseClock);
                    //sonar::common::sleepExact(19);

                    {
                        // Locking may fail in some occasions, but we want to be sure the thread quits.
                        if(mVoiceMutex.tryLock(1000))
                        {
                            mClient->update();
                            mVoiceMutex.unlock();
                        }
                    }
                }

                QThread::currentThread()->quit();
            }

            VoiceClient::VoiceClient(UserRef user)
                : QObject(NULL)
                , mUser(user)
                , mVoiceClientOptions(user)
                , mClient(NULL)
                , mVoiceUpdateThread(NULL)
                , mVoiceUpdateWorker(NULL)
                , mBlockedListCache()
                , mEmptyConversationTimer()
                , mBlockListHasChanged(false)
                , mIsConnectedToSettings(false)
                , mIsPTTActive(false)
            {
                sonar::common::RegisterLogCallback(&SonarLogger);
                sonar::common::RegisterTelemetryCallback(&SonarTelemetry);

                // grab the jitter buffer size from the server config, override with local setting
                int jitterBufferSize = Services::OriginConfigService::instance()->miscConfig().voipJitterBufferSize;
                if (Services::SettingsManager::instance()->isSettingSpecified(Origin::Services::SETTING_SonarJitterBufferSize)) {
                    jitterBufferSize = Origin::Services::readSetting(Origin::Services::SETTING_SonarJitterBufferSize);
                }

                int networkQualityPingStartupDuration = Services::OriginConfigService::instance()->miscConfig().voipNetworkQualityPingStartupDuration;
                if (Services::SettingsManager::instance()->isSettingSpecified(Origin::Services::SETTING_SonarNetworkQualityPingStartupDuration)) {
                    networkQualityPingStartupDuration = Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkQualityPingStartupDuration);
                }

                int networkQualityPingShortInterval = Services::OriginConfigService::instance()->miscConfig().voipNetworkQualityPingShortInterval;
                if (Services::SettingsManager::instance()->isSettingSpecified(Origin::Services::SETTING_SonarNetworkQualityPingShortInterval)) {
                    networkQualityPingShortInterval = Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkQualityPingShortInterval);
                }

                int networkQualityPingLongInterval = Services::OriginConfigService::instance()->miscConfig().voipNetworkQualityPingLongInterval;
                if (Services::SettingsManager::instance()->isSettingSpecified(Origin::Services::SETTING_SonarNetworkQualityPingLongInterval)) {
                    networkQualityPingLongInterval = Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkQualityPingLongInterval);
                }

                int clientPingInterval = Origin::Services::readSetting(Origin::Services::SETTING_SonarClientPingInterval);
                int clientTickInterval = Origin::Services::readSetting(Origin::Services::SETTING_SonarClientTickInterval);
                int registerTimeout = Origin::Services::readSetting(Origin::Services::SETTING_SonarRegisterTimeout);
                int registerMaxRetries = Origin::Services::readSetting(Origin::Services::SETTING_SonarRegisterMaxRetries);
                int networkGoodLoss = Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkGoodLoss);
                int networkGoodJitter = Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkGoodJitter);
                int networkGoodPing = Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkGoodPing);
                int networkOkLoss = Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkOkLoss);
                int networkOkJitter = Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkOkJitter);
                int networkOkPing = Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkOkPing);
                int networkPoorLoss = Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkPoorLoss);
                int networkPoorJitter = Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkPoorJitter);
                int networkPoorPing = Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkPoorPing);
                sonar::ConnectionOptions connectionOptions(clientPingInterval, clientTickInterval, jitterBufferSize, registerTimeout, registerMaxRetries,
                    networkGoodLoss, networkGoodJitter, networkGoodPing, networkOkLoss, networkOkJitter, networkOkPing, networkPoorLoss, networkPoorJitter, networkPoorPing,
                    networkQualityPingStartupDuration, networkQualityPingShortInterval, networkQualityPingLongInterval);
                mClient = new sonar::Client(&mVoiceClientOptions, connectionOptions, this);

                ORIGIN_VERIFY_CONNECT_EX(
                    this, &VoiceClient::voiceConnected,
                    this, &VoiceClient::onVoiceConnected,
                    Qt::QueuedConnection);

                ORIGIN_VERIFY_CONNECT_EX(
                    this, &VoiceClient::voiceDisconnected,
                    this, &VoiceClient::onVoiceDisconnected,
                    Qt::QueuedConnection);

                ORIGIN_VERIFY_CONNECT_EX(
                    this, &VoiceClient::voiceUserJoined,
                    this, &VoiceClient::onVoiceUserJoined,
                    Qt::QueuedConnection);

                ORIGIN_VERIFY_CONNECT_EX(
                    this, &VoiceClient::voiceUserLeft,
                    this, &VoiceClient::onVoiceUserLeft,
                    Qt::QueuedConnection);

                mEmptyConversationTimer.setSingleShot(true);
                mEmptyConversationTimer.setTimerType(Qt::VeryCoarseTimer);
                ORIGIN_VERIFY_CONNECT(&mEmptyConversationTimer, SIGNAL(timeout()), this, SLOT(onEmptyConversationTimeout()));
            }

            VoiceClient::~VoiceClient()
            {
                QMutexLocker locker(&mVoiceMutex);

                mEmptyConversationTimer.stop();
                ORIGIN_VERIFY_DISCONNECT(&mEmptyConversationTimer, SIGNAL(timeout()), this, SLOT(onEmptyConversationTimeout()));

                ORIGIN_VERIFY_DISCONNECT(
                    this, &VoiceClient::voiceConnected,
                    this, &VoiceClient::onVoiceConnected);

                ORIGIN_VERIFY_DISCONNECT(
                    this, &VoiceClient::voiceDisconnected,
                    this, &VoiceClient::onVoiceDisconnected);

                ORIGIN_VERIFY_DISCONNECT(
                    this, &VoiceClient::voiceUserJoined,
                    this, &VoiceClient::onVoiceUserJoined);

                ORIGIN_VERIFY_DISCONNECT(
                    this, &VoiceClient::voiceUserLeft,
                    this, &VoiceClient::onVoiceUserLeft);

                disconnect("VoiceClient", "destructing");

                delete mClient;
                mClient = NULL;
            }

            bool VoiceClient::createVoiceConnection(QString token, const VoiceSettings &settings, bool remoteEcho)
            {
                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "createVoiceConnection: t=" << QDateTime::currentMSecsSinceEpoch();

                QString voipAddressBefore = Origin::Services::readSetting(Origin::Services::SETTING_VoipAddressBefore);
                QString voipAddressAfter = Origin::Services::readSetting(Origin::Services::SETTING_VoipAddressAfter);
                if (!voipAddressAfter.isEmpty()) {
                    if (voipAddressBefore.isEmpty()) {
                        // SONAR2||54.210.105.28:30000|Origin|2374532421||da341070c61c452b9ac86985ad0269e3|||1430857260|VZE8HKX1D+lrsqZ8gcXJGaE87WKUeb1/vTzHRO2BrzbGAc0Hd7OiXlybntIZygTrMjbhSpSwzB6HBwzwrK64hQ==
                        int start = token.indexOf("|");
                        start = token.indexOf("|", start + 1) + 1;
                        int end = token.indexOf(":", start + 1);
                        voipAddressBefore = token.mid(start, end - start);
                    }
                    token.replace(voipAddressBefore, voipAddressAfter);
                }

                bool connected = connect(token, settings, remoteEcho);
                return connected;
            }

            void VoiceClient::stopVoiceConnection(const QString& reasonType, const QString& reasonDescription, bool joiningVoice)
            {
                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "VoiceClient::stopVoiceConnection: t=" << QDateTime::currentMSecsSinceEpoch();

                bool disconnected = disconnect(reasonType, reasonDescription, joiningVoice);
                ORIGIN_ASSERT(disconnected);
            }

            bool VoiceClient::isInVoice()
            {
                return isConnected();
            }

            bool VoiceClient::isConnectingToVoice()
            {
                return isConnecting();
            }

            int VoiceClient::getUsersInVoice()
            {
                return getNumClientsInChannel();
            }

            bool VoiceClient::isAudioAvailable() const
            {
                return mClient && mClient->isCaptureDeviceValid() && mClient->isPlaybackDeviceValid();
            }

            int VoiceClient::lastAudioError() const
            {
                int err = 0;

                if (mClient)
                {
                    err = mClient->lastCaptureDeviceError();
                    if (err == 0)
                        err = mClient->lastPlaybackDeviceError();
                }

                return err;
            }

            bool VoiceClient::connect(const QString& token, const VoiceSettings& settings, bool remoteEcho)
            {
                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "VoiceClient::connect: t=" << QDateTime::currentMSecsSinceEpoch();

                ORIGIN_LOG_EVENT << "voiceclient: connect " << token;

                UserRef user = mUser.toStrongRef();
                if (user)
                {
                    Origin::Engine::Social::SocialController* socialController = user->socialControllerInstance();
                    ORIGIN_ASSERT(socialController);
                    Origin::Chat::OriginConnection* connection = socialController->chatConnection();
                    ORIGIN_ASSERT(connection);
                    Origin::Chat::ConnectedUser* user = connection->currentUser();
                    ORIGIN_ASSERT(user);
                    Origin::Chat::BlockList* blockList = user->blockList();
                    ORIGIN_ASSERT(blockList);
                    mBlockedListCache = blockList->blockedIds();
                    mBlockListHasChanged = true;
                    ORIGIN_VERIFY_CONNECT(blockList, SIGNAL(updated()), this, SLOT(onBlockListUpdated()));
                }

                QMutexLocker locker(&mVoiceMutex);
                mClient->addEventListener(this);

                setCaptureMode(settings.pushToTalk, settings.autoGain, settings.voiceActivationThreshold);

                setSpeakerGain(settings.speakerGain);
                setMicrophoneGain(settings.microphoneGain);

                mClient->setRemoteEcho(remoteEcho);

                // set push to talk hotkey setting after setConfig() as it will write it back as a string
                qulonglong pushToTalkKeySetting = Services::readSetting(Services::SETTING_PushToTalkKey);
                if (pushToTalkKeySetting)
                {
                    const int kVK_KEY_STROKE_ARRAY_SIZE = 8;
                    int vkkey[kVK_KEY_STROKE_ARRAY_SIZE];
                    int size = kVK_KEY_STROKE_ARRAY_SIZE;
                    Origin::Services::InGameHotKey settingHotKeys = Origin::Services::InGameHotKey::fromULongLong(pushToTalkKeySetting);
                    settingHotKeys.getVirtualKeyArray(vkkey, size);
                    if (size == 0)
                    {
                        ORIGIN_LOG_ERROR << "Push to Talk setting for virtual key array is zero-length";
                    }
                    else
                        setPushToTalkKey(vkkey, size);
                }
                else
                {   // if zero then it is a mouse press
                    QString pushToTalkMouseString = Services::readSetting(Services::SETTING_PushToTalkMouse);
                    setPushToTalkKey(pushToTalkMouseString);
                }

                // Verify that we haven't left crap running from an earlier connect
                ORIGIN_ASSERT_MESSAGE(!mVoiceUpdateThread, "Hmm, voice update thread is lingering");
                ORIGIN_ASSERT_MESSAGE(!mVoiceUpdateWorker, "Hmm, voice update worker is lingering");

                if (mVoiceUpdateThread == NULL)
                {
                    ORIGIN_LOG_DEBUG_IF(ENABLE_VOICECLIENT_TRACING) << "*** voice client creating update thread";
                    mVoiceUpdateThread = new QThread();
                    mVoiceUpdateThread->setObjectName(QString("Origin Voice Worker(%1)").arg(objectName()));
                }

                ORIGIN_LOG_DEBUG_IF(ENABLE_VOICECLIENT_TRACING) << "*** voice client creating update worker";
                mVoiceUpdateWorker = new VoiceUpdateWorker(mClient);
                ORIGIN_ASSERT(mVoiceUpdateWorker);
                mVoiceUpdateWorker->moveToThread(mVoiceUpdateThread);
                ORIGIN_VERIFY_CONNECT(mVoiceUpdateThread, SIGNAL(finished()), mVoiceUpdateWorker, SLOT(deleteLater()));
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(startUpdate()), mVoiceUpdateWorker, SLOT(run()));
                mVoiceUpdateThread->start(QThread::HighPriority);

                mVoiceClientOptions.resetAll();

                emit (startUpdate());

                bool wasClientConnected = mClient->connect(token.toStdString());

                return wasClientConnected;
            }

            bool VoiceClient::disconnect(const QString& reasonType, const QString& reasonDescription, bool joiningVoice)
            {
                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "VoiceClient::disconnect: t=" << QDateTime::currentMSecsSinceEpoch();

                ORIGIN_LOG_EVENT << "voiceclient: disconnect " << reasonType << ", " << reasonDescription;

                UserRef user = mUser.toStrongRef();
                if (user)
                {
                    Origin::Engine::Social::SocialController* socialController = user->socialControllerInstance();
                    ORIGIN_ASSERT(socialController);
                    Origin::Chat::OriginConnection* connection = socialController->chatConnection();
                    ORIGIN_ASSERT(connection);
                    Origin::Chat::ConnectedUser* user = connection->currentUser();
                    ORIGIN_ASSERT(user);
                    Origin::Chat::BlockList* blockList = user->blockList();
                    ORIGIN_ASSERT(blockList);
                    ORIGIN_VERIFY_DISCONNECT(blockList, SIGNAL(updated()), this, SLOT(onBlockListUpdated()));
                }

                QMutexLocker locker(&mVoiceMutex);

                bool disconnected = true;
                if (mClient && !mClient->isDisconnected())
                {
                    disconnected = mClient->disconnect(reasonType.toStdString(), reasonDescription.toStdString());
                    ORIGIN_ASSERT(disconnected);
                }
                
                // Terminate update thread
                if (mVoiceUpdateWorker)
                {
                    ORIGIN_LOG_DEBUG_IF(ENABLE_VOICECLIENT_TRACING) << "*** voice client stopping update worker";

                    ORIGIN_VERIFY_DISCONNECT(this, SIGNAL(startUpdate()), mVoiceUpdateWorker, SLOT(run()));
                    mVoiceUpdateWorker->stop();
                }

                if (mVoiceUpdateThread)
                {
                    ORIGIN_LOG_DEBUG_IF(ENABLE_VOICECLIENT_TRACING) << "*** voice client waiting on update thread to terminate";

                    mVoiceUpdateThread->wait();
                    delete mVoiceUpdateThread;
                    mVoiceUpdateThread = NULL;
                    mVoiceUpdateWorker = NULL; // will be deleted on the next event loop

                    ORIGIN_LOG_DEBUG_IF(ENABLE_VOICECLIENT_TRACING) << "*** voice client deleted update thread";
                }

                if( mClient )
                {
                    mClient->removeEventListener(this);
                }

                // TBD: route the stats to telemetry - dump them for the time being
                bool isLoggingSonar = Services::readSetting(Services::SETTING_LogSonar);
                if(isLoggingSonar)
                {
                    ORIGIN_LOG_EVENT << "*** Sonar call stats - start ***";
                    ORIGIN_LOG_EVENT << " tx packets = " << mVoiceClientOptions.audioMessagesSent() << " bytes = " << mVoiceClientOptions.audioBytesSent();
                    ORIGIN_LOG_EVENT << " rx packets = " << mVoiceClientOptions.audioMessagesReceived() << " bytes = " << mVoiceClientOptions.audioBytesReceived();
                    ORIGIN_LOG_EVENT << " rx out of sequence packets = " << mVoiceClientOptions.audioMessagesOutOfSequence();
                    ORIGIN_LOG_EVENT << " rx lost packets = " << mVoiceClientOptions.audioMessagesLost();
                    ORIGIN_LOG_EVENT << " capture pre-gain min = " << mVoiceClientOptions.capturePreGainMin();
                    ORIGIN_LOG_EVENT << " capture pre-gain max = " << mVoiceClientOptions.capturePreGainMax();
                    ORIGIN_LOG_EVENT << " capture pre-gain clip = " << mVoiceClientOptions.capturePreGainClip();
                    ORIGIN_LOG_EVENT << " capture post-gain min = " << mVoiceClientOptions.capturePostGainMin();
                    ORIGIN_LOG_EVENT << " capture post-gain max = " << mVoiceClientOptions.capturePostGainMax();
                    ORIGIN_LOG_EVENT << " capture post-gain clip = " << mVoiceClientOptions.capturePostGainClip();
                    ORIGIN_LOG_EVENT << " capture device lost = " << mVoiceClientOptions.captureDeviceLost();
                    ORIGIN_LOG_EVENT << " capture no audio = " << mVoiceClientOptions.captureNoAudio();
                    ORIGIN_LOG_EVENT << " playback playback pre-gain min = " << mVoiceClientOptions.playbackPreGainMin();
                    ORIGIN_LOG_EVENT << " playback playback pre-gain max = " << mVoiceClientOptions.playbackPreGainMax();
                    ORIGIN_LOG_EVENT << " playback playback pre-gain clip = " << mVoiceClientOptions.playbackPreGainClip();
                    ORIGIN_LOG_EVENT << " playback playback post-gain min = " << mVoiceClientOptions.playbackPostGainMin();
                    ORIGIN_LOG_EVENT << " playback playback post-gain max = " << mVoiceClientOptions.playbackPostGainMax();
                    ORIGIN_LOG_EVENT << " playback playback post-gain clip = " << mVoiceClientOptions.playbackPostGainClip();
                    ORIGIN_LOG_EVENT << " playback under-run = " << mVoiceClientOptions.playbackUnderrun();
                    ORIGIN_LOG_EVENT << " playback overflow = " << mVoiceClientOptions.playbackOverflow();
                    ORIGIN_LOG_EVENT << " playback device lost = " << mVoiceClientOptions.playbackDeviceLost();
                    ORIGIN_LOG_EVENT << "*** Sonar call stats - end ***";
                }

                if (disconnected)
                    emit voiceClientDisconnectCompleted(joiningVoice);

                return disconnected;
            }   

            bool VoiceClient::isConnected()
            {
                QMutexLocker locker(&mVoiceMutex);
                if (!mClient)
                    return false;
                return mClient->isConnected();
            }

            bool VoiceClient::isConnecting()
            {
                QMutexLocker locker(&mVoiceMutex);
                if (!mClient)
                    return false;
                return mClient->isConnecting();
            }

            QString VoiceClient::getChannelDesc()
            {
                QMutexLocker locker(&mVoiceMutex);
                return mClient->getChannelDesc().c_str();
            }

            QString VoiceClient::getChannelId()
            {
                QMutexLocker locker(&mVoiceMutex);
                return mClient->getChannelId().c_str();
            }

            int VoiceClient::getNumClientsInChannel()
            {
                sonar::PeerList clients = mClient->getClients();

                return clients.size();
            }

            int VoiceClient::getNetworkQuality() const
            {
                if( mClient && mClient->isConnected() )
                {
                    int quality = mClient->getNetworkQuality();
                    return quality;
                }

                return 0;
            }

            void VoiceClient::onBlockListUpdated()
            {
                Origin::Chat::BlockList* blockList = dynamic_cast<Origin::Chat::BlockList*>(sender());
                ORIGIN_ASSERT(blockList);
                mBlockedListCache = blockList->blockedIds();
                mBlockListHasChanged = true;
            }

            const sonar::BlockedList& VoiceClient::blockList() const
            {
                ORIGIN_LOG_DEBUG << "blockList: " << mBlockedListCache.size() << " IDs blocked";
                if (mBlockedListCache.size() > 0)
                    ORIGIN_LOG_DEBUG << "blockList[0] = " << mBlockedListCache[0];

                return mBlockedListCache;
            }

            bool VoiceClient::hasChanges() const
            {
                return mBlockListHasChanged;
            }

            void VoiceClient::clearChanges()
            {
                mBlockListHasChanged = false;
            }

            void VoiceClient::changeOutputDevice(const QString& deviceId)
            {
                QMutexLocker locker(&mVoiceMutex);

                // There is some sort of firmware version before the real ID, we want to strip that out
                QStringList list = deviceId.split(".");
                mClient->setPlaybackDevice(list[list.size() - 1].toStdString());
            }

            void VoiceClient::changeInputDevice(const QString& deviceId)
            {
                QMutexLocker locker(&mVoiceMutex);

                // There is some sort of firmware version before the real ID, we want to strip that out
                QStringList list = deviceId.split(".");
                mClient->setCaptureDevice(list[list.size() - 1].toStdString());
            }

            void VoiceClient::onVoiceConnectedAndDevicesActivated()
            {
                if (!isAudioAvailable())
                {
                    // cause the audio error to be set
                    sonar::CString captureDeviceId = mClient->getConfig().get("captureDeviceId", "");
                    sonar::CString playbackDeviceId = mClient->getConfig().get("playbackDeviceId", "");
                    mClient->setCaptureDevice(captureDeviceId);
                    mClient->setPlaybackDevice(playbackDeviceId);

                    int err = lastAudioError();
                    if (err == 0) // fall back
                    {
                        mClient->setCaptureDevice("DEFAULT");
                        mClient->setPlaybackDevice("DEFAULT");
                    }

                    emit voiceConnectError((err == 0) ? lastAudioError() : err);

					if( !isConnectedToSettings() )
						stopVoiceConnection("User disconnection", "onVoiceConnectedAndDevicesActivated");
                }
            }

            QMap<QString, QString> VoiceClient::getCaptureDevices()
            {
                QMap<QString, QString> deviceMap;

                sonar::AudioDeviceList deviceList = mClient->getCaptureDevices();
                sonar::AudioDeviceList::iterator it = deviceList.begin();
                for( ; it != deviceList.end(); ++it )
                {
                    sonar::AudioDeviceId device = *it;
                    if( device.id == "DEFAULT")
                    {
                        continue;
                    }
                    deviceMap[QString::fromStdString(device.name)] = QString::fromStdString(device.id);
                }

                return deviceMap;
            }

            QMap<QString, QString> VoiceClient::getPlaybackDevices()
            {
                QMap<QString, QString> deviceMap;

                sonar::AudioDeviceList deviceList = mClient->getPlaybackDevices();
                sonar::AudioDeviceList::iterator it = deviceList.begin();
                for( ; it != deviceList.end(); ++it )
                {
                    sonar::AudioDeviceId device = *it;
                    if( device.id == "DEFAULT")
                    {
                        continue;
                    }
                    deviceMap[QString::fromStdString(device.name)] = QString::fromStdString(device.id);
                }

                return deviceMap;
            }

            void VoiceClient::enableRemoteEcho(bool enable)
            {
                QMutexLocker locker(&mVoiceMutex);

                mClient->setRemoteEcho(enable);
            }

            void VoiceClient::enablePushToTalk(bool enable)
            {
                QMutexLocker locker(&mVoiceMutex);
                mClient->setSoftPushToTalk(enable);       
            }

            void VoiceClient::setAutoGain(bool autoGain)
            {
                QMutexLocker locker(&mVoiceMutex);
                mClient->setAutoGain(autoGain);
            }

            void VoiceClient::setCaptureMode(bool enablePushToTalk, bool enableAutoGain, int voiceActivationThreshold)
            {
                QMutexLocker locker(&mVoiceMutex);
                sonar::CaptureMode sonarCaptureMode = (enablePushToTalk) ? sonar::PushToTalk : sonar::VoiceActivation;

                mClient->setCaptureMode(sonarCaptureMode);

                if (enablePushToTalk)
                {
                    // No auto gain when using pushToTalk
                    mClient->setAutoGain(false);                    
                }
                else // mode == VoiceActivation
                {
                    // Voice activated
                    mClient->setAutoGain(enableAutoGain);

                    if (!enableAutoGain)
                    {
                        int voiceSensitivity = 100 - voiceActivationThreshold;
                        mClient->setVoiceSensitivity(voiceSensitivity);
                    }
                }
            }

            VoiceClient::CaptureMode VoiceClient::getCaptureMode()
            {
                QMutexLocker locker(&mVoiceMutex);
                bool pushToTalk = (mClient->getCaptureMode() == sonar::PushToTalk);

                if( pushToTalk )
                    return PushToTalk;

                return VoiceActivation;
            }

            void VoiceClient::setPushToTalkKey(QString key) // for mouse input
            {
                QMutexLocker locker(&mVoiceMutex);
                mClient->bindPushToTalkKey(key.toStdString(), false);
            }

            void VoiceClient::setPushToTalkKey(int vkkey[], int size)   // for keyboard input
            {
                QMutexLocker locker(&mVoiceMutex);
                mClient->bindPushToTalkKey( vkkey, size, false);
            }

            void VoiceClient::setMicrophoneGain(int gain)
            {
                int sonarGain = gain * 2 / SONAR_MAX_CAPTURE_GAIN;
                QMutexLocker locker(&mVoiceMutex);
                mClient->setCaptureGain(sonarGain);
            }

            void VoiceClient::setSpeakerGain(int gain)
            {
                int sonarGain = gain * 2 / SONAR_MAX_PLAYBACK_GAIN;
                QMutexLocker locker(&mVoiceMutex);
                mClient->setPlaybackGain(sonarGain);
            }

            void VoiceClient::setVoiceActivationThreshold(int threshold)
            {
                int voiceSensitivity = 100 - threshold;
                if (voiceSensitivity < 0)
                    voiceSensitivity = 0;
                else if (voiceSensitivity > 100)
                    voiceSensitivity = 100;
                voiceSensitivity = scaleVoiceSensitivity(voiceSensitivity);

				ORIGIN_LOG_DEBUG_IF(ENABLE_VOICECLIENT_TRACING) << "set " << 100 - threshold << " => " << voiceSensitivity;

                // prevent turning off the threshold
                if (voiceSensitivity == 100)
                    voiceSensitivity = 99;

                QMutexLocker locker(&mVoiceMutex);
                mClient->setVoiceSensitivity(voiceSensitivity);
            }

            void VoiceClient::muteUser(const QString& userId)
            {
                mClient->muteClient(userId.toStdString(), true);
            }

            void VoiceClient::unmuteUser(const QString& userId)
            {
                mClient->muteClient(userId.toStdString(), false);
            }

            void VoiceClient::mute()
            {
                mClient->setCaptureMute(true);
            }

            void VoiceClient::unmute()
            {
                mClient->setCaptureMute(false);
            }

            bool VoiceClient::isMuted()
            {
                return mClient->getCaptureMute();
            }

            void VoiceClient::evtStrokeRecorded(sonar::CString& stroke)
            {

            }

            void VoiceClient::evtCaptureModeChanged(sonar::CaptureMode mode)
            {

            }

            void VoiceClient::evtPlaybackMuteKeyChanged(sonar::CString& stroke, bool block)
            {

            }

            void VoiceClient::evtCaptureMuteKeyChanged(sonar::CString& stroke, bool block)
            {

            }

            void VoiceClient::evtPushToTalkKeyChanged(sonar::CString& stroke, bool block)
            {
            }

            void VoiceClient::evtPushToTalkActiveChanged(bool active)
            {
                ORIGIN_LOG_DEBUG << "Push To Talk : " << active;
                mIsPTTActive = active;
                emit(voicePushToTalkActive(active));
            }

            void VoiceClient::evtTalkTimeOverLimit(bool over)
            {
                emit(voiceTalkTimeOverLimit(over));
            }

            void VoiceClient::evtChannelInactivity(unsigned long interval)
            {
                unsigned long inactivityTimeout = SONAR_INACTIVITY_TIMEOUT;
                bool useOneMinuteTimeout = Origin::Services::readSetting(Origin::Services::SETTING_SonarOneMinuteInactivityTimeout);
                if( useOneMinuteTimeout )
                    inactivityTimeout = 60000; // 1 minute (in milliseconds)

                if( interval >= inactivityTimeout )
                    emit(voiceChannelInactivity());
            }

            void VoiceClient::evtConnected(int chid, sonar::CString& channelId, sonar::CString channelDesc)
            {
                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "VoiceClient::evtConnected: t=" << QDateTime::currentMSecsSinceEpoch();

                QString id = QString::fromUtf8(channelId.c_str());
                QString desc = QString::fromUtf8(channelDesc.c_str());
                emit(voiceConnected(chid, id, desc));
            }

            void VoiceClient::evtDisconnected(sonar::CString& reasonType, sonar::CString& reasonDesc)
            {
                QString type = QString::fromUtf8(reasonType.c_str());
                QString desc = QString::fromUtf8(reasonDesc.c_str());
                emit(voiceDisconnected(type, desc));
            }

            void VoiceClient::evtShutdown()
            {

            }

            void VoiceClient::evtClientJoined(const sonar::PeerInfo& client)
            {
                QString user = client.userId.c_str();
                emit(voiceUserJoined(user));
            }

            void VoiceClient::evtClientParted(const sonar::PeerInfo& client, sonar::CString& reasonType, sonar::CString &reasonDesc)
            {
                QString user = client.userId.c_str();
                emit(voiceUserLeft(user));
            }

            void VoiceClient::evtClientMuted(const sonar::PeerInfo& client)
            {
                QString user = client.userId.c_str();
                if (client.isMuted)
                    emit(voiceUserMuted(user));
                else
                    emit(voiceUserUnMuted(user));
            }

            void VoiceClient::evtClientTalking(const sonar::PeerInfo& client)
            {
                QString user = client.userId.c_str();
                if (client.isTalking)
                    emit(voiceUserTalking(user));
                else
                    emit(voiceUserStoppedTalking(user));
            }

            void VoiceClient::evtCaptureAudio (bool clip, float avgAmp, float peakAmp, bool vaState, bool xmit)
            {
                float amp = scaleVoiceSensitivity(peakAmp / 327.67);

				ORIGIN_LOG_DEBUG_IF(ENABLE_VOICECLIENT_TRACING) << "evt " << peakAmp / 327.67 << "(" << peakAmp << ") => " << amp;

                // Clamp level to zero when PTT enabled and not active
                if ((mClient->getCaptureMode() == sonar::PushToTalk) && !mIsPTTActive)
                    amp = 0;

                emit(voiceLevel(amp));
            }

            void VoiceClient::evtPlaybackMuteChanged(bool state)
            {

            }

            void VoiceClient::evtCaptureMuteChanged(bool state)
            {
                if (state)
                    emit (voiceLocalUserMuted());
                else
                    emit (voiceLocalUserUnMuted());
            }

            void VoiceClient::evtCaptureDeviceChanged(sonar::CString& deviceId, sonar::CString& deviceName)
            {

            }

            void VoiceClient::evtPlaybackDeviceChanged(sonar::CString& deviceId, sonar::CString& deviceName)
            {

            }

            void VoiceClient::evtLoopbackChanged(bool state)
            {

            }

            void VoiceClient::evtAutoGainChanged(bool state)
            {

            }

            void VoiceClient::evtCaptureGainChanged(int percent)
            {

            }

            void VoiceClient::evtPlaybackGainChanged(int percent)
            {

            }

            void VoiceClient::evtVoiceSensitivityChanged(int percent)
            {

            }

            void VoiceClient::onEnableEchoCancellation(const bool &state)
            {
                mClient->setEchoCancellation(state);
            }

            void VoiceClient::onEchoQueuedDelayChanged(int value)
            {
                mClient->setEchoQueuedDelay(value);
            }

            void VoiceClient::onEchoTailMultiplierChanged(int value)
            {
                mClient->setEchoTailMultiplier(value);   
            }

            void VoiceClient::onEmptyConversationTimeout()
            {
                if (!isConnected())
                    return;

                stopVoiceConnection("User disconnection", "onVoiceInactivity");
            }

            // The following slots are required so that the timer can be used on the same thread that created them.

            void VoiceClient::onVoiceConnected(int chid, const QString& /*channelId*/, const QString& /*channelDescription*/)
            {
                // if we're the only ones in the conversation, start the timeout timer immediately
                if (mClient->getClientCount() == 1) {
                    const int emptyConversationTimeoutDuration = Origin::Services::readSetting(Origin::Services::SETTING_SonarTestingEmptyConversationTimeoutDuration);
                    mEmptyConversationTimer.start(emptyConversationTimeoutDuration);
                }
            }

            void VoiceClient::onVoiceDisconnected(const QString& /*reasonType*/, const QString& /*reasonDescription*/)
            {
                // stop the timeout timer regardless
                mEmptyConversationTimer.stop();
            }

            void VoiceClient::onVoiceUserJoined(const QString& /*user*/)
            {
                // stop the timer if there's more than one user in the call
                if (mClient->getClientCount() > 1) {
                    mEmptyConversationTimer.stop();
                }
            }

            void VoiceClient::onVoiceUserLeft(const QString& /*user*/)
            {
                if (mClient->getClientCount() == 1 && !mEmptyConversationTimer.isActive()) {
                    const int emptyConversationTimeoutDuration = Origin::Services::readSetting(Origin::Services::SETTING_SonarTestingEmptyConversationTimeoutDuration);
                    mEmptyConversationTimer.start(emptyConversationTimeoutDuration);
                }
            }
        }
    }
}

#endif //ENABLE_VOICE_CHAT

// Copyright (C) 2013 Electronic Arts. All rights reserved.

#ifndef VOICE_CLIENT_H
#define VOICE_CLIENT_H

#if ENABLE_VOICE_CHAT

#define NOMINMAX 
#include <QObject>
#include <QString>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <SonarClient/Client.h>
#include <SonarConnection/common.h>

#include "engine/voice/VoiceSettings.h"
#include "engine/login/User.h"
#include "services/debug/DebugService.h"

namespace Origin
{
    namespace Engine
    {
        namespace Voice
        {
            class SonarNetworkWorker;
            class VoiceUpdateWorker;

            class VoiceClientOptions : public sonar::Options, public sonar::IStatsProvider
            {
            public:

                VoiceClientOptions(UserRef user)
                    : mUser(user.toWeakRef())
                {
                    resetAll();
                }

                virtual ~VoiceClientOptions() {}

                virtual bool& enableCapturePcmLogging() { return _enableCapturePcmLogging; }
                virtual bool& enableCaptureSpeexLogging() { return _enableCaptureSpeexLogging; }
                virtual bool& enablePlaybackDsoundLogging() { return _enablePlaybackDsoundLogging; }
                virtual bool& enablePlaybackSpeexLogging() { return _enablePlaybackSpeexLogging; }

                virtual bool& enableClientSettingsLogging() { return _enableClientSettingsLogging; }
                virtual bool& enableClientDSoundTracing() { return _enableClientDSoundTracing; }
                virtual bool& enableClientMixerStats() { return _enableClientMixerStats; }

                virtual bool& enableSonarTimingLogging() { return _enableSonarTimingLogging; }
                virtual bool& enableConnectionStatsLogging() { return _enableSonarConnectionStatsLogging; }

                virtual bool enableSpeexEchoCancellation() const { return _enableSpeexEchoCancellation; }
                virtual int speexEchoQueuedDelay() const { return _speexEchoQueuedDelay; }
                virtual int speexEchoTailMultiplier() const { return _speexEchoTailMultiplier; }
                virtual bool& enableSonarJitterTracing() { return _enableSonarJitterTracing; }
                virtual bool& enableSonarPayloadTracing() { return _enableSonarPayloadTracing; }
                virtual int& sonarJitterMetricsLogLevel() { return _sonarJitterMetricsLogLevel; }

                virtual bool allowHardwareTelemetry()
                {
                    UserRef user = mUser.toStrongRef();
                    bool allowHardwareTelemetry = (user)
                        ? !Origin::Services::readSetting(Origin::Services::SETTING_HW_SURVEY_OPTOUT, user->getSession())
                        : true;
                    return allowHardwareTelemetry;
                }

                virtual int speexSettingAgcEnable() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarSpeexAgcEnable); }
                virtual float speexSettingAgcLevel() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarSpeexAgcLevel); }
                virtual int speexSettingComplexity() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarSpeexComplexity); }
                virtual int speexSettingQuality() const  { return Origin::Services::readSetting(Origin::Services::SETTING_SonarSpeexQuality); }
                virtual int speexSettingVbrEnable() const  { return Origin::Services::readSetting(Origin::Services::SETTING_SonarSpeexVbrEnable); }
                virtual float speexSettingVbrQuality() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarSpeexVbrQuality); }

                virtual int qaTestingAudioError() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarTestingAudioError); }
                virtual int qaTestingRegisterPacketLossPercentage() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarTestingRegisterPacketLossPercentage); }
                virtual sonar::CString qaTestingVoiceServerRegisterResponse() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarTestingVoiceServerRegisterResponse).toString().toStdString(); }

                virtual int sonarJitterBufferSize() const { return 7; }
                virtual int sonarRegisterTimeout() const { return 10000; }
                virtual int sonarRegisterMaxRetries() const { return 0; }
                virtual int sonarNetworkPingInterval() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkPingInterval); }

                virtual int networkGoodLoss() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkGoodLoss); }
                virtual int networkGoodJitter() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkGoodJitter); }
                virtual int networkGoodPing() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkGoodPing); }
                virtual int networkOkLoss() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkOkLoss); }
                virtual int networkOkJitter() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkOkJitter); }
                virtual int networkOkPing() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkOkPing); }
                virtual int networkPoorLoss() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkPoorLoss); }
                virtual int networkPoorJitter() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkPoorJitter); }
                virtual int networkPoorPing() const { return Origin::Services::readSetting(Origin::Services::SETTING_SonarNetworkPoorPing); }

                virtual IStatsProvider& stats() { return *this; }
                virtual IStatsProvider const& stats() const { return *this; }

                virtual void resetAll()
                {
                    _jitterArrivalMeanCumulative = 0;
                    _jitterArrivalMaxCumulative = 0;
                    _jitterPlaybackMeanCumulative = 0;
                    _jitterPlaybackMaxCumulative = 0;
                    _jitterNumMeasurements = 0;

                    resetNonCumulativeStats();
                    resetOptions();
                }

                virtual void resetNonCumulativeStats()
                {
                    _bytesSent = 0;
                    _bytesReceived = 0;
                    _capturePreGainMin = 0.0f;
                    _capturePreGainMax = 0.0f;
                    _capturePreGainClip = 0;
                    _capturePostGainMin = 0.0f;
                    _capturePostGainMax = 0.0f;
                    _capturePostGainClip = 0;
                    _capDeviceLost = 0;
                    _capNoAudio = 0;
                    _playbackPreGainMin = 32767.0f;
                    _playbackPreGainMax = -32768.0f;
                    _playbackPreGainClip = 0;
                    _playbackPostGainMin = 32767.0f;
                    _playbackPostGainMax = -32768.0f;
                    _playbackPostGainClip = 0;

                    std::memset(&_stats, 0, sizeof(_stats));
                }

                void resetOptions()
                {
                    _enableCapturePcmLogging = Origin::Services::readSetting(Origin::Services::SETTING_LogSonarDirectSoundCapture);
                    _enableCaptureSpeexLogging = Origin::Services::readSetting(Origin::Services::SETTING_LogSonarSpeexCapture);
                    _enablePlaybackDsoundLogging = Origin::Services::readSetting(Origin::Services::SETTING_LogSonarDirectSoundPlayback);
                    _enablePlaybackSpeexLogging = Origin::Services::readSetting(Origin::Services::SETTING_LogSonarSpeexPlayback);
                    _enableClientSettingsLogging = Origin::Services::readSetting(Origin::Services::SETTING_LogSonar);
                    _enableClientDSoundTracing = false;
                    _enableSonarJitterTracing = Origin::Services::readSetting(Origin::Services::SETTING_LogSonarJitterTracing);
                    _enableSonarPayloadTracing = Origin::Services::readSetting(Origin::Services::SETTING_LogSonarPayloadTracing);
                    _enableClientMixerStats = _enableClientSettingsLogging;
                    _enableSonarTimingLogging = Origin::Services::readSetting(Origin::Services::SETTING_LogSonarTiming);
                    _enableSonarConnectionStatsLogging = Origin::Services::readSetting(Origin::Services::SETTING_LogSonarConnectionStats);
                    _sonarJitterMetricsLogLevel = Origin::Services::readSetting(Origin::Services::SETTING_LogSonarJitterMetricsLevel);
                    _enableSpeexEchoCancellation = Origin::Services::readSetting(Origin::Services::SETTING_EchoCancellation);
                    _speexEchoQueuedDelay = 5;//Origin::Services::readSetting(Origin::Services::SETTING_EchoQueuedDelay);
                    _speexEchoTailMultiplier = 5;//Origin::Services::readSetting(Origin::Services::SETTING_EchoTailMultiplier);
                }

                virtual unsigned int& audioBytesSent() { return _bytesSent; }
                virtual unsigned int& audioBytesReceived() { return _bytesReceived; }

                virtual float& capturePreGainMin() { return _capturePreGainMin; }
                virtual float& capturePreGainMax() { return _capturePreGainMax; }
                virtual unsigned int& capturePreGainClip() { return _capturePreGainClip; }
                virtual float& capturePostGainMin() { return _capturePostGainMin; }
                virtual float& capturePostGainMax() { return _capturePostGainMax; }
                virtual unsigned int& capturePostGainClip() { return _capturePostGainClip; }
                virtual unsigned int& captureDeviceLost() { return _capDeviceLost; }
                virtual unsigned int& captureNoAudio() { return _capNoAudio; }

                virtual float& playbackPreGainMin() { return _playbackPreGainMin; }
                virtual float& playbackPreGainMax() { return _playbackPreGainMax; }
                virtual unsigned int& playbackPreGainClip() { return _playbackPreGainClip; }
                virtual float& playbackPostGainMin() { return _playbackPostGainMin; }
                virtual float& playbackPostGainMax() { return _playbackPostGainMax; }
                virtual unsigned int& playbackPostGainClip() { return _playbackPostGainClip; }

                virtual unsigned int& audioMessagesLost() { return _stats.lost; }
                virtual unsigned int audioMessagesLost() const { return _stats.lost; }

                virtual unsigned int& audioMessagesOutOfSequence() { return _stats.outOfSequence; }
                virtual unsigned int audioMessagesOutOfSequence() const { return _stats.outOfSequence; }

                virtual unsigned int& audioMessagesSent() { return _stats.sent; }
                virtual unsigned int audioMessagesSent() const { return _stats.sent; }

                virtual unsigned int& audioMessagesReceived() { return _stats.received; }
                virtual unsigned int audioMessagesReceived() const { return _stats.received; }

                virtual unsigned int& audioMixClipping() { return _stats.audioMixClipping; }
                virtual unsigned int& badCid() { return _stats.badCid; }
                virtual unsigned int& truncatedMessages() { return _stats.truncated; }
                virtual unsigned int& socketRecvError() { return _stats.socketRecvError; }
                virtual unsigned int& socketSendError() { return _stats.socketSendError; }

                virtual unsigned int& deltaPlaybackMean() { return _stats.deltaPlaybackMean; }
                virtual unsigned int& deltaPlaybackMax() { return _stats.deltaPlaybackMax; }
                virtual unsigned int& receiveToPlaybackMean() { return _stats.receiveToPlaybackMean; }
                virtual unsigned int& receiveToPlaybackMax() { return _stats.receiveToPlaybackMax; }

                virtual int& jitterBufferDepth() { return _stats.jitterBufferDepth; }
                virtual int const jitterBufferDepth() const { return _stats.jitterBufferDepth; }

                virtual int& jitterArrivalMean() { return _stats.jitterArrivalMean; }
                virtual int& jitterArrivalMax() { return _stats.jitterArrivalMax; }
                virtual int& jitterPlaybackMean() { return _stats.jitterPlaybackMean; }
                virtual int& jitterPlaybackMax() { return _stats.jitterPlaybackMax; }
                int& jitterArrivalMeanCumulative() override { return _jitterArrivalMeanCumulative; }
                int& jitterArrivalMaxCumulative() override { return _jitterArrivalMaxCumulative; }
                int& jitterPlaybackMeanCumulative() override { return _jitterPlaybackMeanCumulative; }
                int& jitterPlaybackMaxCumulative() override { return _jitterPlaybackMaxCumulative; }
                int& jitterNumMeasurements() override { return _jitterNumMeasurements; }

                virtual unsigned int& playbackUnderrun() { return _stats.playbackUnderrun; }
                virtual unsigned int& playbackOverflow() { return _stats.playbackOverflow; }
                virtual unsigned int& playbackDeviceLost() { return _stats.playbackDeviceLost; }
                virtual unsigned int& dropNotConnected() { return _stats.dropNotConnected; }
                virtual unsigned int& dropReadServerFrameCounter() { return _stats.dropReadServerFrameCounter; }
                virtual unsigned int& dropReadTakeNumber() { return _stats.dropReadTakeNumber; }
                virtual unsigned int& dropTakeStopped() { return _stats.dropTakeStopped; }
                virtual unsigned int& dropReadCid() { return _stats.dropReadCid; }
                virtual unsigned int& dropNullPayload() { return _stats.dropNullPayload; }
                virtual unsigned int& dropReadClientFrameCounter() { return _stats.dropReadClientFrameCounter; }
                virtual unsigned int& jitterbufferUnderrun() { return _stats.jitterbufferUnderrun; }

                virtual void setEchoCancellation(bool value) { _enableSpeexEchoCancellation = value;}
                virtual void setEchoQueuedDelay(int value) { _speexEchoQueuedDelay = value; }
                virtual void setEchoTailMultiplier(int value) { _speexEchoTailMultiplier = value; }

            private:

                UserWRef mUser;
                unsigned int _bytesSent;
                unsigned int _bytesReceived;
                float _capturePreGainMin;
                float _capturePreGainMax;
                unsigned int _capturePreGainClip;
                float _capturePostGainMin;
                float _capturePostGainMax;
                unsigned int _capturePostGainClip;
                unsigned int _capDeviceLost;
                unsigned int _capNoAudio;
                float _playbackPreGainMin;
                float _playbackPreGainMax;
                unsigned int _playbackPreGainClip;
                float _playbackPostGainMin;
                float _playbackPostGainMax;
                unsigned int _playbackPostGainClip;
                sonar::ClientToServerStats _stats;
                int _jitterArrivalMeanCumulative;
                int _jitterArrivalMaxCumulative;
                int _jitterPlaybackMeanCumulative;
                int _jitterPlaybackMaxCumulative;
                int _jitterNumMeasurements;

                int _sonarJitterMetricsLogLevel;
                bool _enableCapturePcmLogging;
                bool _enableCaptureSpeexLogging;
                bool _enablePlaybackDsoundLogging;
                bool _enablePlaybackSpeexLogging;
                bool _enableClientSettingsLogging;
                bool _enableClientDSoundTracing;
                bool _enableSonarJitterTracing;
                bool _enableSonarPayloadTracing;
                bool _enableClientMixerStats;
                bool _enableSonarTimingLogging;
                bool _enableSonarConnectionStatsLogging;
                bool _enableSpeexEchoCancellation;
                int _speexEchoQueuedDelay;
                int _speexEchoTailMultiplier;
            };

            class VoiceClient : public QObject, public sonar::IClientEvents, public sonar::IBlockedListProvider
            {
                Q_OBJECT
            public:

                enum CaptureMode
                {
                    PushToTalk,
                    VoiceActivation
                };

                VoiceClient(UserRef user);
                virtual ~VoiceClient();

                VoiceClientOptions const& options() const {
                    return mVoiceClientOptions;
                }

                bool createVoiceConnection(QString channelId, const VoiceSettings& settings, bool remoteEcho);
                void stopVoiceConnection(const QString& reasonType, const QString& reasonDescription, bool joiningVoice = false);
                bool isInVoice();
                bool isConnectingToVoice();
                int  getUsersInVoice();
                void changeInputDevice(const QString& deviceId);
                void changeOutputDevice(const QString& deviceId);
                QMap<QString, QString> getCaptureDevices();
                QMap<QString, QString> getPlaybackDevices();

                /// \brief Returns true if the voice client's audio devices are activated.
                bool isAudioAvailable() const;

                /// \brief Returns the last error encountered by the voice client's audio devices.
                int lastAudioError() const;

                void enableRemoteEcho(bool enable);
                void enablePushToTalk(bool enable);
                void setAutoGain(bool autoGain);
                void setCaptureMode(bool enablePushToTalk, bool enableAutoGain, int voiceActivationThreshold);
                void setPushToTalkKey(QString key);
                void setPushToTalkKey(int vkkey[], int size);
                void setMicrophoneGain(int gain);               // 0 - 100
                void setSpeakerGain(int gain);                  // 0 - 100
                void setVoiceActivationThreshold(int threshold);    // 0 - 100

                CaptureMode getCaptureMode();

                void muteUser(const QString& userId);
                void unmuteUser(const QString& userId);
                void mute();
                void unmute();
                bool isMuted();
                QString getChannelId();

                void setConnectedToSettings(bool connected) { mIsConnectedToSettings = connected; }
                bool isConnectedToSettings() { return mIsConnectedToSettings; }

                /// \brief Returns the network quality in the form of an integer from 0-3 (low to high)
                int getNetworkQuality() const;

                // IBlockedListProvider interface
                virtual const sonar::BlockedList& blockList() const;
                virtual bool hasChanges() const;
                virtual void clearChanges();

            public slots:

                /// \brief This slot gets called after the voice has connected and its audio devices have finished activating.
                void onVoiceConnectedAndDevicesActivated();
                void onEnableEchoCancellation(const bool&);
                void onEchoQueuedDelayChanged(int);
                void onEchoTailMultiplierChanged(int);

            private:

                // Implement IClientEvents interface
                virtual void evtStrokeRecorded(sonar::CString& stroke);
                virtual void evtCaptureModeChanged(sonar::CaptureMode mode);
                virtual void evtPlaybackMuteKeyChanged(sonar::CString& stroke, bool block);
                virtual void evtCaptureMuteKeyChanged(sonar::CString& stroke, bool block);
                virtual void evtPushToTalkKeyChanged(sonar::CString& stroke, bool block);
                virtual void evtPushToTalkActiveChanged(bool active);
                virtual void evtTalkTimeOverLimit(bool over);
                virtual void evtConnected(int chid, sonar::CString &channelId, sonar::CString channelDesc);
                virtual void evtDisconnected(sonar::CString& reasonType, sonar::CString& reasonDesc);
                virtual void evtShutdown(void);
                virtual void evtClientJoined (const sonar::PeerInfo& client);
                virtual void evtClientParted (const sonar::PeerInfo& client, sonar::CString &reasonType, sonar::CString &reasonDesc);
                virtual void evtClientMuted (const sonar::PeerInfo& client);
                virtual void evtClientTalking (const sonar::PeerInfo& client);
                virtual void evtCaptureAudio (bool clip, float avgAmp, float peakAmp, bool vaState, bool xmit);
                virtual void evtPlaybackMuteChanged (bool state);
                virtual void evtCaptureMuteChanged (bool state);
                virtual void evtCaptureDeviceChanged(sonar::CString& deviceId, sonar::CString& deviceName);
                virtual void evtPlaybackDeviceChanged(sonar::CString& deviceId, sonar::CString& deviceName);
                virtual void evtLoopbackChanged (bool state);
                virtual void evtAutoGainChanged(bool state);
                virtual void evtCaptureGainChanged (int percent);
                virtual void evtPlaybackGainChanged (int percent);
                virtual void evtChannelInactivity(unsigned long interval);

                virtual void evtVoiceSensitivityChanged (int percent);
                bool connect(const QString& token, const VoiceSettings& settings, bool remoteEcho);
                bool disconnect(const QString& reasonType, const QString& reasonDescription, bool joiningVoice = false);
                bool isConnected();
                bool isConnecting();

                QString getChannelDesc();

                int getNumClientsInChannel();

                QString convertToSonarKeyString(const QString &key);

            signals:
                void voiceConnected(int chid, const QString& channelId, const QString& channelDescription);
                void voiceDisconnected(const QString& reasonType, const QString& reasonDescription);
                void voiceUserMuted(const QString& userId);
                void voiceUserUnMuted(const QString& userId);
                void voiceLocalUserMuted();
                void voiceLocalUserUnMuted();
                void voiceUserTalking(const QString& userId);
                void voiceUserStoppedTalking(const QString& userId);
                void voiceUserJoined(const QString& userId);
                void voiceUserLeft(const QString& userId);
                void voiceLevel(float level);
                void voicePushToTalkActive(bool active);
                void voiceTalkTimeOverLimit(bool over);
                void voiceChannelInactivity();

                void voiceConnectError(int connectError);

                void voiceClientDisconnectCompleted(bool joiningVoice);

                void underThreshold();
                void overThreshold();

                void startUpdate();
            
            private slots:
                void onBlockListUpdated();

                /// \brief This slot is used to handle timeout for conversations with only one user.
                void onEmptyConversationTimeout();

                void onVoiceConnected(int chid, const QString& channelId, const QString& channelDescription);
                void onVoiceDisconnected(const QString& reasonType, const QString& reasonDescription);
                void onVoiceUserJoined(const QString& user);
                void onVoiceUserLeft(const QString& user);

            private:

                UserWRef mUser;
                VoiceClientOptions mVoiceClientOptions;
                sonar::Client* mClient;
                QThread* mVoiceUpdateThread;
                VoiceUpdateWorker* mVoiceUpdateWorker;
                sonar::BlockedList mBlockedListCache;
                QTimer mEmptyConversationTimer;
                bool mBlockListHasChanged;
                bool mIsConnectedToSettings;
                bool mIsPTTActive;
            };

            class VoiceUpdateWorker : public QObject
            {
                Q_OBJECT
            public:
                VoiceUpdateWorker(sonar::Client* client)
                    : mClient(client)
                    , mFinished(false)
                {

                }

                void stop()
                {
                    mFinished = true;
                }

            public slots:
                void run();

            private:
                sonar::Client* mClient;
                volatile bool mFinished;
            };
        }
    }
}

#endif //ENABLE_VOICE_CHAT

#endif //VOICE_CLIENT_H

#pragma once

#define NOMINMAX

#ifndef SONAR_CLIENT_BUILD_CONFIG
#define SONAR_CLIENT_BUILD_CONFIG <SonarClient/DefaultBuildConfig.h>
#endif
#include SONAR_CLIENT_BUILD_CONFIG

#include <SonarCommon/Common.h>

#include <SonarConnection/Connection.h>
#include <SonarClient/IClientCommands.h>
#include <SonarClient/IClientEvents.h>

#include <SonarClient/ClientConfig.h>
#include <SonarClient/AudioPreprocess.h>
#include <SonarClient/Codec.h>
#include <SonarClient/AudioQueue.h>
#include <SonarClient/Peer.h>

#include <algorithm>

namespace sonar
{
    class Options
    {
    public:

        virtual bool& enableCapturePcmLogging() = 0;
        virtual bool& enableCaptureSpeexLogging() = 0;
        virtual bool& enablePlaybackDsoundLogging() = 0;
        virtual bool& enablePlaybackSpeexLogging() = 0;

        virtual bool& enableClientSettingsLogging() = 0;
        virtual bool& enableClientDSoundTracing() = 0;
        virtual bool& enableClientMixerStats() = 0;
        virtual bool& enableConnectionStatsLogging() = 0;

        virtual bool& enableSonarTimingLogging() = 0;
        virtual bool& enableSonarJitterTracing() = 0;
        virtual int& sonarJitterMetricsLogLevel() = 0;
        virtual bool& enableSonarPayloadTracing() = 0;

        virtual bool enableSpeexEchoCancellation() const = 0;
        virtual int speexEchoQueuedDelay() const = 0;
        virtual int speexEchoTailMultiplier() const = 0;
        virtual void setEchoCancellation(bool state) = 0;
        virtual void setEchoQueuedDelay(int value) = 0;
        virtual void setEchoTailMultiplier(int value) = 0;
        virtual bool allowHardwareTelemetry() = 0;

        virtual int speexSettingAgcEnable() const = 0;
        virtual float speexSettingAgcLevel() const = 0;
        virtual int speexSettingComplexity() const = 0;
        virtual int speexSettingQuality() const = 0;
        virtual int speexSettingVbrEnable() const = 0;
        virtual float speexSettingVbrQuality() const = 0;

        virtual int qaTestingAudioError() const = 0;
        virtual int qaTestingRegisterPacketLossPercentage() const = 0;
        virtual CString qaTestingVoiceServerRegisterResponse() const = 0;

        virtual int sonarJitterBufferSize() const = 0;

        virtual int networkGoodLoss() const = 0;
        virtual int networkGoodJitter() const = 0;
        virtual int networkGoodPing() const = 0;
        virtual int networkOkLoss() const = 0;
        virtual int networkOkJitter() const = 0;
        virtual int networkOkPing() const = 0;
        virtual int networkPoorLoss() const = 0;
        virtual int networkPoorJitter() const = 0;
        virtual int networkPoorPing() const = 0;

        virtual IStatsProvider& stats() = 0;
        virtual IStatsProvider const& stats() const = 0;
    };

    class DefaultOptions : public Options, public IStatsProvider
    {
    public:

        DefaultOptions()
        {
            resetAll();
        }

        virtual bool& enableCapturePcmLogging() { return _enableCapturePcmLogging; }
        virtual bool& enableCaptureSpeexLogging() { return _enableCaptureSpeexLogging; }
        virtual bool& enablePlaybackDsoundLogging() { return _enablePlaybackDsoundLogging; }
        virtual bool& enablePlaybackSpeexLogging() { return _enablePlaybackSpeexLogging; }

        virtual bool& enableClientSettingsLogging() { return _enableClientSettingsLogging; }
        virtual bool& enableClientDSoundTracing() { return _enableClientDSoundTracing; }
        virtual bool& enableClientMixerStats() { return _enableClientMixerStats; }
        virtual bool& enableConnectionStatsLogging() { return _enableConnectionStatsLogging; }

        virtual bool& enableSonarTimingLogging() { return _enableSonarTimingLogging; }
        virtual bool& enableSonarJitterTracing() { return _enableSonarJitterTracing; }
        virtual int& sonarJitterMetricsLogLevel() { return _sonarJitterMetricsLogLevel; }
        virtual bool& enableSonarPayloadTracing() { return _enableSonarPayloadTracing; }

        virtual bool enableSpeexEchoCancellation() const { return _enableSpeexEchoCancellation; }
        virtual int speexEchoQueuedDelay() const { return _speexEchoQueuedDelay; }
        virtual int speexEchoTailMultiplier() const { return _speexEchoTailMultiplier; }
        virtual bool allowHardwareTelemetry() { return true; }

        virtual int speexSettingAgcEnable() const { return 0; }
        virtual float speexSettingAgcLevel() const { return 3500.0; }
        virtual int speexSettingComplexity() const { return 4; }
        virtual int speexSettingQuality() const { return 6; }
        virtual int speexSettingVbrEnable() const { return 1; }
        virtual float speexSettingVbrQuality() const { return 6.0f; }

        virtual int qaTestingAudioError() const { return 0; }
        virtual int qaTestingRegisterPacketLossPercentage() const { return 0; }
        virtual CString qaTestingVoiceServerRegisterResponse() const { return ""; }

        virtual int sonarJitterBufferSize() const { return 8; }

        virtual int networkGoodLoss() const { return sonar::protocol::NETWORK_GOOD_LOSS; }
        virtual int networkGoodJitter() const { return sonar::protocol::NETWORK_GOOD_JITTER; }
        virtual int networkGoodPing() const { return sonar::protocol::NETWORK_GOOD_PING; }
        virtual int networkOkLoss() const { return sonar::protocol::NETWORK_OK_LOSS; }
        virtual int networkOkJitter() const { return sonar::protocol::NETWORK_OK_JITTER; }
        virtual int networkOkPing() const { return sonar::protocol::NETWORK_OK_PING; }
        virtual int networkPoorLoss() const { return sonar::protocol::NETWORK_POOR_LOSS; }
        virtual int networkPoorJitter() const { return sonar::protocol::NETWORK_POOR_JITTER; }
        virtual int networkPoorPing() const { return sonar::protocol::NETWORK_POOR_PING; }

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
            _captureDeviceLost =  0;
            _captureNoAudio =  0;
            _playbackPreGainMin = 0.0f;
            _playbackPreGainMax = 0.0f;
            _playbackPreGainClip = 0;
            _playbackPostGainMin = 0.0f;
            _playbackPostGainMax = 0.0f;
            _playbackPostGainClip = 0;
            std::memset(&_stats, 0, sizeof(_stats));
        }

        void resetOptions()
        {
            _enableCapturePcmLogging = false;
            _enableCaptureSpeexLogging = false;
            _enablePlaybackDsoundLogging = false;
            _enablePlaybackSpeexLogging = false;
            _enableClientSettingsLogging = false;
            _enableClientDSoundTracing = false;
            _enableClientMixerStats = false;
            _enableSonarTimingLogging = false;
            _enableConnectionStatsLogging = false;
            _enableSonarJitterTracing = false;
            _enableSonarPayloadTracing = false;
            _sonarJitterMetricsLogLevel = 0;
            _enableSpeexEchoCancellation = true;
            _speexEchoQueuedDelay = 5;
            _speexEchoTailMultiplier = 5;
        }

        virtual unsigned int& audioBytesSent() { return _bytesSent; }
        virtual unsigned int& audioBytesReceived() { return _bytesReceived; }

        virtual float& capturePreGainMin() { return _capturePreGainMin; }
        virtual float& capturePreGainMax() { return _capturePreGainMax; }
        virtual unsigned int& capturePreGainClip() { return _capturePreGainClip; }
        virtual float& capturePostGainMin() { return _capturePostGainMin; }
        virtual float& capturePostGainMax() { return _capturePostGainMax; }
        virtual unsigned int& capturePostGainClip() { return _capturePostGainClip; }
        virtual unsigned int& captureDeviceLost() { return _captureDeviceLost; }
        virtual unsigned int& captureNoAudio() { return _captureNoAudio; }

        virtual float& playbackPreGainMin() { return _playbackPreGainMin; }
        virtual float& playbackPreGainMax() { return _playbackPreGainMax; }
        virtual unsigned int& playbackPreGainClip() { return _playbackPreGainClip; }
        virtual float& playbackPostGainMin() { return _playbackPostGainMin; }
        virtual float& playbackPostGainMax() { return _playbackPostGainMax; }
        virtual unsigned int& playbackPostGainClip() { return _playbackPostGainClip; }

        virtual unsigned int& audioMessagesSent() { return _stats.sent; }
        virtual unsigned int audioMessagesSent() const { return _stats.sent; }

        virtual unsigned int& audioMessagesReceived() { return _stats.received; }
        virtual unsigned int audioMessagesReceived() const { return _stats.received; }

        virtual unsigned int& audioMessagesOutOfSequence() { return _stats.outOfSequence; }
        virtual unsigned int audioMessagesOutOfSequence() const { return _stats.outOfSequence; }

        virtual unsigned int& audioMessagesLost() { return _stats.lost; }
        virtual unsigned int audioMessagesLost() const { return _stats.lost; }

        virtual unsigned int& badCid() { return _stats.badCid; }
        virtual unsigned int& truncatedMessages() { return _stats.truncated; }
        virtual unsigned int& audioMixClipping() { return _stats.audioMixClipping; }
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

        virtual void setEchoCancellation(bool state) { _enableSpeexEchoCancellation = state; }
        virtual void setEchoQueuedDelay(int value) { _speexEchoQueuedDelay = value; }
        virtual void setEchoTailMultiplier(int value) { _speexEchoTailMultiplier = value; }

    private:

        unsigned int _bytesSent;
        unsigned int _bytesReceived;
        float _capturePreGainMin;
        float _capturePreGainMax;
        unsigned int _capturePreGainClip;
        float _capturePostGainMin;
        float _capturePostGainMax;
        unsigned int _capturePostGainClip;
        unsigned int _captureDeviceLost;
        unsigned int _captureNoAudio;
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

        bool _enableCapturePcmLogging;
        bool _enableCaptureSpeexLogging;
        bool _enablePlaybackDsoundLogging;
        bool _enablePlaybackSpeexLogging;

        bool _enableClientSettingsLogging;
        bool _enableClientDSoundTracing;
        bool _enableClientMixerStats;
        bool _enableSonarTimingLogging;
        bool _enableConnectionStatsLogging;
        bool _enableSonarJitterTracing;
        bool _enableSonarPayloadTracing;
        int _sonarJitterMetricsLogLevel;
        bool _enableSpeexEchoCancellation;
        int _speexEchoQueuedDelay;
        int _speexEchoTailMultiplier;
    };

	class Client : public IClientCommands, private IConnectionEvents
	{
	public:
		Client(Options* options = NULL, const ConnectionOptions = ConnectionOptions(), IBlockedListProvider* blockedListProvider = NULL);
		virtual ~Client(void);

		void update();

        Options& options() const { return m_options; }

		void setConfig(const ClientConfig &config);
		ClientConfig getConfig (void);

		void addEventListener(IClientEvents *listener);
		void removeEventListener(IClientEvents *listener);

		/* sonar::IClientCommands */
		bool connect(CString &token);
		bool disconnect(sonar::CString &reasonType, sonar::CString &reasonDesc);
		void muteClient(CString &userId, bool state);
		void muteClient(int chid, bool state);
		bool isClientMuted(int chid);
		void setCaptureMode(CaptureMode mode);
		CaptureMode getCaptureMode();
		void recordStroke();

		void bindPushToTalkKey(CString &stroke, bool block);
        void bindPushToTalkKey(int vkkeys[], int size, bool block);
		CString getPushToTalkKeyStroke();
		bool getPushToTalkKeyBlock();
		void bindPlaybackMuteKey(CString &stroke, bool block);
		CString getPlaybackMuteKeyStroke();
		bool getPlaybackMuteKeyBlock();
		void bindCaptureMuteKey(CString &stroke, bool block);
		CString getCaptureMuteKeyStroke();
		bool getCaptureMuteKeyBlock();

		bool getPlaybackMute(void);
		void setPlaybackMute(bool state);
		bool getCaptureMute(void);
		void setCaptureMute(bool state);

		bool getSoftPushToTalk(void);
		void setSoftPushToTalk(bool state);

        void setEchoCancellation(bool state);
        void setEchoQueuedDelay(int value);
        void setEchoTailMultiplier(int value);

        void setRemoteEcho(bool state);

		PeerList getClients();
        int getClientCount() const;

        bool isCaptureDeviceValid() const;
        int lastCaptureDeviceError() const { return common::LastCaptureDeviceError(); }
        bool isPlaybackDeviceValid() const;
        int lastPlaybackDeviceError() const { return common::LastPlaybackDeviceError(); }

		AudioDeviceList getCaptureDevices();
		AudioDeviceList getPlaybackDevices();
		void setCaptureDevice (CString &deviceId);
		void setPlaybackDevice (CString &deviceId);
		AudioDeviceId getCaptureDevice();
		AudioDeviceId getPlaybackDevice();
		void setLoopback(bool state);
		bool getLoopback(void);
		void setAutoGain(bool state);
		bool getAutoGain(void);
		void setCaptureGain(int percent);
		int getCaptureGain(void);
		void setPlaybackGain(int percent);
		int getPlaybackGain(void);
		void setVoiceSensitivity(int percent);
		int getVoiceSensitivity(void);
		bool isDisconnected(void);
		bool isConnected(void);
		bool isConnecting(void);
		CString getUserId();
		CString getUserDesc();
		CString getChannelId();
		CString getChannelDesc();
		int getChid(void);
		Token getToken(void);
        int getNetworkQuality();

	private:
		/* IConnectionEvents */
		void onParted(CString &reasonType, CString &reasonDesc, int adminChid);
		void onConnect(CString &channelId, CString &channelDesc);
		void onDisconnect(CString &reasonType, CString &reasonDesc);
		void onClientJoined(int chid, CString &userId, CString &userDesc);
		void onClientParted(int chid, CString &userId, CString &userDesc, CString &reasonType, CString &reasonDesc);
		void onSubChannelJoined (int chid, int schid, int adminChid);
		void onSubChannelParted (int chid, int schid, int adminChid);
		void onTakeBegin();
		void onFrameBegin(long long timestamp);
		void onFrameClientPayload(int chid, const void *payload, size_t cbPayload);
        void onFrameEnd(long long timestamp = 0);
		void onTakeEnd();
		void onEchoMessage(const Message &message);
        void onChannelInactivity(unsigned long interval);
		Codec *getCodec(void);

	private:
		void processConnection(void);
		void processCapture(void);
		void processPlayback(bool runOnce);
        void printFrame(AudioFrame &frame, String message);
#ifdef SONAR_CLIENT_HAS_INPUT
		void processInput(void);
#endif
        void processStats(unsigned long long currentTime);
        void processStatsTelemetry(unsigned long long currentTime, sonar::String const& channelId);
        void sendStatsTelemetry(unsigned long long currentTime);
		void processCaptureFrame(AudioFrame &frame);
		void init(void);

		void cleanupChannel(void);
		void setError(CString &errorType, CString &errorDesc);

        void stopPlayback();

		struct
		{
			AudioPreprocess *preprocess;
			AudioDeviceId audioDevice;
			AudioCaptureDevice *device;
			CaptureMode mode;
			bool mute;
			bool loopback;
			bool autoGain;
			bool softPushToTalk;
			int vaSensitivity;
			float gain;
			int codecBitRate;
			int bitsPerSample;
			int samplesPerFrame;
			int samplesPerSecond;
			int framesPerSecond;
			int talkBalance;
			int talkBalanceMin;
			int talkBalanceMax;
			int talkBalancePenalty;
		} m_capture;

		struct
		{
			AudioPlaybackBuffer *buffer;
			AudioPlaybackDevice *device;
			AudioDeviceId audioDevice;
			bool mute;
			float gain;
			AudioFrame currentFrame;
            AudioQueue voiceQueue;
			AudioQueue loopbackQueue;
            int mixCount;
            UINT32 frameCounter; // corresponds to and changes with 'currentFrame'
		} m_playback;
		
#ifdef SONAR_CLIENT_HAS_INPUT
		struct
		{
			struct
			{
				int id;
				String stroke;
				bool block;
				bool active;
			} pushToTalk;

			struct
			{
				int id;
				String stroke;
				bool block;
				bool active;
			} captureMute;
			struct
			{
				int id;
				String stroke;
				bool block;
				bool active;
			} playbackMute;
		} m_binds;
#endif

		typedef SonarMap<int, Peer *> PeerMap;
		typedef SonarMap<int, bool> TalkersList;

		struct
		{
			String channelId;
			String channelDesc;
			Codec *codec;
			PeerMap peers;
			bool isTransmitting;

			bool talkers[sonar::protocol::CHANNEL_MAX_CLIENTS + 1];
			TalkersList oldTalkers;

			bool remoteEcho;

		} m_channel;

		AudioRuntime m_runtime;

		ClientConfig m_config;

		String m_errorType;
		String m_errorDesc;

		typedef SonarVector<IClientEvents *> EventListeners;
		EventListeners m_events;

	protected:
#ifdef SONAR_CLIENT_HAS_INPUT
		Input m_input;
#endif
        DefaultOptions m_defaultOptions;
        Options& m_options;
		Connection m_conn;

        bool mTiming;
        unsigned long long mStartingTime;
        unsigned long long mTime;
        unsigned long long mPreviousStatsUploadTime;
        unsigned long long mPreviousStatsTelemetryUploadTime;
	};

    // inlines

    inline int Client::getClientCount() const {
        return m_channel.peers.size();
    }
}
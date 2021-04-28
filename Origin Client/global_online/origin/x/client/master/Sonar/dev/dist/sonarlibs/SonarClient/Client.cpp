#include <SonarClient/Client.h>
#include <SonarClient/ClientConfig.h>
#include <SonarCommon/Common.h>
#include <SonarAudio/dscommon.h>
#include <math.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#if _WIN32
#include <ctime>
#endif
namespace sonar
{

static DebugLogger s_logger;
static TimeProvider s_timeProvider;
static NetworkProvider s_networkProvider;

const static float CAPTURE_MAX_GAIN = 8.0f;
const static float PLAYBACK_MAX_GAIN = 8.0f;
const unsigned long long MIN_TALKING_TIME = 600000; // 10 minutes 

const unsigned int STATS_UPLOAD_FREQUENCY_IN_SECS = 60; // upload stats every 60 seconds
const unsigned int STATS_TELEMETRY_UPLOAD_FREQUENCY_IN_SECS = 30; // upload stats every 30 seconds

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

static int PLAYBACK_GAIN_TO_PERCENT(float gain)
{
	gain /= PLAYBACK_MAX_GAIN;
	gain *= 100.0f;

	int percent = (int) ceil(gain);

	return min(percent, 100);
}

static float PLAYBACK_PERCENT_TO_GAIN(int percent)
{
	float gain = (float) percent;
	gain = gain * (CAPTURE_MAX_GAIN / 100.0f);

	return gain;
}


#define CAPTURE_GAIN_TO_PERCENT(_gain) PLAYBACK_GAIN_TO_PERCENT(_gain)
#define CAPTURE_PERCENT_TO_GAIN(_percent) PLAYBACK_PERCENT_TO_GAIN(_percent)

std::ofstream playbackSpeexLog;
std::ofstream captureSpeexLog;
std::ofstream echoSpeexLog;

std::ofstream captureDecodedSpeexLog;
Codec* decodedSpeexCodec = NULL;

Client::Client(Options* opts, ConnectionOptions connectionOptions, IBlockedListProvider* blockedListProvider)
	: m_options(opts ? *opts : m_defaultOptions)
    , m_conn(s_timeProvider, s_networkProvider, s_logger, connectionOptions, 0, &m_options.stats(), blockedListProvider)
    , mPreviousStatsUploadTime(0)
    , mPreviousStatsTelemetryUploadTime(0)
{
    common::ENABLE_CAPTURE_PCM_LOGGING = options().enableCapturePcmLogging();
    common::ENABLE_CAPTURE_SPEEX_LOGGING = options().enableCaptureSpeexLogging();
    common::ENABLE_PLAYBACK_DSOUND_LOGGING = options().enablePlaybackDsoundLogging();
    common::ENABLE_PLAYBACK_SPEEX_LOGGING = options().enablePlaybackSpeexLogging();
    common::ENABLE_CLIENT_SETTINGS_LOGGING = options().enableClientSettingsLogging();
    common::ENABLE_CLIENT_DSOUND_TRACING = options().enableClientDSoundTracing();
    common::ENABLE_CLIENT_MIXER_STATS = options().enableClientMixerStats();
    common::ENABLE_SONAR_TIMING_LOGGING = options().enableSonarTimingLogging();
    common::ENABLE_CONNECTION_STATS = options().enableConnectionStatsLogging();
    common::ENABLE_JITTER_TRACING = options().enableSonarJitterTracing();
    common::ENABLE_PAYLOAD_TRACING = options().enableSonarPayloadTracing();
    common::JITTER_METRICS_LOG_LEVEL = options().sonarJitterMetricsLogLevel();
    common::ENABLE_SPEEX_ECHO_CANCELLATION = options().enableSpeexEchoCancellation();
    common::SPEEX_ECHO_QUEUE_DELAY = options().speexEchoQueuedDelay();
    common::SPEEX_ECHO_TAIL_MULTIPLIER = options().speexEchoTailMultiplier();
    internal::sonarTestingAudioError = options().qaTestingAudioError();
    internal::sonarTestingRegisterPacketLossPercentage = options().qaTestingRegisterPacketLossPercentage();
    internal::sonarTestingVoiceServerRegisterResponse = options().qaTestingVoiceServerRegisterResponse();

	m_channel.codec = NULL;
	m_channel.remoteEcho = false;
	m_capture.codecBitRate = 12000;
    m_capture.samplesPerSecond = protocol::SAMPLES_PER_SECOND;
	m_capture.bitsPerSample = 16;
    m_capture.samplesPerFrame = protocol::SAMPLES_PER_FRAME;
    m_capture.framesPerSecond = protocol::FRAMES_PER_SECOND;
	m_capture.device = NULL;
	m_capture.mode = PushToTalk;
	m_capture.mute = false;
	m_capture.loopback = false;
	m_capture.preprocess = NULL;
	m_capture.autoGain = true;
	m_capture.vaSensitivity = 25;
	m_capture.gain = 1.0f;
	m_capture.softPushToTalk = false;
	m_capture.talkBalance = 0;
	m_playback.buffer = NULL;
	m_playback.device = NULL;
	m_playback.mute = false;
	m_playback.gain = 1.0f;
#ifdef SONAR_CLIENT_HAS_INPUT
	m_binds.pushToTalk.id = -1;
	m_binds.captureMute.id = -1;
	m_binds.playbackMute.id = -1;
	m_binds.pushToTalk.block = false;
	m_binds.captureMute.block = false;
	m_binds.playbackMute.block = false;
#endif
	m_conn.setEventSink(this);

	init();

    if (common::ENABLE_PLAYBACK_SPEEX_LOGGING) {
        static int index = 0;
        std::ostringstream filename;
        filename << "sonar_playback_speex_" << index++ << ".log";
        playbackSpeexLog.open(filename.str(), std::ios::out | std::ios::trunc | std::ios::binary);
    }

    if (common::ENABLE_CAPTURE_SPEEX_LOGGING) {
        static int index = 0;
        std::ostringstream filename;
        filename << "sonar_capture_speex_" << index++ << ".log";
        captureSpeexLog.open(filename.str(), std::ios::out | std::ios::trunc | std::ios::binary);
    }

    if (common::ENABLE_CAPTURE_DECODED_SPEEX_LOGGING) {
        static int index = 0;
        std::ostringstream filename;
        filename << "sonar_capture_decoded_speex_" << index++ << ".log";
        captureDecodedSpeexLog.open(filename.str(), std::ios::out | std::ios::trunc | std::ios::binary);
    }
}

const static float DYNAMIC_RANGE = 16000.0f;

Client::~Client(void)
{
    if (common::ENABLE_PLAYBACK_SPEEX_LOGGING)
        playbackSpeexLog.close();

    if (common::ENABLE_CAPTURE_SPEEX_LOGGING)
        captureSpeexLog.close();

    if (common::ENABLE_CAPTURE_DECODED_SPEEX_LOGGING)
        captureDecodedSpeexLog.close();

	if (m_capture.device)
	{
		delete m_capture.device;
		m_capture.device = NULL;
	}

	if (m_capture.preprocess)
	{
		delete m_capture.preprocess;
		m_capture.preprocess = NULL;
	}

	if (m_playback.buffer)
	{
		delete m_playback.buffer;
		m_playback.buffer = NULL;
	}

	if (m_playback.device)
	{
		delete m_playback.device;
		m_playback.device = NULL;
	}

	cleanupChannel();
}

void Client::setError(CString &errorType, CString &errorDesc)
{
    common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SClient", errorType.c_str(), 0);
	m_errorType = errorType;
	m_errorDesc = errorDesc;
}

void Client::setConfig(const ClientConfig &config)
{
	m_config = config;
	init();
}

ClientConfig Client::getConfig (void)
{
	return m_config;
}

void Client::init(void)
{
	EventListeners eventListenersCopy = m_events;
	m_events.clear();

	setSoftPushToTalk(false);
	setPlaybackGain(PLAYBACK_GAIN_TO_PERCENT(m_config.get("playbackGain", 1.0f)));
	setCaptureGain(CAPTURE_GAIN_TO_PERCENT(m_config.get("captureGain", 1.0f)));
	setVoiceSensitivity((int) m_config.get("voiceSensitivity", 98));

	setAutoGain(m_config.getBool("captureAutoGain", true));
	setCaptureDevice(m_config.get("captureDeviceId", "DEFAULT"));
	setPlaybackDevice(m_config.get("playbackDeviceId", "DEFAULT"));

	CaptureMode captureMode = (CaptureMode) m_config.get("captureMode", (int) PushToTalk);
	setCaptureMode(captureMode);

	bindPlaybackMuteKey(m_config.get("playbackMuteStroke", ""), m_config.getBool("playbackMuteBlock", false)); 
	bindCaptureMuteKey(m_config.get("captureMuteStroke", ""), m_config.getBool("captureMuteBlock", false)); 
	bindPushToTalkKey(m_config.get("pushToTalkStroke", ""), m_config.getBool("pushToTalkBlock", false)); 

	m_capture.talkBalanceMax = m_config.get("talkBalanceMax", protocol::IN_TAKE_MAX_FRAMES);
	m_capture.talkBalancePenalty = m_config.get("talkBalancePenalty", m_capture.talkBalanceMax);
	m_capture.talkBalanceMin = m_config.get("talkBalanceMin", m_capture.talkBalanceMax / 2);

	m_channel.remoteEcho = m_config.getBool("remoteEcho", false);

	m_events = eventListenersCopy;
    mTiming = false;
    mTime = NULL;
}

Codec *Client::getCodec(void)
{
	return new Codec(m_options, m_capture.codecBitRate, m_capture.samplesPerSecond, m_capture.samplesPerFrame);
}


void Client::processConnection(void)
{
	m_conn.poll();
}

void Client::processCaptureFrame(AudioFrame &frame)
{
	bool xmit, vaState, clip;
	float avg, peak;

	bool hotkeyState = 
#ifdef SONAR_CLIENT_HAS_INPUT
		m_input.isActive(m_binds.pushToTalk.id) ||
#endif 
		m_capture.softPushToTalk;

	if (m_capture.mode != PushToTalk)
	{
		hotkeyState = false;
	}

    if (common::ENABLE_CLIENT_MIXER_STATS)
    {
        float min_sample = 32767.0f;
        float max_sample = -32768.0f;
        AudioFrame::iterator end(frame.end());
        for (AudioFrame::iterator i(frame.begin()); i != end; ++i)
        {
            if (*i < min_sample)
            {
                min_sample = *i;
            }
            else if (*i > max_sample)
            {
                max_sample = *i;
            }
        }

        if (min_sample < m_options.stats().capturePreGainMin())
            m_options.stats().capturePreGainMin() = min_sample;
        if (max_sample > m_options.stats().capturePreGainMax())
            m_options.stats().capturePreGainMax() = max_sample;
        if (min_sample < -32768.0f || min_sample > 32767.0)
            ++m_options.stats().capturePreGainClip();
    }

	m_capture.preprocess->denoise(frame);

	if (m_capture.autoGain)
	{
		m_capture.preprocess->autoGainPath(
			frame, 
			m_capture.mode == VoiceActivation,
			hotkeyState, 
			m_capture.mute, 
			m_capture.framesPerSecond, 
			m_capture.vaSensitivity, 
			xmit, vaState, clip, avg, peak);
	}
	else
	{
		m_capture.preprocess->manualGainPath(
			frame, 
			m_capture.mode == VoiceActivation, 
			hotkeyState, 
			m_capture.mute, 
			m_capture.framesPerSecond, 
			m_capture.vaSensitivity, 
            m_options.speexSettingAgcEnable(),
			m_capture.gain, 
			xmit, vaState, clip, avg, peak);
	}
	
    if (common::ENABLE_CLIENT_MIXER_STATS)
    {
        float min_sample = 32767.0f;
        float max_sample = -32768.0f;
        AudioFrame::iterator end(frame.end());
        for (AudioFrame::iterator i(frame.begin()); i != end; ++i)
        {
            if (*i < min_sample)
            {
                min_sample = *i;
            }
            else if (*i > max_sample)
            {
                max_sample = *i;
            }
        }

        if (min_sample < m_options.stats().capturePostGainMin())
            m_options.stats().capturePostGainMin() = min_sample;
        if (max_sample > m_options.stats().capturePostGainMax())
            m_options.stats().capturePostGainMax() = max_sample;
        if (min_sample < -32768.0f || max_sample > 32767.0)
            ++m_options.stats().capturePostGainClip();
    }

	switch (m_capture.mode)
	{
	case PushToTalk:
		break;
	case VoiceActivation:
		break;
	}

	for (EventListeners::iterator iter = m_events.begin(); iter != m_events.end(); iter ++)
	{
		(*iter)->evtCaptureAudio(clip, avg, peak, vaState, xmit);
	}

	if (m_capture.loopback)
	{
		m_playback.loopbackQueue.enqueue(frame);
	}

	if (!m_conn.isConnected())
	{
		return;
	}

	if (xmit)
	{
		if (!m_channel.isTransmitting)
		{
#if ENABLE_TALK_BALANCE
			if (m_capture.talkBalance < m_capture.talkBalanceMin)
#endif
			{
				m_channel.isTransmitting = true;
#if ENABLE_TALK_BALANCE
                for (EventListeners::iterator iter = m_events.begin(); iter != m_events.end(); iter ++)
                    (*iter)->evtTalkTimeOverLimit(false);
#endif
			}
		}
	} 
	else
	{
		if (m_channel.isTransmitting)
		{
			m_conn.sendAudioStop();
			m_channel.isTransmitting = false;
		}
	}

	if (m_channel.isTransmitting)
	{
#if _WIN32
        if (!mTiming)
        {
            mStartingTime = std::clock();
            mTiming = true;
        }
#endif
#if ENABLE_TALK_BALANCE
		if (m_capture.talkBalance < m_capture.talkBalanceMax)
		{
			m_capture.talkBalance ++;
		}

		if (m_capture.talkBalance == m_capture.talkBalanceMax)
		{
			m_capture.talkBalance = m_capture.talkBalanceMax  + m_capture.talkBalancePenalty;
			
			m_conn.sendAudioStop();
			m_channel.isTransmitting = false;

            for (EventListeners::iterator iter = m_events.begin(); iter != m_events.end(); iter ++)
                (*iter)->evtTalkTimeOverLimit(true);
		}
#endif
	}
	else
	{
#if _WIN32
        if (mTiming)
        {
            mTime = std::clock() - mStartingTime;
            mTiming = false;
        }
        if (mTime)
        {
            if (mTime > MIN_TALKING_TIME)// only send telemetry if the user was talking for longer than this
            {
                if (common::ENABLE_CLIENT_SETTINGS_LOGGING)
                    common::Log("SONAR: User stopped transmitting total time at: %d", mTime);

                common::Telemetry(
                    sonar::SONAR_TELEMETRY_STAT,
                    "Timing",
                    "long-talk ms",
                    static_cast<unsigned long>(mTime));
            }

            mTime = 0;
        }
#endif
#if ENABLE_TALK_BALANCE
		m_capture.talkBalance --;

		if (m_capture.talkBalance < 0)
			m_capture.talkBalance = 0;
#endif
	}
	
	if (!m_channel.isTransmitting)
	{
		return;
	} 
	
	Codec::Payload payload;

    // frame (pcm) -> payload (speex)
	m_channel.codec->encodeAudio(frame, payload);

    if (common::ENABLE_CLIENT_MIXER_STATS)
    {
        static float min_sample = 0.0f;
        static float max_sample = 0.0f;
        AudioFrame::iterator end(frame.end());
        for (AudioFrame::iterator i(frame.begin()); i != end; ++i)
        {
            if (*i < min_sample)
            {
                min_sample = *i;
            }
            else if (*i > max_sample)
            {
                max_sample = *i;
            }
        }
    }

	if (!payload.empty())
    {
        if (common::ENABLE_CAPTURE_SPEEX_LOGGING)
            captureSpeexLog.write(reinterpret_cast<const char*>(payload.data()), payload.size() * sizeof(Codec::Payload::value_type));

        if (common::ENABLE_CAPTURE_DECODED_SPEEX_LOGGING) {
            AudioFrame decodedFrame;
            decodedSpeexCodec->decodeAudio(payload, decodedFrame);
            // normalize
            for (auto i = decodedFrame.begin(); i != decodedFrame.end(); ++i)
                *i = *i / 32768.0f;
            captureDecodedSpeexLog.write(reinterpret_cast<char const*>(&decodedFrame[0]), decodedFrame.size() * sizeof(AudioFrame::value_type));
        }

        m_conn.sendAudioPayload(&payload[0], payload.size(), frame.timestamp());
    }
}


void Client::processCapture(void)
{
	AudioFrame currFrame;

	if (!m_capture.device)
	{
		return;
	}

    // get one frame of pcm data from DirectSound capture buffer
	CaptureResult result = m_capture.device->poll(currFrame);

	switch (result)
	{
	case CR_DeviceLost:
        ++m_options.stats().captureDeviceLost();
        return;

    case CR_NoAudio:
        ++m_options.stats().captureNoAudio();
		return;

	case CR_Success:
		break;
	}

	processCaptureFrame(currFrame);
}

void Client::processPlayback(bool runOnce)
{
	if (!m_playback.device || !m_playback.buffer)
	{
		return;
	}

	for (;;)
	{
		PlaybackResult result = m_playback.buffer->query();

		switch (result)
		{
		case PR_DeviceLost:
            ++m_options.stats().playbackDeviceLost();
			return;
		case PR_Overflow:
            ++m_options.stats().playbackOverflow();
            return;

		case PR_Underrun:
		case PR_Success:
			break;
		}

        // Mix a frame
        long long ts = common::getTimeAsMSEC();
        AudioFrame currFrame(ts, m_capture.samplesPerFrame, 0.0f);
		AudioFrame frame(ts, m_capture.samplesPerFrame, 0.0f);
		AudioQueue *queues[] = { &m_playback.loopbackQueue };

		bool underrun = false;
        bool wasClipped = false;
        bool aggregateClipped = false;

        if (m_playback.voiceQueue.deque(frame, underrun))
		{
			if (underrun)
			{
                ++m_options.stats().playbackUnderrun();

				AudioFrame fakeFrame(0, m_capture.samplesPerFrame, 0.0f);
				Codec::Payload payload;

				for (PeerMap::iterator iter = m_channel.peers.begin(); iter != m_channel.peers.end(); iter ++)
				{
					Peer *peer = iter->second;
					if (!peer->getTalking())
						continue;

					peer->getCodec().decodeAudio(payload, fakeFrame);
					AudioPreprocess::mix(frame, fakeFrame, wasClipped);
                    aggregateClipped |= wasClipped;
				}
			}

			AudioPreprocess::mix(currFrame, frame, wasClipped);
            currFrame.timestamp(frame.timestamp());
            aggregateClipped |= wasClipped;
		}

		for (int index = 0; index < sizeof(queues) / sizeof(AudioQueue *); index ++)
		{
			AudioQueue *queue = queues[index];
			
			bool underrun;

			if (!queue->deque(frame, underrun))
			{
				continue;
			}

			if (!m_playback.mute)
			{
				AudioPreprocess::mix(currFrame, frame, wasClipped);
                aggregateClipped |= wasClipped;
			}
			queue ++;
		}

        if (wasClipped)
            ++m_options.stats().audioMixClipping();

        if (common::ENABLE_CLIENT_MIXER_STATS)
        {
            float frame_min = 32767.0;
            float frame_max = -32768.0;
            AudioFrame::iterator end(currFrame.end());
            for (AudioFrame::iterator i(currFrame.begin()); i != end; ++i)
            {
                if (*i > frame_max)
                    frame_max = *i;
                else if (*i < frame_min)
                    frame_min = *i;
            }

            if (frame_min < m_options.stats().playbackPreGainMin())
                m_options.stats().playbackPreGainMin() = frame_min;
            if (frame_max > m_options.stats().playbackPreGainMax())
                m_options.stats().playbackPreGainMax() = frame_max;

            if (frame_min < -32768.0 || frame_max > 32767.0)
            {
                ++m_options.stats().playbackPreGainClip();
            }
        }

        AudioPreprocess::gain(currFrame, m_playback.gain);

        if (common::ENABLE_CLIENT_MIXER_STATS)
        {
            float frame_min = 32767.0;
            float frame_max = -32768.0;
            AudioFrame::iterator end(currFrame.end());
            for (AudioFrame::iterator i(currFrame.begin()); i != end; ++i)
            {
                if (*i > frame_max)
                    frame_max = *i;
                else if (*i < frame_min)
                    frame_min = *i;
            }

            if (frame_min < m_options.stats().playbackPostGainMin())
                m_options.stats().playbackPostGainMin() = frame_min;
            if (frame_max > m_options.stats().playbackPostGainMax())
                m_options.stats().playbackPostGainMax() = frame_max;

            if (frame_min < -32768.0 || frame_max > 32767.0)
        {
                ++m_options.stats().playbackPostGainClip();
            }
        }

        // Store the playback frame for echo cancellation purpose
        if (m_capture.preprocess)
            m_capture.preprocess->pushEchoFrame(currFrame);

		m_playback.buffer->enqueue(currFrame);

        if (runOnce)
            return;
	}
}

#ifdef SONAR_CLIENT_HAS_INPUT
void Client::processInput(void)
{
	Stroke stroke;

	if (m_input.getRecording(stroke))
	{
		for (EventListeners::iterator iter = m_events.begin(); iter != m_events.end(); iter ++)
			(*iter)->evtStrokeRecorded(stroke.toString());
	}

	bool newState;

	newState = m_input.isActive(m_binds.playbackMute.id);

	if (m_binds.playbackMute.active != newState && newState)
	{
		setPlaybackMute(!getPlaybackMute());
	}
	m_binds.playbackMute.active = newState;

	newState = m_input.isActive(m_binds.captureMute.id);

	if (m_binds.captureMute.active != newState && newState)
	{
		setCaptureMute(!getCaptureMute());
	}
	m_binds.captureMute.active = newState;


    newState = m_input.isActive(m_binds.pushToTalk.id);
    if (m_binds.pushToTalk.active != newState)
    {
        m_binds.pushToTalk.active = newState;
        for (EventListeners::iterator iter = m_events.begin(); iter != m_events.end(); iter ++)
            (*iter)->evtPushToTalkActiveChanged(newState);
    }
}
#endif


void Client::update()
{
	processConnection();

#ifdef SONAR_CLIENT_HAS_INPUT	
	processInput();
#endif

    processPlayback(common::ENABLE_SPEEX_ECHO_CANCELLATION);
    processCapture();

    // process the periodic statistics
    common::Timestamp currentTime = s_timeProvider.getMSECTime();
    if (currentTime > mPreviousStatsUploadTime + STATS_UPLOAD_FREQUENCY_IN_SECS * 1000)
    {
        // get the connection to update its stats
        m_conn.prepareStatsForTransmission();

        processStats(currentTime);
        mPreviousStatsUploadTime = currentTime;
    }

    // process the periodic stats telemetry
    if (currentTime > mPreviousStatsTelemetryUploadTime + STATS_TELEMETRY_UPLOAD_FREQUENCY_IN_SECS * 1000)
    {
        // if we didn't just prepare the stats, do it now
        if (mPreviousStatsUploadTime != currentTime) {
            // get the connection to update its stats
            m_conn.prepareStatsForTransmission();
        }

        processStatsTelemetry(currentTime, m_channel.channelId);
        mPreviousStatsTelemetryUploadTime = currentTime;
}
}

void Client::processStats(unsigned long long currentTime)
{
    // calculate the audio stats
    float deltaPlaybackMean;
    float deltaPlaybackMax;
    float receiveToPlaybackMean;
    float receiveToPlaybackMax;
    m_playback.buffer->calculateStats(deltaPlaybackMean, deltaPlaybackMax, receiveToPlaybackMean, receiveToPlaybackMax);
    m_options.stats().deltaPlaybackMean() = static_cast<unsigned long>(deltaPlaybackMean + 0.5f);
    m_options.stats().deltaPlaybackMax() = static_cast<unsigned long>(deltaPlaybackMax);
    m_options.stats().receiveToPlaybackMean() = static_cast<unsigned long>(receiveToPlaybackMean + 0.5f);
    m_options.stats().receiveToPlaybackMax() = static_cast<unsigned long>(receiveToPlaybackMax);

    // send the stats
    m_conn.sendClientStats(currentTime);
}

void Client::processStatsTelemetry(unsigned long long currentTime, sonar::String const& channelId)
{
    // send the stats
    m_conn.sendClientStatsTelemetry(currentTime, channelId);
}

bool Client::connect(CString &token)
{
    Connection::Result result = m_conn.connect(token);

	switch(result)
	{
	case Connection::InvalidState: setError("INVALID_STATE", ""); return false;
	case Connection::InvalidToken: setError("INVALID_TOKEN", ""); return false;
	case Connection::Success: break;
	}

	return true;
}

bool Client::disconnect(sonar::CString &reasonType, sonar::CString &reasonDesc)
{
    stopPlayback();

    // reset errors cache when a user manually disconnects
    if ((reasonType.find("User disconnection") != sonar::CString::npos) &&
        (reasonDesc.find("leaveVoice") != sonar::CString::npos)
       )
    {
        clearErrorsSent();
    }

    processStats(common::getTimeAsMSEC());

    // This handles the case where a user was disconnected while they were talking
    if (mTiming)
    {
        mTime = std::clock() - mStartingTime;
        mTiming = false;
    }
    if (mTime)
    {
        if (mTime > MIN_TALKING_TIME)// only send telemetry if the user was talking for longer than this
        {
            if (common::ENABLE_CLIENT_SETTINGS_LOGGING)
                common::Log("SONAR: User stopped transmitting total time at: %d", mTime);

            common::Telemetry(
                sonar::SONAR_TELEMETRY_STAT,
                "Timing",
                "call duration ms",
                static_cast<unsigned long>(mTime));
        }
        mTime = NULL;
    }

	Connection::Result result =  m_conn.disconnect(reasonType, reasonDesc);

	switch(result)
	{
	case Connection::InvalidState: setError("INVALID_STATE", ""); return false; break;
	case Connection::Success: break;
	}

	return true;
}

void Client::muteClient(CString &userId, bool state)
{
	for (PeerMap::iterator iter = m_channel.peers.begin(); iter != m_channel.peers.end(); iter ++)
	{
		if (iter->second->getPeerInfo().userId == userId)
		{
			muteClient(iter->second->getPeerInfo().chid, state);
		}
	}
}

bool Client::isClientMuted(int chid)
{
	PeerMap::iterator iter = m_channel.peers.find(chid);

	if (iter == m_channel.peers.end())
	{
		return false;
	}

	return iter->second->getPeerInfo().isMuted;

}

void Client::muteClient(int chid, bool state)
{
	PeerMap::iterator iter = m_channel.peers.find(chid);

	if (iter == m_channel.peers.end())
	{
		return;
	}

	iter->second->setMuted(state);

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtClientMuted(iter->second->getPeerInfo());
}

bool Client::getSoftPushToTalk(void)
{
	return m_capture.softPushToTalk;
}

void Client::setSoftPushToTalk(bool state)
{
	m_capture.softPushToTalk = state;
}

void Client::setCaptureMode(CaptureMode mode)
{
    if (common::ENABLE_CLIENT_SETTINGS_LOGGING)
        common::Log("SONAR: set capture mode=%d", mode);

	switch (mode)
	{
	case PushToTalk:
		break;

	case VoiceActivation:
		break;
	}

	m_capture.mode = mode;
	m_config.set("captureMode", (int) m_capture.mode);

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtCaptureModeChanged(m_capture.mode);


}

CaptureMode Client::getCaptureMode()
{
	return m_capture.mode;
}

void Client::recordStroke()
{
#ifdef SONAR_CLIENT_HAS_INPUT
	m_input.beginRecording();
#endif
}

CString Client::getPushToTalkKeyStroke()
{
#ifdef SONAR_CLIENT_HAS_INPUT
	return m_binds.pushToTalk.stroke;
#else
	return "";
#endif
}

bool Client::getPushToTalkKeyBlock()
{
#ifdef SONAR_CLIENT_HAS_INPUT
	return m_binds.pushToTalk.block;
#else
	return false;
#endif
}

CString Client::getPlaybackMuteKeyStroke()
{
#ifdef SONAR_CLIENT_HAS_INPUT
	return m_binds.playbackMute.stroke;
#else
	return "";
#endif
}

bool Client::getPlaybackMuteKeyBlock()
{
#ifdef SONAR_CLIENT_HAS_INPUT
	return m_binds.playbackMute.block;
#else
	return false;
#endif
}

CString Client::getCaptureMuteKeyStroke()
{
#ifdef SONAR_CLIENT_HAS_INPUT
	return m_binds.captureMute.stroke;
#else
	return "";
#endif
}

bool Client::getCaptureMuteKeyBlock()
{
#ifdef SONAR_CLIENT_HAS_INPUT
	return m_binds.captureMute.block;
#else
	return false;
#endif
}

void Client::bindCaptureMuteKey(CString &_stroke, bool block)
{
#ifdef SONAR_CLIENT_HAS_INPUT
	if (m_binds.captureMute.id != -1)
		m_input.unbind(m_binds.captureMute.id);
	
	m_binds.captureMute.id = -1;
	m_binds.captureMute.active = false;
	m_binds.captureMute.block = block;

	Stroke stroke(_stroke, block);
	m_binds.captureMute.id = m_input.bind(stroke);
	m_binds.captureMute.stroke = stroke.toString();

	m_config.set("captureMuteStroke", m_binds.captureMute.stroke);
	m_config.set("captureMuteBlock", m_binds.captureMute.block);

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtCaptureMuteKeyChanged(stroke.toString(), block);
#endif
}

void Client::bindPlaybackMuteKey(CString &_stroke, bool block)
{
#ifdef SONAR_CLIENT_HAS_INPUT
	if (m_binds.playbackMute.id != -1)
		m_input.unbind(m_binds.playbackMute.id);

	m_binds.playbackMute.id = -1;
	m_binds.playbackMute.active = false;
	m_binds.playbackMute.block = block;

	Stroke stroke(_stroke, block);
	m_binds.playbackMute.id = m_input.bind(stroke);
	m_binds.playbackMute.stroke = stroke.toString();

	m_config.set("playbackMuteStroke", m_binds.playbackMute.stroke);
	m_config.set("playbackMuteBlock", m_binds.playbackMute.block);

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtPlaybackMuteKeyChanged(stroke.toString(), block);
#endif
}

void Client::bindPushToTalkKey(CString &_stroke, bool block)
{
#ifdef SONAR_CLIENT_HAS_INPUT
	if (m_binds.pushToTalk.id != -1)
		m_input.unbind(m_binds.pushToTalk.id);

	m_binds.pushToTalk.id = -1;
	m_binds.pushToTalk.active = false;
	m_binds.pushToTalk.block = block;

	Stroke stroke(_stroke, block);
	m_binds.pushToTalk.id = m_input.bind(stroke);
	m_binds.pushToTalk.stroke = stroke.toString();

	m_config.set("pushToTalkStroke", m_binds.pushToTalk.stroke);
	m_config.set("pushToTalkBlock", m_binds.pushToTalk.block);


	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtPushToTalkKeyChanged(stroke.toString(), block);
#endif
}

void Client::bindPushToTalkKey(int vkkeys[], int size, bool block)
{
#ifdef SONAR_CLIENT_HAS_INPUT
    if (m_binds.pushToTalk.id != -1)
        m_input.unbind(m_binds.pushToTalk.id);

    m_binds.pushToTalk.id = -1;
    m_binds.pushToTalk.active = false;
    m_binds.pushToTalk.block = block;

    Stroke stroke(vkkeys, size, block); // use virtual keys instead of text -> scan code
    m_binds.pushToTalk.id = m_input.bind(stroke);
    m_binds.pushToTalk.stroke = stroke.toString();

    m_config.set("pushToTalkStroke", m_binds.pushToTalk.stroke);
    m_config.set("pushToTalkBlock", m_binds.pushToTalk.block);

    for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
        (*eviter)->evtPushToTalkKeyChanged(stroke.toString(), block);
#endif
}

bool Client::getPlaybackMute(void)
{
	return m_playback.mute;
}

void Client::setPlaybackMute(bool state)
{
	m_playback.mute = state;

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtPlaybackMuteChanged(state);
}

bool Client::getCaptureMute(void)
{
	return m_capture.mute;
}

void Client::setCaptureMute(bool state)
{
    if (common::ENABLE_CLIENT_SETTINGS_LOGGING)
        common::Log("SONAR: set capture mute=%s", state ? "true" : "false");

	m_capture.mute = state;

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtCaptureMuteChanged(state);
}

PeerList Client::getClients()
{
	PeerList ret;

	for (PeerMap::iterator iter = m_channel.peers.begin(); iter != m_channel.peers.end(); iter ++)
	{
		ret.push_back(iter->second->getPeerInfo());
	}

	return ret;
}

bool Client::isCaptureDeviceValid() const
{
    return m_capture.device != NULL;
}

bool Client::isPlaybackDeviceValid() const
{
    return (m_playback.device != NULL) && (m_playback.buffer != NULL);
}

AudioDeviceList Client::getCaptureDevices()
{
	AudioCaptureEnum deviceEnum;
	return deviceEnum.getDevices();
}

AudioDeviceList Client::getPlaybackDevices()
{
	AudioPlaybackEnum deviceEnum;
	return deviceEnum.getDevices();
}

void Client::setCaptureDevice (CString &deviceId)
{
    if (common::ENABLE_CLIENT_SETTINGS_LOGGING)
        common::Log("SONAR: set capture device=%s", deviceId.c_str());

	if (m_capture.device)
	{
		delete m_capture.device;
		delete m_capture.preprocess;
		m_capture.device = NULL;
        m_capture.preprocess = NULL;
	}

	AudioCaptureEnum captureEnum;

	const AudioDeviceList &deviceList = captureEnum.getDevices();

	const AudioDeviceId *pDevice = NULL;

	for (AudioDeviceList::const_iterator iter = deviceList.begin(); iter != deviceList.end(); iter ++)
	{
		const AudioDeviceId &device = *iter;

		if (device.id == deviceId)
		{
			pDevice = &device;
			break;
		}
	}

	m_capture.audioDevice.id = "";
	m_capture.audioDevice.name = "";

	if (pDevice)
	{
		m_capture.device = new AudioCaptureDevice(m_runtime, *pDevice, 
			m_capture.samplesPerSecond, 
			m_capture.samplesPerFrame,
			m_capture.bitsPerSample,
			m_capture.framesPerSecond);

		if (m_capture.device->isCreated())
		{
			m_capture.audioDevice = *pDevice;
			m_capture.preprocess = new AudioPreprocess(m_options, m_capture.samplesPerFrame, m_capture.samplesPerSecond, DYNAMIC_RANGE);
	        m_config.set("captureDeviceId", m_capture.audioDevice.id);
		} 
		else
		{
			delete m_capture.device;
			m_capture.device = NULL;
		}
	}

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtCaptureDeviceChanged(m_capture.audioDevice.id, m_capture.audioDevice.name);
}

void Client::setPlaybackDevice (CString &deviceId)
{
    if (common::ENABLE_CLIENT_SETTINGS_LOGGING)
        common::Log("SONAR: set playback device=%s", deviceId.c_str());

	if (m_playback.device)
	{
		if (m_playback.buffer)
		{
			delete m_playback.buffer;
			m_playback.buffer = NULL;
		}

		delete m_playback.device;
		m_playback.device = NULL;

	}

	AudioPlaybackEnum playbackEnum;

	const AudioDeviceList &deviceList = playbackEnum.getDevices();

	const AudioDeviceId *pDevice = NULL;
	String deviceName;

	for (AudioDeviceList::const_iterator iter = deviceList.begin(); iter != deviceList.end(); iter ++)
	{
		const AudioDeviceId &device = *iter;

		if (device.id == deviceId)
		{
			pDevice = &device;
			break;
		}
	}

	m_playback.audioDevice.id = "";
	m_playback.audioDevice.name = "";
	
	if (pDevice)
	{
		m_playback.device = new AudioPlaybackDevice(m_runtime, *pDevice, 
			m_capture.samplesPerSecond,
			m_capture.samplesPerFrame,
			m_capture.bitsPerSample);

		if (m_playback.device->isCreated())
		{
			m_playback.buffer = m_playback.device->createBuffer(m_capture.samplesPerSecond, m_capture.samplesPerFrame);

			if (!m_playback.buffer->isCreated())
			{
				delete m_playback.device;
				delete m_playback.buffer;
				m_playback.device = NULL;
				m_playback.buffer = NULL;
			}
			else
			{
				m_playback.audioDevice = *pDevice;
	            m_config.set("playbackDeviceId", m_playback.audioDevice.id);
			}
		} 
		else
		{
			delete m_playback.device;
			m_playback.device = NULL;
		}
	}

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtPlaybackDeviceChanged(m_playback.audioDevice.id, m_playback.audioDevice.name);
}

AudioDeviceId Client::getCaptureDevice()
{
	return m_capture.audioDevice;
}

AudioDeviceId Client::getPlaybackDevice()
{
	return m_playback.audioDevice;
}

void Client::setLoopback(bool state)
{
    if (common::ENABLE_CLIENT_SETTINGS_LOGGING)
        common::Log("SONAR: set loopback=%s", state ? "true" : "false");

	m_capture.loopback = state;

	if (!state)
	{
		m_playback.loopbackQueue.stop();
	}

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtLoopbackChanged(m_capture.loopback);
}

bool Client::getLoopback(void)
{
	return m_capture.loopback;
}

bool Client::isDisconnected(void)
{
	return m_conn.isDisconnected();
}

bool Client::isConnected(void)
{
	return m_conn.isConnected();
}

bool Client::isConnecting(void)
{
    return m_conn.isConnecting();
}

void Client::setAutoGain(bool state)
{
    if (common::ENABLE_CLIENT_SETTINGS_LOGGING)
        common::Log("SONAR: set auto gain=%s", state ? "true" : "false");

	m_capture.autoGain = state;

	m_config.set("captureAutoGain", m_capture.autoGain);

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtAutoGainChanged(m_capture.autoGain);
}

bool Client::getAutoGain(void)
{
	return m_capture.autoGain;
}

void Client::setCaptureGain(int percent)
{
    if (common::ENABLE_CLIENT_SETTINGS_LOGGING)
        common::Log("SONAR: set capture gain=%d", percent);

	percent = min(percent, 100);
	m_capture.gain = CAPTURE_PERCENT_TO_GAIN(percent);

	m_config.set("captureGain", m_capture.gain);

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtCaptureGainChanged(percent);
}

int Client::getCaptureGain(void)
{
	return CAPTURE_GAIN_TO_PERCENT(m_capture.gain);
}

void Client::setPlaybackGain(int percent)
{
    if (common::ENABLE_CLIENT_SETTINGS_LOGGING)
        common::Log("SONAR: set playback gain=%d", percent);

	percent = min(percent, 100);
	m_playback.gain = PLAYBACK_PERCENT_TO_GAIN(percent);

	m_config.set("playbackGain", m_playback.gain);

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtPlaybackGainChanged(percent);
}

int Client::getPlaybackGain(void)
{
	return PLAYBACK_GAIN_TO_PERCENT(m_playback.gain);
}

void Client::setVoiceSensitivity(int percent)
{
    if (common::ENABLE_CLIENT_SETTINGS_LOGGING)
        common::Log("SONAR: set voice sensitivity=%d", percent);

	percent = min(percent, 99);
	m_capture.vaSensitivity = percent;

	m_config.set("voiceSensitivity", m_capture.vaSensitivity);

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtVoiceSensitivityChanged(percent);
}

int Client::getVoiceSensitivity(void)
{
	return m_capture.vaSensitivity;
}

CString Client::getUserId()
{
	return m_conn.getToken().getUserId();
}

CString Client::getUserDesc()
{
	return m_conn.getToken().getUserDesc();
}

CString Client::getChannelId()
{
	return m_channel.channelId;
}

CString Client::getChannelDesc()
{
	return m_channel.channelDesc;
}

int Client::getChid(void)
{
	if (m_conn.isConnected())
	{
		return m_conn.getCHID();
	}

	return -1;
}

Token Client::getToken(void)
{
	return m_conn.getToken();
}

int Client::getNetworkQuality()
{
    return m_conn.getNetworkQuality();
}

void Client::stopPlayback()
{
    if (m_playback.buffer)
        m_playback.buffer->stop();
}

void Client::cleanupChannel(void)
{
	m_channel.channelId.clear();
	m_channel.channelDesc.clear();
	m_channel.isTransmitting = false;
	
	if (m_channel.codec)
	{
		delete m_channel.codec;
		m_channel.codec = NULL;
	}

	for (PeerMap::iterator iter = m_channel.peers.begin(); iter != m_channel.peers.end(); iter ++)
	{
		delete iter->second;
	}

	m_channel.peers.clear();
    m_playback.voiceQueue.stop();
}

void Client::onParted(CString &reasonType, CString &reasonDesc, int adminChid)
{
	cleanupChannel();
}

void Client::onConnect(CString &channelId, CString &channelDesc)
{
    common::Telemetry(
        sonar::SONAR_TELEMETRY_CLIENT_CONNECTED,
        channelId.c_str(),
        (options().allowHardwareTelemetry()) ? m_capture.audioDevice.name.c_str() : "",
        static_cast<char>(m_capture.autoGain),
        static_cast<short>(m_capture.gain * 100),
        static_cast<short>(100 - m_capture.vaSensitivity),
        (options().allowHardwareTelemetry()) ? m_playback.audioDevice.name.c_str() : "",
        static_cast<short>(m_playback.gain * 100),
		m_conn.voiceServerAddressIPv4(),
        m_capture.mode == CaptureMode::PushToTalk ? "ptt" : "va"
    );

	m_channel.isTransmitting = false;
	m_channel.channelId = channelId;
	m_channel.channelDesc = channelDesc;

	Codec *codec = getCodec();
    if (common::ENABLE_CAPTURE_DECODED_SPEEX_LOGGING) {
        decodedSpeexCodec = getCodec();
    }

	m_channel.codec = codec;

	Connection::ClientMap clients = m_conn.getClients();

	assert (m_channel.peers.empty());

	for (Connection::ClientMap::iterator iter = clients.begin(); iter != clients.end(); iter ++)
	{
		Codec *clientCodec = getCodec();

		Connection::ClientInfo &ci = iter->second;
		Peer *p = new Peer(ci.id, ci.userId, ci.userDesc, clientCodec);
		m_channel.peers[ci.id] = p;
	}

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtConnected(m_conn.getCHID(), channelId, channelDesc);
}

void Client::onDisconnect(sonar::CString &reasonType, sonar::CString &reasonDesc)
{
    if (common::ENABLE_CAPTURE_DECODED_SPEEX_LOGGING) {
        delete decodedSpeexCodec;
        decodedSpeexCodec = NULL;
    }

    // network stats
    NetworkStats::Stats networkStats = m_conn.getNetworkStatsOverall();
    // jitter buffer stats
    const JitterBufferOccupancy& jitterBufferOccupancyStats = m_conn.getJitterBufferOccupancyStats();
    int32_t jitterBufferOccupancyMean = 0;
    if( jitterBufferOccupancyStats.count > 0 )
        jitterBufferOccupancyMean = jitterBufferOccupancyStats.total / jitterBufferOccupancyStats.count;

    common::Telemetry(
        sonar::SONAR_TELEMETRY_CLIENT_DISCONNECTED,
        m_channel.channelId.c_str(),
        reasonType.c_str(),
        reasonDesc.c_str(),
        m_conn.stats()->audioMessagesSent(),
        m_conn.stats()->audioMessagesReceived(),
        m_conn.stats()->audioMessagesLost(),
        m_conn.stats()->audioMessagesOutOfSequence(),
        m_conn.stats()->badCid(),
        m_conn.stats()->truncatedMessages(),
        m_conn.stats()->audioMixClipping(),
        m_conn.stats()->socketRecvError(),
        m_conn.stats()->socketSendError(),
        m_conn.stats()->receiveToPlaybackMean(),
        m_conn.stats()->receiveToPlaybackMax(),
        m_conn.stats()->deltaPlaybackMean(),
        m_conn.stats()->deltaPlaybackMax(),
        m_conn.stats()->jitterArrivalMeanCumulative(),
        m_conn.stats()->jitterArrivalMaxCumulative(),
        m_conn.stats()->jitterPlaybackMeanCumulative(),
        m_conn.stats()->jitterPlaybackMaxCumulative(),
        m_conn.stats()->playbackUnderrun(),
        m_conn.stats()->playbackOverflow(),
        m_conn.stats()->playbackDeviceLost(),
        m_conn.stats()->dropNotConnected(),
        m_conn.stats()->dropReadServerFrameCounter(),
        m_conn.stats()->dropReadTakeNumber(),
        m_conn.stats()->dropTakeStopped(),
        m_conn.stats()->dropReadCid(),
        m_conn.stats()->dropNullPayload(),
        m_conn.stats()->dropReadClientFrameCounter(),
        m_conn.stats()->jitterbufferUnderrun(),
        networkStats.loss,
        networkStats.jitter,
        networkStats.ping,
        networkStats.numberOfSamples,
        networkStats.quality,
		m_conn.voiceServerAddressIPv4(),
        m_capture.mode == CaptureMode::PushToTalk ? "ptt" : "va",
        jitterBufferOccupancyMean,
        jitterBufferOccupancyStats.max
    );

	cleanupChannel();

	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtDisconnected(reasonType, reasonDesc);
}

void Client::onClientJoined(int chid, CString &userId, CString &userDesc)
{
	PeerMap::iterator peerIt = m_channel.peers.find(chid);
	assert (peerIt == m_channel.peers.end());

	if (peerIt != m_channel.peers.end())
	{
		return;
	}
	Codec *codec = getCodec();
	Peer *p = new Peer(chid, userId, userDesc, codec);

	m_channel.peers[chid] = p;


	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtClientJoined(m_channel.peers[chid]->getPeerInfo());
}

void Client::onClientParted(int chid, CString &userId, CString &userDesc, sonar::CString &reasonType, sonar::CString &reasonDesc)
{
	PeerMap::iterator peerIt = m_channel.peers.find(chid);

	if (peerIt == m_channel.peers.end())
		return;

	PeerInfo pi = peerIt->second->getPeerInfo();
	delete peerIt->second;

	m_channel.peers.erase(peerIt);
		
	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
		(*eviter)->evtClientParted(pi, reasonType, reasonDesc);
}

void Client::onSubChannelJoined (int chid, int schid, int adminChid)
{
	PeerMap::iterator peerIt = m_channel.peers.find(chid);

	if (peerIt == m_channel.peers.end())
		return;
}

void Client::onSubChannelParted (int chid, int schid, int adminChid)
{
	PeerMap::iterator peerIt = m_channel.peers.find(chid);

	if (peerIt == m_channel.peers.end())
		return;
}

void Client::onTakeBegin()
{
}

void Client::onFrameBegin(long long timestamp)
{
	m_playback.currentFrame = AudioFrame(timestamp, m_capture.samplesPerFrame, 0.0f);
    m_playback.mixCount = 0;
	memset(m_channel.talkers, 0, sizeof(m_channel.talkers));
}

void Client::onFrameClientPayload(int chid, const void *_payload, size_t cbPayload)
{
	PeerMap::iterator peerIt = m_channel.peers.find(chid);

	if (peerIt == m_channel.peers.end())
		return;

	Peer *peer = peerIt->second;

    if (peer->getMuted())
    {
        // Still want to know if the muted user is talking
        m_channel.talkers[chid] = true;
        return;
    }

	Codec::Payload payload(cbPayload);

	if (cbPayload)
	{
		memcpy(&payload[0], _payload, cbPayload);
	}

	AudioFrame peerFrame;

	m_channel.talkers[chid] = true;

	if (chid != m_conn.getCHID() || m_channel.remoteEcho)
	{
		peer->getCodec().decodeAudio(payload, peerFrame);

        if (common::ENABLE_CLIENT_MIXER_STATS)
        {
            static float min_sample = 0.0f;
            static float max_sample = 0.0f;
            AudioFrame::iterator end(peerFrame.end());
            for (AudioFrame::iterator i(peerFrame.begin()); i != end; ++i)
            {
                if (*i < min_sample)
                {
                    min_sample = *i;
                }
                else if (*i > max_sample)
                {
                    max_sample = *i;
                }
            }
        }

        bool ignoreWasClipped;
		AudioPreprocess::mix(m_playback.currentFrame, peerFrame, ignoreWasClipped);
        ++m_playback.mixCount;
	}
}

void Client::onFrameEnd(long long timestamp)
{
    if (timestamp != 0)
        m_playback.currentFrame.timestamp(timestamp);

	for (PeerMap::iterator peerIt = m_channel.peers.begin(); peerIt != m_channel.peers.end(); peerIt ++)
	{
		Peer *peer = peerIt->second;
		int chid = peerIt->first;

		if (m_channel.talkers[chid])
		{
			if (!peer->getTalking())
			{
				peer->setTalking(true);

				for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
					(*eviter)->evtClientTalking(peer->getPeerInfo());
			}
		}
		else
		{
			if (peer->getTalking())
			{
				peer->setTalking(false);

				for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
					(*eviter)->evtClientTalking(peer->getPeerInfo());
			}
		}
	}

    if (common::ENABLE_PLAYBACK_SPEEX_LOGGING) {
        // normalize the data to 0.0 - 1.0 for easier import into Audacity
        std::vector<float> normalized(m_playback.currentFrame.begin(), m_playback.currentFrame.end());
        for (auto i = normalized.begin(); i != normalized.end(); ++i) {
            *i /= 32768.0f;
        }
        playbackSpeexLog.write(reinterpret_cast<const char*>(normalized.data()), normalized.size() * sizeof(AudioFrame::value_type));
    }

    m_playback.voiceQueue.enqueue(m_playback.currentFrame);
}

void Client::onTakeEnd()
{
	AudioFrame nullFrame;
    m_playback.voiceQueue.enqueue(nullFrame);

	for (PeerMap::iterator peerIt = m_channel.peers.begin(); peerIt != m_channel.peers.end(); peerIt ++)
	{
		Peer *peer = peerIt->second;

		if (peer->getTalking())
		{
			peer->setTalking(false);

			for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
				(*eviter)->evtClientTalking(peer->getPeerInfo());
		}
	}
}
void Client::onEchoMessage(const Message &message)
{
}

void Client::onChannelInactivity(unsigned long interval)
{
   for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
       (*eviter)->evtChannelInactivity(interval);
}

void Client::addEventListener(IClientEvents *listener)
{
	m_events.push_back(listener);
}

void Client::removeEventListener(IClientEvents *listener)
{
	for (EventListeners::iterator eviter = m_events.begin(); eviter != m_events.end(); eviter ++)
	{
		if ((*eviter) == listener)
		{
			m_events.erase(eviter);
			break;
		}
	}
}

#if 0

class AudioFile
{
public:

    virtual void open() = 0;
    virtual void close() = 0;
    virtual void write() = 0;

};

class RawAudioFile
{
public:

    virtual void open();
    virtual void close();
    virtual void write();

};

class WaveAudioFile
{
public:

    virtual void open();
    virtual void close();
    virtual void write();

};


void RawAudioFile::open()
{
    stream.open();
}

void RawAudioFile::write()
{
}

void RawAudioFile::close()
{
    stream.close();
}

#endif

void Client::setEchoCancellation(bool state)
{
    options().setEchoCancellation(state);
    common::ENABLE_SPEEX_ECHO_CANCELLATION = state;
}

void Client::setEchoQueuedDelay(int value)
{
    options().setEchoQueuedDelay(value);
    common::SPEEX_ECHO_QUEUE_DELAY = value;
    if (m_capture.preprocess)
        m_capture.preprocess->setEchoDelayInFrames(value);
}

void Client::setEchoTailMultiplier(int value)
{
    options().setEchoTailMultiplier(value);
    common::SPEEX_ECHO_TAIL_MULTIPLIER = value;
}

void Client::setRemoteEcho(bool state)
{
    m_channel.remoteEcho = state;
}

void Client::printFrame(AudioFrame &frame, String message)
{
#ifndef _DEBUG
    return;
#endif

    std::vector<String> v_data;
    v_data.push_back(String(""));
    v_data.push_back(String(""));
    v_data.push_back(String(""));
    v_data.push_back(String(""));
    v_data.push_back(String(""));
    String buffer;
    int size = frame.size();
    int dataPosition = 0;
    if (size != 0)
    {
        for (int i=0;i < size; ++i)
        {
            char frameBuffer[60];
            float data = frame[i];
            int buffSize = sprintf(frameBuffer, "\t%f,\n ", data);
            if ((v_data[dataPosition].size() + buffSize) < 2048)
                v_data[dataPosition].append(frameBuffer);
    
            else
            {
                dataPosition++;
                if (dataPosition < 5)
                {
                    v_data[dataPosition].append(frameBuffer);
                }
                else
                {
                    common::Log("%s", "We need more buffers for the frame data;");
                    break;
                }
            }
        }
    }
    message += ("\t%d\n",frame.timestamp());
    // Print the message of current location
    echoSpeexLog.write(message.c_str(), message.size());

    // Print any data we may have.
    //for (int i=0; i<v_data.size();i++)
    //{
    //    if (v_data[i].size() > 0)
    //        echoSpeexLog.write(v_data[i].c_str(), v_data[i].size());
    //}
}

}

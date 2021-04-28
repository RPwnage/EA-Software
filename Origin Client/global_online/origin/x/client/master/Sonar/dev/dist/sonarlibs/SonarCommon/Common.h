#pragma once

#ifndef SONAR_COMMON_TYPES_CONFIG
#define SONAR_COMMON_TYPES_CONFIG <SonarCommon/DefaultTypesConfig.h>
#endif
#include SONAR_COMMON_TYPES_CONFIG

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <Ws2tcpip.h>

#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#endif

#include <stdint.h>
#include <sys/types.h>

#define ENABLE_TALK_BALANCE 0

#if !defined(ENABLE_VOIP_BLOCKING_TRACE)
#define ENABLE_VOIP_BLOCKING_TRACE 0
#endif

#if !defined(ENABLE_CONNECTION_TRACE)
#define ENABLE_CONNECTION_TRACE 0
#endif

#if !defined(SONAR_LOGGING_ENABLED)
#define SONAR_LOGGING_ENABLED 0
#endif

// syslog
#ifdef _WIN32
    #define OPENLOG(ident, option, facility) {}
    #define SYSLOG(priority, format, ...) {}
    #define SYSLOGX(priority, format, ...) {}
    #define CLOSELOG() {}
#else
    #define OPENLOG(ident, option, facility)  openlog(ident, option, facility);
    #define SYSLOG(priority, format, ...)  sonar::common::syslog_with_port(priority, format, ##__VA_ARGS__);
    #define SYSLOGV(priority, format, args)  sonar::common::vsyslog_with_port(priority, format, args);
    #define SYSLOGX(priority, format, ...)  sonar::common::syslog_with_port(priority, "%s %d: " format, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);
    #define CLOSELOG()  closelog();
#endif

namespace sonar
{
#ifdef _WIN32
	typedef unsigned __int64 UINT64;
	typedef unsigned __int32 UINT32;
	typedef unsigned __int16 UINT16;
	typedef unsigned __int8 UINT8;

	typedef __int64 INT64;
	typedef __int32 INT32;
	typedef __int16 INT16;
	typedef __int8 INT8;
#else
	typedef u_int64_t UINT64;
	typedef u_int32_t UINT32;
	typedef u_int16_t UINT16;
	typedef u_int8_t UINT8;
	typedef int64_t INT64;
	typedef int32_t INT32;
	typedef int16_t INT16;
	typedef int8_t INT8;

#endif

    class CriticalSection
    {
    public:

        CriticalSection();
        ~CriticalSection();

        class Locker
        {
        public:

            Locker(CriticalSection& cs);
            ~Locker();

        private:

            CriticalSection& cs;
        };

    private:

#if defined(_WIN32)
        CRITICAL_SECTION& guard();
        CRITICAL_SECTION cs;
#else
        pthread_mutex_t& mutex();
        pthread_mutex_t cs_mutex;
#endif
    };

#if defined(_WIN32)

    inline CriticalSection::CriticalSection()
    {
        InitializeCriticalSection(&cs);
    }

    inline CriticalSection::~CriticalSection()
    {
        LeaveCriticalSection(&cs);
    }

    inline CRITICAL_SECTION& CriticalSection::guard()
    {
        return cs;
    }

    inline CriticalSection::Locker::Locker(CriticalSection& cs_)
        : cs(cs_)
    {
        EnterCriticalSection(&cs.guard());
    }

    inline CriticalSection::Locker::~Locker()
    {
        LeaveCriticalSection(&cs.guard());
    }

#else

    inline CriticalSection::CriticalSection()
    {
        pthread_mutexattr_t recursiveAttr;
        pthread_mutexattr_init(&recursiveAttr);
        pthread_mutexattr_settype(&recursiveAttr, PTHREAD_MUTEX_RECURSIVE);

        int result = pthread_mutex_init(&cs_mutex, &recursiveAttr);
        (void) result;
    }

    inline CriticalSection::~CriticalSection()
    {
        int result = pthread_mutex_destroy(&cs_mutex);
        (void) result;
    }

    inline pthread_mutex_t& CriticalSection::mutex()
    {
        return cs_mutex;
    }

    inline CriticalSection::Locker::Locker(CriticalSection& cs_)
        : cs(cs_)
    {
        pthread_mutex_lock( &cs.mutex() );
    }

    inline CriticalSection::Locker::~Locker()
    {
        pthread_mutex_unlock( &cs.mutex() );
    }


#endif

	struct AudioDeviceId
	{
		String id;
		String name;
	};

	typedef SonarList<AudioDeviceId> AudioDeviceList;

	enum CaptureResult
	{
		CR_Success,
		CR_DeviceLost,
		CR_NoAudio,
	};

	enum PlaybackResult
	{
		PR_Success,
		PR_DeviceLost,
		PR_Underrun,
		PR_Overflow

	};

    class AudioFrame : public SonarVector<float>
    {
    public:

        AudioFrame()
            : SonarVector<float>()
            , ts(0)
        {
        }

        AudioFrame(long long _timestamp, int _numSamples, float _value)
            : SonarVector<float>(_numSamples, _value)
            , ts(_timestamp)
        {
        }

        AudioFrame(AudioFrame const& from)
            : SonarVector<float>(*static_cast<SonarVector<float> const*>(&from))
            , ts(from.ts)
        {
        }

        long long timestamp() const { return ts; }
        void timestamp(long long _timestamp) { ts = _timestamp; }

    private:

        long long ts;
    };

	typedef SonarList<String> TokenList;

#ifdef _WIN32
	typedef ::SOCKET SOCKET;
	typedef int socklen_t;
#else
	typedef int SOCKET;
	typedef ::socklen_t socklen_t;
#endif

    typedef enum
    {
        SONAR_TELEMETRY_SETTING,
        SONAR_TELEMETRY_STAT,
        SONAR_TELEMETRY_ERROR,

        SONAR_TELEMETRY_CLIENT_CONNECTED,
        SONAR_TELEMETRY_CLIENT_DISCONNECTED,
        SONAR_TELEMETRY_CLIENT_STATS
    }
    TelemetryType;

    typedef void (*LoggingFn)(const char* format, va_list args);
    typedef void (*TelemetryFn)(TelemetryType stt, va_list args);

    namespace internal
    {
        extern LoggingFn logFn;
        extern TelemetryFn telemetryFn;
        extern int lastCaptureDeviceError;
        extern int lastPlaybackDeviceError;
        extern int sonarTestingAudioError;
        extern int sonarTestingRegisterPacketLossPercentage;
        extern String sonarTestingVoiceServerRegisterResponse;
    }

	namespace common
	{
        typedef long long Timestamp;

		void PCMShortToFloat (short *_pInBuffer, size_t _cSamples, float *_pOutBuffer);
		void FloatToPCMShort (const float *_pInBuffer, size_t _cSamples, short *_pOutBuffer);

		String wideToUtf8(const wchar_t *wide);
        void sleepFrameDriftAdjusted(sonar::INT64& baseClock);
		void sleepFrame(void);
		void sleepHalfFrame(void);
		void sleepExact(int msec);
        int frameDurationInMS();
		INT64 getTimeAsMSEC(void);
		void setVoipPort(int32_t port);
		int32_t getVoipPort(void);
		void setVoipAddress(sockaddr_in address);
		const sockaddr_in &getVoipAddress();
#ifndef _WIN32
		void syslog_with_port(int priority, const char *format, ...);
		void vsyslog_with_port(int priority, const char *format, va_list args);
#endif

		const int DEF_MSG_NOSIGNAL = 0;
		typedef int socklen_t;

		void SocketClose(SOCKET fd);
		bool SocketWouldBlock(SOCKET fd);
		bool SocketEINPROGRESS(SOCKET fd);
		int SocketGetLastError(void);
		void SocketSetNonBlock(SOCKET fd, bool state);
		int PortableGetCurrentThreadId();
		void YieldThread();
		TokenList tokenize(CString &input, int delim);

        /// \brief Callback to allow logging from inside sonar
        inline void RegisterLogCallback(LoggingFn fn)
        {
            internal::logFn = fn;
        }

        inline void Log(const char* format, ...)
        {
            if (internal::logFn)
            {
                va_list args;
                va_start(args, format);
                (*internal::logFn)(format, args);
                va_end(args);
            }
        }

        /// \brief Callback to allow telemetry from inside sonar

        inline void RegisterTelemetryCallback(TelemetryFn fn)
        {
            internal::telemetryFn = fn;
        }

        inline void Telemetry(TelemetryType stt, ...)
        {
            if (internal::telemetryFn)
            {
                va_list args;
                va_start(args, stt);
                (*internal::telemetryFn)(stt, args);
                va_end(args);
            }
        }

        /// \brief Returns the last encountered internal error & error message.
        /// On Windows, the value returned will be some form of HRESULT.
        /// Calling the function will clear the error.
        inline int LastCaptureDeviceError()
        {
            int ret = internal::lastCaptureDeviceError;
            internal::lastCaptureDeviceError = 0;
            return ret;
        }

        inline int LastPlaybackDeviceError()
        {
            int ret = internal::lastPlaybackDeviceError;
            internal::lastPlaybackDeviceError = 0;
            return ret;
        }

        /// \brief Redirects the std io functions to write to files.
        void RedirectStdioToFile();

        extern bool ENABLE_CAPTURE_PCM_LOGGING;
        extern bool ENABLE_CAPTURE_SPEEX_LOGGING;
        extern bool ENABLE_CAPTURE_DECODED_SPEEX_LOGGING;
        extern bool ENABLE_PLAYBACK_DSOUND_LOGGING;
        extern bool ENABLE_PLAYBACK_SPEEX_LOGGING;
        extern bool ENABLE_CLIENT_SETTINGS_LOGGING;
        extern bool ENABLE_CLIENT_DSOUND_TRACING;
        extern bool ENABLE_CLIENT_MIXER_STATS;
        extern bool ENABLE_SONAR_TIMING_LOGGING;
        extern bool ENABLE_JITTER_TRACING;
        extern bool ENABLE_CONNECTION_STATS;
        extern bool ENABLE_PAYLOAD_TRACING;
        extern int JITTER_METRICS_LOG_LEVEL;
        extern bool ENABLE_SPEEX_ECHO_CANCELLATION;
        extern int SPEEX_ECHO_QUEUE_DELAY;
        extern int SPEEX_ECHO_TAIL_MULTIPLIER;
	}

}


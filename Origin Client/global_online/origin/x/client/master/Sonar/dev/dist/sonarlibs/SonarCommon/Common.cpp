#include <SonarCommon/Common.h>
#include <SonarConnection/Protocol.h>

#include <sys/types.h>
#include <locale.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <Ws2tcpip.h>
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <wchar.h>

#if defined(__APPLE__)
#include <mach/clock.h>
#include <mach/mach.h>
#endif
#if defined(__APPLE__) || defined(__ANDROID__)
#include <sched.h>

// Fix for pthread_yield() not being available on OSX/iOS and Android
int pthread_yield(void) 
{
    sched_yield();
    return 0;
}

#endif

namespace sonar
{
    namespace internal
    {
        LoggingFn logFn = NULL;
        TelemetryFn telemetryFn = NULL;
        int lastCaptureDeviceError = 0;
        int lastPlaybackDeviceError = 0;
        int sonarTestingAudioError = 0;
        int32_t voipPort = 0; // for syslog
        sockaddr_in voipAddress;
        int sonarTestingRegisterPacketLossPercentage = 0;
        String sonarTestingVoiceServerRegisterResponse;
    }

    namespace common
    {
        static bool once = true;
        bool ENABLE_CAPTURE_PCM_LOGGING = false;
        bool ENABLE_CAPTURE_SPEEX_LOGGING = false;
        bool ENABLE_CAPTURE_DECODED_SPEEX_LOGGING = false;
        bool ENABLE_PLAYBACK_DSOUND_LOGGING = false;
        bool ENABLE_PLAYBACK_SPEEX_LOGGING = false;
        bool ENABLE_CLIENT_SETTINGS_LOGGING = false;
        bool ENABLE_CLIENT_DSOUND_TRACING = false;
        bool ENABLE_CLIENT_MIXER_STATS = false;
        bool ENABLE_SONAR_TIMING_LOGGING = false;
        bool ENABLE_CONNECTION_STATS = false;
        bool ENABLE_JITTER_TRACING = false;
        bool ENABLE_PAYLOAD_TRACING = false;
        int JITTER_METRICS_LOG_LEVEL = 0;
        bool ENABLE_SPEEX_ECHO_CANCELLATION = false;
        int SPEEX_ECHO_QUEUE_DELAY = 0;
        int SPEEX_ECHO_TAIL_MULTIPLIER = 0;
        

        static void setupLocale(void)
        {
            if (once)
            {
                setlocale(LC_CTYPE, "en_US.utf8");
                once = false;
            }
        }
        
        void PCMShortToFloat (short *_pInBuffer, size_t _cSamples, float *_pOutBuffer)
        {
            for (size_t index = 0; index < _cSamples; index ++)
            {
                _pOutBuffer[index] = (float) _pInBuffer[index];
            }
        }

        void FloatToPCMShort (const float *_pInBuffer, size_t _cSamples, short *_pOutBuffer)
        {
            for (size_t index = 0; index < _cSamples; index ++)
            {
                _pOutBuffer[index] = (short) _pInBuffer[index];
            }
        }

        String wideToUtf8(const wchar_t *wide)
        {
            if (once) setupLocale();

            size_t wlen = wcslen(wide);

            size_t cbOut = (wlen + 1) * 4;
            char *pmbstr = new char[cbOut];

#ifdef _WIN32
            WideCharToMultiByte (
                CP_UTF8, 
                0,
                wide,
                (int) wlen + 1,
                pmbstr,
                (int) cbOut,
                NULL,
                NULL);
#else
            size_t result = wcstombs(pmbstr, wide, cbOut - 1);
            (void) result;
#endif

            String ret(pmbstr);

            delete[] pmbstr;
            return ret;
        }

        // syslog
        void setVoipPort(int32_t port)
        {
            internal::voipPort = port;
        }

        int getVoipPort(void)
        {
            return internal::voipPort;
        }
        
        void setVoipAddress(sockaddr_in address)
        {
            internal::voipAddress = address;
        }

        const sockaddr_in &getVoipAddress()
        {
            return internal::voipAddress;
        }

#ifndef _WIN32
        void syslog_with_port(int priority, const char *format, ...)
        {
            va_list valist;
            va_start( valist, format );

            char formatWithPort[128];
        #ifdef _WIN32
            sprintf_s(formatWithPort, 128, "[%d] %s", internal::voipPort, format);
        #else
            snprintf(formatWithPort, 128, "[%d] %s", internal::voipPort, format);
        #endif
            vsyslog(priority, formatWithPort, valist);

            va_end(valist);
        }

        void vsyslog_with_port(int priority, const char *format, va_list args)
        {
            char formatWithPort[128];
            snprintf(formatWithPort, 128, "[%d] %s", internal::voipPort, format);
            vsyslog(priority, formatWithPort, args);
        }
#endif

        INT64 getTimeAsMSEC(void)
        {
#ifdef _WIN32
            static LARGE_INTEGER freq;
            static LARGE_INTEGER start;

            static bool once = true;

            if (once)
            {
                QueryPerformanceFrequency (&freq);
                QueryPerformanceCounter (&start);
                freq.QuadPart /= 1000;
                once = false;
            }

            LARGE_INTEGER now;

            QueryPerformanceCounter (&now);

            INT64 delta = now.QuadPart - start.QuadPart;
            return (INT64) (delta / freq.QuadPart);
#elif defined(__APPLE__)


            static bool once = true;
            static clock_serv_t cclock;

            if (once)
            {
                host_get_clock_service(mach_host_self(), SYSTEM_CLOCK , &cclock);
                once = false;
            }

            // iOS/OSX does not implement clock_gettime()
            // The following implementation was snagged from StackOverflow Q5167269.
            mach_timespec_t mts;
            clock_get_time(cclock, &mts);
            return (INT64) mts.tv_nsec / (INT64) 1000000 + (INT64) mts.tv_sec * (INT64) 1000;
#else
            timespec tv = { 0, 0 };

            if (clock_gettime(CLOCK_MONOTONIC, &tv) == -1)
            {
                exit(-4);
            }

            INT64 time = (INT64) tv.tv_nsec / (INT64) 1000000 + (INT64) tv.tv_sec * (INT64) 1000;

            return time;
#endif

        }

        void sleepExact(int msec)
        {
#ifdef _WIN32
            Sleep(msec);
#else
            usleep((msec) * 1000);
#endif
        }

        void sleepFrameDriftAdjusted(sonar::INT64& baseClock)
        {
            int timeToSleep;
            const sonar::INT64 now = common::getTimeAsMSEC();

            if (baseClock == 0) {
                timeToSleep = protocol::FRAME_TIMESLICE_MSEC;
                baseClock = now;
            } else {
                sonar::INT64 drift = now - baseClock;
                timeToSleep = protocol::FRAME_TIMESLICE_MSEC - drift;
                if (timeToSleep < 0) {
                    // TBD: not sure how long to sleep if the drift is greater than one frame
                    timeToSleep = 1; // / 2;
                }
            }

            baseClock += protocol::FRAME_TIMESLICE_MSEC;
            sleepExact(timeToSleep);
        }

    void sleepFrame(void)
    {
      sleepExact(protocol::FRAME_TIMESLICE_MSEC);
    }

    void sleepHalfFrame(void)
    {
      sleepExact(protocol::FRAME_TIMESLICE_MSEC / 2);
    }

    int frameDurationInMS()
    {
        return protocol::FRAME_TIMESLICE_MSEC;
    }

#ifdef _WIN32
        void SocketClose(SOCKET fd)
        {
            ::closesocket(fd);
        }

        bool SocketWouldBlock(SOCKET fd)
        {
            return (WSAGetLastError () == WSAEWOULDBLOCK);
        }

        bool SocketEINPROGRESS(SOCKET fd)
        {
            return (WSAGetLastError () == WSAEINPROGRESS);

        }

        int SocketGetLastError(void)
        {
            return  (int) WSAGetLastError ();

        }

        void SocketSetNonBlock(SOCKET fd, bool state)
        {
            unsigned long flags = (state) ? 1 : 0;
            ioctlsocket(fd, FIONBIO, &flags);
        }

        int PortableGetCurrentThreadId()
        {
            return (int) GetCurrentThreadId();
        }

        void YieldThread()
        {
            SwitchToThread();
        }

#else
        void SocketClose(SOCKET fd)
        {
            ::close(fd);
        }

        bool SocketWouldBlock(SOCKET fd)
        {
            return (errno == EWOULDBLOCK);
        }

        bool SocketEINPROGRESS(SOCKET fd)
        {
            return (errno == EINPROGRESS);
        }

        int SocketGetLastError(void)
        {
            return (int) errno;
        }

        void SocketSetNonBlock(SOCKET fd, bool state)
        {
            if (state)
                fcntl (fd, F_SETFL, O_NONBLOCK);
            else
                fcntl (fd, F_SETFL, 0);
        }

        void YieldThread()
        {
            pthread_yield();
        }
#endif



        TokenList tokenize(CString &str, int delim)
        {
            size_t length = str.size ();
            size_t start = 0;

            TokenList list;

            const char *pstr = str.c_str ();

            for (size_t index = 0; index < length; index ++)
            {
                if (*pstr == delim)
                {
                    list.push_back (str.substr (start, (index - start)));
                    start = index + 1;
                }
                pstr ++;
            }

            list.push_back (str.substr(start, length));

            return list;
        }

#if defined(_WIN32)

        /// \brief Redirect output to stdout and stderr to a file for logging.
        void RedirectStdioToFile()
        {
            // from: http://asawicki.info/news_1326_redirecting_standard_io_to_windows_console.html
            // also: http://support.microsoft.com/kb/105305
            // originally: http://dslweb.nwnexus.com/~ast/dload/guicon.htm

            bool useFiles = 1;

            AllocConsole();

            // redirect unbuffered STDOUT to the console
            FILE* file = NULL;
            if (useFiles) {
                file = fopen("sonar_stdout.txt", "w");
                setvbuf(file, NULL, _IONBF, 0);
            } else {
                HANDLE stdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
                int osfHandle = _open_osfhandle(reinterpret_cast<long>(stdOutHandle), _O_TEXT);
                file = _fdopen(osfHandle, "w");
                setvbuf(file, NULL, _IONBF, 0);
            }
            *stdout = *file;

            // redirect unbuffered STDIN to the console
            if (useFiles) {
                file = fopen("sonar_stdin.txt", "r");
            }
            // fall back to console if the text file doesn't exist
            if (!file) {
                HANDLE stdInHandle = GetStdHandle(STD_INPUT_HANDLE);
                int osfHandle = _open_osfhandle(reinterpret_cast<long>(stdInHandle), _O_TEXT);
                file = _fdopen(osfHandle, "r" );
                setvbuf(file, NULL, _IONBF, 0);
            }
            *stdin = *file;

            // redirect unbuffered STDERR to the console
            if (useFiles) {
                //file = fopen("sonar_stderr.txt", "w");
                file = stdout;
            } else {
                HANDLE stdErrHandle = GetStdHandle(STD_ERROR_HANDLE);
                int osfHandle = _open_osfhandle(reinterpret_cast<long>(stdErrHandle), _O_TEXT);
                file = _fdopen(osfHandle, "w" );
                setvbuf(file, NULL, _IONBF, 0);
            }
            *stderr = *file;

            // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
            std::ios::sync_with_stdio();
        }

#endif

}

}

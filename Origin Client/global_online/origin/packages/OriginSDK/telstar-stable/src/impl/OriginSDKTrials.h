#ifndef ORIGIN_SDK_TRIALS
#define ORIGIN_SDK_TRIALS

#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>

#include <OriginSDK/OriginTypes.h>


//////////////////////////////////////////////////////////////////////////
//  Some constants.
//
const int STARTUP_SLEEP = 2000;
const int RETRY_COUNT = 5;
const int EXTEND_BEFORE_EXPIRE = 60000;
const int SLEEP_BEFORE_NUKE_TIME = 30000;

// This interface needs to be implemented by the trial provider in order to get a time ticket.
class ITrialTicketProvider
{
public:
	virtual void GetTimeTicketRequest(char* requestBuffer, const unsigned int bufferSize) = 0;
};

extern ITrialTicketProvider * g_pTrialTicketProvider;
extern std::atomic<char *> g_pActiveTicket;
extern std::atomic<size_t> g_ActiveTicketSize;

const int DEFAULT_REQUEST_BUFFER = 8192;
const int DEFAULT_TICKET_BUFFER = 32768;

namespace Origin
{

    class Trials
    {
    public:
        Trials();
        virtual ~Trials();        
		
		void *operator new(size_t size);
        void operator delete(void*);
		
		// Ticket Engine 0 is Denuvo.
		void Start(ITrialTicketProvider * trial = g_pTrialTicketProvider, int ticketEngine = 0, int startupSleep = STARTUP_SLEEP);
		void Stop();

		bool IsRunning() { return m_thread.joinable(); }
		bool IsPending() { return m_pending; }

		Trials(const Trials &) = delete;
		Trials(Trials &&) = delete;
		Trials & operator = (const Trials &) = delete;
		Trials & operator = (Trials &&) = delete;

    private:
        void Run(int startupSleep, int ticketEngine);

		std::atomic<bool>			m_pending;
		std::atomic<bool>			m_stop;
		std::thread					m_thread;
		std::mutex					m_stopConditionMutex;
		std::condition_variable		m_stopConditionVariable;

        int							m_TotalTimeRemaining;
        int							m_TimeGranted;

		int							m_TicketEngine;
		ITrialTicketProvider *		m_TrialProvider;
    };
}



#endif
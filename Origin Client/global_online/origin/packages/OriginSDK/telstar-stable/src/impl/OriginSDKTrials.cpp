#include "stdafx.h"
#include "OriginSDKimpl.h"
#include "OriginSDKTrials.h"

#include <chrono>

#ifndef ORIGIN_PC
#include <stdlib.h>
#endif
const uint32_t TIMEOUT = 15000;


char g_TimeTicketRequest[DEFAULT_REQUEST_BUFFER] = { 0 };
char g_TimeTicket[2][DEFAULT_TICKET_BUFFER] = { { 0 }, { 0 } };
size_t g_TimeTicketSize[2] = { 0, 0 };

std::atomic<char *> g_pActiveTicket( NULL );
std::atomic<size_t> g_ActiveTicketSize( 0 );
std::atomic<int> g_ActiveTicketIndex( 0 );

//////////////////////////////////////////////////////////////////////////
//
//  Denuvo Marker stub this function
//


//////////////////////////////////////////////////////////////////
//  

namespace Origin
{
    //  The content of this function will be replaced by Denuvo Code Fusion to the actual implementation

#ifdef ORIGIN_PC
#pragma optimize("", off)
#pragma auto_inline(off)

#define EXPORT_DENUVO __declspec(dllexport)
#else
#define EXPORT_DENUVO

#endif
	extern "C"
	{
		EXPORT_DENUVO const char * GetDenuvoTicketLocation()
		{
			return g_pActiveTicket;
		}

		EXPORT_DENUVO void GetDenuvoTimeTicketRequest(char* /* requestBuffer */, const unsigned int /* bufferSize */)
		{
#ifdef ORIGIN_PC
#pragma warning(disable: 4474)
			sprintf(g_TimeTicketRequest, "", GetDenuvoTicketLocation());
#pragma warning(default: 4474)
#else
			sprintf(g_TimeTicketRequest, "");
#endif
		}
	}

#ifdef ORIGIN_PC
#pragma auto_inline()
#pragma optimize("", on)
#endif

}
//
//
//////////////////////////////////////////////////////////////////


using namespace Origin;

class DenuvoTrialV2 : public ITrialTicketProvider
{
	virtual void GetTimeTicketRequest(char * requestBuffer, const unsigned int bufferSize) override
	{
		GetDenuvoTimeTicketRequest(requestBuffer, bufferSize);
	}
} denuvoV2;

ITrialTicketProvider * g_pTrialTicketProvider = &denuvoV2;


Trials::Trials() :
	m_pending(true),
    m_stop(false),
	m_TotalTimeRemaining(0),
	m_TimeGranted(0),
	m_TrialProvider(NULL)
{

}

Trials::~Trials()
{

}

void *Trials::operator new(size_t size)
{
    return Origin::AllocFunc(size);
}

void Trials::operator delete(void *p)
{
    Origin::FreeFunc(p);
}


void Origin::Trials::Start(ITrialTicketProvider *pProvider, int startupSleep, int ticketEngine)
{
	if (!m_thread.joinable())
	{
		m_TrialProvider = pProvider;
		m_stop = false;
		m_pending = true;
		m_thread = std::thread([=]() { Run(startupSleep, ticketEngine); });
	}
}

void Origin::Trials::Stop()
{
	if (m_thread.joinable())
	{
		{
			std::lock_guard<std::mutex> lock(m_stopConditionMutex);
			m_stop = true;
		}
		m_stopConditionVariable.notify_one();

		m_thread.join();
	}
}



void Trials::Run(int startupSleep, int ticketEngine)
{
    if(!OriginSDK::Get())
        return;

    int retryCount = RETRY_COUNT;
	int sleepPeriod = startupSleep;
    bool bExit = false;

    OriginTrialConfig config;

    while(!m_stop)
    {
		{
			std::unique_lock<std::mutex> lock(m_stopConditionMutex);

			if (m_stopConditionVariable.wait_for(lock, std::chrono::milliseconds(sleepPeriod), [&]() -> bool { return m_stop; }))
			{
				if (m_stop)
					break;
			}
		}

		if (bExit)
			break;

		if (m_TrialProvider)
		{
			m_TrialProvider->GetTimeTicketRequest(g_TimeTicketRequest, DEFAULT_REQUEST_BUFFER);
		}

		OriginErrorT err;
		int nextTicket = g_ActiveTicketIndex ^ 0x1;

		g_TimeTicketSize[nextTicket] = DEFAULT_TICKET_BUFFER;

        if((err = OriginSDK::Get()->ExtendTrial(g_TimeTicketRequest, 
												ticketEngine, 
												&m_TotalTimeRemaining, 
												&m_TimeGranted, 
												g_TimeTicket[nextTicket],
												&g_TimeTicketSize[nextTicket],
												config, 
												TIMEOUT)) == ORIGIN_SUCCESS)
        {
            retryCount = config.RetryCount;

            if(m_TimeGranted == m_TotalTimeRemaining || m_TimeGranted == 0)
            {
                // Wake up when the ticket expires.
                sleepPeriod = m_TimeGranted * 1000;

                if(m_TimeGranted == 0)
                {
                    // This was our last ticket 
                    sleepPeriod += config.SleepBeforeNukeTime;
                    bExit = true;
                }
            }
            else
            {
                // Wake up 1 minute before expiry. 
                sleepPeriod = m_TimeGranted * 1000 - config.ExtendBeforeExpire;
            }

            // Flip the active ticket.
            g_ActiveTicketIndex = nextTicket;
			g_ActiveTicketSize = g_TimeTicketSize[g_ActiveTicketIndex];
            g_pActiveTicket = g_TimeTicket[g_ActiveTicketIndex];


			// We got a valid ticket
			m_pending = false;
		}
        else
        {
            retryCount--;

            if(retryCount <= 0)
            {
                sleepPeriod = config.SleepBeforeNukeTime;
                bExit = true;
            }
            else
            {
                sleepPeriod = config.RetryAfterFail;
            }
        }
    }

	// We couldn't get a valid ticket, or something else went wrong. (or the user just quit.)
	m_pending = false;

    if(bExit)
    {
#ifdef ORIGIN_PC
        TerminateProcess(GetCurrentProcess(), 1);
#else
        exit(EXIT_SUCCESS);
#endif
    }
}

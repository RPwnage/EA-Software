#ifndef __ORIGIN_LSX_EVENT_H__
#define __ORIGIN_LSX_EVENT_H__

#include "OriginSDKImpl.h"
#include "OriginErrorFunctions.h"
#include "OriginLSXMessage.h"
#include <assert.h>

#ifdef ORIGIN_MAC
#define INFINITE 0xFFFFFFFF
#endif

#undef max

#include <chrono>

namespace Origin
{

	namespace Event
	{
		class Dummy
		{
		};
	}

	template<typename EventType> class LSXEvent : public Origin::IXmlMessageHandler
	{
	public:
		typedef OriginErrorT(OriginSDK::*EventCallback)(EventType &Event);

		LSXEvent<EventType>(const char *Service, OriginSDK *pSDK) :
			m_Event("Event", Service),
			m_Key(0),
			m_Error(ORIGIN_SUCCESS),
			m_bReady(false),
            m_pSDK(pSDK),
			m_EventCallbackFunc(NULL)
		{
		}

		LSXEvent<EventType>(const char *Service, OriginSDK *pSDK, EventCallback eventCallback) :
			m_Event("Event", Service),
			m_Key(0),
			m_Error(ORIGIN_SUCCESS),
			m_bReady(false),
			m_pSDK(pSDK),
			m_EventCallbackFunc(eventCallback)
		{
        }

		virtual ~LSXEvent()
		{
            m_pSDK->DeregisterMessageHandler(m_Key);
		}

		void *operator new(size_t size)
		{
			return Origin::AllocFunc(size);
		}

		void operator delete(void *p)
		{
			Origin::FreeFunc(p);
		}

		virtual uint32_t GetPacketIdHash()
		{
			return m_Event->GetHash();
		}

		virtual OriginErrorT HandleMessage(INodeDocument* doc)
		{
			OriginErrorT ret = ORIGIN_SUCCESS;

			if(!m_Event.FromXml(doc))
			{
				ret = ORIGIN_ERROR_LSX_INVALID_RESPONSE;
				m_Error = ret;
			}

			// Signal that we are ready.
			m_bReady = true;
            m_Done.notify_one();

			return REPORTERROR(ret);
		}

        virtual OriginSDK::HandlerKey Register(uint32_t timeout)
		{
			m_Timeout = (timeout == INFINITE) ?
				std::chrono::system_clock::time_point::max() :
				std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);

			m_Key = m_pSDK->RegisterMessageHandler(this, 0);
            return m_Key;
		}
				
		virtual bool HasCallback()
		{
			return m_EventCallbackFunc != NULL;
		}

		virtual OriginErrorT DoCallback()
		{
			if(HasCallback())
			{
				if(IsReady())
				{
					return (m_pSDK->*m_EventCallbackFunc)(GetEvent());
				}
				else
				{
					return ORIGIN_PENDING;
				}
			}
			return REPORTERROR(ORIGIN_SUCCESS);
		}
	
		virtual bool Wait(uint32_t timeout)
		{
			m_Timeout = (timeout == INFINITE) ?
				std::chrono::system_clock::time_point::max() :
				std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);

			// If the wait times out it will return <0
			std::unique_lock<std::mutex> lock(m_Processing);
			if (!m_Done.wait_until(lock, m_Timeout, [&]() -> bool {return IsReady(); }))
			{
				m_Error = ORIGIN_ERROR_LSX_NO_RESPONSE;
			}
		
			return m_Error == ORIGIN_SUCCESS;
		}

		bool IsTimedOut()
		{
			if(!IsReady())
			{
				std::chrono::system_clock::time_point threadTime = std::chrono::system_clock::now();

				if (m_Timeout < threadTime)
				{
					if (m_Error == ORIGIN_SUCCESS)
					{
						m_Error = ORIGIN_ERROR_LSX_NO_RESPONSE;
					}
					return true;
				}
			}
			return false;
		}


		virtual bool IsReady()
		{
			return m_bReady;
		}

		virtual OriginErrorT GetErrorCode() { return m_Error; } 
		EventType &GetEvent() { return m_Event; }

		virtual void StoreMemory(void * /*pData*/)
		{
			// This should not be called.
            assert(0);
		}

		LSXEvent<EventType>(const LSXEvent<EventType> &) = delete;
		LSXEvent<EventType>(LSXEvent<EventType> &&) = delete;
		LSXEvent<EventType> & operator = (const LSXEvent<EventType> &) = delete;
		LSXEvent<EventType> & operator = (LSXEvent<EventType> &&) = delete;

	private:
		LSXMessage<EventType>					m_Event;

		OriginSDK::HandlerKey					m_Key;
		OriginErrorT							m_Error;
		std::mutex								m_Processing;
		std::condition_variable					m_Done;
		std::atomic<bool>						m_bReady;
		std::chrono::system_clock::time_point	m_Timeout;

		OriginSDK *								m_pSDK;
		EventCallback							m_EventCallbackFunc;
	};

}




#endif //__ORIGIN_LSX_EVENT_H__

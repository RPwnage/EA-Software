#ifndef __ORIGIN_LSX_ENUMERATION_H__
#define __ORIGIN_LSX_ENUMERATION_H__

#include "OriginSDKImpl.h"
#include "OriginErrorFunctions.h"
#include "OriginLSXMessage.h"
#include <readercommon.h>

#ifdef ORIGIN_MAC
#define INFINITE 0xFFFFFFFF
#endif

#undef max

#include <chrono>

namespace Origin
{
	namespace Enum
	{
		class Dummy
		{
		};
	}

	template<typename RequestType, typename ResponseType, typename ClientType> class LSXEnumeration : public Origin::IEnumerator
	{
	public:

		typedef OriginErrorT(OriginSDK::*CallbackConvertDataType)(IEnumerator *pEnumerator, ClientType *pData, size_t index, size_t count, ResponseType &response);

		LSXEnumeration<RequestType, ResponseType, ClientType>(const char *Service, OriginSDK *pSDK, CallbackConvertDataType convertDataCallback, OriginEnumerationCallbackFunc clientCallback = NULL, void *pContext = NULL) :
			m_Request("Request", Service),
			m_Response("Response", Service),
			m_key(0),
			m_Error(ORIGIN_SUCCESS),
			m_bReady(false),
			m_Timeout(std::chrono::system_clock::now()),
			m_pSDK(pSDK),
			m_pConvertData(convertDataCallback),
			m_ClientCallback(clientCallback),
			m_pContext(pContext)
		{

		}

		~LSXEnumeration()
		{
			EmptyDataStore();
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
			return m_Response->GetHash();
		}

		virtual OriginErrorT HandleMessage(INodeDocument* doc)
		{
            m_pSDK->DeregisterMessageHandler(m_key);
            
            if(m_Processing.try_lock() && !m_bReady)
            {
                m_Error = ORIGIN_SUCCESS;

                if(!m_Response.FromXml(doc))
                {
                    LSXMessage<lsx::ErrorSuccessT> error("Response", m_Response.GetService());

                    if(error.FromXml(doc))
                    {
                        m_Error = error->Code;
                    }
                    else
                    {
                        m_Error = ORIGIN_ERROR_LSX_INVALID_RESPONSE;
                    }
                }

				m_bReady = true;
                m_Processing.unlock();

				m_Done.notify_one();

                return REPORTERROR(m_Error);
            }
            else
            {
                return REPORTERROR(ORIGIN_ERROR_LSX_NO_RESPONSE);
            }
		}

		bool Execute(uint32_t timeout, size_t *pTotalCount)
		{
			if(m_pSDK->IsConnected())
			{
				uint64_t requestId = m_pSDK->GetNewTransactionId();
		
				m_Timeout = (timeout == INFINITE) ?
					std::chrono::system_clock::time_point::max() :
					std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);

				m_key = m_pSDK->RegisterMessageHandler(this, requestId);

				m_Error = m_pSDK->SendXml(m_Request.ToXml(requestId));

				// If the semaphore times out it will return 0
				std::unique_lock<std::mutex> lock(m_Processing);
				if (!m_Done.wait_until(lock, m_Timeout, [&]()->bool {return IsReady(); }))
				{
                    m_pSDK->DeregisterMessageHandler(m_key);
                    m_Error = ORIGIN_ERROR_LSX_NO_RESPONSE;
                    m_bReady = true;
                }
				else
				{
					*pTotalCount = GetResponse().GetCount();
				}

				return m_Error == ORIGIN_SUCCESS;
			}
			else
			{
				m_Error = ORIGIN_ERROR_CORE_NOTLOADED;
				return false;
			}
		}

		bool ExecuteAsync(uint32_t timeout)
		{
			if(m_pSDK->IsConnected())
			{
				uint64_t requestId = m_pSDK->GetNewTransactionId();

				m_Timeout = (timeout == INFINITE) ?
					std::chrono::system_clock::time_point::max() :
					std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);

				m_key = m_pSDK->RegisterMessageHandler(this, requestId);

				m_Error = m_pSDK->SendXml(m_Request.ToXml(requestId));

				return m_Error == ORIGIN_SUCCESS;
			}
			else
			{
				m_Error = ORIGIN_ERROR_CORE_NOTLOADED;
				return false;
			}
		}

		bool Wait()
		{
			if(!IsReady())
			{
				std::unique_lock<std::mutex> lock(m_Processing);
				if (!m_Done.wait_until(lock, m_Timeout, [&]() -> bool {return IsReady(); }))
				{
					m_Error = ORIGIN_ERROR_LSX_NO_RESPONSE;
				}
				return m_Error == ORIGIN_SUCCESS;
			}
			return true;
		}

		virtual bool HasCallback()
		{
			return m_pSDK != NULL && m_pConvertData != NULL && m_ClientCallback != NULL;
		}


		OriginErrorT DoCallback()
		{
			if(HasClientCallback())
			{
				if(m_Error == ORIGIN_SUCCESS)
				{
					return m_ClientCallback(m_pContext, (OriginHandleT)this, GetTotalCount(), GetAvailableCount(), m_Error);
				}
				else
				{
					return m_ClientCallback(m_pContext, (OriginHandleT)this, 0, 0, m_Error);
				}
			}
			return REPORTERROR(ORIGIN_ERROR_NO_CALLBACK_SPECIFIED);
		}

		bool IsTimedOut()
		{
			if (!IsReady())
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

		OriginErrorT ConvertData(void *pBufPtr, size_t iBufLen, size_t index, size_t count, size_t *pItemCnt)
		{
			if(m_pSDK != NULL && m_pConvertData != NULL)
			{
				if(IsReady())
				{
					size_t itemCount = iBufLen/sizeof(ClientType);

					if(itemCount < GetTotalCount() - index)
						return REPORTERROR(ORIGIN_ERROR_BUFFER_TOO_SMALL);

					if((uint64_t)index + (uint64_t)count>GetTotalCount())
						count = GetTotalCount() - index;

					*pItemCnt = count;

					if(count>itemCount)
						return REPORTERROR(ORIGIN_ERROR_BUFFER_TOO_SMALL);

					return (m_pSDK->*m_pConvertData)(this, (ClientType *)pBufPtr, index, count, GetResponse());
				}
				else
				{
					return REPORTERROR(ORIGIN_PENDING);
				}
			}
			else
			{
				return REPORTERROR(ORIGIN_ERROR_SDK_INTERNAL_ERROR);
			}
			
		}

		bool IsReady()
		{
			return m_bReady;
		}

		RequestType &GetRequest(){return m_Request;}
		ResponseType &GetResponse(){return m_Response;}

		OriginErrorT GetErrorCode()
		{
			if(IsReady())
			{
				return m_Error;
			}
			else
			{
				if(IsTimedOut())
				{
					return ORIGIN_ERROR_LSX_NO_RESPONSE;
				}
				else
				{
					if(m_Error == ORIGIN_SUCCESS)
					{
						return ORIGIN_PENDING;
					}
					else
					{
						return m_Error;
					}
				}
			}
		}						  

		size_t GetTotalCount()
		{
			if(IsReady())
			{
				return GetResponse().GetCount();
			}
			return 0;
		}

		// With our current LSX message scheme we get all data at once.
		size_t GetAvailableCount()
		{
			return GetTotalCount();
		}
 
		size_t GetTypeSize()
		{
			return sizeof(ClientType);
		}

		bool HasClientCallback()
		{
			return m_ClientCallback != NULL;
		}

		OriginErrorT DoClientCallback()
		{
			if(HasClientCallback())
			{
				return m_ClientCallback(m_pContext, (OriginHandleT)this, GetTotalCount(), GetAvailableCount(), GetErrorCode()); 
			}
			else
			{
				return ORIGIN_ERROR_NO_CALLBACK_SPECIFIED;
			}
		}


		// This is used to store resources needed by the client data structures
		void StoreMemory(void *pData)
		{
			m_DataStore.push_back(pData);
		}

		void EmptyDataStore()
		{
			for(auto data : m_DataStore)
			{
				TYPE_DELETE(data);
			}
			m_DataStore.clear();
		}

		LSXEnumeration<RequestType, ResponseType, ClientType>(const LSXEnumeration<RequestType, ResponseType, ClientType> &) = delete;
		LSXEnumeration<RequestType, ResponseType, ClientType>(LSXEnumeration<RequestType, ResponseType, ClientType> &&) = delete;
		LSXEnumeration<RequestType, ResponseType, ClientType> & operator = (const LSXEnumeration<RequestType, ResponseType, ClientType> &) = delete;
		LSXEnumeration<RequestType, ResponseType, ClientType> & operator = (LSXEnumeration<RequestType, ResponseType, ClientType> &&) = delete;

	private:
		LSXMessage<RequestType>					m_Request;
		LSXMessage<ResponseType>				m_Response;

		MemoryContainer							m_DataStore;

		OriginSDK::HandlerKey					m_key;
		OriginErrorT							m_Error;

		std::condition_variable					m_Done;
        std::mutex								m_Processing;
		std::atomic<bool>						m_bReady;
		std::chrono::system_clock::time_point	m_Timeout;


		OriginSDK *								m_pSDK;
		CallbackConvertDataType					m_pConvertData;
		OriginEnumerationCallbackFunc			m_ClientCallback;
		void *									m_pContext;
	};
}




#endif //__ORIGIN_LSX_REQUEST_H__

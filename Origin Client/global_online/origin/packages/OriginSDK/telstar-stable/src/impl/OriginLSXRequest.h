#ifndef __ORIGIN_LSX_REQUEST_H__
#define __ORIGIN_LSX_REQUEST_H__

#include "OriginSDKImpl.h"
#include "OriginErrorFunctions.h"
#include "OriginLSXMessage.h"

#undef max

#include <chrono>

namespace Origin
{
	namespace Request
	{
		class Dummy
		{
		};
	}

    // To be able to report an error in the ErrorCallback for requests that return an ErrorSuccessT response.
    template<typename T> int GetErrorCodeFromResponse(T &t) { return ORIGIN_SUCCESS; }

    // The specialization will return the Code field in the lsx::ErrorSuccessT response.
    template<> int GetErrorCodeFromResponse(LSXMessage<lsx::ErrorSuccessT> &t);

	template<typename RequestType, 
		     typename ResponseType, 
			 typename ClientType = Request::Dummy, 
			 typename CallbackType = OriginResourceCallback> 
	class LSXRequest : public Origin::IXmlMessageHandler
	{
	public:

		typedef OriginErrorT(OriginSDK::*CallbackConvertDataType)(IXmlMessageHandler *pHandler, ClientType &clientData, size_t &clientDataSize, ResponseType &response);

		LSXRequest<RequestType, ResponseType, ClientType, CallbackType>(const char *Service, OriginSDK *pSDK) :
			m_Request("Request", Service),
			m_Response("Response", Service),
			m_Error(ORIGIN_SUCCESS),
			m_bReady(false),
			m_Timeout(std::chrono::system_clock::now()),
			m_pSDK(pSDK),
			m_ConvertDataCallback(NULL),
			m_ClientCallback(NULL),
			m_pContext(NULL),
			m_ClientDataSize(0)
		{

        }

		LSXRequest<RequestType, ResponseType, ClientType, CallbackType>(const char *Service,
																		OriginSDK *pSDK,
																		CallbackConvertDataType convertDataCallback,
																		CallbackType clientCallback,
																		void *pContext) :
			m_Request("Request", Service),
			m_Response("Response", Service),
			m_Error(ORIGIN_SUCCESS),
			m_bReady(false),
			m_Timeout(std::chrono::system_clock::now()),
			m_pSDK(pSDK),
			m_ConvertDataCallback(convertDataCallback),
			m_ClientCallback(clientCallback),
			m_pContext(pContext),
			m_ClientDataSize(0)
		{
            
		}

		virtual ~LSXRequest()
		{
			EmptyDataStore();
		}

		// overloaded allocation function to support game defined heaps.
		void *operator new(size_t size)
		{
			return Origin::AllocFunc(size);
		}

		// overloaded deallocation function to support game defined heaps.
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

            // If this lock fails or ready is set we are already shutting down the request.
            if(m_Processing.try_lock() && !m_bReady)
            {
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
                else
                {
                    m_Error = GetErrorCodeFromResponse(m_Response);
                }

                // Signal that we are ready.
				m_bReady = true;
				m_Processing.unlock();

				m_Done.notify_one();

                return m_Error;
            }
            else
            {
                return ORIGIN_ERROR_LSX_NO_RESPONSE;
            }
		}

		bool Execute(uint32_t timeout)
		{
			if(m_pSDK->IsConnected())
			{
				uint64_t requestId = m_pSDK->GetNewTransactionId();

				m_Timeout = (timeout == INFINITE) ? 
					std::chrono::system_clock::time_point::max() :
					std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);

				m_key = m_pSDK->RegisterMessageHandler(this, requestId);

				m_Error = m_pSDK->SendXml(m_Request.ToXml(requestId));

				std::unique_lock<std::mutex> lock(m_Processing);
				if (!m_Done.wait_until(lock, m_Timeout, [&]() -> bool {return IsReady(); }))
				{
                    // If entered here the callback is not called yet, or has just finished processing.
                    m_pSDK->DeregisterMessageHandler(m_key);
                    m_Error = ORIGIN_ERROR_LSX_NO_RESPONSE;
                    m_bReady = true;
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


		virtual bool HasCallback()
		{
			return m_ConvertDataCallback != NULL;
		}


		virtual OriginErrorT DoCallback()
		{
			if(HasCallback())
			{
				if(IsReady())
				{
					if(m_Error == ORIGIN_SUCCESS)
					{
						m_Error = (m_pSDK->*m_ConvertDataCallback)(this, m_ClientData, m_ClientDataSize, GetResponse());
					}
					DoClientCallback();
				}
				else
				{
					if(m_Error != ORIGIN_SUCCESS)
					{
						DoClientCallback();
					}
					else
					{
						return ORIGIN_PENDING;
					}
				}
			}
			return REPORTERROR(ORIGIN_SUCCESS);
		}

		bool IsTimedOut()
		{
			if(!IsReady())
			{
                std::chrono::system_clock::time_point threadTime = std::chrono::system_clock::now();

				if(m_Timeout < threadTime)
				{
					if(m_Error == ORIGIN_SUCCESS)
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


		bool HasClientCallback()
		{
			return m_ClientCallback != NULL;
		}

		OriginErrorT DoClientCallback()
		{
			if(HasClientCallback())
			{
				OriginErrorT err;

				if(GetErrorCode() == ORIGIN_SUCCESS)
				{
					err =  m_ClientCallback(m_pContext, &m_ClientData, m_ClientDataSize, GetErrorCode()); 
				}
				else
				{
					err =  m_ClientCallback(m_pContext, NULL, 0, GetErrorCode()); 
				}

				REPORTERROR(GetErrorCodeFromResponse(m_Response));

				return err;
			}
			else
			{
				return ORIGIN_ERROR_NO_CALLBACK_SPECIFIED;
			}
		}

		virtual void StoreMemory(void *pData)
		{
			m_DataStore.push_back(pData);
		}

		void EmptyDataStore()
		{
			for(auto data: m_DataStore)
			{
				TYPE_DELETE(data);
			}
			m_DataStore.clear();
		}

		OriginHandleT CopyDataStoreToHandle()
		{
			auto Handles = new HandleContainer();

			OriginHandleT handle = m_pSDK->RegisterResource(OriginResourceContainerT::HandleContainer, Handles);

			for(auto data: m_DataStore)
			{
				Handles->push_back(m_pSDK->RegisterResource(OriginResourceContainerT::String, data));
			}
			m_DataStore.clear();

			return handle;
		}

		OriginErrorT GetErrorCode(){return m_Error;} 

		RequestType &GetRequest(){return m_Request;}
		ResponseType &GetResponse(){return m_Response;}

		LSXRequest<RequestType, ResponseType, ClientType, CallbackType>(const LSXRequest<RequestType, ResponseType, ClientType, CallbackType> &) = delete;
		LSXRequest<RequestType, ResponseType, ClientType, CallbackType>(LSXRequest<RequestType, ResponseType, ClientType, CallbackType> &&) = delete;
		LSXRequest<RequestType, ResponseType, ClientType, CallbackType> & operator = (const LSXRequest<RequestType, ResponseType, ClientType, CallbackType> &) = delete;
		LSXRequest<RequestType, ResponseType, ClientType, CallbackType> & operator = (LSXRequest<RequestType, ResponseType, ClientType, CallbackType> &&) = delete;

	private:
		LSXMessage<RequestType>							m_Request;
		LSXMessage<ResponseType>						m_Response;

		MemoryContainer									m_DataStore;

		OriginSDK::HandlerKey							m_key;
		OriginErrorT									m_Error;

		std::condition_variable							m_Done;
		std::mutex										m_Processing;
		std::atomic<bool>								m_bReady;
		std::chrono::system_clock::time_point			m_Timeout;
		
		OriginSDK *										m_pSDK;
		CallbackConvertDataType							m_ConvertDataCallback;
		CallbackType									m_ClientCallback;
		void *											m_pContext;

		size_t											m_ClientDataSize;
		ClientType										m_ClientData;
	};

}




#endif //__ORIGIN_LSX_REQUEST_H__

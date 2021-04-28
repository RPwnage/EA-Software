#include <EATrace/EATrace.h>
#include <QByteArray>

#include "LSX/LsxMessages.h"
#include "LSX/LsxConnection.h"
#include "LSX/XMLDocument.h"
#include "services/debug/DebugService.h"

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
	        Request::Request( const QByteArray &packet, Connection *connection, Protocol encoding ) :
				m_connection(connection), 
				m_protocol(encoding)
	        {
		        // Don't let the connection object go away while this request exists
		        m_connection->ref();

		        //	Create Parser
				switch(m_protocol)
				{
				case PROTOCOL_XML:
					m_document.reset(CreateXmlDocument());
					break;
				case PROTOCOL_JSON:
					m_document.reset(CreateJsonDocument());
					break;
				default:
					ORIGIN_ASSERT_MESSAGE(0, "Unrecognized Encoding in LSX Request!");
					m_document.reset(CreateXmlDocument());
					break;
				}

                m_msg = packet;

				// Parse Request
		        m_document->Parse(m_msg.data());

				if(QString("LSX") != m_document->GetName())
                {
                    m_document->Root();

                    if(!m_document->FirstChild("LSX"))
                    {
			        return;
		        }
                }

				if(!m_document->FirstChild("Request"))
					return;

				m_recipient = m_document->GetAttributeValue("recipient");
				m_id = m_document->GetAttributeValue("id");

				if(!m_document->FirstChild())
			        return;

			    m_command = m_document->GetName();
		        
				if(m_recipient.isEmpty() || m_id.isEmpty() || m_command.isEmpty())
				{
					// Document is invalid, throw it away.
					m_document.reset(NULL);
				}

				return;
	        } 
		

	        Request::~Request()
	        {
		        m_connection->unref();

	        }


	        Response::Response( const Request &req) : 
				m_id(req.id()),
				m_sender(req.recipient()),
				m_protocol(req.protocol()),
                m_connection(req.connection()),
                m_bPending(true),
                m_bSent(false),
                m_ref(1) // Do implicit addRef
	        {
				init(RESPONSETYPE_RESPONSE);
	        }

			Response::Response( const Response &response) :
				m_protocol(response.m_protocol),
				m_id(response.m_id),
                m_sender(response.m_sender),
                m_connection(response.connection()),
                m_bPending(true),
                m_bSent(false),
                m_ref(1) // Do implicit addRef
	        {
				init(RESPONSETYPE_RESPONSE);
			}
		
			Response::Response( ResponseType type, Protocol protocol, Lsx::Connection * connection) : 
				m_protocol(protocol),
                m_id(0),
                m_connection(connection),
                m_bPending(true),
                m_bSent(false),
                m_ref(1) // Do implicit addRef
		    {
    			init(type);
		    }

			void Response::init(ResponseType type)
	        {
				switch(m_protocol)
		        {
				case PROTOCOL_XML:
					m_document.reset(CreateXmlDocument());
					break;
				case PROTOCOL_JSON:
					m_document.reset(CreateJsonDocument());
					break;
				default:
					ORIGIN_ASSERT_MESSAGE(0, "Unrecognized Encoding in LSX Response!");
					m_document.reset(CreateXmlDocument());
					break;
		        }

				m_document->AddChild("LSX");
				switch(type)
		        {
				case RESPONSETYPE_RESPONSE:
					m_document->AddChild("Response");
					m_document->AddAttribute("id", m_id);
					break;
				case RESPONSETYPE_EVENT:
					m_document->AddChild("Event");
			        break;
				default:
					ORIGIN_ASSERT_MESSAGE(0, "Unknown response type");
					m_document->AddChild("Response");
					m_document->AddAttribute("id", m_id);
			        break;
		        }

                if(!m_sender.isEmpty())
                {
				    m_document->AddAttribute("sender", m_sender);
                }

                if(m_connection)
                    m_connection->ref();

	        }

			Response::~Response()
	        {
                if(m_connection)
                    m_connection->unref();
	        }

	        void Response::setSender( const QByteArray & sender )
	        {
		        m_document->AddAttribute("sender", sender);
	        }

            void Response::send()
            {
                // Ignore if already sent.
                if(!m_bSent)
                {
                    if(m_connection)
                    {
                        m_connection->sendResponse(*this);
                        m_bSent = true;
                    }
                    else
                    {
                        ORIGIN_ASSERT_MESSAGE(0, "Sending a message without a connection. The code is wrong.");
                    }
                }
            }
        }
    }
}
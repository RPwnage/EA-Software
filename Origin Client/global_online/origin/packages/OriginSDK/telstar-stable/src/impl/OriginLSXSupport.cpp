#include "stdafx.h"
#include "OriginSDK/OriginTypes.h"

#include "OriginLSXSupport.h"

#include <string>
#include <iterator>

#include "rapidXml/rapidxml.hpp"
#include "rapidXml/rapidxml_print.hpp"
#include "encryption/SimpleEncryption.h"
#include <lsx.h>
#include <RapidXmlParser.h>

#include "OriginLSXRequest.h"
#include "OriginSDKimpl.h"


#ifdef ORIGIN_MAC
#include <unistd.h>
#include <sys/time.h>
#endif 

namespace origin_parser
{
    namespace rapidxml
    {
        void parse_error_handler(const char * /*what*/, void * /*where*/)
	    {
            //TODO: NSLog?
		    //OutputDebugStringA("#### Parser error:");
		    //OutputDebugStringA(what);
		    //OutputDebugStringA("\n");
	    }
    }
}

namespace Origin {


LSX::LSX() : 
	m_buffer(MAX_LSX_MESSAGE_SIZE)
{
#ifdef _WIN32
	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
#endif
}


LSX::~LSX()
{
	Disconnect();

#ifdef _WIN32
	WSACleanup();
#endif
}


bool LSX::Connect(uint16_t port)
{
	if(m_socket != INVALID_SOCKET)
		Disconnect();

	m_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	if(m_socket != INVALID_SOCKET)
	{
		sockaddr_in clientService;
		clientService.sin_family = AF_INET;
		inet_pton(AF_INET, "127.0.0.1", &(clientService.sin_addr));
		clientService.sin_port = htons( port );

		if(connect(m_socket, (struct sockaddr *)&clientService, sizeof(clientService)) != 0)
			return false;

		m_thread = std::thread([this] { Run(); });

		return m_thread.joinable();
	}

	return false;
}

bool LSX::Disconnect()
{
	if (m_socket != INVALID_SOCKET)
	{
		SOCKET tempSocket = m_socket;
		m_socket = INVALID_SOCKET;
#ifdef ORIGIN_PC
		shutdown(tempSocket, SD_BOTH);
		closesocket(tempSocket);
#else
		shutdown(tempSocket, SHUT_RDWR);
		close(tempSocket);
#endif
		m_useEncrypted = false;
	}

	if(m_thread.joinable())
		m_thread.join();

	return true;
}

void LSX::SetUseEncrypted(bool useEncrypted)
{
    m_useEncrypted = useEncrypted;
}

void LSX::SetSeed(int seed)
{
    m_seed = seed;
}

bool LSX::Write(AllocString request)
{
	bool result = false;
	if(IsConnected() && request.length()>0)
	{
		//OutputDebugStringA("@@@@@LSX::Write(");
		//OutputDebugStringA(request.c_str());
		//OutputDebugStringA(")@@@@@\n");

		AllocString outMessage;

		if (m_useEncrypted)
		{
			// Encrypt message before sending
			SimpleEncryption se(m_seed);
			outMessage = se.encrypt(request);
		}
		else
		{
			outMessage = request;
		}

		// write null-terminator as a message separator
		int cnt = send(m_socket, outMessage.c_str(), (int)(outMessage.length()+1), 0);
		result = (cnt != SOCKET_ERROR);
	}
	return result;
}

bool LSX::Read(AllocString &response)
{
	bool result = false;
	if(IsConnected())
	{
		std::lock_guard<std::mutex> lockIt(m_lock);
		if (m_buffer.HasMessage())
		{
			// copy message into the response string
			if (m_useEncrypted)
			{
				SimpleEncryption se(m_seed);
				response = se.decrypt(m_buffer.ReadPointer());
			}
			else
			{
				response = m_buffer.ReadPointer();
			}

			m_buffer.Read(m_buffer.ReadSize());

			if (response.length())
			{
				result = true;
			}
 		}
	}
	return result;
}

bool LSX::IsConnected() const
{
	return m_socket != INVALID_SOCKET;
}

bool LSX::HasReadDataToRead() const
{
	return m_buffer.HasMessage();
}

bool LSX::IsThreadRunning()
{
	return m_thread.joinable();
}

void LSX::Run()
{
	while (m_socket != INVALID_SOCKET)
	{
		bool signalCondition = false;
		char* readstart;
		int maxbytes;

		{
			std::lock_guard<std::mutex> lockIt(m_lock);

			readstart = m_buffer.WritePointer();
			maxbytes = (int)m_buffer.AvailableSpace();
		}

		if (maxbytes > 0)
		{
			size_t cnt = recv(m_socket, readstart, maxbytes, 0);

			if((cnt != SOCKET_ERROR) && (cnt > 0))
			{
				std::lock_guard<std::mutex> lockIt(m_lock);
				if(m_buffer.Write(cnt))
				{
					// Received a complete message
					signalCondition = true;
				}
			}
			else
			{
				// Either an error occurred, or the connection was closed peacefully
				signalCondition = true;
				
				if(m_socket != INVALID_SOCKET)
				{
#ifdef ORIGIN_PC
					shutdown(m_socket, SD_BOTH);
                    closesocket(m_socket);
#else
                    shutdown(m_socket, SHUT_RDWR);
					close(m_socket);
#endif
					m_socket = INVALID_SOCKET;
				}
			}
		}
		else
		{
			signalCondition = true;
			std::this_thread::yield();
		}

		if (signalCondition)
		{
			std::lock_guard<std::mutex> msgLock(m_messageLock);
			m_condition.notify_one();
		}
	}
	return;
}

}; // namespace Origin

#include <string.h>
#include <stdio.h>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <stdarg.h>
#include <assert.h>
#include <iomanip>
#include <iostream>

#include "AbstractClient.h"

using namespace std;

namespace sonar
{

class to_lower
{
public:
    char operator() (char c) const        // notice the return type
    {
        return (char) tolower(c);
    }
};

AbstractClient::AbstractClient (time_t _keepaliveTimeout)
	: m_readBuffer(PROTO_BUFFER_LENGTH)
	, m_writeBuffer(PROTO_BUFFER_LENGTH)
{
	m_pSocket = NULL;

	m_nCurrProtoId = 1;
	m_tsNextKeepAlive = time(0) + _keepaliveTimeout;
	m_bConnectionLost = false;

	m_bBlockEvents = true;
	m_bStopQueue = true;

	m_keepaliveTimeout = _keepaliveTimeout;
}

AbstractClient::~AbstractClient (void)
{
	if (m_pSocket)
	{
		delete m_pSocket;
	}
}

void AbstractClient::setSocket (TcpSocket *_pSocket)
{
	m_nCurrProtoId = 0;
	m_pSocket = _pSocket;
	m_sockfd = (SOCKET) -1;

	if (m_pSocket)
	{
		m_sockfd = m_pSocket->getSocket ();
	}

	m_bConnectionLost = false;
	m_readBuffer.flush ();
	m_writeBuffer.flush ();
}

TcpSocket *AbstractClient::getSocket (void)
{
	assert (m_pSocket != NULL);
	return m_pSocket;
}


int AbstractClient::sendFormattedToQueue (const char *_format, ...)
{
	char buffer[PROTO_COMMAND_LENGTH + 1];

	int length;

	va_list valist;
	va_start (valist, _format);

	if((length = vsnprintf(buffer, PROTO_COMMAND_LENGTH, _format, valist)) == -1)
	{
		SYSLOGX(LOG_ERR, "connection lost\n");
		m_bConnectionLost = true;
		return -1;
	}

	va_end (valist);

	m_writeBuffer.push(buffer, length);

#ifdef _DEBUG
	for (int index = 0; index < length; index ++)
	{
		if (buffer[index] == '\t')
		{
			buffer[index] = ' ';
		}
	}

	buffer[length] = '\0';
	SYSLOG (LOG_DEBUG, ">>> %08lx %s", clock (), buffer);
#endif

	if (!m_bStopQueue)
	{
		sendQueue ();
	}

	return 0;
}

int AbstractClient::processData (void *_pData, int _cbLength)
{
	//NOTE: This function get's data suffixed by a '\n' and it assumes '\0' as line terminator

	// Parse into a command 
	char *argv[PROTO_MAX_ARGS];
	int argc = 0;
	int index;
	int length = _cbLength;

	char *pLineBuffer = (char *) _pData;

	argv[argc] = pLineBuffer + 0;

	for (index = 0; index < length; index ++)
	{
		if (pLineBuffer[index] == '\t')
		{
			pLineBuffer[index] = '\0';
			argc ++;

			if (argc >= PROTO_MAX_ARGS)
			{
				sendErrorReply (0, "TOO_MANY_ARGUMENTS");
				return 0;
			}

			argv[argc] = pLineBuffer + index + 1;

		}
		if(pLineBuffer[index] == '\n')
			pLineBuffer[index] = '\0';
	}

	argc ++;

#ifdef _DEBUG
	std::stringstream buffer;
	buffer << "<<<" << std::setw(8) << clock();

	for (int index = 0; index < argc; index ++)
	{
		buffer << argv[index] << " ";
	}
	buffer << std::endl;
	SYSLOG (LOG_DEBUG, "%s", buffer.str().c_str());
#endif

	PROTOID id;

	if (sscanf (argv[0], "%lu", &id) != 1)
	{
		sendErrorReply (0, "INVALID_ARGUMENT");
		return 0;
	}

	argc --;

	if (argc < 1)
	{
		sendErrorReply (id, "NOT_ENOUGH_ARGUMENTS");
		return 0;
	}
	
	return processCommand (argv + 1, argc, id);
}

bool AbstractClient::processSocket (void)
{
	sendQueue ();

	int recvResult;
	char buffer[PROTO_COMMAND_LENGTH];

	// Proceed normal recv/send operations
	// Recv operations
	do
	{
		if (!m_pSocket)
		{
			return false;
		}

		recvResult = m_pSocket->recv (buffer, PROTO_COMMAND_LENGTH);

		if (recvResult == 0)
		{
			SYSLOGX(LOG_ERR, "recvResult == 0, connection lost\n");
			m_bConnectionLost = true;
			return false;
		}

		if (recvResult == -1)
		{
			if(!m_pSocket->wouldBlock ())
			{
				SYSLOGX(LOG_ERR, "recv err == %d, connection lost\n", errno);
				m_bConnectionLost = true;
				return false;
			}

			//fprintf (stderr, "AbstractClient::processSocket(), recvResult == -1\n");
			return true;
		}

		if (recvResult > 0)
		{
			m_readBuffer.push (buffer, recvResult);
			//SYSLOGX(LOG_DEBUG, "recvResult > 0\n");
		}

		int popLength;

		while ( (popLength = m_readBuffer.popUntilChar (buffer, PROTO_COMMAND_LENGTH, '\n')) > 0)
		{
			//SYSLOGX(LOG_DEBUG, "buffer = [%s]\n", buffer);
			if (processData (buffer, popLength) == -1)
			{
				return false;
			}
		}

	} while (recvResult > 0);


	return true;
}


void AbstractClient::sendErrorReply (PROTOID _id, const char *_pstrError)
{
	sendFormattedToQueue ("%u\tREPLY\tERROR\t%s\n", _id, _pstrError);
}

void AbstractClient::sendSuccessReply (PROTOID _id)
{
	sendFormattedToQueue ("%u\tREPLY\tOK\n", _id);
}

int AbstractClient::sendQueue (void)
{
	size_t bytesToSend = m_writeBuffer.getPushPtr() - m_writeBuffer.getPopPtr();
	if (bytesToSend == 0)
	{
		return 0;
	}

	if (bytesToSend > 8192) 
		bytesToSend = 8192;


	int sendResult = m_pSocket->send (m_writeBuffer.getPopPtr(), bytesToSend);

	if (sendResult == 0)
	{
		SYSLOGX(LOG_ERR, "sendResult == 0, connection lost\n");
		m_bConnectionLost = true;
		return -1;
	}

	if (sendResult == -1)
	{
		if (!m_pSocket->wouldBlock ())
		{
			SYSLOGX(LOG_ERR, "sendResult = %d, connection lost\n", errno);
			m_bConnectionLost = true;
			return -1;
		}

		return 0;
	}

	m_writeBuffer.pop (NULL, sendResult);
	return 0;
}

int AbstractClient::handleKeepAlive (void)
{
	time_t ts = time (0);

	if (ts > this->m_tsNextKeepAlive)
	{
		sendFormattedToQueue ("%u\tKEEPALIVE\n", getProtoId ());
		m_tsNextKeepAlive = time(0) + m_keepaliveTimeout + rand () % m_keepaliveTimeout;
	}

	return 0;
}

bool AbstractClient::isConnectionLost (void)
{
	return m_bConnectionLost;
}


void AbstractClient::blockEvents (bool _bBlock)
{
	m_bBlockEvents = _bBlock;
}

void AbstractClient::stopQueue (bool _bStop)
{
	m_bStopQueue = _bStop;

}


void AbstractClient::setCommandHandler (const string &_cmdName, CommandHandler _handler)
{
	m_handlerMap[_cmdName] = _handler;
}

int AbstractClient::processCommand (char **argv, int argc, PROTOID _id)
{
	string name = argv[0];

	std::transform (name.begin(), name.end(), name.begin(), to_lower ());

	map<const string, CommandHandler>::iterator iter = m_handlerMap.find (name);

	if (iter == m_handlerMap.end ())
	{
		sendErrorReply (_id, "UNKNOWN_COMMAND");
		return 0;
	}

	CommandHandler &handler = iter->second;

	return handler.first->callCommandHandler (handler.second, argv + 1, argc - 1, _id) ? 0 : -1;
}

AbstractClient::PROTOID AbstractClient::getProtoId (void)
{
	PROTOID retId = m_nCurrProtoId;

	m_nCurrProtoId ++;
	if (m_nCurrProtoId == 0)		// like that's ever gonna happen ;)
	{
		m_nCurrProtoId ++;
	}
	return retId;
}

}
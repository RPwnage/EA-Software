#include "AbstractOutboundClient.h"
#include <stdlib.h>
#include <stdio.h>

using namespace std;

//FIXME: Validate control address in advance
//FIXME: Behave niceley when shutting down (Send "UNREGISTER")

namespace sonar
{

const char *AbstractOutboundClient::getStateName(CLIENTSTATE state)
{
	switch (state)
	{
		case CS_NONE:			return "None";
		case CS_CONNECTION_PENDING:	return "Connection pending";
		case CS_CONNECTED:		return "Connected";
		case CS_REGISTER_PENDING:	return "Register pending";
		case CS_REGISTERED:		return "Registered";
		case CS_DISCONNECTED:		return "Disconnected";
		default:			return "UNKNOWN";
	}
}

AbstractOutboundClient::AbstractOutboundClient (CString &_controlAddress, time_t _keepaliveTimeout, time_t _connectTimeout, time_t _registerTimeout, time_t _disconnectTimeout) : 
	AbstractClient (_keepaliveTimeout)
{
	m_state = CS_NONE;

	setCommandHandler ("reply", CommandHandler(this, (void *) staticReply));
	setCommandHandler ("keepalive", CommandHandler(this, (void *) staticKeepalive));
	setCommandHandler ("unregister", CommandHandler(this, (void *) staticUnregister));

	m_controlAddress = _controlAddress;
	m_connectTimeout = _connectTimeout;
	m_registerTimeout = _registerTimeout;
	m_disconnectTimeout = _disconnectTimeout;
}


AbstractOutboundClient::CLIENTSTATE AbstractOutboundClient::getState ()
{
	return m_state;
}

int AbstractOutboundClient::setState (CLIENTSTATE _state)
{
	CLIENTSTATE oldstate = m_state;
	m_state = _state;
#ifdef _DEBUG
	SYSLOG (LOG_DEBUG, "%s: Changing state from \"%s\" to \"%s\"\n", __FUNCTION__, getStateName(oldstate), getStateName(_state));
#endif

	switch (oldstate)
	{
		case CS_NONE:
			switch (_state)
			{
				case CS_NONE:
				case CS_DISCONNECTED:
				case CS_CONNECTED:
				case CS_REGISTER_PENDING:
				case CS_REGISTERED:
					return -1;
					break;

				case CS_CONNECTION_PENDING:
                default:
					break;
			}	
			break;

		case CS_CONNECTION_PENDING:
			switch (_state)
			{
				case CS_NONE:
				case CS_CONNECTION_PENDING:
				case CS_REGISTER_PENDING:
				case CS_REGISTERED:
					return -1;
					break;

				case CS_DISCONNECTED:
				case CS_CONNECTED:
                default:
					break;
			}	
			break;

		case CS_CONNECTED:
			switch (_state)
			{
				case CS_NONE:
				case CS_CONNECTION_PENDING:
				case CS_CONNECTED:
				case CS_REGISTERED:
					return -1;

				case CS_DISCONNECTED:
				case CS_REGISTER_PENDING:
                default:
					break;
			}	
			break;

		case CS_REGISTER_PENDING:
			switch (_state)
			{
				case CS_NONE:
				case CS_CONNECTION_PENDING:
				case CS_CONNECTED:
				case CS_REGISTER_PENDING:
					return -1;
				case CS_DISCONNECTED:
				case CS_REGISTERED:
                default:
					break;
			}	
			break;

		case CS_REGISTERED:
			switch (_state)
			{
				case CS_NONE:
				case CS_CONNECTION_PENDING:
				case CS_CONNECTED:
				case CS_REGISTER_PENDING:
				case CS_REGISTERED:
					return -1;
				case CS_DISCONNECTED:
                default:
					break;
			}	
			break;

		case CS_DISCONNECTED:
			switch (_state)
			{
				case CS_DISCONNECTED:
				case CS_CONNECTED:
				case CS_REGISTER_PENDING:
				case CS_REGISTERED:
				case CS_NONE:
					return -1;

				case CS_CONNECTION_PENDING:
                default:
					break;
			}	
			break;
      
        default:
			return -1;
	}

	int flag = 1;

	// Execute state change

#ifdef _DEBUG
	SYSLOG (LOG_DEBUG, "%s: Executing state %s\n", __FUNCTION__, getStateName (_state));
#endif

	switch (_state)
	{
		case CS_NONE:
			return -1;

		case CS_CONNECTION_PENDING:
		{
			stopQueue (true);
			blockEvents (true);

			// Setup connection object (non-blocking)
			TcpSocket *pSocket = new TcpSocket ();
			pSocket->setNonBlocking (true);		

			setsockopt (pSocket->getSocket (), IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof (int));

			// Setup connection timeout
			m_tsConnectExpire = time (0) + m_connectTimeout;

			string hostname;
			string port;

			size_t offset = m_controlAddress.find_first_of(':');

			if (offset == string::npos)
			{
				hostname = m_controlAddress;
				port = "22990";
			}

			else
			{
				hostname = m_controlAddress.substr (0, offset);
				port = m_controlAddress.substr (offset + 1);
			}
			
			int nPort = atoi (port.c_str ());


			// Connect
			pSocket->connect (hostname.c_str (), nPort);
			setSocket (pSocket);
			break;
		}

		case CS_CONNECTED:
			// Send "REGISTER" (registration)
			//REGISTER<TAB>[hostname]<TAB>[VOIP addr]<TAB>[VOIP port]<TAB>[bwKbit]<TAB>[bwPackets]<LF>
			stopQueue (false);

			sendRegister ();

			// Change state to REGISTER_PENDING
			setState (CS_REGISTER_PENDING);
			break;

		case CS_REGISTER_PENDING:
			m_tsRegisterExpire = time(0) + m_connectTimeout;
			// Do nothing
			break;

		case CS_REGISTERED:
		{

			stopQueue (true);
			blockEvents (false);

			sendState ();

			stopQueue (false);
			break;
		}

		case CS_DISCONNECTED:
        {
			blockEvents (true);
			stopQueue (true);

			// Tear down connection object

			TcpSocket *pSocket = getSocket ();
			delete pSocket;

			setSocket (NULL);

			if (m_disconnectTimeout == -1)
			{
				SYSLOG (LOG_WARNING, "NOTICE! Connection or registration failed from fatal causes, no reconnect attempt will be made.\n");
				m_tsDisconnectExpire = -1;
			}
			else
			{
				time_t nSec =  m_disconnectTimeout + rand () % m_disconnectTimeout;

				m_tsDisconnectExpire = time (0) + nSec;

				SYSLOG (LOG_NOTICE, "Retrying connection in %d seconds\n", (int) nSec);
			}

			break;
        }
      
        default:
            return -1;
	}


	this->onStateChange (oldstate, _state);
	return 0;
}



bool AbstractOutboundClient::update (void)
{
	if (isConnectionLost ())
	{
		m_disconnectTimeout = onConnectionLost ("SOCKET_ERROR");
		SYSLOG (LOG_DEBUG, "isConnectionLost() == true\n");
		setState (CS_DISCONNECTED);
	}

	switch (m_state)
	{
	case CS_NONE:
		break;

	case CS_DISCONNECTED:
		if (m_tsDisconnectExpire != -1 && time (0) > this->m_tsDisconnectExpire)
		{
			setState (CS_CONNECTION_PENDING);
		}
		return true;

	case CS_CONNECTION_PENDING:
		{
			// Check connection timeout
			if (time (0) > this->m_tsConnectExpire)
			{
				setState (CS_DISCONNECTED);
				return true;
			}

			// Check for connection
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 0;

			fd_set writeSet;
			FD_ZERO (&writeSet);
			FD_SET (getSocket ()->getSocket (), &writeSet);

			int selectResult = select ( (int) getSocket ()->getSocket () + 1, NULL, &writeSet, NULL, &tv);

			if (selectResult > 0)
			{
				// If connection change to BCS_CONNECTED
				setState (CS_CONNECTED);
			}
			break;
		}

	case CS_CONNECTED:
		processSocket ();
		break;

	case CS_REGISTER_PENDING:
		processSocket ();
		if (time (0) > this->m_tsRegisterExpire)
		{
			setState (CS_DISCONNECTED);
			return true;
		}
		break;

	case CS_REGISTERED:
		processSocket ();
		handleKeepAlive ();
		break;

	default:
		{
#ifdef _DEBUG
			SYSLOG (LOG_ERR, "%s: STATE ERROR: unknown state!\n", __FUNCTION__);
#endif
			break;
		}
	}

	return true;
}


typedef bool (*PFNHANDLER)(AbstractOutboundClient *, char **, int, int);

bool AbstractOutboundClient::callCommandHandler (void *_pCmd, char **argv, int argc, int _id)
{
	PFNHANDLER pfnHandler = (PFNHANDLER) _pCmd;
	return pfnHandler (this, argv,argc, _id);
}

bool AbstractOutboundClient::reply(char **argv,  int argc, int _id )
{
	if(argc < 1)
	{
		sendErrorReply(_id, "NOT_ENOUGH_ARGUMENTS");
		return false;
	}

	if (getState () == CS_REGISTER_PENDING)
	{
		if(strcmp (argv[0], "OK") == 0)
		{
			if (!onRegisterSuccess (argv + 1, argc - 1))
			{
				return false;
			}

			SYSLOG (LOG_INFO, "Successfully registrated with control server!\n");
			setState (CS_REGISTERED);
			return true;
		}
		else if(strcmp (argv[0], "ERROR") == 0)
		{
			SYSLOG (LOG_INFO, "Error when registering: %s\n", argc >= 2 ? argv[1] : "");
			m_disconnectTimeout = onRegisterError (argv + 1, argc - 1);
			setState (CS_DISCONNECTED);
			return false;

		}
	}
	else
	{
		onReply (argv, argc, _id);
	}

	return true;
}

void AbstractOutboundClient::onStateChange (CLIENTSTATE _oldState, CLIENTSTATE _newState)
{
}

bool AbstractOutboundClient::keepalive (char **argv,  int argc, int _id )
{
	sendSuccessReply (_id);
	return true;
}

bool AbstractOutboundClient::unregister (char **argv, int argc, int _id)
{
	SYSLOGX(LOG_DEBUG, "unregistering");

	if (argc < 1)
	{
		m_disconnectTimeout = onConnectionLost ("");
	}
	else
	{
		m_disconnectTimeout = onConnectionLost (argv[0]);
	}

	setState (CS_DISCONNECTED);
	return false;
}

bool AbstractOutboundClient::staticReply (AbstractOutboundClient *pInst, char **argv, int argc, int _id )
{
	return pInst->reply (argv, argc, _id);
}

bool AbstractOutboundClient::staticUnregister (AbstractOutboundClient *pInst, char **argv, int argc, int _id)
{
	return pInst->unregister (argv, argc, _id);
}

bool AbstractOutboundClient::staticKeepalive (AbstractOutboundClient *pInst, char **argv, int argc, int _id )
{
	return pInst->keepalive (argv, argc, _id);
}

void AbstractOutboundClient::setDisconnectTimeout (time_t _timeout)
{
	m_disconnectTimeout = _timeout;
}

}

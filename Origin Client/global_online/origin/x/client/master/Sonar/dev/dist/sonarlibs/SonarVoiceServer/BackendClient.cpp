#include "BackendClient.h"
#include <sstream>
#include "Server.h"
#include "ClientConnection.h"
#include "Operator.h"

namespace sonar
{

#define PROTOCOL_VERSION "10.0"

bool BackendClient::rsaValidateToken (RSA *rsaPubKey, CString &message, const char *signature, size_t cbSignature)
{
	SYSLOG (LOG_DEBUG, "Enter rsaValidateToken()\n");

	if (message.empty() || cbSignature == 0)
	{
		return false;
	}

	SHA_CTX sha_ctx = { 0 };
	unsigned char digest[SHA_DIGEST_LENGTH];

	int rc;
	(void) rc;

	rc = SHA1_Init(&sha_ctx);
	rc = SHA1_Update(&sha_ctx, message.c_str(), message.size());
	rc = SHA1_Final(digest, &sha_ctx);

	int err = RSA_verify(NID_sha1, digest, sizeof(digest), (unsigned char *) signature, cbSignature, rsaPubKey);

	if (!err)
	{
        SYSLOG (LOG_ERR, "RSA_verify() returned error code [%d]\n", err);
		return false;
	}

	SYSLOG (LOG_DEBUG, "Exit rsaValidateToken()\n");

	return true;
}

bool BackendClient::decodeBase64 (CString &input, char *outBuffer, size_t *outInSize)
{
  BIO *b64, *bmem, *bmem2;
  b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64,BIO_FLAGS_BASE64_NO_NL);
  bmem = BIO_new_mem_buf( (void *) input.c_str(), input.size());
  bmem2 = BIO_push(b64, bmem);
  int result = BIO_read(bmem2, outBuffer, *outInSize);
  BIO_free_all(bmem2);
      
  *outInSize = (size_t) result;

  if (result < 1)
  {
	  return false;
  }

  return true;
}


static String SafeArgument (const char *_pstrArg, size_t _maxLength)
{
	size_t length = strlen (_pstrArg);

	if (length > _maxLength)
	{
		length = _maxLength;
	}

	return CString (_pstrArg, length);
}

BackendClient::BackendClient (
	CString &_hostname,
	CString &_controlAddress,
	CString &_location,
	time_t _connectTimeout, 
	time_t _registerTimeout, 
	time_t _disconnectTimeout,
	int _maxClients,
	int _voipPort)
	: AbstractOutboundClient (_controlAddress, protocol::BACKEND_KEEPALIVE_INTERVAL_SEC, _connectTimeout, _registerTimeout, _disconnectTimeout)
	, m_uuid(_voipPort)
{
	m_maxClients = _maxClients;
	m_voipPort = _voipPort;
	m_hostname = _hostname;
	m_location = _location;

	setCommandHandler ("client_unregister", CommandHandler(this, (void *) staticClientUnregister));
	setCommandHandler ("channel_destroy", CommandHandler(this, (void *) staticChannelDestroy));
	setCommandHandler ("channel_info", CommandHandler(this, (void *) staticChannelInfo));
	setCommandHandler ("client_register_ok", CommandHandler(this, (void *) staticClientRegisterOk));
	blockEvents (true);

	m_rsaPubKey = NULL;
}

BackendClient::~BackendClient(void)
{
	if (m_rsaPubKey)
	{
		RSA_free(m_rsaPubKey);
		m_rsaPubKey = NULL;
	}
}

RSA *BackendClient::getRSAPubKey(void)
{
	return m_rsaPubKey;
}

bool BackendClient::updateRSAPubKey (const void *pubKey, size_t length)
{
	const unsigned char *ptr = (unsigned char *) pubKey;

	BIO* keyBio = BIO_new_mem_buf( (void *) ptr, length);
	EVP_PKEY *evtPubKey = d2i_PUBKEY_bio(keyBio, NULL);
	RSA *rsaPubKey = EVP_PKEY_get1_RSA(evtPubKey);

	EVP_PKEY_free(evtPubKey);
	BIO_free_all(keyBio);

	if (rsaPubKey == NULL)
	{
		return false;
	}

	if (m_rsaPubKey)
	{
		RSA_free(m_rsaPubKey);
	}

	m_rsaPubKey = rsaPubKey;
	return true;
}


void BackendClient::sendRegister (void)
{
	/*
	REGISTER [uuid] [localIP:port] [server version] [max bandwidth] [max packets] */

	struct sockaddr_in localAddr;
	socklen_t len = sizeof (localAddr);

	const char *pstrAddr;

	if (getsockname (getSocket ()->getSocket (), (struct sockaddr *) &localAddr, &len) == -1)
		pstrAddr = "0.0.0.0";
	else
		pstrAddr = inet_ntoa (localAddr.sin_addr);

	if (!m_hostname.empty ())
	{
		pstrAddr = m_hostname.c_str ();
	}

	sendFormattedToQueue ("%u\tREGISTER\t%s\t%s:%d\t%d\t%s\t%d\t%s\n", 
		getProtoId (),
		m_uuid.get ().c_str (),
		pstrAddr,
		ntohs (localAddr.sin_port), 
		m_voipPort,
		PROTOCOL_VERSION,
		m_maxClients, 
		m_location.c_str());
}

void BackendClient::sendState (void)
{
	// Send channels

	const Server::ChannelList & channelList = Server::getInstance().getChannelList();
  	for (Server::ChannelList::const_iterator iter = channelList.begin(); iter != channelList.end(); iter ++)
	{
		Channel *channel = (*iter);

		if( channel->getClientCount() > 0 )
		{
			// find first client
			const ClientConnection *client = channel->getClient(0);
			if( client != NULL )
			{
				eventClientState( client->getOperator().getId().c_str(), *channel );
			}
		}
	}
}

void BackendClient::onReply (char **argv, int argc, int _id)
{
}

typedef bool (*PFNHANDLER)(BackendClient *, char **, int, int);

bool BackendClient::onCommand (void *_pCmd, char **argv, int argc, int _id)
{
	PFNHANDLER handler = (PFNHANDLER) _pCmd;
	return handler (this, argv + 1, argc - 1, _id);
}

bool BackendClient::clientUnregister (char **argv, int argc, int _id)
{
	/*
	Arguments:
	[serverId] [operatorId] [userId] [reasonType] [reasonDesc] */


	if (argc < 5)
	{
		sendErrorReply (_id, "NOT_ENOUGH_ARGUMENTS");
		return false;
	}

	String serverId = SafeArgument (argv[0], 255);
	String operatorId = SafeArgument (argv[1], 255);
	String userId =	SafeArgument (argv[2], 255);
	String reasonType = SafeArgument (argv[3], 255);
	String reasonDesc = SafeArgument (argv[4], 255);

	Operator &oper = Server::getInstance().getOperator(operatorId.c_str());
	ClientConnection *client = oper.getClient(userId);


	if (client == NULL)
	{
		sendErrorReply (_id, "CLIENT_NOT_FOUND");
		return false;
	}
	
	client->disconnect(reasonType, reasonDesc);
	sendSuccessReply (_id);
	return true;
}


bool BackendClient::channelDestroy (char **argv, int argc, int _id)
{
	/*
	Arguments:
	[serverId] [operatorId] [channelId] [reasonType] [reasonDesc] */

	if (argc < 5)
	{
		sendErrorReply (_id, "NOT_ENOUGH_ARGUMENTS");
		return false;
	}

	String serverId =	SafeArgument (argv[0], 255);
	String operatorId =	SafeArgument (argv[1], 255);
	String channelId =	SafeArgument (argv[2], 255);
	String reasonType = SafeArgument (argv[3], 255);
	String reasonDesc = SafeArgument (argv[4], 255);

	Operator &oper = Server::getInstance().getOperator(operatorId);

	Channel *channel = oper.getChannel(channelId);

	if (channel == NULL)
	{
		sendErrorReply (_id, "CHANNEL_NOT_FOUND");
		return false;
	}

	size_t len;
	ClientConnection **clientsOrg = channel->getClients(&len);
	ClientConnection **clients = new ClientConnection*[len];
	memcpy (clients, clientsOrg, sizeof(ClientConnection *) * len);

	for (size_t index = 0; index < len; index ++)
	{
		ClientConnection *client = clients[index];

		if (client == NULL)
			continue;

		client->disconnect(reasonType, reasonDesc);
	}

	sendSuccessReply (_id);
	delete clients;
	return true;
}

bool BackendClient::channelInfo (char **argv, int argc, int _id)
{
	/*
	Arguments:
	None */

	if (argc > 0)
	{
		sendErrorReply (_id, "TOO_MANY_ARGUMENTS");
		return false;
	}

	sendState();

	sendSuccessReply (_id);
	return true;
}

bool BackendClient::clientRegisterOk (char **argv, int argc, int _id)
{
	/*
	Arguments:
	[operatorId] [channelId] [channelName] [userId] [userName] */

	if (argc < 5)
	{
		sendErrorReply (_id, "NOT_ENOUGH_ARGUMENTS");
		return false;
	}

	String operatorId =	SafeArgument (argv[0], 255);
	String channelId =	SafeArgument (argv[1], 255);
	String channelName =	SafeArgument (argv[2], 255);
	String userId =	        SafeArgument (argv[3], 255);
	String userName =       SafeArgument (argv[4], 255);

	Operator &oper = Server::getInstance().getOperator(operatorId);

	// channel name
	Channel *channel = oper.getChannel(channelId);
	if (channel == NULL)
	{
		sendErrorReply (_id, "CHANNEL_NOT_FOUND");
		return false;
	}
	
	channel->setName(channelName);

	// originId
	ClientConnection *client = oper.getClient(userId);
	if (client == NULL)
	{
		sendErrorReply (_id, "CLIENT_NOT_FOUND");
		return false;
	}
	
	client->setName(userName);
	
	sendSuccessReply (_id);
	return true;
}

bool BackendClient::staticChannelInfo (BackendClient *_pInstance, char **argv, int argc, int _id)
{
	return _pInstance->channelInfo (argv, argc, _id);
}

bool BackendClient::staticClientRegisterOk (BackendClient *_pInstance,char **argv, int argc, int _id)
{
	return _pInstance->clientRegisterOk (argv, argc, _id);
}

bool BackendClient::staticChannelDestroy (BackendClient *_pInstance, char **argv, int argc, int _id)
{
	return _pInstance->channelDestroy (argv, argc, _id);
}

bool BackendClient::staticClientUnregister (BackendClient *_pInstance,char **argv, int argc, int _id)
{
	return _pInstance->clientUnregister(argv, argc, _id);
}

void BackendClient::eventClientState (const char *_pstrOperatorId, Channel &channel)
{
	if (m_bBlockEvents)
	{
		return;
	}

	// construct user info
	std::stringstream userInfo;
	
	size_t len;
	ClientConnection **clients = channel.getClients(&len);
	for (size_t index = 0; index < len; index ++)
	{
		ClientConnection *peer = clients[index];

		if (peer == NULL)
		{
			continue;
		}

		userInfo << '\t' << peer->getUserId().c_str();
	}

	sendFormattedToQueue ("%u\tEVENT_STATE_CLIENT\t%s\t%s\t%s\t%s%s\n",
		getProtoId (),
		m_uuid.get().c_str (),
		_pstrOperatorId,
		channel.getId().c_str(),
		channel.getDesc().c_str(),
		userInfo.str().c_str());	
}

void BackendClient::eventChannelDestroyed (const char *_pstrOperatorId, const char *_pstrChannelId)
{
	if (m_bBlockEvents)
	{
		return;
	}

	// Invoked by Server::UnregisterChannel
	sendFormattedToQueue("%u\tEVENT_CHANNEL_DESTROYED\t%s\t%s\t%s\n", getProtoId(), m_uuid.get().c_str (), _pstrOperatorId, _pstrChannelId);
}

void BackendClient::eventClientRegisteredToChannel(const char *_pstrOperatorId, const char *_pstrUserId, const char *_pstrUsername, const char *_pstrChannelId, const char *_pstrChannelDesc, const char *_pstrSourceIP, int _nPort, int _nClientCount)
{
	if (m_bBlockEvents)
	{
		return;
	}

	// Invoked by Server::RegisterClient
	sendFormattedToQueue("%u\tEVENT_CLIENT_REGISTERED_TOCHANNEL\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%d\t%d\n", getProtoId (), m_uuid.get().c_str (), _pstrOperatorId, _pstrUserId, _pstrUsername, _pstrChannelId, _pstrChannelDesc, _pstrSourceIP, _nPort, _nClientCount);
}

void BackendClient::eventClientUnregistered (const char *_pstrOperatorId, const char *_pstrUserId, const char *_pstrChannelId, int _nClientCount)
{
	if (m_bBlockEvents)
	{
		return;
	}

	// Invoked by Server::UnregisterClient
	// < CLIENT_UNREGISTERED<TAB>[userId]<LF>
	sendFormattedToQueue("%u\tEVENT_CLIENT_UNREGISTERED\t%s\t%s\t%s\t%s\t%d\n", getProtoId(), m_uuid.get().c_str (), _pstrOperatorId, _pstrUserId, _pstrChannelId, _nClientCount);
}

void BackendClient::eventVoiceServerMetrics_State (const char *_pstrMetrics)
{
    if (m_bBlockEvents)
    {
        return;
    }

    sendFormattedToQueue ("%u\tEVENT_SERVER_STATE\t%s\t%s\n", 
        getProtoId (),
        m_uuid.get().c_str (),
        _pstrMetrics);
}

void BackendClient::eventVoiceServerMetrics_Counter (const char *_pstrMetrics)
{
    if (m_bBlockEvents)
    {
        return;
    }

    sendFormattedToQueue ("%u\tEVENT_SERVER_COUNTER\t%s\t%s\n", 
        getProtoId (),
        m_uuid.get().c_str (),
        _pstrMetrics);
}

void BackendClient::eventVoiceServerMetrics_Health (const char *_pstrMetrics)
{
    if (m_bBlockEvents)
    {
        return;
    }

    sendFormattedToQueue ("%u\tEVENT_SERVER_HEALTH_STATS\t%s\t%s\n", 
        getProtoId (),
        m_uuid.get().c_str (),
        _pstrMetrics);
}

void BackendClient::eventVoiceServerMetrics_System  (const char *_pstrMetrics)
{
    if (m_bBlockEvents)
    {
        return;
    }

    sendFormattedToQueue ("%u\tEVENT_SERVER_SYSTEM_METRICS\t%s\t%s\n", 
        getProtoId (),
        m_uuid.get().c_str (),
        _pstrMetrics);
}

void BackendClient::eventVoiceServerMetrics_Network  (const char *_pstrMetrics)
{
    if (m_bBlockEvents)
    {
        return;
    }

    sendFormattedToQueue ("%u\tEVENT_SERVER_NETWORK_STATS\t%s\t%s\n", 
        getProtoId (),
        m_uuid.get().c_str (),
        _pstrMetrics);
}

time_t BackendClient::onConnectionLost (const char *_pstrReason)
{
	if (strcmp (_pstrReason, "LOGGEDIN_ELSEWHERE") == 0)
	{
		return 86400;
	}
	else
	if (strcmp (_pstrReason, "TIMEOUT") == 0)
	{
		return 3600;
	}

	return 10;
}

bool BackendClient::onRegisterSuccess (char **argv, int argc)
{
	SYSLOG (LOG_DEBUG, "Enter onRegisterSuccess()\n");

	if (argc < 4)
	{
		return false;
	}

	String algo = argv[0];
	String format = argv[1];
	String base64PubKey = argv[2];
	String backendEventInterval = argv[3];

	SYSLOG (LOG_DEBUG, "algo = %s\n", algo.c_str());
	SYSLOG (LOG_DEBUG, "format = %s\n", format.c_str());
	SYSLOG (LOG_DEBUG, "base64PubKey = %s\n", base64PubKey.c_str());
	SYSLOG (LOG_DEBUG, "backendEventInterval = %s\n", backendEventInterval.c_str());

	if (base64PubKey.size() > 4096)
	{
		return false;
	}
	
	char binaryPubKey[8192];
	size_t cbBinaryPubKey = sizeof(binaryPubKey);

	if (!decodeBase64(base64PubKey, binaryPubKey, &cbBinaryPubKey))
	{
		return false;
	}

	if (!updateRSAPubKey (binaryPubKey, cbBinaryPubKey))
	{
		return false;
	}

	if (!updateBackendEventInterval(backendEventInterval))
	{
		return false;
	}

	SYSLOG (LOG_DEBUG, "Exit onRegisterSuccess()\n");
	
	return true;
}

time_t BackendClient::onRegisterError (char **argv, int argc)
{
	if (argc < 1)
	{
		return -1;
	}
	if (strcmp (argv[0], "TRY_AGAIN") == 0)
	{
		return 10;
	}
	else
	if (strcmp (argv[0], "CHALLENGE_EXPIRED") == 0)
	{
		return 3600;
	}
	else
	if (strcmp (argv[0], "WRONG_VERSION") == 0)
	{
		SYSLOG (LOG_EMERG, "FATAL! This server is outdated and is not compatible with the control services\n");
		return -1;
	}
	else
	{
		return -1;
	}

}

bool BackendClient::updateBackendEventInterval(sonar::CString &backendEventInterval)
{
	int interval = atoi(backendEventInterval.c_str());
	if( interval <= 0 )
	{
        SYSLOG (LOG_DEBUG, "backend event interval [%d] <= 0!", interval);
		return false;
	}
	Server::getInstance().updateBackendEventInterval(interval);

	return true;
}

}

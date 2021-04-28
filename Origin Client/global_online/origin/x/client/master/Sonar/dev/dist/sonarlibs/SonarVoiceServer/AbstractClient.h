#pragma once
#include <SonarCommon/Common.h>

#include "ByteBuffer.h"
#include "TcpSocket.h"
#include <string>
#include <time.h>
#include <map>

namespace sonar
{

class AbstractClient
{
public:
	typedef unsigned long PROTOID;
	const static int PROTO_MAX_ARGS=128;
	const static int PROTO_BUFFER_LENGTH=(65536 * 2);
	const static int PROTO_COMMAND_LENGTH=65536;
	const static int PROTO_KEEPALIVE_INTERVAL_SEC=5;

	typedef std::pair<AbstractClient *, void *> CommandHandler;

	AbstractClient (time_t keepaliveTimeout);
	virtual ~AbstractClient (void);

	virtual bool update (void) = 0;

protected:
	int sendFormattedToQueue (const char *_format, ...);
	int processData (void *_pData, int _cbLength);
	void sendErrorReply (PROTOID _id, const char *_pstrError);
	void sendSuccessReply (PROTOID _id);
	int sendQueue (void);
	PROTOID getProtoId (void);
	void blockEvents (bool _bBlock);
	void stopQueue (bool _bStop);
	int processCommand (char **argv, int argc, PROTOID _id);
	int handleKeepAlive (void);
	void setCommandHandler (sonar::CString &_cmdName, CommandHandler _handler);

	bool isConnectionLost ();
	void setSocket (TcpSocket *_pSocket);
	TcpSocket *getSocket (void);
	bool processSocket (void);

	virtual bool callCommandHandler (void *_pCmd, char **argv, int argc, int _id) = 0;

private:
	std::map<sonar::CString, CommandHandler> m_handlerMap;
	ByteBuffer m_readBuffer;
	ByteBuffer m_writeBuffer;
	bool m_bConnectionLost;
	time_t m_tsNextKeepAlive;
	unsigned long m_tsReplyExpire;
	bool m_bStopQueue;
	TcpSocket *m_pSocket;
	PROTOID m_nCurrProtoId;
	SOCKET m_sockfd;

protected:
	bool m_bBlockEvents;

	time_t m_keepaliveTimeout;

};

}
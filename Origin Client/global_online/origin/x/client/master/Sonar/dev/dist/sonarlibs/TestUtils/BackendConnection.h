#pragma once
#include <SonarCommon/Common.h>
#include <jlib/Spinlock.h>
#include <SonarIpcCommon/SocketConnection.h>
#include <TestUtils/BackendEvent.h>

namespace sonar
{

class BackendEvent;

class BackendConnection : private SocketConnection
{
public:

	class UnexpectedEvent
	{

	};

	class UnknownCommand
	{

	};

	class EventNotValidated
	{

	};

	BackendConnection(CString &pubRsaBase64);
	~BackendConnection();

	void waitForConnection();
	CString &getGuid();

	void update(void);
	int getPort(void);

	void validateNextEvent(EventType type, BackendEvent **outEvent = NULL);

	typedef SonarDeque<BackendEvent *> EventList;
	EventList m_events;


	void setKeepaliveIgnore(bool state);

private:
	//SocketConnection
	virtual void onMessage(IpcMessage &msg);

	void handleRegister(IpcMessage &msg);
	void handleKeepalive(IpcMessage &msg);
	void handleClientRegisteredToChannel(IpcMessage &msg);
	void handleClientUnregistered(IpcMessage &msg);
	void handleChannelDestroyed(IpcMessage &msg);

	jlib::Spinlock m_lock;

	jlib::SOCKET m_acceptfd;
	int m_port;

	bool m_isRegistered;
	String m_pubRsaBase64;
	String m_guid;

	bool m_firstValidation;
	bool m_ignoreKeepalives;


};

}
#pragma once
#include "AbstractClient.h"

#include <time.h>

namespace sonar
{

class AbstractOutboundClient : public AbstractClient
{
public:
	enum CLIENTSTATE
	{
		CS_NONE = 0,
		CS_CONNECTION_PENDING,
		CS_CONNECTED,
		CS_REGISTER_PENDING,
		CS_REGISTERED,
		CS_DISCONNECTED,
		CS_NUM_STATES
	};

public:
	AbstractOutboundClient (
		CString &_controlAddress, 
		time_t _keepaliveTimeout, 
		time_t _connectTimeout, 
		time_t _registerTimeout, 
		time_t _disconnectTimeout);

	bool update (void);
	int setState (CLIENTSTATE _state);
	CLIENTSTATE getState (void);
	const char *getAddress (void) const { return m_controlAddress.c_str(); }

private:
	const char *getStateName(CLIENTSTATE state);

	bool reply (char **argv,  int argc, int _id );
	bool keepalive (char **argv, int argc, int _id );
	bool unregister (char **argv, int argc, int _id );

	bool callCommandHandler (void *_pCmd, char **argv, int argc, int _id);

	static bool staticReply (AbstractOutboundClient *pInst, char **argv, int argc, int _id);
	static bool staticKeepalive (AbstractOutboundClient *pInst, char **argv, int argc, int _id);
	static bool staticUnregister (AbstractOutboundClient *pInst, char **argv, int argc, int _id);


protected:
	virtual void sendRegister (void) = 0;
	virtual void sendState (void) = 0;
	virtual void onReply (char **argv, int argc, int _id) = 0;
	virtual void onStateChange (CLIENTSTATE _oldState, CLIENTSTATE _newState);

	virtual bool onRegisterSuccess (char **argv, int argc) = 0;
	virtual time_t onRegisterError (char **argv, int argc) = 0;
	virtual time_t onConnectionLost (const char *_pstrReason) = 0;

	void setDisconnectTimeout (time_t _timeout);

private:
	CLIENTSTATE m_state;

	String m_controlAddress;

	time_t m_tsConnectExpire;
	time_t m_tsRegisterExpire;
	time_t m_tsDisconnectExpire;

	time_t m_connectTimeout;
	time_t m_registerTimeout;
	time_t m_disconnectTimeout;
};

}
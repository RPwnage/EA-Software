#pragma once

#include "AbstractOutboundClient.h"
#include "UUIDManager.h"

#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/asn1.h>
#include <openssl/sha.h>

#include "ClientConnection.h"
#include "Channel.h"

namespace sonar
{

class BackendClient : public AbstractOutboundClient
{
public:
	BackendClient (
		const std::string &_hostname,
		const std::string &_controlAddress,  
		const std::string &_location,
		time_t _connectTimeout, 
		time_t _registerTimeout, 
		time_t _disconnectTimeout,
		int _maxClients,
		int _voipPort);

	~BackendClient(void);

	RSA *getRSAPubKey(void);

	static bool rsaValidateToken (RSA *rsaPubKey, CString &message, const char *signature, size_t cbSignature);
	static bool decodeBase64 (CString &input, char *buffer, size_t *cbBuffer);

	void eventClientRegisteredToChannel (const char *_pstrOperatorId, const char *_pstrUserId, const char *_pstrUsername, const char *_pstrChannelId, const char *_pstrChannelDesc, const char *_pstrSourceIP, int _nPort, int _nClientCount);
	void eventChannelDestroyed (const char *_pstrOperatorId, const char *_pstrChannelId);
	void eventClientUnregistered(const char *_pstrOperatorId, const char *_pstrUserId, const char *_pstrChannelId, int _nClientCount);
	void eventVoiceServerMetrics_State (const char *_pstrMetrics);
	void eventVoiceServerMetrics_Counter (const char *_pstrMetrics);
	void eventVoiceServerMetrics_System  (const char *_pstrMetrics);
	void eventVoiceServerMetrics_Health  (const char *_pstrMetrics);
	void eventVoiceServerMetrics_Network  (const char *_pstrMetrics);

	bool updateRSAPubKey (const void *pubKey, size_t length);

protected:
	void sendRegister (void);
	void sendState (void);
	void onReply (char **argv, int argc, int _id);
	bool onCommand (void *_pCmd, char **argv, int argc, int _id);

	bool channelInfo (char **argv, int argc, int _id);    
	bool clientRegisterOk (char **argv, int argc, int _id);
	bool channelDestroy (char **argv, int argc, int _id);
	bool clientUnregister (char **argv, int argc, int _id);

private:
	virtual void eventClientState (const char *_pstrOperatorId, Channel &channel);

	static bool staticChannelDestroy (BackendClient *_pInstance, char **argv, int argc, int _id);
	static bool staticClientUnregister (BackendClient *_pInstance,char **argv, int argc, int _id);
	static bool staticChannelInfo (BackendClient *_pInstance,char **argv, int argc, int _id);
	static bool staticClientRegisterOk (BackendClient *_pInstance,char **argv, int argc, int _id);

	bool onRegisterSuccess (char **argv, int argc);
	time_t onRegisterError (char **argv, int argc);
	time_t onConnectionLost (const char *_pstrReason);
	
	bool updateBackendEventInterval(sonar::CString &backendEventInterval);

private:
	int m_maxClients;
	int m_voipPort;
	std::string m_hostname;
	std::string m_location;

	UUIDManager m_uuid;
	RSA *m_rsaPubKey;
};

}
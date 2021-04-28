#pragma once
#include <SonarConnection/Connection.h>
#include <stdarg.h>
namespace sonar
{

class DefaultTimeProvider : public sonar::ITimeProvider
{
public:
	virtual common::Timestamp getMSECTime() const;
};

class Udp4Transport : public sonar::AbstractTransport
{
public:
	Udp4Transport(sonar::CString &address);
	~Udp4Transport(void);

    virtual void close();
    virtual bool isClosed() const;

	virtual bool rxMessagePoll(void *buffer, size_t *cbBuffer, ssize_t& socketResult);
	virtual bool rxMessageWait(void);
	virtual bool txMessageSend(const void *buffer, size_t cbBuffer, ssize_t& socketResult);
    virtual sockaddr_in getLocalAddress() const;
    virtual sockaddr_in getRemoteAddress() const;

private:
	struct Private;
	struct Private *m_prv;
    sockaddr_in m_addr_local;

public:
	static int m_txPacketLoss;
	static int m_rxPacketLoss;

};


class StderrLogger : public sonar::IDebugLogger
{
public:
	virtual void printf(const char *format, ...) const;
};

class Udp4NetworkProvider : public sonar::INetworkProvider
{
public:
	virtual sonar::AbstractTransport *connect(sonar::CString &address) const;
private:
};

}
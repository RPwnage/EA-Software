#pragma once

#include <eastl/string.h>
#include <eastl/queue.h>

#include <eathread/eathread_thread.h>
#include <eathread/eathread_mutex.h>
#include <EATrace/EALog_imp.h>

class ProtoHttpRefT;
class SocketT;

namespace EA
{
namespace GameApp
{

class Loggly : public EA::Thread::IRunnable, public EA::Trace::LogReporter
{
	// Disallow copy construction/assignment
	Loggly(const Loggly&);
	Loggly& operator=(const Loggly&);

public:
	enum LogLevel
	{
		kLevelNetlog = EA::Trace::kLevelWarn - 20
	};

	enum SeparateLogs
	{
		kBundledLogs,		// Default, chunk logs together before sending them
		kSeparateLogs		// Separate each individual log line into its own log
	};

private:
	ProtoHttpRefT* mHttp;									// The HTTP connection, if used
	SocketT* mSock;											// The TCP socket, if used

	EA::Thread::Thread* mThread;							// Thread processing asynchronously the events

	EA::Thread::Mutex mBufMutex;							// String buffer's mutex
	volatile eastl::queue<eastl::string>* mBuf;				// String buffer

	const eastl::string mUrl;								// HTTP URL or TCP server name
	const uint16_t mPort;									// HTTP (0) or TCP port

	const SeparateLogs mSeparate;							// Individual packets/connections per message (evil!)

public:
	static const EA::Trace::InterfaceId kIID = (EA::Trace::InterfaceId)0x1469691e;

	Loggly(eastl::string httpUrl, SeparateLogs separate = kBundledLogs);							// Initialize as HTTP connection
	Loggly(eastl::string tcpServer, uint16_t tcpPort, SeparateLogs separate = kBundledLogs);		// Initialize as TCP connection
	// (Note: tcp-based separated logs is useless in Loggly's case... but it could be used elsewhere)

	~Loggly();

	virtual intptr_t Run(void* pContext = NULL);									// Thread initializing and processing messages

	virtual bool IsFiltered(const EA::Trace::TraceHelper& helper);
	virtual bool IsFiltered(const EA::Trace::LogRecord& record);

	virtual EA::Trace::tAlertResult Report(const EA::Trace::LogRecord& record);

	void* AsInterface(EA::Trace::InterfaceId iid)
	{
		if (iid == Loggly::kIID) 
			return this;
		return Loggly::AsInterface(iid);
	}
};

}	// Namespace
}	// Namespace
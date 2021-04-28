#include "Loggly.h"

#include <eathread/eathread_thread.h>
#include <EATrace/EATrace.h>
#include <EATrace/EATrace_imp.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EADateTime.h>

#include <dirtysock.h>
#include <protohttp.h>
#include <protoname.h>
#include <netconn.h>

// For UDIDs
#include "GameApplication.h"
#include <Blast/IDevice.h>

namespace EA
{
namespace GameApp
{

enum
{
	kMaxPacketSize = 16384,
	kBufferSize = kMaxPacketSize + 1024,
	kUpdateEveryMs = 5000					// Send new data every few seconds only
};

Loggly::Loggly(eastl::string httpUrl, SeparateLogs separate) :
	LogReporter("LogglyHttp"),
	mHttp(NULL),
	mSock(NULL),
	mThread(new EA::Thread::Thread),
	mBuf(new eastl::queue<eastl::string>),
	mUrl(httpUrl),
	mPort(0),		// Signal to say it's a HTTP connection
	mSeparate(separate)
{
	mThread->Begin(this);
}

Loggly::Loggly(eastl::string tcpServer, uint16_t tcpPort, SeparateLogs separate) :
	LogReporter("LogglyTcp"),
	mHttp(NULL),
	mSock(NULL),
	mThread(new EA::Thread::Thread),
	mBuf(new eastl::queue<eastl::string>),
	mUrl(tcpServer),
	mPort(tcpPort),
	mSeparate(separate)
{
	mThread->Begin(this);
}

Loggly::~Loggly()
{
	if (mThread)
	{
		mThread->WaitForEnd();
		delete mThread;
	}

	if (NetConnStatus('open', 0, NULL, 0) > 0)
	{
		if (mSock)
		{
			// Note: leaks if network is unavailable and socket is still open (read: always)
			SocketClose(mSock);
		}

		if (mPort)
		{
			ProtoHttpDestroy(mHttp);
		}
	}
}

intptr_t Loggly::Run(void* pContext)
{
	bool success = false;

	// Make sure we are started
	bool active = false;
	for (int i = 30; !active && i >= 0; --i)		// 30 seconds
	{
		EA::Thread::ThreadSleep(1000);
		active = NetConnStatus('open', 0, NULL, 0) > 0;
	}
	if (!active)
	{
		goto EndInit;
	}

	if (mPort)		// TCP
	{
		// Get server IP from DNS
		uint32_t n = ProtoNameSync(mUrl.c_str(), 10000);
		if (!n)
		{
			goto EndInit;
		}

		// Create socket
		struct sockaddr peeraddr;
		SockaddrInit(&peeraddr, AF_INET);
		SockaddrInSetAddr(&peeraddr, n);
		SockaddrInSetPort(&peeraddr, mPort);

		if (NetConnStatus('open', 0, NULL, 0) <= 0)
			goto EndInit;

		mSock = SocketOpen(AF_INET, SOCK_STREAM, 0);		// Might leak (see dtor)
		if (mSock == NULL)
		{
			goto EndInit;
		}

		if (NetConnStatus('open', 0, NULL, 0) <= 0)
			goto EndInit;

		// Start the connection
		SocketConnect(mSock, (struct sockaddr *)&peeraddr, sizeof(peeraddr));

		bool connected = false;
		for (int i = 200; !connected && i >= 0; --i)		// 20 seconds
		{
			EA::Thread::ThreadSleep(100);
			if (NetConnStatus('open', 0, NULL, 0) <= 0)
				goto EndInit;

			connected = SocketInfo(mSock, 'stat', 0, NULL, 0) > 0;
		}

		if (!connected)
		{
			goto EndInit;
		}
	}
	else		// HTTP
	{
		mHttp = ProtoHttpCreate(kBufferSize);
		if (!mHttp)
		{
			goto EndInit;
		}

		ProtoHttpControl(mHttp, 'keep', 1, 0, NULL);		// Keep-alive
		ProtoHttpControl(mHttp, 'ncrt', 1, 0, NULL);		// No certificate validation
	}

	success = true;

EndInit:
	if (!success)
	{
		mBufMutex.Lock();
		delete mBuf; mBuf = NULL;
		mBufMutex.Unlock();
		return -1;
	}

	while (!EA::GameApp::GameApp::HasGameApp())
	{
		EA::Thread::ThreadSleep(1000);			// One more second ...
	}
	EA::Blast::IDevice* iDevice = EA::GameApp::GameApp::Get()->GetDevice();
	char header[20+1];		// Will get initialized later on ... chars max (and usually exactly) for header...
	int headerSize = 0;

	// As long as we have a network stack
	while (success && NetConnStatus('open', 0, NULL, 0) > 0)
	{
		mBufMutex.Lock();
		{
			// As long as there's stuff to be done ...
			while (success && !const_cast<eastl::queue<eastl::string>* >(mBuf)->empty())
			{
				if (mSeparate == kBundledLogs)
				{
					// Wait some time to gather up more events
					mBufMutex.Unlock();
					EA::Thread::ThreadSleep(kUpdateEveryMs >> 1);
					mBufMutex.Lock();
				}

				eastl::queue<eastl::string>& buf(*const_cast<eastl::queue<eastl::string>* >(mBuf));	// Once locked = available

				char out[kMaxPacketSize];
				int curSize = 0;

				// Add header
				if (!headerSize)
				{
					const char* uid = iDevice->GetUniqueId();
					const char* uid8 = uid;
					int len = EA::StdC::Strlen(uid);
					if (len > 8) uid8 += len - 8;

					uint64_t startTime = EA::StdC::GetTime()/1000000;		// Get it in MS... Some devices hate nanoseconds and we want uniqueness
					uint32_t startTime32 = (uint32_t)startTime;

					headerSize = EA::StdC::Sprintf(header, "[%s:%08.8x] ", uid8, startTime32);		// Warning: count chars max! (see definition)
					curSize = EA::StdC::Sprintf(out,
						"%s*** New Log: UID=[%s] StartTime(UniqueLogID)=[%llX] - Name=[%s] Manufacturer=[%s] Model=[%s] "
						"Platform=[%s] Version=[%s] Language=[%s] Locale=[%s]\n",
						header, uid, startTime, iDevice->GetName(), iDevice->GetManufacturer(), iDevice->GetModel(),
						iDevice->GetPlatformName(), iDevice->GetPlatformVersion(), iDevice->GetLanguage(), iDevice->GetLocale());
				}

				// As long as we can fit data in our packet ...
				if (!curSize || mSeparate != kSeparateLogs) while (!buf.empty())
				{
					eastl::string& front(buf.front());
					if (front.size() + curSize + headerSize > kMaxPacketSize)
					{
						if (front.size() + headerSize > kMaxPacketSize)
						{
							buf.pop();			// Message too long!
						}
						break;
					}

					EA::StdC::Strcpy(out+curSize, header); curSize += headerSize;
					EA::StdC::Strcpy(out+curSize, front.c_str()); curSize += front.size();
					buf.pop();

					if (mSeparate == kSeparateLogs)
						break;			// ... or only one
				}

				if (curSize == 0)
					continue;

				mBufMutex.Unlock();

				// Post that sh*t
				if (mSock)
				{
					SocketSend(mSock, out, curSize, 0);
				}

				if (mHttp)
				{
					ProtoHttpPost(mHttp, mUrl.c_str(), out, curSize, false);

					// Wait for result
					int32_t result;
					do
					{
						result = ProtoHttpStatus(mHttp, 'done', NULL, 0);

						if (result < 0)
						{	// Any problem, we don't even bother anymore! No spamming!
							success = false;
							break;
						}

						ProtoHttpUpdate(mHttp);
						EA::Thread::ThreadSleep(50);

						if (NetConnStatus('open', 0, NULL, 0) <= 0)
						{
							success = false;
							break;
						}

						ProtoHttpRecv(mHttp, out, 0, kMaxPacketSize);			// We so don't care about result :)
					}
					while (result == 0);
				}

				mBufMutex.Lock();
			}
		}
		mBufMutex.Unlock();

		if (mHttp && NetConnStatus('open', 0, NULL, 0) > 0)
			ProtoHttpUpdate(mHttp);

		EA::Thread::ThreadSleep(kUpdateEveryMs);
	}

	mBufMutex.Lock();
	delete mBuf; mBuf = NULL;
	mBufMutex.Unlock();
	return 0;
}


bool Loggly::IsFiltered(const EA::Trace::TraceHelper& helper)
{
	return 
		((helper.GetOutputType() & EA::Trace::kOutputTypeText) == 0) ||
		EA::Trace::LogReporter::IsFiltered(helper);
}

bool Loggly::IsFiltered(const EA::Trace::LogRecord& record)
{
	return 
		((record.GetTraceHelper()->GetOutputType() & EA::Trace::kOutputTypeText) == 0) ||
		EA::Trace::LogReporter::IsFiltered(record);
}


EA::Trace::tAlertResult Loggly::Report(const EA::Trace::LogRecord& record)
{
	const char* const pOutput = mFormatter->FormatRecord(record);

	bool locked = mBuf != NULL;
	if (locked)
		mBufMutex.Lock();

	if (mBuf)
	{
		eastl::queue<eastl::string>& buf(*const_cast<eastl::queue<eastl::string>* >(mBuf));	// Once locked = available
		buf.push(eastl::string(pOutput));
	}

	if (locked)
		mBufMutex.Unlock();

	return EA::Trace::kAlertResultNone;
}

}	// Namespace
}	// Namespace
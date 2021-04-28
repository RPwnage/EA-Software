#ifndef __ORIGIN_LSX_SUPPORT_H__
#define __ORIGIN_LSX_SUPPORT_H__

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <OriginSDK/OriginTypes.h>
#include "OriginSDKMemory.h"
#include "MessageBuffer.h"

#ifdef ORIGIN_PC
#include <winsock.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR SO_ERROR
#endif

// Ensure that this and TCPServer TCP_SERVER_MAX_MESSAGE_LENGTH are big enough for all messages.
#define MAX_LSX_MESSAGE_SIZE 128*1024

namespace Origin
{

class LSX
{
public:
	LSX();
	virtual ~LSX();

	bool Connect(uint16_t port);

	bool HasReadDataToRead() const;
	bool Read(AllocString &reply);
	bool Write(AllocString request);

	bool Disconnect();
	bool IsConnected() const;

	bool IsThreadRunning();

    void SetUseEncrypted(bool useEncrypted);
    void SetSeed(int seed);

	std::thread & theThread() { return m_thread; }
	std::mutex & theMessageLock() { return m_messageLock; }
	std::condition_variable & theCondition() { return m_condition; }

	LSX(const LSX &) = delete;
	LSX(LSX &&) = delete;
	LSX & operator = (const LSX &) = delete;
	LSX & operator = (LSX &&) = delete;

private:
	void Run();

	std::thread m_thread;
	std::mutex m_lock;
	std::mutex m_messageLock;
	std::condition_variable m_condition;

    SOCKET m_socket = INVALID_SOCKET;
    
    // Authentication with Download Manager is in plain text.
	// This flag is set true after successful authentication when all communication uses encryption.
	bool m_useEncrypted = false;
    int m_seed = 0;

	MessageBuffer m_buffer;
};

}; // namespace Origin

#endif //__ORIGIN_LSX_SUPPORT_H__

#pragma 

#include <jlib/Time.h>
#include <jlib/socketdefs.h>
#include <jlib/Thread.h>
#include <jlib/String.h>
#include <time.h>
#include <SonarConnection/Protocol.h>
#include <SonarConnection/Connection.h>
#include <SonarVoiceServer/Server.h>
#include <TestUtils/TokenSigner.h>
#include <TestUtils/BackendConnection.h>
#include <sstream>
#include <assert.h>

#pragma optimize("", off )
using namespace sonar;

static TokenSigner *g_tokenSigner = NULL;

static void WideAssert(const wchar_t *expr, const wchar_t *file, int line)
{
    char message[1024 + 1];
#ifndef snprintf
#define snprintf _snprintf
#endif
    snprintf (message, 1024, "TEST FAILED at %s:%d> %s\n", file, line, expr);
    fprintf (stderr, "%s", message);
    fflush(stderr);
    exit(-1);
}

#define ASSERT(_Expression) (void)( (!!(_Expression)) || (WideAssert(_CRT_WIDE(#_Expression), _CRT_WIDE(__FILE__), __LINE__), 0) )

static sonar::CString TEST_CHANNEL_DESCRIPTION = "TestChannelDesc";
static sonar::CString TEST_CHANNEL_ID_PREFIX = "testchannel";

static sonar::CString TEST_REASON_TYPE = "MY_OWN_REASON_TYPE";
static sonar::CString TEST_REASON_DESC = "MY_OWN_REASON_DESC";

static int g_serverPort = 22990;

class Event
{
public:
    Event(CString &_name) 
        : name(_name)
    {
    }

    virtual ~Event() { }

    String name;
};

class ConnectEvent : public Event
{
public:
    ConnectEvent(CString &_channelId, CString &_channelDesc) 
        : Event("ConnectEvent")
        , channelId(_channelId)
        , channelDesc(_channelDesc)
    {}

    String channelId;
    String channelDesc;
};

class DisconnectEvent : public Event
{
public:
    DisconnectEvent(sonar::CString &reasonType, sonar::CString &reasonDesc) 
        : Event("DisconnectEvent")
        , reasonType(reasonType)
        , reasonDesc(reasonDesc)
    {}
    CString reasonType;
    CString reasonDesc;
};

class ClientJoinedEvent : public Event
{
public:
    ClientJoinedEvent(int chid, CString &userId, CString &userDesc) 
        : Event("ClientJoinedEvent")
        , chid(chid)
        , userId(userId)
        , userDesc(userDesc)
    {}
    int chid;
    CString userId;
    CString userDesc;
};

class ClientPartedEvent : public Event
{
public:
    ClientPartedEvent(int chid, CString &userId, CString &userDesc, sonar::CString &reasonType, sonar::CString &reasonDesc) 
        : Event("ClientPartedEvent")
        , chid(chid)
        , userId(userId)
        , userDesc(userDesc)
        , reasonType(reasonType)
        , reasonDesc(reasonDesc)
    {}
    int chid;
    CString userId;
    CString userDesc;
    CString reasonType;
    CString reasonDesc;
};

class EchoMessageEvent : public Event
{
public:
    EchoMessageEvent(const sonar::Message &message) 
        : Event("EchoMessageEvent")
        , message(message)
    {}

    sonar::Message message;
};

class ChallengeEvent : public Event
{
public:
    ChallengeEvent(unsigned long challenge) 
        : Event("ChallengeEvent")
        , challenge(challenge)
    {}

    unsigned long challenge;
};

class TakeBeginEvent : public Event
{
public:
    TakeBeginEvent() 
        : Event("TakeBeginEvent")
    {}
};

class FrameBeginEvent : public Event
{
public:
    FrameBeginEvent() 
        : Event("FrameBeginEvent")
    {}
};

class FrameEndEvent : public Event
{
public:
    FrameEndEvent() 
        : Event("FrameEndEvent")
    {}
};

class TakeEndEvent : public Event
{
public:
    TakeEndEvent() 
        : Event("TakeEndEvent")
    {}
};

class ClientPayloadEvent : public Event
{
public:
    ClientPayloadEvent(int chid, const void *payload, size_t cbPayload) 
        : Event("ClientPayloadEvent")
    {
        buffer.resize(cbPayload);
        if (cbPayload > 0)
        {
            memcpy(&buffer[0], payload, cbPayload);
        }

        this->chid = chid;
        this->subChannelMask = subChannelMask;
    }

    sonar::Buffer buffer;
    int chid;
    unsigned long subChannelMask;
};

class ChannelInactivityEvent : public Event
{
public:
    ChannelInactivityEvent(unsigned long interval)
        : Event("ChannelInactivityEvent")
    {
        this->interval = interval;
    }

    unsigned long interval;
};

class TestTransport;

class TestConnection : public sonar::Connection, sonar::IConnectionEvents
{
public:
    static int m_instances;

public:
    TestConnection(const sonar::ITimeProvider &time, const sonar::INetworkProvider &network, const sonar::IDebugLogger &logger, CString &token) :
            sonar::Connection(time, network, logger, ConnectionOptions()), 
            m_token(token)
    {
        setEventSink(this);
        m_instances ++;
    }

    ~TestConnection(void)
    {
        clearEvents();
        m_instances --;
    }

    void setupTransport (sonar::CString &tokenString)
    {
        sonar::Token token;
        token.parse(tokenString);

        m_transport = m_network->connect(token.getVoipAddress());
        dbgForceState(Connected);
    }

    bool connect(void)
    {
        return sonar::Connection::connect(m_token) == sonar::Connection::Success;
    }

    int getChanSize(void)
    {
        return m_chanSize;
    }

    virtual void onConnect(CString &channelId, CString &channelDesc) 
    {
        //fwprintf (stderr, "%s: Connected to %s (%s)\n", getToken().getUserId().c_str (), channelId.c_str (), channelDesc.c_str ());
        events.push_back(new ConnectEvent(channelId, channelDesc));
    }

    virtual void onDisconnect(CString &reasonType, CString &reasonDesc) 
    {
        //fwprintf (stderr, "%s: Disonnect %s (%s)\n", getToken().getUserId().c_str (), reasonType.c_str (), reasonDesc.c_str ());
        events.push_back(new DisconnectEvent(reasonType, reasonDesc));
    }


    virtual void onClientJoined(int chid, CString &userId, CString &userDesc) 
    {
        //fwprintf (stderr, "%s: Client %s joined the channel\n", getToken().getUserId().c_str (), userId.c_str ());
        events.push_back(new ClientJoinedEvent(chid, userId, userDesc));
    }
    virtual void onClientParted(int chid, CString &userId, CString &userDesc, CString &reasonType, CString &reasonDesc) 
    {
        //fwprintf (stderr, "%s: Client %s parted the channel (%d)\n", getToken().getUserId().c_str (), userId.c_str (), (int) rt);
        events.push_back(new ClientPartedEvent(chid, userId, userDesc, reasonType, reasonDesc));
    }

    virtual void onTakeBegin() 
    {
        events.push_back(new TakeBeginEvent());
    }

    virtual void onFrameBegin(long long timestamp) 
    {
        events.push_back(new FrameBeginEvent());
    }

    virtual void onFrameClientPayload(int chid, const void *payload, size_t cbPayload) 
    {
        events.push_back(new ClientPayloadEvent(chid, payload, cbPayload));
    }

    virtual void onFrameEnd(long long timestamp = 0) 
    {
        events.push_back(new FrameEndEvent());
    }

    void onTakeEnd()
    {
        events.push_back(new TakeEndEvent());
    }

    virtual void onEchoMessage(const sonar::Message &message)
    {
        events.push_back(new EchoMessageEvent(message));
    }

    virtual void onChannelInactivity(unsigned long interval)
    {
        events.push_back(new ChannelInactivityEvent(interval));
    }

    TestTransport &getTransport()
    {
        return *((TestTransport *) m_transport);
    }

    void clearEvents()
    {
        for each (Event *evt in events)
        {
            delete evt;
        }

        events.clear();
    }
    
    void overrideCID(int cid)
    {
        setCID(cid);
    }

    virtual void handleMessage(Message &message)
    {
        if (message.getHeader().cmd != protocol::MSG_CHALLENGE)
        {
            Connection::handleMessage(message);
        }

        if (message.getBytesLeft() < 4)
        {
            return;
        }

        events.push_back(new ChallengeEvent(message.readUnsafeLong()));
    }

private:
    String m_token;


public:
    typedef SonarVector<Event *> EventList;
    EventList events;

};

int TestConnection::m_instances = 0;

class TestTimeProvider : public sonar::ITimeProvider
{
    virtual common::Timestamp getMSECTime() const
    {
        return jlib::Time::getTimeAsMSEC();
    }
};

class TestTransport : public sonar::AbstractTransport
{
public:
    TestTransport(sonar::CString &address)
    {
        m_loseNextTxPacket = 0;
        m_loseNextRxPacket = 0;
        m_reorderNextTxPacket = false;
        m_rxLossRate = 0;
        m_txLossRate = 0;
        m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        int size = (1024 * 1024);

        if (setsockopt (m_sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &size, sizeof(size)) == -1)
        {
            ASSERT(false);
        }

        if (setsockopt (m_sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &size, sizeof(size)) == -1)
        {
            ASSERT(false);
        }

        sonar::String host = address.substr(0, address.find(L':'));
        sonar::String service = address.substr(address.find(L':') + 1);
        
        ADDRINFO hints;
        ADDRINFO *pResults;

        memset (&hints, 0, sizeof (hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;
        
        getaddrinfo(host.c_str(), service.c_str(), &hints, &pResults);

        memcpy (&m_remoteAddr, pResults->ai_addr, sizeof (sockaddr_in));
        //m_remoteAddr.sin_port = htons(port),

        freeaddrinfo (pResults);

        struct sockaddr_in bindAddr;
        memcpy (&bindAddr, &m_remoteAddr, sizeof(bindAddr));

        bindAddr.sin_addr.s_addr = INADDR_ANY;
        bindAddr.sin_port = htons(0);

        if (bind(m_sockfd, (sockaddr *) &bindAddr, sizeof(bindAddr)) == -1)
        {
            ASSERT(false);
        }

        if (connect(m_sockfd, (sockaddr *) &m_remoteAddr, sizeof(m_remoteAddr)) == -1)
        {
            ASSERT(false);
        }
        


        jlib::SocketSetNonBlock(m_sockfd, true);
    }

    ~TestTransport(void)
    {
        close();
    }

    virtual void close()
    {
        if (!isClosed())
        {
            jlib::SocketClose(m_sockfd);
            m_sockfd = -1;
        }
    }

    virtual bool isClosed() const
    {
        return m_sockfd == -1;
    }

    virtual bool rxMessagePoll(void *buffer, size_t *cbBuffer, ssize_t& socketResult)
    {
        struct sockaddr_in addr;
        int len = sizeof(addr); 
        socketResult = recvfrom(m_sockfd, (char *) buffer, *cbBuffer, jlib::MSG_NOSIGNAL, (struct sockaddr *) &addr, &len);

        switch(socketResult)
        {
        case -1:
            if (!jlib::SocketWouldBlock(m_sockfd))
            {
                return false;
            }

            *cbBuffer = 0;
            return true;

        case 0:
            return false;

        default:
            if (m_loseNextRxPacket > 0 || packetLoss(m_rxLossRate))
            {
                m_loseNextRxPacket --;
                *cbBuffer = 0;
                return true;
            }

            *cbBuffer = (size_t) socketResult;
            return true;
        }
    }
    virtual bool rxMessageWait(void)
    {
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_sockfd, &readSet);
        int result = select(m_sockfd + 1, &readSet, NULL, NULL, &tv);
        return (result == 1);
    }

    virtual bool txMessageSend(const void *buffer, size_t cbBuffer, ssize_t& socketResult)
    {
        if (m_loseNextTxPacket > 0 || packetLoss(m_txLossRate))
        {
            m_loseNextTxPacket --;
            return true;
        }

        if (m_reorderNextTxPacket)
        {
            m_buffer.resize(cbBuffer);
            memcpy(&m_buffer[0], buffer, cbBuffer);
            m_reorderNextTxPacket = false;
            return true;
        }

        socketResult = sendto(m_sockfd, (char *) buffer, cbBuffer, jlib::MSG_NOSIGNAL, (sockaddr *) &m_remoteAddr, sizeof(m_remoteAddr));

        ASSERT (socketResult == (int) cbBuffer);

        if (!m_buffer.empty())
        {
            socketResult = sendto(m_sockfd, (char *) &m_buffer[0], m_buffer.size(), jlib::MSG_NOSIGNAL, (sockaddr *) &m_remoteAddr, sizeof(m_remoteAddr));
            m_buffer.clear();
        }
        return (socketResult == (int) cbBuffer);
    }

    static bool packetLoss(int rate)
    {
        return ((rand () % 100) + 1) < rate;
    }


    void loseNextTxPacket(void)
    {
        m_loseNextTxPacket = 1;
    }

    void loseNextTxPackets(int count)
    {
        m_loseNextTxPacket = count;
    }

    void loseNextRxPacket(void)
    {
        m_loseNextRxPacket = 1;
    }

    void loseNextRxPackets(int count)
    {
        m_loseNextRxPacket = count;
    }

    void setTxPacketLoss(int rate)
    {
        m_txLossRate = rate;
    }

    void setRxPacketLoss(int rate)
    {
        m_rxLossRate = rate;
    }


    void reorderNextTxPacket()
    {
        m_reorderNextTxPacket = true;
    }


    private:
    jlib::SOCKET m_sockfd;
    
    struct sockaddr_in m_remoteAddr;
    int m_loseNextTxPacket;
    int m_loseNextRxPacket;
    bool m_reorderNextTxPacket;

    int m_rxLossRate;
    int m_txLossRate;

    sonar::Buffer m_buffer;

};


class TestNetworkProvider : public sonar::INetworkProvider
{
public:
    virtual sonar::AbstractTransport *connect(sonar::CString &address) const
    {
        TestTransport *ret = new TestTransport(address);

        if (m_loseNextRxPacket)
        {
            ret->loseNextRxPacket();
            m_loseNextRxPacket = 0;
        }

        if (m_loseNextTxPacket)
        {
            ret->loseNextTxPacket();
            m_loseNextTxPacket  = 0;
        }

        return ret;
    }

    void configureNextTransport(bool loseNextRxPacket, bool loseNextTxPacket)
    {
        m_loseNextRxPacket = loseNextRxPacket;
        m_loseNextTxPacket = loseNextTxPacket;
    }

private:

    static bool m_loseNextRxPacket;
    static bool m_loseNextTxPacket;


};

bool TestNetworkProvider::m_loseNextRxPacket = false;
bool TestNetworkProvider::m_loseNextTxPacket = false;

class TestDebugLogger : public sonar::IDebugLogger
{
public:

    static bool m_output;

    virtual void printf(const char *format, ...) const
    {
        va_list args;
        va_start (args, format);
        if (m_output)
        {
            vfprintf(stderr, format, args);
        }
        va_end(args);
    }
};

bool TestDebugLogger::m_output = true;

static sonar::String MakeToken(int id, sonar::CString &channelPrefix = TEST_CHANNEL_ID_PREFIX, time_t expires = -1)
{
    sonar::StringStream ss;

    if (expires == -1)
    {
        expires = time(0) + 86400;
    }

    int cid = (int) jlib::Thread::getCurrentId();

    ss << "SONAR2|127.0.0.1:" << g_serverPort << "|127.0.0.1:" << g_serverPort << "|default|user";
    ss << id;
    ss << "|User";
    ss << id;
    ss << "|";
    ss << channelPrefix;
    ss << cid;
    ss << "|";
    ss << TEST_CHANNEL_DESCRIPTION;
    ss << "||";
    ss << expires;
    ss << "|";

    Token token;
    token.parse(ss.str());
    return g_tokenSigner->sign(token);
}

TestTimeProvider g_timeProvider;
TestNetworkProvider g_networkProvider;
TestDebugLogger g_logger;

typedef SonarVector<TestConnection *> ConnectionList;

static ConnectionList GetConnections(int count, int idStart = 0, sonar::CString &channelPrefix = TEST_CHANNEL_ID_PREFIX)
{
    ConnectionList ret;

    for (int index = 0; index < count; index++)
    {
        TestConnection *conn = new TestConnection(g_timeProvider, g_networkProvider, g_logger, MakeToken(index + idStart, channelPrefix));
        ret.push_back(conn);
    }

    return ret;
}

static void FreeConnections(ConnectionList &conns)
{
    for each (TestConnection *conn in conns)
    {
        delete conn;
    }

}

class Timeout
{
public:
    Timeout(long long length)
    {
        m_expires = jlib::Time::getTimeAsMSEC() + length;
    }

    bool expired()
    {
        return (jlib::Time::getTimeAsMSEC() > m_expires);
    }

private:
    long long m_expires;

};


static void DisconnectClients(SonarVector<TestConnection *> conns, int start, int end, sonar::CString &reasonType = TEST_REASON_TYPE, sonar::CString &reasonDesc = TEST_REASON_DESC)
{
    Timeout timeout(60000);

    for (int index = start; index < end; index ++)
    {
        ASSERT(conns[index]->disconnect(reasonType, reasonDesc) == sonar::Connection::Success);
    }

    while (!timeout.expired())
    {
        int count = 0;

        for (int index = start; index < end; index ++)
        {
            sonar::common::sleepExact(1);
            conns[index]->poll();

            if (!conns[index]->isDisconnected())
            {
                continue;
            }

            count ++;
        }

        if (count == (end - start))
        {
            break;
        }

    }

    ASSERT (!timeout.expired());
}

static void ConnectClients(SonarVector<TestConnection *> conns, int start, int end)
{
    Timeout timeout(60000 * 100);

    ASSERT (start < end);

    for (int index = start; index < end; index ++)
    {
        ASSERT(conns[index]->connect());
    }

    while (!timeout.expired())
    {
        int count = 0;

        for (int index = start; index < end; index ++)
        {
            conns[index]->poll();
            sonar::common::sleepExact(1);

            if (!conns[index]->isConnected())
            {
                continue;
            }

            count ++;
        }

        if (count == (end - start))
        {
            break;
        }
    }

    ASSERT (!timeout.expired());
}

static void BeInChannel(SonarVector<TestConnection *> conns, int start, int end)
{
    Timeout t(30000);
    while (!t.expired())
    {
        int count = 0;

        for (int index = start; index < end; index ++)
        {
            conns[index]->poll();
            count += conns[index]->getClients().size();
        }

        if (count == (end - start) * (end - start))
        {
            break;
        }

        sonar::common::sleepExact(1);
    }

    ASSERT (!t.expired());

}

static void *ServerContainerProc(void *arg);

class ServerContainer
{
public:

    class KeyNotFoundException
    {
        //if this exception is thrown, run SonarMaster once to have it write the ~/sonarmaster.prv 
    };

    ServerContainer(Server::Config *pConfig = NULL)
    {
        Server::Config config;
        if (pConfig)
        {
            config = *pConfig;
        }

        sonar::String path;

#ifdef _WIN32
        path += getenv("HOMEDRIVE");
        path += getenv("HOMEPATH");
        path += "\\";
#else
        path += getenv("HOME");
        path += "\\";
#endif

        path += "sonarmaster.pub";

        char publicKey[4096];

        FILE *file = fopen(path.c_str(), "rb");

        if (file == NULL)
        {
            throw KeyNotFoundException();
        }

        size_t len = fread(publicKey, 1, sizeof(publicKey), file);
        fclose(file);

        if (len == 0)
        {
            throw KeyNotFoundException();
        }

        m_backend = new BackendConnection(TokenSigner::base64Encode(publicKey, len));

        StringStream backendAddr;
        backendAddr << "127.0.0.1:" << m_backend->getPort();

        m_server = new Server("", "", backendAddr.str(), "", 1000, 0, config);
        g_serverPort = ntohs(m_server->getVoipAddress().sin_port);


        m_isRunning = true;
        m_thread = jlib::Thread::createThread(ServerContainerProc, this);

        m_backend->waitForConnection();
        while (m_server->getBackendClient().getState() != AbstractOutboundClient::CS_REGISTERED)
        {
            sonar::common::sleepFrame();
        }

    }

    ~ServerContainer(void)
    {
        while (m_server->getClientCount() != 0)
        {
            sonar::common::sleepHalfFrame();
        }

        m_isRunning = false;
        m_thread.join();
        
        ASSERT (m_server->cleanup());
        delete m_server;
        delete m_backend;
    }

    void threadProc(void)
    {
        srand(m_thread.getId() + (unsigned int) time(0));
        while (m_isRunning)
        {
            m_server->poll();
            m_backend->update();
            sonar::common::sleepExact(1);
        }
  }

    Server &getServer()
    {
        return *m_server;
    }

    void suspend(void)
    {
        m_thread.suspend();
    }

    void resume(void)
    {
        m_thread.resume();
    }

    BackendConnection &backend(void)
    {
        return *m_backend;
    }


private:
    Server *m_server;
    BackendConnection *m_backend;
    bool m_isRunning;

    jlib::Thread m_thread;
};

static void *ServerContainerProc(void *arg)
{
    ServerContainer *sc = (ServerContainer *) arg;
    sc->threadProc();
    return NULL;
}
//=============================================================================
//= test_ConnectAndDisconnect
//=============================================================================
static void test_ConnectAndDisconnect()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(1);

    ConnectClients(conns, 0, 1);
    DisconnectClients(conns, 0, 1);

    FreeConnections(conns);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_BackendRegisterUnregisterDestroy
//=============================================================================
static void test_BackendRegisterUnregisterDestroy()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(1);
    ConnectionList conns2 = GetConnections(1, 1, "Someother");

    ConnectClients(conns2, 0, 1);
    ConnectClients(conns, 0, 1);
    DisconnectClients(conns, 0, 1);
    DisconnectClients(conns2, 0, 1);

    BackendEvent *evtRegister;
    BackendEvent *evtRegisterFirst;
    BackendEvent *evtUnregister;
    BackendEvent *evtUnregisterLast;
    BackendEvent *evtDestroy;

    sc.backend().validateNextEvent(CLIENT_REGISTERED, &evtRegisterFirst);
    sc.backend().validateNextEvent(CLIENT_REGISTERED, &evtRegister);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED, &evtUnregister);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED, &evtUnregisterLast);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED, &evtDestroy);

    ASSERT(evtRegisterFirst->msg.getArguments().size() == 9);
    ASSERT(evtRegisterFirst->msg.getString(0) == sc.backend().getGuid());
    ASSERT(evtRegisterFirst->msg.getString(1) == conns2[0]->getToken().getOperatorId());
    ASSERT(evtRegisterFirst->msg.getString(2) == conns2[0]->getToken().getUserId());
    ASSERT(evtRegisterFirst->msg.getString(3) == conns2[0]->getToken().getUserDesc());
    ASSERT(evtRegisterFirst->msg.getString(4) == conns2[0]->getToken().getChannelId());
    ASSERT(evtRegisterFirst->msg.getString(5) == conns2[0]->getToken().getChannelDesc());
    ASSERT(evtRegisterFirst->msg.getString(6) == "127.0.0.1");
    ASSERT(atoi(evtRegisterFirst->msg.getString(7).c_str()) > 0);
    ASSERT(atoi(evtRegisterFirst->msg.getString(8).c_str()) == 1);

    ASSERT(evtRegister->msg.getArguments().size() == 9);
    ASSERT(evtRegister->msg.getString(0) == sc.backend().getGuid());
    ASSERT(evtRegister->msg.getString(1) == conns[0]->getToken().getOperatorId());
    ASSERT(evtRegister->msg.getString(2) == conns[0]->getToken().getUserId());
    ASSERT(evtRegister->msg.getString(3) == conns[0]->getToken().getUserDesc());
    ASSERT(evtRegister->msg.getString(4) == conns[0]->getToken().getChannelId());
    ASSERT(evtRegister->msg.getString(5) == conns[0]->getToken().getChannelDesc());
    ASSERT(evtRegister->msg.getString(6) == "127.0.0.1");
    ASSERT(atoi(evtRegister->msg.getString(7).c_str()) > 0);
    ASSERT(atoi(evtRegister->msg.getString(8).c_str()) == 2);

    ASSERT(evtDestroy->msg.getArguments().size() == 3);
    ASSERT(evtDestroy->msg.getString(0) == sc.backend().getGuid());
    ASSERT(evtDestroy->msg.getString(1) == conns2[0]->getToken().getOperatorId());
    ASSERT(evtDestroy->msg.getString(2) == conns2[0]->getToken().getChannelId());

    ASSERT(evtUnregister->msg.getArguments().size() == 5);
    ASSERT(evtUnregister->msg.getString(0) == sc.backend().getGuid());
    ASSERT(evtUnregister->msg.getString(1) == conns[0]->getToken().getOperatorId());
    ASSERT(evtUnregister->msg.getString(2) == conns[0]->getToken().getUserId());
    ASSERT(evtUnregister->msg.getString(3) == conns[0]->getToken().getChannelId());
    ASSERT(atoi(evtUnregister->msg.getString(4).c_str()) == 1);
    
    ASSERT(evtUnregisterLast->msg.getArguments().size() == 5);
    ASSERT(evtUnregisterLast->msg.getString(0) == sc.backend().getGuid());
    ASSERT(evtUnregisterLast->msg.getString(1) == conns2[0]->getToken().getOperatorId());
    ASSERT(evtUnregisterLast->msg.getString(2) == conns2[0]->getToken().getUserId());
    ASSERT(evtUnregisterLast->msg.getString(3) == conns2[0]->getToken().getChannelId());
    ASSERT(atoi(evtUnregisterLast->msg.getString(4).c_str()) == 0);

    FreeConnections(conns);
    FreeConnections(conns2);

    delete evtRegisterFirst;
    delete evtRegister;
    delete evtUnregister;
    delete evtUnregisterLast;
    delete evtDestroy;
}

//=============================================================================
//= test_BackendKeepaliveInterval
//=============================================================================
static void test_BackendKeepaliveInterval()
{
    ServerContainer sc;
    sc.backend().setKeepaliveIgnore(false);

    Timeout t = Timeout((protocol::BACKEND_KEEPALIVE_INTERVAL_SEC - 1) * 1000);

    while (!t.expired())
    {
        ASSERT(sc.backend().m_events.empty());
        sonar::common::sleepFrame();
    }

    t = Timeout((protocol::BACKEND_KEEPALIVE_INTERVAL_SEC / 2) * 1000);

    while (!t.expired() && sc.backend().m_events.empty())
    {
        sonar::common::sleepFrame();
    }

    ASSERT(!t.expired());

    sc.backend().validateNextEvent(KEEPALIVE);


}

//=============================================================================
//= test_ServerGuid
//=============================================================================
static void test_ServerGuid()
{
    UUIDManager um1(31337);
    UUIDManager um2(31337);
    UUIDManager um3(31338);

    ASSERT(um1.get() == um2.get());
    ASSERT(um1.get() != um3.get());
}



//=============================================================================
//= test_ReplaceClient
//=============================================================================
static void test_ReplaceClient()
{
    ServerContainer sc;

    ConnectionList conns1 = GetConnections(1);
    ConnectionList conns2 = GetConnections(1);

    TestConnection *conn1 = conns1[0];
    TestConnection *conn2 = conns2[0];

    ConnectClients(conns1, 0, 1);
    ConnectClients(conns2, 0, 1);

    while (!conn1->isDisconnected() && !conn2->isConnected())
    {
        conn1->poll();
        conn2->poll();
    }

    DisconnectClients(conns1, 0, 1);
    DisconnectClients(conns2, 0, 1);
    
    FreeConnections(conns1);
    FreeConnections(conns2);

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_ConnectAndDisconnectAll
//=============================================================================
static void test_ConnectAndDisconnectAll()
{
    TestDebugLogger::m_output = false;

    ServerContainer sc;

    static const int CLIENTS_TO_CONNECT = protocol::CHANNEL_MAX_CLIENTS;
    
    ConnectionList conns = GetConnections(CLIENTS_TO_CONNECT);

        for (int index = 0; index < CLIENTS_TO_CONNECT; index++)
        {
            ConnectClients(conns, index, index + 1);

            for (int start = 0; start < index; start ++)
            {
                ASSERT(conns[start]->poll());
            }

            sonar::common::sleepHalfFrame();
        }

    BeInChannel(conns, 0, CLIENTS_TO_CONNECT);
    DisconnectClients(conns, 0, CLIENTS_TO_CONNECT);
    
    FreeConnections(conns);
    TestDebugLogger::m_output = true;

    for (size_t index = 0; index < CLIENTS_TO_CONNECT; index ++)
        sc.backend().validateNextEvent(CLIENT_REGISTERED);

    for (size_t index = 0; index < CLIENTS_TO_CONNECT; index ++)
        sc.backend().validateNextEvent(CLIENT_UNREGISTERED);

    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_ConnectOneTooMany
//=============================================================================
static void test_ConnectOneTooMany()
{
    TestDebugLogger::m_output = false;

    ServerContainer sc;

    static const int CLIENTS_TO_CONNECT = protocol::CHANNEL_MAX_CLIENTS;
    
    ConnectionList conns = GetConnections(CLIENTS_TO_CONNECT + 1);

    for (int index = 0; index < CLIENTS_TO_CONNECT; index++)
    {
        ConnectClients(conns, index, index + 1);

        for (int start = 0; start < index; start ++)
        {
            ASSERT(conns[start]->poll());
        }
        sonar::common::sleepExact(1);
    }

    BeInChannel(conns, 0, CLIENTS_TO_CONNECT);

    TestConnection &last = *conns[CLIENTS_TO_CONNECT];

    last.connect();

    Timeout t = Timeout(10000);

    while (!last.isDisconnected() && !t.expired())
    {
        for (int index = 0; index < CLIENTS_TO_CONNECT + 1; index ++)
        {
            conns[index]->poll();
            sonar::common::sleepExact(1);
        }
    }

    ASSERT (!t.expired());

    for (int index = 0; index < CLIENTS_TO_CONNECT - 1; index ++)
    {
        DisconnectClients(conns, index, index + 1);
    }
    
    DisconnectEvent *evDisconnect = (DisconnectEvent *) last.events[0];

    ASSERT (evDisconnect->reasonType == "SERVER_IS_FULL");
    
    FreeConnections(conns);
    TestDebugLogger::m_output = true;

    while (sc.getServer().getClientCount() != 0)
        sonar::common::sleepFrame();

    for (size_t index = 0; index < CLIENTS_TO_CONNECT; index ++)
        sc.backend().validateNextEvent(CLIENT_REGISTERED);

    for (size_t index = 0; index < CLIENTS_TO_CONNECT; index ++)
        sc.backend().validateNextEvent(CLIENT_UNREGISTERED);

    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_ConnectAndJoin
//=============================================================================
static void test_ConnectAndJoin()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(10);
    
    ConnectClients(conns, 0, 1);
    sonar::Connection::ClientMap clients = conns[0]->getClients();
    ASSERT (clients.size () == 1);
    ASSERT (clients[conns[0]->getCHID()].userId == "user0");


    ConnectClients(conns, 1, 10);

    Timeout timeout = Timeout(1000);

    while (conns[0]->getClients().size() != 10 && !timeout.expired())
    {
        for each (TestConnection *conn in conns)
        {
            conn->poll();
        }

        sonar::common::sleepExact(1);
    }

    ASSERT (!timeout.expired());
    clients = conns[0]->getClients();

    ASSERT (clients[conns[0]->getCHID()].userId == "user0"); ASSERT (clients[conns[0]->getCHID()].userDesc == "User0");
    ASSERT (clients[conns[1]->getCHID()].userId == "user1"); ASSERT (clients[conns[1]->getCHID()].userDesc == "User1");
    ASSERT (clients[conns[2]->getCHID()].userId == "user2"); ASSERT (clients[conns[2]->getCHID()].userDesc == "User2");
    ASSERT (clients[conns[3]->getCHID()].userId == "user3"); ASSERT (clients[conns[3]->getCHID()].userDesc == "User3");
    ASSERT (clients[conns[4]->getCHID()].userId == "user4"); ASSERT (clients[conns[4]->getCHID()].userDesc == "User4");
    ASSERT (clients[conns[5]->getCHID()].userId == "user5"); ASSERT (clients[conns[5]->getCHID()].userDesc == "User5");
    ASSERT (clients[conns[6]->getCHID()].userId == "user6"); ASSERT (clients[conns[6]->getCHID()].userDesc == "User6");
    ASSERT (clients[conns[7]->getCHID()].userId == "user7"); ASSERT (clients[conns[7]->getCHID()].userDesc == "User7");
    ASSERT (clients[conns[8]->getCHID()].userId == "user8"); ASSERT (clients[conns[8]->getCHID()].userDesc == "User8");
    ASSERT (clients[conns[9]->getCHID()].userId == "user9"); ASSERT (clients[conns[9]->getCHID()].userDesc == "User9");


    DisconnectClients(conns, 1, 10);

    timeout = Timeout(1000);

    while (conns[0]->getClients().size() != 1 && !timeout.expired())
    {
        for each (TestConnection *conn in conns)
        {
            conn->poll();
        }

        sonar::common::sleepExact(1);
    }

    ASSERT (!timeout.expired());

    clients = conns[0]->getClients();
    ASSERT (clients.size () == 1);
    ASSERT (clients[conns[0]->getCHID()].userId == "user0"); ASSERT (clients[conns[0]->getCHID()].userDesc == "User0");
    
    DisconnectClients(conns, 0, 1);
    FreeConnections(conns);

    for (size_t index = 0; index < conns.size(); index ++)
        sc.backend().validateNextEvent(CLIENT_REGISTERED);

    for (size_t index = 0; index < conns.size(); index ++)
        sc.backend().validateNextEvent(CLIENT_UNREGISTERED);

    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_UnregisterSuccessful
//=============================================================================
static void test_UnregisterSuccessful()
{
    ServerContainer sc;

    CString MY_REASON_TYPE = "RT_LEAVING";

    ConnectionList conns = GetConnections(2);

    ConnectClients(conns, 1, 2);

    ConnectClients(conns, 0, 1);

    DisconnectClients(conns, 0, 1, MY_REASON_TYPE);
    
    ConnectEvent *evConnect1;
    TestConnection &disconnecter = *conns[0];

    {
        evConnect1 = (ConnectEvent *) disconnecter.events[0];
        DisconnectEvent *evDisconnect = (DisconnectEvent *) disconnecter.events[1];
        ASSERT (evDisconnect->reasonType == MY_REASON_TYPE);
        ASSERT (evConnect1->name == "ConnectEvent");
    }


    ConnectEvent *evConnect2;
    {
        TestConnection &peer = *conns[1];

        while (peer.events.size () < 3)
        {
            ASSERT(peer.poll());
            sonar::common::sleepExact(1);
        }

        evConnect2 = (ConnectEvent *) peer.events[0];
        ClientJoinedEvent *evClientJoined = (ClientJoinedEvent *) peer.events[1];
        ClientPartedEvent *evClientParted = (ClientPartedEvent *) peer.events[2];

        ASSERT(evConnect1->channelId == evConnect2->channelId);
        ASSERT(evConnect1->channelDesc == evConnect2->channelDesc);
        ASSERT(evConnect1->channelDesc == TEST_CHANNEL_DESCRIPTION);

        ASSERT(evClientJoined->userId == disconnecter.getToken().getUserId());
        ASSERT(evClientJoined->userDesc == disconnecter.getToken().getUserDesc());

        ASSERT(evClientParted->userId == disconnecter.getToken().getUserId());
        ASSERT(evClientParted->userDesc == disconnecter.getToken().getUserDesc());
        ASSERT(evClientParted->reasonType == MY_REASON_TYPE);
    }


    DisconnectClients(conns, 1, 2);
    FreeConnections(conns);

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_UnregisterFaulty
//=============================================================================
static void test_UnregisterFaulty()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(2);
    ConnectClients(conns, 1, 2);
    ConnectClients(conns, 0, 1);

    TestConnection &disconnecter = *conns[0];
    TestConnection &peer = *conns[1];

    disconnecter.getTransport().setRxPacketLoss(100);

    ConnectEvent *evConnect2;
    {
        while (peer.events.size () < 3)
        {
            ASSERT(peer.poll());
            sonar::common::sleepExact(1);
        }

        evConnect2 = (ConnectEvent *) peer.events[0];
        ClientJoinedEvent *evClientJoined = (ClientJoinedEvent *) peer.events[1];
        ClientPartedEvent *evClientParted = (ClientPartedEvent *) peer.events[2];

        ASSERT(evClientJoined->userId == disconnecter.getToken().getUserId());
        ASSERT(evClientJoined->userDesc == disconnecter.getToken().getUserDesc());

        ASSERT(evClientParted->userId == disconnecter.getToken().getUserId());
        ASSERT(evClientParted->userDesc == disconnecter.getToken().getUserDesc());
        ASSERT(evClientParted->reasonType == "LOCAL_CONNECTION_LOST");
    }
    
    Timeout t(DefaultConnectionOptions.clientPingInterval + (protocol::RESEND_COUNT * protocol::RESEND_TIMEOUT_MSEC) + DefaultConnectionOptions.clientTickInterval * 2);

    while (!disconnecter.isDisconnected() && !t.expired())
    {
        disconnecter.poll();
        sonar::common::sleepExact(1);
        peer.poll();
        sonar::common::sleepExact(1);
    }

    ASSERT (!t.expired());

    DisconnectClients(conns, 1, 2);
    FreeConnections(conns);

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}


//=============================================================================
//= test_RegisterPacketLost
//=============================================================================
static void test_RegisterPacketLost()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(1);

    TestConnection &client = *conns[0];

    g_networkProvider.configureNextTransport(false, true);
    client.connect();

    Timeout t(protocol::REGISTER_TIMEOUT + protocol::RESEND_TIMEOUT_MSEC * 2);

    ASSERT (!client.isConnected());
    ASSERT (!client.isDisconnected());

    while (!client.isDisconnected() && !t.expired())
    {
        client.poll();
        sonar::common::sleepExact(1);
    }

    DisconnectEvent *evDisconnect = (DisconnectEvent *) client.events[0];

    ASSERT (evDisconnect->reasonType == "REGISTER_TIMEDOUT");
    FreeConnections(conns);
}


//=============================================================================
//= test_RegisterReplyLost
//=============================================================================
static void test_RegisterReplyLost()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(1);

    TestConnection &client = *conns[0];

    g_networkProvider.configureNextTransport(true, false);
    client.connect();
    client.getTransport().loseNextRxPackets(1000);

    Timeout t(protocol::REGISTER_TIMEOUT + protocol::RESEND_TIMEOUT_MSEC * 2);

    ASSERT (!client.isConnected());
    ASSERT (!client.isDisconnected());

    while (!client.isDisconnected() && !t.expired())
    {
        client.poll();
        sonar::common::sleepExact(1);
    }

    DisconnectEvent *evDisconnect = (DisconnectEvent *) client.events[0];

    ASSERT (evDisconnect->reasonType == "REGISTER_TIMEDOUT");

    FreeConnections(conns);

    while (sc.getServer().getClientCount() != 0)
        sonar::common::sleepFrame();

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_ReliableFullLoss
//=============================================================================
static void test_ReliableFullLoss()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(1);

    TestConnection &client = *conns[0];
    ConnectClients(conns, 0, 1);
    
    Timeout t(60000);

    sonar::Message msg(protocol::MSG_PING, client.getCID(), true);

    client.getTransport().loseNextTxPackets(1000);
    client.sendMessage(msg);

    while (!client.isDisconnected() && !t.expired())
    {
        if (!client.isDisconnected())
            client.poll();
        sonar::common::sleepExact(1);
    }

    ASSERT(!t.expired());

    DisconnectEvent *evDisconnect = (DisconnectEvent *) client.events[1];
    ASSERT (evDisconnect->reasonType == "LOCAL_CONNECTION_LOST");
        
    FreeConnections(conns);

    while (sc.getServer().getClientCount() != 0)
        common::sleepFrame();
    
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}


//=============================================================================
//= test_ReliableResend
//=============================================================================
static void test_ReliableResend()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(1);

    TestConnection &client = *conns[0];
    ConnectClients(conns, 0, 1);
    

    sonar::Message msg(protocol::MSG_PING, client.getCID(), true);
    client.getTransport().loseNextTxPacket();
    client.sendMessage(msg);

    client.sendMessage(msg);
    client.sendMessage(msg);

    Timeout t((protocol::RESEND_COUNT/2) * protocol::RESEND_TIMEOUT_MSEC);

    while (!t.expired())
    {
        client.getTransport().loseNextTxPacket();
        client.poll();
    sonar::common::sleepFrame();
    }

    ASSERT (client.isConnected());
    
    t = Timeout(protocol::RESEND_TIMEOUT_MSEC * 2);

    client.sendMessage(msg);
    client.sendMessage(msg);
    client.sendMessage(msg);
    client.sendMessage(msg);

    while (client.getTxQueueLength() > 0 && !t.expired())
    {
        client.poll();
    common::sleepFrame();
    }

    ASSERT (!t.expired());

    DisconnectClients(conns, 0, 1);
    FreeConnections(conns);

    while (sc.getServer().getClientCount() != 0)
        sonar::common::sleepFrame();

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_ReliableBigLossBoth
//=============================================================================
static void test_ReliableBigLossBoth()
{
    ServerContainer sc;

    static const int PACKETS_TO_SEND = 10;

    ConnectionList conns = GetConnections(1);
    TestConnection &client = *conns[0];
    ConnectClients(conns, 0, 1);

    for (int index = 0; index < PACKETS_TO_SEND; index ++)
    {
        sonar::Message msg(protocol::MSG_ECHO, client.getCID(), true);
        sonar::StringStream ss;
        ss << "echo-";
        ss << index;
        msg.writeUnsafeSmallString(ss.str());
        client.sendMessage(msg);
    }
    
    Timeout t(60000);

    client.getTransport().setTxPacketLoss(10);
    client.getTransport().setRxPacketLoss(10);

    client.clearEvents();

    while (!t.expired() && client.events.size() < PACKETS_TO_SEND)
    {
        ASSERT (client.poll());
        sonar::common::sleepExact(1);
    }

    //NOTE: Make sure server has SONARSRV_ANSWER_ECHO=1 in it's env, or else we will fail here
    ASSERT (!t.expired());
    ASSERT (client.isConnected());
    DisconnectClients(conns, 0, 1);

    int echoCount = 0;
    for (size_t index = 0; index < client.events.size(); index ++)
    {
        if (client.events[index]->name == "EchoMessageEvent")
        {
            EchoMessageEvent *evt = (EchoMessageEvent *) client.events[index];

            sonar::StringStream ss;
            ss << "echo-";
            ss << echoCount;

            sonar::String exp = ss.str();

            sonar::CString str = evt->message.readSmallString();
            ASSERT (str == exp);

            echoCount ++;
        }
    }

    FreeConnections(conns);

    while (sc.getServer().getClientCount() != 0)
        sonar::common::sleepFrame();

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_ReliableBigOutOfOrder
//=============================================================================
static void test_ReliableBigOutOfOrder()
{
    ServerContainer sc;

    static const int PACKETS_TO_SEND = 10;

    ConnectionList conns = GetConnections(1);
    TestConnection &client = *conns[0];
    ConnectClients(conns, 0, 1);

    for (int index = 0; index < PACKETS_TO_SEND; index ++)
    {
        sonar::Message msg(protocol::MSG_ECHO, client.getCID(), true);

        sonar::StringStream ss;
        ss << index;
        msg.writeUnsafeSmallString(ss.str());

        client.sendMessage(msg);
    }
    
    Timeout t(60000);

    client.getTransport().setTxPacketLoss(10);
    client.getTransport().setRxPacketLoss(10);

    client.clearEvents();

    int lastLength = 0;

    while (!t.expired() && client.events.size() < 10)
    {
        int currLength = client.getTxQueueLength();

        if (lastLength != currLength && currLength % 4 == 1)
        {
            client.getTransport().reorderNextTxPacket();
            lastLength = currLength;
        }

        ASSERT (client.poll());
        sonar::common::sleepExact(1);
    }

    //NOTE: Make sure server has SONARSRV_ANSWER_ECHO=1 in it's env, or else we will fail here
    ASSERT (!t.expired());
    ASSERT (client.isConnected());

    client.getTransport().setTxPacketLoss(0);
    client.getTransport().setRxPacketLoss(0);

    DisconnectClients(conns, 0, 1);

    int echoCount = 0;
    for (size_t index = 0; index < client.events.size(); index ++)
    {
        if (client.events[index]->name == "EchoMessageEvent")
        {
            EchoMessageEvent *evt = (EchoMessageEvent *) client.events[index];

            sonar::StringStream ss;
            ss << echoCount;
            CString str = evt->message.readSmallString();
            ASSERT (str == ss.str());

            echoCount ++;
        }
    }

    FreeConnections(conns);

    while (sc.getServer().getClientCount() != 0)
        sonar::common::sleepFrame();

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}


//=============================================================================
//= test_voiceTalkOneTake
//=============================================================================
static void test_voiceTalkOneTake()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(4);
    TestConnection &talker = *conns[0];
    TestConnection &listener = *conns[1];
    ConnectClients(conns, 0, 4);
    BeInChannel(conns, 0, 4);

    conns[0]->clearEvents();
    conns[1]->clearEvents();
    conns[2]->clearEvents();
    conns[3]->clearEvents();

    const char *payloadData = "This is my paylo";
    
    talker.sendAudioPayload(payloadData + 0, 4);
    talker.sendAudioPayload(payloadData + 4, 4);
    talker.sendAudioPayload(payloadData + 8, 4);
    talker.sendAudioPayload(payloadData + 12, 4);
    talker.sendAudioStop();

    Timeout t = Timeout(2000);

    while (listener.events.size() < 14 && ! t.expired())
    {
        for (int index = 0; index < 4; index ++)
        {
            conns[index]->poll();
        }	

    sonar::common::sleepFrame();
    }

    ASSERT(!t.expired());

    TestConnection::EventList::iterator iter = listener.events.begin();

    ASSERT((*iter)->name == "TakeBeginEvent");
    iter ++;

    sonar::String fullPayload;

    for (int index = 0; index < 4; index ++)
    {
        FrameBeginEvent *evFrameBegin = (FrameBeginEvent *) (*iter);
        ASSERT (evFrameBegin->name == "FrameBeginEvent");
        iter ++;
        ClientPayloadEvent *evClientPayload = (ClientPayloadEvent *) (*iter);
        ASSERT (evClientPayload->name == "ClientPayloadEvent");
        fullPayload += sonar::String(&evClientPayload->buffer[0], evClientPayload->buffer.size());

        iter ++;
        FrameEndEvent *evFrameEnd = (FrameEndEvent *) (*iter);
        ASSERT (evFrameEnd->name == "FrameEndEvent");
        iter ++;
    }

    ASSERT (fullPayload == payloadData);

    ASSERT((*iter)->name == "TakeEndEvent");
    iter ++;

    ASSERT(conns[2]->events.size() == 14);
    ASSERT(conns[3]->events.size() == 14);

    DisconnectClients(conns, 0, 4);
    FreeConnections(conns);

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_voiceTalkOneSendNoStop
//=============================================================================
static void test_voiceTalkOneSendNoStop()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(4);
    TestConnection &talker = *conns[0];
    TestConnection &listener = *conns[1];
    ConnectClients(conns, 0, 4);
    BeInChannel(conns, 0, 4);

    conns[0]->clearEvents();
    conns[1]->clearEvents();
    conns[2]->clearEvents();
    conns[3]->clearEvents();

    const char *payloadData = "This is my paylo";
    
    talker.sendAudioPayload(payloadData + 0, 4);
    talker.sendAudioPayload(payloadData + 4, 4);
    talker.sendAudioPayload(payloadData + 8, 4);
    talker.sendAudioPayload(payloadData + 12, 4);

    Timeout t = Timeout(2000);

    while (!t.expired())
    {
        int hasStop = 0;

        for (int index = 0; index < 4; index ++)
        {
            conns[index]->poll();
            sonar::common::sleepHalfFrame();

            for each (Event *evt in conns[index]->events)
            {
                if (evt->name == "TakeEndEvent")
                {
                    hasStop ++;
                    break;
                }
            }
        }

        if (hasStop == 4)
        {
            break;
        }
    }

    ASSERT (!t.expired());

    TestConnection::EventList::iterator iter = listener.events.begin();

    ASSERT((*iter)->name == "TakeBeginEvent");
    iter ++;

    sonar::String fullPayload;

    static const int EXPECTED_FRAME_COUNT = 4 + protocol::MAX_AUDIO_LOSS;
    static const int EXPECTED_EVENT_COUNT = 1 + EXPECTED_FRAME_COUNT * 3 + 1;

    for (int index = 0; index < EXPECTED_FRAME_COUNT; index ++)
    {
        FrameBeginEvent *evFrameBegin = (FrameBeginEvent *) (*iter);
        ASSERT (evFrameBegin->name == "FrameBeginEvent");
        iter ++;
        ClientPayloadEvent *evClientPayload = (ClientPayloadEvent *) (*iter);
        ASSERT (evClientPayload->name == "ClientPayloadEvent");

        if (index >= 4)
        {
            ASSERT (evClientPayload->buffer.empty());
        }
        else
        {
            // After the first ones we went we expect 25 NULL payloads
            fullPayload += sonar::String(&evClientPayload->buffer[0], evClientPayload->buffer.size());
        }

        iter ++;
        FrameEndEvent *evFrameEnd = (FrameEndEvent *) (*iter);
        ASSERT (evFrameEnd->name == "FrameEndEvent");
        iter ++;
    }

    ASSERT (fullPayload == payloadData);

    ASSERT((*iter)->name == "TakeEndEvent");
    iter ++;

    ASSERT(conns[0]->events.size() == EXPECTED_EVENT_COUNT);
    ASSERT(conns[1]->events.size() == EXPECTED_EVENT_COUNT);
    ASSERT(conns[2]->events.size() == EXPECTED_EVENT_COUNT);
    ASSERT(conns[3]->events.size() == EXPECTED_EVENT_COUNT);

    DisconnectClients(conns, 0, 4);

    FreeConnections(conns);

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_voiceTalkManySendNoStop
//=============================================================================
static void test_voiceTalkManySendNoStop()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(4);
    TestConnection *talker[2] = {conns[0], conns[1]};
    TestConnection &listener = *conns[2];
    ConnectClients(conns, 0, 4);
    BeInChannel(conns, 0, 4);

    conns[0]->clearEvents();
    conns[1]->clearEvents();
    conns[2]->clearEvents();
    conns[3]->clearEvents();

    const char *payloadData = "This is my paylo";
    
    talker[0]->sendAudioPayload(payloadData + 0, 4);
    talker[1]->sendAudioPayload(payloadData + 0, 4);
    talker[0]->sendAudioPayload(payloadData + 4, 4);
    talker[1]->sendAudioPayload(payloadData + 4, 4);
    talker[0]->sendAudioPayload(payloadData + 8, 4);
    talker[1]->sendAudioPayload(payloadData + 8, 4);
    talker[0]->sendAudioPayload(payloadData + 12, 4);
    talker[1]->sendAudioPayload(payloadData + 12, 4);
    
    Timeout t = Timeout(2000);

    while (!t.expired())
    {
        int hasStop = 0;

        for (int index = 0; index < 4; index ++)
        {
            conns[index]->poll();
            sonar::common::sleepHalfFrame();

            for each (Event *evt in conns[index]->events)
            {
                if (evt->name == "TakeEndEvent")
                {
                    hasStop ++;
                    break;
                }
            }
        }

        if (hasStop == 4)
        {
            break;
        }
    }

    ASSERT (!t.expired());

    TestConnection::EventList::iterator iter = listener.events.begin();

    ASSERT((*iter)->name == "TakeBeginEvent");
    iter ++;

    sonar::String fullPayload;

    static const int EXPECTED_FRAME_COUNT = 4 + protocol::MAX_AUDIO_LOSS;
    static const int EXPECTED_EVENT_COUNT = 1 + EXPECTED_FRAME_COUNT * 4 + 1;

    for (int index = 0; index < EXPECTED_FRAME_COUNT; index ++)
    {
        FrameBeginEvent *evFrameBegin = (FrameBeginEvent *) (*iter);
        ASSERT (evFrameBegin->name == "FrameBeginEvent");
        iter ++;
        ClientPayloadEvent *evClientPayload1 = (ClientPayloadEvent *) (*iter);
        ASSERT (evClientPayload1->name == "ClientPayloadEvent");
        ASSERT (evClientPayload1->subChannelMask & 0x01);
        iter ++;
        ClientPayloadEvent *evClientPayload2 = (ClientPayloadEvent *) (*iter);
        ASSERT (evClientPayload2->name == "ClientPayloadEvent");
        ASSERT (evClientPayload2->subChannelMask & 0x01);
        iter ++;

        ASSERT (evClientPayload1->buffer.size() == evClientPayload2->buffer.size());
        ASSERT (evClientPayload1->buffer == evClientPayload2->buffer);

        if (index >= 4)
        {
            ASSERT (evClientPayload1->buffer.empty());
        }
        else
        {
            // After the first ones we went we expect 25 NULL payloads
            fullPayload += sonar::String(&evClientPayload1->buffer[0], evClientPayload1->buffer.size());
        }

        FrameEndEvent *evFrameEnd = (FrameEndEvent *) (*iter);
        ASSERT (evFrameEnd->name == "FrameEndEvent");
        iter ++;
    }

    ASSERT (fullPayload == payloadData);

    ASSERT((*iter)->name == "TakeEndEvent");
    iter ++;

    ASSERT(conns[0]->events.size() == EXPECTED_EVENT_COUNT);
    ASSERT(conns[1]->events.size() == EXPECTED_EVENT_COUNT);
    ASSERT(conns[2]->events.size() == EXPECTED_EVENT_COUNT);
    ASSERT(conns[3]->events.size() == EXPECTED_EVENT_COUNT);

    DisconnectClients(conns, 0, 4);
    FreeConnections(conns);

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);

}

//=============================================================================
//= test_Challenge
//=============================================================================
static void test_Challenge()
{
    ServerContainer sc;
    ConnectionList conns = GetConnections(1);
    TestConnection *conn = conns[0];
    conn->overrideCID(0);

    // Valid SID valid delivery
    Message msgChallenge(protocol::MSG_CHALLENGE, 0, false);
    msgChallenge.writeUnsafeLong(31337);
    conn->setupTransport(MakeToken(0));
    conn->sendMessage(msgChallenge);
    conn->clearEvents();

    Timeout t = Timeout(1000);
    
    while (conn->events.empty() && !t.expired())
    {
        conn->poll();
        sonar::common::sleepHalfFrame();
    }
    
    ASSERT (!t.expired());

    ChallengeEvent *evt = (ChallengeEvent *) conn->events[0];
    ASSERT (evt->challenge == 31337);

    FreeConnections(conns);
}

//=============================================================================
//= test_ChallengeRate
//=============================================================================
static void test_ChallengeRate()
{
    ServerContainer sc;
    ConnectionList conns = GetConnections(1);
    TestConnection *conn = conns[0];
    conn->overrideCID(0);

    // Valid SID valid delivery
    conn->setupTransport(MakeToken(0));

    static const size_t NUM_CHALLENGES_INPUT = 10;
    static const size_t NUM_CHALLENGES_OUTPUT = NUM_CHALLENGES_INPUT / 2;

    for (size_t index = 0; index < NUM_CHALLENGES_INPUT; index ++)
    {
        Message msgChallenge(protocol::MSG_CHALLENGE, 0, false);
        msgChallenge.writeUnsafeLong(31337);
        conn->sendMessage(msgChallenge);
        sonar::common::sleepExact(protocol::CHALLENGE_REPLY_MIN_INTERVAL / 2);
    }
    conn->clearEvents();

    Timeout t = Timeout(1000);
    
    while (!t.expired())
    {
        conn->poll();
        sonar::common::sleepHalfFrame();
    }
    
    ASSERT (conn->events.size() - NUM_CHALLENGES_OUTPUT < 2);
    FreeConnections(conns);
}
//=============================================================================
//= test_ChallengeRateRecheck
//=============================================================================
void test_ChallengeRateRecheck()
{
    common::sleepExact(protocol::CHALLENGE_REPLY_MIN_INTERVAL * 2);
    test_Challenge();
    common::sleepExact(protocol::CHALLENGE_REPLY_MIN_INTERVAL * 2);
    test_Challenge();
}

//=============================================================================
//= test_ChallengeInvalidSID
//=============================================================================
static void test_ChallengeInvalidSID()
{
    ServerContainer sc;
    ConnectionList conns = GetConnections(1);
    TestConnection *conn = conns[0];
    conn->overrideCID(31231);

    // Valid SID valid delivery
    Message msgChallenge(protocol::MSG_CHALLENGE, 31231, false);
    msgChallenge.writeUnsafeLong(31337);
    conn->setupTransport(MakeToken(0));
    conn->sendMessage(msgChallenge);
    conn->clearEvents();

    Timeout t = Timeout(1000);
    
    while (!t.expired())
    {
        conn->poll();
        sonar::common::sleepHalfFrame();
    }
    
    ASSERT (conn->events.empty());

    FreeConnections(conns);
}

//=============================================================================
//= test_ChallengeInvalidDelivery
//=============================================================================
static void test_ChallengeInvalidDelivery()
{
    ServerContainer sc;
    ConnectionList conns = GetConnections(1);
    TestConnection *conn = conns[0];
    conn->overrideCID(0);

    // Valid SID valid delivery
    Message msgChallenge(protocol::MSG_CHALLENGE, 0, true);
    msgChallenge.writeUnsafeLong(31337);
    conn->setupTransport(MakeToken(0));
    conn->sendMessage(msgChallenge);
    conn->clearEvents();

    Timeout t = Timeout(1000);
    
    while (!t.expired())
    {
        conn->poll();
        sonar::common::sleepHalfFrame();
    }
    
    ASSERT (conn->events.empty());
    FreeConnections(conns);
}


//=============================================================================
//= test_voiceTalkMany
//=============================================================================
static void test_voiceTalkMany()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(4);
    ConnectClients(conns, 0, 4);
    BeInChannel(conns, 0, 4);

    TestConnection &listener = *conns[0];

    conns[0]->clearEvents();
    conns[1]->clearEvents();
    conns[2]->clearEvents();
    conns[3]->clearEvents();

    jlib::UINT64 payloads[4] = { 
        (jlib::UINT64) conns[0], 
        (jlib::UINT64) conns[1], 
        (jlib::UINT64) conns[2], 
        (jlib::UINT64) conns[3] 
    };

    for (int tick = 0; tick < 16; tick ++)
    {
        switch (tick)
        {
            // Client 0 is talking
        case 0: 
        case 1:
        case 2:
            conns[0]->sendAudioPayload(&payloads[0], sizeof(payloads[0]));
            break;

        case 3:
            conns[0]->sendAudioPayload(&payloads[0], sizeof(payloads[0]));
            conns[0]->sendAudioStop();

            // Nobody is talking
        case 4: 
        case 5:
        case 6:
        case 7:
            break;

            // Client 1 and 2 are talking
        case 8:
        case 9:
        case 10:
            conns[1]->sendAudioPayload(&payloads[1], sizeof(payloads[1]));
            conns[2]->sendAudioPayload(&payloads[2], sizeof(payloads[2]));
            break;
        case 11:
            conns[1]->sendAudioPayload(&payloads[1], sizeof(payloads[1]));
            conns[2]->sendAudioPayload(&payloads[2], sizeof(payloads[2]));
            conns[1]->sendAudioStop();
            conns[2]->sendAudioStop();
            break;

            // Silence
        case 12:
        case 13:
        case 14:
        case 15:
            break;
        }

        for (int index2 = 0; index2 < 4; index2 ++)
        {
            ASSERT(conns[index2]->poll());
        }

        sonar::common::sleepFrame();
    }

    static const int EXPECTED_EVENTS = 31;

    while (listener.events.size() < EXPECTED_EVENTS)
    {
        for (int index2 = 0; index2 < 4; index2 ++)
        {
            ASSERT(conns[index2]->poll());
        }

        sonar::common::sleepFrame();
    }

    int payloadSources[4];

    memset(payloadSources, 0, sizeof(payloadSources));

    for (size_t index = 0; index < EXPECTED_EVENTS; index ++)
    {
        if (listener.events[index]->name == "ClientPayloadEvent")
        {
            ClientPayloadEvent *evt = (ClientPayloadEvent *) listener.events[index];
            payloadSources[evt->chid] ++;
        }
    }

    ASSERT(abs(payloadSources[conns[0]->getCHID()] - 4) < 2);
    ASSERT(abs(payloadSources[conns[1]->getCHID()] - 4) < 2);
    ASSERT(abs(payloadSources[conns[2]->getCHID()] - 4) < 2);
    
    ASSERT(listener.events[listener.events.size() - 1]->name == "TakeEndEvent");

    DisconnectClients(conns, 0, 4);
    FreeConnections(conns);

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_TooManyTalkers
//=============================================================================
static void test_TooManyTalkers()
{
    ServerContainer sc;

    static const size_t MAX_TALKERS = ((protocol::MESSAGE_BUFFER_MAX_SIZE - sizeof(protocol::MessageHeader)) / protocol::PAYLOAD_MAX_SIZE);
    static const size_t CLIENT_COUNT = MAX_TALKERS + 10;
    char payload[protocol::PAYLOAD_MAX_SIZE];
    
    ConnectionList conns = GetConnections(CLIENT_COUNT);
    ConnectClients(conns, 0, CLIENT_COUNT);
    BeInChannel(conns, 0, CLIENT_COUNT);

    sc.suspend();

    for (size_t index = 0; index < CLIENT_COUNT; index ++)
    {
        conns[index]->sendAudioPayload(payload, sizeof(payload));
        conns[index]->sendAudioStop();
    }

    sc.resume();

    TestConnection &listener = *conns[0];

    listener.clearEvents();

    size_t EVENTS_EXPECTED = 1 + 1 + MAX_TALKERS + 1 + 1;
        
    while (listener.events.size() < EVENTS_EXPECTED)
    {
        for (size_t index = 0; index < CLIENT_COUNT; index ++)
            conns[index]->poll();

        sonar::common::sleepHalfFrame();
    }

    size_t offset = 0;

    ASSERT(listener.events[offset++]->name == "TakeBeginEvent");
    ASSERT(listener.events[offset++]->name == "FrameBeginEvent");

    for (size_t index = 0; index < MAX_TALKERS; index ++)
    {
        ASSERT(listener.events[offset++]->name == "ClientPayloadEvent");
    }

    ASSERT(listener.events[offset++]->name == "FrameEndEvent");
    ASSERT(listener.events[offset++]->name == "TakeEndEvent");

    ASSERT(offset == EVENTS_EXPECTED);

    DisconnectClients(conns, 0, CLIENT_COUNT);
    FreeConnections(conns);

    for (size_t index = 0; index < CLIENT_COUNT; index ++)
        sc.backend().validateNextEvent(CLIENT_REGISTERED);

    for (size_t index = 0; index < CLIENT_COUNT; index ++)
        sc.backend().validateNextEvent(CLIENT_UNREGISTERED);

    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_FullAudioLossRevive
//=============================================================================
static void test_FullAudioLossRevive()
{
    ServerContainer sc;
    char payload[protocol::PAYLOAD_MAX_SIZE];
    
    ConnectionList conns = GetConnections(2);
    ConnectClients(conns, 0, 2);
    BeInChannel(conns, 0, 2);
        
    TestConnection &talker = *conns[0];
    TestConnection &listener = *conns[1];

    talker.sendAudioPayload(payload, sizeof(payload));
    
    listener.clearEvents();

    static const size_t EXPECTED_PAYLOADS = (protocol::MAX_AUDIO_LOSS + 1);
    static const int EVENTS_EXPECTED = (EXPECTED_PAYLOADS * 3) + 2;
    
    while (listener.events.size() < EVENTS_EXPECTED)
    {
        for (size_t index = 0; index < conns.size(); index ++)
            conns[index]->poll();

        sonar::common::sleepHalfFrame();
    }
  
    size_t offset = 0;

    ASSERT(listener.events[offset++]->name == "TakeBeginEvent");

    for (size_t index = 0; index < EXPECTED_PAYLOADS; index ++)
    {
        ASSERT(listener.events[offset++]->name == "FrameBeginEvent");
        ASSERT(listener.events[offset++]->name == "ClientPayloadEvent");
        ASSERT(listener.events[offset++]->name == "FrameEndEvent");
    }

    ASSERT(listener.events[offset++]->name == "TakeEndEvent");
    ASSERT(offset == EVENTS_EXPECTED);

    talker.sendAudioPayload(payload, sizeof(payload));

  listener.clearEvents();

    while (listener.events.size() < EVENTS_EXPECTED)
    {
        for (size_t index = 0; index < conns.size(); index ++)
            conns[index]->poll();

        sonar::common::sleepHalfFrame();
    }

    offset = 0;

    ASSERT(listener.events[offset++]->name == "TakeBeginEvent");

    for (size_t index = 0; index < EXPECTED_PAYLOADS; index ++)
    {
        ASSERT(listener.events[offset++]->name == "FrameBeginEvent");
        ASSERT(listener.events[offset++]->name == "ClientPayloadEvent");
        ASSERT(listener.events[offset++]->name == "FrameEndEvent");
    }

    ASSERT(listener.events[offset++]->name == "TakeEndEvent");
    ASSERT(offset == EVENTS_EXPECTED);


    DisconnectClients(conns, 0, 2);
    FreeConnections(conns);

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_RsaTokenExpired
//=============================================================================
static void test_RsaTokenExpired()
{
    ServerContainer sc;

    ConnectionList conns;
    TestConnection *conn = new TestConnection(g_timeProvider, g_networkProvider, g_logger, MakeToken(0, "", time(0) - protocol::TOKEN_MAX_AGE * 2));
    conns.push_back(conn);

    conn->connect();

    Timeout t = Timeout(1000);

    while (!t.expired())
    {
        conn->poll();
        sonar::common::sleepHalfFrame();
    }

    ASSERT (conn->isDisconnected());
    DisconnectEvent *evt = (DisconnectEvent *) conn->events[0];

    ASSERT (evt->reasonType == "TOKEN_EXPIRED");

    FreeConnections(conns);
}

//=============================================================================
//= test_RsaKeyTokenTamper
//=============================================================================
static void test_RsaKeyTokenTamper()
{
    ServerContainer sc;

    ConnectionList conns;

    String tokenString = MakeToken(0);
    size_t offset= tokenString.rfind('|');
    char orgChar = tokenString[offset + 1];
    char newChar;
    do
    {
        newChar = 'a'+ rand () % 25;
    } while(orgChar == newChar);

    tokenString[offset + 1] = newChar;
    
    TestConnection *conn = new TestConnection(g_timeProvider, g_networkProvider, g_logger, tokenString);
    
    conns.push_back(conn);

    conn->connect();

    Timeout t = Timeout(1000);

    while (!t.expired())
    {
        conn->poll();
        sonar::common::sleepHalfFrame();
    }

    ASSERT (conn->isDisconnected());
    DisconnectEvent *evt = (DisconnectEvent *) conn->events[0];

    ASSERT (evt->reasonType == "TOKEN_INVALID");

    FreeConnections(conns);
}

//=============================================================================
//= test_talkBalanceServer
//=============================================================================
static void test_talkBalanceServer()
{
    Server::Config config;
    config.talkBalanceMax = protocol::FRAMES_PER_SECOND / 2;
    config.talkBalanceMin = protocol::FRAMES_PER_SECOND / 4;
  config.talkBalancePenalty = protocol::FRAMES_PER_SECOND;

    ServerContainer sc(&config);

    ConnectionList conns = GetConnections(2);
    TestConnection &talker = *conns[0];
    TestConnection &listener = *conns[1];
    ConnectClients(conns, 0, 2);
    BeInChannel(conns, 0, 2);

    conns[0]->clearEvents();
    conns[1]->clearEvents();

    for (int index = 0; index < config.talkBalanceMax * 2; index ++)
    {
        talker.sendAudioPayload("31337", 5);

        talker.poll();
        listener.poll();

        sonar::common::sleepFrame();
    }

    size_t offset = 0;

    ASSERT(listener.events[offset++]->name == "TakeBeginEvent");

    int frames = 0;

    for (; offset < listener.events.size(); offset += 3)
    {
        if (listener.events[offset + 0]->name == "TakeEndEvent")
        {
            offset ++;
            break;
        }

        ASSERT(listener.events[offset + 0]->name == "FrameBeginEvent");
        ASSERT(listener.events[offset + 1]->name == "ClientPayloadEvent");
        ASSERT(listener.events[offset + 2]->name == "FrameEndEvent");

        frames ++;
    }

    ASSERT (abs(config.talkBalanceMax - frames) < 2);
    ASSERT (listener.events.size() == offset);

  for (int x = 0; x < (config.talkBalanceMax + config.talkBalancePenalty) - config.talkBalanceMin; x ++)
    common::sleepFrame();

    int framesToSend = config.talkBalanceMax - config.talkBalanceMin;

    for (int index = 0; index < framesToSend; index ++)
    {
        talker.sendAudioPayload("31337", 5);

        talker.poll();
        listener.poll();

        common::sleepFrame();
    }

    frames = 0;

    ASSERT(listener.events[offset++]->name == "TakeBeginEvent");

    for (; offset < listener.events.size(); offset += 3)
    {
        if (listener.events[offset + 0]->name == "TakeEndEvent")
        {
            offset ++;
            break;
        }

        ASSERT(listener.events[offset + 0]->name == "FrameBeginEvent");
        ASSERT(listener.events[offset + 1]->name == "ClientPayloadEvent");
        ASSERT(listener.events[offset + 2]->name == "FrameEndEvent");

        frames ++;
    }
    
    ASSERT (abs(framesToSend - frames) < 2);

    DisconnectClients(conns, 0, 2);
    FreeConnections(conns);

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_FullLossOfChannelState
//=============================================================================
static void test_FullLossOfChannelState()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(1);

    TestConnection &client = *conns[0];

    client.connect();
    
    while (client.getCID() != 1)
    {
        client.poll();
        common::sleepFrame();
    }
    
    client.getTransport().loseNextRxPackets(100);
    
    while (!client.isDisconnected())
    {
        client.poll();
    common::sleepFrame();
    }

    ASSERT(client.getChanSize() == 1);
    FreeConnections(conns);

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_AudioStoppedLatePacketDrop
//=============================================================================
static void test_AudioStoppedLatePacketDrop()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(1);
    TestConnection &client = *conns[0];

    ConnectClients(conns, 0, 1);

    client.sendAudioPayload("TESTTEST", 8);
    client.sendAudioPayload("TESTTEST", 8);
    client.sendAudioPayload("TESTTEST", 8);
    client.sendAudioPayload("TESTTEST", 8);

    client.sendAudioStop();
    client.dbgSetOutTakeNumber(client.dbgGetOutTakeNumber() - 1);
    client.sendAudioPayload("NONO", 4);
    client.sendAudioPayload("NONO", 4);
    client.sendAudioPayload("NONO", 4);
    client.sendAudioPayload("NONO", 4);
    client.sendAudioStop();

    static const int EXPECTED_EVENTS = 1 + 1 + (4 * 3);

    Timeout t = Timeout(1000);

    while (!t.expired())
    {
        client.poll();
        common::sleepFrame();
    }

    ASSERT(sc.getServer().getMetrics().dropLatePacket == 5);

    for (size_t index = 0; index < client.events.size(); index++)
    {
        if (client.events[index]->name != "ClientPayloadEvent")
            continue;

        ClientPayloadEvent *evt = (ClientPayloadEvent *) client.events[index];
        ASSERT(evt->buffer.size() != 4); // 4 is length of NONO
    }


    DisconnectClients(conns, 0, 1);
    FreeConnections(conns);
    
    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_AudioStartedLatePacketDrop
//=============================================================================
static void test_AudioStartedLatePacketDrop()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(1);
    TestConnection &client = *conns[0];

    ConnectClients(conns, 0, 1);

    client.sendAudioPayload("TESTTEST", 8);
    client.sendAudioPayload("TESTTEST", 8);
    client.sendAudioPayload("TESTTEST", 8);
    client.sendAudioPayload("TESTTEST", 8);
    client.dbgSetOutTakeNumber(client.dbgGetOutTakeNumber() - 1);
    client.sendAudioPayload("NONO", 4);
    client.sendAudioPayload("NONO", 4);
    client.sendAudioPayload("NONO", 4);
    client.sendAudioPayload("NONO", 4);

    static const int EXPECTED_EVENTS = 1 + 1 + (4 * 3);

    Timeout t = Timeout(1000);

    while (!t.expired())
    {
        client.poll();
        common::sleepFrame();
    }

    ASSERT(sc.getServer().getMetrics().dropLatePacket == 4);

    for (size_t index = 0; index < client.events.size(); index++)
    {
        if (client.events[index]->name != "ClientPayloadEvent")
            continue;

        ClientPayloadEvent *evt = (ClientPayloadEvent *) client.events[index];
        ASSERT(evt->buffer.size() != 4); // 4 is length of NONO
    }


    DisconnectClients(conns, 0, 1);
    FreeConnections(conns);

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_AudioStartedLatePacketRestart
//=============================================================================
static void test_AudioStartedLatePacketRestart()
{
    ServerContainer sc;

    ConnectionList conns = GetConnections(1);
    TestConnection &client = *conns[0];

    ConnectClients(conns, 0, 1);

    client.sendAudioPayload("TESTTEST", 8);
    client.sendAudioPayload("TESTTEST", 8);
    client.sendAudioPayload("TESTTEST", 8);
    client.sendAudioPayload("TESTTEST", 8);
    client.dbgSetOutTakeNumber(client.dbgGetOutTakeNumber() + 1);
    client.sendAudioPayload("YESYES", 6);
    client.sendAudioPayload("YESYES", 6);
    client.sendAudioPayload("YESYES", 6);
    client.sendAudioPayload("YESYES", 6);
    client.dbgSetOutTakeNumber(client.dbgGetOutTakeNumber() - 1);
    client.sendAudioPayload("NONO", 4);
    client.sendAudioPayload("NONO", 4);
    client.sendAudioPayload("NONO", 4);
    client.sendAudioPayload("NONO", 4);

    static const int EXPECTED_EVENTS = 1 + 1 + (8 * 3);

    Timeout t = Timeout(1000);

    while (!t.expired())
    {
        client.poll();
        common::sleepFrame();
    }

    ASSERT(sc.getServer().getMetrics().dropLatePacket == 4);

    int sizes[10] = { 0 };

    for (size_t index = 0; index < client.events.size(); index++)
    {
        if (client.events[index]->name != "ClientPayloadEvent")
            continue;

        ClientPayloadEvent *evt = (ClientPayloadEvent *) client.events[index];
        ASSERT(evt->buffer.size() == 8 || evt->buffer.size() == 6 || evt->buffer.size() == 0); 

        sizes[evt->buffer.size()] ++;
    }

    ASSERT(sizes[0] <= protocol::MAX_AUDIO_LOSS);
    ASSERT(sizes[8] == 4);
    ASSERT(sizes[6] == 4);


    DisconnectClients(conns, 0, 1);
    FreeConnections(conns);

    sc.backend().validateNextEvent(CLIENT_REGISTERED);
    sc.backend().validateNextEvent(CLIENT_UNREGISTERED);
    sc.backend().validateNextEvent(CHANNEL_DESTROYED);
}

//=============================================================================
//= test_TokenMemLeak
//=============================================================================
void test_TokenMemLeak(void)
{
    for (int index = 0; index < 1000; index ++)
        delete new TokenSigner();
}

//=============================================================================
//= test_TokenSignMemLeak
//=============================================================================
void test_TokenSignMemLeak(void)
{
    String tokenString = "SONAR2|127.0.0.1:31337|127.0.0.1:31337|default|user|User|channel|Channel||31337|";
    Token token;
    token.parse(tokenString);

    for (int index = 0; index < 1000; index ++)
    {
        TokenSigner *signer = new TokenSigner();
        signer->sign(token);

        delete signer;
    }
}

//=============================================================================
//= test_BackendPubKeyMemLeak
//=============================================================================
void test_BackendPubKeyMemLeak(void)
{

}
#define __USE_STACKWALKER__

#ifdef __USE_STACKWALKER__
#include <SonarVoiceServer/Stackwalker.h>
#define INIT_STACKWALKER() InitAllocCheck(ACOutput_XML);
#define SHUT_STACKWALKER() \
  { int _ret = DeInitAllocCheck(); if (_ret > 1340) { fprintf (stderr, "Test leaked %d bytes\n", _ret); ASSERT(false); } }
#endif


#define RUN_TEST(_func) \
  INIT_STACKWALKER(); \
    fprintf (stderr, "%s is running\n", #_func);\
    _func();\
    ASSERT(TestConnection::m_instances == 0); \
  SHUT_STACKWALKER(); \
    fprintf (stderr, "%s is done\n", #_func);


int wmain(int argc, wchar_t **argv)
{
    bool failed = false;
  
    g_tokenSigner = new TokenSigner();

    //_set_abort_behavior(0, _WRITE_ABORT_MSG );

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    RUN_TEST(test_FullAudioLossRevive);
  RUN_TEST(test_ConnectAndDisconnect);
    RUN_TEST(test_TokenMemLeak);
    RUN_TEST(test_TokenSignMemLeak);
    RUN_TEST(test_ServerGuid);
    RUN_TEST(test_ReplaceClient);
    RUN_TEST(test_BackendKeepaliveInterval);
    RUN_TEST(test_BackendRegisterUnregisterDestroy);

#ifdef _DEBUG
    fprintf (stderr, "WARNING: Skipping test test_ConnectOneTooMany in debug build\n");
    failed = true;
#else
    RUN_TEST(test_ConnectOneTooMany);
#endif

#ifdef _DEBUG
    fprintf (stderr, "WARNING: Skipping test test_ConnectAndDisconnectAll in debug build\n");
    failed = true;
#else
    RUN_TEST(test_ConnectAndDisconnectAll);
#endif

    RUN_TEST(test_ServerGuid);
    RUN_TEST(test_ReliableFullLoss);
    RUN_TEST(test_AudioStartedLatePacketRestart);
    RUN_TEST(test_AudioStartedLatePacketDrop);
    RUN_TEST(test_AudioStoppedLatePacketDrop);
    RUN_TEST(test_voiceTalkMany);
    RUN_TEST(test_FullLossOfChannelState);
    RUN_TEST(test_talkBalanceServer);
    RUN_TEST(test_RsaKeyTokenTamper);
    RUN_TEST(test_ChallengeRate);
    RUN_TEST(test_RsaTokenExpired);
    RUN_TEST(test_voiceTalkOneSendNoStop);
    RUN_TEST(test_TooManyTalkers);
    RUN_TEST(test_ChallengeInvalidSID);
    RUN_TEST(test_Challenge);
    RUN_TEST(test_ChallengeRateRecheck);
    RUN_TEST(test_ReplaceClient);
    RUN_TEST(test_voiceTalkOneTake);
    RUN_TEST(test_voiceTalkManySendNoStop);
    RUN_TEST(test_ReliableBigLossBoth);
    RUN_TEST(test_RegisterReplyLost);
    RUN_TEST(test_ReliableResend);
    RUN_TEST(test_ReliableBigOutOfOrder);
    RUN_TEST(test_UnregisterFaulty);
    RUN_TEST(test_RegisterPacketLost);
    RUN_TEST(test_ConnectAndJoin);
    RUN_TEST(test_UnregisterSuccessful);
    RUN_TEST(test_ChallengeInvalidDelivery);

    //getchar();

  ERR_free_strings();
  EVP_cleanup();
  CRYPTO_cleanup_all_ex_data();
    delete g_tokenSigner;

    fprintf (stderr, "DONE!\n");
    return failed ? -1 : 0;
}
#include "Connection.h"

#include <QDebug>
#include <QDomDocument>
#include <QMutexLocker>
#include <QThread>
#include <QTimer>

#include "talk/xmpp/xmppthread.h"
#include "JingleClient.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformService.h"
#include "services/settings/SettingsManager.h"
#include "services/voice/VoiceService.h"
#include "engine/login/LoginController.h"

#include "ConnectedUser.h"
#include "XMPPImplEventAdapter.h"
#include "XMPPImplEventPump.h"
#include "MetaTypes.h"
#include "Roster.h"
#include "MessageFilter.h"
#include "MucRoom.h"

namespace
{
    using namespace Origin::Chat;

    void initializeChatModel()
    {
        static QMutex initLock;
        static bool initialized = false;
        QMutexLocker locker(&initLock);
        
        if (!initialized)
        {
            // Register our types with Qt
            MetaTypes::registerMetaTypes();

            initialized = true;
        }
    }

    void GetCapabilitiesCacheLocation(QString& directoryPath, QString& fullPath)
    {
        directoryPath = Origin::Services::PlatformService::getStorageLocation(QStandardPaths::CacheLocation);
        fullPath = directoryPath + QDir::separator() + "XEP0115Cache.xml";
    }
}

namespace Origin
{
namespace Chat
{
    // XMPP tracing
    class DebugLog : public sigslot::has_slots<>
    {
        void prettyPrint(const std::string& label, const std::string& sdata)
        {
            QString qlabel(QString::fromStdString(label));
            QString qdata(QString("<%1>%2</%3>").arg(qlabel).arg(QString::fromStdString(sdata)).arg(qlabel));
            
            QDomDocument doc;
            QString errorMsg;
            int errorLine = 0;
            int errorColumn = 0;
            bool wasSet = doc.setContent(qdata, &errorMsg, &errorLine, &errorColumn);
            if (wasSet)
                ORIGIN_LOG_EVENT << "\n\n" << doc.toString(2) << "\n";
            else
                ORIGIN_LOG_EVENT << "\n\n<" << qlabel << ">\n  " << sdata << "\n</" << qlabel << ">\n";
        }
        
    public:
    
        void Input(const char * data, int len)
        {
            prettyPrint("xmpp-in", std::string(data, len));
        }
        
        void Output(const char * data, int len)
        {
            prettyPrint("xmpp-out", std::string(data, len));
        }
    };
    DebugLog sDebugLog;
    
    Connection::Connection(const QByteArray &host, int port, const Configuration &config) :
        mHost(host),
        mPort(port),
        mConfiguration(config),
        mJingle(NULL),
        mImplEventPumpThread(NULL),
        mImplEventPump(NULL),
        mJingleThread(NULL),
        mEstablished(0),
        mJingleSettings(new buzz::XmppClientSettings),
        mReconnectAttempts(-1)
    {
        initializeChatModel();

        mImplEventAdapter = new XMPPImplEventAdapter(this);

        // This needs the event adapter
        mCurrentUser = new ConnectedUser(this);

        // Set up a simple identity
        mDiscoverableInformation.addIdentity(DiscoverableIdentity("client", "pc", QCoreApplication::applicationName()));

        mDiscoverableInformation.addSupportedFeatureUri(CHATCAP_CAPS);
        mDiscoverableInformation.addSupportedFeatureUri(CHATCAP_CHATSTATES);
        mDiscoverableInformation.addSupportedFeatureUri(CHATCAP_DISCOINFO);
        mDiscoverableInformation.addSupportedFeatureUri(CHATCAP_MUC);
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( isVoiceEnabled )
            mDiscoverableInformation.addSupportedFeatureUri(CHATCAP_VOIP);
#endif

        // Start up the capabilities cache
        bool isLogging = Origin::Services::readSetting(Origin::Services::SETTING_LogXep0115);
        buzz::CapabilitiesCache::Logging logging = isLogging ? buzz::CapabilitiesCache::ENABLE_LOGGING : buzz::CapabilitiesCache::DISABLE_LOGGING;
        buzz::CapabilitiesCache::instance().Init(logging);

        QString directoryPath;
        QString fullPath;
        GetCapabilitiesCacheLocation(directoryPath, fullPath);
        if (QFile::exists(fullPath))
        {
            QFile capsCache(fullPath);
            if (capsCache.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QString capsCacheXml;
                QTextStream in(&capsCache);
                capsCacheXml = in.readAll();
                if (!capsCacheXml.isEmpty())
                {
                    ORIGIN_LOG_DEBUG << "Capabilities Cache read: " << capsCacheXml;
                    buzz::CapabilitiesCache::instance().FromXml(capsCacheXml.toStdString());
                }
            }
        }

        ORIGIN_VERIFY_CONNECT(mImplEventAdapter, SIGNAL(messageReceived(const JingleMessage &)),
            this, SLOT(jingleMessageReceived(const JingleMessage &)));

        ORIGIN_VERIFY_CONNECT(mImplEventAdapter, SIGNAL(chatStateReceived(const JingleMessage &)),
            this, SLOT(jingleChatStateReceived(const JingleMessage &)));

        ORIGIN_VERIFY_CONNECT(mImplEventAdapter, SIGNAL(presenceUpdate(Origin::Chat::XmppPresenceProxy)),
            this, SLOT(presenceUpdate(Origin::Chat::XmppPresenceProxy)));

        ORIGIN_VERIFY_CONNECT(mImplEventAdapter, SIGNAL(capabilitiesChanged(Origin::Chat::XmppCapabilitiesProxy)),
            this, SLOT(onCapabilitiesChanged(Origin::Chat::XmppCapabilitiesProxy)));

        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(addRequest(buzz::Jid)),
            this, SLOT(addRequest(buzz::Jid)));
        
        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(roomEntered(Origin::Chat::XmppChatroomModuleProxy, QString)),
            this, SLOT(roomEntered(Origin::Chat::XmppChatroomModuleProxy, QString)));

        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(roomEnterFailed(Origin::Chat::XmppChatroomModuleProxy, buzz::XmppChatroomEnteredStatus )),
            this, SLOT(roomEnterFailed(Origin::Chat::XmppChatroomModuleProxy, buzz::XmppChatroomEnteredStatus)));

        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(roomDestroyed(Origin::Chat::XmppChatroomModuleProxy)),
            this, SLOT(roomDestroyed(Origin::Chat::XmppChatroomModuleProxy)));

        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(roomKicked(Origin::Chat::XmppChatroomModuleProxy)),
            this, SLOT(roomKicked(Origin::Chat::XmppChatroomModuleProxy)));

        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(roomPasswordIncorrect(Origin::Chat::XmppChatroomModuleProxy)),
            this, SLOT(roomPasswordIncorrect(Origin::Chat::XmppChatroomModuleProxy)));
        
        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(roomUserJoined(Origin::Chat::XmppChatroomMemberProxy)),
            this, SLOT(roomUserJoined(Origin::Chat::XmppChatroomMemberProxy)));

        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(roomUserLeft(Origin::Chat::XmppChatroomMemberProxy)),
            this, SLOT(roomUserLeft(Origin::Chat::XmppChatroomMemberProxy)));

        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(roomUserChanged(Origin::Chat::XmppChatroomMemberProxy)),
            this, SLOT(roomUserChanged(Origin::Chat::XmppChatroomMemberProxy)));
    
        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(roomDeclineReceived(buzz::Jid, buzz::Jid, QString)),
            this, SLOT(roomDeclineReceived(buzz::Jid, buzz::Jid, QString)));

        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(chatGroupRoomInviteReceived(buzz::Jid, QString, QString)),
            this, SLOT(chatGroupRoomInviteReceived(buzz::Jid, QString, QString)));

        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(streamError(Origin::Chat::StreamErrorProxy)),
            this, SLOT(onStreamError(Origin::Chat::StreamErrorProxy)));

        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(jingleClose(Origin::Chat::JingleErrorProxy)),
            this, SLOT(onJingleClose(Origin::Chat::JingleErrorProxy)));

        // We want to be handling connections on the main thread
        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(connected()),
            this, SLOT(onConnected()));

        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(contactLimitReached(const buzz::Jid&)),
            this, SIGNAL(mutualContactLimitReached(const buzz::Jid&)));

        ORIGIN_VERIFY_CONNECT(
            mImplEventAdapter, SIGNAL(privacyReceived()),
            this, SIGNAL(privacyReceived()));

        mInitialReconnectDelay = rand() % 5000;
        mReconnectTimer.setSingleShot(true);
        ORIGIN_VERIFY_CONNECT(&mReconnectTimer, SIGNAL(timeout()), this, SLOT(doConnection()));
    }

    Connection::~Connection()
    {
        // write out the Capabilities Cache for Jingle
        std::string capsCacheXml = buzz::CapabilitiesCache::instance().ToXml();

        QString directoryPath;
        QString fullPath;
        GetCapabilitiesCacheLocation(directoryPath, fullPath);
        if (QDir().mkpath(directoryPath))
        {
            QFile capsCache(fullPath);
            if (capsCache.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                ORIGIN_LOG_DEBUG << "Capabilities Cache written: " << capsCacheXml;
                QTextStream out(&capsCache);
                out << capsCacheXml.c_str();
            }
            else
            {
                ORIGIN_LOG_EVENT << "Unable to open Capabilities Cache for writing.";
            }
        }
        else
        {
            ORIGIN_LOG_EVENT << "Unable to create Capabilities Cache path.";
        }
        buzz::CapabilitiesCache::instance().Release();

        ORIGIN_VERIFY_DISCONNECT(&mReconnectTimer, SIGNAL(timeout()), this, SLOT(doConnection()));
        shutdownImplementation();
        delete mCurrentUser;
        delete mJingleSettings;
    }

    void Connection::login(const JabberID &username)
    {    
        mJingleSettings->set_user(username.node().toStdString());
        mJingleSettings->set_resource(username.resource().toStdString());
        mJingleSettings->set_host(mHost.constData());
        mJingleSettings->set_allow_plain(false);

        switch(mConfiguration.tlsDisposition())
        {
        case Configuration::TlsDisabled:
            mJingleSettings->set_use_tls(buzz::TLS_DISABLED);
            break;
        case Configuration::TlsPreferSecure:
            mJingleSettings->set_use_tls(buzz::TLS_ENABLED);
            break;
        case Configuration::TlsRequireSecure:
            mJingleSettings->set_use_tls(buzz::TLS_REQUIRED);
            break;
        }

        // Use the 10.10 server to connect to Tony's dev box for testing
        mJingleSettings->set_server(talk_base::SocketAddress(mHost.constData(), mPort)); 
        //mJingleSettings->set_server(talk_base::SocketAddress("10.10.78.239", 5222));        

        doConnection();

        // Update the connected user's Jabber ID
        ORIGIN_LOG_DEBUG << "*** Jabber ID = " << username.toEncoded();
        mCurrentUser->setJabberId(username);
    }

    void Connection::onStreamError(StreamErrorProxy error)
    {
        if (!error.errorBody.empty())
        {
            mReconnect = QString::fromUtf8(error.errorBody.c_str());
            QTimer::singleShot(0, this, SLOT(doStreamReconnect()));
        }
        else if (error.resourceConflict) //single login conflict
        {
            mEstablished = 0;
            shutdownImplementation();
            emit (disconnected(DisconnectStreamConflict));
        }
    }

    void Connection::onJingleClose(Origin::Chat::JingleErrorProxy error)
    {
        // This slot will get hit when we have an actual disconnect (via error) OR when we go offline (either manually or via loss of internet)
        // (Disconnect due to single login conflict will go thru ::onStreamError instead)
        // if going offline mEstablished will already be set to false, and in that case, we don't want to attempt reconnect
        shutdownImplementation();

        bool wasEstablished = established();
        if (wasEstablished)
        {
            //this needs to be set BEFORE the emit because .js depends on it
            mEstablished = 0;
            emit (disconnected(DisconnectClosedError));
        }

        ORIGIN_LOG_EVENT << "connection closed: established = " << wasEstablished << " reconnectAttempts = " << mReconnectAttempts;

        //we need to restore the host back to the main load balancing server, 'cuz when we got the stream error, we set it to a specific server
        //but no guarantee that that server is still active if we attempt reconnect
        mJingleSettings->set_server(talk_base::SocketAddress(mHost.constData(), mPort));        

        //we need to put the reattempt outside of established since if previous attempt failed, it will not be established()
        //but we also don't want to retry if we're in offline mode (but in that case mReconnectAttempt will be 0)
        if (wasEstablished || //first time loss of connection but mReconnectAttempt would be 0 in that case
            mReconnectAttempts) //failed last time, so keep retrying
        {
            //TODO: see if there's a beter way to distinguish between unable to connect at initial login vs. going offline
            //-1 is an uninitialized state, meaning we failed to connect at initial login (which would have mEstablished = 0)
            //it's a way to distinguish from going offline (client), which would have mEstablished = 0 && mReconnectAttempt = 0, but in that case we don't want to reconnect
            if (mReconnectAttempts == -1)
                mReconnectAttempts = 0; //since mReconnectAttempts gets used to compute back off time, need to initialize it to 0

            doBackoff();
            mReconnectAttempts++;
        }
    }

    void Connection::doBackoff()
    {
        // When we disconnect from an error in Jingle, attempt to reconnect. But this error could have been
        // chat going down closing our connection. We don't want all users to reconnect in lock step
        // use the same logic that's used for retrying token renewal in loginRegistrationSession, but add the inital delay as further randomization

        const qint32 RETRY_THRESHOLD = 10;
        const qint32 BASE_RETRY_DURATION_IN_MSEC = 1000;

        // clamp retry interval
        qint32 maxedRetries = (mReconnectAttempts < RETRY_THRESHOLD) ? mReconnectAttempts : RETRY_THRESHOLD; 

        // exponential fall-back of 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 1024, 1024 seconds....
        qint32 reconnectTime = BASE_RETRY_DURATION_IN_MSEC << maxedRetries;

        // add a random jitter on the order of 5% to keep clients from syncronising
        reconnectTime += (reconnectTime * (rand() % 500)) / 10000;
        //and finally add the initial delay to randomize it even further
        reconnectTime += mInitialReconnectDelay;

        ORIGIN_LOG_EVENT << "Attempting reconnect to chat server in  " << reconnectTime << " milliseconds";
        mReconnectTimer.start(reconnectTime);
    }

    void Connection::doStreamReconnect()
    {
        // see-other-host error, redirect to another server
        mJingleSettings->set_server(talk_base::SocketAddress(mReconnect.toStdString(), 5222));

        shutdownImplementation();
        doConnection();
    }

    void Connection::doConnection()
    {
        ORIGIN_LOG_EVENT << "attempting connection to chat server";

        //should be enough to check for established() but check mJingleThread and mJingle just in case
        if (established() && mJingleThread && mJingle)
        {
            ORIGIN_LOG_EVENT << "connection already established";
            return;
        }

        mJingleThread = new buzz::XmppThread();
        mJingleThread->Start();
        mJingle = new JingleClient(mDiscoverableInformation, mJingleThread, mJingleThread->client());
        if (!mJingle)
            return;
        buzz::CapabilitiesCache::instance().SetClient(*mJingle->GetXmppClient());

        if (Origin::Services::readSetting(Origin::Services::SETTING_LogXmpp))
        {
            mJingleThread->client()->SignalLogInput.connect(&sDebugLog, &DebugLog::Input);
            mJingleThread->client()->SignalLogOutput.connect(&sDebugLog, &DebugLog::Output);
        }

        mJingle->SetEvents(mImplEventAdapter);

        //retrieve the current acessToken
        Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();
        if (!session.isNull())
        {
            Origin::Services::Session::AccessToken currToken = Origin::Services::Session::SessionService::accessToken (session);
            if (!currToken.isEmpty())
            {
                mJingleSettings->set_auth_token("XEP-0078", currToken.toStdString());
            }
        }
        mJingleThread->Login(*mJingleSettings);
    }

    void Connection::close()
    {
        mEstablished = 0;
        shutdownImplementation();
        emit (disconnected(DisconnectExplicitClose));
    }

    void Connection::shutdownImplementation()
    {
        mReconnectTimer.stop();

        if (mJingleThread)
        {
            mJingleThread->Disconnect();
            mJingleThread->Stop();
        }

        delete mJingleThread;
        mJingleThread = NULL;

        delete mJingle;
        mJingle = NULL;
    }

    void Connection::sendMessage(const Message &msg)
    {
        if (!mJingle)
        {
            return;
        }

        mJingle->SendChatMessage(msg.toJingle());

        if (msg.subject().compare("AutoReponse") == 0)
        {
            return;
        }

        // Don't want to tell the conversable about this message if it's empty
        if (!msg.body().isEmpty())
        {
            // Notify any listeners that we're sending a message
            Conversable *conversable = conversableForMessage(msg.to(), msg.type());

            messageSent(conversable, msg);

            if (conversable)
            {
                // Emit the user's signal
                conversable->sentMessage(msg);
            }
        }
    }

    void Connection::sendDirectedPresence(
        const JingleExtendedPresence presence,
        const QString& status,
        const JabberID& jid,
        const QString& body)
    {
        if (!mJingle)
        {
            return;
        }

        mJingle->SendDirectedPresence(presence, status, jid.toJingle(), body);
    }

    void Connection::chatGroupRoomInvite(const Chat::JabberID& toJid, const QString& roomId, const QString& channelId)
    {
        mJingle->ChatGroupRoomInvite(toJid.toJingle(), roomId, channelId);
    }

    void Connection::installIncomingMessageFilter(MessageFilter *filter)
    {
        mIncomingMessageFilters.append(filter);
    }

    void Connection::removeIncomingMessageFilter(MessageFilter *filter)
    {
        mIncomingMessageFilters.removeAll(filter);
    }
        
    bool Connection::filterIncomingMessage(const Message &incoming) const
    {
        // Pass this message through our filters
        foreach(MessageFilter *filter, mIncomingMessageFilters)
        {
            if (filter->filterMessage(incoming))
            {
                // The filter has claimed this message
                return true;
            }
        }
        
        return false;
    }

    void Connection::onConnected()
    {
        mReconnectTimer.stop();
        mEstablished = 1;
        emit connected();
        mReconnectAttempts = 0;
    }

    bool Connection::established() const
    {
        return mEstablished.load() != 0;
    }
        
    RemoteUser* Connection::remoteUserForJabberId(const JabberID &jid)
    {
        const JabberID bareJid = jid.asBare();

#ifdef _DEBUG
        {
            QMutexLocker locker(&mMucRoomsLock);

            if (mMucRooms.contains(bareJid))
            {
                // We tried to look up a MUC room as a remote user
                // Bad!
                ORIGIN_ASSERT(0);
                return NULL;
            }
        }
#endif

        {
            QMutexLocker locker(&mRemoteUsersLock);

            QHash<JabberID, RemoteUser *>::ConstIterator existingUserIt = mRemoteUsers.find(bareJid);

            if (existingUserIt != mRemoteUsers.constEnd())
            {
                return *existingUserIt;
            }

            RemoteUser *newUser = new RemoteUser(bareJid, this, this);
            mRemoteUsers.insert(bareJid, newUser);

            return newUser;
        }
    }
        
    Conversable* Connection::conversableForMessage(const JabberID &jid, const QString &type)
    {
        if (type == "groupchat")
        {
            QMutexLocker locker(&mMucRoomsLock);
            return mMucRooms.value(jid.asBare());
        }

        return remoteUserForJabberId(jid);
    }
        
    XMPPUser* Connection::userForJabberId(const JabberID &jid)
    {
        if (jid.asBare() == mCurrentUser->jabberId().asBare())
        {
            return mCurrentUser;
        }

        return remoteUserForJabberId(jid);
    }
        
    MucRoom* Connection::mucRoomForJabberId(const JabberID &jid)
    {
        QMutexLocker locker(&mMucRoomsLock);
        return mMucRooms.value(jid.asBare());
    }
        
    JabberID Connection::roomJabberIdForRoomId(const QString &roomId, const QString &roomNickname) const
    {
        // XXX: Our server doesn't support chat service discovery with the XMPP package
        const QString multiUserChatDomain = QString::fromLatin1("muc." + mHost);
        return JabberID(roomId, multiUserChatDomain, roomNickname);
    }

    void Connection::joinMucRoom(MucRoom* room, const QString& roomId, const QString& password, const QString& roomName, const bool leaveRoomOnJoin, const bool failSilently)
    {
        emit (joiningMucRoom(room, roomId, password, roomName, leaveRoomOnJoin, failSilently));
    }

    void Connection::rejoinMucRoom(MucRoom* room, const QString& roomId)
    {
        emit (rejoiningMucRoom(room, roomId));
    }

    void Connection::leaveMucRoom(const QString &roomId, const QString &nickname)
    {
        const JabberID roomJabberId = roomJabberIdForRoomId(roomId, nickname);

        mJingle->MUCLeaveRoom(roomJabberId.toJingle());
    }

    // REMOVE FOR ORIGIN X
    //void Connection::leaveMucRoom(Origin::Chat::MucRoom* room)
    //{
    //    mJingle->MUCLeaveRoom(room->jabberId().toJingle());
    //}

    void Connection::kickMucUser(const QString& roomId, const QString& userNickname, const QString& by)
    {
        const JabberID roomJabberId = roomJabberIdForRoomId(roomId);

        mJingle->MUCKickOccupant(roomJabberId.asBare().toJingle(), userNickname, by);
    }

    EnterMucRoomOperation* Connection::enterMucRoom(MucRoom* room, const QString &roomId, const QString &nickname, const QString &password, const QString &roomName, bool silentFailure)
    {
        const JabberID roomJabberId = roomJabberIdForRoomId(roomId, nickname);

        mJingle->MUCEnterRoom(nickname, roomJabberId.asBare().toJingle(), password, roomName);
        
        // Create a high level operation for the caller to use
        EnterMucRoomOperation *roomOp = new EnterMucRoomOperation(room, roomJabberId, silentFailure, mReconnect);
        roomOp->startRequestTimer();

        ORIGIN_VERIFY_CONNECT(roomOp, SIGNAL(finished()), this, SLOT(enteringMucRoomOpFinished()));
        
        // Track this for when we get a "room entered" event
        {
            QMutexLocker locker(&mEnteringMucRoomsLock);
            mEnteringMucRooms.insert(roomJabberId.asBare(), roomOp);
        }

        emit (enteringMucRoom(roomId, roomOp));
        return roomOp;
    }

    DestroyMucRoomOperation* Connection::destroyMucRoom(MucRoom* room, const QString& roomId, const QString& nickName, const QString& nucleusId, const QString& password, const QString& roomName)
    {
        const JabberID roomJabberId = roomJabberIdForRoomId(roomId);
        mJingle->MUCDestroyRoom(nickName, nucleusId, roomJabberId.asBare().toJingle(), password, roomName);

        // Create a high level operation for the caller to use
        DestroyMucRoomOperation *roomOp = new DestroyMucRoomOperation(room, roomJabberId.asBare());
        ORIGIN_VERIFY_CONNECT(roomOp, SIGNAL(finished()), this, SLOT(destroyingMucRoomOpFinished()));

        // Track this for when we get a "room entered" event
        {
            QMutexLocker locker(&mDestroyingMucRoomsLock);
            mDestroyingMucRooms.insert(roomJabberId.asBare(), roomOp);
        }

        emit (destroyingMucRoom(roomId, roomOp));
        return roomOp;
    }

    void Connection::enteringMucRoomOpFinished()
    {
        EnterMucRoomOperation *op = dynamic_cast<EnterMucRoomOperation*>(sender());

        if (op)
        {
            op->timer().disconnect(SIGNAL(timeout()));

            QMutexLocker locker(&mEnteringMucRoomsLock);
            mEnteringMucRooms.remove(op->jabberId());

            op->deleteLater();
        }
    }

    void Connection::destroyingMucRoomOpFinished()
    {
        DestroyMucRoomOperation *op = dynamic_cast<DestroyMucRoomOperation*>(sender());

        if (op)
        {
            QMutexLocker locker(&mDestroyingMucRoomsLock);
            mDestroyingMucRooms.remove(op->jabberId());

            op->deleteLater();
        }
    }

    void Connection::mucRoomExited()
    {
        MucRoom *room = dynamic_cast<MucRoom*>(sender());

        if (room)
        {
            QMutexLocker locker(&mMucRoomsLock);
            mMucRooms.remove(room->jabberId());

            ORIGIN_VERIFY_DISCONNECT(room, SIGNAL(exited()), this, SLOT(mucRoomExited()));
        }
    }

    void Connection::jingleChatStateReceived(const JingleMessage &msg)
    {
        Message incoming = Message::fromJingle(msg);

        // Find the user
        XMPPUser *user = userForJabberId(incoming.from());

        emit chatStateReceived(incoming);

        if (user)
        {
            // Have the conversable emit its signal
            user->receivedChatState(incoming);
        }
    }

    void Connection::jingleMessageReceived(const JingleMessage &msg)
    {
        Message incoming = Message::fromJingle(msg);

        if (filterIncomingMessage(incoming))
        {
            return;
        }
        
        if (incoming.type() == "error")
        {
            ORIGIN_LOG_WARNING << "Received error message from " << QString::fromUtf8(incoming.from().toEncoded());
            return;
        }
        else if (incoming.type() == "chat")
        {
            // Find the user
            XMPPUser *user = userForJabberId(incoming.from());

            // If this is not from a user, then it's from a room. Find the MUC room

            emit messageReceived(user, incoming); // send to js
            
            if (user)
            {
                // Have the conversable emit its signal
                user->receivedMessage(incoming);
            }
        }
        else if (incoming.type() == "groupchat")
        {
            // Find the chat room
            MucRoom *room;

            {
                QMutexLocker locker(&mMucRoomsLock);
                // The node/domain part of the "from" identify the room
                room = mMucRooms.value(incoming.from().asBare());
            }

            if (!room)
            {
                ORIGIN_LOG_WARNING << "Received multi-user chat message from unknown room";
                return;
            }

            // Figure out who this actually was from using their nickname
            const QString nickname = incoming.from().resource();

            if (!nickname.isEmpty())
            {
                // This is from a specific user in the room
                JabberID originalJid = room->jabberIdForNickname(nickname);

                if (!originalJid.isValid())
                {
                    ORIGIN_LOG_WARNING << "Received multi-user chat message from unknown nickname " << nickname;
                    return;
                }

                // Rewrite the message with the actual sender Jabber ID
                // The room information is still available with the Conversable pointer we emit
                Message incomingMuc = incoming.withFrom(originalJid);

                if (filterIncomingMessage(incomingMuc))
                {
                    return;
                }

                emit messageReceived(room, incoming); // send to js
                room->receivedMessage(incomingMuc);
            }
            else
            {
                // This is a system message from the room itself
                if (!filterIncomingMessage(incoming))
                {
                    room->receivedSystemMessage(incoming);
                }
            }
        }
    }

    void Connection::presenceUpdate(Origin::Chat::XmppPresenceProxy presence)
    {
        JabberID presenceJid = JabberID::fromJingle(presence.jid).asBare();
        RemoteUser* user = remoteUserForJabberId(presenceJid);
        if (!user)
            return;
        user->setPresenceFromJingle(presence); 
    }

    const UserChatCapabilities Connection::capabilities(const RemoteUser& user) const
    {
        QString jingleCaps(QString::fromStdString(buzz::CapabilitiesCache::instance().CapabilitiesForUser(user.jabberId().toJingle())));
        QStringList caps(jingleCaps.split(","));
        return caps;
    }

    void Connection::onCapabilitiesChanged(Origin::Chat::XmppCapabilitiesProxy capsProxy)
    {
        JabberID jid = JabberID::fromJingle(capsProxy.from).asBare();
        RemoteUser* user = remoteUserForJabberId(jid);
        if (!user)
            return;
        // we don't use signaling to avoid sending signals to every RemoteUser
        user->onCapabilitiesChanged(); 
    }

    void Connection::addRequest(buzz::Jid from)
    {
        const JabberID jid = JabberID::fromJingle(from).asBare();
        RemoteUser* user = remoteUserForJabberId(jid);

        user->requestedSubscription();
    }

    void Connection::roomEntered(XmppChatroomModuleProxy room, QString role)
    {
        const JabberID roomJid = JabberID::fromJingle(room.chatroom_jid);
        EnterMucRoomOperation *roomOp;

        {
            QMutexLocker locker(&mEnteringMucRoomsLock);
            roomOp = mEnteringMucRooms.value(roomJid);
        }

        if (!roomOp)
        {
            // We entered a room we didn't know we were entering
            // Assume this is a MucRoom automatically reconnecting

            MucRoom *mucRoom;

            {
                QMutexLocker locker(&mMucRoomsLock);
                mucRoom = mMucRooms.value(roomJid);
            }

            // This shouldn't happen
            ORIGIN_ASSERT(mucRoom);

            return;
        }
        
        // Create the new room and populate it
        MucRoom *mucRoom = roomOp->room();
        if( mucRoom )
        {
            mucRoom->setNickname(QString::fromStdString(room.nickname));
            mucRoom->setJabberId(roomJid);
            mucRoom->setRole(role);

            {
                // Track the room
                QMutexLocker locker(&mMucRoomsLock);
                mMucRooms.insert(roomJid, mucRoom);
            }

            ORIGIN_VERIFY_CONNECT(mucRoom, SIGNAL(exited()), this, SLOT(mucRoomExited()));
        }

        // Notify anyone listening
        roomOp->roomEntered(mucRoom);
    }

    void Connection::roomEnterFailed(Origin::Chat::XmppChatroomModuleProxy room, buzz::XmppChatroomEnteredStatus status)
    {
        const JabberID roomJid = JabberID::fromJingle(room.chatroom_jid);
        EnterMucRoomOperation *roomOp;

        {
            QMutexLocker locker(&mEnteringMucRoomsLock);
            roomOp = mEnteringMucRooms.value(roomJid);
        }

        if (!roomOp)
        {
            // Shouldn't fail to enter a room we didn't want to enter
            ORIGIN_ASSERT(0);
            return;
        }
        ORIGIN_LOG_ERROR<<"Connection::roomEnterFailed status: "<< status;
        roomOp->roomNotEntered(static_cast<ChatroomEnteredStatus>(status));
    }

    void Connection::roomPasswordIncorrect(Origin::Chat::XmppChatroomModuleProxy room)
    {
        const JabberID roomJid = JabberID::fromJingle(room.chatroom_jid);
        if (room.destroyRoom)
        {
            DestroyMucRoomOperation *roomOp;

            {
                QMutexLocker locker(&mDestroyingMucRoomsLock);
                roomOp = mDestroyingMucRooms.value(roomJid);
            }

            if (!roomOp)
            {
                // Shouldn't fail to enter a room we didn't want to enter
                ORIGIN_ASSERT(0);
                return;
            }
            roomOp->roomNotEntered();
        }
        else
        {
            EnterMucRoomOperation *roomOp;

            {
                QMutexLocker locker(&mEnteringMucRoomsLock);
                roomOp = mEnteringMucRooms.value(roomJid);
            }

            if (!roomOp)
            {
                // Shouldn't fail to enter a room we didn't want to enter
                ORIGIN_ASSERT(0);
                return;
            }
            roomOp->roomNotEntered();
        }
    }

    void Connection::roomUserJoined(Origin::Chat::XmppChatroomMemberProxy member)
    {
        const JabberID bareRoomJid = JabberID::fromJingle(member.member_jid).asBare();
        MucRoom *mucRoom;

        {
            QMutexLocker locker(&mMucRoomsLock);
            mucRoom = mMucRooms.value(bareRoomJid);
            if (!mucRoom)
            {
                // When we enter a room we get info from other users before ourselves
                EnterMucRoomOperation* roomOp = mEnteringMucRooms.value(bareRoomJid);
                if (roomOp)
                {
                    mucRoom = roomOp->room();
                }                
            }
        }

        if (!mucRoom)
        {
            // Don't care
            return;
        }

        mucRoom->addParticipant(member);

        // ORIGIN X
        const JabberID fromJid = JabberID::fromJingle(member.full_jid).withResource(QString::fromStdString(member.name));
        emit userJoined(fromJid, mucRoom);
    }

    void Connection::roomUserLeft(Origin::Chat::XmppChatroomMemberProxy member)
    {
        const JabberID bareRoomJid = JabberID::fromJingle(member.member_jid).asBare();
        MucRoom *mucRoom;

        {
            QMutexLocker locker(&mMucRoomsLock);
            mucRoom = mMucRooms.value(bareRoomJid);
        }

        if (!mucRoom)
        {
            // Don't care
            return;
        }

        mucRoom->removeParticipant(member);

        // ORIGIN X
        const JabberID fromJid = JabberID::fromJingle(member.full_jid).withResource(QString::fromStdString(member.name));
        emit userLeft(fromJid, mucRoom);
    }

    void Connection::roomUserChanged(Origin::Chat::XmppChatroomMemberProxy member)
    {
        const JabberID bareRoomJid = JabberID::fromJingle(member.member_jid).asBare();
        MucRoom *mucRoom;

        {
            QMutexLocker locker(&mMucRoomsLock);
            mucRoom = mMucRooms.value(bareRoomJid);
        }

        if (!mucRoom)
        {
            // Don't care
            return;
        }

        mucRoom->changeParticipant(member);
    }
        
    void Connection::roomDeclineReceived(buzz::Jid decliner, buzz::Jid room, QString reason)
    {
        const JabberID invitee = JabberID::fromJingle(decliner);
        const JabberID bareRoomJid = JabberID::fromJingle(room).asBare();
        MucRoom *mucRoom;

        {
            QMutexLocker locker(&mMucRoomsLock);
            mucRoom = mMucRooms.value(bareRoomJid);
        }

        if (mucRoom)
        {
            mucRoom->inviteDeclined(invitee, reason);
        }
    }

    void Connection::chatGroupRoomInviteReceived(buzz::Jid fromJid, QString groupId, QString channelId)
    {
        const JabberID from = JabberID::fromJingle(fromJid);
        emit groupRoomInviteReceived(from.node(), groupId, channelId);
    }

    void Connection::roomDestroyed(XmppChatroomModuleProxy room)
    {
        const JabberID roomJid = JabberID::fromJingle(room.chatroom_jid);
        const JabberID bareRoomJid = roomJid.asBare();

        {
            MucRoom *mucRoom;
            {
                QMutexLocker locker(&mMucRoomsLock);
                mucRoom = mMucRooms.value(bareRoomJid);
            }
            if (mucRoom) // the user is in the room
            {
                MucRoom::DeactivationType type = (room.destroyRoom || room.destroyRoomReceiver) ? MucRoom::RoomDestroyedType : MucRoom::GroupDestroyedType;

                if( type == MucRoom::GroupDestroyedType )
                {
                    // extract the nucleus id of the room owner
                    QString destroyedBy = QString::fromStdString(room.reason);
                    room.reason = destroyedBy.remove("group destroyed by ").toStdString();
                }

                mucRoom->setDeactivated(type, room.reason.c_str());

                // This was triggering the notification for a room deletion
                // We no longer want this because "rooms" don't exists anymore
                //emit roomDestroyedBy(mucRoom, room.reason.c_str());
            }
        }

        if (room.destroyRoom) // user manually destroyed room
        {
            // clean up the DestroyMucRoomOperation
            {
                DestroyMucRoomOperation *roomOp;

                {
                    QMutexLocker locker(&mDestroyingMucRoomsLock);
                    roomOp = mDestroyingMucRooms.value(roomJid);
                }

                if (!roomOp)
                {
                    // Shouldn't fail to delete a room we didn't want to delete
                    ORIGIN_ASSERT(0);
                    return;
                }
        
                roomOp->roomDestroyed();
            }
        }

        // clean up the room
        emit roomDestroyed(room.chatroom_jid.node.c_str());
    }

    void Connection::roomKicked(XmppChatroomModuleProxy room)
    {
        const JabberID roomJid = JabberID::fromJingle(room.chatroom_jid);
        const JabberID bareRoomJid = roomJid.asBare();

        // we get "kicked by (nucleusId)" if the user was removed from the group and was in this room
        std::size_t found = room.reason.find("kicked by ");
        MucRoom::DeactivationType type = (found) ? MucRoom::RoomKickedType : MucRoom::GroupKickedType;

        {
            MucRoom *mucRoom;

            {
                QMutexLocker locker(&mMucRoomsLock);
                mucRoom = mMucRooms.value(bareRoomJid);
            }
            if (mucRoom) // the user is in the room
            {
                if( type == MucRoom::GroupKickedType )
                {
                    // extract the nucleus id of the room owner
                    QString kickedBy = QString::fromStdString(room.reason);
                    room.reason = kickedBy.remove("kicked by ").toStdString();
                }

                mucRoom->setDeactivated(type, room.reason.c_str());
                if (type != MucRoom::GroupKickedType)
                    emit kickedFromRoom(mucRoom, room.reason.c_str());
            }
        }

        // clean up the group
        if (type == MucRoom::GroupKickedType)
        {
            emit groupKicked(room.chatroom_jid.node.c_str());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    void EnterMucRoomOperation::roomEntered(MucRoom *room)
    {
        emit succeeded(room);
        // This will cause Connection to clean us up
        emit finished();
    }

    void EnterMucRoomOperation::roomNotEntered(Origin::Chat::ChatroomEnteredStatus status)
    {
        emit failed(status);
        // This will cause Connection to clean us up
        emit finished();
    }

    void EnterMucRoomOperation::startRequestTimer()
    {
        // Set up a timer incase we never get anything back from the server
        ORIGIN_VERIFY_CONNECT(&mEnterRequestTimer, SIGNAL(timeout()), this, SIGNAL(enterRoomRequestTimedout()));
        ORIGIN_VERIFY_CONNECT(&mEnterRequestTimer, SIGNAL(timeout()), this, SIGNAL(finished()));
        mEnterRequestTimer.start(5000);
    }

    ////////////////////////////////////////////////////////////////////////////////
    void DestroyMucRoomOperation::roomNotEntered()
    {
        emit failed();
        // This will cause Connection to clean us up
        emit finished();
    }

    void DestroyMucRoomOperation::roomDestroyed()
    {
        emit succeeded();
        // This will cause Connection to clean us up
        emit finished();
    }
}
}

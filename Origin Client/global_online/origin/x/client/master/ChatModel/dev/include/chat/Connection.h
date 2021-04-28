#ifndef _CHATMODEL_CONNECTION_H
#define _CHATMODEL_CONNECTION_H

#include <QByteArray>
#include <QObject>
#include <QList>
#include <QMutex>
#include <QAtomicInt>
#include <qtimer.h>

#include "JabberID.h"
#include "Message.h"
#include "Configuration.h"
#include "XMPPImplTypes.h"
#include "DiscoverableInformation.h"
#include "talk/xmpp/chatroommodule.h"

class QThread;

namespace Origin
{
///
/// Namespace for core chat-related functionality
///
/// Chat is defined as core XMPP support plus Origin specific extensions and enhancements related to chat. Chat is a
/// subset of social which encompasses non-chat functionality such as profile pages.
/// 
namespace Chat
{
    class ConnectedUser;
    class Connection;
    class Conversable;
    class RemoteUser;
    class XMPPUser;
    class XMPPImplEventAdapter;
    class XMPPImplEventPump;
    class MessageFilter;
    class MucRoom;
    struct XmppPresenceProxy;
    struct XmppChatroomModuleProxy;
    struct XmppChatroomMemberProxy;
    struct StreamErrorProxy;
    struct JingleErrorProxy;

    //! Status codes for ChatroomEnteredStatus callback
    enum ChatroomEnteredStatus
    {
        //! User successfully entered the room
        CHATROOM_ENTERED_SUCCESS                    = 0,
        //! The nickname conflicted with somebody already in the room
        CHATROOM_ENTERED_FAILURE_NICKNAME_CONFLICT  = 1,
        //! A password is required to enter the room
        CHATROOM_ENTERED_FAILURE_PASSWORD_REQUIRED  = 2,
        //! The specified password was incorrect
        CHATROOM_ENTERED_FAILURE_PASSWORD_INCORRECT = 3,
        //! The user is not a member of a member-only room
        CHATROOM_ENTERED_FAILURE_NOT_A_MEMBER       = 4,
        //! The user cannot enter because the user has been banned
        CHATROOM_ENTERED_FAILURE_MEMBER_BANNED      = 5,
        //! The room has the maximum number of users already
        CHATROOM_ENTERED_FAILURE_MAX_USERS          = 6,
        //! The room has been locked by an administrator
        CHATROOM_ENTERED_FAILURE_ROOM_LOCKED        = 7,
        //! Someone in the room has blocked you
        CHATROOM_ENTERED_FAILURE_MEMBER_BLOCKED     = 8,
        //! You have blocked someone in the room
        CHATROOM_ENTERED_FAILURE_MEMBER_BLOCKING    = 9,
        //! Client is old. User must upgrade to a more recent version for
        // hangouts to work.
        CHATROOM_ENTERED_FAILURE_OUTDATED_CLIENT    = 10,
        // Group or room could not be found on the server
        CHATROOM_ENTERED_FAILURE_GROUP_NOT_FOUND    = 11,
        // We are on a bad node and server was unresponsive
        CHATROOM_ENTERED_FAILURE_SERVER_REQUEST_TIMEOUT = 12,
        //! Some other reason
        CHATROOM_ENTERED_FAILURE_UNSPECIFIED        = 2000,
    };

    ///
    /// Tracks an operation to enter a multi-user chat room
    ///
    /// These objects are deleted by the event tracking framework. For this reason it is not safe to dereference an
    /// EnterMucRoomOperation
    ///
    class EnterMucRoomOperation : public QObject
    {
        friend class Connection;

        Q_OBJECT
    signals:
        ///
        /// Emitted if the room is successfully entered
        ///
        /// This is emitted before finished()
        /// 
        void succeeded(Origin::Chat::MucRoom *);

        ///
        /// Emitted if we fail to enter to room
        ///
        /// This is emitted before finished()
        ///
        void failed(Origin::Chat::ChatroomEnteredStatus status=CHATROOM_ENTERED_SUCCESS);

        ///
        /// Emitted once the operation finishes
        ///
        /// This is emitted after succeeded() or failed()
        ///
        void finished();

        ///
        /// Emitted 5 seconds after the inital request to enter a room
        void enterRoomRequestTimedout(Origin::Chat::ChatroomEnteredStatus status=CHATROOM_ENTERED_FAILURE_SERVER_REQUEST_TIMEOUT);

    private slots:
        void roomEntered(MucRoom *);
        void roomNotEntered(Origin::Chat::ChatroomEnteredStatus status=CHATROOM_ENTERED_SUCCESS);

    private:
        explicit EnterMucRoomOperation(MucRoom* room, const JabberID &jid, bool silentFailure, const QString& node) 
            : mRoom(room)
            , mJabberId(jid)
            , mSilentFailure(silentFailure)
            , mXMPPNode(node)
        {
             mEnterRequestTimer.setSingleShot(true);
        }
    public:
        JabberID jabberId() const
        {
            return mJabberId;
        }

        MucRoom* room() const
        {
            return mRoom;
        }

        bool isSilentFailure()
        {
            return mSilentFailure;
        }
        
        QTimer& timer()
        {
            return mEnterRequestTimer;
        }

        const QString& xmppNode() const
        {
            return mXMPPNode;
        }

        void startRequestTimer();
    private:
        MucRoom* mRoom;
        JabberID mJabberId;
        bool mSilentFailure;
        QTimer mEnterRequestTimer;
        QString mXMPPNode;
    };

    const QString CHATCAP_CAPS("http://jabber.org/protocol/caps");
    const QString CHATCAP_CHATSTATES("http://jabber.org/protocol/chatstates");
    const QString CHATCAP_DISCOINFO("http://jabber.org/protocol/disco#info");
    const QString CHATCAP_MUC("http://jabber.org/protocol/muc");
#if ENABLE_VOICE_CHAT
    const QString CHATCAP_VOIP("http://jabber.org/protocol/voip");
#endif

    ///
    /// Tracks an operation to destroy a multi-user chat room
    ///
    /// These objects are deleted by the event tracking framework. For this reason it is not safe to dereference an
    /// DestroyMucRoomOperation
    ///
    class DestroyMucRoomOperation : public QObject
    {
        friend class Connection;

        Q_OBJECT
    signals:
        ///
        /// Emitted if the room is successfully destroyed
        ///
        /// This is emitted before finished()
        /// 
        void succeeded();

        ///
        /// Emitted if we fail to destroy the room
        ///
        /// This is emitted before finished()
        ///
        void failed();

        ///
        /// Emitted once the operation finishes
        ///
        /// This is emitted after succeeded() or failed()
        ///
        void finished();

        private slots:
            void roomNotEntered();
            void roomDestroyed();

    private:
        explicit DestroyMucRoomOperation(MucRoom* room, const JabberID &jid) 
            : mRoom(room)
            , mJabberId(jid)
        {
        }

        JabberID jabberId() const
        {
            return mJabberId;
        }

        MucRoom* room() const
        {
            return mRoom;
        }

        MucRoom* mRoom;
        JabberID mJabberId;
    };

    ///
    /// Represents a connection to an XMPP server
    ///
    /// All core XMPP functionality is exposed by this class or its children. In particular currentUser() is the root
    /// most chat related functions.
    ///
    class Connection : public QObject
    {
        friend class ConnectedUser;
        friend class Roster;
        friend class RemoteUser;
        friend class BlockList;
        friend class MucRoom;

        Q_OBJECT
    public:
        ///
        /// Reason for a server disconnection
        ///
        enum DisconnectReason
        {
            ///
            /// Connection was explicitly closed by calling Connection::close()
            ///
            DisconnectExplicitClose,

            ///
            /// A new connection to the server was established with the same Jabber ID and resource
            ///
            DisconnectStreamConflict,

            ///
            /// Connection was closed for an error
            ///
            DisconnectClosedError,

            ///
            /// Unknown reason such as network or server failure
            /// 
            DisconnectUnknown
        };

        ///
        /// Creates a new Connection instance
        ///
        /// This won't establish a connection to the XMPP server. login() should be called to provide the user's
        /// credentials first.
        ///
        /// \param host           Hostname of the XMPP server
        /// \param port           Port of the XMPP server
        /// \param config         Chat::Configuration to use
        ///
        Connection(const QByteArray &host, int port = 5222, const Configuration &config = Configuration());
        virtual ~Connection();

        ///
        /// Connects to the XMPP server and logs in as the given user 
        ///
        /// \param username  Username to log in with
        ///
        void login(const JabberID &username);

        ///
        /// Closes the connection to the XMPP server
        ///
        /// This will emit disconnected(). The connection can be re-established by calling login()
        ///
        void close();
        
        ///
        /// Sends a message over the connection asynchronously
        ///
        /// \param  message  Message to send
        ///
        /// \sa Conversable::sendMessage
        ///
        void sendMessage(const Message &message);

        /// 
        /// Gets the current user associated with this connection
        ///
        ConnectedUser* currentUser()
        {
            return mCurrentUser;
        }

        ///
        /// Returns the configuration this connection was constructed with
        ///
        Configuration configuration() const    
        {
            return mConfiguration;
        }

        QString host() const
        {
            return mHost;
        }

        ///
        /// Installs a message filter
        ///
        /// Message filters can process and intercept messages before we emit messageReceived()
        ///
        void installIncomingMessageFilter(MessageFilter *);

        ///
        /// Removes a previously installed message filter
        ///
        void removeIncomingMessageFilter(MessageFilter *);

        ///
        /// Returns the RemoteUser instance for the passed Jabber ID
        ///
        /// If an instance doesn't exist it will be created
        ///
        RemoteUser *remoteUserForJabberId(const JabberID &jid);
        
        ///
        /// Returns the user for the passed Jabber ID
        ///
        /// This is the same as remoteUserForJabberId except it will return the connected user if their Jabber ID is
        /// passed
        ///
        XMPPUser *userForJabberId(const JabberID &jid);

        ///
        /// Returns the MucRoom instances for the passed room Jabber ID
        ///
        /// IF an instance doesn't exist NULL will be returned
        ///
        MucRoom *mucRoomForJabberId(const JabberID &jid);

        ///
        /// Returns if we are connected to the server and logged in
        ///
        /// All server operation will fail if the connection is not established
        ///
        /// \sa connected()
        /// \sa disconnected()
        ///
        bool established() const;

        ///
        /// Enters the multi-user chat room with the given room ID
        ///
        /// If the room doesn't exist one will be created using the supplied name and password
        ///
        /// \param  room            Room for this to be contained in
        /// \param  roomId          Room ID to either enter or create
        /// \param  nickname        Nickname to use inside the multi-user chat room
        /// \param  password        Password for the room
        /// \param  roomName        Name for the room
        ///
        /// \sa Room::generateUniqueRoomId()
        ///
        EnterMucRoomOperation *enterMucRoom(MucRoom* room, const QString &roomId, const QString &nickname, const QString &password = "", const QString &roomName = "", bool silentFailure = false);

        ///
        /// Leaves the multi-user chat room with the given room ID
        ///
        /// \param  roomId          Room ID to either enter or create
        /// \param  nickname        Nickname to use inside the multi-user chat room
        ///
        void leaveMucRoom(const QString &roomId, const QString &nickname);

        ///
        /// Destroys the multi-user chat room with the given room ID
        /// NOTE: A chat room can not be destroyed unless the owner is in the room.
        ///
        /// \param  room            Room to destroy
        /// \param  roomId          Room ID to destroy
        /// \param  nickName        Nick name of user destroying the room
        /// \param  nucleusId       Nucleus Id of user destroying the room
        /// \param  password        Password for entering the room before destroying (if not already in the room)
        ///
        DestroyMucRoomOperation *destroyMucRoom(MucRoom* room, const QString& roomId, const QString& nickName, const QString& nucleusId, const QString& password = "", const QString& roomName = "");

        void joinMucRoom(MucRoom* room, const QString& roomId, const QString& password = "", const QString& roomName = "", const bool leaveRoomOnJoin = false, const bool failSilently = false);

        void rejoinMucRoom(MucRoom* room, const QString& roomId);

        ///
        /// Kicks a user from a room with the given id
        ///
        /// \param  roomId          Room to remove user from
        /// \param  userNickname    User to remove from room
        /// \param  by              The Origin Id of the user who is kicking
        ///
        void kickMucUser(const QString& roomId, const QString& userNickname, const QString& by);

        void renameMucRoom(const QString& roomId, const QString& roomName);

        void sendDirectedPresence(
            const JingleExtendedPresence presence,
            const QString& status,
            const JabberID& jid,
            const QString& body);
        
        void chatGroupRoomInvite(const Chat::JabberID& toJid, const QString& roomId, const QString& channelId);

        ///
        /// Returns the information this client will respond with when queried with a disco#info
        ///
        /// This will be pre-populated with a generic identity and a list of ChatModel supported features.
        ///
        /// This is defined in XEP-0030
        ///
        DiscoverableInformation discoverableInformation() const
        {
            return mDiscoverableInformation;
        }
        
        ///
        /// Sets the discoverable information for this cleint
        ///
        /// If an invalid DiscoverableInformation is set then disco#info queries will be ignored
        ///
        void setDiscoverableInformation(const DiscoverableInformation &discoInfo)
        {
            mDiscoverableInformation = discoInfo;
        }

        /// \brief Look up the XMPP capabilities of the given user
        const UserChatCapabilities capabilities(const RemoteUser& user) const;

    signals:
        ///
        /// Emitted once login successfully completes or after we've automatically reconnected
        ///
        /// \sa disconnected()
        /// \sa established()
        ///
        void connected();

        ///
        /// Emitted when the connection is disconnected
        ///
        /// \sa connected()
        /// \sa established()
        ///
        void disconnected(Origin::Chat::Connection::DisconnectReason);

        ///
        /// Emitted when a chatState is received on the connection
        ///
        /// This is only emitted if all installed Chat::MessageFilter accept the message
        ///
        /// \param  message      chatState being received
        ///
        /// \sa Chat::Conversable::chatStateReceived
        ///
        void chatStateReceived(const Origin::Chat::Message &message);

        ///
        /// Emitted when a message is received on the connection
        ///
        /// This is only emitted if all installed Chat::MessageFilter accept the message
        ///
        /// \param  conversable  Conversable the message was received from. This may be NULL for multi-user-chat
        ///                      messages received from unknown rooms
        /// \param  message      Message being received
        ///
        /// \sa Chat::Conversable::messageReceived
        ///
        void messageReceived(Origin::Chat::Conversable *conversable, const Origin::Chat::Message &message);
        
        ///
        /// Emitted when a message is sent on the connection
        ///
        /// \param  conversable  Conversable the message was received from. This may be NULL for multi-user-chat
        ///                      messages sent to unknown rooms
        /// \param  message      Message being sent
        ///
        /// \sa Chat::Conversable::messageSent
        ///
        void messageSent(Origin::Chat::Conversable *conversable, const Origin::Chat::Message &message);

        ///
        /// Emitted when a user selects to join a multi-user chat room
        ///
        void joiningMucRoom(Origin::Chat::MucRoom* room, const QString& roomId, const QString& password, const QString& roomName, const bool leaveRoomOnJoin, const bool failSilently);

        ///
        /// Emitted when a user selects to rejoin a multi-user chat room
        ///
        void rejoiningMucRoom(Origin::Chat::MucRoom* room, const QString& roomId);

        ///
        /// Emitted when the connected user attempts to enter a multi-user chat room
        ///
        /// \param  roomId  Room ID being entered
        /// \param  op      EnterMucRoomOperation tracking the progress of entering the multi-user chat room
        ///
        void enteringMucRoom(const QString &roomId, Origin::Chat::EnterMucRoomOperation *op);

        ///
        /// Emitted when the connected user attempts to destroy a multi-user chat room
        ///
        /// \param  roomId  Room ID being entered
        /// \param  op      DestroyMucRoomOperation tracking the progress of deleting the multi-user chat room
        ///
        void destroyingMucRoom(const QString &roomId, Origin::Chat::DestroyMucRoomOperation *op);

        ///
        /// Emitted whenever the server-side mutual contact limit is reached
        ///
        /// Due to server limitations this is not associated with a specific XMPP action. Instead, the action causing
        /// the limit to be reached will silently fail and this signal will be emitted.
        ///
        void mutualContactLimitReached(const buzz::Jid& from);
        
        ///
        /// Emitted after a multi-user chat room has been destroyed on the server
        ///
        void roomDestroyed(const QString& roomId);

        ///
        /// Emitted to notify user of room deletion
        ///
        void roomDestroyedBy(Origin::Chat::MucRoom* mucRoom, const QString &reason);

        ///
        /// Emitted to notify user has been kicked out of a multi-user chat room
        ///
        void kickedFromRoom(Origin::Chat::MucRoom* mucRoom, const QString &reason);

        ///
        /// Emitted after user has been kicked out of a multi-user chat room
        void roomKicked(const QString& roomId);

        ///
        /// Emitted after user has been kicked out of a group
        void groupKicked(const QString& roomId);

        ///
        /// Emitted after an unsuccessful attempt to enter a muc room
        void enterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus);

        ///
        /// Emitted after we get conformation of invisible status from the server
        void privacyReceived();

        ///
        ///
        void groupRoomInviteReceived(const QString &from, const QString& groupId, const QString& channelId);

        /// ORIGIN X
        void userJoined(const Origin::Chat::JabberID & jid, const Origin::Chat::MucRoom * room);
        void userLeft(const Origin::Chat::JabberID & jid, const Origin::Chat::MucRoom * room);

    public slots:
        // REMOVE FOR ORIGIN X
        //void leaveMucRoom(Origin::Chat::MucRoom* room);

    protected:
        JingleClient* jingle() const
        {
            return mJingle;
        }

        ///
        /// Returns our implementation event adapter
        ///
        XMPPImplEventAdapter *implEventAdapter()
        {
            return mImplEventAdapter;
        }

    private slots:
        void jingleMessageReceived(const JingleMessage &msg);
        void jingleChatStateReceived(const JingleMessage &msg);
        void presenceUpdate(Origin::Chat::XmppPresenceProxy presence);
        void addRequest(buzz::Jid from);
        void roomEntered(Origin::Chat::XmppChatroomModuleProxy room, QString role);
        void roomEnterFailed(Origin::Chat::XmppChatroomModuleProxy room, buzz::XmppChatroomEnteredStatus);
        void roomDestroyed(Origin::Chat::XmppChatroomModuleProxy room);
        void roomKicked(Origin::Chat::XmppChatroomModuleProxy room);
        void roomPasswordIncorrect(Origin::Chat::XmppChatroomModuleProxy room);
        void roomUserJoined(Origin::Chat::XmppChatroomMemberProxy member);
        void roomUserLeft(Origin::Chat::XmppChatroomMemberProxy member);
        void roomUserChanged(Origin::Chat::XmppChatroomMemberProxy member);
        void roomDeclineReceived(buzz::Jid decliner, buzz::Jid room, QString reason);

        void chatGroupRoomInviteReceived(buzz::Jid toJid, QString groupId, QString channelId);

        void onCapabilitiesChanged(Origin::Chat::XmppCapabilitiesProxy capabilities);

        void onConnected();
        void onStreamError(Origin::Chat::StreamErrorProxy error);
        void onJingleClose(Origin::Chat::JingleErrorProxy error);
        void doStreamReconnect();

        void doConnection();

        void doBackoff();

        void enteringMucRoomOpFinished();
        void destroyingMucRoomOpFinished();
        void mucRoomExited();

    private:
        void shutdownImplementation();

        bool filterIncomingMessage(const Message &) const;

        JabberID roomJabberIdForRoomId(const QString &roomId, const QString &roomNickname = QString()) const;
        Conversable* conversableForMessage(const JabberID &jid, const QString &type); 

        
        QByteArray mHost;
        int mPort;
        Configuration mConfiguration;

        // Our current connected user
        ConnectedUser *mCurrentUser;

        // All of our remote users
        mutable QMutex mRemoteUsersLock;
        QHash<JabberID, RemoteUser*> mRemoteUsers; 

        // All of the multi-user chat rooms we're attempting to enter
        mutable QMutex mEnteringMucRoomsLock;
        QHash<JabberID, EnterMucRoomOperation*> mEnteringMucRooms;

        // All of the multi-user chat rooms we're attempting to destroy
        mutable QMutex mDestroyingMucRoomsLock;
        QHash<JabberID, DestroyMucRoomOperation*> mDestroyingMucRooms;

        // All of the multi-user chat rooms we're participating in
        mutable QMutex mMucRoomsLock;
        QHash<JabberID, MucRoom*> mMucRooms;

        // Our event adapter. This is reused
        XMPPImplEventAdapter *mImplEventAdapter;

        // Our libJingle implementation
        JingleClient *mJingle;

        // The thread and wrapper object we use to call our implementation's event loop
        QThread *mImplEventPumpThread;
        XMPPImplEventPump *mImplEventPump;

        buzz::XmppThread* mJingleThread;

        // Our message filters
        QList<MessageFilter*> mIncomingMessageFilters;

        // Determines if we've established a connection
        QAtomicInt mEstablished;

        buzz::XmppClientSettings *mJingleSettings;

        QString mReconnect;
        DiscoverableInformation mDiscoverableInformation;

        // For backoff logic
        int mReconnectAttempts;
        int mInitialReconnectDelay;
        QTimer mReconnectTimer;
    };
}
}

#endif

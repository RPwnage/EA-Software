///////////////////////////////////////////////////////////////////////////////
// DirtyBitsClient.h
// Receives notifications from server.
// Created by J. Rivero
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef DIRTYBITSCLIENT_H
#define DIRTYBITSCLIENT_H

#if defined(ORIGIN_PC)
#define NOMINMAX
#endif


#include "services/rest/OriginClientFactory.h"
#include "services/originwebsocket/originwebsocket.h"
#include "services/session/SessionService.h"
#include "services/plugin/PluginAPI.h"

#include <QHostInfo>
#include <QPointer>
#include <QList>
#include <QMutex>
#include <QTimer>
#include <QMap>

namespace Origin
{
    namespace Engine
    {
        class ORIGIN_PLUGIN_API DirtyBitsClientDebugListener
        {
        public:
            virtual void onNewDirtyBitEvent(const QString& context, long long timeStamp, const QByteArray& payload, bool isError) = 0;
        };

        class ORIGIN_PLUGIN_API DirtyBitsClient : public QObject
        {
            Q_OBJECT
        public:
            friend class Origin::Services::OriginClientFactory<DirtyBitsClient>;

            static void init();

            /// DTOR
            ~DirtyBitsClient();

            /// \brief useful typedefs
            typedef unsigned long long DirtyTimeStamp;
            typedef QString DirtyContext;
            struct DirtyMessage 
            {
                DirtyContext mDirtyContext;
                DirtyTimeStamp mDirtyTimeStamp;
                QString mUid;
                QString mVerb;
                QString mData;
                DirtyMessage() : mDirtyContext("unknown"), mDirtyTimeStamp(0LL) {}
                QString data() const
                {
                    return mUid + "|" + mDirtyContext  + "|" + mVerb + "|" + mData;
                }
            };

            /// \brief Registers the handler to be called based on the context.
            /// \param obj owner of the slot
            /// \param slot to call
            /// \param context on which to call the slot on the object
            void registerHandler(QObject* obj, const QByteArray& slot, const DirtyContext& context);

            void registerDebugListener(DirtyBitsClientDebugListener* listener);

            void unregisterDebugListener(DirtyBitsClientDebugListener* listener);

            /// \brief Creates a singleton instance and returns it.
            static DirtyBitsClient* instance();

            /// \brief Releases resources.
            static void finish();

            /// \brief Sets whether or not to log activity.
            ///
            /// Set to log activity; otherwise, no logging.
            static void logActivity();

            /// brief set creationof fake messages DEBUGGING
            void setFakeMessages();

            /// brief set reading of fake messages DEBUGGING
            void deliverFakeMessageFromFile();

            /// brief set DB socket verbosity DEBUGGING
            void setSocketVerbosity();

        private slots:

            /// \brief slot executed upon socket connection
            void onConnected();

            /// \brief slot executed upon socket disconnection
            void onDisconnected();

            /// \brief slot executed upon pong reception
            void onPong(QByteArray ba);

            /// \brief slot executed upon new message received
            void onNewMessage(QByteArray ba);

            /// \brief slot executed upon keep connection alive timer timeout.
            void onKeepConnectionAliveTimeout();

            /// \brief connects WebSocket to host
            void connect();

            /// \brief stop socket from trying to reconnect (stop timer) and release resources, remove subscribers
            void stop();

            /// \brief tries to reconnect to server infinite times or until the client is shutdown
            void onTryReconnectTimeout();

            /// \brief slot executed upon socket error
            void onError(int32_t);

            /// \brief reconnects client to server
            void reconnect();

            /// \brief shortcut to know the online status
            void onOnline();
            void onOffline();

        signals:

        private:

            /// \brief CTOR and DTOR
            explicit DirtyBitsClient(QObject *parent = 0);

            /// \brief struct to hold the registered handlers
            struct HandlerInfo
            {
                QPointer<QObject> mObj; // object to apply the slot on
                QByteArray mSlot; // slot on the object
                DirtyContext mContext; // context

                explicit HandlerInfo(QObject* myobj, const QByteArray& slot, const DirtyContext& context);
            };

            /// \brief list with handlers to invoke when the relevant notification is detected
            QList<HandlerInfo> mHandlerInfoList;

            /// \brief contains the resource in the websocket server we will be connecting to
            QByteArray mResource;

            /// \brief true if we want the client to ignore Ssl errors
            bool mIsIgnoreSslErrors;

            /// \brief true if we want the client to reconnect
            mutable bool mIsReconnect;

            /// \brief headers for the connection to host. Will contain the auth token.
            Services::OriginWebSocketHeaders mHdrs;

            /// \brief to keep connection alive
            QTimer mKeepConnectionAliveTimer;

            /// \brief timer that starts when we disconnect for any reason
            QTimer mTryReconnectTimer;

            /// \brief Every once and awhile we get a duplicate message from dirty bits because Nucleus firehoses
            /// events with no guarantee of uniqueness.  We keep a list of the last 5 or so messages to check for dups
            QMap<QString, DirtyTimeStamp> mMessageChecklist;

            /// \brief counter for the number of messages received vs the ones actualy utilized (not counting the older messages)
            QMap<DirtyContext, size_t> mNotificationCounterMap;

            /// \brief holds the Websocket server hostname
            QByteArray mHostname;

            /// \brief to lock adding/erasing objects to subscribers map
            QMutex mHandlerInfoListMutex;

            /// \brief whether or not to log Dirty bits activity.
            static bool mIsLogActivity;

            /// \brief whether or not DirtyBits has been started.
            static bool mIsStarted;

            bool mIsFakeMessages;

            /// \brief performs necessary steps to initialize DirtyBits client
            void start();

            /// \brief sets the host name to connect to
            void setHostName(const QString& hostname);

            /// \brief sets the required headers for the initial connection
            void setHeaders();

            /// \brief sets the resource to utilize in the remote server
            void setResource();

            /// \brief Sets whether we want to ignore or not Ssl errors.
            /// \param val True if we want to ignore Ssl errors, false otherwise.
            void setIgnoreSslErrors(bool val = true);

            /// \brief Decodes the message type.
            /// \param ba (QByteArray&) TBD.
            /// \param context (CtxTS&) The message payload to parse.
            void messageContext(const QByteArray&, DirtyMessage&);

            /// \brief verifies that the message received isn't a duplicate over the last N messages
            inline bool isDuplicateMessage(const DirtyMessage& message);

            /// \brief broadcasts change to subscribers
            /// \param context to broadcast
            /// \param payload tobradcast
            void broadcastChange(const DirtyContext& context, const QByteArray& payload);

            /// \brief returns true if the websocket client was created and connected properly
            void connectSignals();

            /// \brief starts timer that keeps the connection alive.
            void startConnectionAliveTimer();

            /// \brief Initializes the required values for the class to function.
            void begin();

            /// \brief destroys the WebSocket.
            void destroy();

            /// \brief starts the timer to try to reconnect
            void startTryToReconnectTimer();

            /// \brief print message counters
            void printCounters();

            /// \brief clear lists of counters and subscribers
            void clearLists();

            /// \brief generates and sends fake message to the client.
            void fakeMessage();

            /// \brief detects double connection
            /// \param string to scan for 'dblconn' token
            bool isDoubleConn(const QString&) const;

            Services::OriginWebSocketHeaders mHeaders;

            QSet<DirtyBitsClientDebugListener*> mDebugListeners;

        };


    }
}

#endif // TSTCLIENT_H

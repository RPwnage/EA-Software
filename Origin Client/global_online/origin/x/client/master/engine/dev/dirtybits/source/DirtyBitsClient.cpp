///////////////////////////////////////////////////////////////////////////////
// DirtyBitsClient.cpp
// Receives notifications from server.
// Created by J. Rivero
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <memory>
#include <QPair>
#include <QMutexLocker>
#include "services/settings/SettingsManager.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/rest/UserAgent.h"
#include "engine/login/LoginController.h"
#include "engine/login/User.h"
#include "engine/dirtybits/DirtyBitsClient.h"
#include <NodeDocument.h>
#include "services/platform/PlatformService.h"
#include "TelemetryAPIDLL.h"

#include "services/heartbeat/Heartbeat.h"

namespace Origin
{
    namespace Engine
    {

        namespace
        {
#ifdef _DEBUG
            bool gIsReduceDirtyBitsCappedTimeout = true;
#else
            bool gIsReduceDirtyBitsCappedTimeout = false;
#endif
            bool gConnectInsecure = false; // to connect via 'ws' instead of 'wss' set this to 'true'
#ifdef _DEBUG            
            bool gForceReconnection = false; // To force reconnection after receiving a dblconn notification set this to 'true'
#endif          
            QMap<int,QString> errorsMap;

            const int ONE_SECOND =  1000;
            const int ONE_MINUTE =  60*ONE_SECOND;
            const int ONE_HOUR =  60*ONE_MINUTE;
            const int ONE_DAY =  24*ONE_HOUR;

            bool gIsIdentityChangeBroadcast = true;
            const int MAX_DUPLICATE_MESSAGE_CHECKLIST_SIZE = 5;
            const size_t DUPLICATE_MESSAGE_TIMEFRAME = 1;
            const static int SECS_TO_RECONNECT = 5;
            // to measure time between connection and disconnection time 
            // to log/telemetry 'unsupported data' hard closing of server socket.            
            qint64 gConnectionTime = 0; 
            const size_t KEEPCONNECTIONALIVETIMEOUT =
#if _DEBUG
                25 
#else
                55 
#endif
            * ONE_SECOND;
            // Make this threshold to be 90% of out keep alive timer
            const size_t TOO_FAST_DISCONNECTION_THRESHOLD = KEEPCONNECTIONALIVETIMEOUT - (KEEPCONNECTIONALIVETIMEOUT/10); // seconds

            static int gExp(1);

            bool isNetworkOnline()
            {
                return Origin::Services::Connection::ConnectionStatesService::isGlobalStateOnline();
            }

            Services::OriginWebSocket* webSocket()
            {
                return Origin::Services::OriginWebSocket::instance();
            }

            int reconnectTimeout()
            {
                int retVal;
#ifdef _DEBUG
                retVal =(ONE_SECOND * gExp) + (qrand() % ONE_SECOND);
#else
                retVal = (ONE_MINUTE * gExp) + (qrand() % ONE_MINUTE);
#endif
                gExp *= 2;
                return retVal;
            }

            Origin::Engine::UserRef user()
            {
//                 static int counter=0; // uncomment for testing for null user
//                 if (++counter < 3)
//                 {
//                     return UserRef();
//                 }
                return Origin::Engine::LoginController::instance()->currentUser();
            }

            int g_Retrytimeout = -1;
            int CAP = 0;
            bool isFakeMessages = false;
            int verbosity = 3;

            QString strError(int error)
            {
                if(errorsMap.contains(error))
                    return errorsMap[error];

                return QString::number(error);
            }
        }

        bool DirtyBitsClient::mIsLogActivity = false;
        bool DirtyBitsClient::mIsStarted = false;

        void DirtyBitsClient::init()
        {
            instance();
        }

        // -INSTANTIATION
        DirtyBitsClient* DirtyBitsClient::instance()
        {
            return Origin::Services::OriginClientFactory<DirtyBitsClient>::instance();
        }

        DirtyBitsClient::DirtyBitsClient(QObject *parent)
            : QObject(parent)
            ,mIsIgnoreSslErrors(false)
            ,mIsReconnect(true)
            ,mIsFakeMessages(isFakeMessages)
        {
            if (0==errorsMap.size())
            {
                errorsMap[SOCKERR_NONE]         = "no error" ; // 0          //!< no error
                errorsMap[SOCKERR_CLOSED]       = "the socket is closed" ; // -1       //!< the socket is closed
                errorsMap[SOCKERR_NOTCONN]      = "the socket is not connected" ; // -2      //!< the socket is not connected
                errorsMap[SOCKERR_BLOCKED]      = "operation would result in blocking" ; // -3      //!< operation would result in blocking
                errorsMap[SOCKERR_ADDRESS]      = "the address is invalid" ; // -4      //!< the address is invalid
                errorsMap[SOCKERR_UNREACH]      = "network cannot be accessed by this host" ; // -5      //!< network cannot be accessed by this host
                errorsMap[SOCKERR_REFUSED]      = "connection refused by the recipient" ; // -6      //!< connection refused by the recipient
                errorsMap[SOCKERR_OTHER]        = "unclassified error" ; // -7      //!< unclassified error
                errorsMap[SOCKERR_NOMEM]        = "out of memory" ; // -8      //!< out of memory
                errorsMap[SOCKERR_NORSRC]       = "out of resources" ; // -9      //!< out of resources
                errorsMap[SOCKERR_UNSUPPORT]    = "unsupported operation" ; // -10   //!< unsupported operation
                errorsMap[SOCKERR_INVALID]      = "resource or operation is invalid" ; // -11     //!< resource or operation is invalid
                errorsMap[SOCKERR_ADDRINUSE]    = "address already in use" ; // -12   //!< address already in use
                errorsMap[SOCKERR_CONNRESET]    = "connection has been reset" ; // -13   //!< connection has been reset
                errorsMap[SOCKERR_BADPIPE]      = "EBADF or EPIPE" ; // -14     //!< EBADF or EPIPE
            }

            // initialize random seed
            QTime now = QTime::currentTime();
            qsrand(now.msec());
            begin();
            mIsLogActivity = (Origin::Services::readSetting(Origin::Services::SETTING_LogDirtyBits).toQVariant().toBool() || Origin::Services::DebugLogToggles::dirtyBitsLoggingEnabled);
            gIsReduceDirtyBitsCappedTimeout = Origin::Services::readSetting(Origin::Services::SETTING_ReduceDirtyBitsCappedTimeout);

            CAP = gIsReduceDirtyBitsCappedTimeout ? ONE_MINUTE : ONE_DAY;

            Services::setLogWebSocket(mIsLogActivity);
#ifdef _DEBUG
            if(mIsLogActivity)
            {
                setSocketVerbosity();
            }
#endif // _DEBUG
        }

        DirtyBitsClient::~DirtyBitsClient()
        {
        }

        void DirtyBitsClient::registerDebugListener(DirtyBitsClientDebugListener* listener)
        {
            mDebugListeners.insert(listener);

            if(webSocket()->isConnected())
            {
                listener->onNewDirtyBitEvent("Connected", 0, QByteArray(), false);
            }
            else
            {
                listener->onNewDirtyBitEvent("Disconnected", 0, QByteArray(), true);
            }
        }

        void DirtyBitsClient::unregisterDebugListener(DirtyBitsClientDebugListener* listener)
        {
            mDebugListeners.remove(listener);
        }

        //-INITIALIZATION/CONTROL
        void DirtyBitsClient::begin()
        {
            setIgnoreSslErrors(Origin::Services::readSetting(Origin::Services::SETTING_IgnoreSSLCertError));
            ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(nowOnlineUser()), this, SLOT(onOnline()));
            ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(nowOfflineUser()), this, SLOT(onOffline()));
            // connect timers
            ORIGIN_VERIFY_CONNECT(&mTryReconnectTimer, SIGNAL(timeout()), this, SLOT(onTryReconnectTimeout()));
            ORIGIN_VERIFY_CONNECT(&mKeepConnectionAliveTimer, SIGNAL(timeout()), this, SLOT(onKeepConnectionAliveTimeout()));
            setHostName(Origin::Services::readSetting(Origin::Services::SETTING_DirtyBitsServiceHostname));
            connectSignals();
        }

        void DirtyBitsClient::start()
        {
            if (!isNetworkOnline())
            {
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[warning] User is offline.";
                return;
            }

            if(webSocket()->isConnected())
            {
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[warning] WebSocket already connected.";
                return;
            }
            destroy(); // Destroys WebSocket
            if (user().isNull())
            {
                if (mIsReconnect)
                {
                    ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[###]  Dirty bits user not available, reconnecting in [" << ONE_SECOND*SECS_TO_RECONNECT << "] milliseconds.";
                    QTimer::singleShot(ONE_SECOND*SECS_TO_RECONNECT, this, SLOT(reconnect()));
                }
                return;
            }
            ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[_-_-_-_-] Starting Dirty bits client.";
            setResource();
            connect();
            mIsStarted = true;
            ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[_-_-_-_-] Dirty bits client started.";
            ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[information] Retry cap is set to: [" << CAP/1000.0 << "] seconds.";
        }

        void DirtyBitsClient::connect()
        {
            setHeaders();
            bool isConnect(false);
            if (gConnectInsecure)
            {
                isConnect = webSocket()->connectInsecure(mHostname, mResource, mHdrs);
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[-_-_-_-_] Dirty bits trying to connect [INSECURE]...";
            }
            else
            {
                isConnect = webSocket()->connectSecure(mHostname, mResource, mHdrs);
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[myinfo] Dirty bits trying to connect [secure]...";
            }
            if (isConnect)
            {
                mIsReconnect = true;
                mTryReconnectTimer.stop();
            }
            else
            {
                if (isNetworkOnline())
                {
                    if (mIsReconnect)
                    {
                        ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[###]  Dirty bits client *NOT* connected, reconnecting in [" << ONE_SECOND*SECS_TO_RECONNECT << "] milliseconds.";
                        QTimer::singleShot(ONE_SECOND*SECS_TO_RECONNECT, this, SLOT(reconnect()));
                    }
                }
            }
        }


        void DirtyBitsClient::onOnline()
        {
            start();
        }
        void DirtyBitsClient::onOffline()
        {
            stop();
        }

        void DirtyBitsClient::stop()
        {
            isFakeMessages = false;
            verbosity = 3;
            mIsReconnect = false;
            webSocket()->disconnect();
            gExp = 1;
            ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[myinfo] DirtyBits stopped.";
        }

        void DirtyBitsClient::finish()
        {
            if (mIsStarted)
            {
                Origin::Services::OriginClientFactory<DirtyBitsClient>::instance()->printCounters();
                Origin::Services::OriginClientFactory<DirtyBitsClient>::instance()->clearLists();
                Origin::Services::OriginClientFactory<DirtyBitsClient>::instance()->stop();
                g_Retrytimeout =  - 1;
                GetTelemetryInterface()->Metric_DIRTYBITS_NETWORK_DISCONNECTED(strError(SOCKERR_NONE).toLocal8Bit());

                // HEARTBEAT: Update dirty bit info
                Origin::Services::Heartbeat::instance()->updateDirtyBitsInfo(false);
            }
        }

        void DirtyBitsClient::destroy()
        {
            webSocket()->destroy();
        }

        // -SETTING
        void DirtyBitsClient::setHostName(const QString& hostname)
        {
            mHostname = hostname.toLocal8Bit();
        }

        void DirtyBitsClient::setResource()
        {
            const QString resource = Origin::Services::readSetting(Origin::Services::SETTING_DirtyBitsServiceResource);
            mResource = resource.toUtf8();
            if (!mResource.endsWith("/"))
                mResource += "/";
            mResource += QString::number(user()->userId()).toUtf8();
            ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[myinfo] Setting WebSocket resource: [" << mResource  << "]";
        }

        void DirtyBitsClient::setHeaders()
        {
            ORIGIN_ASSERT(user());
            if (user())
            {
                QString accessToken = Origin::Services::Session::SessionService::accessToken(user()->getSession());

                mHdrs.clear();
                mHdrs.insert("AuthToken", accessToken.toUtf8());
                mHdrs.insert(Services::UserAgent::title().constData(), Services::UserAgent::headerAsBA());
                mHdrs.insert("X-Origin-MachineHash", Services::PlatformService::machineHashAsString().toUtf8());
                mHdrs.insert("X-Origin-Platform",

#if defined(ORIGIN_PC)
                    "PCWIN"
#elif defined(ORIGIN_MAC)
                    "MAC"
#else
#error Must specialize for other platform.
#endif
                    );
            }
        }

        void DirtyBitsClient::setIgnoreSslErrors( bool val )
        {
            mIsIgnoreSslErrors = val;
            if (mIsIgnoreSslErrors)
            {
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[warning] WebSocket will ignore SSL Errors";
                webSocket()->control('ncrt', 0);
            }
        }

        // -SLOTS
        void DirtyBitsClient::onConnected()
        {
            gConnectionTime = QDateTime::currentMSecsSinceEpoch();
            foreach(DirtyBitsClientDebugListener* listener, mDebugListeners)
            {
                listener->onNewDirtyBitEvent("Connected", gConnectionTime, QByteArray(), false);
            }

            ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"[-----] DirtyBits client Connected.";
            // reset reconnection retry timer exponential
            gExp = 1;
            startConnectionAliveTimer();
            GetTelemetryInterface()->Metric_DIRTYBITS_NETWORK_CONNECTED();
              
            // HEARTBEAT: Update dirty bit inf
            Origin::Services::Heartbeat::instance()->updateDirtyBitsInfo(true);
        }

        bool DirtyBitsClient::isDoubleConn(const QString& str) const
        {
            if(str.contains("dblconn"))
            {
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[error] Double connection detected, server will disconnect us and we will not reconnect.";
#if _DEBUG
                mIsReconnect = gForceReconnection;
#else
                mIsReconnect = false; // the server will disconnect us, we shouldn't try to reconnect.
#endif
                return true;
            }
            return false;
        }

        void DirtyBitsClient::onDisconnected()
        {
            mKeepConnectionAliveTimer.stop();

            qint64 disconnectionTime = QDateTime::currentMSecsSinceEpoch();

            foreach(DirtyBitsClientDebugListener* listener, mDebugListeners)
            {
                listener->onNewDirtyBitEvent("Disconnected", disconnectionTime, QByteArray(), true);
            }

            // HEARTBEAT: Update dirty bit inf
            Origin::Services::Heartbeat::instance()->updateDirtyBitsInfo(false);

            Services::LastWebSocketStatus mLwss = Services::OriginWebSocket::lastWebSocketCloseReason();
            ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[error] Client disconnected : status [" << mLwss.mStatus << "] error string [" << mLwss.mErrorStr << "] socket error [" << errorsMap[mLwss.mErrorStatus] << "]";

            isDoubleConn(mLwss.mErrorStr);

            bool suddenDisconnect = false;
            if (0<gConnectionTime && mIsReconnect) // was there a connection?
            {
                // Testing if the disconnection time happened too fast
                if ((disconnectionTime - gConnectionTime) < (TOO_FAST_DISCONNECTION_THRESHOLD))
                {
                    disconnectionTime = disconnectionTime - gConnectionTime;
                    ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[myinfo] The Dirty Bits client was closed way too fast: [Disconnection time: " << disconnectionTime << "; Threshold " << TOO_FAST_DISCONNECTION_THRESHOLD << "] Mseconds";
                    suddenDisconnect = true;
                }
                gConnectionTime = 0;
            }

            if (suddenDisconnect)
            {
                GetTelemetryInterface()->Metric_DIRTYBITS_NETWORK_SUDDEN_DISCONNECTION(disconnectionTime);
            }
            else
            {
                GetTelemetryInterface()->Metric_DIRTYBITS_NETWORK_DISCONNECTED(strError(SOCKERR_NONE).toLocal8Bit());
            }
        }

        void DirtyBitsClient::onTryReconnectTimeout()
        {
            if (!isNetworkOnline())
                return;
            mTryReconnectTimer.stop();
            ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[myinfo] Trying to re-connect...";
            start();
        }

        void DirtyBitsClient::setSocketVerbosity()
        {
            ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[####] setting web socket verbosity [" << (verbosity > 0? "ON" : "off") << "]";
            webSocket()->control('spam', verbosity);
            verbosity = verbosity > 0? 0 : 3;
        }

        void DirtyBitsClient::onKeepConnectionAliveTimeout()
        {
            if (webSocket()->isConnected())
            {
                webSocket()->control('pong');
                if (mIsLogActivity)
                {
                    static int pongCounter = 0;
                    static int pongEvery = 10;
                    if (0==(pongCounter++ % pongEvery)) // will log every pongEvery pongs
                    {
                        pongEvery += pongEvery;
                        ORIGIN_LOG_EVENT_IF (mIsLogActivity) << "[information] I have sent [" << pongCounter << "] PONG(s). Next log will occur at: " << pongEvery << " pongs.";
                    }
                }
                fakeMessage();
            }
        }

        void DirtyBitsClient::onError(int32_t error)
        {
            foreach(DirtyBitsClientDebugListener* listener, mDebugListeners)
            {
                listener->onNewDirtyBitEvent("Error", QDateTime::currentMSecsSinceEpoch(), QString("{ \"errorCode\" : \"%1\", \"errorString\" : \"%2\" }").arg(QString::number(error)).arg(strError(error)).toUtf8(), true);
            }

            if (SOCKERR_CLOSED == error)
            {
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"WebSocket Error NETWORK FAILURE [" << strError(error) <<"]";
            }
            else
            {
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"WebSocket Error [" << strError(error) <<"]";
            }
            GetTelemetryInterface()->Metric_DIRTYBITS_NETWORK_DISCONNECTED(strError(error).toLocal8Bit());
            
            // HEARTBEAT: Update dirty bit info
            Origin::Services::Heartbeat::instance()->updateDirtyBitsInfo(false);

            if (isNetworkOnline())
            {
                if (mIsReconnect)
                {
                    ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[###]  reconnecting in [" << ONE_SECOND*SECS_TO_RECONNECT << "] milliseconds.";
                    QTimer::singleShot(ONE_SECOND*SECS_TO_RECONNECT, this, SLOT(reconnect()));
                }
            }
        }

        void DirtyBitsClient::onPong(QByteArray pong)
        {
            Q_UNUSED(pong);
            ORIGIN_LOG_EVENT_IF(mIsLogActivity)  << "[information] PONG received [" << pong << "]" ;
            return;     
        }

        void DirtyBitsClient::onNewMessage(QByteArray incomingMessage)
        {
            ORIGIN_LOG_EVENT_IF (mIsLogActivity) << "[myinfo] Incoming message: [" << incomingMessage << "]";

            mNotificationCounterMap["Total received"]++;
            ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[information] Total messages received [" << mNotificationCounterMap["Total received"] << "]";

            // HEARTBEAT: Update dirty bit info
            Origin::Services::Heartbeat::instance()->updateDirtyBitsInfo(true, true);

            // detect the context of message we have received.
            DirtyMessage dirtyMessage;
            messageContext(incomingMessage, dirtyMessage);

            foreach(DirtyBitsClientDebugListener* listener, mDebugListeners.values())
            {
                listener->onNewDirtyBitEvent(dirtyMessage.mDirtyContext, dirtyMessage.mDirtyTimeStamp, incomingMessage, false);
            }

            if (dirtyMessage.mDirtyContext == "unknown")
            {
                mNotificationCounterMap["unknown"]++;
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[_-_-_-_-_] Unknown messages received [" << mNotificationCounterMap["unknown"] << "]";
                return;
            }

            // define whether or not broadcast this message
            if (isDuplicateMessage(dirtyMessage))
            {
                mNotificationCounterMap["duplicate"]++;
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[_-_-_-_-_] Duplicate messages received [" << mNotificationCounterMap["duplicate"] << "]";
                return;
            }

            broadcastChange(dirtyMessage.mDirtyContext, incomingMessage);
        }

        // -HANDLING NOTIFICATIONS
        void DirtyBitsClient::registerHandler( QObject* obj, const QByteArray& slot, const DirtyContext& context)
        {
            QMutexLocker locker(&mHandlerInfoListMutex);
            // have we registered this one yet?
            QList<HandlerInfo>::iterator it = mHandlerInfoList.begin();
            while (it != mHandlerInfoList.end()) 
            {
                if (obj==(*it).mObj && context == (*it).mContext && slot == (*it).mSlot)
                {
                    return;
                }
                else
                    ++it;
            }

            ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[myinfo] Subscribing with Dirty Bits: QObject* [" << obj << "] slot/handler [" << slot << "] for the context [" << context << "]";
            mHandlerInfoList.append(HandlerInfo(obj, slot, context));
        }

        void DirtyBitsClient::broadcastChange( const DirtyContext& context, const QByteArray& payload)
        {
            QMutexLocker locker(&mHandlerInfoListMutex);
            mNotificationCounterMap[context]++;


            // Clean list of objects that have passed away
            QList<HandlerInfo>::iterator it = mHandlerInfoList.begin();
            while (it != mHandlerInfoList.end()) 
            {
                if (NULL==(*it).mObj)
                    it = mHandlerInfoList.erase(it);
                else
                    ++it;
            }

            if (mHandlerInfoList.isEmpty())
                return;

            for (int i = 0; i < mHandlerInfoList.size(); ++i)
            {
                if (NULL!=mHandlerInfoList[i].mObj)
                {
                    if (mHandlerInfoList[i].mContext == context)
                    {
                        if ("email" == context || "originid" == context || "password" == context)
                        {
                            if (gIsIdentityChangeBroadcast)
                            {
                                ORIGIN_VERIFY(
                                    QMetaObject::invokeMethod(mHandlerInfoList[i].mObj
                                    , mHandlerInfoList[i].mSlot
                                    , Qt::QueuedConnection
                                    , Q_ARG(QByteArray, payload)));
                                ORIGIN_LOG_EVENT_IF (mIsLogActivity) << "[-----] Invoking:" << mHandlerInfoList[i].mSlot;
                            } 
                            else
                            {
                                ORIGIN_LOG_EVENT_IF (mIsLogActivity) << "[warning] NOT Invoking:" << mHandlerInfoList[i].mSlot;
                            }
                        }
                        else
                        {
                            ORIGIN_VERIFY(
                                QMetaObject::invokeMethod(mHandlerInfoList[i].mObj
                                , mHandlerInfoList[i].mSlot
                                , Qt::QueuedConnection
                                , Q_ARG(QByteArray, payload)));
                            ORIGIN_LOG_EVENT_IF (mIsLogActivity) << "[-----] Invoking:" << mHandlerInfoList[i].mSlot;
                        }

                    }
                }
            }
        }

        DirtyBitsClient::HandlerInfo::HandlerInfo( QObject* myobj, const QByteArray& slot, const DirtyContext& context ) 
            : mObj( myobj), mSlot(slot), mContext(context)
        {
        }

        void DirtyBitsClient::messageContext(const QByteArray& ba, DirtyMessage& dirtyMessage)
        {
            QScopedPointer<INodeDocument> json(CreateJsonDocument());

            if(json && json->Parse(ba.constData()))
            {
                QString ctx = json->GetAttributeValue("ctx"); // Context

                // detect if we are connecting from two sources simultaneously
                isDoubleConn(ctx);

                QString sTs = json->GetAttributeValue("ts"); // TimeStamp
                DirtyTimeStamp ts = sTs.toULongLong();

                dirtyMessage.mDirtyContext = ctx;
                dirtyMessage.mDirtyTimeStamp = ts;
                dirtyMessage.mUid = json->GetAttributeValue("uid");
                dirtyMessage.mVerb = json->GetAttributeValue("verb");
                if (json->FirstChild("data"))
                {
                    dirtyMessage.mData = json->GetValue();
                }
            }
        }

        bool DirtyBitsClient::isDuplicateMessage(const DirtyMessage& message)
        {
            bool result = false;
            QString messageData = message.data();
            auto it = mMessageChecklist.find(messageData);
            if (it != mMessageChecklist.end())
            {
                // looks like we have a duplicate
                DirtyTimeStamp differ = message.mDirtyTimeStamp - mMessageChecklist[messageData];
                if ((DUPLICATE_MESSAGE_TIMEFRAME) < differ)
                {
                    //valid message, it arrived more than some seconds later
                    result = false;
                }
                else
                {
                    result = true; // is duplicated
                    ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[####] DUPLICATE MESSAGE [" << messageData << ":" << mMessageChecklist[messageData] << "] they arrived within [" << differ << "] msec one from another.";
                }
                // erase the current element
                mMessageChecklist.erase(it);
            }

            if(mMessageChecklist.size() >= MAX_DUPLICATE_MESSAGE_CHECKLIST_SIZE)
            {
                mMessageChecklist.clear();
            }

            // map it
            mMessageChecklist[messageData] = message.mDirtyTimeStamp;
            return result;
        }

        void DirtyBitsClient::clearLists()
        {
            mNotificationCounterMap.clear();
            mHandlerInfoList.clear();
            mMessageChecklist.clear();
        }

        // -HELPERS
        void DirtyBitsClient::connectSignals()
        {
            mKeepConnectionAliveTimer.stop();

            if (webSocket()->isValid())
            {
                ORIGIN_VERIFY_CONNECT(webSocket(), SIGNAL(connected()),this, SLOT(onConnected()));
                ORIGIN_VERIFY_CONNECT(webSocket(), SIGNAL(disconnected()),this, SLOT(onDisconnected()));
                ORIGIN_VERIFY_CONNECT(webSocket(), SIGNAL(newMessage(QByteArray)),this, SLOT(onNewMessage(QByteArray)));
                ORIGIN_VERIFY_CONNECT(webSocket(), SIGNAL(socketError(int32_t)),this, SLOT(onError(int32_t)));
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[myinfo] WebSocket signals connected.";
            }
        }

        void DirtyBitsClient::startConnectionAliveTimer()
        {
            mKeepConnectionAliveTimer.stop();
            mKeepConnectionAliveTimer.start(KEEPCONNECTIONALIVETIMEOUT);
            ORIGIN_LOG_EVENT_IF (mIsLogActivity) << "[information] Started heartbeat timer.";
        }

        void DirtyBitsClient::logActivity()
        {
            mIsLogActivity = !mIsLogActivity;
            ORIGIN_LOG_EVENT << "[myinfo] " << (mIsLogActivity?"Started":"Stopped") << " Logging activity.";
        }

        void DirtyBitsClient::setFakeMessages()
        {
            isFakeMessages = !isFakeMessages;
            mIsFakeMessages = isFakeMessages;
            ORIGIN_LOG_EVENT_IF (mIsLogActivity) << (mIsFakeMessages?"[information] Faking messages":"[information] NOT faking messages");
        }

        void DirtyBitsClient::fakeMessage()
        {
            if(mIsFakeMessages)
            {
                QString msg = "{\"uid\" : %1, \"ver\": 1, \"ctx\": \"%2\", \"ts\": \"%3\", \"actn\": \"update\", \"data\": { %4 } }";

                unsigned long long timestamp = QDateTime::currentMSecsSinceEpoch()/1000;

                QString context;
                QString uid = Origin::Services::Session::SessionService::instance()->nucleusUser(Origin::Services::Session::SessionService::currentSession());
                QString pid = Origin::Services::Session::SessionService::instance()->nucleusPersona(Origin::Services::Session::SessionService::currentSession());
                QString payload = "\"val\" : \"jose@ea.com\"";

                // Modify this value to get the message you want.
                static int j = 5;

                switch(j)
                {
                case 0:
                    context="email";
                    break;
                case 1:
                    context="passwd";
                    break;
                case 2:
                    context="originId";
                    break;
                case 3:
                    context="dblconn";
                    break;
                case 4:
                    context="ach";
                    payload = QString("\"pid\":%1,\"prodid\":\"68481_69317_50844\",\"achid\":\"102\"").arg(pid);
                case 5:
                    {
                        static int avatarIdCounter = 16;
                        context="avatar";
                        payload = QString("\"avatarid\":%1").arg(avatarIdCounter++);
                    }
                }

                msg = msg.arg(uid).arg(context).arg(timestamp).arg(payload);
                QByteArray ba = msg.toUtf8();
                onNewMessage(ba);
            }
        }
        
        void DirtyBitsClient::deliverFakeMessageFromFile()
        {
            QByteArray fakePayload;
            QString fakePayloadFile = Origin::Services::readSetting(Origin::Services::SETTING_FakeDirtyBitsMessageFile).toString();
            
            if(!fakePayloadFile.isEmpty())
            {
                QFile payload(fakePayloadFile);

                if(payload.open(QIODevice::ReadOnly))
                {
                    fakePayload = payload.readAll();
                }

                if(fakePayload.isEmpty())
                {
                    ORIGIN_LOG_ERROR << "Failed to read fake message file [" << fakePayloadFile << "] designated by setting [" << Origin::Services::SETTING_FakeDirtyBitsMessageFile.name() << "]";
                }
        
                if(!fakePayload.isEmpty())
                {
                    ORIGIN_LOG_ACTION << "Delivering fake message: " << fakePayload;
                    onNewMessage(fakePayload);
                }
            }
            else
            {
                ORIGIN_LOG_ERROR << "Must have FakeDirtyBitsMessageFile configured in [QA] section of EACore.ini to use this option.";
            }
        }

        void DirtyBitsClient::printCounters()
        {

                ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"Total received: " << mNotificationCounterMap["Total received"];
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"email: " << mNotificationCounterMap["email"];
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"originid: " << mNotificationCounterMap["originid"];
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"password: " << mNotificationCounterMap["password"];
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"dblconn: " << mNotificationCounterMap["dblconn"];
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"ach: " << mNotificationCounterMap["ach"];
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"avatar: " << mNotificationCounterMap["avatar"];
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"entitlement: " << mNotificationCounterMap["entitlement"];
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"catalog: " << mNotificationCounterMap["catalog"];
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"unknown: " << mNotificationCounterMap["unknown"];
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) <<"duplicate: " << mNotificationCounterMap["duplicate"];


            GetTelemetryInterface()->Metric_DIRTYBITS_MESSAGE_COUNTER(
                 QString::number(mNotificationCounterMap["Total received"]).toLocal8Bit()
                 ,QString::number(mNotificationCounterMap["email"]).toLocal8Bit()
                ,QString::number(mNotificationCounterMap["originid"]).toLocal8Bit()
                ,QString::number(mNotificationCounterMap["password"]).toLocal8Bit()
                ,QString::number(mNotificationCounterMap["dblconn"]).toLocal8Bit()
                ,QString::number(mNotificationCounterMap["ach"]).toLocal8Bit()
                ,QString::number(mNotificationCounterMap["avatar"]).toLocal8Bit()
                ,QString::number(mNotificationCounterMap["entitlement"]).toLocal8Bit()
                ,QString::number(mNotificationCounterMap["catalog"]).toLocal8Bit()
                ,QString::number(mNotificationCounterMap["unknown"]).toLocal8Bit()
                ,QString::number(mNotificationCounterMap["duplicate"]).toLocal8Bit());
            }

        void DirtyBitsClient::startTryToReconnectTimer()
        {
            // connected to onTryReconnectTimeout()
            int retryTimeout = -1;
            if(g_Retrytimeout > CAP)
            {
                ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[warning] Retry timeout Cap reached: [" << g_Retrytimeout/1000.0 << "] seconds.";
                QString logStr;
                logStr = QString::number(g_Retrytimeout/1000.0);
                logStr += " secs.";
                GetTelemetryInterface()->Metric_DIRTYBITS_RETRIES_CAPPED(logStr.toLocal8Bit());
            }
            else
            {
                g_Retrytimeout = reconnectTimeout();
            }
            retryTimeout = g_Retrytimeout;

            ORIGIN_LOG_EVENT_IF(mIsLogActivity) << "[_-_-_-_-_] Retry timeout: [" << retryTimeout/1000.0 << "] seconds.";
            mTryReconnectTimer.start(retryTimeout); 
        }

        void DirtyBitsClient::reconnect()
        {
            if (!mTryReconnectTimer.isActive())
            {
                if (mIsReconnect)
                {
                    startTryToReconnectTimer();
                }
            }
        }

    }
}

///////////////////////////////////////////////////////////////////////////////
// originwebsocket.cpp
// Wrapper around DirtySDK's implementation of protowebsocket.
// Created by J. Rivero
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#if defined(ORIGIN_PC)
#define NOMINMAX
#elif defined(ORIGIN_MAC)
#else
#error Must specialize for other platform.
#endif


#include "services/originwebsocket/originwebsocket.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "DirtySDK/dirtysock/dirtycert.h"

#include <QPair>

namespace Origin
{
    namespace Services
    {
        LastWebSocketStatus OriginWebSocket::mLwss;
        namespace
        {
            bool g_isLog = 
#ifdef _DEBUG
                true;
#else
                false;
#endif // _DEBUG
                
            // build the header for the websocket
            QByteArray buildHeaders(const QByteArray& hostname, const OriginWebSocketHeaders& headers)
            {
                auto it = headers.constBegin();
                QByteArray myHeaders;
                while(it != headers.constEnd())
                {
                    myHeaders += it.key();
                    myHeaders += ":";
                    myHeaders += it.value();
                    myHeaders += "\r\n";
                    it++;
                }
                if (!headers.contains("Host")) 
                {
                    myHeaders += "Host";
                    myHeaders += ":";
                    myHeaders += hostname;
                    myHeaders += ":";
                    if (hostname.startsWith("ws://"))
                    {
                        myHeaders += "80";
                    }
                    else
                    {
                        myHeaders += "443";
                    }
                    myHeaders += "\r\n";
                }
                return myHeaders;
            }
        }

        void setLogWebSocket(bool val)
        {
            g_isLog = val;
        }

        namespace WebSocketPriv
        {
            WebSocket::WebSocket() :
                pWebSocket(NULL)
                ,pSendBuf(NULL)
                ,mIsConnected(false)
            {
                ORIGIN_LOG_EVENT_IF(g_isLog) << "[-----] WebSocket instantiated.";
                reset();
            }

            WebSocket::~WebSocket()
            {
                disconnect();
                destroy();
                ORIGIN_LOG_EVENT_IF(g_isLog) << "[information] WebSocket destroyed.";
            }

            void WebSocket::create()
            {
                if (NULL!=pWebSocket)
                {
                    ORIGIN_LOG_EVENT_IF(g_isLog) << "[warning] WebSocket module already created";
                    return;
                }

                // create a WebSocket module if it isn't already started
                if ((pWebSocket = ProtoWebSocketCreate(0)) == NULL)
                {
                    ORIGIN_LOG_EVENT_IF(g_isLog) << "[error] creating WebSocket module";
                }
                else
                {
                    ORIGIN_LOG_EVENT_IF(g_isLog) << "[-----] WebSocket created";
                }
                char myName[] = "ORIGINWEBSOCKET";
                DirtyCertControl('snam', 0, 0, &myName[0]);
            }

                /*
                 *Web Socket commands
                'apnd'      The given buffer will be appended to future headers sent by
                ProtoWebSocket.  The Accept header in the default header will
                be omitted.  If there is a User-Agent header included in the
                append header, it will replace the default User-Agent header.
                'clse'      Send a close message with optional reason (iValue) and data
                (string passed in pValue)
                'keep'      Set keep-alive timer, in seconds; client sends a 'pong' at this
                rate to keep connection alive (0 disables; disabled by default)
                'ping'      Send a ping message with optional data (string passed in pValue)
                'pong'      Send a pong message with optional data (string passed in pValue)
                'spam'      Sets debug output verbosity (0...n)
                'time'      Sets ProtoWebSocket client timeout in seconds (default=300s)
                '
                'ProtoSSL commands
                 'ciph'      Set enabled/disabled ciphers (PROTOSSL_CIPHER_*)
                 'ncrt'      Disable client certificate validation.
                 'rbuf'      Set socket recv buffer size (must be called before Connect()).
                 'sbuf'      Set socket send buffer size (must be called before Connect()).
                 'secu'      Start secure negotiation on an established unsecure connection.
                 'spam'      Set debug logging level (default=1)
                 'xdns'      Enable Xenon secure-mode DNS lookups for this ref.
                 **/
            QString intToText(int32_t command)
            {
                static QMap<int32_t,QString> WSCommandsMap;
                if (WSCommandsMap.size()==0)
                {
                    WSCommandsMap['apnd'] = "apnd";
                    WSCommandsMap['clse'] = "clse";
                    WSCommandsMap['keep'] = "keep";
                    WSCommandsMap['ping'] = "ping";
                    WSCommandsMap['pong'] = "pong";
                    WSCommandsMap['spam'] = "spam";
                    WSCommandsMap['time'] = "time";
                    WSCommandsMap['ciph'] = "ciph";
                    WSCommandsMap['ncrt'] = "ncrt";
                    WSCommandsMap['rbuf'] = "rbuf";
                    WSCommandsMap['sbuf'] = "sbuf";
                    WSCommandsMap['secu'] = "secu";
                    WSCommandsMap['spam'] = "spam";
                    WSCommandsMap['xdns'] = "xdns";
                }
                if (WSCommandsMap.contains(command))
                {
                    return WSCommandsMap[command];
                }
                ORIGIN_ASSERT(false);
                return "unkn";
            }

            void WebSocket::applyControlCommands()
            {
                if (mDirtyControlVector.size()==0)
                {
                    return;
                }
                // Applying control commands now that we are connected
                int32_t result = 0;
                QList<int32_t> keylist = mDirtyControlVector.keys();
                foreach(int32_t index, keylist)
                {
                    DirtyControl dc = mDirtyControlVector[index];
                    result = ProtoWebSocketControl(pWebSocket, dc.iCmd, dc.iValue1, dc.iValue2, dc.pValue);
                    ORIGIN_LOG_EVENT_IF(g_isLog) << "[myinfo] Applied [" << intToText(dc.iCmd) << ":" << dc.iValue1 << "] control to WebSocket returned [" << result << "]";
                }
            }

            int32_t WebSocket::connect( QByteArray& myUrl )
            {
                ORIGIN_LOG_EVENT_IF(g_isLog) << "[information] WebSocket connecting to: " << myUrl;
                int32_t iResult = ProtoWebSocketConnect(pWebSocket, myUrl.data());
                return iResult;
            }

            int32_t WebSocket::disconnect()
            {
                if (NULL==pWebSocket)
                    return 0;

                int32_t iResult = ProtoWebSocketDisconnect(pWebSocket);
                if (0!=iResult)
                {
                    ORIGIN_LOG_EVENT_IF(g_isLog) << "[error] WebSocket disconnection failed: " << iResult;
                }
                mIsConnected =  false;
                return iResult;
            }

            void WebSocket::destroy()
            {
                if (NULL!=pWebSocket)
                {
                    ProtoWebSocketDestroy(pWebSocket);
                    reset();
                    ORIGIN_LOG_EVENT_IF(g_isLog) << "[information] WebSocket destroyed";
                }
            }

            void WebSocket::reset()
            {
                pWebSocket = NULL;
                resetSendBuffer();
                resetReceiveBuffer();
                ORIGIN_LOG_EVENT_IF(g_isLog) << "[information] WebSocket data reset.";
            }

            void WebSocket::resetSendBuffer()
            {
                if (NULL!=pSendBuf)
                {
                    delete []  pSendBuf;
                }
                pSendBuf = NULL;
                iSendLen = 0;
                iSendOff = 0;
            }

            void WebSocket::resetReceiveBuffer()
            {
                memset(&strRecvBuf[0],0,BUFFER_SIZE);
            }

            void WebSocket::appendHeaders(QByteArray& myHeaders )
            {
                // append header to web socket.
                ORIGIN_LOG_EVENT_IF(g_isLog) << "[information] Appending headers to WebSocket: " << myHeaders;
                ProtoWebSocketControl(pWebSocket, 'apnd', 0, 0, myHeaders.data());
            }

            void WebSocket::setSendBuffer(const char* mydata, size_t size)
            {
                pSendBuf = new char[size];
                ::memcpy(pSendBuf, mydata, size);
                iSendLen =  size;
                iSendOff = 0;
                ORIGIN_LOG_EVENT_IF(g_isLog) << "[information] WebSocket send buffer set data [" << pSendBuf << "] size [" << size << "]";
            }

            bool WebSocket::isValid() const
            {
                return NULL!=pWebSocket;
            }

            bool WebSocket::isConnected() const
            {
                return mIsConnected;
            }

            int32_t WebSocket::control( int32_t iCmd, int32_t iValue1, int32_t iValue2, void* pValue )
            {
                if ('pong'!=iCmd) // don't queue the pong control
                    addToCtrlQueue(iCmd, iValue1, iValue2, pValue);

                if (isConnected())
                {
                    if ('pong'!=iCmd) // don't log all the pong control commands! :)
                    {
                        ORIGIN_LOG_EVENT_IF(g_isLog) << "[myinfo] Applied [" << intToText(iCmd) << ":" << iValue1 << "] control to WebSocket.";
                    }
                    return ProtoWebSocketControl(pWebSocket, iCmd, iValue1, iValue2, pValue);
                }
                return int32_t();
            }

            int32_t WebSocket::status( int32_t iSelect, QString& result ) const
            {
                char strBuffer[256] = "";
                int32_t iResult = ProtoWebSocketStatus(pWebSocket, iSelect, strBuffer, sizeof(strBuffer));
                //ORIGIN_LOG_EVENT_IF(g_isLog) << "ProtoWebSocketStatus returned: " << iResult << " [" << strBuffer << "]";
                result = strBuffer;
                return iResult;
            }

            void WebSocket::addToCtrlQueue( int32_t iCmd, int32_t iValue1, int32_t iValue2, void* pValue )
            {
                DirtyControl dc(iCmd, iValue1, iValue2, pValue);
                // if we receive two commands, the newest will overwrite the oldest
                mDirtyControlVector[iCmd] = dc;
                ORIGIN_LOG_EVENT_IF(g_isLog) << "[myinfo] Added [" << intToText(iCmd) << ":" << iValue1 << "] control to WebSocket control Queue.";
            }

        }

        OriginWebSocket* OriginWebSocket::instance()
        {
            return Origin::Services::OriginClientFactory<OriginWebSocket>::instance();
        }

        OriginWebSocket::OriginWebSocket( QObject *parent /*= 0*/ )
        {
            create();
            ORIGIN_LOG_EVENT_IF(g_isLog) << "[-----] OriginWebSocket Instantiated.";
        }

        OriginWebSocket::~OriginWebSocket()
        {
            ORIGIN_LOG_EVENT_IF(g_isLog) << "[information] OriginWebSocket destroyed.";
        }

        bool OriginWebSocket::connectInsecure(QByteArray& hostname, const QByteArray& resource, const OriginWebSocketHeaders& headers )
        {
            if (!hostname.startsWith("ws://"))
            {
                hostname = QByteArray("ws://") + hostname;
            }
            return connect(hostname, resource, headers );
        }

        bool OriginWebSocket::connectSecure(QByteArray& hostname, const QByteArray& resource, const OriginWebSocketHeaders& headers )
        {
            if (!hostname.startsWith("wss://"))
            {
                hostname = QByteArray("wss://") + hostname;
            }
            return connect(hostname, resource, headers );
        }

        bool OriginWebSocket::connect( const QByteArray& hostname, const QByteArray& resource, const OriginWebSocketHeaders& headers )
        {
            create();

            if (mWSAppT.isValid())
            {

                mWSAppT.mHeaders = buildHeaders(hostname, headers);
                mWSAppT.appendHeaders(mWSAppT.mHeaders);

                // get the hostname + resource
                QByteArray myUrl = hostname;
                if (!myUrl.endsWith("/") && !resource.startsWith("/"))
                {
                    myUrl += "/";
                }
                myUrl += resource;

                // finally, connect to remote peer
                int32_t iResult = mWSAppT.connect(myUrl);
                if (0==iResult)
                {
                    ORIGIN_VERIFY_CONNECT(&mUpdateTimer, SIGNAL(timeout()), this, SLOT(_update()));
                    ORIGIN_LOG_EVENT_IF(g_isLog) << "[information] OriginWebSocket update timer created.";
                    // if we connected, start update timer
                    startUpdateTimer();
                    return true;
                }
            }
            else
            {
                ORIGIN_LOG_EVENT_IF(g_isLog) << "[warning] OriginWebSocket update timer *NOT* connected.";
            }
            return false;
        }

        void OriginWebSocket::create()
        {
            mWSAppT.create();

            if (mWSAppT.isValid())
            {
                ORIGIN_LOG_EVENT_IF(g_isLog) << "[-----] OriginWebSocket created.";
            }
            else
            {
                ORIGIN_LOG_EVENT_IF(g_isLog) << "[error] OriginWebSocket NOT created.";
            }

        }

        void OriginWebSocket::disconnect()
        {
            ORIGIN_VERIFY_DISCONNECT(&mUpdateTimer, SIGNAL(timeout()), this, SLOT(_update()));
            mUpdateTimer.stop();
            ORIGIN_LOG_EVENT_IF(g_isLog) << "[information] OriginWebSocket update timer disconnected.";
            if (mWSAppT.isConnected())
            {
                mWSAppT.disconnect();
            }
            destroy();
            emit disconnected();
        }


        void OriginWebSocket::destroy()
        {
            mWSAppT.destroy();
            ORIGIN_LOG_EVENT_IF(g_isLog) << "[information] OriginWebSocket destroyed";
        }


        void OriginWebSocket::sendMessage(QByteArray message )
        {
            mWSAppT.setSendBuffer(message.data(), message.size());
            ORIGIN_ASSERT(mWSAppT.pSendBuf);
        }

        bool OriginWebSocket::isValid() const
        {
            return mWSAppT.isValid();
        }

        bool OriginWebSocket::isConnected() const
        {
            return mWSAppT.isConnected();
        }

        int32_t OriginWebSocket::control( int32_t iCmd, int32_t iValue1, int32_t iValue2, void* pValue )
        {
            // issue the control call
            return mWSAppT.control(iCmd, iValue1, iValue2, pValue);
        }

        int32_t OriginWebSocket::status( int32_t iSelect, QString& result ) const
        {
            return mWSAppT.status(iSelect, result);
        }

        void OriginWebSocket::dealWithDisconnection(int32_t iResult)
        {
            // retrieve last web socket close reason before disposing of web socket object.
            mLwss.reset();
            mLwss.mErrorStatus = iResult;
            mLwss.mStatus = status('crsn', mLwss.mErrorStr);
            disconnect();
            emit socketError(iResult);
        }

        int32_t OriginWebSocket::connectionStatus() const
        {
            static QString result;
            static int32_t mystatus = -1;
            static int32_t myPrevstatus = mystatus;
            mystatus = status('stat', result);

            if (myPrevstatus != mystatus)
            {
                ORIGIN_LOG_EVENT_IF(g_isLog) << (1==mystatus ? "[####] " : "[information] ") << "OriginWebSocket connection status: " << mystatus << " result: " << (1==mystatus ? " connected." : " not connected.");
                myPrevstatus = mystatus;
            }
            return mystatus;
        }

        void OriginWebSocket::_update()
        {
            if (!mWSAppT.isValid())
                return;
            // give life to the module
            // update the module
            ProtoWebSocketUpdate(mWSAppT.pWebSocket);

            if (!mWSAppT.mIsConnected)
            {
                int32_t mystatus = connectionStatus();
                mWSAppT.mIsConnected =  1==mystatus;
                if (mWSAppT.mIsConnected)
                {
                    mWSAppT.applyControlCommands();
                    emit connected();
                }

#ifdef _DEBUG
                if(0) // comment this line out to test sudden disconnection
                {
                    if (1==mystatus)
                    {
                        static int count = 0;
                        if (count++ < 3)
                        {
                            dealWithDisconnection(count+1); // this line disonnects the web socket
                            return;
                        }
                    }
                }
#endif // _DEBUG
            }

            if (!mWSAppT.pWebSocket)
            {
                ORIGIN_LOG_EVENT_IF(g_isLog) << "[_-_-_-_-_-]  ProtoWebSocket update timer called when Web Socket is invalid.";
                return;
            }

            int32_t iResult = ProtoWebSocketRecv(mWSAppT.pWebSocket, mWSAppT.strRecvBuf, sizeof(mWSAppT.strRecvBuf)-1);
            // try and receive some data
            if (iResult > 0)
            {
                // null terminate it so we can print it
                mWSAppT.strRecvBuf[iResult] = '\0';
                ORIGIN_LOG_EVENT_IF(g_isLog) << "OriginWebSocket Received server frame: " << mWSAppT.strRecvBuf;
                QByteArray msg = mWSAppT.strRecvBuf;
                emit newMessage(msg);
                // print it
                NetPrintWrap(mWSAppT.strRecvBuf, 80);
            }

            if (0>iResult)
            {
                dealWithDisconnection(iResult);
            }

            // try and send some data
            if (mWSAppT.pSendBuf != NULL)
            {
                if ((iResult = ProtoWebSocketSend(mWSAppT.pWebSocket, mWSAppT.pSendBuf+mWSAppT.iSendOff, mWSAppT.iSendLen-mWSAppT.iSendOff)) > 0)
                {
                    mWSAppT.iSendOff += iResult;
                    if (mWSAppT.iSendOff == mWSAppT.iSendLen)
                    {
                        ORIGIN_LOG_EVENT_IF(g_isLog) << "OriginWebSocket sent " << mWSAppT.iSendLen <<" byte message: " << mWSAppT.pSendBuf;
                        mWSAppT.resetSendBuffer();
                    }
                }
                else if (0>iResult)
                {
                    ORIGIN_LOG_EVENT_IF(g_isLog) << "[error] OriginWebSocket sending message failed: " << iResult;
                    dealWithDisconnection(iResult);
                }
            }
        }

        void OriginWebSocket::startUpdateTimer()
        {
            mUpdateTimer.start(16);
        }

        LastWebSocketStatus OriginWebSocket::lastWebSocketCloseReason()
        {
            return mLwss;
        }

    }
}


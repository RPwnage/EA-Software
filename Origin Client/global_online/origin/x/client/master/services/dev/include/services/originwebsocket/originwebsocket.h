///////////////////////////////////////////////////////////////////////////////
// originwebsocket.h
// Wrapper around DirtySDK's implementation of protowebsocket.
// Created by J. Rivero
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ORIGINWEBSOCKET_H_INCLUDED
#define ORIGINWEBSOCKET_H_INCLUDED

#include <QObject>
#include <QByteArray>
#include <QMultiMap>
#include <QMap>
#include <QTimer>

#include "services/rest/OriginClientFactory.h"
#include "services/plugin/PluginAPI.h"

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protowebsocket.h"

namespace Origin
{
    namespace Services
    {

        class OriginWebSocket;

        namespace WebSocketPriv
        {
            class ORIGIN_PLUGIN_API WebSocket
            {
                struct DirtyControl
                {
                    int32_t iCmd;
                    int32_t iValue1; 
                    int32_t iValue2;
                    void* pValue;
                    DirtyControl(int32_t icmd = 0,int32_t ivalue1 = 0,int32_t ivalue2 = 0,void* pvalue = NULL) : 
                    iCmd(icmd)
                    ,iValue1(ivalue1) 
                    ,iValue2(ivalue2)
                    ,pValue(pvalue)
                    {

                    }
                };

                QMap<int32_t, DirtyControl> mDirtyControlVector;

                friend class Origin::Services::OriginWebSocket;

                enum WebSocketBuffersize
                {
                    BUFFER_SIZE = 1024
                };
                ProtoWebSocketRefT *pWebSocket;
                char *pSendBuf;
                int32_t iSendLen;
                int32_t iSendOff;
                char strRecvBuf[BUFFER_SIZE];
                bool mIsConnected;
                QByteArray mHeaders;
                WebSocket();

                ~WebSocket();

                /// \brief returns true if the socket is connected
                bool isConnected() const;

                /// \brief creates protowebsocket
                void create();

                /// \brief disconnects protowebsocket
                int32_t disconnect();

                /// \brief destroys protowebsocket
                void destroy();

                /// \brief cleans member variables 
                void reset();

                /// \brief cleans send member variables 
                void resetSendBuffer();

                /// \brief cleans receive member variables 
                void resetReceiveBuffer();

                /// \brief appends header
                void appendHeaders(QByteArray& myHeaders);

                /// \brief connects web socket
                int32_t connect(QByteArray& myUrl);

                /// \brief sets message to be sent
                void setSendBuffer(const char* mydata, size_t size);

                /// \brief tests validity of web socket
                bool isValid() const;

                /// \brief issues control command
                int32_t control( int32_t iCmd, int32_t iValue1, int32_t iValue2, void* pValue );

                void addToCtrlQueue( int32_t iCmd, int32_t iValue1, int32_t iValue2, void* pValue );

                /// \brief issues status command
                int32_t status( int32_t iSelect, QString& result ) const;

                /// \brief apply control commands
                void applyControlCommands();

                // disable copy ctor and assign operator
                WebSocket(const WebSocket&);
                WebSocket& operator=(const WebSocket&);
            };
        }

        struct OriginWebSocketHeaders: public QMultiMap<QByteArray, QByteArray>
        {
        };

        /// \brief store last status before resetting socket
        struct LastWebSocketStatus
        {
            int32_t mStatus;
            int32_t mErrorStatus;
            QString mErrorStr;
            LastWebSocketStatus() : mStatus(-1), mErrorStatus(-1) {}
            void reset() 
            {
                mStatus = -1;
                mErrorStatus = -1;
                mErrorStr.clear();
            }
        };

        class ORIGIN_PLUGIN_API OriginWebSocket : public QObject
        {
            Q_OBJECT
        public:
            friend class Origin::Services::OriginClientFactory<OriginWebSocket>;

            /// DTOR
            ~OriginWebSocket();

            /// \brief returns true if the socket is connected
            bool isConnected() const;

            /// \brief returns instance of OriginWebSocket
            static OriginWebSocket* instance();

            /*********************************************************************************/
            /*!
                \Function ProtoWebSocketControl

                \Description
                    Control module behavior

                \Input iSelect              - control selector
                \Input iValue               - selector specific
                \Input iValue2              - selector specific
                \Input *pValue              - selector specific

                \Output
                    int32_t                 - selector specific

                \Notes
                    Selectors are:

                    \verbatim
                        SELECTOR    DESCRIPTION
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
                    \endverbatim

                    Unhandled selectors are passed on to ProtoSSL.

                \Version 11/30/2012 (jbrookes)
            */
            /********************************************************************************F*/
            int32_t control(int32_t iCmd, int32_t iValue1 = 0, int32_t iValue2 = 0, void* pValue = NULL);

            /// \brief destroys OriginWebSocket.
            void destroy();

            /*F********************************************************************************/
            /*!
                \Function ProtoWebSocketStatus

                \Description
                    Get module status
                    

                    \Input iSelect              - status selector
                    \Input result               - String result stored in passed parameter "result"

                \Output
                    int32_t                 - selector specific

                \Notes
                    Selectors are:

                \verbatim
                    SELECTOR    RETURN RESULT
                    'crsn'      returns close reason (PROTOWEBSOCKET_CLOSEREASON_*)
                    'essl'      returns protossl error state
                    'host'      current host copied to output buffer
                    'port'      current port
                    'stat'      connection status (-1=not connected, 0=connection in progress, 1=connected)
                    'time'      TRUE if the client timed out the connection, else FALSE
                \endverbatim

                \Version 11/26/2012 (jbrookes)
            */
            /********************************************************************************F*/
            int32_t status(int32_t iSelect, QString& result) const;

            /// \func sendMessage
            /// \brief sends message
            /// \param message to send
            void sendMessage(QByteArray message = QByteArray());

            /// \brief returns true if the socket is valid
            bool isValid() const;

            ///
            /// \func connectInsecure
            /// \brief establish the connection to the DirtyBits server
            /// \param hostname to connect to
            /// \param resource to utilize
            /// \param headers for connection
            bool connectInsecure(QByteArray& hostname, const QByteArray& resource, const  OriginWebSocketHeaders& headers);

            ///
            /// \func connectSecure
            /// \brief establish the connection to the DirtyBits server
            /// \param hostname to connect to
            /// \param resource to utilize
            /// \param headers for connection
            bool connectSecure(QByteArray& hostname, const QByteArray& resource, const  OriginWebSocketHeaders& headers);

            /// 
            /// \func lastWebsocketStatus
            /// \brief returns last close reason status from web socket
            static LastWebSocketStatus lastWebSocketCloseReason();

signals:
            void connected();
            void disconnected();
            void newMessage(QByteArray);
            void socketError(int32_t);

        public slots:
            /// \func disconnect
            /// \brief disconnects dirty bits
            void disconnect();

        private slots:
            //
            // \func update
            // \brief keeps the socket alive
            void _update();

        private:
            /// \brief CTOR
            explicit OriginWebSocket(QObject *parent = 0);

            /// \brief WebSocket object
            WebSocketPriv::WebSocket mWSAppT;

            /// \brief to keep connection alive
            QTimer mUpdateTimer;

            /// \brief Last Web Socket Status
            static LastWebSocketStatus mLwss;

            /// \brief creates OriginWebSocket
            void create();

            ///
            /// \func connect
            /// \brief establish the connection to the DirtyBits server
            /// \param hostname to connect to
            /// \param resource to utilize
            /// \param headers for connection
            bool connect(const QByteArray& hostname, const QByteArray& resource, const  OriginWebSocketHeaders& headers);

            /// \brief starts timer
            void startUpdateTimer();

            /// \brief process required code when disconnection is detected
            void dealWithDisconnection(int32_t);

            /// \brief requests connection status
            int32_t connectionStatus() const;

            // disable copy ctor and assign operator
            OriginWebSocket(const OriginWebSocket&);
            OriginWebSocket& operator=(const OriginWebSocket&);
        };

        // Sets/unsets logging flag (prototype)
        ORIGIN_PLUGIN_API void setLogWebSocket(bool val);
    }
}

#endif // !ORIGINWEBSOCKET_H_INCLUDED


///////////////////////////////////////////////////////////////////////////////
//
//  Heartbeat.h
//
//  Copyright (c) 2011, Electronic Arts Inc. All rights reserved.
//
//	Description
//	http://online.ea.com/confluence/display/EBI/Heartbeat+Monitoring
//
///////////////////////////////////////////////////////////////////////////////

#ifndef HEARTBEAT_H
#define HEARTBEAT_H


///////////////////////////////////////////////////////////////////////////////
//  INCLUDE
///////////////////////////////////////////////////////////////////////////////

#include <QDateTime>
#include <QFile>
#include <QList>
#include <QMap>
#include <QMutex> 
#include <QNetworkReply>
#include <QTextStream>
#include <QTimer>
#include <QAtomicInt>

#include "services/plugin/PluginAPI.h"

///////////////////////////////////////////////////////////////////////////////
//  DEFINES
///////////////////////////////////////////////////////////////////////////////

#define HEARTBEAT_RESPONSEMAX (20)


///////////////////////////////////////////////////////////////////////////////
//  DEFINES
///////////////////////////////////////////////////////////////////////////////

namespace Origin
{
    namespace Services
    {
        class NetworkAccessManager;
        class ClientNetworkConfigReader;
        class ClientNetworkConfig;

        ///////////////////////////////////////////////////////////////////////////////
        //  CLASS
        ///////////////////////////////////////////////////////////////////////////////

        struct HeartbeatNetworkResponse : public QObject
        {
            Q_OBJECT

            //=========================================================================
            //	CONSTRUCTOR/DESTRUCTOR
            //=========================================================================
            private:
                QNetworkReply* mpReply;  // Reference only
                int mHandle;

            public:
                ORIGIN_PLUGIN_API HeartbeatNetworkResponse(QNetworkReply* pReply);
                ORIGIN_PLUGIN_API virtual ~HeartbeatNetworkResponse();

            private:
                HeartbeatNetworkResponse(){} // inaccessible

                public slots:
                    ORIGIN_PLUGIN_API void onNetworkData(qint64 bytesRx, qint64 bytesTotal);
                    ORIGIN_PLUGIN_API void onNetworkFinish();

            private:
                QString getTransactionType();
        };


        ///////////////////////////////////////////////////////////////////////////////
        //  STRUCT
        ///////////////////////////////////////////////////////////////////////////////

        struct MessageStruct
        {
            int mResponseTimeCount;
            int mResponseTimeMax;
            int mResponseTimeTotal;
            int mErrorCode; // last one only
            int mErrorCount;
            MessageStruct() : 
                mResponseTimeCount(0), 
                mResponseTimeMax(0),
                mResponseTimeTotal(0), 
                mErrorCode(0),
                mErrorCount(0) {}
        };


        ///////////////////////////////////////////////////////////////////////////////
        //  CLASS
        ///////////////////////////////////////////////////////////////////////////////

        /// \brief Heartbeat is used to report real-time vitals of the Origin client.
        /// 
        /// The Heartbeat works by listening to the NetworkAccessManagerFactory
        /// "networkAccessManagerCreated" signal, which allows it to know when a new NAM
        /// is constructed and therefore can hook into its network requested created/
        /// completed events and see if there is anything relevant to report.
        ///
        /// Therefore, the Heartbeat must be created shortly after the 
        /// NetworkAccessManagerFactory is initialized, before the factory has created
        /// any new NAM objects, otherwise signals from the created NAM won't be detected
        /// by the heartbeat, causing bugs (EBIBUGS-21977).
        ///
        /// \note The heartbeat host has been traditionally defined as: heartbeat.dm.origin.com
        ///
        /// \sa NetworkAccessManagerFactory, NetworkAccessManager
        class ORIGIN_PLUGIN_API Heartbeat : public QObject
        {
            Q_OBJECT

            //=========================================================================
            //	CONSTRUCTOR/DESTRUCTOR
            //=========================================================================
            public:

                /// \brief Readies the Heartbeat for reporting vital client metrics.
                /// 
                /// Initializes Heartbeat by listening for NetworkAccessManagerFactory's 
                /// "networkAccessManagerCreated" signal.
                ///
                /// \sa NetworkAccessManagerFactory, NetworkAccessManagerFactory::networkAccessManagerCreated
                Heartbeat(QObject* parent = NULL);
                virtual ~Heartbeat();

            //=========================================================================
            //	SINGLETON
            //=========================================================================
            private:
                static Heartbeat* mpSingleton;

            public:
                static Heartbeat* instance()
                {
                    return mpSingleton;
                }
                static void create();
                static void destroy();

            public:
                void initialize();

            //=========================================================================
            //	CONFIGURATION
            //=========================================================================
            private:
                QString mMessageServerName;
                int mMessageInterval;

            private:
                QMutex mConfigMutex;
                ClientNetworkConfigReader* mpConfigReader;
                QTimer mConfigTimer;

            private:
                void setConfigInterval(int intervalMs);

            public:
                void setConfigServerName(const QString& serverName);

            //=========================================================================
            //	DATA LOGGING:  ERRORS
            //=========================================================================
            public:
                struct ErrorStruct
                {
                    int mErrorCount;
                    int mLastErrorCode;
                    QString mTransactionType;
                    ErrorStruct() : mErrorCount(0), mLastErrorCode(0) {}
                };

            private:
                QList<ErrorStruct> mErrors;

            public:
                void resetErrors();

            //=========================================================================
            //	DATA LOGGING:  RESPONSE TIMES
            //=========================================================================
            private:
                struct ResponseTimeStruct
                {
                    int mTime;
                    QString mTransactionType;
                    ResponseTimeStruct() : mTime(0) {}
                };

            private:
                ResponseTimeStruct mResponseTime[HEARTBEAT_RESPONSEMAX]; // Circular buffer on responses
                int mResponseStart;
                int mResponseCount;

            public:
                void resetReponses();

            //=========================================================================
            //	DATA LOGGING:  START TIMES
            //=========================================================================
            private:
                struct StartTimeStruct
                {
                    int mHandle;
                    QDateTime mTime;
                    StartTimeStruct() : mHandle(-1) {}
                };

            private:
                int mHandleCounter;

            private:
                bool extractStartTimeByHandle(int handle, StartTimeStruct& startTime);

            private:
                QList<StartTimeStruct> mStartTime;

            //=========================================================================
            //	DOWNLOAD ERRORS
            //=========================================================================
            private:
                int mDownloadErrorCode; // Error type from downloader
                QString mCdnVendor;

            public:
                void setDownloadError(const int errorCode, const QString& cdnVendor);

            //===
            //   DIRTYBITS HEALTH
            //===
            private:
                bool mDirtyBitsInfoReceived;
                bool mDirtyBitsConnected;
                int mDirtyBitsMessagesReceived;

            public:
                void updateDirtyBitsInfo(bool connected, bool messageReceived=false);

            //=========================================================================
            //	LOG FILE
            //=========================================================================
            private:
                QFile mLogFile;
                QTextStream mLogStream;

            private:
                void logClose();
                void logWrite(const QString& logString);

            public:
                void logOpen();

            //=========================================================================
            //	LOGIN ERRORS
            //=========================================================================
            private:
                int mLoginErrorCode; // REST error code from login server

            public:
                void resetLoginError();
                void setLoginError(const int errorCode);

            //=========================================================================
            //	CDN CATALOG DEFINITIONS
            //=========================================================================
            private:
                QAtomicInt mCatalogLookupSuccessCount;
                QAtomicInt mCatalogLookupFailCount;
                QAtomicInt mCatalogLookupUnchangedCount;

            public:
                void catalogLookupSuccess();
                void catalogLookupFailed();
                void catalogLookupUnchanged();

            //=========================================================================
            //	MESSAGE
            //=========================================================================
            private:
                QTimer mMessageTimer;
                QNetworkReply* mpMessage;

            private:
                void createMessageData(QString& data);

                private slots:
                    void sendHeartbeat();

            //=========================================================================
            //	TRANSACTION INTERFACE:  BASIC
            //=========================================================================
            private:
                QString mUsername; // MD5 of email, empty string if not logged in
                QMutex mMutex;

            public:
                void onLogin(const QString& username);
                void onLogoff();

                int  onTransactionCreate();
                void onTransactionError(int handle, const QString& transactionType, int errorCode);
                void onTransactionIgnore(int handle);
                void onTransactionSuccess(int handle, const QString& transactionType);

            //=========================================================================
            //	TRANSACTION INTERFACE:  NETWORK
            //=========================================================================
            public:
                //	Register QNetworkReply to determine when reply started
                QMap<QNetworkReply*, HeartbeatNetworkResponse*> mNetworkReply;
                QMutex mNetworkReplyMutex;

                public slots:
                    void onNetworkAccessManagerCreated(Origin::Services::NetworkAccessManager *);
                    void onNetworkRequest(const QNetworkRequest &, QNetworkReply *pReply);
                    void onNetworkReply(QNetworkReply* pReply);
        };
    }
}


#endif
///////////////////////////////////////////////////////////////////////////////
//
//  Heartbeat.cpp
//
//  Copyright (c) 2011, Electronic Arts Inc. All rights reserved.
//
//	Description
//	http://online.ea.com/confluence/display/EBI/Heartbeat+Monitoring
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  INCLUDE
///////////////////////////////////////////////////////////////////////////////

#include "services/heartbeat/Heartbeat.h"
#include "services/network/ClientNetworkConfigReader.h"
#include "services/settings/SettingsManager.h"
#include "services/network/NetworkAccessManager.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "services/debug/DebugService.h"
#include "services/crypto/md5.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/session/SessionService.h"

#include <EAIO/EAFileUtil.h>
#include <QTimerEvent>
#include <QDir.h>
 
namespace Origin
{
    namespace Services
    {
        ///////////////////////////////////////////////////////////////////////////////
        //	DEFINES
        ///////////////////////////////////////////////////////////////////////////////

        #define HEARTBEAT_SENDMESSAGEENABLE    // Enable to allow send heartbeat messages
        //#define HEARTBEAT_LOGAUTOENABLE        // Enable to allow heartbeat to data to file
        //#define HEARTBEAT_DEBUGGING            // Enable to use debugging settings

        //#define HEARTBEAT_DISABLETRANSACTIONS


        //	DEBUG:  Messages
        #if defined(HEARTBEAT_DEBUGGING) 
            #define HEARTBEAT_DEBUGNETWORKTOCONSOLE
            #define HEARTBEAT_DEBUGMESSAGETOCONSOLE
            #define HEARTBEAT_DEBUGRESPONSETOCONSOLE
        #endif

        //	Maps urls to more friendly names.  Note urls expanded are place last
        #define SERVICEURLDEFS\
            SERVICEURLDEF(ConnectPortalBaseUrl,      url_connectportal)\
            SERVICEURLDEF(ActivationURL,             url_activation)\
            SERVICEURLDEF(SupportURL,                url_support)\
            SERVICEURLDEF(EbisuForumURL,             url_forum)\
            SERVICEURLDEF(HeartbeatURL,		         url_heartbeat)\
            SERVICEURLDEF(GamerProfileURL,           url_gameprofile)\
            SERVICEURLDEF(PromoURL,                  url_promo)\
            SERVICEURLDEF(WizardURL,                 url_wizard)\
            SERVICEURLDEF(NewUserExpURL,             url_newuserexp)\
            SERVICEURLDEF(EcommerceURL,              url_ecommerce)


        ///////////////////////////////////////////////////////////////////////////////
        //  CONSTANTS
        ///////////////////////////////////////////////////////////////////////////////

        //	Number of errors in message
        static const int gMessageErrorsMax = 5;  // Send the top 5 errors

        #if defined(HEARTBEAT_DEBUGGING)
            //	Debugging values
            static const int gConfigInterval = (10 * 1000);
            static const QString gsConfigServerName = "http://static.cdn.ea.com/ebisu/u/f/ClientConfig.xml";
            static const int gMessageInterval    = (15 * 1000);
            static const int gMessageIntervalMin = (15 * 1000);
        #else
            //	Production values
#if 0
            static const int gConfigInterval = (60 * 60 * 1000); //	Interval to retrieve config
#endif
            static const QString gsConfigServerName = "http://static.cdn.ea.com/ebisu/u/f/ClientConfig.xml";
            static const int gMessageInterval    = (60 * 1000); //	Interval to send messages
            static const int gMessageIntervalMin = (60 * 1000); // 60 seconds minimum
        #endif



        ///////////////////////////////////////////////////////////////////////////////
        //  GLOBALS
        ///////////////////////////////////////////////////////////////////////////////

        Heartbeat* Heartbeat::mpSingleton = NULL;


        ///////////////////////////////////////////////////////////////////////////////
        //  CLASS
        ///////////////////////////////////////////////////////////////////////////////

        //-----------------------------------------------------------------------------
        //	CONSTRUCTOR
        //-----------------------------------------------------------------------------
        HeartbeatNetworkResponse::HeartbeatNetworkResponse(QNetworkReply* pReply) :
            mpReply(pReply),
            mHandle(-1)
        {
            //	Setup the signal
            ORIGIN_VERIFY_CONNECT(pReply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(onNetworkData(qint64, qint64)));
            ORIGIN_VERIFY_CONNECT(pReply, SIGNAL(finished()), this, SLOT(onNetworkFinish()));

            //	Initiate start time
            mHandle = Heartbeat::instance()->onTransactionCreate();
        }

        //-----------------------------------------------------------------------------
        //	DESTRUCTOR
        //-----------------------------------------------------------------------------
        HeartbeatNetworkResponse::~HeartbeatNetworkResponse()
        {
            //	Remove and ignore transaction if handle still exists
            if (mHandle >= 0)
            {
                Heartbeat::instance()->onTransactionIgnore(mHandle);
                mHandle = -1;
            }
        }

        //-----------------------------------------------------------------------------
        //
        //-----------------------------------------------------------------------------
        void HeartbeatNetworkResponse::onNetworkData(qint64 bytesRx, qint64 bytesTotal)
        {
            Q_UNUSED(bytesTotal)

            //	Skip processing callback if no data has been received (might be error)
            if (bytesRx <= 0)
                return;

            //	Skip processing if we don't have a valid handle
            if (mHandle < 0)
                return;

            //	DEBUG:  Print out the URL of Network's HTTP request
            #ifdef HEARTBEAT_DEBUGNETWORKTOCONSOLE
                QString url(mpReply->request().url().toString());
                QByteArray urlascii = url.toAscii();
                qDebug("OnNetworkData: %s %d %d", urlascii.constData(), static_cast<int>(bytesRx), static_cast<int>(bytesTotal));
            #endif

            //	Notify heartbeat that transaction was successful
            QString transactionType(getTransactionType());
            if (transactionType.startsWith(QString("url_")) == true)
            {
                Heartbeat::instance()->onTransactionSuccess(mHandle, transactionType);
            }
            else
            {
                #ifdef HEARTBEAT_DEBUGNETWORKTOCONSOLE
                    qDebug("OnNetworkData:  Ignore");
                #endif
                Heartbeat::instance()->onTransactionIgnore(mHandle);
            }
            mHandle = -1;
        }

        //-----------------------------------------------------------------------------
        //
        //-----------------------------------------------------------------------------
        void HeartbeatNetworkResponse::onNetworkFinish()
        {
            //	Skip processing if we don't have a valid handle
            if (mHandle < 0)
                return;

            //	DEBUG:  Print out the URL of Network's HTTP request
            #ifdef HEARTBEAT_DEBUGNETWORKTOCONSOLE
                QString url(mpReply->request().url().toString());
                QByteArray urlascii = url.toAscii();
                qDebug("OnNetworkFinish: %s", urlascii.constData());
            #endif

            //	Finish transaction 
            QString transactionType(getTransactionType());
            if (transactionType.startsWith(QString("url_")) == true)
            {
                QNetworkReply::NetworkError error = mpReply->error();
                if (error == QNetworkReply::NoError)
                {
                    Heartbeat::instance()->onTransactionSuccess(mHandle, transactionType);
                }
                else
                {
                    //	DEBUG:  Print out the error code
                    #ifdef HEARTBEAT_DEBUGNETWORKTOCONSOLE
                        qDebug("OnNetworkFinish ERROR: %d - %s ", error, mpReply->errorString().toAscii().constData());
                    #endif

                    Heartbeat::instance()->onTransactionError(mHandle, 
                        transactionType, -static_cast<int>(error)); // Qt errors mapped in negated form
                }
            }
            else
            {
                #ifdef HEARTBEAT_DEBUGNETWORKTOCONSOLE
                    qDebug("OnNetworkFinish:  Ignore");
                #endif

                Heartbeat::instance()->onTransactionIgnore(mHandle);
            }
    
            mHandle = -1;
        }

        //-----------------------------------------------------------------------------
        //	Here we process the replied URL and convert it to the corresponding
        //	transaction type.
        //-----------------------------------------------------------------------------
        QString HeartbeatNetworkResponse::getTransactionType()
        {
            QString url(mpReply->request().url().toString());

            //	Match the URL with the service type
            #define SERVICEURLDEF(SettingName, TransactionType)\
                if (url.contains(Origin::Services::readSetting(Origin::Services::SETTING_##SettingName).toString(), Qt::CaseInsensitive) == true) return #TransactionType;

                SERVICEURLDEFS
            #undef SERVICEURLDEF

            //	Default URL
            return mpReply->request().url().host();
        }


        ///////////////////////////////////////////////////////////////////////////////
        //  CLASS
        ///////////////////////////////////////////////////////////////////////////////

        //-----------------------------------------------------------------------------
        //	CONSTRUCTOR
        //-----------------------------------------------------------------------------
        Heartbeat::Heartbeat(QObject* parent) : 
            QObject(parent),
            mMessageInterval(gMessageInterval),
            mpConfigReader(NULL),
            mResponseStart(0),
            mResponseCount(0),
            mHandleCounter(1000), // arbitrary
            mDownloadErrorCode(0),
            mDirtyBitsInfoReceived(false),            
            mDirtyBitsConnected(false),
            mDirtyBitsMessagesReceived(0),
            mLoginErrorCode(0),
            mCatalogLookupSuccessCount(0),
            mCatalogLookupFailCount(0),
            mCatalogLookupUnchangedCount(0),
            mpMessage(NULL)
        {
            // Hook any new network access managers
            ORIGIN_VERIFY_CONNECT_EX(Origin::Services::NetworkAccessManagerFactory::instance(),
                             SIGNAL(networkAccessManagerCreated(Origin::Services::NetworkAccessManager *)),
                             this,
                             SLOT(onNetworkAccessManagerCreated(Origin::Services::NetworkAccessManager *)),
                             Qt::DirectConnection);
        }

        //-----------------------------------------------------------------------------
        //	DESTRUCTOR
        //-----------------------------------------------------------------------------
        Heartbeat::~Heartbeat()
        {
            //	Delete and destroy data
            {
                QMutexLocker locker(&mNetworkReplyMutex);
                QMapIterator<QNetworkReply*, HeartbeatNetworkResponse*> i(mNetworkReply);
                while (i.hasNext())
                {
                    i.next();
                    delete i.value();
                }	
                mNetworkReply.clear();
            }

            //	Close the log file
            logClose();
        }


        //=============================================================================
        //  SINGLETON
        //=============================================================================

        //-----------------------------------------------------------------------------
        //	STATIC:  Creates and initializes singleton
        //-----------------------------------------------------------------------------
        void Heartbeat::create()
        {
            //	Instantiate singleton if necessary
            if (mpSingleton == NULL)
            {
                mpSingleton = new Heartbeat;
            }
        }

        //-----------------------------------------------------------------------------
        //  STATIC:  Cleans up and destroys singleton
        //-----------------------------------------------------------------------------
        void Heartbeat::destroy()
        {
            delete mpSingleton;
            mpSingleton = NULL;
        }

        //-----------------------------------------------------------------------------
        //	Set the initial state of the heartbeat
        //-----------------------------------------------------------------------------
        void Heartbeat::initialize()
        {
        #if 0	// no longer used - https://developer.origin.com/support/browse/EBISUP-2105
            //	When heartbeat is instantiated we make a request to get the 
            //	configuration file from the server if it is available.  It may contain
            //	values to override the default built-in values.  
            ORIGIN_VERIFY_CONNECT(&mConfigTimer, SIGNAL(timeout()), this, SLOT(requestConfigFromNetwork()));
            srand( (unsigned)time( NULL ) );
            int randomInterval = gConfigInterval + (rand() % gConfigInterval);
            mConfigTimer.start(randomInterval);
        #elif defined(HEARTBEAT_DEBUGGING)
            (void) gConfigInterval;
        #endif

            //	Start the message timer
            mMessageInterval = gMessageInterval;
            ORIGIN_VERIFY_CONNECT(&mMessageTimer, SIGNAL(timeout()), this, SLOT(sendHeartbeat()));
            mMessageTimer.start(mMessageInterval);

            //	Open the log file
            #ifdef HEARTBEAT_LOGAUTOENABLE
                logOpen();
            #endif
        }


        //=============================================================================
        //  CONFIGURATION
        //=============================================================================

        //-----------------------------------------------------------------------------
        //	If the 'serverName' is set then the heartbeat's server name will be
        //	overridden.  If the 'ServerName' is an empty string then the server name 
        //	will be reverted to the default built-in name.
        //-----------------------------------------------------------------------------
        void Heartbeat::setConfigServerName(const QString& serverName)
        {
            QMutexLocker locker(&mConfigMutex);

            if (serverName.isEmpty() == false)
            {
                //	Use overridden value
                mMessageServerName = serverName;
            }
            else
            {
                //	Use default value
                mMessageServerName = Origin::Services::readSetting(Origin::Services::SETTING_HeartbeatURL).toString();
            }
        }

        //-----------------------------------------------------------------------------
        //	If the 'intervalMs' is within a valid range then it overrides the default
        //	value otherwise the default built-in value will be used.
        //-----------------------------------------------------------------------------
        void Heartbeat::setConfigInterval(int intervalMs)
        {
            //	Disable heartbeat if interval is 0
            if (intervalMs == 0)
            {
                mMessageTimer.stop();
                mMessageInterval = 0;
            }
            else
            {
                //	Ensure heartbeat interval is above or equal to minimum
                int newInterval = gMessageInterval;
                if (intervalMs >= gMessageIntervalMin)
                {
                    newInterval = intervalMs;
                }

                //	Update the message timer to new interval if different
                if (newInterval != mMessageInterval)
                {
                    //	Turn on timer if necessary
                    if (mMessageTimer.isActive() == false)
                    {
                        mMessageTimer.start(newInterval);
                    }
                    else //	Modify interval if necessary
                    {
                        mMessageTimer.setInterval(newInterval);
                    }
                }
                mMessageInterval = newInterval;
            }
        }


        //=============================================================================
        //	DATA LOGGING:  ERRORS
        //=============================================================================

        //-----------------------------------------------------------------------------
        //	Resets all the errors
        //
        //	Thread:  Caller of this function MUST have mutex lock!
        //-----------------------------------------------------------------------------
        void Heartbeat::resetErrors()
        {
            mErrors.clear();
        }


        //=============================================================================
        //	DATA LOGGING:  RESPONSE TIMES
        //=============================================================================

        //-----------------------------------------------------------------------------
        //	Resets the response times
        //
        //	Thread:  Caller of this function MUST have mutex lock!
        //-----------------------------------------------------------------------------
        void Heartbeat::resetReponses()
        {
            mResponseStart = mResponseCount = 0;
        }


        //=============================================================================
        //	DATA LOGGING:  START TIMES
        //=============================================================================

        //-----------------------------------------------------------------------------
        //	Finds the start time based on handle.  Return true if found.
        //
        //	Thread:  Caller of this function MUST have mutex lock!
        //-----------------------------------------------------------------------------
        bool Heartbeat::extractStartTimeByHandle(int handle, StartTimeStruct& startTime)
        {
            //	Iterate through the start times to find entry with handle
            QMutableListIterator<StartTimeStruct> i(mStartTime);
            while (i.hasNext()) 
            {
                //	Check if we found the start time
                startTime = i.next();
                if (startTime.mHandle == handle)
                {
                    //	Remove the start time from the list
                    i.remove();

                    //	Notify that we found the start time and "startTime" is valid
                    return true;
                }
            }
            return false;
        }


        //=============================================================================
        //	DOWNLOAD ERRORS
        //=============================================================================

        //-----------------------------------------------------------------------------
        //	Add download errors to heartbeat payload
        //-----------------------------------------------------------------------------
        void Heartbeat::setDownloadError(const int errorCode, const QString& cdnVendor)
        {
            QMutexLocker locker(&mMutex);
            mCdnVendor = cdnVendor;
            mDownloadErrorCode = errorCode;
        }

        void Heartbeat::updateDirtyBitsInfo(bool connected, bool messageReceived)
        {
            QMutexLocker locker(&mMutex);
            mDirtyBitsInfoReceived = true;
            mDirtyBitsConnected = connected;

            if(messageReceived)
            {
                mDirtyBitsMessagesReceived++;
            }
        }

        //=============================================================================
        //  LOG FILE
        //=============================================================================

        //-----------------------------------------------------------------------------
        //	Close the log file
        //-----------------------------------------------------------------------------
        void Heartbeat::logClose()
        {
            mLogFile.close();
        }

        //-----------------------------------------------------------------------------
        //	Open the log file
        //-----------------------------------------------------------------------------
        void Heartbeat::logOpen()
        {
            //	Get the application's data directory
            //	Win32 example:  C:/ProgramData
            const size_t dataDirectorySize = 512;
            char16_t     dataDirectory[dataDirectorySize];
            int result = EA::IO::GetSpecialDirectory(EA::IO::kSpecialDirectoryCommonApplicationData, dataDirectory, true, dataDirectorySize);
            if (result > 0)
            {
                QString logPath(QString::fromUtf16(reinterpret_cast<ushort*>(dataDirectory), result));
                logPath.append(QDir::separator());
                logPath.append("Origin");
                logPath.append(QDir::separator());
                logPath.append("hb.log");

                //	Open and initialize log file
                mLogFile.setFileName(logPath);
                if (mLogFile.open(QIODevice::WriteOnly | QIODevice::Text) == true)
                {
                    mLogStream.setDevice(&mLogFile);
                }
            }
        }

        //-----------------------------------------------------------------------------
        //	Write log string to file
        //-----------------------------------------------------------------------------
        void Heartbeat::logWrite(const QString& logString)
        {
            if (mLogFile.handle() >= 0)
            {
                mLogStream << logString << endl;
            }
        }


        //=============================================================================
        //	LOGIN ERRORS
        //=============================================================================

        //-----------------------------------------------------------------------------
        //	Add login errors to heartbeat payload
        //-----------------------------------------------------------------------------
        void Heartbeat::setLoginError(const int errorCode)
        {
            QMutexLocker locker(&mMutex);

            mLoginErrorCode = errorCode;
        }


        //=============================================================================
        //	CDN CATALOG DEFINITIONS
        //=============================================================================

        //-----------------------------------------------------------------------------
        //	Lookup successful
        //-----------------------------------------------------------------------------
        void Heartbeat::catalogLookupSuccess()
        {
            // Don't need a mutex since this is an atomic int.
            ++mCatalogLookupSuccessCount;
        }

        //-----------------------------------------------------------------------------
        //	Lookup error
        //-----------------------------------------------------------------------------
        void Heartbeat::catalogLookupFailed()
        {
            // Don't need a mutex since this is an atomic int.
            ++mCatalogLookupFailCount;
        }

        //-----------------------------------------------------------------------------
        //	Lookup unchanged
        //-----------------------------------------------------------------------------
        void Heartbeat::catalogLookupUnchanged()
        {
            // Don't need a mutex since this is an atomic int.
            ++mCatalogLookupUnchangedCount;
        }


        //=============================================================================
        //  MESSAGE
        //=============================================================================

        //-----------------------------------------------------------------------------
        //	Construct heartbeat message
        //-----------------------------------------------------------------------------
        static bool sortErrorStruct(const Heartbeat::ErrorStruct &a, const Heartbeat::ErrorStruct &b)
        {
            return (a.mErrorCount > b.mErrorCount);
        }

        void Heartbeat::createMessageData(QString& data)
        {
            QMap<QString, MessageStruct> message;

            //	Locking to ensure that no new transactions are added while we are
            //	creating and sending the heartbeat message.  We also need to reset
            //	the logged data before unlocking to allow new data to come in.
            QMutexLocker locker(&mMutex);


            //	Populate data with response time first
            for (int r = 0; r < mResponseCount; r++)
            {
                MessageStruct& msg = message[mResponseTime[r].mTransactionType];
                msg.mResponseTimeCount++;
                msg.mResponseTimeTotal += mResponseTime[r].mTime;

                //	Save the maximum response time
                if (msg.mResponseTimeMax < mResponseTime[r].mTime)
                {
                    msg.mResponseTimeMax = mResponseTime[r].mTime;
                }
            }
    

            //	Sort errors so transactions with the most errors are first
            qSort(mErrors.begin(), mErrors.end(), sortErrorStruct);

            //	Populate the message list with transactions with the most errors
            int errorCount = 0;
            QList<ErrorStruct>::const_iterator iterError = mErrors.begin();
            while ((iterError != mErrors.end()) && (errorCount < gMessageErrorsMax))
            {
                const ErrorStruct& error = *iterError;
                MessageStruct& msg = message[error.mTransactionType];
                msg.mErrorCode = error.mLastErrorCode; // value is error code

                // Record number of errors for this transaction type
                msg.mErrorCount = error.mErrorCount;

                ++iterError;
                ++errorCount;
            }


            //	Construct HTTP message body
            QMapIterator<QString, MessageStruct> i(message);
            while (i.hasNext())
            {
                i.next();

                //	Start new transaction with &
                data += '&';

                //	Compute response time
                int AvrRspTime = 0;
                if (i.value().mResponseTimeCount > 0)
                {
                    AvrRspTime = i.value().mResponseTimeTotal / i.value().mResponseTimeCount;
                }

                //	Transaction data
                QString tx = QString("%1=%2,%3,%4,%5,%6")
                    .arg(i.key())
                    .arg(i.value().mResponseTimeCount)  // only successful network responses
                    .arg(i.value().mErrorCount)
                    .arg(AvrRspTime)
                    .arg(i.value().mResponseTimeMax)
                    .arg(i.value().mErrorCode);
                data += tx;
            }


            //	Reset logged data
            resetErrors();
            resetReponses();

            // Add successful/failed CDN catalog lookup counts since last message
            if (mCatalogLookupSuccessCount > 0 || mCatalogLookupFailCount > 0)
            {
                QString tx = QString("&catalog_lookup_counts=%1,%2,0,0,0").arg(mCatalogLookupSuccessCount).arg(mCatalogLookupFailCount);
                data += tx;
                mCatalogLookupSuccessCount = 0; // reset counter since last
                mCatalogLookupFailCount = 0; // reset counter since last
            }

            // Add unchanged CDN catalog lookup counts since last message
            if (mCatalogLookupUnchangedCount > 0)
            {
                QString tx = QString("&catalog_lookup_counts_304=%1,0,0,0,0").arg(mCatalogLookupUnchangedCount);
                data += tx;
                mCatalogLookupUnchangedCount = 0; // reset counter since last
            }

            //  Add login error code
            if (mLoginErrorCode != 0)
            {
                QString tx = QString("&loginerror%1=0,0,0,0,1").arg(mLoginErrorCode);
                data += tx;
                mLoginErrorCode = 0; // automatically reset error code
            }

            //  Add download error code
            if (mDownloadErrorCode != 0)
            {
                QString tx = QString("&%1_downloaderror%2=0,0,0,0,1").arg(mCdnVendor).arg(mDownloadErrorCode);
                data += tx;
                mDownloadErrorCode = 0; // automatically reset error code
                mCdnVendor = "";
            }

            if (mDirtyBitsInfoReceived)
            {
                if(mDirtyBitsConnected)
                {
                    QString tx = QString("&db_conn=1,0,0,0,0");
                    data += tx;
                }

                if(mDirtyBitsMessagesReceived>0)
                {
                    QString tx = QString("&db_msgcount=%1,0,0,0,0").arg(mDirtyBitsMessagesReceived);
                    data += tx;
                    mDirtyBitsMessagesReceived = 0; // reset counter since last
                }
            }
        }

        //-----------------------------------------------------------------------------
        //	Construct and send heartbeat message to server
        //-----------------------------------------------------------------------------
        void Heartbeat::sendHeartbeat()
        {
            //	Locking to ensure that the server name does not change while
            //	we are building the URL.
            QString urlString;
            {
                QMutexLocker locker(&mConfigMutex);
                urlString = mMessageServerName;
            }

            //	If server string is not set then skip 
            if (urlString.isEmpty() == true)
            {
                return;
            }

            //	Construct URL
            if (mUsername.isEmpty() == true)
            {
                urlString += "/pulse?off&user=0";
            }
            else
            {
                //  Authenticated user:  Determine if online or offline
                if (Connection::ConnectionStatesService::isUserOnline(Origin::Services::Session::SessionService::instance()->currentSession()))
                {
                    urlString += "/pulse?authon&user=";
                }
                else
                {
                    urlString += "/pulse?authoff&user=";
                }
                urlString += mUsername;
            }

    
            //	Construct message data
            QString dataString;
            createMessageData(dataString);
            QByteArray data(dataString.toLatin1());


            //	DEBUG:  Send message to console
            #ifdef HEARTBEAT_DEBUGMESSAGETOCONSOLE
                qDebug("Message URL = %s", urlString.toAscii().constData());
                qDebug("Message Data = %s", data.constData());
            #endif

            logWrite(QString("HURL: %1").arg(urlString));
            logWrite(QString("HDAT: %1").arg(dataString));

    
            //disable sending the message only if there's not an internet connection or we're going to sleep (losing internet connection occurs as a result of sleep but sleep happens first)
            //this is to prevent some systems from trying to connect to a WiFi in search of a connection once it is lost
            bool isComputerAsleep = !(Origin::Services::Connection::ConnectionStatesService::connectionField (Origin::Services::Connection::GLOB_OPR_IS_COMPUTER_AWAKE) == Origin::Services::Connection::CS_TRUE);
            bool isInternetDown = !(Origin::Services::Connection::ConnectionStatesService::connectionField(Origin::Services::Connection::GLOB_OPR_IS_INTERNET_REACHABLE) == Origin::Services::Connection::CS_TRUE);
            if (!isComputerAsleep && !isInternetDown)
            { 
                #ifdef HEARTBEAT_SENDMESSAGEENABLE
                    urlString += dataString;	//	Combine the URL with data
                    QUrl source(urlString);
                    QNetworkRequest request(source);
                    // Make sure there are no cookies attached to this unencrypted heartbeat request
                    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
                    mpMessage = Origin::Services::NetworkAccessManager::instance()->get(request);
                #endif
            }
            /*
            else
            {
                //	If previous message did NOT finished successfully then we skip
                //	sending the heartbeat message otherwise we will leak NetworkReply's
                //	that are not deleted properly.

                #ifdef HEARTBEAT_DEBUGMESSAGETOCONSOLE
                    qDebug("Previous message NOT finished");
                #endif

                #if 0
                    mpMessage->abort();
                    mpMessage->deleteLater();
                    mpMessage = NULL;
                #endif
            }
            */
        }


        //=============================================================================
        //	TRANSACTION INTERFACE:  BASIC
        //=============================================================================

        //-----------------------------------------------------------------------------
        //
        //-----------------------------------------------------------------------------
        void Heartbeat::onLogin(const QString& username)
        {
            QMutexLocker locker(&mMutex);

            //	Convert username to lower case 8-bit ASCII
            QByteArray asciiUsername(username.toLower().toLatin1());

            //	Convert username to md5 in string form
            std::string md5(calculate_md5(asciiUsername.constData(), asciiUsername.length()));
            mUsername = md5.c_str();
        }

        //-----------------------------------------------------------------------------
        //
        //-----------------------------------------------------------------------------
        void Heartbeat::onLogoff()
        {
            QMutexLocker locker(&mMutex);

            //	Empty user name string means we are logged out.
            mUsername.clear();
        }

        //-----------------------------------------------------------------------------
        //
        //-----------------------------------------------------------------------------
        int Heartbeat::onTransactionCreate()
        {
        #ifdef HEARTBEAT_DISABLETRANSACTIONS
            return 0;
        #else
            QMutexLocker locker(&mMutex);

            //	TODO:  Sanity check to remove stale start times in case some
            //	transactions never ended properly.

            //	Define handle to start time
            int returnHandle = mHandleCounter++;

            //	Add a start time data to list
            StartTimeStruct startTime; 
            startTime.mHandle = returnHandle;
            startTime.mTime = QDateTime::currentDateTime();
            mStartTime.append(startTime);

            return returnHandle;
        #endif
        }

        //-----------------------------------------------------------------------------
        //
        //-----------------------------------------------------------------------------
        void Heartbeat::onTransactionError(int handle, const QString& transactionType, int errorCode)
        {
        #ifdef HEARTBEAT_DISABLETRANSACTIONS
            Q_UNUSED(handle);
            Q_UNUSED(transactionType);
            Q_UNUSED(errorCode);
        #else
            QMutexLocker locker(&mMutex);

            logWrite(QString("TXER: %1 %2").arg(transactionType).arg(errorCode));

            //	Remove start time since but it won't be needed since we have an error
            StartTimeStruct startTime;
            extractStartTimeByHandle(handle, startTime);

            //	Find transaction type in error list
            QList<ErrorStruct>::Iterator iterError = mErrors.begin();
            while (iterError != mErrors.end())
            {
                ErrorStruct& error = *iterError;
                if (error.mTransactionType == transactionType)
                {
                    error.mErrorCount++;
                    error.mLastErrorCode = errorCode;
                    return;
                }
                ++iterError;
            }

            //	If we reached here then we need to create new entry in error list
            if (iterError == mErrors.end())
            {
                ErrorStruct error;
                error.mTransactionType = transactionType;
                error.mLastErrorCode = errorCode;
                error.mErrorCount++;
                mErrors.push_back(error);
            }
        #endif
        }

        //-----------------------------------------------------------------------------
        //	This function gets called when we are trying to remove transaction without
        //	logging an error or response time usually when we're shutting down the
        //	heartbeat monitor.
        //-----------------------------------------------------------------------------
        void Heartbeat::onTransactionIgnore(int handle)
        {
        #ifdef HEARTBEAT_DISABLETRANSACTIONS
            Q_UNUSED(handle);
        #else
            QMutexLocker locker(&mMutex);

            //	Remove start time since but it won't be needed since we are ignoring entry
            StartTimeStruct startTime;
            extractStartTimeByHandle(handle, startTime);
        #endif
        }

        //-----------------------------------------------------------------------------
        //
        //-----------------------------------------------------------------------------
        void Heartbeat::onTransactionSuccess(int handle, const QString& transactionType)
        {
        #ifdef HEARTBEAT_DISABLETRANSACTIONS
            Q_UNUSED(handle);
            Q_UNUSED(transactionType);
        #else
            QMutexLocker locker(&mMutex);

            //	Get the start time from the handle
            StartTimeStruct startTime;
            if (extractStartTimeByHandle(handle, startTime) == true)
            {
                //	Compute the response time
                int rspTime = static_cast<int>(startTime.mTime.msecsTo(QDateTime::currentDateTime()));
             
                #ifdef HEARTBEAT_DEBUGRESPONSETOCONSOLE
                    qDebug("%s", transactionType.toAscii().constData());
                    qDebug("RspTime = %d", rspTime);
                #endif

                logWrite(QString("TXOK: %1 %2").arg(transactionType).arg(rspTime));

                //	Add response time to circular buffer
                mResponseTime[mResponseStart].mTime = rspTime;
                mResponseTime[mResponseStart].mTransactionType = transactionType;
                mResponseStart = (mResponseStart + 1) % HEARTBEAT_RESPONSEMAX;
                if (mResponseCount < HEARTBEAT_RESPONSEMAX)
                {
                    mResponseCount++;
                }
            }
        #endif
        }


        //=============================================================================
        //	TRANSACTION INTERFACE:  NETWORK
        //=============================================================================
        
        // Called when a new NetworkAccessManager is created
        void Heartbeat::onNetworkAccessManagerCreated(Origin::Services::NetworkAccessManager *networkAccess)
        {
            ORIGIN_VERIFY_CONNECT_EX(networkAccess, SIGNAL(requestCreated(const QNetworkRequest &, QNetworkReply *)), 
                            this, SLOT(onNetworkRequest(const QNetworkRequest &, QNetworkReply *)),
                            Qt::DirectConnection);
    
            ORIGIN_VERIFY_CONNECT_EX(networkAccess, SIGNAL(finished(QNetworkReply*)), 
                             this, SLOT(onNetworkReply(QNetworkReply*)),
                             Qt::DirectConnection);
        }

        //-----------------------------------------------------------------------------
        //	When the NetworkAccessManager creates a request we get this callback so 
        //	we can register the NetworkReply and log a start time.
        //-----------------------------------------------------------------------------
        void Heartbeat::onNetworkRequest(const QNetworkRequest &, QNetworkReply* pReply)
        {
            Origin::Services::NetworkAccessManager::RequestSourceHint sourceHint = Origin::Services::NetworkAccessManager::sourceForRequest(pReply->request());

            if (sourceHint == Origin::Services::NetworkAccessManager::UserRequestSourceHint ||
                sourceHint == Origin::Services::NetworkAccessManager::DownloaderSourceHint)
            {
                // Don't heartbeat user generated requests or downloader requests
                return;
            }

            QMutexLocker locker(&mNetworkReplyMutex);
        
            //	Remove and delete old entry even though it is unlikely.
            if (mNetworkReply.contains(pReply) == true)
            {
                delete mNetworkReply[pReply];
                mNetworkReply[pReply] = NULL;
            }

            //	Add new reply as a new transaction.  A start time will be logged there.
            mNetworkReply[pReply] = new HeartbeatNetworkResponse(pReply);
        }

        //-----------------------------------------------------------------------------
        //	This callback is called when the NetworkAccessManager is "finished" with
        //	the network response.  
        //-----------------------------------------------------------------------------
        void Heartbeat::onNetworkReply(QNetworkReply* pReply)
        {
            Origin::Services::NetworkAccessManager::RequestSourceHint sourceHint = Origin::Services::NetworkAccessManager::sourceForRequest(pReply->request());

            if (sourceHint == Origin::Services::NetworkAccessManager::UserRequestSourceHint ||
                sourceHint == Origin::Services::NetworkAccessManager::DownloaderSourceHint)
            {
                // Don't heartbeat user generated requests or downloader requests
                return;
            }

            //	We remove the entry for the reply
            {
                QMutexLocker locker(&mNetworkReplyMutex);
                if(mNetworkReply.contains(pReply))
                {
                    HeartbeatNetworkResponse* response = mNetworkReply[pReply];
                    mNetworkReply.remove(pReply);
                    response->deleteLater();
                }    
            }

            //	If reply is the heartbeat message to server then we need to 
            //	set the delete flag.  This is important because otherwise the
            //	NetworkAccessManager will stop sending after 6 messages.
            if (pReply == mpMessage)
            {
                pReply->deleteLater();
                mpMessage = NULL;
            }
        }


        //=============================================================================
        //	DEBUG
        //=============================================================================

        //-----------------------------------------------------------------------------
        //
        //-----------------------------------------------------------------------------

        #if 0
        #include <qwaitcondition.h>

        void HeartbeatTest()
        {
            //	Create wait to emulate Sleep()
            QMutex dummy;
            dummy.lock();
            QWaitCondition waitCondition;


            //	0.  Login with empty message
            Heartbeat::Instance()->OnLogin("basilchan@ea.com");
            Heartbeat::Instance()->SendHeartbeat();
            Heartbeat::Instance()->OnLogoff();
    

            //	1.  Empty message
            Heartbeat::Instance()->SendHeartbeat();

            //	2.  Response time: 1
            {
                int h = Heartbeat::Instance()->OnTransactionCreate();
                Heartbeat::Instance()->OnTransactionSuccess(h, "test2");
                Heartbeat::Instance()->SendHeartbeat();
            }

            //	3.  Response time:  2 same, sequential
            {
                int h = Heartbeat::Instance()->OnTransactionCreate();
                waitCondition.wait(&dummy, 1000);
                Heartbeat::Instance()->OnTransactionSuccess(h, "test3");

                h = Heartbeat::Instance()->OnTransactionCreate();
                waitCondition.wait(&dummy, 1000);
                Heartbeat::Instance()->OnTransactionSuccess(h, "test3");

                Heartbeat::Instance()->SendHeartbeat();
            }

            //	4.  Response time:  2 same, parallel
            {
                int h1 = Heartbeat::Instance()->OnTransactionCreate();
                int h2 = Heartbeat::Instance()->OnTransactionCreate();
                Heartbeat::Instance()->OnTransactionSuccess(h1, "test4");
                Heartbeat::Instance()->OnTransactionSuccess(h2, "test4");
                Heartbeat::Instance()->SendHeartbeat();
            }

            //	5.  Response time:  2 different, sequential
            {
                int h = Heartbeat::Instance()->OnTransactionCreate();
                Heartbeat::Instance()->OnTransactionSuccess(h, "test5a");
                h = Heartbeat::Instance()->OnTransactionCreate();
                Heartbeat::Instance()->OnTransactionSuccess(h, "test5b");
                Heartbeat::Instance()->SendHeartbeat();
            }

            //	6.  Response time:  2 different, parallel
            {
                int h1 = Heartbeat::Instance()->OnTransactionCreate();
                int h2 = Heartbeat::Instance()->OnTransactionCreate();
                Heartbeat::Instance()->OnTransactionSuccess(h1, "test6a");
                Heartbeat::Instance()->OnTransactionSuccess(h2, "test6b");
                Heartbeat::Instance()->SendHeartbeat();
            }

            //	e1.  Error: 1
            {
                int h = Heartbeat::Instance()->OnTransactionCreate();
                Heartbeat::Instance()->OnTransactionError(h, "teste1", -1);
                Heartbeat::Instance()->SendHeartbeat();
            }

            //	e2.  Error: 2 same, sequential
            {
                int h1 = Heartbeat::Instance()->OnTransactionCreate();
                Heartbeat::Instance()->OnTransactionError(h1, "teste2", -1);
                Heartbeat::Instance()->SendHeartbeat();
                int h2 = Heartbeat::Instance()->OnTransactionCreate();
                Heartbeat::Instance()->OnTransactionError(h2, "teste2", -1);
                Heartbeat::Instance()->SendHeartbeat();
            }

            //	e3.  Error: 2 same, parallel
            {
                int h1 = Heartbeat::Instance()->OnTransactionCreate();
                int h2 = Heartbeat::Instance()->OnTransactionCreate();
                Heartbeat::Instance()->OnTransactionError(h1, "teste3", -1);
                Heartbeat::Instance()->OnTransactionError(h2, "teste3", -1);
                Heartbeat::Instance()->SendHeartbeat();
            }

            dummy.unlock();
        }
        #endif
    }
}
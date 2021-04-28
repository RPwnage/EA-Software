//
// Copyright 2011, Electronic Arts Inc
//
#include "InstallProgress.h"
#include <QStringList>
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

namespace Origin
{

namespace Downloader
{

InstallProgressServer::InstallProgressServer() :
	mServer(NULL),
	mSocket(NULL),
	mProgress(0.0f)
{
}

InstallProgressServer::~InstallProgressServer()
{
}

void InstallProgressServer::Init()
{
    ORIGIN_LOG_EVENT << "Initializing InstallProgressServer";

    mProgress = 0.0f;
    mDescription.clear();
    mSocket = NULL;
    mServer = new QLocalServer(this);
    ORIGIN_ASSERT( mServer );

    if( mServer )
    {
        ORIGIN_VERIFY_CONNECT( mServer, SIGNAL(newConnection()), this, SLOT(OnNewConnection()) );
        if( mServer->listen( kINSTALL_PROGRESS_PIPE_NAME ) == false )
        {
            ORIGIN_LOG_WARNING << "server listen failed for pipe [" << kINSTALL_PROGRESS_PIPE_NAME << "]";
        }
    }
}

void InstallProgressServer::Shutdown()
{
    ORIGIN_LOG_EVENT << "Shutting down InstallProgressServer";

    // Disconnect all signals
    QObject::disconnect(this, 0, 0, 0);

    mServer->close();
    delete mServer; // deleting the server automatically deletes the socket
    mServer = NULL;
    mSocket = NULL;

    // Signal the thread to exit it's event loop
    QThread::currentThread()->quit();
}

void InstallProgressServer::OnNewConnection()
{
    ORIGIN_ASSERT( mSocket == NULL );
    if( mSocket == NULL )
    {
        ORIGIN_ASSERT( mServer );
        if( mServer )
        {
            mSocket = mServer->nextPendingConnection();
            ORIGIN_ASSERT( mSocket != NULL );
            if( mSocket )
            {
                ORIGIN_VERIFY_CONNECT(mSocket, SIGNAL(readyRead()), this, SLOT(OnReadyRead()));
                ORIGIN_VERIFY_CONNECT(mSocket, SIGNAL(disconnected()), this, SLOT(OnDisconnected()));

                ORIGIN_ASSERT( mSocket->state() == QLocalSocket::ConnectedState );

                emit Connected();
                emit ReceivedStatus(0.0f, "");
#if defined(EA_DEBUG)
                qDebug() << "CONNECTED!";
#endif
            }
        }
    }
}

void InstallProgressServer::OnReadyRead()
{
    ORIGIN_ASSERT( mSocket != NULL );
    ORIGIN_ASSERT( mSocket->state() == QLocalSocket::ConnectedState );

	if( mSocket->state() != QLocalSocket::ConnectedState )
		return;

    QString content;
    QByteArray byteArray;
    if( mSocket )
    {
        byteArray = mSocket->readAll();
        content += QString::fromUtf16( (const ushort*) byteArray.constData() );
    }

    qDebug() << QString("bytes read: [%1]").arg(content);

    // split up tab-delimited string
    QStringList contentList;
    contentList = content.split("\t");
    qDebug() << contentList;

    // get progress percent
	float newProgress = contentList.at(0).toFloat() / 100.0f; // normalize to [0.0f, 1.0f]
    if( newProgress > mProgress )
		mProgress = newProgress;
	else
		ORIGIN_LOG_WARNING << "New progress [" << newProgress << "] " << "is not more than the old progress [" << mProgress << "].";

    qDebug() << mProgress;

    if( contentList.size() > 1 )
    {
        // get description
        mDescription = contentList.at(1);
        qDebug() << mDescription;
    }

    emit ReceivedStatus(mProgress, mDescription);
}

void InstallProgressServer::OnDisconnected()
{
    ORIGIN_ASSERT( mSocket != NULL );
    if( mSocket )
    {
        ORIGIN_ASSERT( mSocket->state() == QLocalSocket::UnconnectedState );
        ORIGIN_VERIFY_DISCONNECT(mSocket, SIGNAL(readyRead()), this, SLOT(OnReadyRead()));
        ORIGIN_VERIFY_DISCONNECT(mSocket, SIGNAL(disconnected()), this, SLOT(OnDisconnected()));
        mSocket->flush();
        //mSocket = NULL;
    }
    emit ReceivedStatus(1.0f, "");
    emit Disconnected();

#if defined(EA_DEBUG)
    qDebug() << "DISCONNECTED!";
#endif
}

InstallProgress::InstallProgress()
: mServerThread(NULL)
, mInstallProgressServer(NULL)
, mConnected( false )
{
}

InstallProgress::~InstallProgress()
{
}

void InstallProgress::StartMonitor()
{
	mServerThread = new QThread();
	ORIGIN_ASSERT( mServerThread );
    mInstallProgressServer = new InstallProgressServer();
    ORIGIN_ASSERT( mInstallProgressServer );

    if( mServerThread && mInstallProgressServer )
    {
		mServerThread->setObjectName("Install Progress Server Thread");
		mServerThread->start();

        mInstallProgressServer->moveToThread(mServerThread);
        ORIGIN_VERIFY_CONNECT( mInstallProgressServer, SIGNAL(ReceivedStatus(float, QString)), this, SIGNAL(ReceivedStatus(float, QString)) );
        ORIGIN_VERIFY_CONNECT( mInstallProgressServer, SIGNAL(Connected()), this, SLOT(OnConnected()) );
        ORIGIN_VERIFY_CONNECT( mInstallProgressServer, SIGNAL(Disconnected()), this, SLOT(OnDisconnected()) );
		
		// Post the init message on the thread's event loop
        bool success = QMetaObject::invokeMethod(mInstallProgressServer, "Init", Qt::QueuedConnection);
        ORIGIN_ASSERT(success);
        if (!success)
        {
            ORIGIN_LOG_ERROR << "Unable to invoke InstallProgressServer::Init method.";
        }
   }
    else
    {
        ORIGIN_LOG_WARNING << "Failed to start the install progress server thread and connect signals";
    }
}

float InstallProgress::GetProgress()
{
	ORIGIN_ASSERT( mInstallProgressServer );
    if( mInstallProgressServer )
        return mInstallProgressServer->GetProgress();

    return 0.0f;
}

void InstallProgress::StopMonitor()
{
	// It is valid for 'mServerThread' and 'mInstallProgressServer' to be NULL
	// in the case where a previous touchup installer execution started but did not
	// finish in a previous Origin session.

	if( mServerThread && mInstallProgressServer )
    {
        // Post a shutdown message on the thread event loop, which will cause the sever to terminate it's thread.
        // We do this to ensure that the shutdown happens entirely on the server thread.
		bool success = QMetaObject::invokeMethod(mInstallProgressServer, "Shutdown", Qt::QueuedConnection);
        ORIGIN_ASSERT(success);
        if (success)
        {
            // Block and wait for the thread (for 5 seconds) to exit.  This should normally be nearly instantaneous.
            // We shouldn't wait forever because if for some unexpected reason this blocks forever, we would be blocking the InstallFlow thread
            if (mServerThread->wait(5000))
            {
                // Delete the object
                delete mInstallProgressServer;

                ORIGIN_LOG_EVENT << "InstallProgressServer thread finished.";

                // Clean up the worker thread
		        delete mServerThread;
            }
            else
            {
                ORIGIN_LOG_WARNING << "Timed out waiting for InstallProgressServer thread to finish.  Leaking objects.";
            }
        }
        else
        {
            ORIGIN_LOG_ERROR << "Unable to invoke InstallProgressServer::Shutdown method.";
        }

        mInstallProgressServer = NULL;
        mServerThread = NULL;
    }
    else
    {
        ORIGIN_LOG_WARNING << "Failed to stop install progress server and disconnect signals";
    }
}
} // Downloader
} // Origin
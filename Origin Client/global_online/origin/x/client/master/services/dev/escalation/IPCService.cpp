#include <limits>
#include <QDateTime>
#include <QObject>
#include <QtNetwork>
#include <QDebug>
#include <QString>
#include <QElapsedTimer>

#include "IPCService.h"
#include "IEscalationService.h"
#include "services/crypto/CryptoService.h"
#include "services/platform/PlatformService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#ifdef WIN32
// The buffer size need to be 0 otherwise data could be
// lost if the socket that has written data closes the connection
// before it is read.  Pipewriter is used for write buffering.
#define BUFSIZE 0

#include <windows.h>
#include <sddl.h>

typedef BOOL (WINAPI *FnGetNamedPipeClientProcessId)(HANDLE pipe, PULONG clientProcessId);
#endif

namespace Origin
{ 
    namespace Escalation
    {

#ifdef WIN32
		HANDLE CreateNamedPipeForWindows();
#endif

        Origin::Services::CryptoService::Cipher sCipher = Origin::Services::CryptoService::BLOWFISH;
        Origin::Services::CryptoService::CipherMode sCipherMode = Origin::Services::CryptoService::CIPHER_BLOCK_CHAINING;

        QByteArray encryptionKey()
        {
            static bool isInitialized = false;
            static QByteArray key;

            if (!isInitialized)
            {
                quint64 machineHash = Origin::Services::PlatformService::machineHash();
                QByteArray keyData(QByteArray(reinterpret_cast<char*>(&machineHash), sizeof(machineHash)));
                key = Origin::Services::CryptoService::padKeyFor(keyData, sCipher, sCipherMode);
                isInitialized = true;
            }

            return key;
        }

        IPCClient::IPCClient(QObject* parent) : QObject(parent)
        {

            mSocket = new QLocalSocket(this);
            //connect(mSocket, SIGNAL(connected()), this, SLOT(socket_connected()));
            //connect(mSocket, SIGNAL(disconnected()), this, SLOT(socket_disconnected()));

            //connect(mSocket, SIGNAL(readyRead()), this, SLOT(socket_readReady()));
            //connect(mSocket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(socket_error(QLocalSocket::LocalSocketError)));
        }

        IPCClient::~IPCClient() {
            mSocket->abort();
            delete mSocket;
            mSocket = NULL;
        }

        QString IPCClient::sendMessage(const QString &message) 
		{
			if (!mSocket->isValid() || mSocket->state() != QLocalSocket::ConnectedState)
			{
				mSocket->abort();
				mSocket->connectToServer(ESCALATION_SERVICE_PIPE, QIODevice::ReadWrite);

				if (!mSocket->waitForConnected(ESCALATION_SERVICE_TIMEOUT))
				{
					ORIGIN_LOG_ERROR << "Connection to escalation service failed. Qt LocalSocketError: " << (int)mSocket->error();
					return "";
				}
			}

			mResponse.clear();
            mMessage = message;
            
            // encrypt message
            QByteArray encryptedData;
            bool wasEncrypted = Origin::Services::CryptoService::encryptSymmetric(encryptedData, message.toUtf8(), Origin::Escalation::encryptionKey(), Origin::Escalation::sCipher, Origin::Escalation::sCipherMode);
            ORIGIN_ASSERT(wasEncrypted);
            Q_UNUSED(wasEncrypted);
           
            mSocket->write(encryptedData.toBase64(), encryptedData.toBase64().size());
                
			mSocket->waitForBytesWritten(ESCALATION_SERVICE_TIMEOUT);

            ORIGIN_LOG_DEBUG << "Wrote message [" << message << "], waiting for response...";

			int timeout = message.startsWith("CreateDirectory") ? ESCALATION_SERVICE_MKDIR_TIMEOUT : ESCALATION_SERVICE_TIMEOUT;

			QElapsedTimer elapsed;

			// Gather some info about mkdir timings
			if (message.startsWith("CreateDirectory"))
			{
				elapsed.start();
			}

            // receive
            if(!mSocket->waitForReadyRead(timeout))
            {
                ORIGIN_LOG_ERROR << "Response timed out.";
            }
            else
            {

                QByteArray response = mSocket->readAll();
                mResponse = QString::fromUtf8((QByteArray::fromBase64(response)));
                ORIGIN_LOG_DEBUG << "Received response [" << mResponse << "]";
            }
            
			if (message.startsWith("CreateDirectory"))
			{
				qint64 msecs = elapsed.elapsed();
				ORIGIN_LOG_DEBUG << "CreateDirectory command took " << msecs << " milliseconds";
			}

            return mResponse;
        }


        void IPCClient::socket_connected(){
        }

        void IPCClient::socket_disconnected(){
        }


        void IPCClient::socket_readReady(){
        }

        void IPCClient::socket_error(QLocalSocket::LocalSocketError){
        }

        //////////////////////////////////////////////////////////////////////////////////////////////

        // Initialize the IPC server to listen for new connections
        IPCServer::IPCServer(QObject* parent) : QObject(parent), mFnCheckPipeClientPID(NULL)
        {
			mIsValid = false;
			mServer = new QLocalServer(this);

            connect(mServer, SIGNAL(newConnection()), this, SLOT(newConnectionEstablished()));

#ifdef WIN32
            // We can't check the pipe client PID on Windows XP, so set the bool appropriately
            OSVERSIONINFOEX  osvi;
            ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
            osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

            if ( ::GetVersionEx( ( OSVERSIONINFO * ) &osvi ) )
            {
                // Only Vista and newer support this API
                if (osvi.dwMajorVersion >= 6)
                {
                    mFnCheckPipeClientPID = (void*)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetNamedPipeClientProcessId");
                }
            }

			// Create the named pipe with the proper DACL for Windows
			// This is necessary because Qt (qlocalserver_win.cpp:65) creates the named pipe
			// with a NULL DACL.  We need our named pipe to have the proper permissions to allow
			// us to connect to it from a non-administrative user context.  So we will create it
			// first here, and then when Qt 'creates' it, Qt will pick up the same named pipe with
			// our custom DACL instead of making a new one with a NULL DACL.
			HANDLE                hPipe = CreateNamedPipeForWindows();

			if(hPipe == INVALID_HANDLE_VALUE)
			{
				ORIGIN_LOG_ERROR << "Could not create pipe, win32 error: " << GetLastError();
				return;
			}
#endif

            if (!mServer->listen(ESCALATION_SERVICE_PIPE)) {
                ORIGIN_LOG_ERROR << "Not able to start the Server, Qt SocketError: " << (int)mServer->serverError();
            }
			else
			{
				ORIGIN_LOG_EVENT << "LocalSocket listening on pipe: " << ESCALATION_SERVICE_PIPE;
				mIsValid = true;
			}

#ifdef WIN32
			CloseHandle(hPipe);
#endif
        }

        // Initialize the IPC server to use pending connections that a separate entity/object was listening for/accepted
        IPCServer::IPCServer(bool useManualConnections, QObject* parent) : QObject(parent), mFnCheckPipeClientPID(NULL)
        {
			mIsValid = true;
			mServer = NULL;            
        }

        IPCServer::~IPCServer() 
        {
            for (int idx = 0; idx < mManualConnections.size(); ++idx)
            {
                QLocalSocket* socket = mManualConnections.at(idx);
                socket->close();
                socket->deleteLater();
            }
            
            if (mServer)
			{
				disconnect(mServer, 0, 0, 0);
                mServer->deleteLater();
			}
        }

        // Use this method only when the server is setup to use pending connections managed by a separate entity/object.
        void IPCServer::addPendingConnection(int socketID)
        {
            ORIGIN_LOG_DEBUG << "Adding new connection with socketID: " << socketID;
            
            QLocalSocket* clientConnection = new QLocalSocket();
            clientConnection->setSocketDescriptor(socketID);

            connect(clientConnection, SIGNAL(disconnected()),
					this, SLOT(connectionDisconnected()));
            
			connect(clientConnection, SIGNAL(readyRead()),
					this, SLOT(newDataAvailable()));

            mManualConnections.append(clientConnection);
        }

        void IPCServer::newConnectionEstablished() 
		{
            QLocalSocket *clientConnection = mServer->nextPendingConnection();

			connect(clientConnection, SIGNAL(disconnected()),
					this, SLOT(connectionDisconnected()));

			connect(clientConnection, SIGNAL(readyRead()),
					this, SLOT(newDataAvailable()));

            // Clear the list of validated callers to make sure we re-check
            mValidatedCallers.clear();
        }

		void IPCServer::newDataAvailable()
		{
			QLocalSocket *clientConnection = dynamic_cast<QLocalSocket*>(sender());
			if (!clientConnection)
			{
				ORIGIN_LOG_WARNING << "Got readyRead signal but no socket existed.";
				return;
			}

#ifdef Q_OS_WIN
            // On Windows, we should verify our caller PID by who is connected to the pipe
            // We need to do this before we attempt to read any data from the pipe to be safe!!!
            ULONG pid = 0;
            if (mFnCheckPipeClientPID)
            {
                if (((FnGetNamedPipeClientProcessId)mFnCheckPipeClientPID)((HANDLE)clientConnection->socketDescriptor(), &pid))
                {
                    //ORIGIN_LOG_EVENT << "Pipe Client PID: " << pid;
                    if (pid != 0 && !mValidatedCallers.contains(pid))
                    {
                        QString errorResponse;
                        if (validatePipeClient(pid, errorResponse))
                        {
                            // Successful validation, add the PID to the allowed list to avoid re-checking in the future
                            mValidatedCallers.insert(pid);
                        }
                        else
                        {
                            // Validation failed, we explicitly do NOT want to read the data (it could be malicious)
                            // Send an error response and signal that we should be shut down

                            sendResponse(errorResponse, clientConnection);

                            // Flag that we should shut down the pipe
                            mIsValid = false;

                            emit clientValidationFailed();

                            return;
                        }
                    }
                }
            }
#else
            // OSX handles this for us, nothing to do here
#endif

			if (clientConnection->bytesAvailable() <= 0)
			{
				ORIGIN_LOG_WARNING << "Got readyRead signal but no data available.";
				return;
			}

			QByteArray response = clientConnection->readAll();
            
			// decrypt message
			QByteArray encryptedData(QByteArray::fromBase64(response));
			QByteArray decryptedData;
			bool wasDecrypted = Origin::Services::CryptoService::decryptSymmetric(decryptedData, encryptedData, Origin::Escalation::encryptionKey(), Origin::Escalation::sCipher, Origin::Escalation::sCipherMode);
			ORIGIN_ASSERT(wasDecrypted);
            Q_UNUSED(wasDecrypted);

			QString message = QString::fromUtf8(decryptedData);

			ORIGIN_LOG_EVENT << "Received data: " << message;

			sendResponse(processMessage(message), clientConnection);
            
            emit newDataProcessed();
		}

		void IPCServer::connectionDisconnected()
		{
			ORIGIN_LOG_EVENT << "Pipe disconnected.";
            
            if (!mServer)
            {
                QLocalSocket* socket = static_cast<QLocalSocket*>(sender());
                
                for (int idx = 0; idx < mManualConnections.size(); ++idx)
                {
                    if (mManualConnections.at(idx) == socket)
                    {
                        mManualConnections.removeAt(idx);
                        break;
                    }
                }
            }
            
			if(sender() != NULL)
			{
				// Disconnect all signals/slots
				disconnect(sender(), 0, 0, 0);
				sender()->deleteLater();
			}
		}

        void IPCServer::sendResponse(const QString &messag, QLocalSocket *clientConnection)
        {
			if (!clientConnection || clientConnection->state() == QLocalSocket::UnconnectedState || clientConnection->state() == QLocalSocket::ClosingState)
                return;

            QByteArray block(messag.toUtf8());
            clientConnection->write(block.toBase64(), block.toBase64().size());
            //clientConnection->flush();
            if(clientConnection->bytesToWrite() && !clientConnection->waitForBytesWritten(ESCALATION_SERVICE_TIMEOUT))
            {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_ERROR << "sendResponse waitForBytesWritten timed out, bytes to write: " << clientConnection->bytesToWrite();
#endif
            }
        }

#ifdef WIN32
		HANDLE CreateNamedPipeForWindows()
		{
			// Create the named pipe with the proper DACL for Windows
			HANDLE                hPipe;
			SECURITY_DESCRIPTOR   sd;
			SECURITY_ATTRIBUTES   sa;

			// Create a security descriptor that allows anyone to write to the pipe...
			//pSD = (PSECURITY_DESCRIPTOR) malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);

			InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

			sa.nLength              = sizeof(sa);
			sa.bInheritHandle       = TRUE;
			sa.lpSecurityDescriptor = &sd;

			// Define the SDDL for the DACL. This example sets the following access:
			//      Built-in guests are denied all access.
			//      Anonymous Logon is denied all access.
			//      Authenticated Users are allowed read/write/execute access.
			//      Administrators are allowed full control.
			// Modify these values as needed to generate the proper
			// DACL for your application.

			wchar_t* dacl = L"D:"                   // Discretionary ACL
						L"(D;OICI;GA;;;BG)"     // Deny access to Built-in Guests
						L"(D;OICI;GA;;;AN)"     // Deny access to Anonymous Logon
						L"(A;OICI;GRGWGX;;;AU)" // Allow read/write/execute to Authenticated Users
						L"(A;OICI;GA;;;BA)";    // Allow full control to Administrators

			ConvertStringSecurityDescriptorToSecurityDescriptor(dacl, SDDL_REVISION_1, &sa.lpSecurityDescriptor, NULL);

			// Open our named pipe...
			QString pipeName = QString("\\\\.\\pipe\\") + QString(ESCALATION_SERVICE_PIPE);
			hPipe = CreateNamedPipe(pipeName.utf16(),                    // name of pipe
									PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,       // read/write access
									PIPE_TYPE_BYTE |          // byte type pipe
									PIPE_READMODE_BYTE |      // byte-read mode
									PIPE_WAIT,                // blocking mode
									PIPE_UNLIMITED_INSTANCES, // max. instances
									BUFSIZE,                  // output buffer size
									BUFSIZE,                  // input buffer size
									3000,                     // client time-out
									&sa);                     // security attributes

			
			return hPipe;
		}
#endif
    }//namespace Escalation
}//namespace Origin


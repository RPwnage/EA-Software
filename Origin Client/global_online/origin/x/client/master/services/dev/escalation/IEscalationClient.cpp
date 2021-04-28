///////////////////////////////////////////////////////////////////////////////
// IEscalationClient.cpp
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include "IEscalationClient.h"
#include "services/crypto/CryptoService.h"
#include "services/platform/PlatformService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#ifdef DEBUG
#include "services/settings/SettingsManager.h"
#endif
#include "TelemetryAPIDLL.h"

#include <QtConcurrentRun>

#include <qapplication>
#include <qprocess>
#include <qthread>

#if defined(Q_OS_WIN)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Shellapi.h>

#include "EscalationClientWin.h"
#include "ServiceUtilsWin.h"

#else
#include "EscalationClientOSX.h"
#include <unistd.h>
#endif

const QString & kDefaultEscalationServiceAppPath = "OriginClientService.exe";

namespace Origin
{
    namespace Escalation
    {
        static ProcessId sEscalationClientProcessId = 0;
        extern Origin::Services::CryptoService::Cipher sCipher;
        extern Origin::Services::CryptoService::CipherMode sCipherMode;
        extern QByteArray encryptionKey();
#if defined(Q_OS_WIN)
        const qint32 maxConnectAttempts = 10;
#endif
        typedef QMap<CommandError, QString> CommandErrorStringMap;
        CommandErrorStringMap commandErrorStringMap;
        
        /// \brief Maps CommandError values to QString representation
        void initCommandErrorStringMap()
        {
            // Avoid repopulating again if already populated
            if (commandErrorStringMap.isEmpty())
            {

#define MAP_COMMAND_ERROR_STRING(ErrorValue) commandErrorStringMap[ErrorValue] = #ErrorValue;

                MAP_COMMAND_ERROR_STRING(kCommandErrorNone)
                MAP_COMMAND_ERROR_STRING(kCommandErrorUnknown)
                MAP_COMMAND_ERROR_STRING(kCommandErrorUnsupportedOS)
                MAP_COMMAND_ERROR_STRING(kCommandErrorInvalidCommand)
                MAP_COMMAND_ERROR_STRING(kCommandErrorCallerInvalid)
                MAP_COMMAND_ERROR_STRING(kCommandErrorProcessInvalid)
                MAP_COMMAND_ERROR_STRING(kCommandErrorProcessExecutionFailure)
                MAP_COMMAND_ERROR_STRING(kCommandErrorNotElevatedUser)
                MAP_COMMAND_ERROR_STRING(kCommandErrorUserCancelled)
                MAP_COMMAND_ERROR_STRING(kCommandErrorUACTimedOut)
                MAP_COMMAND_ERROR_STRING(kCommandErrorUACRequestAlreadyPending)
                MAP_COMMAND_ERROR_STRING(kCommandErrorCommandFailed)
                MAP_COMMAND_ERROR_STRING(kCommandErrorNoResponse)
#undef MAP_COMMAND_ERROR_STRING

            }
        }

        /// \brief Returns string representation of given CommandError value.
        /// \return CommandError string representation, default-constructed QString otherwise.
        QString commandErrorString(CommandError commandError)
        {
            QString errorString = "Unknown";

            CommandErrorStringMap::const_iterator iter = commandErrorStringMap.find(commandError);

            if (iter != commandErrorStringMap.end())
            {
                errorString = *iter;
            }

            return errorString;
        }

////////////////////////////////////////////////////////////////////////////////////////////////

        class IWorkerFunctor
        {
        public:
            virtual ~IWorkerFunctor() {}
            virtual bool start() = 0;
        };

        EscalationWorker::EscalationWorker(QThread* thread, IWorkerFunctor* functor)
        : mThread(thread), mFunctor(functor)
        {
        }

        EscalationWorker::~EscalationWorker()
        {
            delete mFunctor;
        }

        EscalationWorker* EscalationWorker::createWorker(IWorkerFunctor* functor)
        {
            QThread* thread = new QThread;
            EscalationWorker* worker = new EscalationWorker(thread, functor);
            worker->moveToThread(thread);
            
            ORIGIN_VERIFY_CONNECT(thread, SIGNAL(started()), worker, SLOT(started()));
            ORIGIN_VERIFY_CONNECT(worker, SIGNAL(finished(bool)), thread, SLOT(quit()));
            ORIGIN_VERIFY_CONNECT(worker, SIGNAL(finished(bool)), worker, SLOT(deleteLater()));
            ORIGIN_VERIFY_CONNECT(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
            
            return worker;
        }

        void EscalationWorker::start()
        {
            mThread->start();
        }
        
        void EscalationWorker::started()
        {
            bool success = false;
            if (mFunctor)
                success = mFunctor->start();

            emit finished(success);
        }
        
////////////////////////////////////////////////////////////////////////////////////////////////

        IEscalationClient* IEscalationClient::createEscalationClient(QObject* parent)
        {
#if defined(Q_OS_WIN)
            return new EscalationClientWin(parent);
#else
            return new EscalationClientOSX(parent);
#endif
        }

        bool IEscalationClient::quickEscalate(const QString& reason)
        {
            QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
            int errCode = 0;
            return Origin::Escalation::IEscalationClient::quickEscalate(reason, errCode, escalationClient);
        }

        bool IEscalationClient::quickEscalate(const QString& reason, int& outErrCode, QSharedPointer<Origin::Escalation::IEscalationClient>& outEscalationClient)
        {
            ORIGIN_LOG_EVENT << "Elevation requested.  UAC Reason: " << reason;

            outEscalationClient.reset(Origin::Escalation::IEscalationClient::createEscalationClient());
            outEscalationClient->enableEscalationService(true);

            // Check the result of the escalation
            outErrCode = outEscalationClient->checkIsRunningElevatedAndUACOK();

            if (evaluateEscalationResult(reason, "checkIsRunningElevatedAndUACOK", outErrCode))
            {
                ORIGIN_LOG_EVENT << "Elevation successful.  UAC Reason: " << reason;
                GetTelemetryInterface()->Metric_APP_ESCALATION_SUCCESS("checkIsRunningElevatedAndUACOK", qPrintable(reason));
                return true;
            }
            return false;
        }

        class QuickEscalateFunctor : public IWorkerFunctor
        {
        public:
            QuickEscalateFunctor(const QString& reason) : mReason(reason) {}
            virtual bool start() { return IEscalationClient::quickEscalate(mReason); }

        private:
            QString mReason;
        };

        EscalationWorker* IEscalationClient::quickEscalateASync(const QString& reason)
        {
            return EscalationWorker::createWorker(new QuickEscalateFunctor(reason));
        }

        bool IEscalationClient::evaluateEscalationResult(const QString& reason, const QString& command, int errCode, int systemErrCode)
        {
            // If the result was success, no need to take any action
            if (errCode == Origin::Escalation::kCommandErrorNone)
                return true;

            // The escalation command failed, so log telemetry and any appropriate errors
            GetTelemetryInterface()->Metric_APP_ESCALATION_ERROR(qPrintable(command), errCode, systemErrCode, qPrintable(reason), qPrintable(Origin::Escalation::commandErrorString((Origin::Escalation::CommandError) errCode)));
            ORIGIN_LOG_ERROR << "Escalation failure: " << qPrintable(Origin::Escalation::commandErrorString((Origin::Escalation::CommandError) errCode)) 
                             << " System Error: " << systemErrCode << " Command: " << command << " UAC Reason: " << reason;
            return false;
        }

        IEscalationClient::IEscalationClient(QObject* parent) : QObject(parent),
            mClient(NULL),
            mbEnableEscalationService(true),
            mError(kCommandErrorNone),
            mSystemError(0),
            mCommandText(),
            mResponseText(),
            mResponseArray()
        {
            // Map CommandError values to their string representations
            initCommandErrorStringMap();
        }

        bool IEscalationClient::startEscalationServiceApp(const QString & serviceApplicationPath)
        {
#if defined(Q_OS_WIN)
            // Is the Origin System Service already running?  If so, we don't need to do anything else
            bool serviceInstalled = false;
            bool running = false;
            if (QueryOriginServiceStatus(QS16(ORIGIN_ESCALATION_SERVICE_NAME), running))
            {
                serviceInstalled = true;
                if (running)
                    return true;
            }

            bool    bReturnValue = false;
            mUACExpired = false;
            mSystemError = 0;

            QElapsedTimer timer;
            timer.start();

            QString mServiceApplicationPath = serviceApplicationPath;

            if(mServiceApplicationPath.isEmpty())
            {
                if(mEscalationServiceAppPath.isEmpty())
                {
                    QString appLocation = QDir::toNativeSeparators(QApplication::applicationDirPath());
                    appLocation+=QDir::separator()+kDefaultEscalationServiceAppPath;
                    mServiceApplicationPath = appLocation;
                }
                else
                    mServiceApplicationPath = mEscalationServiceAppPath;
            }

            ORIGIN_LOG_EVENT << "Starting OriginClientService " << mServiceApplicationPath;

            if(!mServiceApplicationPath.isEmpty())
            {
                QString arguments;
                qint64 currentProcessId = QApplication::applicationPid();

                arguments = " -callerProcessId:%1";
                arguments = arguments.arg(currentProcessId);

                QByteArray encryptedData;
                bool wasEncrypted = Origin::Services::CryptoService::encryptSymmetric(encryptedData, arguments.toUtf8(), Origin::Escalation::encryptionKey(), Origin::Escalation::sCipher, Origin::Escalation::sCipherMode);
                ORIGIN_ASSERT(wasEncrypted);
                if(!wasEncrypted)
                {
                    ORIGIN_LOG_ERROR << "Failed to encrypt escalation args.";
                }
                arguments = mServiceApplicationPath + QString(" -args:%1");
                arguments = arguments.arg(QString(encryptedData.toBase64()));

                bool serviceStarted = false;
                if (serviceInstalled)
                {
#ifdef DEBUG
                    // For debug we only want to start the service if we've specifically set via EACore.ini that we should do so
                    bool debugEscalationService = Origin::Services::readSetting(Origin::Services::SETTING_DebugEscalationService);
                    if (debugEscalationService)
                    {
#endif
                    ORIGIN_LOG_EVENT << "Attempting to start Origin Client Service as a system service...";

                    if (StartOriginService(QS16(ORIGIN_ESCALATION_SERVICE_NAME), arguments))
                    {
                        serviceStarted = true;
                    }
#ifdef DEBUG
                    }
#endif
                }

                ProcessId processId = 0;
                
                if (!serviceStarted)
                {
                    processId = executeProcessInternal(mServiceApplicationPath, arguments, "", "");
                }
                
                if(processId != 0 || serviceStarted)
                {
                    sEscalationClientProcessId = processId;

                    qint32 nMaxConnectAttempts = maxConnectAttempts;

                    bReturnValue = true;

                    while (!(bReturnValue = pingEscalationService()) && nMaxConnectAttempts-- > 0)
                    {
                        if (mError != kCommandErrorNone)
                        {
                            ORIGIN_LOG_WARNING << "Escalation Service returned non-retryable error: " << mError;
                            break;
                        }

                        Origin::Services::PlatformService::sleep(2000);
                        ORIGIN_LOG_EVENT << "Attempting to ping Escalation Service...attempts left:" << nMaxConnectAttempts;
                    }
                }
            }

            qint64 elapsedTime = timer.elapsed();
            ORIGIN_LOG_EVENT << "Starting OriginClientService result: " << bReturnValue << ", timer = " << elapsedTime;

            //  TELEMETRY:  Send the elapsed time of the UAC escalation prompt.  Hoping time will
            //  reveal if prompt close due to user rejection or OS error.
            QString elapsedTimeStr = QString("%1").arg(elapsedTime);
            GetTelemetryInterface()->Metric_ERROR_NOTIFY("OriginClientService_Result", elapsedTimeStr.toUtf8().data());

            const int UACTimeout = 120 * 1000; // 2 minutes

            // Check timer to see if a UAC prompt timed out
            if (mSystemError == ERROR_CANCELLED)
            {
                if ( elapsedTime > UACTimeout )
                {
                    mUACExpired = true;
                    mError = kCommandErrorUACTimedOut;
                    ORIGIN_LOG_EVENT << "The UAC prompt timed out.";
                }
                else
                {
                    mUACExpired = false;
                    mError = kCommandErrorUserCancelled;
                    ORIGIN_LOG_EVENT << "The user cancelled the UAC prompt.";
                }
            }
            

            return bReturnValue;
            
#elif defined(ORIGIN_MAC)
            
            ORIGIN_LOG_EVENT << "Checking the escalation service install";
            
            // Install the Escalation Services helper tool if:
            // 1) first-time install
            // 2) the version numbers don't match
            // 3) the certificates don't match
            char errorMsg[512];
            if (Origin::Services::PlatformService::installEscalationServiceHelperDaemon(errorMsg, sizeof(errorMsg)))
            {
                ORIGIN_LOG_EVENT << "The escalation service is ready";
                return true;
            }
            
            else
            {
                ORIGIN_LOG_EVENT << "Failed to check/install the escalation service: " << errorMsg;
                return false;
            }
#endif
        }

        void IEscalationClient::parseEscalationServiceReturnValue()
        {
            mError       = 0;
            mSystemError = 0;

            mResponseArray.clear();

#if defined(Q_OS_WIN)
            #pragma warning(push)
            #pragma warning(disable: 4018)  // warning C4018: '<' : signed/unsigned mismatch
            // this is intended, n2 will overflow on a negative value!!!
#endif

            for(quint32 n1 = 0, n2 = 0, lineIndex = 0; n2 < quint32(mResponseText.length()); n1 = n2 + 1, lineIndex++)
            {
                n2 = mResponseText.indexOf('\n', n1);

                if(n2 < quint32(mResponseText.length()))
                {
                    QString sValue(mResponseText.mid(n1, n2 - n1));

                    bool unused = false;
                    if(lineIndex == 0)
                        mError=sValue.toInt();
                    else if(lineIndex == 1)
                        mSystemError=sValue.toInt(&unused, 16); // System error is hex

                    mResponseArray.append(sValue);
                }
            }
#if defined(Q_OS_WIN)
            #pragma warning(pop)
#endif

        }


        bool IEscalationClient::connectToEscalationService(bool bConnect)
        {
            TIME_BEGIN("IEscalationClient::connectToEscalationService")
            static bool startRequested = false;
            static QMutex mutex;

            QMutexLocker lock(&mutex); // Prevent other threads from trying to connect at the same time

            ORIGIN_LOG_DEBUG << "Connecting to OriginClientService";

            // If a request to start the escalation service has already been attempted and
            // the ping fails, the process may still be starting. We will wait after the ping
            // failure for a short amount of time and the retry the ping one time before
            // requesting the service to start again.
            bool bResult = false;
            bool retry = false;
    
#ifdef ORIGIN_MAC
            // Always check the escalation service the first time around for any update
            if (startRequested || (!startRequested && !bConnect))
            {
#endif
            do
            {
                retry = !retry;
                bResult = pingEscalationService();

                // If the error was not a connectivity error
                if (!bResult && mError != kCommandErrorNone)
                {
                    ORIGIN_LOG_ERROR << "Connect to OriginClientService - ping did not succeed, service returned error code: " << mError;
                    retry = false;
                }

                if (!bResult && startRequested && retry)
                {
                    ORIGIN_LOG_WARNING << "Connect to OriginClientService - ping did not succeed, waiting once and trying again.";

                    QSemaphore barrier(0);
                    barrier.tryAcquire(1, 2000);
                }
            }
            while (!bResult && retry && startRequested);
#ifdef ORIGIN_MAC
            } // end check the escalation service the first time around for any update
#endif

            if(!bResult)
            {
                bool started = false;
                if(bConnect)
                {
                    {
                        started = startEscalationServiceApp();

#ifdef ORIGIN_MAC
                        // For Mac, startRequested = check the service is properly installed
                        startRequested = started;
#else
                        startRequested = true;
#endif
                    }

                    if (started)
                        ORIGIN_LOG_DEBUG << "Escalation service ready";
                }
                // We don't currently have a way to stop it.

                // Try again.
                if (started)
                    bResult = pingEscalationService();
            }
            
            ORIGIN_LOG_DEBUG << "Connect to OriginClientService result: " << bResult;
            TIME_END("IEscalationClient::connectToEscalationService")

            return bResult;
        }

        bool IEscalationClient::pingEscalationService()
        {
            TIME_BEGIN("IEscalationClient::pingEscalationService")
            bool bResult = false;

            if (!mClient)
                mClient = new IPCClient(this);

            // Erase any previous error
            mError = kCommandErrorNone;

            QString sCommandText;

            sCommandText  = "Ping\n";
            sCommandText += '\0';

            mResponseText = mClient->sendMessage(sCommandText);

            ORIGIN_LOG_DEBUG << "Ping OriginClientService result: \"" << mResponseText << "\"";

            // Makes sure the pipe response wasn't an error.
            bResult = (mResponseText.indexOf("0\n") == 0) || (mResponseText.indexOf("ADMIN\n") == 0);

            if(!bResult)
            {
                if(mResponseText != "")
                    parseEscalationServiceReturnValue();
            }

            TIME_END("IEscalationClient::pingEscalationService")
            return bResult;
        }


        int IEscalationClient::executeEscalationServiceCommand(const QString & pCommand)
        {
            mCommandText = pCommand;

            executeEscalationServiceCommand();

            return mError;
        }


        bool IEscalationClient::executeEscalationServiceCommand()
        {
            TIME_BEGIN("IEscalationClient::executeEscalationServiceCommand")
            bool bResult = connectToEscalationService(true); // Makes sure the escalation service is started if not already.

            if(!bResult)
            {
                if(mResponseText != "")
                    parseEscalationServiceReturnValue();
            }
            else
            {
                bResult = false;

                if (!mClient)
                    mClient = new IPCClient(this);

                mResponseText = mClient->sendMessage(mCommandText);

                if (mResponseText.isEmpty())
                {
                    // No response from client service
                    mError = kCommandErrorNoResponse;

                    // No associated system error for this condition
                    mSystemError = 0;

                    return false;
                }
                else
                {
                    parseEscalationServiceReturnValue();
                }

                bResult = (mError == kCommandErrorNone);
            }
            // Else the error values should be set already.

            TIME_END("IEscalationClient::executeEscalationServiceCommand")
            return bResult;
        }


        void IEscalationClient::enableEscalationService(bool bEnableEscalationService)
        {
            mbEnableEscalationService = bEnableEscalationService;
        }


        ProcessId IEscalationClient::executeProcess(const QString & pProcessPath, const QString & pProcessArguments, const QString & pProcessDirectory, const QString& environment, const QString &jobName)
        {
            TIME_BEGIN("IEscalationClient::executeProcess")
            ORIGIN_LOG_EVENT << "Executing Process " << pProcessPath << " args: " << pProcessArguments << " working path: " << pProcessDirectory << " environment: " << environment;
            ProcessId processId = 0;

            // We reset the error state upon every call.
            mError       = kCommandErrorUnknown;
            mSystemError = 0;

            if(mbEnableEscalationService)
            {
                mCommandText = QString("ExecuteProcess\n%1\n%2\n%3\n%4\n%5\n%6").arg(pProcessPath,
                    pProcessArguments, pProcessDirectory, environment, jobName, QChar(0));

                if(executeEscalationServiceCommand())
                {
                    // The result should now be stored in mError/mSystemError.
                    qint64 val = 0;
                    getReturnInt64(2, val);
                    processId = static_cast<ProcessId>(val);
                }
                // Else the error values should be set already.
            }
            else
            {
                ORIGIN_LOG_EVENT << "Executing process without OriginClientService";
                processId = executeProcessInternal(pProcessPath, pProcessArguments, pProcessDirectory, environment);
            }

            TIME_END("IEscalationClient::executeProcess")
            return processId;
        }

        qint32 IEscalationClient::closeProcess(qint32 processId)
        {
            TIME_BEGIN("IEscalationClient::closeProcess")

            // We reset the error state upon every call.
            mError       = kCommandErrorUnknown;
            mSystemError = 0;

            if(mbEnableEscalationService)
            {
                mCommandText  = "CloseProcess";
                mCommandText += '\n';

                mCommandText += QString::number(processId);
                mCommandText += '\n';

                mCommandText += '\0';

                if(executeEscalationServiceCommand())
                {
                    // The result should now be stored in mError/mSystemError.
                }
            }
            else
            {
                ORIGIN_LOG_EVENT << "close process without OriginClientService";
                closeProcessInternal(processId);
            }

            TIME_END("IEscalationClient::closeProcess")
            return mError;
        }

        qint32 IEscalationClient::addToMonitorList(qint32 processId, const QString &processName)
        {
            TIME_BEGIN("IEscalationClient::addToMonitorList")

            // We reset the error state upon every call.
            mError       = kCommandErrorUnknown;
            mSystemError = 0;

            if(mbEnableEscalationService)
            {
                mCommandText  = "AddToMonitorList";
                mCommandText += '\n';

                mCommandText += QString::number(processId);
                mCommandText += '\n';

                mCommandText += processName;
                mCommandText += '\n';

                mCommandText += '\0';

                executeEscalationServiceCommand();
            }
            else
            {
                ORIGIN_LOG_WARNING << "Escalation Service not running: could not execute addToMonitorList";
            }

            TIME_END("IEscalationClient::addToMonitorList")
            return mError;
        }

        qint32 IEscalationClient::removeFromMonitorList(qint32 processId)
        {
            TIME_BEGIN("IEscalationClient::removeFromMonitorList")

            // We reset the error state upon every call.
            mError       = kCommandErrorUnknown;
            mSystemError = 0;

            if(mbEnableEscalationService)
            {
                mCommandText  = "RemoveFromMonitorList";
                mCommandText += '\n';

                mCommandText += QString::number(processId);
                mCommandText += '\n';

                mCommandText += '\0';

                executeEscalationServiceCommand();
            }
            else
            {
                ORIGIN_LOG_WARNING << "Escalation Service not running: could not execute removeFromMonitorList";
            }

            TIME_END("IEscalationClient::removeFromMonitorList")
            return mError;
        }

        qint32 IEscalationClient::terminateProcess(qint32 processId)
        {
            TIME_BEGIN("IEscalationClient::terminateProcess")
 
            // We reset the error state upon every call.
            mError       = kCommandErrorUnknown;
            mSystemError = 0;

            if(mbEnableEscalationService)
            {
                mCommandText  = "TerminateProcess";
                mCommandText += '\n';

                mCommandText += QString::number(processId);
                mCommandText += '\n';

                mCommandText += '\0';


                if(executeEscalationServiceCommand())
                {
                    // The result should now be stored in mError/mSystemError.
                }
            }
            else
            {
                ORIGIN_LOG_EVENT << "Executing process without OriginClientService";
                terminateProcessInternal(processId);
            }

            TIME_END("IEscalationClient::terminateProcess")
            return mError;
        }

#if defined(Q_OS_WIN)

        qint32 IEscalationClient::injectIGOIntoProcess(qint32 processId, qint32 threadId)
        {
            TIME_BEGIN("IEscalationClient::injectIGOIntoProcess")
 
            // We reset the error state upon every call.
            mError       = kCommandErrorUnknown;
            mSystemError = 0;

            if(mbEnableEscalationService)
            {
                mCommandText  = "InjectIGOIntoProcess";
                mCommandText += '\n';

                mCommandText += QString::number(processId);
                mCommandText += '\n';

                mCommandText += QString::number(threadId);
                mCommandText += '\n';

                mCommandText += '\0';


                if(executeEscalationServiceCommand())
                {
                    // The result should now be stored in mError/mSystemError.
                }
            }
            else
            {
                ORIGIN_LOG_EVENT << "Executing process without OriginClientService";
                terminateProcessInternal(processId);
            }

            TIME_END("IEscalationClient::injectIGOIntoProcess")
            return mError;
        }

        QString hkeyToQString(HKEY registryKey)
        {
            if (HKEY_CLASSES_ROOT==registryKey)
                return "0x80000000";
            else if(HKEY_CURRENT_USER==registryKey)
                return "0x80000001";
            else if(HKEY_LOCAL_MACHINE==registryKey)
                return "0x80000002";
            else if(HKEY_USERS==registryKey)
                return "0x80000003";
            else if (HKEY_CURRENT_CONFIG==registryKey)
                return "0x80000005";
            else
                return "";
        }

        QString valTypeToQString(IEscalationClient::RegistryStringValueTypes registryStringValueType)
        {
            switch (registryStringValueType)
            {

            case IEscalationClient::RegExpandSz:
                return "REG_EXPAND_SZ";
                break;
            case IEscalationClient::RegLink:
                return "REG_LINK";
                break;
            case IEscalationClient::RegMultiSz:
                return "REG_MULTI_SZ";
                break;
            default:
            case IEscalationClient::RegSz:
                return "REG_SZ";
                break;
            }
        }

        int IEscalationClient::setRegistryStringValue(RegistryStringValueTypes valType, HKEY registryKey, const QString& registrySubKey, const QString& valueName, const QString& val)
        {
            TIME_BEGIN("IEscalationClient::setRegistryStringValue")

                // We reset the error state upon every call.
                mError       = kCommandErrorUnknown;
            mSystemError = 0;

            if(mbEnableEscalationService)
            {
                mCommandText  = "setRegistryStringValue";
                mCommandText += '\n';

                mCommandText += hkeyToQString(registryKey);
                mCommandText += '\n';

                mCommandText += registrySubKey;
                mCommandText += '\n';

                mCommandText += valueName;
                mCommandText += '\n';

                mCommandText += val;
                mCommandText += '\n';

                mCommandText += valTypeToQString(valType);
                mCommandText += '\n';

                // end string
                mCommandText += '\0';

                if(executeEscalationServiceCommand())
                {
                    // The result should now be stored in mError/mSystemError.
                }
                // Else the error values should be set already.
            }
            TIME_END("IEscalationClient::setRegistryStringValue")
                return mError;
        }

        QString valTypeToQString(IEscalationClient::RegistryIntValueTypes registryIntValueType)
        {
            switch (registryIntValueType)
            {
            default:
            case IEscalationClient::RegDWord:
                return "REG_DWORD";
                break;
            case IEscalationClient::RegDWordBigEndian:
                return "REG_DWORD_BIG_ENDIAN";
                break;
            case IEscalationClient::RegQWord:
                return "REG_QWORD";
                break;
            }
        }
        
        int IEscalationClient::setRegistryIntValue(RegistryIntValueTypes valType, HKEY registryKey, const QString& registrySubKey, const QString& valueName, __int64 val, int base)
        {
            TIME_BEGIN("IEscalationClient::setRegistryIntValue")

                // We reset the error state upon every call.
                mError       = kCommandErrorUnknown;
            mSystemError = 0;

            if(mbEnableEscalationService)
            {
                mCommandText  = "setRegistryIntValue";
                mCommandText += '\n';

                mCommandText += hkeyToQString(registryKey);
                mCommandText += '\n';

                mCommandText += registrySubKey;
                mCommandText += '\n';

                mCommandText += valueName;
                mCommandText += '\n';

                mCommandText += QString::number(val, base);
                mCommandText += '\n';

                mCommandText += valTypeToQString(valType);
                mCommandText += '\n';

                // end string
                mCommandText += '\0';

                if(executeEscalationServiceCommand())
                {
                    // The result should now be stored in mError/mSystemError.
                }
                // Else the error values should be set already.
            }
            TIME_END("IEscalationClient::setRegistryIntValue")
                return mError;
        }

        int IEscalationClient::deleteRegistrySubKey(HKEY registryKey, const QString& registrySubKey)
        {
            TIME_BEGIN("IEscalationClient::deleteRegistrySubKey")

                // We reset the error state upon every call.
                mError       = kCommandErrorUnknown;
            mSystemError = 0;

            if(mbEnableEscalationService)
            {
                mCommandText  = "deleteRegistrySubKey";
                mCommandText += '\n';

                mCommandText += hkeyToQString(registryKey);
                mCommandText += '\n';

                mCommandText += registrySubKey;
                mCommandText += '\n';

                // end string
                mCommandText += '\0';

                if(executeEscalationServiceCommand())
                {
                    // The result should now be stored in mError/mSystemError.
                }
                // Else the error values should be set already.
            }
            TIME_END("IEscalationClient::deleteRegistrySubKey")
                return mError;
        }

#endif

        int IEscalationClient::createDirectory(const QString & pDirectory, const QString & pAccessControlList)
        {
            TIME_BEGIN("IEscalationClient::createDirectory")

            // We reset the error state upon every call.
            mError       = kCommandErrorUnknown;
            mSystemError = 0;

            if(mbEnableEscalationService)
            {
                mCommandText  = "CreateDirectory";
                mCommandText += '\n';

                mCommandText += pDirectory;
                mCommandText += '\n';

#if defined(Q_OS_WIN)
                if(!pAccessControlList.isEmpty())
                    mCommandText += pAccessControlList;
#elif defined(ORIGIN_MAC)
                // On Mac, we add the unix user id and group id to mCommandText so that the escalation service
                // can set ownership of directories to the user.
                mCommandText += QString("%1;%2").arg(getuid()).arg(getgid());
#endif
                mCommandText += '\n';
                mCommandText += '\0';

                if(executeEscalationServiceCommand())
                {
                    // The result should now be stored in mError/mSystemError.
                }
                // Else the error values should be set already.
            }
            else
            {
                createDirectoryFallback(pDirectory, pAccessControlList);
            }            
            TIME_END("IEscalationClient::createDirectory")
            return mError;
        }

#if defined(ORIGIN_MAC)
        // Run a compiled AppleScript file.
        int IEscalationClient::runScript(const QString& scriptName)
        {
            TIME_BEGIN("IEscalationClient::runScript")
            if(mbEnableEscalationService)
            {
                mCommandText  = "RunScript";
                mCommandText += '\n';
                
                mCommandText += scriptName;
                mCommandText += '\n';
                
                mCommandText += '\0';                
                if(executeEscalationServiceCommand())
                {
                    // The result should now be stored in mError/mSystemError.
                }
                // Else the error values should be set already.
            }
            else
            {
                mError       = kCommandErrorNotElevatedUser;
                mSystemError = 0;
            }
            TIME_END("IEscalationClient::runScript")
            return mError;
        }

        // Run a compiled AppleScript file.
        int IEscalationClient::enableAssistiveDevices()
        {
            TIME_BEGIN("IEscalationClient::enableAssistiveDevices")
            if(mbEnableEscalationService)
            {
                mCommandText  = "EnableAssistiveDevices";
                mCommandText += '\n';
                
                mCommandText += '\0';                
                if(executeEscalationServiceCommand())
                {
                    // The result should now be stored in mError/mSystemError.
                }
                // Else the error values should be set already.
            }
            else
            {
                mError       = kCommandErrorNotElevatedUser;
                mSystemError = 0;
            }
            TIME_END("IEscalationClient::enableAssistiveDevices")
            return mError;
        }
        
        int IEscalationClient::deleteBundleLicense(const QString & pDirectory)
        {
            TIME_BEGIN("IEscalationClient::deleteBundleLicense")
            if(mbEnableEscalationService)
            {
                mCommandText  = "DeleteBundleLicense";
                mCommandText += '\n';
                
                mCommandText += pDirectory;
                mCommandText += '\n';
                
                mCommandText += '\0';
                
                if(executeEscalationServiceCommand())
                {
                    // The result should now be stored in mError/mSystemError.
                }
                // Else the error values should be set already.
            }
            else
            {
                mError       = kCommandErrorNotElevatedUser;
                mSystemError = 0;
            }
            TIME_END("IEscalationClient::deleteBundleLicense")
            return mError;
        }
#endif // ORIGIN_MAC
        
        // Delete a folder.
        int IEscalationClient::deleteDirectory(const QString& dirName)
        {
            TIME_BEGIN("IEscalationClient::deleteDirectory")
            if(mbEnableEscalationService)
            {
                mCommandText  = "DeleteDirectory";
                mCommandText += '\n';
                
                mCommandText += dirName;
                mCommandText += '\n';
                
                mCommandText += '\0';
                if(executeEscalationServiceCommand())
                {
                    // The result should now be stored in mError/mSystemError.
                }
                // Else the error values should be set already.
            }
            else
            {
                mError       = kCommandErrorNotElevatedUser;
                mSystemError = 0;
            }
            TIME_END("IEscalationClient::deleteDirectory")
            return mError;
        }

        bool IEscalationClient::getReturnString(int index, QString& sValue)
        {
            if(index < mResponseArray.size())
            {
                sValue = mResponseArray[index];
                return true;
            }

            return false;
        }


        bool IEscalationClient::getReturnInt64(int index, qint64& value)
        {
            QString sValue;

            if(getReturnString(index, sValue))
            {
                value = sValue.toLongLong(0, 16);   // hex
                return true;
            }

            return false;
        }


        int IEscalationClient::getError() const
        {
            return mError;
        }


        int IEscalationClient::getSystemError() const
        {
            return mSystemError;
        }


        //check if the user is admin by pinging the escalation service
        //and checking return string
        int IEscalationClient::checkIsRunningElevatedAndUACOK()
        {
#ifdef ORIGIN_MAC
            if (connectToEscalationService(true))
                return kCommandErrorNone;
            else
                return kCommandErrorNotElevatedUser;
#else            
            QString returnString;
            bool isAdmin;

            //reset the error state
            mError = kCommandErrorNone;

            //mError will get set in ConnectToEscalationService
            connectToEscalationService(true);

            //if the error is none then continue parsing to check for admin
            if(mError == kCommandErrorNone)
            {
                parseEscalationServiceReturnValue();
                getReturnString(0, returnString);
                isAdmin = (returnString.contains("ADMIN"));

                //if not admin set the not elevated user error
                if(!isAdmin)
                    mError = kCommandErrorNotElevatedUser;

            }
            else
            {
                // No system error is associated with this condition
                mSystemError = 0;
            }
        
            return mError;
#endif
        }

        ProcessId IEscalationClient::getProcessId()
        {
            return sEscalationClientProcessId;
        }
    }
} // namespace EA

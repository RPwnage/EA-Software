///////////////////////////////////////////////////////////////////////////////
// IEscalationService.cpp
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/platform/PlatformService.h"
#include "services/crypto/CryptoService.h"
#include "services/debug/DebugService.h"

#if defined(Q_OS_WIN)
#include "CommandProcessorWin.h"
#else
#include "CommandProcessorOSX.h"
#endif

namespace Origin
{

    namespace Escalation
    {

        extern Origin::Services::CryptoService::Cipher sCipher;
        extern Origin::Services::CryptoService::CipherMode sCipherMode;
        extern QByteArray encryptionKey();

        IEscalationService::IEscalationService(bool runningAsService, QStringList serviceArgs, QObject* parent) : IPCServer(parent)
        {
			if (!mIsValid)
			{
				ORIGIN_LOG_ERROR << "IPC Server failed to initialize, exiting.";
				return;
			}

            Origin::Services::PlatformService::init();

            // get command line arguments and clear our process list
            // TODO: add decryption of commandline + TCP data
            mCallerProcessId = 0;
            mIsValid = true;

            // Hook up our signal to watch our validity flag
            ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(clientValidationFailed()), this, SLOT(onIPCServer_clientValidationFailed()), Qt::QueuedConnection);

#if defined(Q_OS_WIN) // TODO - THIS SHOULD BE MOVED TO EscalationServiceWin.cpp
            QStringList arguments = runningAsService ? serviceArgs : QCoreApplication::arguments();

            QString encryptedArguments;
            QStringList decryptedArguments;
            for(int i = 1; i < arguments.count(); ++i)
            {
                if(arguments.at(i).startsWith("-args:"))
                {
                    ORIGIN_LOG_EVENT << "Found escalation args.";
                    QString tempValue = arguments.at(i);
                    encryptedArguments = tempValue.mid(tempValue.indexOf(":")+1);
                    break;
                }
            }

            // decrypt the command line arguments
            QByteArray encryptedData(QByteArray::fromBase64(encryptedArguments.toUtf8()));
            QByteArray decryptedData;
            bool wasDecrypted = Origin::Services::CryptoService::decryptSymmetric(decryptedData, encryptedData, Origin::Escalation::encryptionKey(), Origin::Escalation::sCipher, Origin::Escalation::sCipherMode);
            ORIGIN_ASSERT(wasDecrypted);

#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
            if(!wasDecrypted)
            {
                ORIGIN_LOG_ERROR << "Command line arguments failed to decrypt.";
            }
#endif

            decryptedArguments = QString::fromUtf8(decryptedData).split(" ");

            for(int i = 1; i < decryptedArguments.count(); ++i)
            {
                if(decryptedArguments.at(i).startsWith("-callerProcessId:"))
                {
                    QString tempValue = decryptedArguments.at(i);
                    tempValue = tempValue.mid(tempValue.indexOf(":")+1);
                    mCallerProcessId = tempValue.toULongLong();
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                    ORIGIN_LOG_EVENT << "Successfully determined caller process ID: " << mCallerProcessId;
#endif
                    break;
                }
            }

            if(!mCallerProcessId)
            {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_ERROR << "Failed to determine caller process ID.";
#endif

                mIsValid = false;
                return;
            }

            // Tell the command processor how we will invoke other processes
            CommandProcessorWin::setOriginPid(mCallerProcessId);
            CommandProcessorWin::setProcessExecutionMode(runningAsService);
#endif // Q_OS_WIN
        }

#ifdef ORIGIN_MAC
        IEscalationService::IEscalationService(bool useManualConnections, QObject* parent) : IPCServer(useManualConnections, parent)
        {
			if (!mIsValid)
			{
				ORIGIN_LOG_ERROR << "IPC Server failed to initialize, exiting.";
				return;
			}
            
            Origin::Services::PlatformService::init();
            
            // get command line arguments and clear our process list
            // TODO: add decryption of commandline + TCP data
            mCallerProcessId = 0;
            mIsValid = true;            
        }
#endif
        bool IEscalationService::validatePipeClient(const quint32 clientPid, QString& errorResponse)
        {
            int result =  validateCaller(clientPid);
            
            if(result == kCommandErrorNone)
            {
                return true;
            }
            else
            {
#if defined(Q_OS_WIN)
                CommandProcessorWin commandProcessor(this);
#else
                CommandProcessorOSX commandProcessor(this);
#endif

#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                // If we have received a command from an invalid caller, the only safe course of action is to shut down after we send this response
                ORIGIN_LOG_ERROR << "Command received from invalid caller, shutting down for safety.  ValidateCaller Result = " << result;
#endif

                // Let the CommandProcessor handle the failure as if it was executing the validation.
                commandProcessor.processExternalError(errorResponse, result, "OriginClientService: Caller validation failed.", false);

                return false;
            }
        }

        QString IEscalationService::processMessage(const QString message)
        {
            QString response;

#if defined(Q_OS_WIN)
            CommandProcessorWin commandProcessor(this);
#else
            CommandProcessorOSX commandProcessor(this);
#endif

            commandProcessor.setCommandText(message);
            commandProcessor.processCommand(response);

            //these commands have custom responses that are handled with in the escalation client
            //if we end up with more of these commands we may need to figure out a beter way to handle these
            if(response.startsWith("addMonitorList"))
            {
                addToMonitorListInternal(response);
            }
            else
            if(response.startsWith("removeMonitorList"))
            {
                removeFromMonitorListInternal(response);
            }

            return response;
        }


        IEscalationService::~IEscalationService()
        {
            Origin::Services::PlatformService::release();
            
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
            ORIGIN_LOG_EVENT << "Escalation service shutdown.";
#endif
        }

        void IEscalationService::onIPCServer_clientValidationFailed()
        {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
            ORIGIN_LOG_ERROR << "Shutting down Escalation Service due to IPC server failure.";
#endif
            QCoreApplication::exit(0);
        }

    }// namespace Escalation

} // namespace Origin

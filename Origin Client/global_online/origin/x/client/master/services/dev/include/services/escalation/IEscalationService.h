///////////////////////////////////////////////////////////////////////////////
// IEscalationService.h
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef IESCALATIONSERVICE_H
#define IESCALATIONSERVICE_H

#include <limits>
#include <QDatetime>
#include <QtCore>
#include <QString>
#include <QVector>
#include "IPCService.h"
#include "services/plugin/PluginAPI.h"

#define ENABLE_ESCALATION_SERVICE_LOGGING
 
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
#include "services/log/LogService.h"
#endif

namespace Origin
{
    namespace Escalation
    {
        // ESCALATION_SERVICE_NAME
        
        // TODO: those definitions should really be local to each implementation .cpp
        const QString ESCALATION_SERVICE_NAME = "OriginClientService";

        // During updates, users who had downloaded the entitlement to an external drive were experiencing timeouts
        // when the downloader attempted to verify that the directories and subdirectories had the correct permissions
        // using SetNamedSecurityInfo. Increased the timeout from 10 to 30 seconds. Need to investigate a way to modify
        // the permissions in a non recursive manner which will allow a short timeout for each folder instead of one
        // large timeout for all folders.
        const int ESCALATION_SERVICE_TIMEOUT = 30000;            // 30 seconds
		const int ESCALATION_SERVICE_MKDIR_TIMEOUT = 180000;		 // 180 seconds = 3 minutes, surely enough time for anyone

#ifdef ORIGIN_MAC
        const QString ESCALATION_SERVICE_PIPE = "/var/run/com.ea.origin.eshelper.sock";
#else
        const QString ESCALATION_SERVICE_PIPE = "OriginClientService";      // local host
#endif
        const QString ORIGIN_APP_NAME = "Origin";

        typedef qint64 ProcessId;

        struct ProcessIdExePair
        {
            ProcessId mPid;
            QString mProcessExeName;
        };

        enum CommandType
        {
            kCommandNone,               // 
            kCommandPing,               // "Ping"
            kCommandExecuteProcess,     // "ExecuteProcess"
            kCommandCreateDirectory,    // "CreateDirectory"
            kCommandDeleteBundleLicense,// "DeleteBundleLicense"
            kCommandRunScript           // "RunScript"
        };


        enum CommandError
        {
            kCommandErrorNone,                      // 
            kCommandErrorUnknown,                   // Some unspecified error.
            kCommandErrorUnsupportedOS,             // The command couldn't be completed due to the OS not supporting it.
            kCommandErrorInvalidCommand,            // The command the caller issued was malformed.
            kCommandErrorCallerInvalid,             // The caller is not authenticated.
            kCommandErrorProcessInvalid,            // The process the caller wants to execute is not authenticated.
            kCommandErrorProcessExecutionFailure,   // The process the caller wants to execute failed to run.
            kCommandErrorNotElevatedUser,			// The user is not an elevated user
            kCommandErrorUserCancelled,             // The user refused to accept the UAC dialog
            kCommandErrorUACTimedOut,               // The UAC prompt timed out
            kCommandErrorUACRequestAlreadyPending,  // The UAC prompt is already up/waiting on the user
            kCommandErrorCommandFailed,             // The service attempted to execute the command but it failed. 
            kCommandErrorNoResponse                 // The service did not respond to the command.
        };

        ORIGIN_PLUGIN_API QString commandErrorString(CommandError commandError);

        class ORIGIN_PLUGIN_API IEscalationService : public IPCServer
        {
            Q_OBJECT
        public:

            explicit IEscalationService(bool runningAsService, QStringList serviceArgs, QObject* parent = 0);
#ifdef ORIGIN_MAC
            explicit IEscalationService(bool useManualConnections, QObject* parent = 0);
#endif
            ~IEscalationService();

            bool isRunning() { return mIsValid; }

        protected:
            virtual CommandError validateCaller(const quint32 clientPid) = 0;
            virtual void addToMonitorListInternal(QString response) = 0;
            virtual void removeFromMonitorListInternal(QString response) = 0;
            bool validatePipeClient(const quint32 clientPid, QString& errorResponse);
            QString processMessage(const QString message);

        protected slots:
            void onIPCServer_clientValidationFailed();

        protected:
            // Stores a set of the process ids that have been validated
            ProcessId mCallerProcessId;
        };
    } // namespace Origin
} // namespace Origin


#endif // Header include guard

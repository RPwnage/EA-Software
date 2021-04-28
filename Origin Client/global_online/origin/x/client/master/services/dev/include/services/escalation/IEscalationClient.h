///////////////////////////////////////////////////////////////////////////////
// IEscalationClient.h
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef IESCALATIONCLIENT_H
#define IESCALATIONCLIENT_H


#include "IEscalationService.h"
#include "IPCService.h"
#include "services/plugin/PluginAPI.h"

#if defined(Q_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace Origin
{ 

    namespace Escalation
    {
        // Helper to run queries asynchronuously
        class IWorkerFunctor;
        class EscalationWorker : public QObject
        {
            Q_OBJECT
            
        public:
            // Use this to automatically create a new worker/thread
            static EscalationWorker* createWorker(IWorkerFunctor* functor);

            // Use this to get the worker going
            void start();

        public slots:
            void started();
            
        signals:
            // Use this to get notified when the worker is done
            void finished(bool success);
            
        private:
             EscalationWorker(QThread* thread, IWorkerFunctor* functor);
             EscalationWorker(const EscalationWorker& worker);
            ~EscalationWorker();

            QThread* mThread;
            IWorkerFunctor* mFunctor;
        };

        ///////////////////////////////////////////////////////////////////////////////
        // IEscalationClient
        ///////////////////////////////////////////////////////////////////////////////

        class ORIGIN_PLUGIN_API IEscalationClient : public QObject
        {
            Q_OBJECT

        public:
            static IEscalationClient* createEscalationClient(QObject* parent = 0);

            // Create an IEscalationClient, and attempt to escalate.
            // The reason string represents a free-form text string which will be reported in logs/telemetry in case of failures.
            // If successful, the function returns true, and if the extended override version was called, the UAC error code and a QSharedPointer to the
            // IEscalationClient instance are returned by reference.
            static bool quickEscalate(const QString& reason);
            static bool quickEscalate(const QString& reason, int& outErrCode, QSharedPointer<Origin::Escalation::IEscalationClient>& outEscalationClient);
            static EscalationWorker* quickEscalateASync(const QString& reason);

            static bool evaluateEscalationResult(const QString& reason, const QString& command, int errCode, int systemErrCode = 0);
        protected:
            explicit IEscalationClient(QObject* parent = 0);

        public:
            // EnableEscalationService
            //
            // Sets whether this code should use the external OriginClientService for processing commands.
            //
            void enableEscalationService(bool bEnableEscalationService);


            // ExecuteEscalationServiceCommand
            //
            // Low Level command execution function.
            // Returns an error id. See enum CommandError.
            //
            int executeEscalationServiceCommand(const QString & pCommand);


            // ExecuteProcess
            //
            // Spawns the process specified by the path, arguments, and working directory.
            // If the escalation service is enabled then the process is created in the 
            // privilege of the escalation service.
            //
            // Returns a ProcessId or 0. 
            // In the case of an invalid process id, the GetError and GetSystemError functions
            // can be used to get error information.
            //
            ProcessId executeProcess(const QString & pProcessPath, const QString & pArguments, const QString & pWorkingDirectory, const QString& environment, const QString &jobName);

            // TerminateProcess
            //
            // stops an application by killing the process
            //
            // Returns an error id.
            //
            qint32 terminateProcess(qint32 handle);

            // CloseProcess
            //
            // trys to stop an application cleanly by requesting it close
            //
            // Returns an error id.
            //
            qint32 closeProcess(qint32 processId);

            // addToMonitorList
            //
            // adds a process id and exe name to a list that will be checked if Origin.exe is killed and will be killed
            //
            // Returns an error id.
            //
            qint32 addToMonitorList(qint32 processId, const QString &processName);

            // removeFromMonitorList
            //
            // remove the entry from the list
            //
            // Returns an error id.
            //
            qint32 removeFromMonitorList(qint32 processId);

            // CreateDirectory
            //
            // Creates the given directory path.
            // This function currently can create only the leaf directory node and cannot 
            // create each entry in the path.
            //
            // Returns an error id. See enum CommandError.
            //
            // The access control list is platform-specific.
            // Windows documentation regardin ACL:
            //     http://msdn.microsoft.com/en-us/library/aa379567%28v=VS.85%29.aspx
            //     http://msdn.microsoft.com/en-us/library/ms717798%28VS.85%29.aspx
            //
            int createDirectory(const QString & pDirectory, const QString & pAccessControlList = QString());

#if defined(Q_OS_WIN)

            // InjectIGOIntoProcess
            //
            // injects IGO into a process
            //
            // Returns an error id.
            //
            qint32 injectIGOIntoProcess(qint32 processId, qint32 threadId);

            enum RegistryStringValueTypes
            {
                RegExpandSz = REG_EXPAND_SZ
                ,RegLink = REG_LINK
                ,RegMultiSz = REG_MULTI_SZ
                ,RegSz = REG_SZ
            };

            /// \brief Writes the registry key (string version).
			/// \param valType TBD.
            /// \param registryKey The handle to the registry key. It can be one of the following:
            ///        HKEY_CLASSES_ROOT
            ///        HKEY_CURRENT_CONFIG
            ///        HKEY_CURRENT_USER
            ///        HKEY_LOCAL_MACHINE
            ///        HKEY_USERS
            /// \param registrySubKey The name of the registry subkey to be opened, non sensitive.
            /// \param valueName The name of the value to be set/stored.
            /// \param val The string value to be set/stored.
			int setRegistryStringValue(RegistryStringValueTypes valType, HKEY registryKey, const QString& registrySubKey, const QString& valueName, const QString& val);

            enum RegistryIntValueTypes
            {
                RegDWord = REG_DWORD
                ,RegDWordBigEndian = REG_DWORD_BIG_ENDIAN
                ,RegQWord = REG_QWORD
            };

            /// \brief Writes the registry key (int version).
			/// \param valType TBD.
            /// \param registryKey The handle to the registry key.
            /// It can be one of the following:
            ///        HKEY_CLASSES_ROOT
            ///        HKEY_CURRENT_CONFIG
            ///        HKEY_CURRENT_USER
            ///        HKEY_LOCAL_MACHINE
            ///        HKEY_USERS
            /// \param registrySubKey The name of the registry subkey to be opened, non sensitive.
            /// \param valueName The name of the value to be set/stored.
            /// \param val The int value to be set/stored.
			/// \param base TBD.
            int setRegistryIntValue(RegistryIntValueTypes valType, HKEY registryKey, const QString& registrySubKey, const QString& valueName, __int64 val, int base = 10);

            /// \brief Deletes the subkey and value from the specified registry key.
            /// \param registryKey The registry key to remove the subkey from.
            /// \param registrySubKey The subkey to remove.
            int deleteRegistrySubKey(HKEY registryKey, const QString& registrySubKey);

#endif

#if defined(ORIGIN_MAC)
            // RunScript
            //
            // Run a compiled AppleScript file. For security purposes/because we really only use this for one purpose
            // right now, we hardcoded the content of the script and refer to it by name.
            //
            // Returns an error id. See enum CommandError.
            int runScript(const QString& scriptName);
            
            // EnableAssistiveDevices
            //
            // Enable the assistive devices setting for Mac.
            //
            // Returns an error id. See enum CommandError.
            int enableAssistiveDevices();
            
            // DeleteBundleLicense
            //
            // Deletes the OOA license from a Mac bundle.  This function will only delete license files, from a specific
            // bundle subdirectory path.
            //
            // Returns an error id. See enum CommandError.
            int deleteBundleLicense(const QString & pDirectory);
#endif
            
            // DeleteDirectory
            //
            // Delete a specific folder.
            //
            // Returns an error id. See enum CommandError.
            int deleteDirectory(const QString& dirName);

            // GetReturnString
            //
            // Generic function to get a result in string form.
            //
            bool getReturnString(int index, QString& sValue);


            // GetReturnString
            //
            // Generic function to get a result in qint64 form.
            //
            bool getReturnInt64(int index, qint64& value);


            // GetError
            //
            // Returns our error type. See enum CommandError.
            // A return value of kCommandErrorNone means there was no error in the last operation.
            //
            int getError() const;


            // GetSystemError
            //
            // Returns a system-specific error id. 
            // The return value is significant only if GetError returns an error value.
            // A return value of 0 means there was no system error, though GetError might still
            // have indicated an error.
            // 
            int getSystemError() const;


            // GetSystemErrorText
            // 
            // Generates a localized string representing the system error id.
            // Returns errorText.c_str().
            // 
            virtual void getSystemErrorText(int systemErrorId, QString& errorText) = 0;

            //checks to see if user is running the OriginClientServices elevated
            int checkIsRunningElevatedAndUACOK();
            bool startEscalationServiceApp(const QString & pServiceApplicationPath = QString());

            ProcessId getProcessId();

            bool hasUACExpired() { return mUACExpired; };
        protected:
            bool                pingEscalationService();
            void                parseEscalationServiceReturnValue();
            bool                connectToEscalationService(bool bConnect);
            bool                executeEscalationServiceCommand();

            virtual ProcessId   executeProcessInternal(const QString & pProcessPath, const QString & pProcessArguments, const QString & pProcessDirectory, const QString& environment) = 0;
            virtual void        createDirectoryFallback(const QString & pDirectory, const QString & pAccessControlList) = 0;
            virtual void        terminateProcessInternal(qint32 handle) = 0;
            virtual void        closeProcessInternal(qint32 processId) = 0;          
        protected:
            IPCClient*    mClient;
            bool          mbEnableEscalationService;
            int           mError;                               // See enum CommandError.
            int           mSystemError;                         //
            QString       mCommandText;                         // The raw received command text.
            QString       mResponseText;                        // 
            QStringList   mResponseArray;                       // 
            QString       mEscalationServiceAppPath;            // The path to OriginClientService.exe, defaults to "OriginClientService.exe" in the current path if empty.
        private:
            bool mUACExpired;
        };
    } // namespace Escalation
} // namespace Origin


#endif // Header include guard

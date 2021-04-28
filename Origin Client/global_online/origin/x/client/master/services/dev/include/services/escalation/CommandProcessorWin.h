///////////////////////////////////////////////////////////////////////////////
// CommandProcessorWin.h
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef COMMANDPROCESSORWIN_H
#define COMMANDPROCESSORWIN_H

#include <QString>
#include <QObject>
#include "qt_windows.h"
#include "ICommandProcessor.h"
#include "services/plugin/PluginAPI.h"


///////////////////////////////////////////////////////////////////////////////
// General command format:
//     Command:
//         <command type>\n
//         [<arg 0>\n]
//         [<arg 1>\n]
//         [. . .]
//         \0
//
//     Response:
//         <enum CommandError>\n
//         <system error id>\n
//         [(command specific return values)]\n
//         \0
//
// Echo command:
//     Command:
//         Echo\n
//         \0
//
//     Response:
//         0\n
//         0\n
//         \0
//
// Execute process command:
//     Command:
//         ExecuteProcess\n
//         <process path>\n
//         [<process arguments>]\n
//         [<working directory>]\n
//         \0
//
//     Response:
//         <enum CommandError>\n
//         <system error id>\n
//         <process id, 0 if error>\n
//         \0
//
// Create directory command:
//     Command:
//         CreateDirectory\n
//         <directory path>\n
//         <directory access rights>\n   // On Windows thss is a DACL string.
//         \0
//
//     Response:
//         <enum CommandError>\n
//         <system error id>\n
//         \0
//
///////////////////////////////////////////////////////////////////////////////


namespace Origin 
{ 

    namespace Escalation
    { 
        ///////////////////////////////////////////////////////////////////////////////
        // CommandProcessorWin
        ///////////////////////////////////////////////////////////////////////////////

        class ORIGIN_PLUGIN_API CommandProcessorWin : public ICommandProcessor
        {
            friend class ICommandProcessor;
            Q_OBJECT
        public:
            explicit CommandProcessorWin(QObject* parent = 0);
            explicit CommandProcessorWin(const QString& sCommandText, QObject* parent = 0);

            static void setProcessExecutionMode(bool createOnUserSession);
            static void setOriginPid(const quint32 pid);
        protected:
            
            bool setDirectoryPermissions(const QString& sFile, SECURITY_DESCRIPTOR* pSD, int &errCode);

            int  runScript();
            int  enableAssistiveDevices();
            int  executeProcess();
            int  injectIGOIntoProcess();
            int  terminateProcess();
            int  closeProcess();
            int  addToMonitorList();
            int  removeFromMonitorList();

            int  createDirectory();
            int  deleteBundleLicense();
            int  deleteDirectory();
            bool isOriginUserAdmin();
            void getSystemErrorText(int systemErrorId, QString& errorText);
            int setRegistryStringValue();
            int setRegistryIntValue();
            void setRegistryValuePostProcess();
            int deleteRegistrySubKey();

            static bool shellExecuteProcess(QString pProcessPath, QString pProcessArguments, QString pProcessDirectory, QString processEnvironment, quint32& pid, quint32& errorCode);
            static bool createProcessOnUserSession(QString pProcessPath, QString pProcessArguments, QString pProcessDirectory, QString processEnvironment, quint32& pid, quint32& errorCode);
        private:
            QString systemError();

        };

        enum CommandIndexes
        {
            RegistryKey  = 1
            ,RegistrySubKey
            ,RegistryValueName
            ,RegistryValue
            ,RegistryValueType
        };

    } // namespace Escalation

} // namespace Origin


#endif // Header include guard

///////////////////////////////////////////////////////////////////////////////
// CommandProcessorOSX.h
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef COMMANDPROCESSOROSX_H
#define COMMANDPROCESSOROSX_H

#include "ICommandProcessor.h"
#include <QString>
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
// Delete bundle license command:
//     Command:
//         DeleteBundleLicense\n
//         <directory path>\n
//         \0
//
//     Response:
//         <enum CommandError>\n
//         <system error id>\n
//         \0
//
// RunScript code command:
//     Command:
//         RunScript\n
//         <script name>\n
//         \0
//
//     Response:
//         <enum CommandError>\n
//         <system error id>\n
//         \0
//
// EnableAssistiveDevices code command:
//     Command:
//         EnableAssistiveDevices\n
//         \0
//
//     Response:
//         <enum CommandError>\n
//         <system error id>\n
//         \0
//
// DeleteDirectory code command:
//     Command:
//         DeleteDirectory\n
//         <directory name>\n
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
        // CommandProcessorOSX
        ///////////////////////////////////////////////////////////////////////////////

        class ORIGIN_PLUGIN_API CommandProcessorOSX : public ICommandProcessor
        {
            friend class ICommandProcessor;
            Q_OBJECT
        public:
            explicit CommandProcessorOSX(QObject* parent = 0);
            explicit CommandProcessorOSX(const QString& sCommandText, QObject* parent = 0);
        protected:
            int  runScript();
            int  enableAssistiveDevices();
            int  executeProcess();
            int  terminateProcess();
            int injectIGOIntoProcess();
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
            int deleteRegistrySubKey();
        };
    } // namespace Escalation

} // namespace Origin


#endif // Header include guard

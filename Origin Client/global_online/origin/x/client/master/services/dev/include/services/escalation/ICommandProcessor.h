///////////////////////////////////////////////////////////////////////////////
// ICommandProcessor.h
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef ICOMMANDPROCESSOR_H
#define ICOMMANDPROCESSOR_H

#include "IEscalationService.h"
#include <QObject>
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
        // ICommandProcessor
        ///////////////////////////////////////////////////////////////////////////////

        class ORIGIN_PLUGIN_API ICommandProcessor : public QObject
        {
            Q_OBJECT
        public:
            explicit ICommandProcessor(QObject* parent = 0);
            explicit ICommandProcessor(const QString& sCommandText, QObject* parent = 0);
            virtual ~ICommandProcessor() {};

            void reset();
            void setCommandText(const QString& sCommandText);
            int  processCommand(QString& sResponseText);
            int  processExternalError(QString& sResponseText, int error, const QString & pErrorText, bool bLastSystemError);
            int  getError() const;

        protected:
            void doSuccess();
            void doError(int error, const QString & pErrorText, bool bLastSystemError);
            int  parseCommandText();
            int  ping();

            virtual int closeProcess() = 0;
            virtual int terminateProcess() = 0;
            virtual int injectIGOIntoProcess() = 0;
            virtual int  runScript() = 0;
            virtual int  enableAssistiveDevices() = 0;
            virtual int  executeProcess() = 0;
            virtual int  createDirectory() = 0;
            virtual int  deleteBundleLicense() = 0;
            virtual int  deleteDirectory() = 0;
            virtual int  addToMonitorList() = 0;
            virtual int  removeFromMonitorList() = 0;
            virtual bool isOriginUserAdmin() = 0;
            virtual void getSystemErrorText(int systemErrorId, QString& errorText) = 0;
            virtual int setRegistryStringValue() = 0;
            virtual int setRegistryIntValue() = 0;
            virtual int deleteRegistrySubKey() = 0;

            int           mError;           // See enum CommandError.
            int           mSystemError;     // OS specific error
            QString       mCommandText;     // The raw received command text.
            QStringList   mCommandArray;    // The parsed command text.
            QString       mResponseText;    // 
            QStringList   mResponseArray;   // 
        };
    } // namespace Escalation

} // namespace Origin


#endif // Header include guard

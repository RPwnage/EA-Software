///////////////////////////////////////////////////////////////////////////////
// ICommandProcessor.cpp
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include "ICommandProcessor.h"

namespace Origin
{

    namespace Escalation
    {

        ICommandProcessor::ICommandProcessor(QObject* parent)
            : QObject(parent),
            mError(kCommandErrorNone),
            mSystemError(0),
            mCommandText(),
            mCommandArray(),
            mResponseText(),
            mResponseArray()
        {
        }


        ICommandProcessor::ICommandProcessor(const QString& sCommandText, QObject* parent)
            : QObject(parent),
            mError(kCommandErrorNone),
            mSystemError(0),
            mCommandText(),
            mCommandArray(),
            mResponseText(),
            mResponseArray()
        {
            setCommandText(sCommandText);
        }


        void ICommandProcessor::reset()
        {
            mError = kCommandErrorNone;
            mSystemError = 0;
            mCommandText.clear();
            mCommandArray.clear();
            mResponseText.clear();
            mResponseArray.clear();
        }


        void ICommandProcessor::setCommandText(const QString& sCommandText)
        {
            mCommandText = sCommandText;
        }


        int ICommandProcessor::processCommand(QString& sResponseText)
        {
            mError = kCommandErrorNone;

            int result = parseCommandText();

            if(result == kCommandErrorNone)
            {
                if(mCommandArray[0] == "Ping")
                    result = ping();
                else if(mCommandArray[0] == "ExecuteProcess")
                    result = executeProcess();
                else if(mCommandArray[0] == "CreateDirectory")
                    result = createDirectory();
                else if(mCommandArray[0] == "DeleteBundleLicense")
                    result = deleteBundleLicense();
                else if(mCommandArray[0] == "DeleteDirectory")
                    result = deleteDirectory();
                else if(mCommandArray[0] == "TerminateProcess")
                    result = terminateProcess();
                else if(mCommandArray[0] == "InjectIGOIntoProcess")
                    result = injectIGOIntoProcess();
                else if(mCommandArray[0] == "CloseProcess")
                    result = closeProcess();
                else if(mCommandArray[0] == "AddToMonitorList")
                    result = addToMonitorList();
                else if(mCommandArray[0] == "RemoveFromMonitorList")
                    result = removeFromMonitorList();
                else if(mCommandArray[0] == "RunScript")
                    result = runScript();
				else if(mCommandArray[0] == "EnableAssistiveDevices")
					result = enableAssistiveDevices();
                else if(mCommandArray[0] == "setRegistryStringValue")
                    result = setRegistryStringValue();
                else if(mCommandArray[0] == "setRegistryIntValue")
                    result = setRegistryIntValue();
                else if(mCommandArray[0] == "deleteRegistrySubKey")
                    result = deleteRegistrySubKey();
                
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_DEBUG << "Processed command [" << mCommandArray[0] << "], result was " << result << " and response text is [" << mResponseText << "]";
#endif
            }
            
            else
            {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_DEBUG << "Skipping empty command";
#endif
            }

            sResponseText = mResponseText;

            return result;
        }


        int ICommandProcessor::processExternalError(QString& sResponseText, int error, const QString & pErrorText, bool bLastSystemError)
        {
            doError(error, pErrorText, bLastSystemError);
            sResponseText = mResponseText;

            return error;
        }


        int ICommandProcessor::ping()
        {
            CommandError error = kCommandErrorNone;
            if(isOriginUserAdmin())
            {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_DEBUG << "Responding to ping() with ADMIN";
#endif
                mResponseText = "ADMIN\n";
            }
            else
            {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_DEBUG << "Responding to ping() that we do not have ADMIN.";
#endif
                mResponseText  = "0\n0\n";
                mResponseText += '\0';
            }
            return error;
        }



        // ParseCommandText
        //
        // We assume a command of the form:
        //     <val0>\n
        //     <val1>\n
        //     .
        //     .
        //
        int ICommandProcessor::parseCommandText()
        {
#if defined(Q_OS_WIN)
            #pragma warning(push)
            #pragma warning(disable: 4018)  // warning C4018: '<' : signed/unsigned mismatch
            // this is intended, n2 will overlfow on a negative value!!!
#endif
            for(quint32 n1 = 0, n2 = 0; n2 < quint32(mCommandText.length()); n1 = n2 + 1)
            {
                n2 = mCommandText.indexOf('\n', n1);

                if(n2 < quint32(mCommandText.length()))
                {
                    QString sValue(mCommandText.mid(n1, n2 - n1));

                    mCommandArray.push_back(sValue);
                }
            }
#if defined(Q_OS_WIN)
            #pragma warning(pop)
#endif
            if(mCommandArray.isEmpty())
                mError = kCommandErrorInvalidCommand;
            else
                mError = kCommandErrorNone;

            return mError;
        }



        void ICommandProcessor::doSuccess()
        {
            mError = kCommandErrorNone;

            mResponseArray.clear();
            mResponseText.clear();

            mResponseArray.append("0\n");
            mResponseText    += mResponseArray[0];

            mResponseArray.append("0\n");
            mResponseText    += mResponseArray[1];
        }



        // doError
        //
        // Writes the following:
        //     <enum CommandError>
        //     <system error id>
        //
        // Example output mResponseArray:
        //     2
        //     0x8003482
        //
        void ICommandProcessor::doError(int error, const QString & pErrorText, bool bLastSystemError)
        {
            QString sLogMessage;

            mError = error;

            mResponseArray.clear();
            mResponseText.clear();

            // Error id
            mResponseArray.append(QString::number(error)+"\n");
            sLogMessage       += mResponseArray[0];
            mResponseText     += mResponseArray[0];

            // Error string
            sLogMessage       += pErrorText;
            sLogMessage       += "\n";

            if(bLastSystemError)
            {
                // System  error id
                mResponseArray.append(QString::number(mSystemError, 16)+"\n");
                sLogMessage       += mResponseArray[1];
                mResponseText     += mResponseArray[1];

                // System error string
                QString buffer;
                getSystemErrorText(mSystemError, buffer);
                sLogMessage += buffer;
                sLogMessage += "\n";
            }
            else
            {
                // System  error id
                mResponseArray.append("0x0000000\n");
                sLogMessage       += mResponseArray[1];
                mResponseText     += mResponseArray[1];

                // System error string
                sLogMessage += "\n";
            }
        }


        int ICommandProcessor::getError() const
        {
            return mError;
        }

    } // namespace Escalation
} // namespace Origin


///////////////////////////////////////////////////////////////////////////////
// CommandFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef COMMANDFLOW_H
#define COMMANDFLOW_H

#include "AbstractFlow.h"
#include "FlowResult.h"
#include "services/plugin/PluginAPI.h"

#include <qpointer.h>
#include <qscopedpointer.h>
#include <qmap.h>
#include <qstringlist.h>
#include <qfile.h>

namespace Origin
{
    namespace Services
    {
        class Setting;
        class Variant;
    }

    namespace Client
    {
        class PendingActionFlow;
        /// \brief Handles all high-level actions related to the main flow.
        class ORIGIN_PLUGIN_API CommandFlow : public AbstractFlow
        {
            Q_OBJECT

        public:
            /// \brief Initiates the flow execution.
            virtual void start();

            /// \brief Creates a new CommandFlow object.
            static void create();

            /// \brief Destroys the current CommandFlow object.
            static void destroy();

            /// \brief Gets the current CommandFlow object.
            /// \return The current CommandFlow object.
            static CommandFlow* instance();

            /// \brief set member var mCommandLine
            /// \param commandLine string
            void setCommandLine (QString& cmdLine);

            /// \brief set member var command line
            /// \param cmdLineArgs list of command line arguments
            void setCommandLine (QStringList cmdLineArgs);

            /// \brief set member var command line
            /// \param cmdLineArgs list of command line arguments, set of 8-bit arguments for verification - from bootstrapper, double-quotes get stripped out
            void setCommandLine (QStringList cmdLineArgs, int argc, char **argv);

            /// \brief append the argument to existing mCommandLine and mCommandLineArgs
            /// \param cmdArg the argument to append
            void appendToCommandLine (QString cmdArg);

            /// \brief if param exists, then replace the expression
            // \param param = name of the parameter
            // \param exp  expression to replace
            void removeParameter (QString param, const QRegularExpression exp);

            /// \brief takes the original origin:// url and translated it into origin2:// format
            QUrl translateToOrigin2 (QString& originalProtocolStr);

            /// \brief returns the entire command line
            QString& commandLine () { return mCommandLine;}

            /// \brief returns a set of command line arguments
            QStringList& commandLineArgs () { return mCommandLineArgs; }

            /// \brief searches the list of command line arguments for origin:// or origin2://
            /// \return origin:// or origin2:// argument, if not found, then returns empty
            const QString urlProtocolArgument ();

            /// \brief searches the list of command line arguments to see if arg is contained the arg list; it's not a strict comparison for equality but just checks contains()
            /// \return true if arg is contained in the list of arguments 
            bool findArgument (const QString &arg);

            bool isActionPending () { return (!urlProtocolArgument().isEmpty()); }

            bool parseRtPFile(QString rtpPath, QString &contentIds, QString &contentTitle);

            bool isLaunchedFromBrowser();
            bool isOpenAutomateParam();
            QUrl getActionUrl();

        private:
            /// \brief The CommandFlow constructor.
            CommandFlow();

            /// \brief The CommandFlow destructor.
            virtual ~CommandFlow();

            /// \brief hack to restore the command line when it contains origin:// because IE9 replaced %20 in origin:// with " "
            QStringList fixOriginProtocolCmdLine (QString& cmdLine);

            //functions to translate from origin:// to origin2://
            QUrl translateHostStore (QUrl& commandUrl, bool fromJumpList);
            QUrl translateHostLibrary (QUrl& commandUrl, bool fromJumpList);
            QUrl translateHostLaunchGame (QUrl& commandUrl, bool bParseRTP, bool fromJumpList);
            QUrl translateToCurrentTab (QString& urlHost);

            /// \brief Returns the hash of xmlFile with the value of hash replaced by the salt value
            QString calculateRtPHash(QFile &rtpFile, const QString &hash);

            /// \brief Checks for origin scheme first, then eadm, and then if the url is not first in the list, remove the initial part of the command line so that
            /// the url is first.
            /// \param ln The line to check.
            /// \return The QUrl created from the line.
            QUrl getUrlFromCommandLine(QString const& ln);


            QString mCommandLine;
            QStringList mCommandLineArgs;
        };
    }
}

#endif // COMMANDFLOW_H

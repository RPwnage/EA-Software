///////////////////////////////////////////////////////////////////////////////
// CommandLine.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <QString>

#include "services/plugin/PluginAPI.h"

class LoginComponent;
class QUrl;

namespace Origin
{
    namespace Client
    {
        /// \brief The %CommandLine namespace contains the functions that parse %Origin Client commandline parameters.
        ///
        /// <span style="font-size:14pt;color:#404040;"><b>See the relevant wiki docs:</b></span> <a style="font-weight:bold;font-size:14pt;color:#eeaa00;" href="https://developer.origin.com/documentation/display/EBI/Origin+Command-Line+Options">Origin Command-Line Options</a>
        namespace CommandLine
        {	
            /// \brief Returns escaped characters in the given string to their original format.
            /// \param sStringToDecode This function will modify this value.  The string to unescape.
            ORIGIN_PLUGIN_API void UnicodeUnescapeString(QString& sStringToDecode);

            /// \brief Breaks up a QString into a QStringList, keeping the spaces between quotes.
            /// \param commandLine This function will modify this value.  The string to break up.
            ORIGIN_PLUGIN_API QStringList parseCommandLineWithQuotes(QString commandLine);
        }
    }
}

#endif
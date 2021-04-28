//  CommandLine.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "CommandLine.h"
#include "common/source/OriginApplication.h"
#include "services/settings/SettingsManager.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"

#include <string>
#include <sstream>
#include <QString>
#include <QUrl>
#include <QDomDocument>

namespace Origin
{
    namespace Client
    {
        // This decodes all characters from Unicode escape-sequences %uXXXX
        void CommandLine::UnicodeUnescapeString(QString& sStringToDecode)
        {
            int nIndex = 0;
            while ( (nIndex = sStringToDecode.indexOf("%u")) >= 0 )
            {
                bool ok;
                QString sValue(sStringToDecode.mid(nIndex + 2, 4));
                uint nValue = sValue.toUInt(&ok, 16);
                sStringToDecode.remove(nIndex, 6);
                sStringToDecode.insert(nIndex, QChar(nValue));
            }
        }

        // Keep the spaces between quotes. Because file names may have spaces, we cannot use ' ' as a seperator.
        QStringList CommandLine::parseCommandLineWithQuotes(QString commandLine)
        {
            int     outsideQuotes = 1;  // start off being outsde a pair of quotes
            for (QChar *data = commandLine.data(); *data != 0; data++)
            {
                if (*data == '"')
                    outsideQuotes++;    // toggle low bit to keep track of being inside or outside a pair of quotes
                else if ((*data == ' ') && (outsideQuotes & 1))
                    *data = 0x1F;    // convert the space to 0x1F (unit seperator character) if not inside quotes
            }
            QStringList argv = commandLine.split(0x1F, QString::SkipEmptyParts); // now we can use 0x1F to seperate the args
            return argv;
        }

    } //namespace Client
} //namespace Origin

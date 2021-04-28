///////////////////////////////////////////////////////////////////////////////
// LocalizedContentString.h
//
// taken from original CoreLanguage.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _LOCALIZEDCONTENTSTRING_H
#define _LOCALIZEDCONTENTSTRING_H

#include <QObject>
#include <QString>
#include <QLocale>
#include "engine/downloader/IContentInstallFlow.h"
#include "engine/downloader/ContentInstallFlowState.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client 
    {
        /// \brief Class for formatting strings used by content (usually downloader stats).
        class ORIGIN_PLUGIN_API LocalizedContentString : public QObject
        {
            
        public:
	        /// \brief Creates a new LocalizedContentString for the passed locale.
	        /// \param locale  Locale to format for. Defaults to the current locale.
	        LocalizedContentString(const QLocale &locale = QLocale(), QObject* parent = NULL);

	        /// \brief Formats a byte value for localized display as: "(Bytes in Scaled Units) / (Unit Label)". 
            /// For example, 12121 bytes would be formatted to: 12.1 KB
	        /// \param nBytes Actual number of bytes.
	        /// \param bShowGB True if we should show values in gigabytes, otherwise, only show megabytes or kilobytes.
	        /// \return The formatted string.
    		QString makeByteSizeString( qint64 nBytes, bool bShowGB = true );

            /// \brief Returns "XX (unit) of YY (unit)", localized.
            /// \param nBytesDownloaded Number of bytes downloaded so far.
            /// \param totalBytes Total download size.
            /// \return The formatted string.
            QString makeProgressDisplayString (qint64 nBytesDownloaded, qint64 totalBytes);

            /// \brief Returns "XX (unit) / second", localized.
            /// \param bps Bytes per second.
            /// \return The formatted string.
            QString makeProgressRateDisplayString (qint64 bps);

            /// \brief Returns "time remaining: (localized time remaining)"
            /// \param elapsedSeconds The number of seconds.
            /// \return The formatted string.
            QString makeTimeRemainingString (qint64 elapsedSeconds);

            /// \brief Returns flow state string (e.g. RUNNING)
            /// \param ContentInstallFlowState::value Current operation state
            /// \return The formatted string.
            QString flowStateCode(const Downloader::ContentInstallFlowState::value& flowState);

            /// \brief Returns operation string (e.g. UPDATING)
            /// \param ContentOperationType Type of operation
            /// \return The formatted string.
            QString operationTypeCode(const Downloader::ContentOperationType& operationType);

            /// \brief Returns operation phase string (e.g. Updating Paused)
            /// \param ContentOperationType Type of operation
            /// \param ContentInstallFlowState::value Current operation state
            /// \return The formatted string.
            QString operationPhaseDiplay(const Downloader::ContentOperationType& operationType, const Downloader::ContentInstallFlowState::value& flowState);

            /// \brief Returns operation phase string (e.g. Updating Paused)
            /// \param QString operation type code (DOWNLOAD, UPLOAD, PRELOAD, etc)
            /// \param QString flow state code (READYTOSTART, RUNNING, etc)
            /// \return The formatted string.
            QString operationPhaseDiplay(const QString& operationTypeCode, const QString& flowStateCode);

            /// \brief Returns operation phase string (e.g. Update)
            /// \param ContentOperationType Type of operation
            /// \return The formatted string.
            QString operationDisplay(const Downloader::ContentOperationType& operationType);

            /// \brief Returns bytes in localized format e.g. 4 B, 4 MB, 4 GB 
            QString formatBytes(const qulonglong& bytes);

        private:
            QLocale mLocale; ///< The locale.
            QMap<QString, QString> mOperationStrMap;
        };

    }
}

#endif

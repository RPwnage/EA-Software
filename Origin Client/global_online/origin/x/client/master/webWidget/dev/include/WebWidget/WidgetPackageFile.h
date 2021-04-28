#ifndef _WEBWIDGET_WIDGETPACKAGEFILE_H
#define _WEBWIDGET_WIDGETPACKAGEFILE_H

#include <QString>
#include <QByteArray>
#include <QFileInfo>

#include "WebWidgetPluginAPI.h"

namespace WebWidget
{
    ///
    /// Represents a file inside a widget package
    ///
    class WEBWIDGET_PLUGIN_API WidgetPackageFile
    {
        friend class WidgetPackage;
    public:
        ///
        /// Creates a non-existent package file
        ///
        /// \param packagePath  Path to the non-existent file
        ///
        explicit WidgetPackageFile(const QString &packagePath = QString()) :
            mPackagePath(packagePath), mExists(false)
        {
        }

        ///
        /// Returns the canonical package path of the file
        ///
        QString packagePath() const { return mPackagePath; }

        ///
        /// Returns information about the backing local file
        ///
        QFileInfo backingFileInfo() const { return mBackingFileInfo; }

        ///
        /// Returns the media type of the file
        ///
        /// \return Media type or null QByteArray if sniffing should be used
        ///
        QByteArray mediaType() const { return mMediaType; }
        
        ///
        /// Returns the character encoding of the file
        ///
        /// \return Encoding or null QByteArray if sniffing should be used
        ///
        QByteArray encoding() const { return mEncoding; }

        ///
        /// Return if this file exists
        ///
        bool exists() const { return mExists; }

    protected:
        ///
        /// Creates an existent package file
        ///
        /// \param packagePath      Canonical package path of the file
        /// \param backingFileInfo  Information about the local backing file
        /// \param mediaType        Media type of the file or null if sniffing should be used
        /// \param encoding         Encoding of the file or null if sniffing should be used
        ///
        WidgetPackageFile(const QString &packagePath, const QFileInfo &backingFileInfo, const QByteArray &mediaType, const QByteArray encoding = QByteArray()) :
            mPackagePath(packagePath), 
            mBackingFileInfo(backingFileInfo), 
            mMediaType(mediaType),
            mEncoding(encoding),
            mExists(true)
        {
        }

    private:
        QString mPackagePath;
        QFileInfo mBackingFileInfo;
        QByteArray mMediaType;
        QByteArray mEncoding;
        bool mExists;
    };
}

#endif

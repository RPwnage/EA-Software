#ifndef _WEBWIDGET_WIDGETDIRECTORYFILEPROVIDER_H
#define _WEBWIDGET_WIDGETDIRECTORYFILEPROVIDER_H

#include <QStringList>
#include "WidgetFileProvider.h"

#ifdef _DEBUG
// Emulate a case sensitive filesystem even if the underlying filesystem isn't
// This can reduce performance so it is only enabled for debug
#define WEBWIDGET_EMULATE_CASESENSITIVITY
#endif

namespace WebWidget
{
    ///
    /// WidgetFileProvider that serves files directly from a directory on the local filesystem
    ///
    /// Widget directories mirror the contents of the ZIP package file on the local filesystem. This in intended to
    /// be used for development or for reading from resources files. When dealing with zipped package files use 
    /// UnpackedArchive and UnpackedArchiveFileProvider instead.
    ///
    class WEBWIDGET_PLUGIN_API WidgetDirectoryFileProvider : public WidgetFileProvider
    {
    public:
        ///
        /// Creates a new WidgetDirectoryFileProvider
        ///
        /// \param localPath  Root path of the widget on the local filesystem
        ///
        explicit WidgetDirectoryFileProvider(const QString &localPath);

        QFileInfo backingFileInfo(const QString &canonicalPackagePath) const;

    private:
        QString mLocalPath;

#ifdef WEBWIDGET_EMULATE_CASESENSITIVITY
        void addValidPaths(const QString &dir);

        QStringList mValidPaths;
#endif
    };
}

#endif

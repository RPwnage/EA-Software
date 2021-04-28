#ifndef _WEBWIDGET_WIDGETPACKAGE_H
#define _WEBWIDGET_WIDGETPACKAGE_H

#include <QString>
#include <QByteArray>
#include <QSharedDataPointer>

#include "WebWidgetPluginAPI.h"
#include "WidgetPackageFile.h"
#include "WidgetConfiguration.h"
#include "WidgetFileProvider.h"

namespace WebWidget
{
    class InstalledWidget;

    ///
    /// Represents a packaged widget and allows lookups of files inside a widget package
    ///
    /// This class should not be used directly as it does not implement higher level functionality such as localization
    /// rules. WidgetPackageReader should be used to access package contents instead.
    ///
    class WEBWIDGET_PLUGIN_API WidgetPackage
    {
    public:
        ///
        /// Creates a new widget package with the given file provider
        ///
        /// \param  fileProvider  Backend provider of widget files. 
        ///                       Passing NULL can be used to create a null package
        ///
        explicit WidgetPackage(const WidgetFileProvider *fileProvider = NULL);

        ///
        /// Returns if this is a null widget package
        ///
        bool isNull() const;

        ///
        /// Returns the widget package file for a given path
        ///
        /// This function is primarily intended for WidgetPackageReader. See WidgetPackageReader::findFile() for
        /// the high level, locale-aware version of this function
        ///
        /// \param  packagePath        Path of the package file to search for
        /// \param  providedMediaType  Media type for the file if provided by the referencing document
        /// \param  providedEncoding   Character encoding for the file if provided by the referencing document
        /// \return WidgetPackageFile for the file. Use WidgetPackageFile::exists() to determine if the path exists
        ///
        WidgetPackageFile packageFile(const QString &packagePath, 
            const QByteArray &providedMediaType = QByteArray(), 
            const QByteArray &providedEncoding = QByteArray()) const;

        ///
        /// Returns the canonical widget path for a given widget path
        ///
        /// This removes redundant path elements and disallows transversal above the package root directory
        ///
        /// \param  path  Forward slash separated widget path
        /// \return Canonicalized widget path or null QString for invalid paths
        ///
        static QString canonicalizePath(const QString &path);

    private:
        mutable WidgetConfiguration mConfiguration;
        
        QSharedDataPointer<const WidgetFileProvider> mFileProvider;
    };
}

#endif
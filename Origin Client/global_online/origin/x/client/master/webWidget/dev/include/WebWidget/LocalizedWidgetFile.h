#ifndef _WEBWIDGET_LOCALIZEDWIDGETFILE_H
#define _WEBWIDGET_LOCALIZEDWIDGETFILE_H

#include <QString>
#include <QByteArray>

#include "WebWidgetPluginAPI.h"

namespace WebWidget
{
    ///
    /// Represents a file inside a widget package returned from a localized search 
    ///
    /// This extends WidgetPackageFile to keep track of the path used to initially search for the file (referred to
    /// as the widget path) in addition to the path of the localized file within the widget package (referrred to as
    /// the package path). For files that are not explicitly localized the widget path and package path will be
    /// identical.
    ///
    /// \sa WidgetPackageReader::findFile
    ///
    class WEBWIDGET_PLUGIN_API LocalizedWidgetFile : public WidgetPackageFile
    {
        friend class WidgetPackageReader;
    public:
        ///
        /// Creates a non-existent localized widget file
        ///
        /// \param widgetPath  Path to the non-existent file
        ///
        explicit LocalizedWidgetFile(const QString &widgetPath = QString()) :
            WidgetPackageFile(widgetPath),
            mWidgetPath(widgetPath)
        {
        }

        ///
        /// Returns the canonical widget path of the file
        ///
        /// This is the path the widget uses to refer to the file before any localization rules have been applied.
        /// For example, if a lookup for images/foo.png matched locales/en-us/images/foo.png this would return
        /// images/foo.png
        ///
        QString widgetPath() const { return mWidgetPath; }

    private:
        LocalizedWidgetFile(const QString &widgetPath,
                const QString &packagePath, 
                const QFileInfo &backingFileInfo,
                const QByteArray &mediaType, 
                const QByteArray encoding = QByteArray()) :
            WidgetPackageFile(packagePath, backingFileInfo, mediaType, encoding),
            mWidgetPath(widgetPath)
        {
        }

        static LocalizedWidgetFile fromPackageFile(const QString &widgetPath, const WidgetPackageFile &packageFile)
        {
            if (packageFile.exists())
            {
                return LocalizedWidgetFile(widgetPath, 
                        packageFile.packagePath(),
                        packageFile.backingFileInfo(),
                        packageFile.mediaType(),
                        packageFile.encoding());
            }
            else
            {
                return LocalizedWidgetFile(widgetPath);
            }
        }

        QString mWidgetPath;
    };
}

#endif

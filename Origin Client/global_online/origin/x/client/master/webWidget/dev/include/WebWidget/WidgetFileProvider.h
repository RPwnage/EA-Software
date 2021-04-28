#ifndef _WEBWIDGET_WIDGETFILEPROVIDER_H
#define _WEBWIDGET_WIDGETFILEPROVIDER_H

#include <QString>
#include <QSharedData>
#include <QFileInfo>

#include "WebWidgetPluginAPI.h"

namespace WebWidget
{
    ///
    /// Interface for mapping canonical widget paths to backing local files
    ///
    /// Subclasses can be used to define new ways of looking up widget package contents based on their widget path.
    /// They can also implement rules to filter access to widget packages that have been modified or compromised.
    ///
    class WEBWIDGET_PLUGIN_API WidgetFileProvider : public QSharedData
    {
    public:
        ///
        /// Returns the backing file information for a canonical widget path
        ///
        /// \param  canonicalPackagePath  Canonical path relative to the widget package root. 
        ///                               This has been previously been processed using 
        ///                               WidgetPackage::canonicalizePath() and can be trusted to not contain relative
        ///                               path elements.
        /// \return QFileInfo instance describing the local file 
        ///
        virtual QFileInfo backingFileInfo(const QString &canonicalPackagePath) const = 0;
        virtual ~WidgetFileProvider(){};
    };
}

#endif

#ifndef _WEBWIDGET_INSTALLEDWIDGET_H
#define _WEBWIDGET_INSTALLEDWIDGET_H

#include <QString>

#include "WebWidgetPluginAPI.h"
#include "WidgetPackage.h"
#include "SecurityOrigin.h"

namespace WebWidget
{
    class WidgetRepository;
    class WidgetFileProvider;
    class UnpackedArchive;

    ///
    /// Represents a locally installed version of a web widget
    ///
    /// InstalledWidget instances can come from two places:
    /// -# Returned from a WidgetRepository, a managed repository of bundled widgets and their updates.
    /// -# Created from an UnpackedArchive when dealing directly with a ZIP widget package (.WGT files)
    ///
    class WEBWIDGET_PLUGIN_API InstalledWidget
    {
        friend class WidgetRepository;

    public:
        ///
        /// Creates a null InstalledWidget
        ///
        InstalledWidget() : 
           mNull(true), mPackage(NULL) {}

        ///
        /// Create an InstalledWidget from an UnpackedArchive
        ///
        /// \param unpackedArchive  UnpackedArchive to create the installed widget from
        /// \param authority        Authority for the security origin. If none is specified a temporary one will be
        ///                         generated using generateAuthority()
        ///
        /// \sa generateAuthority()
        ///
        InstalledWidget(const UnpackedArchive &unpackedArchive, const QString &authority = QString());

        ///
        /// Generates a new widget authority
        ///
        /// This is useful for generating an authority for an unpacked archive to be used across multiple invocations. 
        /// As long as InstalledWidget(const UnpackedArchive &, const QString &) is called with the same authority
        /// the widget will be able to access local storage from previous invocations. 
        ///
        static QString generateAuthority();

        ///
        /// Returns the security origin of the widget
        ///
        /// This can be used to uniquely identify the widget across widget versions
        ///
        SecurityOrigin securityOrigin() const { return mSecurityOrigin; }
    
        ///
        /// Returns if this is a null widget
        ///
        bool isNull() const;

        ///
        /// Returns the widget package
        ///
        /// This contains the widget's files. To read a widget package instantiate a WidgetPackageReader
        ///
        const WidgetPackage& package() const { return mPackage; }

    private:
        ///
        /// Creates an InstalledWidget from a WidgetFileProvider and SecurityOrigin
        ///
        /// \param  fileProvider    Provider for the widget's package files
        /// \param  securityOrigin  Security origin for the widget
        ///
        InstalledWidget(WidgetFileProvider *fileProvider, const SecurityOrigin &securityOrigin);

        SecurityOrigin mSecurityOrigin;

        bool mNull;
        WidgetPackage mPackage;
    };
}

#endif

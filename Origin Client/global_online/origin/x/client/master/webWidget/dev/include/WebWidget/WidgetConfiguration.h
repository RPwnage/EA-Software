#ifndef _WEBWIDGET_WIDGETCONFIGURATION_H
#define _WEBWIDGET_WIDGETCONFIGURATION_H

#include <QString>
#include <QList>
#include <QMap>

#include "WebWidgetPluginAPI.h"
#include "AccessRequest.h"
#include "FeatureRequest.h"
#include "CustomStartFile.h"
#include "UriTemplate.h"

class QDomElement;

namespace WebWidget
{
    class WidgetPackageFile;

    ///
    /// Represents a widget's configuration document
    ///
    /// Conforms to a subset of the configuration format defined in:
    /// - http://www.w3.org/TR/widgets/
    /// - http://www.w3.org/TR/widgets-access/
    ///
    class WEBWIDGET_PLUGIN_API WidgetConfiguration
    {
    public:
        ///
        /// Creates a widget configuration from a configuration document
        ///
        /// \param  configDoc  Configuration document to parse
        ///
        explicit WidgetConfiguration(const WidgetPackageFile &configDoc);
        
        ///
        /// Creates an invalid widget configuration
        ///
        WidgetConfiguration();

        ///
        /// Returns if the widget configuration is valid
        ///
        bool isValid() const { return mValid; }

        ///
        /// Returns the widget version
        ///
        /// This will return a null QString if a version is not specified in the widget's configuration
        ///
        QString version() const { return mVersion; }

        ///
        /// Returns the custom start file for the widget
        ///
        /// If specified this will be used instead of searching for a default start file. This field is optional
        ///
        CustomStartFile customStartFile() const { return mCustomStartFile; }

        ///
        /// Returns a list of widget access requests made in the configuration
        ///
        QList<AccessRequest> accessRequests() const { return mAccessRequests; }

        ///
        /// Returns a list of widget feature requests made in the configuration
        ///
        QList<FeatureRequest> featureRequests() const { return mFeatureRequests; }

        ///
        /// Returns the default locale configured for the widget
        ///
        /// This is used as a fallback for localized content lookups
        ///
        /// \return Configured default locale or null QString if none was specified
        ///
        QString defaultLocale() const { return mDefaultLocale; }

        ///
        /// Returns a list of additional content links
        ///
        /// This allows deep linking in to the widget package for specific links identified by a role URI. Configuring
        /// these links in the widget package abstracts the layout of the widget package files from the user of the
        /// widget.
        ///
        /// This is an Origin extension
        ///
        /// \return Map of link role URIs to package path templates
        ///
        /// \sa WidgetView::loadLink
        ///
        QMap<QString, UriTemplate> links() const { return mLinks; }

    private:
        void handleWidgetElement(const QDomElement &);
        void handleAccessElement(const QDomElement &);
        void handleContentElement(const QDomElement &);
        void handleFeatureElement(const QDomElement &);
        void handleLinkElement(const QDomElement &);

        bool mValid;

        QString mVersion;
        QString mDefaultLocale;
        QList<AccessRequest> mAccessRequests;
        QList<FeatureRequest> mFeatureRequests;
        CustomStartFile mCustomStartFile;
        QMap<QString, UriTemplate> mLinks;
    };
}

#endif

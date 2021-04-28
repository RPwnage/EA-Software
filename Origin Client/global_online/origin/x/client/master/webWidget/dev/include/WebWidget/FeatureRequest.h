#ifndef _WEBWIDGET_FEATUREREQUEST_H
#define _WEBWIDGET_FEATUREREQUEST_H

#include <QString>
#include <QMultiHash>

#include "WebWidgetPluginAPI.h"

namespace WebWidget
{
    ///
    /// Hash of feature request parameter name-value pairs
    ///
    typedef QMultiHash<QString, QString> FeatureParameters;

    ///
    /// Represents a widget's request to access a URI identifiable runtime component
    ///
    /// Origin uses widget feature requests to dynamically bind JavaScript APIs requested by the widget
    ///
    class WEBWIDGET_PLUGIN_API FeatureRequest
    {
    public:
        ///
        /// Creates a new feature request
        ///
        /// \param  uri         URI identifying the requested feature
        /// \param  required    Flag indicating if the feature is required for the widget the function correctly
        /// \param  parameters  Widget specified parameters for the feature
        ///
        explicit FeatureRequest(const QString &uri, bool required = true, const FeatureParameters &parameters = FeatureParameters()) :
            mUri(uri),
            mRequired(required),
            mParameters(parameters)
        {
        }

        ///
        /// Returns the URI identifying the requested feature
        ///
        /// This value only has meaning to the widget and user agents supporting the feature.
        ///
        QString uri() const { return mUri; }

        ///
        /// Returns if this feature is required for the widget to function correctly
        ///
        /// Widgets should not be loaded if the user agent doesn't support a required feature
        ///
        bool required() const { return mRequired; }

        ///
        /// Returns a list of parameters defined by the widget
        ///
        /// These are meaningful to the feature being requested but are otherwise opaque
        ///
        FeatureParameters parameters() const { return mParameters; }

    private:
        QString mUri;
        bool mRequired;
        FeatureParameters mParameters;
    };
}

#endif

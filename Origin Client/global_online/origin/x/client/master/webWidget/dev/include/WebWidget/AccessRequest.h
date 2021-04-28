#ifndef _WEBWIDGET_ACCESSREQUEST_H
#define _WEBWIDGET_ACCESSREQUEST_H

#include <QUrl>

#include "WebWidgetPluginAPI.h"
#include "SecurityOrigin.h"

namespace WebWidget
{
    ///
    /// Represents a widget's request to access security origins over the network
    ///
    /// These access requests are statically defined in the widget's configuration and enforced at runtime
    /// for both resources requested from markup and dynamically by scripts.
    ///
    /// Note this is only designed for HTTP-like URL schemes such as http, https and widget. This is safe as
    /// WidgetNetworkAccessManager only supports those schemes. Use caution when applying to other URL schemes
    ///
    class WEBWIDGET_PLUGIN_API AccessRequest
    {
    public:
        ///
        /// Scope of the access request relative to the security origin
        ///
        enum AccessScope
        {
            ///
            /// This request only applies to the specific security origin
            ///
            SecurityOriginOnly,

            ///
            /// This request applies the security origin and its subdomains
            ///
            SecurityOriginAndSubdomains,

            ///
            /// This request applies to all security origins
            ///
            AllResources
        };

        ///
        /// Creates a new widget access request
        ///
        /// \param  scope           Scope of this access request
        /// \param  securityOrigin  Security origin for the access request. 
        ///                         Should be left as default for the scope of AllResources
        ///
        explicit AccessRequest(AccessScope scope, const SecurityOrigin &securityOrigin = SecurityOrigin());

        ///
        /// Tests if this access request applies to a given resource URL
        ///
        bool appliesToResource(const QUrl &) const;

        ///
        /// Returns the requested security origin 
        ///
        SecurityOrigin securityOrigin() const { return mSecurityOrigin; }

        ///
        /// Returns the scope of this request
        ///
        AccessScope scope() const { return mScope; }

    private:
        SecurityOrigin mSecurityOrigin;
        AccessScope mScope;
    };
}

#endif

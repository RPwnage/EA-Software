#ifndef _WEBWIDGET_SECURITYORIGIN_H
#define _WEBWIDGET_SECURITYORIGIN_H

#include <QString>
#include <QUrl>

#include "WebWidgetPluginAPI.h"

namespace WebWidget
{
    ///
    /// Represents a web security origin
    ///
    /// See http://en.wikipedia.org/wiki/Same_origin_policy for more information
    /// 
    class WEBWIDGET_PLUGIN_API SecurityOrigin
    {
    public:
        ///
        /// Represents an unknown port number
        ///
        static const int UnknownPort = -1;

        ///
        /// Creates a security origin from components
        ///
        SecurityOrigin(const QString &scheme, const QString &host, int port);

        ///
        /// Creates a security origin from a URL
        ///
        explicit SecurityOrigin(const QUrl &);

        ///
        /// Creates a null security origin
        ///
        SecurityOrigin();

        ///
        /// Returns if this security origin is null
        ///
        bool isNull() const { return mScheme.isEmpty() || mHost.isEmpty(); }

        ///
        /// Scheme this security origin applies to
        /// 
        QString scheme() const { return mScheme; }

        ///
        /// Host this security origin applies to
        ///
        QString host() const { return mHost; }

        ///
        /// Port this security origin applies to
        ///
        int port() const { return mPort; }

        ///
        /// Returns the default port for a scheme
        ///
        /// \return Port number or SecurityOrigin::UnknownPort on error
        ///
        static int defaultPortForScheme(const QString &scheme);

        ///
        /// Compares two security origins for equality
        ///
        bool operator==(const SecurityOrigin &) const;
        
        ///
        /// Compares two security origins for inequality
        ///
        bool operator!=(const SecurityOrigin &other) const
        {
            return !(*this == other);
        }

    private:
        QString mHost;
        int mPort;
        QString mScheme;
    };
}

#endif
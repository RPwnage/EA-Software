#include "AccessRequest.h"

namespace WebWidget
{
    AccessRequest::AccessRequest(AccessScope scope, const SecurityOrigin &securityOrigin) :
        mSecurityOrigin(securityOrigin), mScope(scope)
    {
    }

    bool AccessRequest::appliesToResource(const QUrl &url) const
    {
        if (scope() == AllResources)
        {
            return true;
        }

        // Save our scheme as its referenced a lot
        QString scheme(url.scheme().toLower());

        // Right scheme?
        if (scheme != securityOrigin().scheme())
        {
            return false;
        }

        // Right port?
        int defaultPort(SecurityOrigin::defaultPortForScheme(scheme));

        if (url.port(defaultPort) != securityOrigin().port())
        {
            return false;
        }

        // Compare the lowercase ACE forms as required by the standard
        // This presumably avoids some trickiness people could do with Unicode forms
        QByteArray canonicalOriginHost(QUrl::toAce(securityOrigin().host()).toLower());
        QByteArray canonicalUrlHost(QUrl::toAce(url.host()).toLower());

        if (canonicalUrlHost.isEmpty())
        {
            // Not a valid hostname
            return false;
        }

        if (canonicalOriginHost == canonicalUrlHost)
        {
            // Exact host match
            return true;
        }

        if (scope() == SecurityOriginAndSubdomains)
        {
            return canonicalUrlHost.endsWith("." + canonicalOriginHost);
        }
        else
        {
            // We required a direct match
            return false;
        }
    }
}
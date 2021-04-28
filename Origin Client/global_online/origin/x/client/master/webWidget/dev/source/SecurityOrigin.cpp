#include "SecurityOrigin.h"

namespace WebWidget
{
    SecurityOrigin::SecurityOrigin(const QString &scheme, const QString &host, int port) :
        mHost(host.toLower()),
        mPort(port),
        mScheme(scheme.toLower())
    {
    }
        
    SecurityOrigin::SecurityOrigin(const QUrl &url) :
        mHost(url.host()),
        mPort(url.port(defaultPortForScheme(url.scheme()))),
        mScheme(url.scheme())
    {
    }

    SecurityOrigin::SecurityOrigin() :
        mPort(UnknownPort)
    {
    }

    bool SecurityOrigin::operator==(const SecurityOrigin &other) const
    {
        return scheme() == other.scheme() &&
            host() == other.host() &&
            port() == other.port();
    }

    int SecurityOrigin::defaultPortForScheme(const QString &scheme)
    {
        if (scheme.toLower() == "http")
        {
            return 80;
        }
        else if (scheme.toLower() == "https")
        {
            return 443;
        }

        return UnknownPort;
    }        
}
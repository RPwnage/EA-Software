#include <QUrl>

#include "JabberID.h"

namespace Origin
{
namespace Chat
{
    JabberID::JabberID(const QString &node, const QString &domain, const QString &resource) :
        mNode(node), mDomain(domain), mResource(resource)
    {
    }

    JabberID::JabberID() 
    {
    }

    bool JabberID::isValid() const
    {
        return !mNode.isEmpty() && !mDomain.isEmpty();
    }
    
    bool JabberID::operator==(const JabberID &other) const
    {
        return (node() == other.node()) &&
            (domain() == other.domain()) &&
            (resource() == other.resource());
    }

    // Takes a RFC 6122 formatted string
    JabberID JabberID::fromEncoded(const QByteArray &encoded)
    {
        QString node;
        QString domain;
        QString resource;

        QList<QByteArray> jidParts = encoded.split('@');

        if (jidParts.count() != 2)
        {
            return JabberID();
        }

        node = QUrl::fromPercentEncoding(jidParts[0]);

        QByteArray domainAndResource = jidParts[1];

        // See if we have a resource separator
        int resourceSepPos = domainAndResource.indexOf('/');

        if (resourceSepPos == -1)
        {
            // The whole thing is the domain
            domain = QUrl::fromPercentEncoding(domainAndResource);
        }
        else
        {
            domain = QUrl::fromPercentEncoding(domainAndResource.left(resourceSepPos));
            resource = QUrl::fromPercentEncoding(domainAndResource.mid(resourceSepPos + 1));
        }

        return JabberID(node, domain, resource);
    }

    // Produces a 6122 formatted string
    QByteArray JabberID::toEncoded() const
    {
        if (!isValid())
        {
            return QByteArray();
        }

        // Use QUrl to do the conversion
        QByteArray ret = QUrl::toPercentEncoding(node()) + "@" + QUrl::toPercentEncoding(domain());

        if (!resource().isEmpty())
        {
            ret += "/" + QUrl::toPercentEncoding(resource());
        }

        return ret;
    }

    ///
    /// Returns a bare JabberID (ie without a resource) based on this one
    ///
    JabberID JabberID::asBare() const
    {
        if (!isValid())
        {
            return JabberID();
        }

        // Build ourselves without our resource
        return JabberID(node(), domain());
    }

    JabberID JabberID::withResource(const QString &resource) const
    {
        if (!isValid())
        {
            return JabberID();
        }

        return JabberID(node(), domain(), resource);
    }

    JabberID JabberID::fromJingle(const buzz::Jid& impl)
    {
        // We have to do URL decoding for our impl
        QString node;
        QString domain;
        QString resource;

        node = QUrl::fromPercentEncoding(impl.node().c_str());
        domain = QUrl::fromPercentEncoding(impl.domain().c_str());
        resource = QUrl::fromPercentEncoding(impl.resource().c_str());

        return JabberID(node, domain, resource);
    }

    JabberID JabberID::fromJingle(const Jid& impl)
    {
        // We have to do URL decoding for our impl
        QString node;
        QString domain;
        QString resource;

        node = QUrl::fromPercentEncoding(impl.node.c_str());
        domain = QUrl::fromPercentEncoding(impl.domain.c_str());
        resource = QUrl::fromPercentEncoding(impl.resource.c_str());

        return JabberID(node, domain, resource);
    }

    buzz::Jid JabberID::toJingle() const
    {
        QByteArray encodedNode = QUrl::toPercentEncoding(node());
        QByteArray encodedDomain = QUrl::toPercentEncoding(domain());
        QByteArray encodedResource = QUrl::toPercentEncoding(resource());
        
        return buzz::Jid(std::string(encodedNode), std::string(encodedDomain), std::string(encodedResource));
    }
}
}

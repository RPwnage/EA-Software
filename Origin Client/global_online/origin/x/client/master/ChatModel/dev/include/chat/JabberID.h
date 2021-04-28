#ifndef _CHATMODEL_JABBERID_H
#define _CHATMODEL_JABBERID_H

#include <QString>
#include <QByteArray>
#include <QMetaType>

#include "qdebug.h"

// GCC 4.2 wants this forward declared before QHash because of bug #29131
namespace Origin
{
namespace Chat
{
    class JabberID;
    inline uint qHash(const JabberID &jabberId);
}
}

#include <QHash>

#include "XMPPImplTypes.h"
#include "talk/xmpp/jid.h"

namespace Origin
{
namespace Chat
{
    ///
    /// Represents an Jabber ID 
    /// 
    /// Jabber IDs are the core of XMPP user identification and addressing. They can be thought of as the XMPP analog
    /// of an email address.
    ///
    /// Defined in RFC 6122
    ///
    class JabberID
    {
    public:
        ///
        /// Creates a Jabber ID from components
        ///
        /// \param node      Node of the Jabber ID. This is the part before the @
        /// \param domain    Domain of the Jabber ID
        /// \param resource  Resource of the Jabber ID. If this is a null QString a bare Jabber ID is created.
        ///
        JabberID(const QString &node, const QString &domain, const QString &resource = QString());

        ///
        /// Creates an invalid JabberID
        ///
        JabberID();

        ///
        /// Returns the node of the Jabber ID
        ///
        QString node() const
        {
            return mNode;
        }

        ///
        /// Returns the domain of the Jabber ID
        ///
        QString domain() const
        {
            return mDomain;
        }

        ///
        /// Returns the resource of the Jabber ID
        ///
        /// This is a null QString for bare Jabber IDs
        ///
        /// \sa isBare()
        /// \sa asBare()
        ///
        QString resource() const
        {
            return mResource;
        }

        ///
        /// Returns if this represents a valid Jabber ID
        ///
        bool isValid() const;

        ///
        /// Creates a JabberID instance from an RFC 6122 formatted string
        ///
        static JabberID fromEncoded(const QByteArray &);

        /// 
        /// Produces a 6122 formatted string representing this instance
        ///
        /// \return Encoded string for valid Jabber IDs; null QByteArray for invalid Jabber IDs
        ///
        QByteArray toEncoded() const;

        ///
        /// Returns a bare JabberID (ie without a resource) based on this one
        ///
        JabberID asBare() const;

        ///
        /// Returns a JabberID based on this one with the supplied resource
        ///
        JabberID withResource(const QString &resource) const;
    
        ///
        /// Returns if this is a bare Jabber ID
        ///
        bool isBare() const
        {
            return mResource.isEmpty();
        }

        ///
        /// Compares Jabber IDs for equality
        ///
        /// Note this must be an exact match; if you want to only compare the domain and node use asBare() first
        ///
        bool operator==(const JabberID &) const;

        ///
        /// Compares Jabber IDs for inequality
        ///
        bool operator!=(const JabberID &other) const
        {
            return !(*this == other);
        }

        ///
        /// Creates a JabberID instance from an implementation Jabber ID
        ///
        /// This is for internal use only
        ///
        static JabberID fromJingle(const buzz::Jid& impl);

        ///
        /// Creates a JabberID instance from a proxied Jabber ID
        ///
        /// This is for internal use only
        ///
        static JabberID fromJingle(const Jid& impl);

        ///
        /// Returns an implementation Jabber ID based on this instance
        ///
        /// This is for internal use only
        ///
        buzz::Jid toJingle() const;

    private:
        bool mValid;

        QString mNode;
        QString mDomain;
        QString mResource;
    };

    inline uint qHash(const Origin::Chat::JabberID &jabberId)
    {
        return qHash(jabberId.node()) ^ 
            qHash(jabberId.domain()) ^ 
            qHash(jabberId.resource());
    }
}
}
    
Q_DECLARE_METATYPE(Origin::Chat::JabberID);

#endif

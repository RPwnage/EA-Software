#ifndef _CHATMODEL_ORIGINCONNECTION_H
#define _CHATMODEL_ORIGINCONNECTION_H

#include <QMutex>
#include <QUrl>

#include "services/session/SessionService.h"

#include "Connection.h"
#include "Configuration.h"
#include "NucleusID.h"

class QNetworkAccessManager;

namespace Origin
{
namespace Chat
{
    class XMPPUser;

    ///
    /// Represents a connection to an XMPP server with Origin-specific extensions
    ///
    class OriginConnection : public Connection
    {
        Q_OBJECT
    public:
        ///
        /// Creates a new OriginConnection instance
        ///
        /// \param session  Session to use 
        /// \param host     Hostname of the XMPP server
        /// \param port     Port of the XMPP server
        /// \param config   Chat::Configuration to use
        ///
        OriginConnection(Services::Session::SessionRef session, const QByteArray &host, int port = 5222, const Configuration &config = Configuration());
        virtual ~OriginConnection();

        ///
        /// Logs using the Origin session we were passed at construction
        ///
        /// \param uniqueResource =true means create a unique jabber resource (essentially disables single login conflict check)
        ///
        void login(bool uniqueResource);

        ///
        /// Returns our current Origin session
        ///
        Services::Session::SessionRef session() const;

        ///
        /// Returns the Nucleus ID for a given Jabber ID on the current connection
        ///
        /// \return Nucleus ID or Chat::InvalidNucleusID on error
        ///
        NucleusID jabberIdToNucleusId(const JabberID &jid) const;
        
        ///
        /// Returns the Nucleus ID for a given XMPPUser on the current connection
        ///
        /// \return Nucleus ID or Chat::InvalidNucleusID on error
        ///
        NucleusID nucleusIdForUser(const XMPPUser *) const;

        ///
        /// Returns the Jabber ID on the current connection for a given Nucleus ID
        /// 
        JabberID nucleusIdToJabberId(NucleusID nucleusId) const;

        ///
        /// Returns the Jabber ID for the given domain and Nucleus ID
        ///
        /// \return Nucleus ID or Chat::InvalidNucleusID on error
        ///
        static JabberID nucleusIdToJabberId(NucleusID nucleusId, const QByteArray &domain);
        
    private:
        Services::Session::SessionRef mSession;

        QByteArray mDomain;
    };
}
}

#endif

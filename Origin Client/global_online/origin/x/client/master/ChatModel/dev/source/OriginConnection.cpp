#include <QMutexLocker>

#include "OriginConnection.h"
#include "XMPPUser.h"

#include "services/session/SessionService.h"
#include "ChatGroup.h"
#include "ConnectedUser.h"

namespace Origin
{
namespace Chat
{
    using namespace Services::Session;

    OriginConnection::OriginConnection(SessionRef session, const QByteArray &domain, int port, const Configuration &config) :
        Connection(domain, port, config),
        mSession(session),
        mDomain(domain)
    {
    }
    
    OriginConnection::~OriginConnection()
    {
    }

    void OriginConnection::login(bool uniqueResource)
    {
        // Convert our session in to what XMPP needs
        const NucleusID nucleusId(SessionService::nucleusUser(mSession).toULongLong());

        QString resourceStr = "origin";
        if (uniqueResource)
        {
            //for single login conflict, we use random # to create unique resource but we also want to
            //have resourceStr include 'origin' so that web side can distinguish that it's a C++ client running
            int rand_number = qrand();
            resourceStr += QString ("%1").arg(rand_number);
        }

        const JabberID jid = nucleusIdToJabberId(nucleusId).withResource(resourceStr);
        Connection::login(jid);

        currentUser()->chatGroups()->initializeGroups();
    }

    SessionRef OriginConnection::session() const
    {
        return mSession;
    }

    NucleusID OriginConnection::jabberIdToNucleusId(const JabberID &jid) const
    {
        if (jid.domain() != mDomain)
        {
            return InvalidNucleusID;
        }

        return jid.node().toULongLong();
    }
       
    NucleusID OriginConnection::nucleusIdForUser(const XMPPUser *user) const
    {
        return jabberIdToNucleusId(user->jabberId());
    }

    JabberID OriginConnection::nucleusIdToJabberId(NucleusID nucleusId) const
    {
        return nucleusIdToJabberId(nucleusId, mDomain);
    }

    JabberID OriginConnection::nucleusIdToJabberId(NucleusID nucleusId, const QByteArray &domain)
    {
        return JabberID(QString::number(nucleusId), domain);
    }
}
}

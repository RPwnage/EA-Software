#ifndef _CHATMODEL_BLOCKLIST_H
#define _CHATMODEL_BLOCKLIST_H

// We need to include this before QSet to work around GCC bug #29131
#include "JabberID.h"

#include <QObject>
#include <QSet>
#include <QMutex>

#include "XMPPImplTypes.h"

namespace Origin
{
namespace Chat
{
    class Connection;

    ///
    /// Represent a list of blocked users
    ///
    /// This is internally implemented as an XMPP privacy list
    ///
    class BlockList : public QObject
    {
        Q_OBJECT

        friend class ConnectedUser;

    public:
        ///
        /// Returns if the block list has loaded
        ///
        /// If this returns false then the loaded signal will be emitted once the list has loaded
        ///
        bool hasLoaded() const;

        ///
        /// Returns the set of Jabber IDs on the block list
        ///
        QSet<JabberID> jabberIDs() const;

        /// 
        /// Returns the set of user ids in a form suitable for Sonar
        /// 
        std::vector<std::string> blockedIds() const;

        ///
        /// Replaces the block list with a set of Jabber IDs
        ///
        void setJabberIDs(const QSet<JabberID> &);

        ///
        /// Adds a Jabber ID to the block list
        ///
        /// updated() will be emitted once the server accepts the new block list
        ///
        /// \return True on success; false if block list hasn't loaded
        ///
        bool addJabberID(const JabberID &);

        ///
        /// Removes a Jabber ID from the block list
        ///
        /// updated() will be emitted once the server accepts the new block list
        ///
        /// \return True on success; false if block list hasn't loaded
        ///
        bool removeJabberID(const JabberID &);

        ///
        /// Empties the block list
        ///
        /// updated() will be emitted once the server accepts the new block list
        ///
        void clear();

        ///
        /// Re-queries the block list
        ///
        /// updated() will be emitted once the client has reloaded the block list
        ///
        void forceRefresh();
    signals:
        ///
        /// Emitted once the block list has loaded
        ///
        void loaded();

        ///
        /// Emitted on any block list update
        ///
        /// For client initiated changes this is only emitted once the server accepts the new block list
        ///
        void updated();

    private: // Friend functions
        ///
        /// Creates a new BlockList instance for the passed connection
        ///
        BlockList(Connection *);

    private slots:        
        ///
        /// Refreshes the block list from the copy on the server
        ///
        void refresh();

        void jingleBlockListUpdate(const QList<buzz::Jid>& jidList);

    private:

        // Pushes the block list to Jingle
        void setJingleJabberIDs(const QSet<JabberID> &) const;

        // Protects everything
        mutable QMutex mMutex;

        QSet<JabberID> mBlockList;

        Connection *mConnection;
        bool mLoaded;
    };
}
}

#endif

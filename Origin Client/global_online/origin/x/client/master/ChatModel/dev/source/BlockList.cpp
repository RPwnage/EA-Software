#include <QMutex>

#include "services/debug/DebugService.h"
#include "engine/login/LoginController.h"
#include "engine/login/User.h"
#include "engine/voice/VoiceController.h"

#include "BlockList.h"
#include "Connection.h"
#include "XMPPImplEventAdapter.h"

namespace Origin
{
namespace Chat
{
    BlockList::BlockList(Connection *connection) : 
        mBlockList(),
        mConnection(connection), 
        mLoaded(false)
    {
        // We need a direct connection because the list will otherwise contain bad references to JabberIDs
        ORIGIN_VERIFY_CONNECT_EX(connection->implEventAdapter(), SIGNAL(blockListUpdate(const QList<buzz::Jid>&)),
            this, SLOT(jingleBlockListUpdate(const QList<buzz::Jid>&)), Qt::DirectConnection);

        // Refresh our block list once we log in
        ORIGIN_VERIFY_CONNECT(connection, SIGNAL(connected()), this, SLOT(refresh()));
    }

    bool BlockList::hasLoaded() const
    {
        QMutexLocker locker(&mMutex);
        return mLoaded;
    }

    QSet<JabberID> BlockList::jabberIDs() const
    {
        QMutexLocker locker(&mMutex);
        return mBlockList;
    }

    std::vector<std::string> BlockList::blockedIds() const
    {
        QMutexLocker locker(&mMutex);
        std::vector<std::string> blockList;
        for(auto i = mBlockList.constBegin(), last = mBlockList.constEnd(); i != last; ++i)
        {
            blockList.push_back(i->node().toStdString());
        }
        return blockList;
    }

    void BlockList::setJabberIDs(const QSet<JabberID> &jids)
    {
        QMutexLocker locker(&mMutex);

        mBlockList = jids;
        setJingleJabberIDs(mBlockList);
    }

    bool BlockList::addJabberID(const JabberID &jid)
    {
        QMutexLocker locker(&mMutex);

        if (!mLoaded)
        {
            return false;
        }

        if (mBlockList.contains(jid))
        {
            // Nothing to do
            return true;
        }

        mBlockList += jid;
        setJingleJabberIDs(mBlockList);

        return true;
    }

    bool BlockList::removeJabberID(const JabberID &jid)
    {
        QMutexLocker locker(&mMutex);

        if (!mLoaded)
        {
            return false;
        }

        if (!mBlockList.contains(jid))
        {
            // Nothing to do
            return true;
        }

        mBlockList -= jid;
        setJingleJabberIDs(mBlockList);

        return true;
    }

    void BlockList::clear()
    {
        QMutexLocker locker(&mMutex);
        
        if (mLoaded && mBlockList.isEmpty())
        {
            // Nothing to do
            return;
        }

        mBlockList.clear();
        setJingleJabberIDs(mBlockList);
    }

    void BlockList::jingleBlockListUpdate(const QList<buzz::Jid>& jidList)
    {
        bool emitLoadedSignal;

        {
            QMutexLocker locker(&mMutex);

            emitLoadedSignal = !mLoaded;

            mBlockList.clear();
            for (QList<buzz::Jid>::const_iterator it = jidList.constBegin();
                it != jidList.constEnd();
                it++)
            {
                mBlockList << JabberID::fromJingle(*it);
            }

            // We've loaded
            mLoaded = true;
        }

        if (emitLoadedSignal)
        {
            // Emit this outside the lock
            emit loaded();
        }

        emit (updated());
    }

    void BlockList::setJingleJabberIDs(const QSet<JabberID> &jabberIDs) const
    {
        QList<buzz::Jid> blockList;

        for(QSet<JabberID>::ConstIterator it = jabberIDs.constBegin();
            it != jabberIDs.constEnd();
            it++)
        {
            blockList.push_back(it->toJingle());
        }

        if (mConnection && mConnection->established() && mConnection->jingle())
        {
            mConnection->jingle()->SetBlockList(&blockList);
        }
    }

    void BlockList::forceRefresh()
    { 
        this->refresh(); 
    }

    void BlockList::refresh()
    {
        if (mConnection && mConnection->established() && mConnection->jingle())
        {
            mConnection->jingle()->RequestBlockList();
        }
    }
}
}

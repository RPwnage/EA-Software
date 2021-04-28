//    Copyright (c) 2013, Electronic Arts
//    All rights reserved.
#ifndef _COMMONTITLESCONTROLLER_H
#define _COMMONTITLESCONTROLLER_H

#include <engine/login/User.h>
#include <engine/content/Entitlement.h>
#include <services/plugin/PluginAPI.h>
#include <QSet>

namespace Origin
{

namespace Chat
{
    class Roster;
}

namespace Engine
{
namespace Social
{

class CommonTitlesCache;
class SocialController;

///
/// \brief Determines which titles a user has in common with another user
///
/// This class is not thread-safe. It must be called from the main thread.
///
class ORIGIN_PLUGIN_API CommonTitlesController : public QObject
{
    Q_OBJECT
    friend class SocialController;
public:

    virtual ~CommonTitlesController();

    /// \brief Returns if the given friend (by Nucleus ID) owns a similar entitlement
    bool nucleusIdHasSimilarEntitlement(quint64 nucleusId, Engine::Content::EntitlementRef ent) const;

    /// \brief Refreshes our state from the server
    void refresh();

signals:
    /// \brief Emitted whenever nucleusIdHasMasterTitleId() can return a different value than it previously did for 
    ///        the same arguments
    void changed();

private slots:
    void rosterLoaded();
    void queueCacheConverageCheck();

    void nonOriginAppIdQueryFinished();
    void originTitleQueryFinished();
    void onUserContentUpdated(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>);

protected:
    CommonTitlesController(UserRef user, Chat::Roster *roster);
   
private:
    QSet<quint64> friendNucleusIds() const;
    
    Q_INVOKABLE void checkCacheCoverage(bool forceReload = false);
    void requestFinished();

    void queryNonOriginAppIdForCommonNucleusIds(UserRef user, const QString &);
    void queryNucleusIdForOwnedOriginTitles(UserRef user, quint64 nucleusId);

    UserWRef mUser;
    Chat::Roster *mRoster;
    CommonTitlesCache *mCache;
    bool mCacheCoverageCheckQueued;

    QSet<QString> mInFlightNonOriginAppIdRequests;
    QSet<quint64> mInFlightOriginTitleRequests;

    bool mChangedSignalPending;
};

}
}
}

#endif

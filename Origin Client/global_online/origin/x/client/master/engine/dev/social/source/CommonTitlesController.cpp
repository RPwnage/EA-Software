#include "engine/social/CommonTitlesController.h"

#include <QDir>

#ifdef _WIN32
#include <Windows.h>
#include <ShlObj.h>
#endif

#include "CommonTitlesCache.h"
#include <chat/Roster.h>
#include <chat/RemoteUser.h>
#include <engine/content/ContentController.h>
#include <services/debug/DebugService.h>
#include <services/platform/PlatformService.h>
#include <services/rest/GamesServiceClient.h>
#include <services/rest/NonOriginGameServiceClient.h>
#include <services/connection/ConnectionStatesService.h>

namespace Origin
{
namespace Engine
{
namespace Social
{

namespace
{
    // How many milliseconds we delay checking our cache coverage and sending requests waiting for more changes.
    // to come in
    const unsigned int CoverageCheckDelayMs = 1000;
    
    QString roamingDataDirectory()
    {
#ifdef _WIN32
        // Get the path to roaming app data
        wchar_t appDataPath[MAX_PATH + 1];
        if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath) != S_OK)
        {
            ORIGIN_ASSERT(0);
            return QString();
        }
        
        return QDir::fromNativeSeparators(QString::fromUtf16(appDataPath)) + "/Origin";
#else
        return Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DataLocation);
#endif
    }
}
    
CommonTitlesController::CommonTitlesController(UserRef user, Chat::Roster *roster) :
    mUser(user),
    mRoster(roster),
    mCache(new CommonTitlesCache(roamingDataDirectory() + "/CommonTitles/", user->userId())),
    mCacheCoverageCheckQueued(false),
    mChangedSignalPending(false)
{
    // Check when the user has added a game in case it's a non-Origin game
    ORIGIN_VERIFY_CONNECT(
            user->contentControllerInstance(), SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)),
            this, SLOT(onUserContentUpdated(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)));

    // Check when we go online in case we missed a check when we were offline
    ORIGIN_VERIFY_CONNECT(
            Services::Connection::ConnectionStatesService::instance(), SIGNAL(nowOnlineUser()),
            this, SLOT(queueCacheConverageCheck()));

    // Check once the roster loads
    ORIGIN_VERIFY_CONNECT(roster, SIGNAL(loaded()), this, SLOT(rosterLoaded()));
}

CommonTitlesController::~CommonTitlesController()
{
    delete mCache;
}

void CommonTitlesController::rosterLoaded()
{
    // Check for new contacts now
    ORIGIN_VERIFY_CONNECT(
            mRoster, SIGNAL(contactAdded(Origin::Chat::RemoteUser *)),
            this, SLOT(queueCacheConverageCheck()));

    queueCacheConverageCheck();
}

void CommonTitlesController::refresh()
{
    // Forcibly reload our state from the server
    checkCacheCoverage(true);
}

bool CommonTitlesController::nucleusIdHasSimilarEntitlement(quint64 nucleusId, Engine::Content::EntitlementRef entRef) const
{
    if (entRef->contentConfiguration()->isNonOriginGame())
    {
		const QString productId = entRef->contentConfiguration()->productId();
        return mCache->nonOriginGameEntryForAppId(productId).nucleusIdHasInCommon(nucleusId);
    }
    else
    {
        Services::CommonGame game;

        game.masterTitleId = entRef->contentConfiguration()->masterTitleId();

        if(entRef->contentConfiguration()->platformEnabled(Services::PlatformService::PlatformThis))
        {
            game.multiPlayerId = entRef->contentConfiguration()->multiplayerId(Services::PlatformService::PlatformThis);
        }
        else
        {
            game.multiPlayerId = entRef->contentConfiguration()->multiplayerId(Services::PlatformService::PlatformOther);
        }

        return mCache->originTitleEntryForNucleusId(nucleusId).ownsCompatibleGame(game);
    }
}

void CommonTitlesController::onUserContentUpdated(const QList<Origin::Engine::Content::EntitlementRef> newContent, const QList<Origin::Engine::Content::EntitlementRef> removedContent)
{
    // Check to see if any newly added content is a base game and queue a check if so
    foreach(const Origin::Engine::Content::EntitlementRef& ent, newContent)
    {
        if(!ent.isNull() && ent->contentConfiguration()->isBaseGame())
        {
            queueCacheConverageCheck();
            return;
        }
    }
}

void CommonTitlesController::queueCacheConverageCheck()
{
    // Defer checking cache converage in case we're getting a lot of updates at once
    // This is especially true of social which will feed us a single Nucleus ID at a time
    if (!mCacheCoverageCheckQueued)
    {
        mCacheCoverageCheckQueued = true;
        
        // Wait half a second for more updates to come in - we're in no rush
        QTimer::singleShot(CoverageCheckDelayMs, this, SLOT(checkCacheCoverage()));
    }
}
    
QSet<quint64> CommonTitlesController::friendNucleusIds() const
{
    QSet<quint64> ret;
    QSet<Chat::RemoteUser *> contacts = mRoster->contacts();
    
    for(QSet<Chat::RemoteUser *>::ConstIterator contactIt = contacts.constBegin();
        contactIt != contacts.constEnd();
        contactIt++)
    {
        Chat::RemoteUser *contact = *contactIt;
        
        // We're only friends if we have a mutual subscription
        if (contact->subscriptionState().direction() == Chat::SubscriptionState::DirectionBoth)
        {
            const Chat::NucleusID nucleusId = contact->nucleusId();

            if (nucleusId != Chat::InvalidNucleusID)
            {
                ret << contact->nucleusId();
            }
        }
    }

    return ret;
}
    
void CommonTitlesController::checkCacheCoverage(bool forceReload)
{
    UserRef user = mUser.toStrongRef();

    mCacheCoverageCheckQueued = false;

    if (user.isNull())
    {
        return;
    }
    
    if (!Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()))
    {
        // We can't issue new network requests so there's no point checking coverage
        return;
    }

    if (!mRoster->hasLoaded())
    {
        // Nothing to do
        return;
    }

    QList<Content::EntitlementRef> entitlements = user->contentControllerInstance()->entitlements();

    if (entitlements.isEmpty())
    {
        // Nothing to do
        return;
    }

    QSet<quint64> friendNucleusIdSet = friendNucleusIds();
    
    for(QSet<quint64>::ConstIterator it = friendNucleusIdSet.constBegin();
        it != friendNucleusIdSet.constEnd();
        it++)
    {
        const quint64 friendNucleusId = *it;
        const OriginTitleCacheEntry &cacheEntry = mCache->originTitleEntryForNucleusId(friendNucleusId);

        if (forceReload || !cacheEntry.isFresh())
        {
            queryNucleusIdForOwnedOriginTitles(user, friendNucleusId);
        }

        for(QList<Content::EntitlementRef>::ConstIterator entIt = entitlements.constBegin();
            entIt != entitlements.constEnd();
            entIt++)
        {
            const Content::EntitlementRef &ent = *entIt;

            // We have to treat non-Origin and Origin games completely differently
            // This is because we query non-Origin games by their app ID and get a list of friends while we query Origin
            // games by their friend ID and get a list of master title IDs
            if (ent->contentConfiguration()->isNonOriginGame())
            {
                const QString appId(ent->contentConfiguration()->productId());
                const NonOriginGameCacheEntry &cacheEntry = mCache->nonOriginGameEntryForAppId(appId);

                if (forceReload || !cacheEntry.isFresh())
                {
                    // No cache entry at all
                    queryNonOriginAppIdForCommonNucleusIds(user, appId);
                }
                else
                {
                    if (!cacheEntry.coversNucleusId(friendNucleusId))
                    {
                        // Our cache doesn't cover this Nucleus ID
                        queryNonOriginAppIdForCommonNucleusIds(user, appId);
                        // We can break because that request will cover all friends owning that game
                        break;
                    }
                }
            }
        }
    }
}
    
void CommonTitlesController::queryNonOriginAppIdForCommonNucleusIds(UserRef user, const QString &appId)
{
    using namespace Services;

    if (mInFlightNonOriginAppIdRequests.contains(appId))
    {
        return;
    }

    NonOriginGameFriendResponse *resp = NonOriginGameServiceClient::friendsWithSameContent(user->getSession(), appId); 
    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(nonOriginAppIdQueryFinished()));

    mInFlightNonOriginAppIdRequests << appId;
}

void CommonTitlesController::nonOriginAppIdQueryFinished()
{
    using namespace Services;

    NonOriginGameFriendResponse *resp = dynamic_cast<NonOriginGameFriendResponse*>(sender());

    if (!resp)
    {
        return;
    }
    
    // Housekeeping
    resp->deleteLater();
    mInFlightNonOriginAppIdRequests.remove(resp->appId());

    if (resp->error() != Services::restErrorSuccess)
    {
        requestFinished();
        return;
    }

    // There's a race here if we added a friend while this query was in flight
    // Presumably we'll add another friend or the cache entry will expire before anyone notices
    mCache->setNonOriginGameEntryForAppId(resp->appId(), friendNucleusIds(), resp->friendIds().toSet());

    // Coverage changes aren't externally visible
    // Only notify if we had friends with this title in common
    // This assumes we'll only gain games in common
    if (!mChangedSignalPending)
    {
        mChangedSignalPending = !resp->friendIds().isEmpty();
    }

    requestFinished();
}

void CommonTitlesController::queryNucleusIdForOwnedOriginTitles(UserRef user, quint64 nucleusId)
{
    using namespace Services;

    QList<quint64> nucleusIdList;

    if (mInFlightOriginTitleRequests.contains(nucleusId))
    {
        return;
    }

    // XXX: Should we be batching this?
    nucleusIdList << nucleusId;

    GamesServiceFriendResponse *resp = GamesServiceClient::instance()->commonGames(user->getSession(), &nucleusIdList);
    resp->setProperty("_origin_nucleus_id", nucleusId);

    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(originTitleQueryFinished()));

    mInFlightOriginTitleRequests << nucleusId;
}

void CommonTitlesController::originTitleQueryFinished()
{
    using namespace Services;

    GamesServiceFriendResponse *resp = dynamic_cast<GamesServiceFriendResponse*>(sender());
    UserRef user = mUser.toStrongRef();

    if (!resp || user.isNull())
    {
        return;
    }

    const quint64 nucleusId = resp->property("_origin_nucleus_id").toULongLong();

    // Housekeeping
    resp->deleteLater();
    mInFlightOriginTitleRequests.remove(nucleusId);

    if (resp->error() != Services::restErrorSuccess)
    {
        requestFinished();
        return;
    }

    const FriendsCommonGames friendsCommonGames = resp->getFriendGames();
    bool foundCommonGames = false;
    QList<CommonGame> commonGames;

    // The loginreg version of this call will return the data for all friends (!)
    // We need to filter out the other friends
    for(QList<FriendCommonGames>::ConstIterator it = friendsCommonGames.friends.constBegin();
        it != friendsCommonGames.friends.constEnd();
        it++)
    {
        if (it->userId == nucleusId)
        {
            commonGames = it->games;
            foundCommonGames = true;
            break;
        }
    }

    if (!foundCommonGames)
    {
        // Not expected
        requestFinished();
        return;
    }

    QSet<CommonGame> owned;

    // Despite the service being called "common" it's actually returning all owned games
    for(auto it = commonGames.constBegin(); it != commonGames.constEnd(); it++)
    {
        owned << *it;
    }

    mCache->setOriginTitleEntryForNucleusId(nucleusId, owned);

    if (!mChangedSignalPending)
    {
        // Coverage changes aren't externally visible
        // Only notify if the response had titles in common
        // This assumes we'll only gain titles in common
        mChangedSignalPending = !owned.isEmpty();
    }

    requestFinished();
}

void CommonTitlesController::requestFinished()
{
    if (mInFlightNonOriginAppIdRequests.isEmpty() && mInFlightOriginTitleRequests.isEmpty())
    {
        // That was our last request - we can do expensive things like writing to disk and notifying callers of 
        // changes now
        mCache->writeToDisk();

        if (mChangedSignalPending)
        {
            emit changed();
            mChangedSignalPending = false;
        }
    }

}

}
}
}

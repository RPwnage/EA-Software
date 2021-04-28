#include <QMutexLocker>

#include "engine/social/UserNamesCache.h"
#include "services/debug/DebugService.h"
#include "services/rest/AtomServiceClient.h"
#include "services/rest/AtomServiceResponses.h"
        
namespace Origin
{
namespace Engine
{
namespace Social
{
    UserNamesCache::UserNamesCache(UserRef user, QObject *parent) :
        QObject(parent),
        mUser(user),
        mRequestWantedUserInfoQueued(false)
    {
        // Store our own names
        mCachedUserNames[user->userId()] = UserNames(user->eaid(), user->firstName(), user->lastName());
    }

    void UserNamesCache::subscribe(quint64 nucleusId, bool wantRealName)
    {
        QMutexLocker locker(&mStateLock);

        if (mWantedUserInfo.contains(nucleusId) || (nucleusId == 0))
        {
            // Request already in progress or bogus ID
            return;
        }
        
        if (!wantRealName)
        {
            UserNames currentNames = mCachedUserNames.value(nucleusId);

            if (!currentNames.isNull() && (currentNames.realNameLoaded()))
            {
                // Have all the information we need
                return;
            }
        }
        
        // We want this user's info
        mWantedUserInfo << nucleusId;
        requestWantedUserInfoAsync();
    }

    UserNames UserNamesCache::namesForNucleusId(quint64 nucleusId) const
    {
        QMutexLocker locker(&mStateLock);
        return mCachedUserNames[nucleusId];
    }
        
    UserNames UserNamesCache::namesForCurrentUser() const
    {
        UserRef user = mUser.toStrongRef();

        if (!user)
        {
            return UserNames();
        }

        return namesForNucleusId(user->userId());
    }

    // Must be called with mStateLock
    void UserNamesCache::requestWantedUserInfoAsync()
    {
        if (!mRequestWantedUserInfoQueued)
        {
            mRequestWantedUserInfoQueued = true;
            QMetaObject::invokeMethod(this, "requestWantedUserInfo", Qt::QueuedConnection);
        }
    }

    void UserNamesCache::requestWantedUserInfo()
    {
        using namespace Services;

        QMutexLocker locker(&mStateLock);
        mRequestWantedUserInfoQueued = false;

        UserRef user = mUser.toStrongRef();
        
        if (!user)
        {
            return;
        }

        // Request the profiles
        // This will take a number of real name requests and turn them to a hopefully smaller number of HTTP requests
        const QList<UserResponse*> resps = AtomServiceClient::user(Session::SessionService::currentSession(), mWantedUserInfo.toList());

        // Hook up the responses
        for(QList<UserResponse*>::ConstIterator it = resps.constBegin();
            it != resps.constEnd();
            it++)
        {
            ORIGIN_VERIFY_CONNECT(*it, SIGNAL(finished()), *it, SLOT(deleteLater()));
            ORIGIN_VERIFY_CONNECT(*it, SIGNAL(success()), this, SLOT(onAtomUserSuccess()));
        }
    }
        
    void UserNamesCache::setOriginIdForNucleusId(quint64 nucleusId, const QString &originId, bool overwrite)
    {
        UserNames oldNames;
        UserNames newNames;

        {
            QMutexLocker locker(&mStateLock);
            
            oldNames = mCachedUserNames.value(nucleusId);

            if (!oldNames.isNull() && !overwrite)
            {
                // Don't overwrite this name
                return;
            }

            if (oldNames.isNull() || !oldNames.realNameLoaded())
            {
                // Just overwrite the existing entry
                newNames = UserNames(originId);
            }
            else
            {
                // Keep the real name
                newNames = UserNames(originId, oldNames.firstName(), oldNames.lastName());
            }

            mCachedUserNames.insert(nucleusId, newNames);
        }

        if (oldNames != newNames)
        {
            emit namesChanged(nucleusId, newNames);
        }
    }
        
    void UserNamesCache::setNamesForNucleusId(quint64 nucleusId, const UserNames &newNames)
    {
        UserNames oldNames;

        {
            QMutexLocker locker(&mStateLock);
            
            oldNames = mCachedUserNames.value(nucleusId);
            mCachedUserNames.insert(nucleusId, newNames);
        }

        if (oldNames != newNames)
        {
            emit namesChanged(nucleusId, newNames);
        }
    }

    void UserNamesCache::onAtomUserSuccess()
    {
        Services::UserResponse *resp = dynamic_cast<Services::UserResponse*>(sender());

        if (!resp)
        {
            return;
        }

        const QList<Services::User> users(resp->users());

        for(QList<Services::User>::ConstIterator it = users.constBegin();
            it != users.constEnd();
            it++)
        {
            const Services::User &user = *it;

            UserNames oldNames;
            UserNames newNames;

            {
                QMutexLocker locker(&mStateLock);

                // We no longer want this even if they don't have a Nucleus ID
                mWantedUserInfo.remove(user.userId);

                oldNames = mCachedUserNames.value(user.userId);
                newNames = UserNames(user.originId, user.firstName, user.lastName);

                mCachedUserNames.insert(user.userId, newNames);
            }

            if (oldNames != newNames)
            {
                emit namesChanged(user.userId, newNames);
            }
        }
    }
}
}
}

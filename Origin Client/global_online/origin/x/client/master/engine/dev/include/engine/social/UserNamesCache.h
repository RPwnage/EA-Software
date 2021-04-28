#ifndef _USERNAMESCACHE_H
#define _USERNAMESCACHE_H

#include <QObject>
#include <QString>
#include <QSet>
#include <QHash>
#include <QMutex>

#include <engine/login/User.h>
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace Social
{
    class SocialController;
    class UserNamesCache;

    /// \brief Value class for storing a user's Origin ID and real name
    class ORIGIN_PLUGIN_API UserNames
    {
        friend class UserNamesCache;
    public:
        /// \brief Constructs a null UserNames
        UserNames() : mRealNameLoaded(false)
        {
        }
        
        /// \brief Constructs a non-null UserNames with just an Origin ID
        explicit UserNames(const QString &originId) :
            mOriginId(originId),
            mRealNameLoaded(false)
        {
        }

        /// \brief Constructs a non-null UserNames with all fields
        UserNames(const QString &originId, const QString &firstName, const QString &lastName) :
            mOriginId(originId),
            mRealNameLoaded(true),
            mFirstName(firstName),
            mLastName(lastName)
        {
        }

        /// \brief Returns true if this is a null instance
        bool isNull() const { return mOriginId.isNull(); }

        /// \brief Returns the user's Origin ID
        QString originId() const { return mOriginId; }

        ///
        /// \brief Returns the user's first name
        ///
        /// This will be null if the real name isn't known or non-null and empty if it's known to be unset or hidden 
        ///
        QString firstName() const { return mFirstName; }

        ///
        /// \brief Returns the user's first name
        ///
        /// This will be null if the real name isn't known or non-null and empty if it's known to be unset or hidden 
        ///
        QString lastName() const { return mLastName; }

        bool operator==(const UserNames &other) const
        {
            return (mOriginId == other.mOriginId) &&
                (mRealNameLoaded == other.mRealNameLoaded) &&
                (mFirstName == other.mFirstName) &&
                (mLastName == other.mLastName);
        }

        bool operator!=(const UserNames &other) const
        {
            return !(*this == other);
        }

    private:
        bool realNameLoaded() const { return mRealNameLoaded; }

        QString mOriginId;

        bool mRealNameLoaded;
        QString mFirstName;
        QString mLastName;
    };

    /// \brief  Class for tracking and caching the identity of users based on Nucleus ID 
    class ORIGIN_PLUGIN_API UserNamesCache : public QObject
    {
        Q_OBJECT

        friend class SocialController;
    public:
        ///
        /// \brief Subscribes to a given Nucleus ID's Origin ID and optionally their real name
        ///
        /// \param nucleusId The NucleusID of the person to subscribe to.
        /// \param wantRealName  True if the contact's real name is wanted.
        ///
        void subscribe(quint64 nucleusId, bool wantRealName = false);

        ///
        /// \brief Returns the real name for a given user or a null UserNames if none is available
        ///
        UserNames namesForNucleusId(quint64 nucleusId) const;

        ///
        /// \brief Returns the real name for the user this cache was constructed with
        ///
        UserNames namesForCurrentUser() const;
    
        ///
        /// \brief Sets the Origin for a Nucleus ID
        ///
        /// If a real name already exists for the Nucleus ID it will be preserved
        ///
		/// \param  nucleusId  TBD.
        /// \param  originId   Origin ID to set.
        /// \param  overwrite  If true, an existing Origin ID will be overwritten. This should be set to false if the
        ///                    Origin ID is coming from an unreliable source.
        ///
        /// \sa setNamesForNucleusId()
        ///
        void setOriginIdForNucleusId(quint64 nucleusId, const QString &originId, bool overwrite = true);

        ///
        /// \brief Sets the names for a Nucleus ID
        ///
        /// If a name is already set for the Nucleus ID it will be replaced
        ///
        /// \sa setOriginIdForNucleusId()
        ///
        void setNamesForNucleusId(quint64 nucleusId, const UserNames &names);

    signals:
        void namesChanged(quint64 nucleusId, const Origin::Engine::Social::UserNames &);

    private slots:
        void onAtomUserSuccess();

    private:
        UserNamesCache(UserRef user, QObject *parent = NULL);

        void requestWantedUserInfoAsync();
        Q_INVOKABLE void requestWantedUserInfo();

        UserWRef mUser;

        // mStateLock protects everything below
        mutable QMutex mStateLock;

        bool mRequestWantedUserInfoQueued;
        QHash<quint64, UserNames> mCachedUserNames; 
        QSet<quint64> mWantedUserInfo;
    };
}
}
}

#endif


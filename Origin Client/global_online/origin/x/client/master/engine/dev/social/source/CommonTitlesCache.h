//    Copyright (c) 2013, Electronic Arts
//    All rights reserved.
#ifndef _COMMONTITLESCACHE_H
#define _COMMONTITLESCACHE_H

#include "services/rest/GamesServiceResponse.h"

#include <QString>
#include <QByteArray>
#include <QSet>
#include <QHash>

namespace Origin
{
namespace Engine
{
namespace Social
{

class CommonTitlesCache;

/// \brief Internal helper class for dealing with salted Nucleus IDs
class SaltedNucleusIdHashUser
{
public:
    SaltedNucleusIdHashUser(quint64 salt = 0);

    /// \brief Returns the salt currently in use
    quint64 salt() const { return mSalt; }

    /// \brief Sets a new salt value
    void setSalt(quint64 salt);
    
    /// \brief Returns a salted, hashed value for a Nucleus ID with our current salt
    QByteArray saltedNucleusIdHash(quint64 nucleusId) const;
    
    // \brief Returns a salted, hashed value for a Nucleus ID with the passed salt
    static QByteArray saltedNucleusIdHash(quint64 nucleusId, quint64 salt);

private:        
    quint64 mSalt;
    mutable QHash<quint64, QByteArray> mCache;
};

///
/// \brief Cache entry for Origin titles
///
/// These are indexed by nucleus IDs
///
class OriginTitleCacheEntry
{
    friend class CommonTitlesCache;
public:
    bool isFresh() const;

    bool ownsCompatibleGame(const Services::CommonGame &game) const;

private:
    OriginTitleCacheEntry() : mNull(true) {}
    OriginTitleCacheEntry(const QSet<Services::CommonGame> &owned, qint64 expiryEpochMsecs);

    qint64 expiryEpochMsecs() const { return mExpiryEpochMsecs; }

    bool mNull;
    QSet<Services::CommonGame> mOwnedGames;
    qint64 mExpiryEpochMsecs;
};

///
/// \brief Cache entry for non-Origin games
///
/// These are indexed by app IDs
///
class NonOriginGameCacheEntry : private SaltedNucleusIdHashUser
{
    friend class CommonTitlesCache;
public:
    bool isFresh() const;

    bool coversNucleusId(quint64 nucleusId) const; 
    bool nucleusIdHasInCommon(quint64 nucleusId) const; 

private:
    NonOriginGameCacheEntry() {}
    NonOriginGameCacheEntry(quint64 salt, const QSet<QByteArray> &covered, const QSet<QByteArray> &common, qint64 expiryEpochMsecs);
    
    qint64 expiryEpochMsecs() const { return mExpiryEpochMsecs; }

    QSet<QByteArray> mCoveredHashedNucleusIds;
    QSet<QByteArray> mCommonHashedNucleusIds; 
    qint64 mExpiryEpochMsecs;
};

/// \brief Persistent cache for common titles data
class CommonTitlesCache : private SaltedNucleusIdHashUser
{
public:
    /// \brief Creates a new CommonTitlesCache managing the cache at the given filesystem path
    CommonTitlesCache(const QString &cacheDirPath, quint64 nucleusId);

    ///
    /// \brief Returns the cache entry for Origin titles based on Nucleus ID 
    ///
    /// \param nucleusId  Nucleus ID to query
    ///
    const OriginTitleCacheEntry &originTitleEntryForNucleusId(quint64 nucleusId);
    
    ///
    /// \brief Updates the cache entry for Origin titles based on Nucleus ID
    ///
    /// \param nucleusId   Nucleus ID we're updating
    /// \param ownedGames  The games the Nucleus ID owns 
    ///
    void setOriginTitleEntryForNucleusId(quint64 nucleusId, const QSet<Services::CommonGame> &ownedGames);

    /// \brief Returns the cache entry for non-Origin games based on their app ID
    const NonOriginGameCacheEntry &nonOriginGameEntryForAppId(const QString &appId); 

    /// \brief Updates the cache entry for non-Origin games based on their app ID
    void setNonOriginGameEntryForAppId(const QString &appId, const QSet<quint64> &coveredNucleusIds, const QSet<quint64> &commonNucleusIds); 

    /// \brief Flushes our state to disk
    void writeToDisk();

private:
    bool readFromDisk();

    QString mCacheFilename;

    const OriginTitleCacheEntry mNullOriginTitleEntry;
    const NonOriginGameCacheEntry mNullNonOriginGameEntry;

    QHash<QByteArray, OriginTitleCacheEntry> mOriginTitleEntries; 
    QHash<QString, NonOriginGameCacheEntry> mNonOriginGameEntries;
};

}
}
}

#endif


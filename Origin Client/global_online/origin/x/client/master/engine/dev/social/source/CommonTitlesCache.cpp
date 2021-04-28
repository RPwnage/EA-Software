#include "CommonTitlesCache.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDataStream>
#include <QByteArray>
#include <QFile>
#include <QDir>

#include <stdlib.h>

namespace Origin
{
namespace Engine
{
namespace Social
{

namespace
{
    // Random unique magic number
    const quint32 CacheMagicNumber = 0xa0299735;
    const int CacheDataStreamVersion = QDataStream::Qt_4_8;  

    // Fixed salt we use the generate the filename for our cache file
    const quint64 FilenameHashSalt = 0x58b718449643ceb5LL;

    // Creates a random expiry time for a cache entry
    // These values are random so our server hits are staggered
    qint64 randomExpiryEpochMsecs()
    {
        const unsigned int MinimumExpiryMsecs = 3 * 24 * 60 * 60 * 1000;
        const unsigned int MaximumExpiryMsecs = 7 * 24 * 60 * 60 * 1000;

        return QDateTime::currentMSecsSinceEpoch() + 
               MinimumExpiryMsecs + 
               (rand() % (MaximumExpiryMsecs - MinimumExpiryMsecs));
    }

    // Create a new salt value
    quint64 generateRandomSalt()
    {
        quint64 ret;

        ret = rand();
        ret = ret << 32;
        ret |= rand();

        return ret;
    }
}

SaltedNucleusIdHashUser::SaltedNucleusIdHashUser(quint64 salt) :
    mSalt(salt)
{
}

void SaltedNucleusIdHashUser::setSalt(quint64 salt)
{
    mSalt = salt;
    mCache.clear();
}

QByteArray SaltedNucleusIdHashUser::saltedNucleusIdHash(quint64 nucleusId) const
{
    // Skip the hash if we can
    if (!mCache.contains(nucleusId))
    {
        mCache.insert(nucleusId, saltedNucleusIdHash(nucleusId, salt()));
    }

    return mCache.value(nucleusId);
}

QByteArray SaltedNucleusIdHashUser::saltedNucleusIdHash(quint64 nucleusId, quint64 salt)
{
    QCryptographicHash hasher(QCryptographicHash::Sha1);
    
    // XOR the salt on
    nucleusId ^= salt;
    hasher.addData(reinterpret_cast<const char *>(&nucleusId), 8);

    return hasher.result();
}

OriginTitleCacheEntry::OriginTitleCacheEntry(const QSet<Services::CommonGame> &owned, qint64 expiryEpochMsecs) :
    mNull(false),
    mOwnedGames(owned),
    mExpiryEpochMsecs(expiryEpochMsecs)
{
}

bool OriginTitleCacheEntry::isFresh() const
{
    return !mNull && (mExpiryEpochMsecs > QDateTime::currentMSecsSinceEpoch());
}
    
bool OriginTitleCacheEntry::ownsCompatibleGame(const Services::CommonGame &game) const
{
    return mOwnedGames.contains(game);
}

NonOriginGameCacheEntry::NonOriginGameCacheEntry(quint64 salt, const QSet<QByteArray> &covered, const QSet<QByteArray> &common, qint64 expiryEpochMsecs) :
    SaltedNucleusIdHashUser(salt),
    mCoveredHashedNucleusIds(covered),
    mCommonHashedNucleusIds(common),
    mExpiryEpochMsecs(expiryEpochMsecs)
{
}

bool NonOriginGameCacheEntry::isFresh() const
{
    return !mCoveredHashedNucleusIds.isEmpty() && (mExpiryEpochMsecs > QDateTime::currentMSecsSinceEpoch());
}
    
bool NonOriginGameCacheEntry::coversNucleusId(quint64 nucleusId) const
{
    return mCoveredHashedNucleusIds.contains(saltedNucleusIdHash(nucleusId));
}

bool NonOriginGameCacheEntry::nucleusIdHasInCommon(quint64 nucleusId) const
{
    return mCommonHashedNucleusIds.contains(saltedNucleusIdHash(nucleusId));
}
    
CommonTitlesCache::CommonTitlesCache(const QString &cacheDirPath, quint64 nucleusId)
{
    QDir cacheDir(cacheDirPath);

    if (!QDir(cacheDir).exists())
    {
        QDir("/").mkdir(cacheDirPath);
    }

    mCacheFilename = cacheDir.filePath(saltedNucleusIdHash(nucleusId, FilenameHashSalt).toHex());

    if (!readFromDisk())
    {
        // Clear any state readFromDisk might have left behind
        mOriginTitleEntries.clear();
        mNonOriginGameEntries.clear();

        // Set a new salt
        setSalt(generateRandomSalt());
    }
}
    
const OriginTitleCacheEntry& CommonTitlesCache::originTitleEntryForNucleusId(quint64 nucleusId)
{
    const QByteArray hashedId(saltedNucleusIdHash(nucleusId));
    QHash<QByteArray, OriginTitleCacheEntry>::ConstIterator it = mOriginTitleEntries.find(hashedId);

    if (it == mOriginTitleEntries.constEnd())
    {
        // We return by ref so we can't use a temporary
        return mNullOriginTitleEntry;
    }

    return it.value();
}
    
void CommonTitlesCache::setOriginTitleEntryForNucleusId(quint64 nucleusId, const QSet<Services::CommonGame> &ownedGames)
{
    mOriginTitleEntries.insert(
            saltedNucleusIdHash(nucleusId), 
            OriginTitleCacheEntry(ownedGames, randomExpiryEpochMsecs())
    );
}
    
const NonOriginGameCacheEntry& CommonTitlesCache::nonOriginGameEntryForAppId(const QString &appId)
{
    QHash<QString, NonOriginGameCacheEntry>::ConstIterator it = mNonOriginGameEntries.find(appId);

    if (it == mNonOriginGameEntries.constEnd())
    {
        return mNullNonOriginGameEntry;
    }

    return it.value();
}
    
void CommonTitlesCache::setNonOriginGameEntryForAppId(const QString &appId, const QSet<quint64> &coveredNucleusIds, const QSet<quint64> &commonNucleusIds)
{
    // First we have to hash all the Nucleus IDs
    QSet<QByteArray> hashedCoveredIds;
    QSet<QByteArray> hashedCommonIds;

    for(QSet<quint64>::ConstIterator it = coveredNucleusIds.constBegin();
        it != coveredNucleusIds.constEnd();
        it++)
    {
        hashedCoveredIds << saltedNucleusIdHash(*it);
    }

    for(QSet<quint64>::ConstIterator it = commonNucleusIds.constBegin();
        it != commonNucleusIds.constEnd();
        it++)
    {
        hashedCommonIds << saltedNucleusIdHash(*it);
    }

    mNonOriginGameEntries.insert(
        appId, 
        NonOriginGameCacheEntry(salt(), hashedCoveredIds, hashedCommonIds, randomExpiryEpochMsecs())
    );
}
    
bool CommonTitlesCache::readFromDisk()
{
    QByteArray cacheData;
    QFile cacheFile(mCacheFilename);

    if (!cacheFile.open(QIODevice::ReadOnly))
    {
        // Can't open
        return false;
    }

    cacheData = qUncompress(cacheFile.readAll());

    QDataStream cacheStream(&cacheData, QIODevice::ReadOnly);
    cacheStream.setVersion(CacheDataStreamVersion);

    quint32 magic;
    quint64 salt;

    cacheStream >> magic;
    cacheStream >> salt;
    
    if ((magic != CacheMagicNumber) || (cacheStream.status() != QDataStream::Ok))
    {
        return false;
    }
    setSalt(salt);
    
    int originEntryCount;
    cacheStream >> originEntryCount;

    if (originEntryCount < 0)
    {
        return false;
    }
    
    mOriginTitleEntries.reserve(originEntryCount);
    while(originEntryCount--)
    {
        QSet<Services::CommonGame> ownedGames;

        QByteArray key;
        qint64 ownedGamesCount;
        qint64 expiryEpochMsecs;
        
        cacheStream >> key;
        cacheStream >> ownedGamesCount;

        while(ownedGamesCount--)
        {
            Services::CommonGame game;
            cacheStream >> game.masterTitleId;
            cacheStream >> game.multiPlayerId;
            ownedGames << game;
        }

        cacheStream >> expiryEpochMsecs;

        if (cacheStream.status() != QDataStream::Ok)
        {
            return false;
        }

        mOriginTitleEntries.insert(key, OriginTitleCacheEntry(ownedGames, expiryEpochMsecs));
    }
    
    int nonOriginEntryCount;
    cacheStream >> nonOriginEntryCount;

    if (nonOriginEntryCount < 0)
    {
        return false;
    }

    mNonOriginGameEntries.reserve(nonOriginEntryCount);

    while(nonOriginEntryCount--)
    {
        QString key;
        QSet<QByteArray> coveredHashedNucleusIds;
        QSet<QByteArray> commonHashedNucleusIds;
        qint64 expiryEpochMsecs;

        cacheStream >> key;
        cacheStream >> coveredHashedNucleusIds;
        cacheStream >> commonHashedNucleusIds;
        cacheStream >> expiryEpochMsecs;

        if (cacheStream.status() != QDataStream::Ok)
        {
            return false;
        }

        mNonOriginGameEntries.insert(
                key, 
                NonOriginGameCacheEntry(salt, coveredHashedNucleusIds, commonHashedNucleusIds, expiryEpochMsecs)
        );
    }

    return true;
}

void CommonTitlesCache::writeToDisk()
{
    QByteArray cacheData;
    QDataStream cacheStream(&cacheData, QIODevice::WriteOnly);

    cacheStream.setVersion(CacheDataStreamVersion);

    cacheStream << CacheMagicNumber;
    cacheStream << salt();

    cacheStream << mOriginTitleEntries.count();
    for(QHash<QByteArray, OriginTitleCacheEntry>::ConstIterator it = mOriginTitleEntries.constBegin();
        it != mOriginTitleEntries.constEnd();
        it++)
    {
        cacheStream << it.key();

        cacheStream << static_cast<qint64>(it->mOwnedGames.count());

        for(auto gameIt = it->mOwnedGames.constBegin(); gameIt != it->mOwnedGames.constEnd(); gameIt++)
        {
            cacheStream << gameIt->masterTitleId;
            cacheStream << gameIt->multiPlayerId;
        }

        cacheStream << it->mExpiryEpochMsecs;
    }

    cacheStream << mNonOriginGameEntries.count();
    for(QHash<QString, NonOriginGameCacheEntry>::ConstIterator it = mNonOriginGameEntries.constBegin();
        it != mNonOriginGameEntries.constEnd();
        it++)
    {
        cacheStream << it.key();
        cacheStream << it->mCoveredHashedNucleusIds;
        cacheStream << it->mCommonHashedNucleusIds;
        cacheStream << it->mExpiryEpochMsecs;
    }

    QFile cacheFile(mCacheFilename);
    if (cacheFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        cacheFile.write(qCompress(cacheData));
    }
}

}
}
}

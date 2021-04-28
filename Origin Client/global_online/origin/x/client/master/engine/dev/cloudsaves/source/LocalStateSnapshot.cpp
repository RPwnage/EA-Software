#include <limits>
#include <QDateTime>
#include <QDataStream>
#include <QFileInfo>
#include <QDir>
#include <QMutexLocker>

#include "engine/cloudsaves/LocalStateSnapshot.h"
#include "engine/cloudsaves/RemoteStateSnapshot.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#if defined(ORIGIN_PC)
#include <io.h>
#include <windows.h>
#elif defined(ORIGIN_MAC)
#include <sys/stat.h>
#include <time.h>
#endif

namespace
{
    // Random magic number
    const quint32 CacheMagicNumber = 0xe4dd1243;
    // We currently support loading version 3 and version 4 and saving version 4
    const quint32 CacheCurrentVersion = 4;
    // The Qt data stream format we use
    const int CacheDataStreamVersion = QDataStream::Qt_4_7;

    // Platform specific signature
    // These are split up because POSIX can do a stat() directly on either a path
    // or an fd
    QByteArray instanceIdForPath(const QString &path);
    QByteArray instanceIdForFile(const QFile &file);

#if defined(ORIGIN_PC)
    QDataStream &operator<<(QDataStream &stream, DWORD dword)
    {
        return stream << quint32(dword);
    }

    QDataStream &operator<<(QDataStream &stream, const FILETIME &fileTime)
    {
        return stream << fileTime.dwLowDateTime << fileTime.dwHighDateTime;
    }

    QByteArray instanceIdForPath(const QString &path)
    {
        // Windows needs an open file handle
        QFile file(path);

        if (!file.open(QIODevice::ReadWrite))
        {
            return QByteArray();
        }

        return instanceIdForFile(file);
    }

    QByteArray instanceIdForFile(const QFile &file)
    {
        ORIGIN_ASSERT(file.isOpen());

        HANDLE hFile = (HANDLE)_get_osfhandle(file.handle());
        BY_HANDLE_FILE_INFORMATION winFileInfo;

        if (!GetFileInformationByHandle(hFile, &winFileInfo))
        {
            // Can't get file info
            return QByteArray();
        }
        
        // Build the instance ID
        QByteArray instanceId;
        QDataStream instanceStream(&instanceId, QIODevice::WriteOnly);

        // Dump all the info that would cause us to consider the file changed
        instanceStream << winFileInfo.ftCreationTime;
        instanceStream << winFileInfo.ftLastWriteTime;
    
        instanceStream << winFileInfo.nFileSizeHigh;
        instanceStream << winFileInfo.nFileSizeLow;

        instanceStream << winFileInfo.dwVolumeSerialNumber;
        instanceStream << winFileInfo.nFileIndexHigh;
        instanceStream << winFileInfo.nFileIndexLow;

        return instanceId;
    }

#elif defined(ORIGIN_MAC)
    QDataStream &operator<<(QDataStream &stream, const timespec &timeSpec)
    {
        // Cast to 64bit so we have a consistent file format between 64 and 32bit
        return stream << static_cast<qint64>(timeSpec.tv_sec) << static_cast<qint64>(timeSpec.tv_nsec);
    }

    // This is used by both instanceIdForPath and instanceIdForFile
    QByteArray instanceIdForStat(const struct stat &unixFileInfo)
    {
        // Build the instance ID
        QByteArray instanceId;
        QDataStream instanceStream(&instanceId, QIODevice::WriteOnly);

        instanceStream << unixFileInfo.st_birthtimespec;
        instanceStream << unixFileInfo.st_mtimespec;
        instanceStream << unixFileInfo.st_size;
        instanceStream << unixFileInfo.st_dev;
        instanceStream << unixFileInfo.st_ino;

        return instanceId;
    }

    QByteArray instanceIdForPath(const QString &path)
    {
        struct stat unixFileInfo;

        if (stat(QFile::encodeName(QDir::toNativeSeparators(path)), &unixFileInfo) == 0)
        {
            return instanceIdForStat(unixFileInfo);
        }

        return QByteArray();
    }
    
    QByteArray instanceIdForFile(const QFile &file)
    {
        struct stat unixFileInfo;

        if (fstat(file.handle(), &unixFileInfo) == 0)
        {
            return instanceIdForStat(unixFileInfo);
        }

        return QByteArray();
    }
#endif

}

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    LocalStateSnapshot::LocalStateSnapshot(const QUuid &lineage)
    {
        // Create a new lineage
        // This will be overwritten if we're read from disk
        if (lineage.isNull())
        {
            m_lineage = QUuid::createUuid();
        }
        else
        {
            m_lineage = lineage;
        }
    }
    
    LocalStateSnapshot::LocalStateSnapshot(const RemoteStateSnapshot &snap)
    {
        const QHash<FileFingerprint, SyncableFile> &remoteHash = snap.fileHash();

        m_lineage = snap.lineage();

        for(QHash<FileFingerprint, SyncableFile>::const_iterator hashIt = remoteHash.constBegin();
            hashIt != remoteHash.constEnd();
            hashIt++)
        {
            // Add this to our hash
            m_fileHash[hashIt.key()] = hashIt.value();

            // Trust everything was downloaded and the fingerprint is correct
            const FileFingerprint &fingerprint = hashIt.value().fingerprint();
            const QSet<LocalInstance> &localInstances = hashIt.value().localInstances();

            for(QSet<LocalInstance>::const_iterator localInstanceIt = localInstances.constBegin();
                localInstanceIt != localInstances.constEnd();
                localInstanceIt++)
            {
                // Get our instance ID
                const QString &localPath = (*localInstanceIt).path();

                QByteArray instanceId = instanceIdForPath(localPath);
                m_md5Cache[localPath] = CacheEntry(*localInstanceIt, fingerprint, instanceId);
            }
        }
    }
        
    bool LocalStateSnapshot::load(QDataStream &loadStream)
    {
        // Do some basic sanity checks
        quint32 magicNumber = 0x0;
        quint32 cacheVersion = 1;

        loadStream.setVersion(CacheDataStreamVersion);

        loadStream >> magicNumber;
        loadStream >> cacheVersion;

        if ((loadStream.status() != QDataStream::Ok) ||
            (magicNumber != CacheMagicNumber))
        {
            // Something is wrong 
            ORIGIN_LOG_WARNING << "Invalid header while loading cloud saves local state snapshot";
            ORIGIN_ASSERT(0);

            return false;
        }

        if ((cacheVersion < 3) || (cacheVersion > CacheCurrentVersion))
        {
            // Unsupported version
            ORIGIN_LOG_WARNING << "Unsupported version while loading cloud saves local state snapshot";
            return false;
        }

        loadStream >> m_lineage;

        quint32 entries = 0;
        loadStream >> entries;

        for(quint32 i = 0; i < entries; i++) 
        {
            QString path;
            QByteArray instanceId;
            qint64 size;
            QByteArray md5;
            QDateTime lastModified;
            QDateTime created;

            loadStream >> path;
            loadStream >> instanceId;
            loadStream >> size;
            loadStream >> md5;
            
            // Old versions don't have this data so we use the default null datetimes
            // The rest of cloud saves interpret this as "unknown"
            if (cacheVersion >= 4)
            {
                loadStream >> lastModified;
                loadStream >> created;
            }

            if (loadStream.status() != QDataStream::Ok)
            {
                // Break out and the test below the loop will handle this
                break;
            }

            LocalInstance newInstance(path, lastModified, created);
            addLocalInstance(newInstance, FileFingerprint(size, md5), instanceId);
        }

        if (loadStream.status() != QDataStream::Ok)
        {
            // Truncated?
            ORIGIN_LOG_WARNING << "Possibly truncated stream while loading cloud saves local state snapshot";
            ORIGIN_ASSERT(0);
            m_md5Cache.clear();

            return false;
        }

        return true;
    }

    bool LocalStateSnapshot::load(QIODevice *dev)
    {
        QByteArray sourceData = qUncompress(dev->readAll());
        QDataStream sourceStream(sourceData);
        return load(sourceStream);
    }
    
    bool LocalStateSnapshot::save(QDataStream &saveStream) const
    {
        saveStream.setVersion(CacheDataStreamVersion);

        saveStream << CacheMagicNumber;
        saveStream << CacheCurrentVersion;

        saveStream << m_lineage;

        saveStream << quint32(m_md5Cache.count());

        for(MD5Cache::const_iterator it = m_md5Cache.constBegin();
            it != m_md5Cache.constEnd();
            it++)

        {
            saveStream << it.key();
            saveStream << it.value().instanceId;
            saveStream << it.value().fingerprint.size();
            saveStream << it.value().fingerprint.md5();
            saveStream << it.value().instance.lastModified();
            saveStream << it.value().instance.created();
        }

        return true;
    }

    bool LocalStateSnapshot::save(QIODevice *dev) const
    {
        // Go through a QByteArray so we can compress our result
        QByteArray saveData;
        QDataStream saveStream(&saveData, QIODevice::WriteOnly);

        if (!save(saveStream))
        {
            return false;
        }

        dev->write(qCompress(saveData));

        return true;
    }
        
    bool LocalStateSnapshot::importLocalFile(const QFileInfo &info, LocalInstance::FileTrust trust, const LocalStateSnapshot &other)
    {
        MD5Cache const &cache = other.m_md5Cache;
        QString path = info.absoluteFilePath();

        if (cache.isEmpty())
        {
            // Nothing in the MD5 cache
            return false;
        }

        // Try to search the cache directly
        MD5Cache::const_iterator it = cache.constFind(path);

        if (it == cache.constEnd())
        {
            // Now try the canonical path 
            // Getting the canonical path can be expensive on network drives
            QString canonical = info.canonicalFilePath();
            it = cache.constFind(canonical);
        }
        
        if (it == cache.constEnd())
        {
            // Not in the cache
            return false; 
        }

        if (it.value().instanceId != instanceIdForPath(path))
        {
            // Not the expected instance ID
            // Possibly modified
            return false;
        }

        // Add to our start
        addLocalInstance(it.value().instance.withTrust(trust), it.value().fingerprint, it.value().instanceId);

        return true;
    }
        
    void LocalStateSnapshot::addLocalFile(const QFile &file, LocalInstance::FileTrust trust, const FileFingerprint &fingerprint)
    {
        // Find out our instance ID
        QByteArray instanceId = instanceIdForFile(file);
        QFileInfo info(file);
        
        // Directly add the file
        addLocalInstance(LocalInstance::fromFileInfo(info, trust), fingerprint, instanceId);
    }
    
    void LocalStateSnapshot::addLocalInstance(const LocalInstance &instance, const FileFingerprint &fingerprint, const QByteArray &instanceId)
    {
        // This is slow so get it outside our lock
        QMutexLocker locker(&m_addFileMutex);

        // Update our MD5 cache
        m_md5Cache[instance.path()] = CacheEntry(instance, fingerprint, instanceId);

        FileHashes::iterator existingFile = m_fileHash.find(fingerprint);

        if (existingFile == m_fileHash.end())
        {
            QSet<LocalInstance> newRemoteLocalInstances;
            newRemoteLocalInstances << instance;

            // Create a new file
            // Note the path we generate for the remote path is not authoritative
            // We'll use the path of an existing remote file on S3 if it exists
            m_fileHash.insert(fingerprint, SyncableFile(fingerprint, newRemoteLocalInstances));
        }
        else
        {
            // Duplicate local file - add to existing remote
            (*existingFile).addLocalInstance(instance);
        }
    }
    
    QDateTime LocalStateSnapshot::lastModified() const
    {
        QDateTime newest;

        for(FileHashes::const_iterator syncableIt = m_fileHash.constBegin(); syncableIt != m_fileHash.constEnd(); syncableIt++)
        {
            const SyncableFile &syncable = *syncableIt;

            for(LocalInstances::const_iterator localInstanceIt = syncable.localInstances().constBegin();
                localInstanceIt != syncable.localInstances().constEnd();
                localInstanceIt++)
            {
                QFileInfo localInstanceInfo((*localInstanceIt).path());

                if (newest.isNull())
                {
                    newest = localInstanceInfo.lastModified();
                }
                else
                {
                    newest = qMax(localInstanceInfo.lastModified(), newest);
                }
            }
        }

        return newest;
    }
}
}
}

//    Copyright (c) 2014, Electronic Arts
//    All rights reserved.
//    Author: Jonathan Kolb

#include "UserSpecificContentCache.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"

#include "services/config/OriginConfigService.h"
#include "services/crypto/CryptoService.h"
#include "services/session/SessionService.h"

#ifdef _WIN32
#include <shlobj.h>
#include <shobjidl.h>
#else
#include "services/platform/PlatformService.h"
#endif

#include "TelemetryAPIDLL.h"

namespace Origin
{

namespace Engine
{

namespace Content
{

const unsigned int CacheSerializationMagic = 0xb25a145e;
const QDataStream::Version CacheSerializationVersion = QDataStream::Qt_5_0;

UserSpecificContentCache::UserSpecificContentCache()
    : m_cacheDataVersion(0)
    , m_cacheCipher(Services::CryptoService::BLOWFISH)
    , m_cacheCipherMode(Services::CryptoService::CIPHER_BLOCK_CHAINING)
{
}

UserSpecificContentCache::~UserSpecificContentCache()
{
}

void UserSpecificContentCache::writeUserCacheData()
{
    ORIGIN_ASSERT(!m_cacheFolder.isEmpty());
    ORIGIN_ASSERT(!m_cacheEntropy.isEmpty());

    Services::Session::SessionRef session = Services::Session::SessionService::currentSession();
    if (session.isNull())
    {
        ORIGIN_LOG_WARNING << "Invalid session in writeUserCacheData.";
        return;
    }

    QByteArray cacheContents;
    QDataStream cacheContentStream(&cacheContents, QIODevice::WriteOnly);
    cacheContentStream.setVersion(CacheSerializationVersion);
    cacheContentStream << CacheSerializationMagic;
    cacheContentStream << m_cacheDataVersion;

    serializeCacheData(cacheContentStream);

    QByteArray encrypted;
    if (!Services::CryptoService::encryptSymmetric(encrypted, cacheContents, getCryptKey(session), m_cacheCipher, m_cacheCipherMode))
    {
        ORIGIN_LOG_ERROR << "Failed to encrypt cache data" << Services::CryptoService::sslError();
        return;
    }

    const QString &cacheFilePath = getCacheFilePath(session);
    QFile cacheFile(cacheFilePath);
    if (!cacheFile.open(QIODevice::WriteOnly))
    {
        ORIGIN_LOG_ERROR << "Failed to open cache file for write: " << cacheFilePath;
        return;
    }

    cacheFile.write(encrypted);
    cacheFile.close();
}

void UserSpecificContentCache::readUserCacheData()
{
    Services::Session::SessionRef session = Services::Session::SessionService::currentSession();
    if (session.isNull())
    {
        ORIGIN_LOG_WARNING << "Invalid session in readUserCacheData.";
        return;
    }

    const QString &cacheFilePath = getCacheFilePath(session);
    QFile cacheFile(cacheFilePath);
    if (!cacheFile.exists() || !cacheFile.open(QIODevice::ReadOnly))
    {
        return;
    }

    QByteArray encryptedContents = cacheFile.readAll();
    QByteArray contents;
    if (!Services::CryptoService::decryptSymmetric(contents, encryptedContents, getCryptKey(session), m_cacheCipher, m_cacheCipherMode))
    {
        ORIGIN_LOG_ERROR << "Failed to decrypt cache file: " << Services::CryptoService::sslError();
        return;
    }

    QDataStream cacheContentStream(&contents, QIODevice::ReadOnly);
    cacheContentStream.setVersion(CacheSerializationVersion);
    unsigned int magic;
    cacheContentStream >> magic;

    if (magic != CacheSerializationMagic)
    {
        ORIGIN_LOG_ERROR << "Unrecognized cache format!";
        return;
    }

    quint32 dataVersion;
    cacheContentStream >> dataVersion;

    if (m_cacheDataVersion != dataVersion)
    {
        ORIGIN_LOG_DEBUG << "Ignoring cache data version (" << dataVersion << "); current data version is (" << m_cacheDataVersion << ")";
        return;
    }

    deserializeCacheData(cacheContentStream);
}

QString UserSpecificContentCache::getCacheFilePath(Services::Session::SessionRef session) const
{
    QStringList path;

#ifdef _WIN32
    // Get the path to roaming app data
    wchar_t appDataPath[MAX_PATH + 1];
    if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath) != S_OK)
    {
        ORIGIN_ASSERT(0);
        return QString();
    }

    path << QString::fromUtf16(appDataPath) << "Origin";
#else
    path << Services::PlatformService::getStorageLocation(QStandardPaths::DataLocation);
#endif

    path << m_cacheFolder;

    QString cachePath = path.join(QDir::separator());
    if (!QDir("/").exists(cachePath))
    {
        ORIGIN_LOG_ACTION << "Creating entitlement cache folder: " << cachePath;
        QDir("/").mkpath(cachePath);
    }

    QString userId = Services::Session::SessionService::nucleusUser(session);
    QString filename = QCryptographicHash::hash(userId.toUtf8(), QCryptographicHash::Sha1).toHex().toUpper();
    if (!m_cacheFileExtension.isEmpty())
    {
        filename += "." + m_cacheFileExtension;
    }

    return QString("%1%2%3").arg(cachePath, QDir::separator(), filename);
}

// Returns a byte array containing data suitable for use as encryption key. 
QByteArray UserSpecificContentCache::getCryptKey(Services::Session::SessionRef session) const
{
    ORIGIN_ASSERT(!m_cacheEntropy.isEmpty());

    QCryptographicHash hash(QCryptographicHash::Sha1);

    hash.addData(m_cacheEntropy);
    hash.addData(Services::Session::SessionService::nucleusUser(session).toUtf8());

    // pad the key appropriately for the given cipher
    return Services::CryptoService::padKeyFor(hash.result(), m_cacheCipher, m_cacheCipherMode);
}

} // namespace Content

} // namespace Engine

} // namespace Origin

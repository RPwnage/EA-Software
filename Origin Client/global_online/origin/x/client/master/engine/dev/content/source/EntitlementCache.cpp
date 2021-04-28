#include "engine/content/EntitlementCache.h"
#include "services/settings/SettingsManager.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "services/platform/PlatformService.h"
#include <QDesktopServices>
#include <QMutexLocker>

#ifdef _WIN32
#include <shlobj.h>
#include <shobjidl.h>
#endif

#if defined(_DEBUG)
#define ENCRYPT_CACHE 1
#else
// ensure that the following remains at 1 so that we always encrypt in release builds
#define ENCRYPT_CACHE 1
#endif

#if ENCRYPT_CACHE
#include "services/crypto/CryptoService.h"
#endif

namespace Origin
{

namespace Engine
{

namespace Content
{

const QString EntitlementCache::SERVICE_DATA_KEY = "EntitlementCache";
const QString BASEGAMES_CACHENAME = "bg%1.dat";
const QString EXTRACONTENT_CACHENAME = "ec%1.dat";
const QString SDKENTITLEMENTS_CACHENAME = "sdke%1.dat";

QPointer<EntitlementCache> sInstance;

void EntitlementCache::init()
{
    if (sInstance.isNull())
        sInstance = new EntitlementCache;
}

void EntitlementCache::release()
{
    delete sInstance.data();
    sInstance = NULL;
}

EntitlementCache* EntitlementCache::instance()
{
    ORIGIN_ASSERT(sInstance.data());
    return sInstance.data();
}

EntitlementCache::EntitlementCache()
{
}

#if ENCRYPT_CACHE
static CryptoService::Cipher const sOfflineCipher = CryptoService::BLOWFISH;
static CryptoService::CipherMode const sOfflineCipherMode = CryptoService::CIPHER_BLOCK_CHAINING;

// given a string containing any high-entropy data, return a byte array containing
// data suitable for use as encryption key. 
QByteArray cryptKey(QString const& entropy)
{
    QByteArray key;

    if(entropy.isEmpty())
    {
        key.append("jdue923k12mdmccjsajiuesasksow2398201ksdkei483j2wsk"); // a little better than nothing?
    }
    else
    {
        key.append(entropy);
    }

    key = QCryptographicHash::hash(key, QCryptographicHash::Sha1);

    // pad the key appropriately for the given cipher
    return CryptoService::padKeyFor(key, sOfflineCipher, sOfflineCipherMode);
}
#endif

bool EntitlementCache::readCacheFile(CachedResponseData& contents, QString const& path, Session::SessionRef session)
{
    QFile file(path);
    if ( file.open(QIODevice::ReadOnly) )
    {
        bool ret = true;
        QDataStream stream(&file);
        int responseType;
        QByteArray encrypted;
        stream >> responseType >> encrypted >> contents.m_responseSignature >> contents.m_responseCustomData;
        contents.m_responseType = static_cast<Content::CachedResponseType>(responseType);

#if ENCRYPT_CACHE
        ret = CryptoService::decryptSymmetric(contents.m_responseData, encrypted, cryptKey(contents.m_responseSignature), sOfflineCipher, sOfflineCipherMode);
        if ( !ret )
        {
            QString error("Failed to decrypt entitlement cache: ");
            error += CryptoService::sslError();
            ORIGIN_LOG_ERROR << error;
        }
#else
        contents.m_responseData = encrypted;
#endif
        return ret;
    }
    return false;
}

bool EntitlementCache::writeCacheFile(CachedResponseData const& contents, QString const& path, Session::SessionRef session)
{
    QByteArray encrypted;
#if ENCRYPT_CACHE
    if ( !CryptoService::encryptSymmetric(encrypted, contents.m_responseData, cryptKey(contents.m_responseSignature), sOfflineCipher, sOfflineCipherMode) )
    {
        ORIGIN_LOG_ERROR << "Failed to encrypt entitlement cache";
        return false;
    }
#else
    encrypted = contents.m_responseData;
#endif

    QFile file(path);
    if ( file.open(QIODevice::WriteOnly) )
    {
        QDataStream stream(&file);
        stream << static_cast<int>(contents.m_responseType) << encrypted << contents.m_responseSignature << contents.m_responseCustomData;

        if ( stream.status() == QDataStream::Ok)
            return true;

        ORIGIN_LOG_ERROR << "Failed to write encrypt entitlement cache file";
    }
    else
    {
        ORIGIN_LOG_ERROR << "Failed to open encrypt entitlement cache file";
    }
    return false;
}

void EntitlementCache::onBeginSessionComplete(Session::SessionError, Session::SessionRef session)
{
    //this slot could be called even if user failed to login so session could be NULL
    if (!session.isNull())
    {
        // compute the path to the entitlement cache, which consists of a platform dependent
        // root directory, hard-coded application specific directories, environment directory 
        // (omitted in production), and a per-user directory containing the actual cache files.
        QStringList path;
    #if defined(ORIGIN_PC)
        WCHAR buffer[MAX_PATH];
        SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, buffer );
        path << QString::fromWCharArray(buffer) << "Origin";
    #elif defined(ORIGIN_MAC)
        path << Origin::Services::PlatformService::commonAppDataPath();
    #else
    #error Must specialize for other platform.
    #endif

        QString environment = readSetting(SETTING_EnvironmentName);
        path << "EntitlementCache";
        if ( environment != "production" )
            path << environment;

        QString userId = Session::SessionService::nucleusUser(session);
        path << QCryptographicHash::hash(userId.toLatin1(), QCryptographicHash::Sha1).toHex().toUpper();
        QString cacheDirName    = path.join(QDir::separator()) + QDir::separator();

        path << BASEGAMES_CACHENAME;

        m_cacheFileTemplate = path.join(QDir::separator());

        path.removeLast();

        path << EXTRACONTENT_CACHENAME;

        m_extraContentCacheFileTemplate = path.join(QDir::separator());
        
        path.removeLast();
        path << SDKENTITLEMENTS_CACHENAME;
        m_sdkEntitlementsCacheFileTemplate = path.join(QDir::separator());

        // Create the object that will hold the actual entitlement cache data within the session.
        // After a failed login this will already exist.
        if ( !session->serviceData(SERVICE_DATA_KEY) )
            session->addServiceData(SERVICE_DATA_KEY, Session::AbstractSession::ServiceDataRef(new EntitlementCacheServiceData()));

        QVector<CachedResponseData>* cache = cacheData(session);
        ORIGIN_ASSERT(cache);
        if (!cache)
            return;
        
        QHash<QString, CachedResponseData>* ecCache = extraContentCacheData(session);
        ORIGIN_ASSERT(ecCache);
        if(!ecCache)
            return;
                
        QHash<QString, CachedResponseData>* sdkCache = sdkEntitlementsCacheData(session);
        ORIGIN_ASSERT(sdkCache);
        if(!sdkCache)
            return;

        QWriteLocker writeLock(&m_cacheLock);

        cache->clear();
        ecCache->clear();
        sdkCache->clear();

        QDir cacheDir(cacheDirName);

        // see if directory exists, create it if not
        if ( cacheDir.exists() ) 
        {
            // read zero or more offline cache files using 0-based indexing
            for( int i = 0; true; i++ )
            {
                QString path = m_cacheFileTemplate.arg(i++);
                CachedResponseData contents;
                if ( !readCacheFile(contents, path, session) )
                    break;
                cache->push_back(contents);
            }

            // Read all ec cache files
            QFileInfoList entryList = cacheDir.entryInfoList(QStringList(EXTRACONTENT_CACHENAME.arg("*")));

            foreach(const QFileInfo& ecCacheFile, entryList)
            {
                CachedResponseData contents;
                if(!readCacheFile(contents, ecCacheFile.absoluteFilePath(), session))
                {
                    ORIGIN_LOG_WARNING << "Failed to read EC cache file.";
                    continue;
                }
                else
                {
                    ORIGIN_LOG_DEBUG_IF(Origin::Services::DebugLogToggles::entitlementLoggingEnabled) << "Read cache file [" << ecCacheFile.fileName() << "] for master title id: " << contents.m_responseCustomData;
                }

                ecCache->insert(contents.m_responseCustomData, contents);
            }

            // Read all sdk cache files
            QFileInfoList sdkEntryList = cacheDir.entryInfoList(QStringList(SDKENTITLEMENTS_CACHENAME.arg("*")));

            foreach(const QFileInfo& sdkCacheFile, sdkEntryList)
            {
                CachedResponseData contents;
                if(!readCacheFile(contents, sdkCacheFile.absoluteFilePath(), session))
                {
                    ORIGIN_LOG_WARNING << "Failed to read SDK cache file.";
                    continue;
                }
                else
                {
                    ORIGIN_LOG_DEBUG_IF(Origin::Services::DebugLogToggles::entitlementLoggingEnabled) << "Read cache file [" << sdkCacheFile.fileName() << "]";
                }

                sdkCache->insert(contents.m_responseCustomData, contents);
            }
        }
        else if(!QDir().mkpath(cacheDirName))
        {
            ORIGIN_LOG_ERROR << "Could not create EntitlementCache directory: " << cacheDirName;
        }
    }
}

EntitlementCache::~EntitlementCache()
{
}

QVector<Content::CachedResponseData>* EntitlementCache::cacheData(Session::SessionRef session)
{
    QReadLocker readLock(&m_cacheLock);

    if ( Session::SessionService::isValidSession(session) )
    {
        QSharedPointer<EntitlementCacheServiceData> data = session->serviceData(SERVICE_DATA_KEY).dynamicCast<EntitlementCacheServiceData>();
        ORIGIN_ASSERT(data);
        if ( data )
        {
            return &data->m_offlineCacheData;
        }
    }
    return NULL;
}

QHash<QString, Content::CachedResponseData>* EntitlementCache::extraContentCacheData(Session::SessionRef session)
{
    QReadLocker readLock(&m_cacheLock);

    if ( Session::SessionService::isValidSession(session) )
    {
        QSharedPointer<EntitlementCacheServiceData> data = session->serviceData(SERVICE_DATA_KEY).dynamicCast<EntitlementCacheServiceData>();
        ORIGIN_ASSERT(data);
        if ( data )
        {
            return &data->m_offlineExtraContentCacheData;
        }
    }
    return NULL;
}

QHash<QString, Engine::Content::CachedResponseData>* EntitlementCache::sdkEntitlementsCacheData(Session::SessionRef session)
{
    QReadLocker readLock(&m_cacheLock);

    if ( Session::SessionService::isValidSession(session) )
    {
        QSharedPointer<EntitlementCacheServiceData> data = session->serviceData(SERVICE_DATA_KEY).dynamicCast<EntitlementCacheServiceData>();
        ORIGIN_ASSERT(data);
        if ( data )
        {
            return &data->m_offlineSdkEntitlementsCacheData;
        }
    }
    return NULL;
}

bool EntitlementCache::insert( Session::SessionRef session, QString const& entropy, CachedResponseData const& data )
{
    if(data.m_responseType == Content::ResponseTypeExtraContent)
    {
        QHash<QString, CachedResponseData>* cache = extraContentCacheData(session);
        ORIGIN_ASSERT(cache);
        if (!cache)
            return false;

        QString masterTitleId = data.m_responseCustomData;
        cache->insert(masterTitleId, data);
        
        QString path = m_extraContentCacheFileTemplate.arg(masterTitleId);
        return writeCacheFile(data, path, session);
    }
    else if(data.m_responseType == Content::ResponseTypeSdkEntitlements)
    {
        QHash<QString, CachedResponseData>* cache = sdkEntitlementsCacheData(session);
        ORIGIN_ASSERT(cache);
        if (!cache)
            return false;

        QString cacheKey = data.m_responseCustomData;
        cache->insert(cacheKey, data);
        
        QString path = m_sdkEntitlementsCacheFileTemplate.arg(cacheKey);
        return writeCacheFile(data, path, session);
    }
    else
    {
        QVector<CachedResponseData>* cache = cacheData(session);
        ORIGIN_ASSERT(cache);
        if (!cache)
            return false;

        QWriteLocker writeLock(&m_cacheLock);

        // get vector size before push_back to get 0-based indexing of files
        QString path = m_cacheFileTemplate.arg(cache->size()); 
        cache->push_back(data);

        // Tried encrypting and writing the data in a separate thread here. The
        // speedup using multithreading was 0.035 seconds (difference of averages 
        // with N=9 for both) when tested on an account with 93 entitlements. 
        // Going with single threading here. The average overall duration of the 
        // write operation for the single-threaded case was 0.16 seconds.

        //QtConcurrent::run(*this, &EntitlementCache::writeCacheFile, data, path, session);

        return writeCacheFile(data, path, session);
    }
}

void EntitlementCache::clear(Session::SessionRef session)
{
    QVector<CachedResponseData>* cache = cacheData(session);

    QWriteLocker writeLock(&m_cacheLock);

    ORIGIN_ASSERT(cache);
    if (cache)
        cache->clear();

    QString tmpl = m_cacheFileTemplate.arg("*");
    tmpl.replace("\\","/"); // just to be sure
    int lastSlash = tmpl.lastIndexOf("/");
    QString dir = tmpl.left(lastSlash+1);
    QString filespec = tmpl.mid(lastSlash+1);

    QFileInfoList toDelete = QDir(dir).entryInfoList(QStringList() << filespec, QDir::Files);
    for ( QFileInfoList::const_iterator i = toDelete.begin(); i != toDelete.end(); ++i )
        if ( !QFile::remove(i->absoluteFilePath()) )
            ORIGIN_LOG_ERROR << "Could not remove " << i->absoluteFilePath();
}

void EntitlementCache::clearExtraContent(Session::SessionRef session, const QString& masterTitleId)
{
    QHash<QString, CachedResponseData>* cache = extraContentCacheData(session);

    QWriteLocker writeLock(&m_cacheLock);

    ORIGIN_ASSERT(cache);

    if (cache)
    {
        if(!masterTitleId.isEmpty())
        {
            cache->remove(masterTitleId);
        }
        else
        {
            cache->clear();
        }
    }

    QString tmpl;
    if(masterTitleId.isEmpty())
    {
        tmpl = m_extraContentCacheFileTemplate.arg("*");
    }
    else
    {
        tmpl = m_extraContentCacheFileTemplate.arg(masterTitleId);
    }

    tmpl.replace("\\","/"); // just to be sure
    int lastSlash = tmpl.lastIndexOf("/");
    QString dir = tmpl.left(lastSlash+1);
    QString filespec = tmpl.mid(lastSlash+1);

    QFileInfoList toDelete = QDir(dir).entryInfoList(QStringList() << filespec, QDir::Files);
    for ( QFileInfoList::const_iterator i = toDelete.begin(); i != toDelete.end(); ++i )
        if ( !QFile::remove(i->absoluteFilePath()) )
            ORIGIN_LOG_ERROR << "Could not remove " << i->absoluteFilePath();
}

}

}

}


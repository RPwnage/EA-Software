///////////////////////////////////////////////////////////////////////////////
// NonOriginContentCache.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "engine/content/NonOriginContentCache.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "services/crypto/CryptoService.h"
#include "TelemetryAPIDLL.h"

#include "engine/content/NonOriginGameData.h"
#include "services/publishing/ConversionUtil.h"

#include "LSXWrapper.h"
#include "nonorigingame.h"
#include "nonorigingameReader.h"
#include "nonorigingameWriter.h"

#ifdef _WIN32
#include <shlobj.h>
#include <shobjidl.h>
#endif

#include <QDesktopServices>



#include <QDomDocument>

namespace Origin
{

namespace Engine
{

namespace Content
{

namespace Cache
{
    QPointer<NonOriginContentCache> sNonOriginCacheInstance;
    bool NonOriginContentCache::sIsDirty = false;

    const QString NonOriginContentCache::sCacheFileName = "NOGCache.xml";
    const QString NonOriginContentCache::sCacheVersion = "1";

    void NonOriginContentCache::init()
    {
        if (sNonOriginCacheInstance.isNull())
        {
            sNonOriginCacheInstance = new NonOriginContentCache();
        }
    }

    void NonOriginContentCache::release()
    {
        delete sNonOriginCacheInstance.data();
        sNonOriginCacheInstance = NULL;
    }

    NonOriginContentCache* NonOriginContentCache::instance()
    {
        return sNonOriginCacheInstance.data();
    }

    bool NonOriginContentCache::isDirty()
    {
        return sIsDirty;
    }

    void NonOriginContentCache::resetDirtyBit(bool isDirty)
    {
        sIsDirty = isDirty;
    }

    NonOriginContentCache::NonOriginContentCache()
    {
        sIsDirty = true;
    }

    NonOriginContentCache::~NonOriginContentCache()
    {

    }

    QString NonOriginContentCache::getCacheDirectory(Origin::Services::Session::SessionRef session)
    {
        QString cacheDir;
        if (!session.isNull())
        {
            QString environment = Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName);
            QString userId = Origin::Services::Session::SessionService::nucleusUser(session);
            userId = QCryptographicHash::hash(userId.toLatin1(), QCryptographicHash::Sha1).toHex().toUpper();

            // compute the path to the entitlement cache, which consists of a platform dependent
            // root directory, hard-coded application specific directories, environment directory 
            // (omitted in production), and a per-user directory containing the actual cache files.
            QStringList path;
            #if defined(ORIGIN_PC)
                WCHAR buffer[MAX_PATH];
                SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, buffer);
                path << QString::fromWCharArray(buffer) << "Origin";
            #elif defined(ORIGIN_MAC)
                path << Origin::Services::PlatformService::commonAppDataPath();
            #else
                #error Must specialize for other platform.
            #endif

            path << "NonOriginContentCache";
            if (environment != "production")
            {
                path << environment;
            }

            path << userId;
            cacheDir = path.join(QDir::separator());

            if (!QDir(cacheDir).exists())
            {
                QDir().mkpath(cacheDir);
            }
        }

        return cacheDir;
    }

    QString NonOriginContentCache::getCacheFilePath(Origin::Services::Session::SessionRef session)
    {
        QString cachePath;

        if (!session.isNull())
        {
            QStringList path = getCacheDirectory(session).split(QDir::separator());
            path << sCacheFileName;

            cachePath = path.join(QDir::separator());
        }

        return cachePath;
    }

    bool NonOriginContentCache::saveCache(Origin::Services::Session::SessionRef session, const QMap<QString, Engine::Content::NonOriginGameData>& productIdToNogMap)
    {
        bool result = false;

        QString cacheFilename = getCacheFilePath(session);

        nonorigingame::nogsT nogs;
        nogs.version = sCacheVersion;

        for (QMap<QString, Engine::Content::NonOriginGameData>::const_iterator i = productIdToNogMap.constBegin(); i != productIdToNogMap.constEnd(); ++i)
        {
            nonorigingame::nogT nog;
            nog.displayName = i.value().getDisplayName();
            nog.executableFile = i.value().getExecutableFile();
            nog.executableParameters = i.value().getExecutableParameters();
            nog.isIgoEnabled = i.value().isIgoEnabled();          
            nog.entitleDate = Services::Publishing::ConversionUtil::ConvertTime(i.value().getEntitleDate());

            nogs.nog.push_back(nog);
        }

        LSXWrapper res;
        nonorigingame::Write(&res, nogs);

        FILE * fp = fopen(cacheFilename.toStdString().c_str(), "w");

        if ( fp )
        {
            result = true;
            fputs(res.GetRootNode().ownerDocument().toByteArray(), fp);
            fclose(fp);

            sIsDirty = true;
        }

        return result;
    }

    bool NonOriginContentCache::readCache(Origin::Services::Session::SessionRef session, QMap<QString, Engine::Content::NonOriginGameData>& productIdToNogMap)
    {
        bool result = false;

        QString cacheFilePath = getCacheFilePath(session);
        QFile cacheFile(cacheFilePath);
        if (cacheFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QDomDocument doc;
            if (doc.setContent(&cacheFile))
            {
                QDomNodeList nogList = doc.documentElement().elementsByTagName("nog");
                for (int i = 0; i < nogList.size(); ++i)
                {
                    nonorigingame::nogT nog;
                    QScopedPointer<INodeDocument> nogNode(CreateXmlDocument(doc, nogList.at(i)));

                    if (nonorigingame::Read(nogNode.data(), nog))
                    {
                        Engine::Content::NonOriginGameData gameData;

                        gameData.setDisplayName(nog.displayName);
                        gameData.setExecutableFile(nog.executableFile);
                        gameData.setExecutableParameters(nog.executableParameters);
                        gameData.setIgoEnabled(nog.isIgoEnabled);
                        gameData.setEntitleDate(Services::Publishing::ConversionUtil::ConvertTime(nog.entitleDate));

                        productIdToNogMap.insert(gameData.getProductId(), gameData);
                     }
                }

                result = true;
            }
        }
        
        return result;
    }

    bool NonOriginContentCache::readLegacyCache(Origin::Services::Session::SessionRef session, QVector<QByteArray>& cache)
    {
        bool cacheRead = false;
        QStringList legacyCacheFilePath = getCacheDirectory(session).split(QDir::separator());
        legacyCacheFilePath << "nonOriginContentCache.dat";

        QFile legacyCacheFile(legacyCacheFilePath.join(QDir::separator()));
        if (legacyCacheFile.open(QIODevice::ReadOnly))
        {
            QDataStream stream(&legacyCacheFile);

            int numEntries;
            stream >> numEntries;

            for (int i = 0; i < numEntries; ++i)
            {
                QVariant v;
                stream >> v;

                cache.push_back(v.toByteArray());
            }

            cacheRead = true;
        }

        return cacheRead;
    }

    bool NonOriginContentCache::removeLegacyCache(Origin::Services::Session::SessionRef session)
    {
        QStringList legacyCacheFilePath = getCacheDirectory(session).split(QDir::separator());
        legacyCacheFilePath << "nonOriginContentCache.dat";

        QFile legacyCacheFile(legacyCacheFilePath.join(QDir::separator()));
        return legacyCacheFile.remove();
    }

} // Cache
} // Content
} // Engine
} // Origin

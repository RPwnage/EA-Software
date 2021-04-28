///////////////////////////////////////////////////////////////////////////////
// CustomBoxartCache.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/entitlements/cache/CustomBoxartCache.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformService.h"
#include "services/settings/SettingsManager.h"
#include "services/crypto/CryptoService.h"

#include "LSXWrapper.h"
#include "customboxart.h"
#include "customboxartReader.h"
#include "customboxartWriter.h"

#if defined(ORIGIN_PC)
#include <shlobj.h>
#include <shobjidl.h>
#endif

#include <QDesktopServices>
#include <QDomDocument>

namespace Origin
{

namespace Services
{

namespace Entitlements
{

namespace Cache
{
    QPointer<CustomBoxartCache> sCustomBoxartCacheInstance;
    bool CustomBoxartCache::sIsDirty = false;

    const QString CustomBoxartCache::sCacheFileName = "BoxartCache.xml";
    const QString CustomBoxartCache::sCacheVersion = "1";

    void CustomBoxartCache::init()
    {
        if (sCustomBoxartCacheInstance.isNull())
        {
            sCustomBoxartCacheInstance = new CustomBoxartCache();
        }
    }

    void CustomBoxartCache::release()
    {
        delete sCustomBoxartCacheInstance.data();
        sCustomBoxartCacheInstance = NULL;
    }

    CustomBoxartCache* CustomBoxartCache::instance()
    {
        return sCustomBoxartCacheInstance.data();
    }

    bool CustomBoxartCache::isDirty()
    {
        return sIsDirty;
    }

    void CustomBoxartCache::resetDirtyBit(bool isDirty)
    {
        sIsDirty = isDirty;
    }

    CustomBoxartCache::CustomBoxartCache()
    {
        sIsDirty = true;
    }

    CustomBoxartCache::~CustomBoxartCache()
    {

    }

    QString CustomBoxartCache::getCacheDirectory(Origin::Services::Session::SessionRef session)
    {
        QString cacheDir;
        if (!session.isNull())
        {
            QString environment = readSetting(SETTING_EnvironmentName);
            QString userId = Session::SessionService::nucleusUser(session);
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

            path << "CustomBoxartCache";
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

    QString CustomBoxartCache::getCacheFilePath(Origin::Services::Session::SessionRef session)
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

    bool CustomBoxartCache::saveCache(Origin::Services::Session::SessionRef session, const QMap<QString, Services::Entitlements::Parser::BoxartData>& productIdToBoxartMap)
    {
        bool result = false;

        QString cacheFilename = getCacheFilePath(session);

        customboxart::customizedT root;
        root.version = sCacheVersion;

        for (QMap<QString, Services::Entitlements::Parser::BoxartData>::const_iterator i = productIdToBoxartMap.constBegin(); i != productIdToBoxartMap.constEnd(); ++i)
        {
            customboxart::boxartT boxart;
            boxart.left = i.value().getBoxartDisplayArea().left();
            boxart.top = i.value().getBoxartDisplayArea().top();
            boxart.width = i.value().getBoxartDisplayArea().width();
            boxart.height = i.value().getBoxartDisplayArea().height();
            boxart.isDefault = i.value().usesDefaultBoxart();
            boxart.src = i.value().getBoxartSource();
			boxart.base = i.value().getBoxartBase();
            boxart.productId = i.key();

            root.boxart.push_back(boxart);
        }

        LSXWrapper res;
        customboxart::Write(&res, root);

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

    bool CustomBoxartCache::readCache(Origin::Services::Session::SessionRef session, QMap<QString, Services::Entitlements::Parser::BoxartData>& productIdToBoxartMap)
    {
        bool result = false;

        QString cacheFilePath = getCacheFilePath(session);
        QFile cacheFile(cacheFilePath);
        if (cacheFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QDomDocument doc;
            if (doc.setContent(&cacheFile))
            {
                QDomNodeList boxartList = doc.documentElement().elementsByTagName("boxart");
                for (int i = 0; i < boxartList.size(); ++i)
                {
                    customboxart::boxartT boxart;
                    QScopedPointer<INodeDocument> nogNode(CreateXmlDocument(doc, boxartList.at(i)));

                    if (customboxart::Read(nogNode.data(), boxart))
                    {
                        Services::Entitlements::Parser::BoxartData boxartData;

                        boxartData.setBoxartSource(boxart.src);
						boxartData.setBoxartBase(boxart.base);
                        boxartData.setUsesDefaultBoxart(boxart.isDefault);
                        boxartData.setBoxartDisplayArea(boxart.left, boxart.top,
                            boxart.width, boxart.height);
                        boxartData.setProductId(boxart.productId);

                        productIdToBoxartMap.insert(boxartData.getProductId(), boxartData);
                     }
                }

                result = true;
            }
        }
        
        return result;
    }

} // Cache
} // Entitlements
} // Services
} // Origin

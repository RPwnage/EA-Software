//    Copyright � 2011-2012, Electronic Arts
//    All rights reserved.
//    Author:

#include <limits>
#include <QDateTime>
#include <QFileInfo>
#include <QPixMap>
#include <QPainter>
#include <QIcon>
#include <QColor>

#include "engine/content/NonOriginContentController.h"
#include "engine/content/CustomBoxartController.h"

#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/GamesController.h"

#include "services/downloader/Common.h"
#include "services/publishing/CatalogDefinition.h"
#include "services/entitlements/cache/CustomBoxartCache.h"
#include "services/session/SessionService.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformService.h"

#include "TelemetryAPIDLL.h"

namespace Origin
{
namespace Engine
{
namespace Content
{
    NonOriginContentController::NonOriginContentController(ContentController* parent)
        : m_contentController(parent)
    {
    }

    NonOriginContentController::~NonOriginContentController()
    {
    }

    void NonOriginContentController::init()
    {
        migrateLegacyNogs();

        Engine::Content::Cache::NonOriginContentCache::instance()->readCache(Services::Session::SessionService::currentSession(), mProductIdToNogMap);

        foreach(const Engine::Content::NonOriginGameData& nog, mProductIdToNogMap.values()) 
        {
            createOwnedContentForNog(nog);
        }
    }

    Origin::Engine::Content::EntitlementRef NonOriginContentController::createOwnedContentForNog(const Engine::Content::NonOriginGameData& nog)
    {
        Origin::Services::Publishing::CatalogDefinitionRef fakeDefinition = Origin::Services::Publishing::CatalogDefinition::createEmptyDefinition();
        fakeDefinition->setDisplayName(nog.getDisplayName());
        fakeDefinition->setMasterTitleId(nog.getMasterTitleId());
        fakeDefinition->setContentId(nog.getContentId());
        fakeDefinition->setProductId(nog.getProductId());
        fakeDefinition->setOriginDisplayType(Services::Publishing::OriginDisplayTypeFullGame);
        fakeDefinition->setItemSubType(Services::Publishing::ItemSubTypeNonOrigin);

        Origin::Services::Publishing::SoftwareServerData& softwareData = fakeDefinition->softwareServerData(Origin::Services::PlatformService::runningPlatform());
        softwareData.setPlatformEnabled(true);
        softwareData.setExecutePath(nog.getExecutableFile());
        softwareData.setExecuteParameters(nog.getExecutableParameters());
        softwareData.setInstallCheck(nog.getExecutableFile());
        softwareData.setMonitorPlay(true);
        softwareData.setIgoPermission(nog.getIgoPermission());
        softwareData.setReleaseDate(QDateTime::fromString(QDATESTRING_PAST_DEFAULTVALUE, QDATESTRING_FORMAT));
        softwareData.setDownloadStartDate(QDateTime::fromString(QDATESTRING_PAST_DEFAULTVALUE, QDATESTRING_FORMAT));
        softwareData.setUseEndDate(QDateTime::fromString(QDATESTRING_FUTURE_DEFAULTVALUE, QDATESTRING_FORMAT));

        fakeDefinition->processPublishingOverrides();

        EntitlementRef newNog = Entitlement::create(fakeDefinition);
        m_nonOriginContent.insert(nog.getProductId(), newNog);
        emit nonOriginGameAdded(newNog);

        return newNog;
    }

    void NonOriginContentController::migrateLegacyNogs()
    {
        QVector<QByteArray> cache;
        if (Engine::Content::Cache::NonOriginContentCache::instance()->readLegacyCache(Services::Session::SessionService::currentSession(), cache))
        {
            QMap<QString, Engine::Content::NonOriginGameData> migratedNogs;

            for (QVector<QByteArray>::iterator i = cache.begin(); i != cache.end(); ++i)
            {
                QVariantMap cacheData;

                QDataStream stream(*i);
                stream.setVersion(QDataStream::Qt_4_8);
                stream >> cacheData;

                Engine::Content::NonOriginGameData migratedData;
                migratedData.setDisplayName(cacheData["displayName"].toString());
                migratedData.setExecutableFile(cacheData["executePath"].toString());
                migratedData.setExecutableParameters(cacheData["executeParameters"].toString());
                migratedData.setEntitleDate(cacheData["entitleDate"].toDateTime());
                migratedData.setIgoEnabled(cacheData["igoPermission"].toInt() == Services::Publishing::IGOPermissionSupported);

                migratedNogs.insert(migratedData.getProductId(), migratedData);

                CustomBoxartController *boxartController = m_contentController->boxartController();
                if (boxartController)
                {
                    Services::Entitlements::Parser::BoxartData boxartData;
                    boxartData.setProductId(migratedData.getProductId());
                    QString currentBoxartFilePath = cacheData["detailImage"].toString();
                    QFile currentBoxartFile(currentBoxartFilePath);
                    if (!currentBoxartFile.exists())
                    {
                        boxartData.setBoxartSource(boxartController->createExecutableIconBoxart(migratedData.getProductId(), migratedData.getExecutableFile()));
                        boxartData.setUsesDefaultBoxart(true);
                    }
                    else
                    {
                        boxartData.setUsesDefaultBoxart(currentBoxartFilePath.compare(getLegacyDefaultBoxartFilePath(migratedData.getExecutableFile()), Qt::CaseInsensitive) == 0);
                        boxartData.setBoxartSource(boxartController->saveBoxartSource(migratedData.getProductId(), currentBoxartFilePath));
                    }
                    boxartController->saveCustomBoxart(boxartData);
                }
                
            }

            if (Engine::Content::Cache::NonOriginContentCache::instance()->saveCache(Services::Session::SessionService::currentSession(), migratedNogs))
            {
                Engine::Content::Cache::NonOriginContentCache::instance()->removeLegacyCache(Services::Session::SessionService::currentSession());
            }
        }
    }

    void NonOriginContentController::addContent(QStringList executablePaths)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QStringList, executablePaths));

        for (QStringList::const_iterator i = executablePaths.constBegin(); i != executablePaths.constEnd(); ++i)
        {
            QString exePath(*i);

            if (!exePath.contains("://")/*steam:// ?*/ && !QFile::exists(exePath))
            {
                //TODO error
                continue;
            }

            Engine::Content::NonOriginGameData newData;
            newData.setExecutableFile(exePath);
            QString productId = newData.getProductId();
            if (!mProductIdToNogMap.contains(productId))
            {
                QFileInfo exeFileInfo(exePath);

                newData.setDisplayName(exeFileInfo.completeBaseName());
                newData.setIgoEnabled(true);
                newData.setEntitleDate(QDateTime::currentDateTimeUtc());
                mProductIdToNogMap.insert(productId, newData);

                Services::Entitlements::Parser::BoxartData boxartData;
                boxartData.setProductId(productId);
                boxartData.setBoxartSource(m_contentController->boxartController()->createExecutableIconBoxart(
                    productId, exePath));
                boxartData.setUsesDefaultBoxart(true);

                m_contentController->boxartController()->saveCustomBoxart(boxartData);

                createOwnedContentForNog(newData);

                GetTelemetryInterface()->Metric_NON_ORIGIN_GAME_ADD(exeFileInfo.fileName().toUtf8().data());
            }
        }

        Engine::Content::Cache::NonOriginContentCache::instance()->saveCache(Services::Session::SessionService::currentSession(), mProductIdToNogMap);
    }
    
    void NonOriginContentController::removeContentByProductId(const QString& productId)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QString, productId));

        if (m_nonOriginContent.contains(productId))
        {
            EntitlementRef removedNog = m_nonOriginContent.take(productId);
            emit nonOriginGameRemoved(removedNog);
        }

        if (mProductIdToNogMap.remove(productId) > 0)
        {
            Engine::Content::Cache::NonOriginContentCache::instance()->saveCache(Services::Session::SessionService::currentSession(), mProductIdToNogMap);
            m_contentController->boxartController()->removeCustomBoxart(productId);
        }
    }

    void  NonOriginContentController::removeContentByExecutePath(const QString& executePath)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QString, executePath));

        QString productId = getProductIdFromExecutable(executePath);
        removeContentByProductId(productId);
       
    }

    QSet<EntitlementRef> NonOriginContentController::nonOriginGames()
    {
        return m_nonOriginContent.values().toSet();
    }

    void NonOriginContentController::modifyContent(const QString& originalProductId, const Engine::Content::NonOriginGameData& gameData)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(Engine::Content::NonOriginGameData, gameData));

        QString productId = gameData.getProductId();
        
        if (mProductIdToNogMap.contains(originalProductId))
        {
            // If the product id changed (which it will if user modifies executable path), we have to remove it 
            if(originalProductId != productId)
            {
                mProductIdToNogMap.remove(originalProductId);

                // If there's custom box art we have to remove it as well
                m_contentController->boxartController()->removeCustomBoxart(originalProductId);
            }
        }
        EntitlementRef removedNog;

        if (m_nonOriginContent.contains(originalProductId))
        {
            removedNog = m_nonOriginContent.take(originalProductId);
            emit nonOriginGameRemoved(removedNog);
        }

        Engine::Content::NonOriginGameData modifiedData(gameData);
        mProductIdToNogMap.insert(productId, modifiedData);

        EntitlementRef newNog = createOwnedContentForNog(modifiedData);

        Engine::Content::Cache::NonOriginContentCache::instance()->saveCache(
            Services::Session::SessionService::currentSession(), mProductIdToNogMap);

        emit nonOriginGameModified(removedNog, newNog);
    }

    void NonOriginContentController::scan()
    {
        ASYNC_INVOKE_GUARD;
    }

    QList<ScannedContent> NonOriginContentController::getScannedContent() const
    {
        return mScannedContent;
    }

    QString NonOriginContentController::getLegacyDefaultBoxartFilePath(const QString& executablePath)
    {
        QString defaultBoxartFilePath;
        QString id = getLegacyBoxartId(executablePath);
        if (!id.isEmpty())
        {
            defaultBoxartFilePath = buildDefaultBoxartFilePath(id);
        }

        return defaultBoxartFilePath;
    }

    QString NonOriginContentController::buildDefaultBoxartFilePath(const QString& id, const QString& fileExtension /*= "png"*/)
    {
        QString path(Engine::Content::Cache::NonOriginContentCache::instance()->getCacheDirectory(Services::Session::SessionService::currentSession()));
        path.replace("\\", "/");
        if (!path.endsWith("/"))
        {
            path.append("/");
        }
        path.append(QString("%1_boxart.%2").arg(id).arg(fileExtension));

        return path;
        
    }

    QString NonOriginContentController::getLegacyBoxartId(const QString& executablePath)
    {
        QString id;
        QFileInfo fileInfo(executablePath);

        if (fileInfo.exists() || fileInfo.filePath().contains("://") /*steam:// ?*/)
        {
            id = QString("%1").arg(fileInfo.completeBaseName());
            id.replace(" ", "_");
            id.replace("'", "_");
        }

        return id;
    }

    bool NonOriginContentController::wasAdded(const QString& executablePath) const
    {
        QString productId = getProductIdFromExecutable(executablePath);
        return mProductIdToNogMap.contains(productId);
    }


    bool NonOriginContentController::hasCacheChanged() const
    {
        return Engine::Content::Cache::NonOriginContentCache::isDirty();
    }

    QString NonOriginContentController::getProductIdFromExecutable(const QString& executable)
    {
        return QString("NOG_%1").arg(qHash(executable));
    }

    bool NonOriginContentController::getProperties(const QString& productId, Engine::Content::NonOriginGameData& nog)
    {
        bool result = false;
        QMap<QString, Engine::Content::NonOriginGameData>::const_iterator i = mProductIdToNogMap.find(productId);
        if (i != mProductIdToNogMap.constEnd())
        {
            nog = i.value();
            result = true;
        }

        return result;
    }

} //namespace Content
} //namespace Engine
}//namespace Origin


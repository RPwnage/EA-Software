//    Copyright � 2011-2012, Electronic Arts
//    All rights reserved.
//    Author:

#include "engine/content/CustomBoxartController.h"
#include "engine/content/ContentController.h"
#include "engine/content/ProductArt.h"

#include "services/downloader/Common.h"
#include "services/session/SessionService.h"
#include "services/log/LogService.h"
#include "services/entitlements/cache/CustomBoxartCache.h"
#include "services/network/GlobalConnectionStateMonitor.h"

#include <QFileInfo>
#include <QPixMap>
#include <QPainter>
#include <QIcon>
#include <QColor>

namespace Origin
{
namespace Engine
{
namespace Content
{   
    const QString CustomBoxartController::sDefaultBoxartBackground = ":/compiledWidgets/mygames/mygames/images/card/default-non-origin-boxart.png";
    const int CustomBoxartController::sBoxartWidth = 231;
    const int CustomBoxartController::sBoxartHeight = 326;
	const QString CustomBoxartController::NEW_IMAGE_TAG = "ORIGIN_NEW";
    bool CustomBoxartController::sCurrentlyManipulatingCache = false;

    CustomBoxartController::CustomBoxartController(ContentController* parent)
    {
        ORIGIN_VERIFY_CONNECT(parent, SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)),
            this, SLOT(onContentUpdated(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)));

        Services::Entitlements::Cache::CustomBoxartCache::instance()->readCache(Services::Session::SessionService::currentSession(), mProductIdToBoxartMap);

        ORIGIN_VERIFY_CONNECT(&mCacheDirectoryChangeDetector, SIGNAL(directoryChanged(const QString&)), this, SLOT(onCacheDirectoryChanged(const QString&)));
        mCacheDirectoryChangeDetector.addPath(Services::Entitlements::Cache::CustomBoxartCache::instance()->getCacheDirectory(Services::Session::SessionService::currentSession()));
    }

    CustomBoxartController::~CustomBoxartController()
    {

    }
    
    void CustomBoxartController::removeCustomBoxart(const QString& productId)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QString, productId));

        Services::Entitlements::Parser::BoxartData boxartData;
        if (this->getBoxartData(productId, boxartData))
        {   
            sCurrentlyManipulatingCache = true;

            QFile::remove(boxartData.getBoxartSource());
            
            mProductIdToBoxartMap.remove(productId);
            Services::Entitlements::Cache::CustomBoxartCache::instance()->saveCache(Services::Session::SessionService::currentSession(), mProductIdToBoxartMap);

            sCurrentlyManipulatingCache = false;
        }

        emit boxartChanged(productId);
        emit removeComplete();
    }

    void CustomBoxartController::saveCustomBoxart(const Services::Entitlements::Parser::BoxartData& boxartData)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(Services::Entitlements::Parser::BoxartData, boxartData));
        
        QString productId = boxartData.getProductId();
        mProductIdToBoxartMap.insert(productId, boxartData);

        sCurrentlyManipulatingCache = true;
        Services::Entitlements::Cache::CustomBoxartCache::instance()->saveCache(
                Services::Session::SessionService::currentSession(), mProductIdToBoxartMap);
        sCurrentlyManipulatingCache = false;

        emit boxartChanged(productId);
        emit saveComplete();
    }

    void CustomBoxartController::onCacheDirectoryChanged(const QString& dir)
    {
        Q_UNUSED(dir);
        
        // Don't react to changes caused by client..
        if(sCurrentlyManipulatingCache)
        {
            return;
        }

        // Currently we only check to see if existing box art was removed...
        QList<QString> orphanedProductBoxarts;
        QMap<QString, Origin::Services::Entitlements::Parser::BoxartData>::const_iterator iter = mProductIdToBoxartMap.constBegin();
        while(iter != mProductIdToBoxartMap.constEnd())
        {
            if(!QFile::exists(iter.value().getBoxartSource()))
            {
                ORIGIN_LOG_EVENT << "Found orphaned custom box art configuration for product [" << iter.key() << "].  Reverting to default box art.";
                orphanedProductBoxarts.push_back(iter.key());
            }
            iter++;
        }

        clearOrphanedBoxartConfiguration(orphanedProductBoxarts);

        foreach(QString productId, orphanedProductBoxarts)
        {
            emit boxartChanged(productId);
        }
    }

    QString CustomBoxartController::buildDefaultBoxartFilePath(const QString& id, const QString& fileExtension /*= "png"*/, const bool isBasefile /*= false*/)
    {
        QString filename(id);
        filename.replace(":", "_");

        QString path = getBoxartCachePath();
		if (isBasefile)
	        path.append(QString("%1_base_boxart.%2").arg(filename).arg(fileExtension));
		else
			path.append(QString("%1_boxart.%2").arg(filename).arg(fileExtension));
        return path;
        
    }

	QString CustomBoxartController::getBoxartCachePath()
	{
		QString path(Services::Entitlements::Cache::CustomBoxartCache::instance()->getCacheDirectory(Services::Session::SessionService::currentSession()));
		path.replace("\\", "/");
        if (!path.endsWith("/"))
        {
            path.append("/");
        }
		return path;
	}

    bool CustomBoxartController::createBoxart(const QPixmap& innerImage, QPixmap& boxart)
    {
        if (!boxart.load(sDefaultBoxartBackground, "PNG"))
        {
            ORIGIN_LOG_WARNING << "Failed to load resource: " << sDefaultBoxartBackground;
            return false;
        }

        if (!innerImage.isNull())
        {
            if (innerImage.width() <= boxart.width() && innerImage.height() <= boxart.height())
            {
                int startX = (boxart.width() - innerImage.width()) / 2;
                int startY = (boxart.height() - innerImage.height()) / 2;
                
                QPainter painter;
                painter.begin(&boxart);
                painter.drawPixmap(startX, startY, innerImage);
                painter.end();

                // The background image isn't the correct size
                boxart = boxart.scaled(QSize(sBoxartWidth, sBoxartHeight));
                
                return true;
            } 
        }

        return false;
    }

    QPixmap CustomBoxartController::createDefaultBoxartImage(Engine::Content::EntitlementRef eRef)
    {
        QPixmap boxart;
        if (!eRef.isNull())
        {
            if (eRef->contentConfiguration()->itemSubType() != Services::Publishing::ItemSubTypeNonOrigin)
            {
                boxart.load(":/compiledWidgets/mygames/mygames/images/card/default-boxart.jpg", "JPG");
            }
            else
            {
                QIcon icon = Services::PlatformService::iconForFile(eRef->contentConfiguration()->executePath());
                if(!icon.isNull())
                {
                    QPixmap iconPixmap;
                    if (!boxart.load(sDefaultBoxartBackground, "PNG"))
                    {
                        ORIGIN_LOG_WARNING << "Failed to load resource: " << sDefaultBoxartBackground;
                        return QPixmap();
                    }

                    int desiredWidth = boxart.width()*0.75;
                    QSize iconSize(desiredWidth, desiredWidth);          
                    iconPixmap = icon.pixmap(iconSize);
            
                    if(!iconPixmap.isNull())
                    {
                        int width = iconPixmap.width();
                        if (width < desiredWidth && 2 * width < desiredWidth)
                        {
                            iconPixmap = iconPixmap.scaled(QSize(2 * width, 2 * width), Qt::KeepAspectRatio);
                        }

                        createBoxart(iconPixmap, boxart);
                    }
                }
            }
        }

        return boxart;
    }

    QString CustomBoxartController::saveBoxartSource(const QString& productId, const QPixmap& pixmap)
    {
        QString boxartCacheFilename;

        if (!productId.isEmpty() && !pixmap.isNull())
        {
            QString currentBoxartFilename;
            Services::Entitlements::Parser::BoxartData boxartData;
            if (getBoxartData(productId, boxartData))
            {
                currentBoxartFilename = boxartData.getBoxartSource();
            }

            sCurrentlyManipulatingCache = true;

            QString cachePath = buildDefaultBoxartFilePath(productId);
            QFile currentBoxartFile(currentBoxartFilename);
            if (currentBoxartFile.exists())
            {
                currentBoxartFile.remove();
            }

            if (pixmap.save(cachePath, "PNG"))
            {
                boxartCacheFilename = cachePath;
            }

            sCurrentlyManipulatingCache = false;
        }

        return boxartCacheFilename;
    }

    QString CustomBoxartController::saveBoxartSource(const QString& productId, const QString& newBoxartFilename)
    {
		return saveBoxartFile(productId, newBoxartFilename, false);
    }

	    QString CustomBoxartController::saveBoxartBase(const QString& productId, const QString& newBoxartFilename)
    {
		return saveBoxartFile(productId, newBoxartFilename, true);
    }

	QString CustomBoxartController::saveBoxartFile(const QString& productId, const QString& newBoxartFilename, const bool isBaseFile)
	{
		QString boxartCacheFilename; 
        QFile newBoxartFile(newBoxartFilename);
        
        if (!productId.isEmpty() && newBoxartFile.exists())
        {
            QString currentBoxartFilename;
            Services::Entitlements::Parser::BoxartData boxartData;
            if (getBoxartData(productId, boxartData))
            {
				if(isBaseFile)
	                currentBoxartFilename = boxartData.getBoxartBase();
				else
					currentBoxartFilename = boxartData.getBoxartSource();
            }

            QFileInfo newFileInfo(newBoxartFilename);
            QString cachePath = buildDefaultBoxartFilePath(productId, newFileInfo.suffix(), isBaseFile);
            bool isSameFile = newBoxartFilename.compare(cachePath, Qt::CaseInsensitive) == 0;

            QFile currentBoxartFile(currentBoxartFilename);            
            bool cacheFileExists = currentBoxartFile.exists();

            if (cacheFileExists && isSameFile)
            {
                return newBoxartFilename;
            }

            sCurrentlyManipulatingCache = true;

            if (cacheFileExists)
            {
                currentBoxartFile.remove();
            }

            if (newBoxartFile.copy(cachePath))
            {
                boxartCacheFilename = cachePath;
            }

			if (newBoxartFilename.contains(NEW_IMAGE_TAG))
			{
				// clean up after ourseves and remove the "newly created cropped box art file.
				newBoxartFile.remove();
			}

            sCurrentlyManipulatingCache = false;
		}

        return boxartCacheFilename;
	}

    QString CustomBoxartController::createExecutableIconBoxart(const QString& productId, const QString& executePath)
    {
        QString boxartFilePath;
        if (!productId.isEmpty() && !executePath.isEmpty())
        {
            QIcon icon = Services::PlatformService::iconForFile(executePath);
            if(!icon.isNull())
            {
                QPixmap boxart;
                QPixmap iconPixmap;
                if (!boxart.load(sDefaultBoxartBackground, "PNG"))
                {
                    ORIGIN_LOG_WARNING << "Failed to load resource: " << sDefaultBoxartBackground;
                    return QString();
                }

                int desiredWidth = boxart.width()*0.75;
                QSize iconSize(desiredWidth, desiredWidth);          
                iconPixmap = icon.pixmap(iconSize);
            
                if(!iconPixmap.isNull())
                {
                    int width = iconPixmap.width();
                    if (width < desiredWidth && 2 * width < desiredWidth)
                    {
                        iconPixmap = iconPixmap.scaled(QSize(2 * width, 2 * width), Qt::KeepAspectRatio);
                    }

                    createBoxart(iconPixmap, boxart);

                    sCurrentlyManipulatingCache = true;

                    boxartFilePath = buildDefaultBoxartFilePath(productId);
                    if (!boxart.save(boxartFilePath, "PNG"))
                    {
                        ORIGIN_LOG_WARNING << "Failed to save boxart to path: " << boxartFilePath;
                        boxartFilePath.clear();
                    }        

                    sCurrentlyManipulatingCache = false;
                }
            }
            else
            {
                ORIGIN_LOG_WARNING << "Failed to extract icon for executable: " << executePath;
            }
        }

        return boxartFilePath;
    }

    QString CustomBoxartController::createDefaultBoxart(Engine::Content::EntitlementRef eRef)
    {  
        QString boxartFilePath;
        if (!eRef.isNull())
        {
            if (eRef->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeNonOrigin)
            {
                boxartFilePath = createExecutableIconBoxart(eRef->contentConfiguration()->productId(), eRef->contentConfiguration()->executePath());
            }
        }
  
        return boxartFilePath;
    }

    void CustomBoxartController::onContentUpdated(const QList<Origin::Engine::Content::EntitlementRef> added, const QList<Origin::Engine::Content::EntitlementRef> removed)
    {
        Services::Entitlements::Cache::CustomBoxartCache::resetDirtyBit(false);
    }

    bool CustomBoxartController::hasCacheChanged() const
    {
        return Services::Entitlements::Cache::CustomBoxartCache::isDirty();
    }

    void CustomBoxartController::clearOrphanedBoxartConfiguration(const QList<QString>& productIds)
    {
        Services::Entitlements::Parser::BoxartData boxartData;
        foreach(QString productId, productIds)
        {
            mProductIdToBoxartMap.remove(productId);
        }

        sCurrentlyManipulatingCache = true;
        Services::Entitlements::Cache::CustomBoxartCache::instance()->saveCache(Services::Session::SessionService::currentSession(), mProductIdToBoxartMap);
        sCurrentlyManipulatingCache = false;
    }

    bool CustomBoxartController::getBoxartData(const QString& productId, Services::Entitlements::Parser::BoxartData& boxartData)
    {
        bool result = false;
        QMap<QString, Services::Entitlements::Parser::BoxartData>::const_iterator i = mProductIdToBoxartMap.find(productId);
        if (i != mProductIdToBoxartMap.constEnd())
        {
            boxartData = i.value();
            result = true;
        }

        return result;
    }

} //namespace Content
} //namespace Engine
}//namespace Origin


//    Copyright � 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: 

#ifndef CUSTOMBOXARTCONTROLLER_H
#define CUSTOMBOXARTCONTROLLER_H

#include "engine/content/Entitlement.h"
#include "services/entitlements/BoxartData.h"
#include "services/plugin/PluginAPI.h"

#include <QObject>
#include <QString>
#include <QFileSystemWatcher>

namespace Origin 
{
namespace Engine
{
namespace Content
{
        class ContentController;

        class ORIGIN_PLUGIN_API CustomBoxartController : public QObject
        {
            Q_OBJECT

            public:

                static const QString sDefaultBoxartBackground;
                static const int sBoxartWidth;
                static const int sBoxartHeight;
                static const QString NEW_IMAGE_TAG;

                static bool createBoxart(const QPixmap& innerImage, QPixmap& boxart);

                static QString createDefaultBoxart(Engine::Content::EntitlementRef eRef);
                static QString createExecutableIconBoxart(const QString& productId, const QString& executePath);

                static QPixmap createDefaultBoxartImage(Engine::Content::EntitlementRef eRef);

                QString saveBoxartSource(const QString& productId, const QPixmap& pixmap);
                QString saveBoxartSource(const QString& productId, const QString& newBoxartFile);

                QString saveBoxartBase(const QString& productId, const QString& newBoxartFile);
                QString saveBoxartFile(const QString& productId, const QString& newBoxartFilename, const bool isBaseFile);
                static QString getBoxartCachePath();

                CustomBoxartController(ContentController* parent);
                ~CustomBoxartController();

                Q_INVOKABLE void saveCustomBoxart(const Services::Entitlements::Parser::BoxartData& boxartData);
                Q_INVOKABLE void removeCustomBoxart(const QString& productId);

                bool hasCacheChanged() const;

                bool getBoxartData(const QString& productId, Services::Entitlements::Parser::BoxartData& boxartData);
                
            signals:

                void saveComplete();
                void removeComplete();
                void boxartChanged(const QString& productId);

            private slots:
                void onCacheDirectoryChanged(const QString& path);
                void onContentUpdated(const QList<Origin::Engine::Content::EntitlementRef> added, const QList<Origin::Engine::Content::EntitlementRef> removed);

            private:
                void clearOrphanedBoxartConfiguration(const QList<QString>& productIds);

                QFileSystemWatcher mCacheDirectoryChangeDetector;
                
                static QString buildDefaultBoxartFilePath(const QString& id, const QString& fileExtension = "png", const bool isBasefile = false);
                QMap<QString, Services::Entitlements::Parser::BoxartData> mProductIdToBoxartMap;
                static bool sCurrentlyManipulatingCache;
        };

} //Content        
} //Engine
} //Origin

#endif // NONORIGINCONTENTCONTROLLER_H

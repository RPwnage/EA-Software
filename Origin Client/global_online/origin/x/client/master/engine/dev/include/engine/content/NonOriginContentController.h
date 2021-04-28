//    Copyright � 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: 

#ifndef NONORIGINCONTENTCONTROLLER_H
#define NONORIGINCONTENTCONTROLLER_H

#include "engine/content/Entitlement.h"

#include "engine/content/NonOriginGameData.h"
#include "engine/content/NonOriginContentCache.h"

#include "services/publishing/CatalogDefinition.h"
#include "services/plugin/PluginAPI.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

namespace Origin 
{

namespace Engine
{
namespace Content
{
        struct ScannedContent
        {
            QString displayName;
            QString iconFilename;
            QString executablePath;
        };

        class ContentController;
        class CustomBoxartController;

        class ORIGIN_PLUGIN_API NonOriginContentController : public QObject
        {
            Q_OBJECT

            public:

                static QString getLegacyDefaultBoxartFilePath(const QString& executablePath);
                static QString getLegacyBoxartId(const QString& executablePath);

                NonOriginContentController(ContentController* parent);
                ~NonOriginContentController();

                void init();

                QList<ScannedContent> getScannedContent() const;

                Q_INVOKABLE void addContent(QStringList executablePaths);
                Q_INVOKABLE void removeContentByProductId(const QString& productId);
                Q_INVOKABLE void removeContentByExecutePath(const QString& executePath);
                Q_INVOKABLE void modifyContent(const QString& originalProductId, const Engine::Content::NonOriginGameData& gameData);
                Q_INVOKABLE void scan();

                bool wasAdded(const QString& executablePath) const;
                bool hasCacheChanged() const;

                static QString getProductIdFromExecutable(const QString& executable);
                bool getProperties(const QString& productId, Engine::Content::NonOriginGameData& nog);

                QSet<EntitlementRef> nonOriginGames();

            signals:
                void scanComplete();

                void nonOriginGameModified(Origin::Engine::Content::EntitlementRef originalNog, Origin::Engine::Content::EntitlementRef modifiedNog);
                void nonOriginGameAdded(Origin::Engine::Content::EntitlementRef);
                void nonOriginGameRemoved(Origin::Engine::Content::EntitlementRef); 

            private:
                QMap<QString, Origin::Engine::Content::EntitlementRef> m_nonOriginContent;
                ContentController *m_contentController;

                Origin::Engine::Content::EntitlementRef createOwnedContentForNog(const Engine::Content::NonOriginGameData& nog);

                static QString buildDefaultBoxartFilePath(const QString& id, const QString& fileExtension = "png");
                void migrateLegacyNogs();

                QList<ScannedContent> mScannedContent;

                QMap<QString, Engine::Content::NonOriginGameData> mProductIdToNogMap;
        };

} //Content
} //Engine
} //Origin

#endif // NONORIGINCONTENTCONTROLLER_H

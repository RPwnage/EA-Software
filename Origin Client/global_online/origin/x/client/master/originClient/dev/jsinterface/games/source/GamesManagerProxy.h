#ifndef _GAMESMANAGER_PROXY_H_
#define _GAMESMANAGER_PROXY_H_

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QSharedPointer>
#include "engine/content/Entitlement.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            class GameProxy;

            class GamesManagerProxy : public QObject
            {
                Q_OBJECT
            public:
                // The web widget framework destructs us so we need a public destructor
                // instead of a static destroy()
                GamesManagerProxy(QObject *parent);
                ~GamesManagerProxy();

                Q_PROPERTY(bool initialRefreshCompleted READ initialRefreshCompleted);
                bool initialRefreshCompleted();

                Q_INVOKABLE QVariant games(const QString &productId);
                Q_INVOKABLE QVariantList requestGamesStatus();
                Q_INVOKABLE QVariantMap onRemoteGameAction(QString productId, QString action, QVariantMap actionParams);
                Q_INVOKABLE QVariantList getNonOriginGames();

                Q_PROPERTY(bool isGamePlaying READ isGamePlaying);
                bool isGamePlaying();

            signals:
                void basegamesupdated(QVariantMap);
                void changed(QVariantList);
                void progressChanged(QVariantList);
                void operationFailed(QVariantList);
                void downloadQueueChanged(QVariantList);
            private:
                GameProxy* gameProxyForEntitlement(Origin::Engine::Content::EntitlementRef);
                QHash<Engine::Content::Entitlement*, GameProxy*> mGameProxies;
                
            private slots:
                void onGameProxyChanged(QJsonObject data);
                void onGameProxyProgressChanged(QJsonObject data);
                void onGameProxyOperationFailed(QJsonObject data);
                void onDownloadQueueInitialized();
                void onDownloadQueueChanged ();
                void onDownloadQueueChangedEnqueued(Origin::Engine::Content::EntitlementRef);
                void onDownloadQueueChangedRemoved(const QString&, const bool&, const bool&);
                void onDownloadQueueChangedHeadChanged(Origin::Engine::Content::EntitlementRef, Origin::Engine::Content::EntitlementRef);
                void onDownloadQueueChangedHeadBusy(const bool& busy);
                void onNogUpdated();
            };

        }
    }
}

#endif

/////////////////////////////////////////////////////////////////////////////
// GameProxy.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef _GameProxy_H
#define _GameProxy_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QString>
#include <QVariant>

#include "engine/content/ContentConfiguration.h"
#include "engine/content/Entitlement.h"
#include "engine/content/LocalContent.h"

namespace Origin
{
namespace Client
{

namespace JsInterface
{

class EntitlementInstallFlowProxy;
class EntitlementCloudSaveProxy;
class NonOriginGameProxy;
class BoxartProxy;

class GameProxy : public QObject
{
    friend class GamesManagerProxy;
    Q_OBJECT
    public:
        GameProxy(Origin::Engine::Content::EntitlementRef, QObject *parent);

        //Actions from JS
        Q_INVOKABLE QVariantMap play(QVariantMap args);

        Q_INVOKABLE QVariantMap startDownload(QVariantMap args);
        Q_INVOKABLE QVariantMap pauseDownload(QVariantMap args);
        Q_INVOKABLE QVariantMap resumeDownload(QVariantMap args);
        Q_INVOKABLE QVariantMap cancelDownload(QVariantMap args);

        Q_INVOKABLE QVariantMap checkForUpdateAndInstall(QVariantMap args);
        Q_INVOKABLE QVariantMap installUpdate(QVariantMap args);
        Q_INVOKABLE QVariantMap pauseUpdate(QVariantMap args);
        Q_INVOKABLE QVariantMap resumeUpdate(QVariantMap args);
        Q_INVOKABLE QVariantMap cancelUpdate(QVariantMap args);

        Q_INVOKABLE QVariantMap repair(QVariantMap args);
        Q_INVOKABLE QVariantMap pauseRepair(QVariantMap args);
        Q_INVOKABLE QVariantMap resumeRepair(QVariantMap args);
        Q_INVOKABLE QVariantMap cancelRepair(QVariantMap args);

        Q_INVOKABLE QVariantMap install(QVariantMap args);
        Q_INVOKABLE QVariantMap cancelInstall(QVariantMap args);
        Q_INVOKABLE QVariantMap pauseInstall(QVariantMap args);
        Q_INVOKABLE QVariantMap resumeInstall(QVariantMap args);

        Q_INVOKABLE QVariantMap uninstall(QVariantMap args);
        Q_INVOKABLE QVariantMap cancelCloudSync(QVariantMap args);
        Q_INVOKABLE QVariantMap pollCurrentCloudUsage(QVariantMap args);
        Q_INVOKABLE QVariantMap restoreLastCloudBackup(QVariantMap args);
        Q_INVOKABLE QVariantMap removeFromLibrary(QVariantMap args);
        Q_INVOKABLE QVariantMap installParent(QVariantMap args);
        Q_INVOKABLE QVariantMap customizeBoxArt(QVariantMap args);

        Q_INVOKABLE QVariantMap moveToTopOfQueue(QVariantMap args);

        Q_INVOKABLE QVariantMap getCustomBoxArtInfo(QVariantMap args);
        Q_INVOKABLE QVariantMap getCustomBoxArtSize(QVariantMap args);
    signals:
        void changed(QJsonObject);
        void progressChanged(QJsonObject);
        void operationError(QJsonObject);

    private:
        const QJsonObject createStateObj();
        const QJsonObject createProgressStateObj();
        const QJsonObject createDownloadQueueObj();
        QString releaseStatus();
        bool repairSupported();
        bool updateSupported();
        bool isITOActive();

        QString pauseCurrentOperation(EntitlementInstallFlowProxy *flow);
        QString resumeCurrentOperation(EntitlementInstallFlowProxy *flow);
        QString cancelCurrentOperation(EntitlementInstallFlowProxy *flow);

        EntitlementInstallFlowProxy * downloadOperation();
        EntitlementInstallFlowProxy * updateOperation();
        EntitlementInstallFlowProxy * unpackOperation();
        EntitlementInstallFlowProxy * repairOperation();
        EntitlementInstallFlowProxy * currentOperation();
        EntitlementInstallFlowProxy * installOperation();

        Origin::Engine::Content::EntitlementRef mEntitlement;
        EntitlementCloudSaveProxy *mCloudSaves;
        EntitlementInstallFlowProxy *mInstallFlowProxy;
        NonOriginGameProxy *mNonOriginGame;
        BoxartProxy *mBoxart;

    private slots:
        void onProgressChanged();
        void onStateChanged();
        void onOperationError(qint32 type, qint32 code, QString context, QString productId);
        void onUpdateCheckCompletedForRepair();
        void onUpdateCheckCompletedForInstall();
};

}
}
}

#endif

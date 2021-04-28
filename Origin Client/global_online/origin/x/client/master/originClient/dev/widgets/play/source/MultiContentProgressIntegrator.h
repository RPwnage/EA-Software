/////////////////////////////////////////////////////////////////////////////
// MultiContentProgressIntegrator.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef MultiContentProgressIntegrator_H
#define MultiContentProgressIntegrator_H

#include <QPair>
#include <QMap>
#include <QWidget>
#include "engine/content/Entitlement.h"
#include "engine/content/ContentTypes.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "engine/downloader/ContentInstallFlowContainers.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginTransferBar;
    }
    namespace Client
    {
        /**
         * This class integrates the download progress information from one or more concurrently downloading
         * PDLC entitlements and updates a progress bar with the comboned progress.
         */
        class ORIGIN_PLUGIN_API MultiContentProgressIntegrator : public QObject
        {
            Q_OBJECT
        public:
            /// Manages the operation information for update and download in the play flow
            MultiContentProgressIntegrator(QList<Origin::Engine::Content::EntitlementRef> entitlementList,const QString& id, QObject* parent = NULL, const bool& inDebug = false);
            QJsonObject progressObj(const QString& title, const QString& operation, const QString& operationStatus, const QString& percent, const QString& rate, const QString& timeRemaining, const QString& phase, const QString& completedPercent);

        signals:
            /// Emitted when all downloading PDLCs are done downloading
            void PDLCDownloadsComplete(const QString& overrideId);
            void updateValues(const QString& id, const QJsonObject& updateContent);
            
        public slots:
            /// Notify of changes in state in any of the PDLC contents being tracked by this widget
            void onStateChanged(const Origin::Downloader::ContentInstallFlowState& flowState);


        private slots:
            void onDebugTimeout();
        
        private:
            Downloader::ContentInstallFlowState possibleMaskInstallState(Origin::Engine::Content::EntitlementRef entitlement);
            /// The list of entitlements that are currently in progress
            QList<Origin::Engine::Content::EntitlementRef> mEntitlementsInProgress;
            const QString mId;
            const bool mInDebug;
        };
    }
}

#endif // MultiContentProgressIntegrator_H

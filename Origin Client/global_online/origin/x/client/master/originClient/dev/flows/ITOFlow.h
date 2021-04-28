///////////////////////////////////////////////////////////////////////////////
// ITOFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ITOFLOW_H
#define ITOFLOW_H

#include "flows/AbstractFlow.h"
#include "engine/content/Entitlement.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "services/plugin/PluginAPI.h"
#include <QNetworkCookie>
#include <QTimer>

namespace Origin
{
	namespace Client
	{
        /// \brief Handles all high-level actions related to ITO.
		class ORIGIN_PLUGIN_API ITOFlow : public AbstractFlow
		{
			Q_OBJECT

		public:
            enum CompleteDialogTypes
            {
                Success,
                InstallFailed,
                PatchFailed,
                FailedNoGameName,
                Canceled,
                FreeToPlayNoEntitlement,
                SysReqFailed,
                DLCSysReqFailed
            };

            /// \brief Creates a new ITOFlow object.
            static void create();

            /// \brief Destroys the current ITOFlow object if any.
            static void destroy();

            /// \brief Gets the current ITOFlow object.
            /// \return The current ITOFlow object.
            static ITOFlow* instance();

            /// \brief The ITOFlow constructor.
			ITOFlow();

            /// \brief Public interface for starting the ITO flow.
			virtual void start();

            /// \brief Checks if the given error code is a disc read error.
            /// \param errorType The error code to check.
            /// \return True if the given error is a disc read error.
            static bool isReadDiscErrorType (qint32 errorType);

            /// \brief Reads in ITO Game Display title from manifest on disc from the command line.
            /// \return string of game display name.
			static QString getGameTitleFromManifest();

            void requestFreeGameEntitlement();

            void setFreeEntitlementFail();

		protected:
            /// \brief Starts the ITO flow.
			void startITO();

            QNetworkReply *mFreeGameEntitlementReply;

            QTimer *mFreeGameRequestTimer;
            
            void initiateFreeGamesRequest(const QUrl &url, QNetworkReply **reply);

		protected slots:
            /// \brief Slot that is triggered the first time the user's entitlements are updated.
            void onEntitlementsUpdatedForITOStart();
            
            /// \brief Slot that is triggered each time the user's entitlements are updated.
            /// \param newlyAdded List of entitlements that have been added to the user's entitlements.
            /// \param newlyRemoved List of entitlements that have been removed from the user's entitlements.
            void onCurrentUserEntitlementsUpdated(const QList<Origin::Engine::Content::EntitlementRef> newlyAdded, const QList<Origin::Engine::Content::EntitlementRef> newlyRemoved);

            /// \brief Slot that is triggered each time the user's entitlements fail to update.
            void onCurrentUserEntitlementsUpdateFailed();

            /// \brief Slot that is triggered when the user starts an ITO install.
            void onStartITOviaCommandLine();

            /// \brief Slot that is triggered when the install flow changes state.
            /// \param newState The new state.
            void onEntitlementStateChanged(Origin::Downloader::ContentInstallFlowState newState);

            /// \brief Slot that is triggered when the mygames view is loaded
            void onMyGamesLoaded();

            void freeGameEntitlementResponseReceived();

            void onFreeGameRequestTimeout();
		};
	}
}

#endif // ITOFLOW_H

#ifndef PENDING_ACTION_GAME_DOWNLOAD_VIEW_CONTROLLER_H
#define PENDING_ACTION_GAME_DOWNLOAD_VIEW_CONTROLLER_H

#include <QObject>
#include <QUrlQuery>
#include "PendingActionBaseViewController.h"
#include "engine/content/ContentController.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API PendingActionGameDownloadViewController : public PendingActionBaseViewController
        {
            Q_OBJECT

            public:
                PendingActionGameDownloadViewController(const QUrlQuery &queryParams, const bool launchedFromBrowser, const QString &uuid, const QString &lookupId);
                virtual ~PendingActionGameDownloadViewController(){};

                /// \brief kicks off a refresh if needed
                void performMainAction();

                /// \brief checks for a valid clientPageParam
                bool parseAndValidateParams();

            private:                

                /// \brief holds the profile id
                QString mProfileId;

                /// \brief an enum which holds a EntitlementLookupType designating how were are retrieving the entitlement info
                Engine::Content::ContentController::EntitlementLookupType mEntitlementLookupId;

                /// \brief entitlement id
                QString mEntitlementId;

                /// \brief finds the entitlement we intend to show in teh games detail page
                Origin::Engine::Content::EntitlementRef findEntitlementToShow(QString ids, Engine::Content::ContentController::EntitlementLookupType lookup);

                /// \brief try to move to the front of the download queue
                bool mFrontOfQueue;
        };
    }
}

#endif
#ifndef PENDING_ACTION_LIBRARY_OPEN_VIEW_CONTROLLER_H
#define PENDING_ACTION_LIBRARY_OPEN_VIEW_CONTROLLER_H

#include <QObject>
#include <QUrlQuery>
#include "PendingActionBaseViewController.h"
#include "engine/content/ContentTypes.h"
#include "engine/content/ContentController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API PendingActionLibraryOpenViewController : public PendingActionBaseViewController
        {
            Q_OBJECT

            public:
                PendingActionLibraryOpenViewController(const QUrlQuery &queryParams, const bool launchedFromBrowser, const QString &uuid, const QString &lookupId);
                virtual ~PendingActionLibraryOpenViewController(){};

                /// \brief shows the approriate main page and kicks off a refresh if needed
                void performMainAction();

                /// \brief returns PendingAction::StartupTabMyGamesId ("mygames")
                QString startupTab();

                /// \brief checks for a valid clientPageParam
                bool parseAndValidateParams();

                /// \brief send out telemetry
                void sendTelemetry();

            private:                

                /// \brief holds the string id of the client page to show
                QString mClientPage;

                /// \brief holds the profile id
                QString mProfileId;

                /// \brief an enum which holds a EntitlementLookupType designating how were are retrieving the entitlement info
                Engine::Content::ContentController::EntitlementLookupType mEntitlementLookupId;
                
                /// \brief a list of ids used to determine which entitlement to show the details for
                QString mEntitlementIds;

                /// \brief shows the client appropriate client page
                void (PendingActionLibraryOpenViewController::*mShowClientPage)();

                /// \brief finds the entitlement we intend to show in teh games detail page
                Origin::Engine::Content::EntitlementRef findEntitlementToShow(QString ids, Engine::Content::ContentController::EntitlementLookupType lookup);

                //client page functions
                void showGameDetails();
                void showMyProfile();
                void showAchievements();
                void showSettingsGeneral();
                void showSettingsInGame();
                void showSettingsNotifications();
                void showSettingsAdvanced();
                void showAboutMe();
                void showOrderHistory();
                void showAccountProfilePrivacy();
                void showAccountProfileSecurity();
                void showAccountProfilePaymentShipping();
                void showAccountProfileRedeem();
        };
    }
}

#endif
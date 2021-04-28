#ifndef PENDING_ACTION_GAME_LAUNCH_H
#define PENDING_ACTION_GAME_LAUNCH_H

#include <QObject>
#include <QUrlQuery>
#include "PendingActionBaseViewController.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API PendingActionGameLaunchViewController : public PendingActionBaseViewController
        {
            Q_OBJECT

        public:
            PendingActionGameLaunchViewController(const QUrlQuery &queryParams, const bool launchedFromBrowser, const QString &uuid, const QString &lookupId);
            virtual ~PendingActionGameLaunchViewController() {};

            ///\brief no popup windows are allowed when running this action
            void showPopupWindow() {};

            ///\brief intiates the rtp flow
            void performMainAction();

            ///\brief checks to see if we have a list of offerIds/contentIds
            bool parseAndValidateParams();

            /// \brief send out telemetry
            void sendTelemetry();

            /// \brief returns true if the SSO token should be consumed (it depends on the action and if that action has forceOnline functionality)
            virtual bool shouldConsumeSSOToken();

        private:
            QStringList mIdList;
            QString mGameTitle;
            QString mCmdParams;
            bool mAutoDownload;
            bool mGameRestart;
            bool mForceOnline;
            QString mItemSubType;
        };
    }
}

#endif
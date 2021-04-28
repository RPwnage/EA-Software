//    Copyright © 2012 Electronic Arts
//    All rights reserved.

#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebHistoryItem>
#include <QWidget>

#include "services/debug/DebugService.h"
#include "engine/login/LoginController.h"
#include "AchievementsViewController.h"
#include "WebWidget/WidgetPage.h"
#include "WebWidget/WidgetView.h"
#include "OfflineCdnNetworkAccessManager.h"

#include "AchievementsViewController.h"

#include "NavigationController.h"
#include "NativeBehaviorManager.h"
#include "WebWidgetController.h"
#include "TelemetryAPIDLL.h"

using namespace WebWidget;

namespace Origin
{
    using namespace Engine::Content;

    namespace Client
    {
        namespace
        {
            const QString AchievementDetailsLinkRole("http://widgets.dm.origin.com/linkroles/achievementdetails");
            const QString AchievementHomeViewLinkRole("http://widgets.dm.origin.com/linkroles/achievements");
        }

        AchievementsViewController::AchievementsViewController(Engine::UserRef user, QWidget *parent)
            : QObject(parent)
            , mAchievementsView(new WebWidget::WidgetView(parent))
            ,mLastLink(NoLink)
        {
            // Act and look more native
            NativeBehaviorManager::applyNativeBehavior(mAchievementsView);
            
            // Allow external network access
            mAchievementsView->widgetPage()->setExternalNetworkAccessManager(new OfflineCdnNetworkAccessManager(mAchievementsView->page()));

            // Load the widget up
            WebWidgetController::instance()->loadUserSpecificWidget(mAchievementsView->widgetPage(), "achievements", user);

            // Register our history
            NavigationController::instance()->addWebHistory(ACHIEVEMENT_TAB, mAchievementsView->page()->history());

            // Connect signals
            ORIGIN_VERIFY_CONNECT(mAchievementsView, SIGNAL(urlChanged(const QUrl&)), this, SLOT(onUrlChanged(const QUrl&)));

        }

        AchievementsViewController::~AchievementsViewController()
        {
        }

        void AchievementsViewController::onUrlChanged(const QUrl& url)
        {
            QString myUrl = url.toString();
            NavigationController::instance()->recordNavigation(ACHIEVEMENT_TAB, myUrl);
        }

        QWidget* AchievementsViewController::view() const
        {
            return mAchievementsView;
        }

        void AchievementsViewController::showAchievementSetDetails(const QString& achievementSetId, const QString& userId, const QString& gameTitle)
        {
            if(mAchievementsView)
            {
                QMap<QString, QString> variables;
                if(achievementSetId != "")
                    variables["id"] = achievementSetId;
                if(userId != "")
                    variables["userid"] = userId;
                if(gameTitle != "")
                    variables["gameTitle"] = gameTitle;

                bool isForceReload = true;
                mAchievementsView->loadLink(WidgetLink(AchievementDetailsLinkRole, variables), isForceReload);
                mLastLink=Details;
            }
        }

        void AchievementsViewController::showAchievementHome(bool recordNavigation)
        {
            if(mAchievementsView)
            {
                if (mLastLink!=Home)
                {
                    mAchievementsView->loadLink(WidgetLink(AchievementHomeViewLinkRole));
                    mLastLink=Home;
                }
                QString myUrl = mAchievementsView->page()->currentFrame()->url().toString();
                if (recordNavigation)
                {
                    NavigationController::instance()->recordNavigation(ACHIEVEMENT_TAB, myUrl);
                }
            }
        }

        void AchievementsViewController::onLoadUrl(QString url)
        {
            if (mAchievementsView)
            {
                mAchievementsView->load(QUrl(url));
            }
        }

    }
}

//
//  IGOAchievementViewController.cpp
//  originClient
//
//  Created by Frederic Meraud on 2/12/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#include "IGOAchievementsViewController.h"

#include "originwebview.h"
#include "NativeBehaviorManager.h"
#include "OfflineCdnNetworkAccessManager.h"

#include "WebWidgetController.h"
#include "WebWidget/WidgetPage.h"

#include "services/debug/DebugService.h"


namespace Origin
{
namespace Client
{

IGOAchievementsViewController::IGOAchievementsViewController()
{
    using namespace Origin::UIToolkit;
    
    mWidgetView = new WebWidget::WidgetView();
    NativeBehaviorManager::applyNativeBehavior(mWidgetView);
    
    mWidgetView->widgetPage()->setExternalNetworkAccessManager(new OfflineCdnNetworkAccessManager(mWidgetView->page()));
    
    mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Close, NULL, OriginWindow::WebContent,
                                                  QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Window, OriginWindow::OIG);
    if(mWindow->webview())
    {
        mWindow->webview()->setIsContentSize(false);
        mWindow->webview()->setHasSpinner(true);
        mWindow->webview()->setShowSpinnerAfterInitLoad(false);
        mWindow->webview()->setWebview(mWidgetView);
    }
    
    mWindow->resize(1024,640);
    mWindow->setMinimumSize(995,640);
    
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(deleteLater()));
    
    if (Origin::Engine::IGOController::instance()->showWebInspectors())
        Origin::Engine::IGOController::instance()->showWebInspector(mWindow->webview()->webview()->page());
}

IGOAchievementsViewController::~IGOAchievementsViewController()
{
    mWindow->deleteLater();
}

void IGOAchievementsViewController::show(Origin::Engine::UserRef user, uint64_t userId, const QString& achievementSet)
{
    if (achievementSet != "")
    {
        QMap<QString, QString> variables;
        
        if(userId != 0)
            variables["userid"] = QString::number(userId);
        variables["id"] = achievementSet;
        variables["inigo"] = QString("true");
        
        const QString AchievementDetailsLinkRole("http://widgets.dm.origin.com/linkroles/achievementdetails");
        WebWidgetController::instance()->loadUserSpecificWidget(mWidgetView->widgetPage(), "achievements", user, WebWidget::WidgetLink(AchievementDetailsLinkRole, variables));
    }
    
    else
    {
        // We shouldn't be able to get here if there isn't an achievement set ID.
        // However, load the achievements index page as a fail safe.
        QMap<QString, QString> variables;
        variables["inigo"] = QString("true");
        
        const QString AchievementHomeViewLinkRole("http://widgets.dm.origin.com/linkroles/achievements");
        WebWidgetController::instance()->loadUserSpecificWidget(mWidgetView->widgetPage(), "achievements", user, WebWidget::WidgetLink(AchievementHomeViewLinkRole, variables));
    }
}

bool IGOAchievementsViewController::ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior)
{
    QPoint pos = Origin::Engine::IGOController::instance()->igowm()->centeredPosition(ivcView()->size());
    properties->setPosition(pos);
    properties->setRestorable(true);
    
    behavior->pinnable = true;
    behavior->draggingSize = 40;
    behavior->nsBorderSize = 6;
    behavior->ewBorderSize = 4;
    
    return true;
}

} // Client
} // Origin
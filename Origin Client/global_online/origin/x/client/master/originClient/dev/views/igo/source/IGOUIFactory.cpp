//
//  IGOUIFactory.cpp
//  originClient
//
//  Created by Frederic Meraud on 1/30/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#include "IGOUIFactory.h"

#include "UIScope.h"
#include "IGOTitle.h"
#include "IGOClock.h"
#include "IGOLogo.h"
#include "IGONavigation.h"
#include "WebBrowserViewController.h"
#include "IGOAchievementsViewController.h"
#include "SearchViewController.h"
#include "ProfileViewController.h"
#include "PDLCViewController.h"
#include "IGOBroadcastingWidget.h"
#include "IGOSettingsViewController.h"
#include "IGOCustomerSupportViewController.h"
#include "CustomerSupportDialog.h"
#include "IGOInviteFriendsToGameViewController.h"
#include "IGOKoreanTooMuchFunViewController.h"
#include "IGOErrorViewController.h"
#include "IGOPinButton.h"
#include "IGOWebInspectorController.h"
#include "IGOCodeRedemptionViewController.h"
#include "BroadcastViewController.h"
#include "IGOSharedViewController.h"
#include "IGOSocialHubViewController.h"
#include "IGOSocialConversationViewController.h"

#include "TelemetryAPIDLL.h"

#include "services/debug/DebugService.h"


namespace Origin
{
namespace Client
{
    
IGOUIFactory::IGOUIFactory()
{
    
}

IGOUIFactory::~IGOUIFactory()
{
    
}

// IIGOUIFactory interface
Origin::Engine::IDesktopTitleViewController* IGOUIFactory::createTitle()
{
    return new IGOTitle();
}

Origin::Engine::IDesktopLogoViewController* IGOUIFactory::createLogo()
{
    return new IGOLogo();
}

Origin::Engine::IDesktopNavigationViewController* IGOUIFactory::createNavigation()
{
    return new IGONavigation();
}

Origin::Engine::IDesktopClockViewController* IGOUIFactory::createClock()
{
    return new IGOClock();
}

Origin::Engine::IWebBrowserViewController* IGOUIFactory::createWebBrowser(const QString& url, Origin::Engine::IWebBrowserViewController::BrowserFlags flags)
{
    GetTelemetryInterface()->Metric_IGO_BROWSER_START();
    return new WebBrowserViewController(IGOScope, flags, NULL);
}

Origin::Engine::IAchievementsViewController* IGOUIFactory::createAchievements()
{
    return new IGOAchievementsViewController();
}

Origin::Engine::ISearchViewController* IGOUIFactory::createSearch()
{
    return new SearchViewController(IGOScope, NULL);
 }

Origin::Engine::IProfileViewController* IGOUIFactory::createProfile(quint64 nucleusId)
{
    return new ProfileViewController(Origin::Client::IGOScope, nucleusId, NULL);
}

Origin::Engine::ISocialHubViewController* IGOUIFactory::createSocialHub()
{
    return new IGOSocialHubViewController();
}

Origin::Engine::ISocialConversationViewController* IGOUIFactory::createSocialChat()
{
    return new IGOSocialConversationViewController();
}

Origin::Engine::ISharedSPAViewController* IGOUIFactory::createSharedSPAWindow(const QString& id)
{
    return new IGOSharedViewController(id);
}

Origin::Engine::IPDLCStoreViewController* IGOUIFactory::createPDLCStore()
{
    return new PDLCViewController();
}

Origin::Engine::IGlobalProgressViewController* IGOUIFactory::createGlobalProgress()
{
    return NULL; // TODO Will be done with the oig global progress feature!
}

Origin::Engine::ITwitchBroadcastIndicator* IGOUIFactory::createTwitchBroadcastIndicator()
{
    return new IGOBroadcastingWidget();
}

Origin::Engine::ISettingsViewController* IGOUIFactory::createSettings(Origin::Engine::ISettingsViewController::Tab displayTab /*= ISettingsViewController::Tab_GENERAL*/)
{
    return new IGOSettingsViewController(displayTab);
}

Origin::Engine::ICustomerSupportViewController* IGOUIFactory::createCustomerSupport()
{
    QString strCustomerSupportURL;
    if (CustomerSupportDialog::checkShowCustomerSupportInBrowser(strCustomerSupportURL))
    {
        emit Origin::Engine::IGOController::instance()->showBrowser(strCustomerSupportURL, false);
        return NULL;
    }
    
    return new IGOCustomerSupportViewController(strCustomerSupportURL);
}

Origin::Engine::IInviteFriendsToGameViewController* IGOUIFactory::createInviteFriendsToGame(const QString& gameName)
{
    return new IGOInviteFriendsToGameViewController(gameName);
}

Origin::Engine::IKoreanTooMuchFunViewController* IGOUIFactory::createKoreanTooMuchFun()
{
    return new IGOKoreanTooMuchFunViewController();
}
    
Origin::Engine::IErrorViewController* IGOUIFactory::createError()
{
    return new IGOErrorViewController();
}

Origin::Engine::IPinButtonViewController* IGOUIFactory::createPinButton(QWidget* window)
{
    IGOPinButton* btn = new IGOPinButton(IGOScope, window);
    
    Origin::UIToolkit::OriginWindow* originWindow = dynamic_cast<UIToolkit::OriginWindow *>(window);
    if (originWindow)
        originWindow->setTitleBarContent(btn);
    
    return btn;
}

Origin::Engine::IWebInspectorController* IGOUIFactory::createWebInspector(QWebPage* page)
{
    return new IGOWebInspectorController(page);
}

#ifdef ORIGIN_PC
Origin::Engine::ITwitchViewController* IGOUIFactory::createTwitchBroadcast()
{
    return new BroadcastViewController();
} 
#endif

Origin::UIToolkit::OriginWindow* IGOUIFactory::createSharedWindow(QSize defaultSize, QSize minimumSize)
{
    UIToolkit::OriginWindow* window = new UIToolkit::OriginWindow(UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Close, NULL, UIToolkit::OriginWindow::Custom
                                                                  , QDialogButtonBox::NoButton, UIToolkit::OriginWindow::ChromeStyle_Window, UIToolkit::OriginWindow::OIG);
    if (defaultSize.isValid())
        window->resize(defaultSize);
    
    if (minimumSize.isValid())
        window->setMinimumSize(minimumSize);
    
    return window;
}

Origin::Engine::ICodeRedemptionViewController* IGOUIFactory::createCodeRedemption()
{
    return new IGOCodeRedemptionViewController();
}

} // Client
} // Origin
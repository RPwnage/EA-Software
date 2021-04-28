//
//  IGOUIFactory.h
//  originClient
//
//  Created by Frederic Meraud on 1/30/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#ifndef __originClient__IGOUIFactory__
#define __originClient__IGOUIFactory__

#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
    class ORIGIN_PLUGIN_API IGOUIFactory : public Origin::Engine::IIGOUIFactory
    {
    public:
        enum ClientWID
        {
            ClientWID_SSLINFO = Origin::Engine::IViewController::WI_CLIENT_IDS,
        };
    
    public:
        IGOUIFactory();
        virtual ~IGOUIFactory();
    
        // IIGOUIFactory interface
        virtual Origin::Engine::IDesktopTitleViewController* createTitle();
        virtual Origin::Engine::IDesktopLogoViewController* createLogo();
        virtual Origin::Engine::IDesktopNavigationViewController* createNavigation();
        virtual Origin::Engine::IDesktopClockViewController* createClock();
        virtual Origin::Engine::IWebBrowserViewController* createWebBrowser(const QString& url, Origin::Engine::IWebBrowserViewController::BrowserFlags flags);
        virtual Origin::Engine::IAchievementsViewController* createAchievements();
        virtual Origin::Engine::ISearchViewController* createSearch();
        virtual Origin::Engine::IProfileViewController* createProfile(quint64 nucleusId);
        virtual Origin::Engine::ISocialHubViewController* createSocialHub();
        virtual Origin::Engine::ISocialConversationViewController* createSocialChat();
        virtual Origin::Engine::ISharedSPAViewController* createSharedSPAWindow(const QString& id);
        virtual Origin::Engine::IPDLCStoreViewController* createPDLCStore();
        virtual Origin::Engine::IGlobalProgressViewController* createGlobalProgress();
        virtual Origin::Engine::ITwitchBroadcastIndicator* createTwitchBroadcastIndicator();
        virtual Origin::Engine::ISettingsViewController* createSettings(Origin::Engine::ISettingsViewController::Tab displayTab = Origin::Engine::ISettingsViewController::Tab_GENERAL);
        virtual Origin::Engine::ICustomerSupportViewController* createCustomerSupport();
        virtual Origin::Engine::IInviteFriendsToGameViewController* createInviteFriendsToGame(const QString& gameName);
        virtual Origin::Engine::IKoreanTooMuchFunViewController* createKoreanTooMuchFun();
        virtual Origin::Engine::IErrorViewController* createError();
        virtual Origin::Engine::IPinButtonViewController* createPinButton(QWidget* window);
        virtual Origin::Engine::IWebInspectorController* createWebInspector(QWebPage* page);
#ifdef ORIGIN_PC 
        virtual Origin::Engine::ITwitchViewController* createTwitchBroadcast();
#endif
        virtual Origin::UIToolkit::OriginWindow* createSharedWindow(QSize defaultSize, QSize minimumSize);
        virtual Origin::Engine::ICodeRedemptionViewController* createCodeRedemption();
    };
}
}
#endif /* defined(__originClient__IGOUIFactory__) */

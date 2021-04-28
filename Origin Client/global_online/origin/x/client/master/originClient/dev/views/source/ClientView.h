/////////////////////////////////////////////////////////////////////////////
// ClientView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CLIENTVIEW_H
#define CLIENTVIEW_H

#include <QWidget>
#include <QUrl>

#include "engine/login/User.h"
#include "services/plugin/PluginAPI.h"
#include "UIScope.h"

//LEGACY
#include "RedeemBrowser.h"

class UserArea;
class GlobalDownloadProgress;
class CustomerSupportDialog;

namespace Ui {
    class ClientView;
}

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
        class OriginMsgBox;
    }

    namespace Client
    {
        class AuthenticatedWebUIController;
        class StoreViewController;
        class AccountSettingsViewController;
        class ApplicationSettingsViewController;
        class OriginWebController;
        class AchievementsViewController;
        class ProfileViewController;
        class SearchViewController;
        class GlobalProgressViewController;

        class ORIGIN_PLUGIN_API ClientView : public QWidget
        {
            Q_OBJECT
        public:
            ClientView(Engine::UserRef user, QWidget *parent = NULL);
            ~ClientView();

            void clearViewControllers();

            void init();

            void updateUserArea();
            UserArea* userArea();
            QWidget *myGamesTab();
            QWidget* getNavBar();
            QWidget* getMessageArea();
            Origin::UIToolkit::OriginWindow* createRedemptionWindow (RedeemBrowser::SourceType src /*= Origin*/, RedeemBrowser::RequestorID requestorID /*= OriginCodeClient*/, const QString &code);
            Origin::UIToolkit::OriginWindow* createWindowWithWebContent ();

            void setMyOriginURL(QString url){ mMyOriginUrl = url; }

            /// \brief enables/disable social tab
            void enableSocialTab(bool isOnline);

            /// \brief records that the client is in social
            /// \param nucleusID of the person been viewed in social
            void setInSocial(const QString& nucleusID);

            StoreViewController *storeViewController() const { return mStoreViewController; }

            /// \brief retuns profile's starting URL
            QUrl profileStartUrl() const;

            /// \brief search keyword in friends
            void search(const QString& keyword);

            /// \brief search keyword in friends for OIG
            QWidget* searchViewForOIG(const QString& keyword);

            /// \brief shows search page
            void showSearchPage();

        public slots:
            void showStore(bool loadStoreHome = true);
            void showStore(const QUrl& url);
            void showStoreProduct(const QString& productId, const QString& masterTitleId="");
            void showStoreFreeGames();
            void showStoreOnTheHouse(const QString& trackingParam);
            void showStoreHome();

            void showMyOrigin(bool home = true);

            void showSettingsAccount();
            void showSettingsApplication(int tab = 0);
            void showSettingsGeneral();
            void showSettingsNotifications();
            void showSettingsInGame();
            void showSettingsAdvanced();
            void refreshAccountSettingPage(const int& accountSettingPath);
            void showSettingsVoice();
            void showOrderHistory();
            void showAccountProfilePrivacy();
            void showAccountProfileSecurity();
            void showAccountProfilePaymentShipping();
            void showAccountProfileSubscription(const QString& status = "");
            void showAccountProfileRedeem();

            void showMyGames();
            void showSocial();
            void showAchievementsHome(bool recordNavigation);
            void showAchievements(bool recordNavigation);
            void showGlobalProgress(const ClientViewWindowShowState& showState = ClientWindow_SHOW_NORMAL);
            /// \brief Generates URL and shows an achievement set's details based on the parameters passed.
            void showAchievementSetDetails(const QString& achievementSetId, const QString& userId = "", const QString& gameTitle = "");
            void showFeedbackPage(const QString& urlToShow, UIScope scope);
            void showProfilePage(quint64 nucleusId);
            void showProfilePage(QString);

            void showCustomerSupport();
            /// \brief Called when the window should zoom. Customizes the zoom 
            /// size of the window.
            void zoomedCustomerSupport();
            void closeCustomerSupport();    
            void refreshEntitlements();
            void showEbisuForum();  
            void showAbout();
            void closeAboutPage();

            void onChangeMessageAreaVisibility(const bool& isVisible);

            /// \brief connected to when request to retrieve authorizationCode to SSO into CE page
            void onCustomerSupportAuthCodeRetrieved();
            void onForumAuthCodeRetrieved();

        signals:
            void lostAuthentication();
            void showSettings();
            void recordMyOriginNavigation(const QString& url);
            void recordSocialNavigation(const QString& url);
            void recordFeedbackNavigation(const QString& url);

        protected slots:
            void onSocialUrlChanged(const QUrl& url);
            void onFeedbackUrlChanged(const QUrl& url);
            void onMyOriginUrlChanged(const QUrl&);

            void navigateToStoreUrl(const QString& url);

            /// \brief Called when the IGO Profile Window is closed or it's close
            /// button is clicked.
            void closeIGOProfileWindow();
            /// \brief Might reload OIG profile page upon adding a friend.

        protected:
            Ui::ClientView*             ui;

            AchievementsViewController*      mAchievementsViewController;
            AuthenticatedWebUIController*    mMyOriginWebController;
            StoreViewController*             mStoreViewController;
            AccountSettingsViewController*   mAccountSettingsViewController;
            ApplicationSettingsViewController*   mApplicationSettingsViewController;
            GlobalProgressViewController*       mGlobalProgressViewController;
            OriginWebController*             mFeedbackPageWebController;

            QWebView*                   mMyOriginView;

            QString                     mMyOriginUrl;

            UIToolkit::OriginWindow*            mIGOProfileWindow;
            OriginWebController*                mIGOProfileController;
            UIToolkit::OriginWindow*            mAboutEbisuDialog;
            UIToolkit::OriginWindow*            mRedeemWindow;
            UIToolkit::OriginWindow*            mCustomerSupportWindow;
            CustomerSupportDialog*              mCustomerSupportDialog;

            /// \brief am I seeing my own profile
            bool    mIsSeeingMyProfile;

            Engine::UserRef mUser;
            QSharedPointer<ProfileViewController> mProfileViewController;
            QSharedPointer<SearchViewController> mSearchViewController;

        private:

             /// \brief cleans the search page ready for next search. Each search is different.
             void removeSearchWidget();

             /// \brief cleans the profile page ready for next profile. Each profile might be different.
             void removeProfileWidget();

            QTimer mAuthCodeRetrieveTimer;
            QTimer mForumAuthCodeRetrieveTimer;
        };
    }
}

#endif

/////////////////////////////////////////////////////////////////////////////
// ClientView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ClientView.h"
#include "ui_ClientView.h"
#include "OriginApplication.h"
#include "engine/login/User.h"
#include "jshelpers/source/SettingsJsHelper.h"
#include "services/debug/DebugService.h"
#include "NavigationController.h"
#include "engine/igo/IGOController.h"
#include "WebWidget/FeatureRequest.h"
#include "TelemetryAPIDLL.h"
#include "AuthenticatedWebUIController.h"
#include "StoreViewController.h"
#include "AccountSettingsViewController.h"
#include "ApplicationSettingsViewController.h"
#include "AchievementsViewController.h"
#include "StoreUrlBuilder.h"
#include "ProfileViewController.h"
#include "SearchViewController.h"

//needed to retrieve authCode for CE page
#include "services/rest/AuthPortalServiceClient.h"
#include "services/rest/AuthPortalServiceResponses.h"
#include "services/log/LogService.h"

#ifdef ORIGIN_MAC
#include "OriginApplicationDelegateOSX.h"
#endif

#include "engine/content/ContentController.h"
#include "engine/social/SocialController.h"
#include "engine/login/LoginController.h"
#include "engine/subscription/SubscriptionManager.h"

#include "chat/OriginConnection.h"

#include "CustomerSupportJsHelper.h"
#include "BuddyListJsHelper.h"

#include <QDesktopServices>
#include <QDesktopWidget>
#include <QtWebKitWidgets/QWebView>

#include "originmsgbox.h"
#include "originscrollablemsgbox.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include "originpushbutton.h"
#include "origincheckbutton.h"

#include "OriginWebController.h"

// Legacy APP
#include "SocialJsHelper.h"
#include "../ui/CustomerSupportDialog.h"
#include "AboutEbisuDialog.h"
#include "MyGameJsHelper.h"
#include "RedeemBrowser.h"

#include "services/connection/ConnectionStatesService.h"
#include "services/settings/SettingsManager.h"
#include "services/qt/QtUtil.h"

namespace Origin
{
    namespace Client
    {


        ClientView::ClientView(Engine::UserRef user, QWidget* parent)
            : QWidget(parent)
            , ui(new Ui::ClientView)
            , mAchievementsViewController(NULL)
            , mMyOriginWebController(NULL)
            , mAccountSettingsViewController(NULL)
            , mApplicationSettingsViewController(NULL)
            , mFeedbackPageWebController(NULL)
            , mMyOriginView(NULL)
            , mIGOProfileWindow(NULL)
            , mIGOProfileController(NULL)
            , mAboutEbisuDialog(NULL)
            , mRedeemWindow(NULL)
            , mCustomerSupportWindow(NULL)
            , mCustomerSupportDialog(NULL)
            , mIsSeeingMyProfile(false)
            , mUser(user)
            , mProfileViewController(NULL)
            , mSearchViewController(NULL)
        {
            ui->setupUi(this);

            init();
        }

        ClientView::~ClientView()
        {
            mAuthCodeRetrieveTimer.stop();
            mFeedbackPageWebController = NULL;
            mAchievementsViewController = NULL;
            mMyOriginWebController = NULL;
            mAccountSettingsViewController = NULL;
            mApplicationSettingsViewController = NULL;

            clearViewControllers();

            closeIGOProfileWindow();

            closeAboutPage();

            closeCustomerSupport();
        }

        void ClientView::init()
        {
            ui->messageArea->setVisible(false);

            // Set up our store
            mStoreViewController = new StoreViewController(ui->page_Store);
            ui->page_Store->layout()->addWidget(mStoreViewController->view());

            ORIGIN_VERIFY_CONNECT(mStoreViewController, SIGNAL(lostAuthentication()), this, SIGNAL(lostAuthentication()));

            mAccountSettingsViewController = new AccountSettingsViewController(ui->page_AccountSettings);
            ui->page_AccountSettings->layout()->addWidget(mAccountSettingsViewController->view());

            ORIGIN_VERIFY_CONNECT(mAccountSettingsViewController, SIGNAL(lostAuthentication()), this, SIGNAL(lostAuthentication()));

            mApplicationSettingsViewController = new ApplicationSettingsViewController(ui->page_ApplicationSettings);
            ui->page_ApplicationSettings->layout()->addWidget(mApplicationSettingsViewController->view());
            
            // Set up My Origin (home)
            mMyOriginView = new QWebView(ui->page_MyOrigin);
            ui->page_MyOrigin->layout()->addWidget(mMyOriginView);

            mMyOriginWebController = new AuthenticatedWebUIController(mUser->getSession(), mMyOriginView);
            mMyOriginView->settings()->setAttribute(QWebSettings::PluginsEnabled, true);

            mMyOriginWebController->errorHandler()->setErrorPage(DefaultErrorPage::centerAlignedPage());
            mMyOriginWebController->errorDetector()->disableErrorCheck(PageErrorDetector::FinishedWithFailure);

            ORIGIN_VERIFY_CONNECT(mMyOriginView->page()->currentFrame(), SIGNAL(urlChanged(const QUrl&)), this, SLOT(onMyOriginUrlChanged(const QUrl&)));
        
            NavigationController::instance()->addWebHistory(MY_ORIGIN_TAB, mMyOriginView->page()->history());

            // Set up achievements page
            mAchievementsViewController = new AchievementsViewController(mUser, ui->page_Achievements);
            ORIGIN_VERIFY_CONNECT(NavigationController::instance(), SIGNAL(loadUrl(QString)), mAchievementsViewController, SLOT(onLoadUrl(QString)));

            ui->page_Achievements->layout()->addWidget(mAchievementsViewController->view());
        }

        void ClientView::onSocialUrlChanged(const QUrl& url)
        {
            emit (recordSocialNavigation(url.toString()));
        }

        void ClientView::onFeedbackUrlChanged(const QUrl& url)
        {
            emit (recordFeedbackNavigation(url.toString()));
        }

        void ClientView::onMyOriginUrlChanged(const QUrl& url)
        {
            GetTelemetryInterface()->Metric_HOME_NAVIGATE_URL(Services::EncodeUrlForTelemetry(url).data());
            emit (recordMyOriginNavigation(url.toString()));
        }

        void ClientView::showStore(bool loadStoreHome /* = true */)
        {
            ui->mainArea->setCurrentWidget(ui->page_Store);
            NavigationController::instance()->clicked(STORE_TAB);
            if(loadStoreHome)
            {
                ORIGIN_LOG_ACTION << "Trying to load store home";
                mStoreViewController->loadStoreUrl(mStoreViewController->storeUrlBuilder()->homeUrl());
            }
        }

        void ClientView::showStore(const QUrl& url)
        {
            mStoreViewController->loadStoreUrl(url);
            showStore(false);
        }

        void ClientView::showStoreProduct(const QString& productId, const QString& masterTitleId)
        {
            mStoreViewController->loadStoreUrl(mStoreViewController->storeUrlBuilder()->productUrl(productId, masterTitleId));
            showStore(false);
        }
            
        void ClientView::showStoreFreeGames()
        {
            mStoreViewController->loadStoreUrl(mStoreViewController->storeUrlBuilder()->freeGamesUrl());
            showStore(false);
        }

        void ClientView::showStoreOnTheHouse(const QString& trackingParam)
        {
            mStoreViewController->loadStoreUrl(mStoreViewController->storeUrlBuilder()->onTheHouseStoreUrl(trackingParam));
            showStore(false);
        }

        void ClientView::showStoreHome()
        {
            mStoreViewController->loadStoreUrl(mStoreViewController->storeUrlBuilder()->homeUrl());
            showStore();
        }

        void ClientView::showMyOrigin(bool bHome /* = true */)
        {
            ui->mainArea->setCurrentWidget(ui->page_MyOrigin);

            QUrl currentUrl = mMyOriginView->url();
            if (currentUrl.isEmpty() || bHome)
            {
                currentUrl = mMyOriginUrl;
            }
            
            WebWidget::FeatureRequest achievementManager("http://widgets.dm.origin.com/features/interfaces/achievementManager");
            WebWidget::FeatureRequest clientNavigation("http://widgets.dm.origin.com/features/interfaces/clientNavigation");
            WebWidget::FeatureRequest dateFormat("http://widgets.dm.origin.com/features/interfaces/dateFormat");
            WebWidget::FeatureRequest entitlementManager("http://widgets.dm.origin.com/features/interfaces/entitlementManager");
            WebWidget::FeatureRequest originPageVisibility("http://widgets.dm.origin.com/features/interfaces/originPageVisibility");
            WebWidget::FeatureRequest originSocial("http://widgets.dm.origin.com/features/interfaces/originSocial");
            WebWidget::FeatureRequest originUser("http://widgets.dm.origin.com/features/interfaces/originUser");

            mMyOriginWebController->jsBridge()->addFeatureRequest(achievementManager);
            mMyOriginWebController->jsBridge()->addFeatureRequest(clientNavigation);
            mMyOriginWebController->jsBridge()->addFeatureRequest(dateFormat);
            mMyOriginWebController->jsBridge()->addFeatureRequest(entitlementManager);
            mMyOriginWebController->jsBridge()->addFeatureRequest(originPageVisibility);
            mMyOriginWebController->jsBridge()->addFeatureRequest(originSocial);
            mMyOriginWebController->jsBridge()->addFeatureRequest(originUser);

            mMyOriginWebController->loadAuthenticatedUrl(currentUrl);

            NavigationController::instance()->clicked(MY_ORIGIN_TAB);
        }

        void ClientView::showSettingsAccount()
        {
            GetTelemetryInterface()->Metric_GUI_SETTINGS_VIEW(EbisuTelemetryAPI::GUISettingsView_Account);
            mAccountSettingsViewController->loadWebPage(AccountSettingsViewController::Home);
            ui->mainArea->setCurrentWidget(ui->page_AccountSettings);
            NavigationController::instance()->recordNavigation(SETTINGS_ACCOUNT_TAB);
        }

        void ClientView::showSettingsApplication(int tab)
        {
            GetTelemetryInterface()->Metric_GUI_SETTINGS_VIEW(EbisuTelemetryAPI::GUISettingsView_Application);
            mApplicationSettingsViewController->initialLoad((ApplicationSettingsViewController::SettingsTab) tab);
            ui->mainArea->setCurrentWidget(ui->page_ApplicationSettings);
            NavigationController::instance()->recordNavigation(SETTINGS_APPLICATION_TAB);
        }

        void ClientView::showSettingsGeneral()
        {
            showSettingsApplication(ApplicationSettingsViewController::GeneralTab);
        }

        void ClientView::showSettingsNotifications()
        {
            showSettingsApplication(ApplicationSettingsViewController::NotificationsTab);
        }

        void ClientView::showSettingsInGame()
        {
            showSettingsApplication(ApplicationSettingsViewController::OIGTab);
        }

        void ClientView::showSettingsAdvanced()
        {
            showSettingsApplication(ApplicationSettingsViewController::AdvancedTab);
        }

        void ClientView::refreshAccountSettingPage(const int& accountSettingPath)
        {
            mAccountSettingsViewController->refreshPage(static_cast<AccountSettingsViewController::Path>(accountSettingPath));
        }

        void ClientView::showSettingsVoice()
        {
            showSettingsApplication(ApplicationSettingsViewController::VoiceTab);
        }

        void ClientView::showOrderHistory()
        {
            mAccountSettingsViewController->loadWebPage(AccountSettingsViewController::OrderHistory);
            ui->mainArea->setCurrentWidget(ui->page_AccountSettings);
            NavigationController::instance()->recordNavigation(SETTINGS_ACCOUNT_TAB);
        }

        void ClientView::showAccountProfilePrivacy()
        {
            mAccountSettingsViewController->loadWebPage(AccountSettingsViewController::Privacy);
            ui->mainArea->setCurrentWidget(ui->page_AccountSettings);
            NavigationController::instance()->recordNavigation(SETTINGS_ACCOUNT_TAB);
        }

        void ClientView::showAccountProfileSecurity()
        {
            mAccountSettingsViewController->loadWebPage(AccountSettingsViewController::Security);
            ui->mainArea->setCurrentWidget(ui->page_AccountSettings);
            NavigationController::instance()->recordNavigation(SETTINGS_ACCOUNT_TAB);
        }

        void ClientView::showAccountProfilePaymentShipping()
        {
            mAccountSettingsViewController->loadWebPage(AccountSettingsViewController::PaymentShipping);
            ui->mainArea->setCurrentWidget(ui->page_AccountSettings);
            NavigationController::instance()->recordNavigation(SETTINGS_ACCOUNT_TAB);
        }

        void ClientView::showAccountProfileSubscription(const QString& status)
        {
            mAccountSettingsViewController->loadWebPage(AccountSettingsViewController::Subscription, status);
            ui->mainArea->setCurrentWidget(ui->page_AccountSettings);
            NavigationController::instance()->recordNavigation(SETTINGS_ACCOUNT_TAB);
        }

        void ClientView::showAccountProfileRedeem()
        {
            mAccountSettingsViewController->loadWebPage(AccountSettingsViewController::Redemption);
            ui->mainArea->setCurrentWidget(ui->page_AccountSettings);
            NavigationController::instance()->recordNavigation(SETTINGS_ACCOUNT_TAB);
        }

        void ClientView::showMyGames()
        {
            NavigationController::instance()->clicked(MY_GAMES_TAB);
            ui->mainArea->setCurrentWidget(ui->page_Games);
        }
        
        void ClientView::showSocial()
        {
            ui->mainArea->setCurrentWidget(ui->page_Profile);
        }

        void ClientView::showAchievementsHome(bool recordNavigation)
        {
            NavigationController::instance()->clicked(ACHIEVEMENT_TAB);
            //clearViewControllers();

            if(mAchievementsViewController)
            {
                mAchievementsViewController->showAchievementHome(recordNavigation);
            }
            showAchievements(false);
        }

        void ClientView::showAchievements(bool recordNavigation)
        {
            ui->mainArea->setCurrentWidget(ui->page_Achievements);
            if (recordNavigation)
            {
                NavigationController::instance()->clicked(ACHIEVEMENT_TAB);
                NavigationController::instance()->recordNavigation(ACHIEVEMENT_TAB);
            }
        }

        void ClientView::showGlobalProgress(const ClientViewWindowShowState& showState)
        {
            switch(showState)
            {
            case ClientWindow_SHOW_NORMAL_IF_CREATED:
                break;
            case ClientWindow_SHOW_NORMAL:
                break;
            case ClientWindow_SHOW_MINIMIZED:
            case ClientWindow_SHOW_MINIMIZED_SYSTEMTRAY:
                break;
            default:
                break;
            }
        }

        void ClientView::showAchievementSetDetails(const QString& achievementSetId, const QString& userId, const QString& gameTitle)
        {
            if(mAchievementsViewController)
            {
                mAchievementsViewController->showAchievementSetDetails(achievementSetId, userId, gameTitle);
            }
            ui->mainArea->setCurrentWidget(ui->page_Achievements);
        }

        void ClientView::navigateToStoreUrl(const QString& url)
        {
            mStoreViewController->loadEntryUrlAndExpose(url);
            ui->mainArea->setCurrentWidget(ui->page_Store);
        }

        QUrl ClientView::profileStartUrl() const
        {
            ORIGIN_ASSERT(mProfileViewController);
            return mProfileViewController->startUrl();
        }

        void ClientView::showProfilePage(QString id)
        {
            quint64 nucleusId = ProfileViewController::nucleusIdByUserId(id);
            showProfilePage(nucleusId);
        }

        void ClientView::showProfilePage(quint64 nucleusId)
        {
            if (mProfileViewController)
            {
                if (!mProfileViewController->showProfileForNucleusId(nucleusId))
                {
                    NavigationController::instance()->recordNavigation(SOCIAL_TAB);
                }
            }
            else
            {
                mProfileViewController.clear();
                mProfileViewController = QSharedPointer<ProfileViewController>(new ProfileViewController(ClientScope, nucleusId, ui->page_Profile));
                ui->page_Profile->layout()->addWidget(mProfileViewController->view());
                NavigationController::instance()->addWebHistory(SOCIAL_TAB, mProfileViewController->history());
            }
            showSocial();
        }

        void ClientView::showFeedbackPage(const QString& urlToShow, UIScope scope)
        {
            if (scope == ClientScope)
            {
                if (mFeedbackPageWebController == NULL)
                {
                    mFeedbackPageWebController = new OriginWebController(ui->feedbackPageTab->webview());
                    mFeedbackPageWebController->errorHandler()->setErrorPage(DefaultErrorPage::centerAlignedPage());

                    mFeedbackPageWebController->jsBridge()->addHelper("MyGameJsHelper", new MyGameJsHelper(this));
                            
                    // Add our JavaScript interfaces
                    WebWidget::FeatureRequest clientNavigation("http://widgets.dm.origin.com/features/interfaces/clientNavigation");
                    mFeedbackPageWebController->jsBridge()->addFeatureRequest(clientNavigation);
                    
                    mFeedbackPageWebController->errorDetector()->disableErrorCheck(PageErrorDetector::MainFrameRequestFailed);
                    mFeedbackPageWebController->errorDetector()->disableErrorCheck(PageErrorDetector::FinishedWithFailure);
                    
                    ORIGIN_VERIFY_CONNECT(ui->feedbackPageTab->webview()->page()->currentFrame(), SIGNAL(urlChanged(const QUrl&)), this, SLOT(onFeedbackUrlChanged(const QUrl&)));
                    NavigationController::instance()->addWebHistory(FEEDBACK_TAB, ui->feedbackPageTab->webview()->page()->history());
                }

                ui->mainArea->setCurrentWidget(ui->page_Feedback);

                QWebSettings::clearMemoryCaches();

                mFeedbackPageWebController->loadTrustedUrl(urlToShow);

            }
            // else in IGO - can't happen (not yet anyway)
        }

       void ClientView::closeIGOProfileWindow()
       {
           if(mIGOProfileWindow)
           {
               ORIGIN_VERIFY_DISCONNECT(mIGOProfileWindow, SIGNAL(rejected()), this, SLOT(closeIGOProfileWindow()));
               mIGOProfileWindow->close();
               mIGOProfileWindow = NULL;
               mIGOProfileController = NULL;
           }
       }

       Origin::UIToolkit::OriginWindow* ClientView::createRedemptionWindow (RedeemBrowser::SourceType src /*= Origin*/, RedeemBrowser::RequestorID requestorID /*= OriginCodeClient*/, const QString &code)
       {
           using namespace Origin::UIToolkit;
            
           OriginWindow* redeemWin = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close, new RedeemBrowser(NULL, src, requestorID,NULL, code));
           redeemWin->setAccessibleName(tr("ebisu_client_redeem_game_code")); // set the accessiblity name for this window
           redeemWin->setObjectName("RedeemCodeWindow");

           return redeemWin;
       }

       Origin::UIToolkit::OriginWindow* ClientView::createWindowWithWebContent ()
       {
           using namespace Origin::UIToolkit;
           OriginWindow *webContentWin = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close, NULL, OriginWindow::WebContent);
           webContentWin->setAttribute(Qt::WA_DeleteOnClose, false);
           return webContentWin;
       }

        ////////////////////////////////////////////////////////////////
        // All these were moved from the old UserArea class. I'm not really
        // sure they belong here, but this is the top level "view" class. Seems
        // like maybe we need an application view or somesuch to put global
        // cruft like this into.


        void ClientView::showCustomerSupport()
        {   
//            ORIGIN_LOG_ACTION << "User selected customer support.";

            bool offline = (mUser.isNull() || !Origin::Services::Connection::ConnectionStatesService::isUserOnline (mUser->getSession()));
            QString locale = OriginApplication::instance().locale();
    
            if (offline)
            {   
                // this should be allowed for all languages with 8.5
                UIToolkit::OriginWindow::alert(UIToolkit::OriginMsgBox::Info, tr("ebisu_login_offline_mode_title"), tr("ebisu_client_ce_offline_body").arg(tr("application_name")), tr("ebisu_client_close"));
            }
            else
            {
                QString strCustomerSupportURL = "";

                if (CustomerSupportDialog::checkShowCustomerSupportInBrowser(strCustomerSupportURL))
                {
                    //MY: we need to retrieve the authorizationCode so that we an SSO into the help page
                    const int NETWORK_TIMEOUT_MSECS = 30000;
                    const QString CE_CLIENTID = "origin_CE";

                    ORIGIN_LOG_EVENT << "Customer Support: retrieving authorization code";

                    QString accessToken = Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession());
                    Origin::Services::AuthPortalAuthorizationCodeResponse *resp = Origin::Services::AuthPortalServiceClient::retrieveAuthorizationCode(accessToken, CE_CLIENTID);

                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onCustomerSupportAuthCodeRetrieved()));
                    ORIGIN_VERIFY_CONNECT(&mAuthCodeRetrieveTimer, SIGNAL (timeout()), resp, SLOT(abort())); 

                    mAuthCodeRetrieveTimer.setSingleShot(true);
                    mAuthCodeRetrieveTimer.setInterval (NETWORK_TIMEOUT_MSECS);
                    mAuthCodeRetrieveTimer.start();

                    resp->setDeleteOnFinish(true);                    
                }
                else
                {
                    using namespace Origin::UIToolkit;
                    if(mCustomerSupportWindow == NULL)
                    {
                        CustomerSupportDialog* customerSupportDialog = new CustomerSupportDialog(strCustomerSupportURL);
                        ORIGIN_VERIFY_CONNECT(customerSupportDialog, SIGNAL(lostAuthentication()), this, SIGNAL(lostAuthentication()));
                        ORIGIN_VERIFY_CONNECT(customerSupportDialog, SIGNAL(lostAuthentication()), this, SLOT(closeCustomerSupport()));

                        mCustomerSupportWindow = new OriginWindow(OriginWindow::AllItems, customerSupportDialog, OriginWindow::Custom, QDialogButtonBox::NoButton, OriginWindow::ChromeStyle_Window);
                        mCustomerSupportWindow->setTitleBarText(tr("ebisu_client_ce_window_title").arg(tr("application_name")));
                        mCustomerSupportWindow->setContentMargin(0);
                        mCustomerSupportWindow->setMinimumSize(480, 480);

                        ORIGIN_VERIFY_CONNECT(mCustomerSupportWindow, SIGNAL(rejected()), this, SLOT(closeCustomerSupport()));
						if(mCustomerSupportWindow->manager())
                            ORIGIN_VERIFY_CONNECT(mCustomerSupportWindow->manager(), SIGNAL(customizeZoomedSize()), this, SLOT(zoomedCustomerSupport()));
                    }


                    mCustomerSupportWindow->showWindow();
                }
            }
        }

        void ClientView::onCustomerSupportAuthCodeRetrieved ()
        {
            Origin::Services::AuthPortalAuthorizationCodeResponse* response = dynamic_cast<Origin::Services::AuthPortalAuthorizationCodeResponse*>(sender());
            ORIGIN_ASSERT(response);

            mAuthCodeRetrieveTimer.stop();

            if ( !response )
            {
                //just ignore since it really shouldn't happen (abort() won't make it null)
                return;
            }
            else
            {
                ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(onCustomerSupportAuthCodeRetrieved()));
                ORIGIN_VERIFY_DISCONNECT(&mAuthCodeRetrieveTimer, SIGNAL (timeout()), response, SLOT(abort())); 
            }
                
            bool success = ((response->error() == Origin::Services::restErrorSuccess) && (response->httpStatus() == Origin::Services::Http302Found));

            if (!success)   //we need to log it
            {
                ORIGIN_LOG_ERROR << "CS: auth code retrieval failed with: restError = " << response->error() << " httpStatus = " << response->httpStatus();
            }

            //launch the CE page reardless of whether we have authCode or not so that the user will have feedback that something happened
            //they will need to log in to the page manually tho
            {
                QString authCode = response->authorizationCode();

                QString strCustomerSupportURL = "";

                if (CustomerSupportDialog::checkShowCustomerSupportInBrowser(strCustomerSupportURL, authCode))
                {
                    ORIGIN_LOG_EVENT << "CE: Launching help page";
                    if (strCustomerSupportURL.isEmpty())
                    {
                        ORIGIN_ASSERT_MESSAGE (!strCustomerSupportURL.isEmpty(), "CustomerSupport URL is empty");
                        ORIGIN_LOG_ERROR << "CustomerSupport Url is empty";
                        return;
                    }
                    else
                    {
                        ORIGIN_LOG_DEBUG << "CustomerSupportURL: " << strCustomerSupportURL;
                        QDesktopServices::openUrl(strCustomerSupportURL);
                    }
                }
            }
        }

        void ClientView::zoomedCustomerSupport()
        {
            if(mCustomerSupportWindow)
            {
                const QRect availableGeo = QDesktopWidget().availableGeometry(mCustomerSupportWindow);
                mCustomerSupportWindow->setGeometry(mCustomerSupportWindow->x(), availableGeo.y(), 
                                                    960, availableGeo.height());
                if(mCustomerSupportWindow->manager())
                    mCustomerSupportWindow->manager()->ensureOnScreen();
            }
        }


        void ClientView::closeCustomerSupport()
        {
            if(mCustomerSupportWindow)
            {
                ORIGIN_VERIFY_DISCONNECT(mCustomerSupportWindow, SIGNAL(rejected()), mCustomerSupportWindow, SLOT(closeCustomerSupport()));
                if(mCustomerSupportWindow->manager())
                    ORIGIN_VERIFY_DISCONNECT(mCustomerSupportWindow->manager(), SIGNAL(customizeZoomedSize()), this, SLOT(zoomedCustomerSupport()));
                mCustomerSupportWindow->close();
                mCustomerSupportWindow = NULL;
            }
        }


        void ClientView::refreshEntitlements()
        {
            GetTelemetryInterface()->Metric_ENTITLEMENT_RELOAD("menu");
            Origin::Engine::Content::ContentController::currentUserContentController()->refreshUserGameLibrary(Engine::Content::UserRefresh);
        }

        void ClientView::showEbisuForum()
        {
            bool offline = (mUser.isNull() || !Origin::Services::Connection::ConnectionStatesService::isUserOnline (mUser->getSession()));
    
            if (offline)
            {   
                // this should be allowed for all languages with 8.5
                UIToolkit::OriginWindow::alert(UIToolkit::OriginMsgBox::Info, tr("ebisu_login_offline_mode_title"), tr("ebisu_client_ce_offline_body").arg(tr("application_name")), tr("ebisu_client_close"));
            }
            else
            {
                //MY: we need to retrieve the authorizationCode so that we can SSO into the forum website
                const int NETWORK_TIMEOUT_MSECS = 30000;
                const QString CE_CLIENTID = "origin_CE";

                ORIGIN_LOG_EVENT << "Origin Forum: retrieving authorization code";

                QString accessToken = Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession());
                Origin::Services::AuthPortalAuthorizationCodeResponse *resp = Origin::Services::AuthPortalServiceClient::retrieveAuthorizationCode(accessToken, CE_CLIENTID);

                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onForumAuthCodeRetrieved()));
                ORIGIN_VERIFY_CONNECT(&mForumAuthCodeRetrieveTimer, SIGNAL (timeout()), resp, SLOT(abort())); 

                mForumAuthCodeRetrieveTimer.setSingleShot(true);
                mForumAuthCodeRetrieveTimer.setInterval (NETWORK_TIMEOUT_MSECS);
                mForumAuthCodeRetrieveTimer.start();

                resp->setDeleteOnFinish(true);                    
            }
        }

        void ClientView::onForumAuthCodeRetrieved()
        {
            Origin::Services::AuthPortalAuthorizationCodeResponse* response = dynamic_cast<Origin::Services::AuthPortalAuthorizationCodeResponse*>(sender());
            ORIGIN_ASSERT(response);

            mForumAuthCodeRetrieveTimer.stop();

            if ( !response )
            {
                //just ignore since it really shouldn't happen (abort() won't make it null)
                return;
            }
            else
            {
                ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(onForumAuthCodeRetrieved()));
                ORIGIN_VERIFY_DISCONNECT(&mForumAuthCodeRetrieveTimer, SIGNAL (timeout()), response, SLOT(abort())); 
            }
                
            bool success = ((response->error() == Origin::Services::restErrorSuccess) && (response->httpStatus() == Origin::Services::Http302Found));
            if (!success)   //we need to log it
            {
                ORIGIN_LOG_ERROR << "Origin Forum: auth code retrieval failed with: restError = " << response->error() << " httpStatus = " << response->httpStatus();
            }

            QString authCode = response->authorizationCode();
            QString strForumURL;

            // To comply with the Toubon Law, we use a seperate forum url for the fr_FR locale: https://developer.origin.com/support/browse/EBIBUGS-27457
            // Note: the forum website may be upgraded in the future to handle its own localization, so this
            // separate fr_FR url may become unnecessary.
            // Also, if we failed to retrieve a valid auth code, then we use the old, non-SSO version of the forum website.
            const QString locale = Services::readSetting(Services::SETTING_LOCALE);
            if(locale.compare("fr_FR", Qt::CaseInsensitive) == 0)
            {
                if(!authCode.isEmpty())
                {
                    strForumURL = Services::readSetting(Services::SETTING_EbisuForumFranceSSOURL).toString();
                    strForumURL.replace("{code}", authCode);
                }
                else
                {
                    strForumURL = Services::readSetting(Services::SETTING_EbisuForumFranceURL).toString();
                }
            }
            else
            {
                if(!authCode.isEmpty())
                {
                    strForumURL = Services::readSetting(Services::SETTING_EbisuForumSSOURL).toString();
                    strForumURL.replace("{code}", authCode);                    
                }
                else
                {
                    strForumURL = Services::readSetting(Services::SETTING_EbisuForumURL).toString();
                }
            }

            ORIGIN_LOG_EVENT << "CE: Launching Origin Forum page";
            ORIGIN_LOG_DEBUG << "OriginForumURL: " << strForumURL;
            QDesktopServices::openUrl(strForumURL);
        }

        void ClientView::showAbout()
        {
//            ORIGIN_LOG_ACTION << "User selected about dialog.";
            if (mAboutEbisuDialog == NULL)
            {
                using namespace Origin::UIToolkit;
                mAboutEbisuDialog = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close, NULL, 
                    OriginWindow::ScrollableMsgBox, QDialogButtonBox::Close);
                mAboutEbisuDialog->scrollableMsgBox()->setup(OriginScrollableMsgBox::NoIcon, tr("application_name_uppercase"), "");
                mAboutEbisuDialog->manager()->setupButtonFocus();
                mAboutEbisuDialog->setContent(new AboutEbisuDialog());
                mAboutEbisuDialog->setTitleBarText(tr("ebisu_client_about_dialog").arg(tr("application_name")));

                ORIGIN_VERIFY_CONNECT(mAboutEbisuDialog, SIGNAL(rejected()), this, SLOT(closeAboutPage()));
                ORIGIN_VERIFY_CONNECT(mAboutEbisuDialog->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(closeAboutPage()));
            }
            mAboutEbisuDialog->showWindow();
        }

        void ClientView::closeAboutPage()
        {
            if (mAboutEbisuDialog)
            {
                ORIGIN_VERIFY_DISCONNECT(mAboutEbisuDialog, SIGNAL(rejected()), this, SLOT(closeAboutPage()));
                ORIGIN_VERIFY_DISCONNECT(mAboutEbisuDialog->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(closeAboutPage()));
                mAboutEbisuDialog->close();
                mAboutEbisuDialog = NULL;
            }
        }

        void ClientView::onChangeMessageAreaVisibility(const bool& isVisible)
        {
            setUpdatesEnabled(false);
            ui->messageArea->setVisible(isVisible);
            setUpdatesEnabled(true);
        }

        void ClientView::setInSocial(const QString& nucleusID)
        {
            QString myNucleusID = QString::number(Origin::Engine::LoginController::currentUser()->userId());
            mIsSeeingMyProfile = myNucleusID == nucleusID;
        }

        void ClientView::enableSocialTab(bool isOnline)
        {
            ui->page_Profile->setEnabled(isOnline || mIsSeeingMyProfile);
        }

        QWidget* ClientView::myGamesTab() 
        { 
            return ui->page_Games;
        }
        
        QWidget* ClientView::getNavBar() 
        { 
            return ui->navBar; 
        }

        QWidget* ClientView::getMessageArea() 
        { 
            return ui->messageArea; 
        }
        
        void ClientView::search(const QString& keyword)
        {
            //removeSearchWidget();
            if (!mSearchViewController)
            {
                mSearchViewController = QSharedPointer<SearchViewController>(new SearchViewController(Origin::Client::ClientScope, ui->page_Search));
                ui->page_Search->layout()->addWidget(mSearchViewController->view());
            }
            mSearchViewController->isLoadSearchPage(keyword);
            showSearchPage();
        }

        void ClientView::removeSearchWidget()
        {
            if (!mSearchViewController.isNull())
            {
                ui->page_Search->layout()->removeWidget(mSearchViewController->view());
                mSearchViewController.clear();
                mSearchViewController = QSharedPointer<SearchViewController>();
            }
        }

        void ClientView::removeProfileWidget()
        {
            if (mProfileViewController)
            {
                ui->page_Profile->layout()->removeWidget(mProfileViewController->view());
                mProfileViewController.clear();
                mProfileViewController = QSharedPointer<ProfileViewController>();
            }
        }

        void ClientView::showSearchPage()
        {
            ui->mainArea->setCurrentWidget(ui->page_Search);
        }

        void ClientView::clearViewControllers()
        {
            removeProfileWidget();
            removeSearchWidget();
        }

    }
}

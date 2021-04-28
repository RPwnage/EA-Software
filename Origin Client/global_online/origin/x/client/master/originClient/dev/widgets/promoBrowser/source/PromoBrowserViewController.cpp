/////////////////////////////////////////////////////////////////////////////
// PromoBrowserViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "PromoBrowserViewController.h"
#include "PromoView.h"

#include "OriginApplication.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "services/session/SessionService.h"
#include "services/platform/PlatformService.h"

#include "engine/login/User.h"
#include "engine/login/LoginController.h"
#include "engine/content/ContentController.h"
#include "engine/subscription/SubscriptionManager.h"

#include "flows/MainFlow.h"
#include "flows/RTPFlow.h"
#include "flows/ClientFlow.h"
#include "flows/PendingActionFlow.h"
#include "MOTDViewController.h"
#include "NetPromoterViewController.h"
#include "TelemetryAPIDLL.h"

// Error dialog for manual promo browser
#include "originmsgbox.h"
#include "originwindow.h"
#include "originwindowmanager.h"

#include <QDate>
#include <QDesktopWidget>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKitWidgets/QWebView>
#include <QNetworkCookie>

namespace
{
    const int PROMOBROWSER_SUPPRESS_ORIGIN_STARTED_DAYS = 1;
    const int PROMOBROWSER_SUPPRESS_GAME_FINISHED_DAYS = 2;
    const int PROMOBROWSER_SUPPRESS_TRIAL_GAME_FINISHED_DAYS = 0;
    const int PROMOBROWSER_SUPPRESS_DOWNLOAD_UNDERWAY_DAYS = 0;

    inline bool isFreeTrialPromoType(const Origin::Client::PromoBrowserContext& context)
    {
        return context.promoType == Origin::Client::PromoContext::FreeTrialExited ||
            context.promoType == Origin::Client::PromoContext::FreeTrialExpired;
    }

    inline bool isFreeTrialViewOfferContext(const Origin::Client::PromoBrowserContext& context)
    {
        return isFreeTrialPromoType(context) &&
            context.scope != Origin::Client::PromoContext::NoScope;
    }
}

namespace Origin
{
    using namespace UIToolkit;

	namespace Client
    {
        // This is used for debugging the non-appearance of the promo manager.
        // We log each HTTP response and dump them all to the log file when
        // we're done, but only if ForcePromos is true.
        typedef struct PromoTraffic {
            int statusCode;
            QString url;
        } PromoTraffic;

        static QList<PromoTraffic> debugPromoTraffic;


        PromoBrowserViewController::PromoBrowserViewController(QWidget *parent)
        : QObject(parent)
            , mPromoWindow(NULL)
            , mNoPromoWindow(NULL)
            , mPendingPromoView(NULL)
			, mContext(PromoContext::NoType)
			, mPromoLoading(false)
            , mForcedToShow(false)
        {

        }


        PromoBrowserViewController::~PromoBrowserViewController()
        {
            closePromoDialog();
            closeNoPromoDialog();
            cleanupPromoRequestWebView();
        }

		
        void PromoBrowserViewController::show(const PromoBrowserContext& context, const Engine::Content::EntitlementRef ent, const bool force)
        {
            if(okToShowPromoDialog(context, ent, force))
            {
                mForcedToShow = force;
				mPromoLoading = true;
				mContext = context;
                mProductId.clear();

                if (ent)
                {
                    mProductId = ent->contentConfiguration()->productId();
                }

                // When a request is successful, we NULL out the web view. If
                // the web view is not null, then a request is still in 
                // progress, in which case we disconnect the webview and 
                // clean up its memory. In this way, we favor new requests 
                // over older ones.
                cleanupPromoRequestWebView();

                // The promo view object's memory will be freed by the promo
                // OriginWindow object when it's deleted.
                mPendingPromoView = new PromoView();

                ORIGIN_VERIFY_CONNECT(mPendingPromoView, &PromoView::loadFinished, this, &PromoBrowserViewController::promoDialogLoadResult);

                mPendingPromoView->load(context, ent);

                //  TELEMETRY:  Mark the start time of the promo dialog
                GetTelemetryInterface()->Metric_PROMOMANAGER_START(
                    mProductId.toUtf8().constData(),
                    PromoContext::promoTypeString(mContext.promoType).toUtf8().constData(), 
                    PromoContext::scopeString(mContext.scope).toUtf8().constData());

                if (PromoContext::userInitiatedContext(mContext))
                {
                    GetTelemetryInterface()->Metric_PROMOMANAGER_MANUAL_OPEN(
                        mProductId.toUtf8().constData(), 
                        PromoContext::promoTypeString(mContext.promoType).toUtf8().constData(), 
                        PromoContext::scopeString(mContext.scope).toUtf8().constData());
                }
            }
        }

        void PromoBrowserViewController::hide()
        {
            if(mPromoWindow != NULL && mPromoWindow->isVisible())
            {
                mPromoWindow->showMinimized();
            }
        }

		bool PromoBrowserViewController::ensurePromoBrowserVisible()
		{
			if(mPromoWindow == NULL)
			{
				return false;
			}
			else
			{
				mPromoWindow->showWindow();
				return mPromoWindow->isVisible();
			}
		}
        
        bool isTimeToShow(const int& lastJulianDay, const int& daysToWait)
        {
            QDate lastShowedPromo;
            if(lastJulianDay)
            {
                lastShowedPromo = QDate::fromJulianDay(lastJulianDay); 
            }

            return (!lastShowedPromo.isValid() || lastShowedPromo.daysTo(QDate::currentDate()) >= daysToWait);
        }


        bool PromoBrowserViewController::okToShowPromoDialog(const PromoBrowserContext& context, const Engine::Content::EntitlementRef ent, const bool force)
        {
            bool retval = false;
            int overridePromos = Services::readSetting(Services::SETTING_OverridePromos);

            // We can't load two promos at once, let the previous one finish.
            if(mPromoLoading)
            {
                ORIGIN_LOG_ACTION << "Not showing - Promo loading";
                return false;
            }

            // Can't load promos while offline.
            if(!Services::Connection::ConnectionStatesService::isUserOnline (Engine::LoginController::currentUser()->getSession()))
            {
                ORIGIN_LOG_ACTION << "Not showing - User offline";
                return false;
            }

#if defined(ENABLE_SUBS_TRIALS)
            // SERVICE DEPENDENCY need to go before forced: Subscription is enabled, but the status hasn't been received
            if(Engine::Subscription::SubscriptionManager::instance()->enabled() && Engine::Subscription::SubscriptionManager::instance()->isTrialEligible() == -1)
            {
                ORIGIN_LOG_ACTION << "Not showing - subscription trial status not available yet";
                return false;
            }
#endif

            // SERVICE DEPENDENCY need to go before forced: NPS is visible or the service response hasn't come yet
            if(NetPromoterViewController::isSurveyVisible() != 0)
            {
                ORIGIN_LOG_ACTION << "Not showing - NPS is visible. Status: " << QString::number(NetPromoterViewController::isSurveyVisible());
                return false;
            }


            // Always show the promo, over every other behaviour, if we're
            // being triggered via the debug menu or if there's an eacore.ini
            // setting to always show promos.  Additionally the MenuPromotions context
			// always shows the promo if available.
            if(force || PromoContext::userInitiatedContext(context) || overridePromos == Services::PromosEnabled)
            {
                ORIGIN_LOG_ACTION << "Showing - forced";
                retval = true;
            }
            else
            {
                // We don't show the promo in the following cases:
                // In addition the code calling us must account for these cases:
                // DownloadStarted promo but the download is less than 500MB
                // GameFinished promo but the game crashed

                // Existing promo window open or loading
                if(mPromoWindow)
                {
                    ORIGIN_LOG_ACTION << "Not showing - Existing promo window open or loading";
                    return false;
                }

                // Non-production environment and disabled via ea.core.ini
                if(nonProductionPromosDisabled())
                {
                    ORIGIN_LOG_ACTION << "Not showing - disabled";
                    return false;
                }

                // No valid user (we've been called while client is logged out)
                if(Engine::LoginController::currentUser().isNull())
                {
                    ORIGIN_LOG_ACTION << "Not showing - No valid user";
                    return false;
                }

                // Underage user (no access to store)
                if(Engine::LoginController::currentUser()->isUnderAge())
                {
                    ORIGIN_LOG_ACTION << "Not showing - Underage";
                    return false;
                }

                // Client started as part of the RtP flow (only affects login promo)
                if(context.promoType == PromoContext::OriginStarted && MainFlow::instance() && MainFlow::instance()->rtpFlow() && MainFlow::instance()->rtpFlow()->isPending())
                {
                    ORIGIN_LOG_ACTION << "Not showing - started as part of the RtP flow";
                    return false;
                }

                // Client started as part of the ITE flow (only affects login promo)
                if(context.promoType == PromoContext::OriginStarted && OriginApplication::instance().isITE())
                {
                    ORIGIN_LOG_ACTION << "Not showing - Client started as part of the ITE flow";
                    return false;
                }

                // MOTD is visible
                if(MOTDViewController::isBrowserVisible())
                {
                    ORIGIN_LOG_ACTION << "Not showing - MOTD is visible";
                    return false;
                }

                PendingActionFlow *actionFlow = PendingActionFlow::instance();
                if (actionFlow && actionFlow->suppressPromoState())
                {
                    ORIGIN_LOG_ACTION << "Not showing - Suppressed via localhost/origin2:// command";
                    return false;
                }

                const Services::Setting *setting=NULL;
                settingsInfoFromContext(context, &setting);

                switch (context.promoType)
                {
                case PromoContext::OriginStarted:
                default:
                    if(static_cast<bool>(Services::readSetting(Services::SETTING_DISABLE_PROMO_MANAGER)))
                        retval = false;
                    else
                    {
                        retval = isTimeToShow(Services::readSetting(*setting, Services::Session::SessionService::currentSession()), PROMOBROWSER_SUPPRESS_ORIGIN_STARTED_DAYS);
                    }
                    break;

                case PromoContext::GameFinished:
                    // If we don't get an entitlement we should err on the
                    // side of showing the promo. We might deliberately
                    // have been passed a NULL (eg. from the debug menu).
                    if (!ent || 
                        ent->contentConfiguration()->isFreeTrial() || 
                        ent->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeAlpha || 
                        ent->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeBeta ||
                        ent->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeDemo)
                    {
                        retval = isTimeToShow(Services::readSetting(*setting, Services::Session::SessionService::currentSession()), PROMOBROWSER_SUPPRESS_TRIAL_GAME_FINISHED_DAYS);
                    }
                    else
                    {
                        if(static_cast<bool>(Services::readSetting(Services::SETTING_DISABLE_PROMO_MANAGER)))
                            retval = false;
                        else
                        {
                            retval = isTimeToShow(Services::readSetting(*setting, Services::Session::SessionService::currentSession()), PROMOBROWSER_SUPPRESS_GAME_FINISHED_DAYS);
                        }
                    }
                    break;

                case PromoContext::DownloadUnderway:
                    if(static_cast<bool>(Services::readSetting(Services::SETTING_DISABLE_PROMO_MANAGER)))
                        retval = false;
                    else
                    {
                        // All 'while downloading' promos show all the time, so
                        // that free trials get maximum exposure.
                        retval = isTimeToShow(Services::readSetting(*setting, Services::Session::SessionService::currentSession()), PROMOBROWSER_SUPPRESS_DOWNLOAD_UNDERWAY_DAYS);
                    }
                    break;

                case PromoContext::FreeTrialExited:
                case PromoContext::FreeTrialExpired:
                    retval = true;
                    break;
                }
            }

            return retval;
        }

        bool PromoBrowserViewController::nonProductionPromosDisabled()
        {
            bool retval = false;
            int overridePromos = Services::readSetting(Services::SETTING_OverridePromos);
            QString envStr = Services::readSetting(Origin::Services::SETTING_EnvironmentName, Origin::Services::Session::SessionService::currentSession()).toString();

            if (envStr.compare("production", Qt::CaseInsensitive) != 0 && overridePromos == Services::PromosDisabledNonProduction)
            {
                retval = true;
            }

            return retval;
        }

        void PromoBrowserViewController::onUserClosePromoDialog()
        {
            if (mPromoWindow)
            {
                //  TELEMETRY:  Mark the end time of the promo dialog and send event only if promo was shown
                GetTelemetryInterface()->Metric_PROMOMANAGER_END(
                    mProductId.toUtf8().constData(),
                    PromoContext::promoTypeString(mContext.promoType).toUtf8().constData(), 
                    PromoContext::scopeString(mContext.scope).toUtf8().constData());
            }

            //  Continue with regular close promo logic
            closePromoDialog();
        }

        void PromoBrowserViewController::closePromoDialog()
        {
            // Clear out our context only if we're not showing the "no promo"
            // dialog (otherwise we'll need the context info for telemetry).
            if (!mNoPromoWindow)
            {
			    mContext = PromoContext::NoType;
            }

			mPromoLoading = false;

            if(mPromoWindow)
            {
				ORIGIN_VERIFY_DISCONNECT(mPromoWindow, SIGNAL(rejected()), this, SLOT(onUserClosePromoDialog()));
                ORIGIN_VERIFY_DISCONNECT(mPromoWindow, SIGNAL(rejected()), this, SLOT(closePromoDialog()));
                mPromoWindow->close();
                mPromoWindow = NULL;
            }
        }
        
        void PromoBrowserViewController::closeNoPromoDialog()
        {
            if(mNoPromoWindow)
            {
                mNoPromoWindow->deleteLater();
                mNoPromoWindow = NULL;
            }
        }

        void PromoBrowserViewController::onViewProductInStore()
        {
            GetTelemetryInterface()->Metric_PROMOMANAGER_MANUAL_VIEW_IN_STORE(
                mProductId.toUtf8().constData(),
                PromoContext::promoTypeString(mContext.promoType).toUtf8().constData(), 
                PromoContext::scopeString(mContext.scope).toUtf8().constData());
            
            Client::ClientFlow::instance()->showProductIDInStore(mProductId);
        }

		void PromoBrowserViewController::showNoPromosAvailable()
		{
            if(mNoPromoWindow == NULL)
            {
                using namespace Origin::UIToolkit;
                QString titleBar, title, description;
                switch (mContext.promoType)
                {
                case PromoContext::FreeTrialExited:
                case PromoContext::FreeTrialExpired:
                    {
                        // For free trial promotions with scope, we assume the
                        // user is trying to view a "special offer" so we title the window
                        // as such.
                        if (mContext.scope != PromoContext::NoScope)
                        {
                            titleBar = tr("ebisu_client_special_offer");
                            title = tr("ebisu_client_no_offers_today_bold");
                            description = tr("ebisu_client_no_special_offers_text");
                            break;
                        }
    
                        // Fall through if it's a free trial and there's no additional scope.
                        // There's no particular reason for this - there's currently no use 
                        // case for showing "no promos available" window for "free trial 
                        // exited/expired" context without a scope.
                    }
                default:
                    {
                        titleBar = tr("ebisu_client_promo_title_menu_promotions");
                        title = tr("menu_promotions_error_title");
                        description = tr("menu_promotions_error_description2");
                        break;
                    }
                }
    
                // When the user has selected to view a free trial offer, we give them an additional
                // option in the "no promos" dialog to view the product in the store.
                if(isFreeTrialViewOfferContext(currentPromoContext()))
                {
                    mNoPromoWindow = OriginWindow::promptNonModal(OriginMsgBox::NoIcon, title, description, tr("ebisu_client_buy_now"), tr("ebisu_client_close"), QDialogButtonBox::Yes);
                    ORIGIN_VERIFY_CONNECT(mNoPromoWindow->button(QDialogButtonBox::Yes), SIGNAL(clicked()), this, SLOT(onViewProductInStore()));
                }
                else
                {
                    mNoPromoWindow = OriginWindow::alertNonModal(OriginMsgBox::NoIcon, title, description, tr("ebisu_client_close"));
                }
        
                mNoPromoWindow->setTitleBarText(titleBar);
                ORIGIN_VERIFY_CONNECT(mNoPromoWindow, SIGNAL(finished(int)), this, SLOT(closeNoPromoDialog()));
            }
            mNoPromoWindow->showWindow();
		}

        void PromoBrowserViewController::promoDialogLoadResult(const int httpCode, const bool loadSuccessful)
        {
            logDebugPromoTraffic();

			mPromoLoading = false;

            ORIGIN_LOG_EVENT << "Page loaded: " << loadSuccessful << "(status=" << httpCode << ")";

            if (PromoContext::userInitiatedContext(mContext))
            {
	            GetTelemetryInterface()->Metric_PROMOMANAGER_MANUAL_RESULT(
                    httpCode,
                    mProductId.toUtf8().constData(),
                    PromoContext::promoTypeString(mContext.promoType).toUtf8().constData(), 
                    PromoContext::scopeString(mContext.scope).toUtf8().constData());
            }

            if(httpCode == 200 && loadSuccessful && mPendingPromoView)
            {
                if(mPromoWindow == NULL)
                {
                    mPromoWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close, mPendingPromoView, OriginWindow::WebContent);
                    mPromoWindow->setContentMargin(0);

                    // Close if the user presses the close button
                    ORIGIN_VERIFY_CONNECT(mPromoWindow, SIGNAL(rejected()), this, SLOT(onUserClosePromoDialog()));

                    // Hide promo dialog when user begins playing a game
                    ORIGIN_VERIFY_CONNECT(Engine::Content::ContentController::currentUserContentController(), SIGNAL(playStarted(Origin::Engine::Content::EntitlementRef, bool)), this, SLOT(hide()));

                    // Close window when page calls window.promoManager.close() JS helper
                    ORIGIN_VERIFY_CONNECT(mPendingPromoView, &PromoView::closeWindow, mPromoWindow, &OriginWindow::close);
                    ORIGIN_VERIFY_CONNECT(mPendingPromoView, &PromoView::closeWindow, mPromoWindow, &OriginWindow::deleteLater);
                }
                else
                {
                    mPromoWindow->setContent(mPendingPromoView);
                    mPromoWindow->webview()->updateSize();
                }

                // Now that this webview has been serviced, removing any references to it,
                // but don't delete it since it's an active dialog.
                cleanupPromoRequestWebView(false);

                // Set title text once the request is completed in case the user already has the 
                // promo window open (from a previous context) with a different title. We don't
                // set the title text up front because we're not sure if the request will succeed.
                setPromoWindowTitleText();

                // Minimize the window if:
                // 1. we are auto starting from windows and have auto login checked.
                // 2. the user is playing a game.
                const bool quietModeEnabled = (OriginApplication::instance().startupState() != StartNormal && Engine::LoginController::isCurrentSessionAutoLogin()) ||
                    !Engine::Content::ContentController::currentUserContentController()->entitlementByState(0, Engine::Content::LocalContent::Playing).isEmpty();
                if(mForcedToShow == false && quietModeEnabled)
                {
                    //after switching to Qt5, we no longer are able to call showMinimized directly
                    //we seem to need to call show first and then call showMinimized in the next event loop
                    //this will cause a flash of the promo before being minimized but it's an acceptable hack for now.
                    mPromoWindow->show();
					QTimer::singleShot(1000, this, SLOT(onShowMinimized()));
                }
                else
                {	
					if(mPromoWindow->isVisible())
					{					
						mPromoWindow->activateWindow();
						mPromoWindow->showWindow();
					}
					else
					{
						// Move the promo a bit to the side so we don't completely
						// cover the main Origin window.
						int promoWidth = mPromoWindow->width();
						int promoHeight = mPromoWindow->height();

						QRect availableSize = QDesktopWidget().availableGeometry();
						int screenWidth = availableSize.width();
						int screenHeight = availableSize.height();

						int xPos = ((screenWidth * 2) / 3) - (promoWidth / 2);
						int yPos = (screenHeight / 2) - (promoHeight / 2);

						mPromoWindow->move(xPos, yPos);
                        mPromoWindow->showWindow();
					}
                }

                // Update the setting that keeps track of the time that the promo browser was last shown, for a particular context
                const Services::Setting *setting=NULL;
                settingsInfoFromContext(mContext, &setting);
                Services::writeSetting(*setting, QDate::currentDate().toJulianDay(), Services::Session::SessionService::currentSession());

				emit promoBrowserLoadComplete(mContext, true);
            }
            else
			{
				// If the user requested the promo via menu, we show a dialog that says no promos are available
                if(PromoContext::userInitiatedContext(mContext))
				{
					showNoPromosAvailable();
				}
                
                // If the request failed and the user doesn't have an existing successful promo open, clean up the failed promo.
                if(mPromoWindow && mPromoWindow->isHidden())
                {
                    closePromoDialog();
                }

                // Clean-up the webview associated with the failed request.
                cleanupPromoRequestWebView();

				emit promoBrowserLoadComplete(mContext, false);
			}
        }

        void PromoBrowserViewController::onShowMinimized ()
        {
            mPromoWindow->showMinimized();
        }

        void PromoBrowserViewController::logDebugPromoTraffic(void)
        {
            int overridePromos = Services::readSetting(Services::SETTING_OverridePromos);

            // Only write the traffic to the log if ForcePromos=1 in eacore.ini
            // so we don't spam everybody's logs all the time.
            if (overridePromos == Services::PromosEnabled)
            {
                ORIGIN_LOG_EVENT << "Promo traffic: " << debugPromoTraffic.size();

                QListIterator<PromoTraffic> iter(debugPromoTraffic);

                while (iter.hasNext())
                {
                    PromoTraffic pt = iter.next();
                    ORIGIN_LOG_EVENT << "Response: " << pt.statusCode << " URL: " << pt.url;
                }
            }
        }

        void PromoBrowserViewController::settingsInfoFromContext(const PromoBrowserContext& context, const Services::Setting** setting)
        {
            switch (context.promoType)
            {
            case PromoContext::OriginStarted:
            default:
                (*setting) = &Services::SETTING_PROMOBROWSER_ORIGINSTARTED_LASTSHOWN;
                break;

            case PromoContext::GameFinished:
                (*setting) = &Services::SETTING_PROMOBROWSER_GAMEFINISHED_LASTSHOWN;
                break;

            case PromoContext::DownloadUnderway:
                (*setting) = &Services::SETTING_PROMOBROWSER_DOWNLOADUNDERWAY_LASTSHOWN;
                break;
            }
        }

        void PromoBrowserViewController::setPromoWindowTitleText()
        {
            switch (mContext.promoType)
            {
            case PromoContext::OriginStarted:
            default:
                mPromoWindow->setTitleBarText(tr("ebisu_client_promo_title_login"));
                break;

            case PromoContext::MenuPromotions:
                mPromoWindow->setTitleBarText(tr("ebisu_client_promo_title_menu_promotions"));
                break;

            case PromoContext::GameFinished:
                mPromoWindow->setTitleBarText(tr("ebisu_client_promo_title_endgame"));
                break;

            case PromoContext::FreeTrialExited:
            case PromoContext::FreeTrialExpired:
            {
                Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(mProductId);
                const Services::Publishing::ItemSubType itemSubType = (ent && ent->contentConfiguration()) ? ent->contentConfiguration()->itemSubType() : Services::Publishing::ItemSubTypeUnknown;
                if (itemSubType == Services::Publishing::ItemSubTypeTimedTrial_Access || itemSubType == Services::Publishing::ItemSubTypeTimedTrial_GameTime)
                {
                    mPromoWindow->setTitleBarText(tr("ebisu_subs_trial_window_launch_window_title"));
                }
                else
                {
                    mPromoWindow->setTitleBarText(tr("ebisu_client_special_offer"));
                }
                break;
            }

            case PromoContext::DownloadUnderway:
                mPromoWindow->setTitleBarText(tr("ebisu_client_promo_title_download"));
                break;
            }
        }

        void PromoBrowserViewController::cleanupPromoRequestWebView(bool deleteWebView)
        {
            if (mPendingPromoView)
            {
                ORIGIN_VERIFY_DISCONNECT(mPendingPromoView, &PromoView::loadFinished, this, &PromoBrowserViewController::promoDialogLoadResult);

                if (deleteWebView)
                {
                    mPendingPromoView->deleteLater();
                }

                mPendingPromoView = NULL;
            }
        }
    }
}

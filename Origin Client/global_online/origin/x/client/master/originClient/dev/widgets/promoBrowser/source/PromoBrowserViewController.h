/////////////////////////////////////////////////////////////////////////////
// PromoBrowserViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef PROMO_BROWSER_VIEW_CONTROLLER_H
#define PROMO_BROWSER_VIEW_CONTROLLER_H

#include <QObject>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "PromoContext.h"
#include "engine/content/ContentTypes.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace UIToolkit
    {
    class OriginWindow;
    }
    namespace Services
    {
    class Setting;
    }
    namespace Client
    {
        class PromoView;

        /// \brief Controller for promo windows. Helps load promo web pages,
        /// display, and log debug info.
        class ORIGIN_PLUGIN_API PromoBrowserViewController : public QObject
        {
            Q_OBJECT

        public:
            /// \brief Constructor
            /// \param parent The parent of the PromoBrowser - shouldn't be used.
            PromoBrowserViewController(QWidget* parent = 0);

            /// \brief Destructor - calls closePromoDialog()
            ~PromoBrowserViewController();

            /// \brief Shows the promo window.
            /// \param context Context to show - origin started, game finished, download underway.
            /// \param ent Pointer to the entitlement that the promo applies to (if any).
            /// \param force Forces the window to show. Bypasses any "should show" checks.
            void show(const PromoBrowserContext& context, const Engine::Content::EntitlementRef ent, const bool force);

            /// \brief Returns if promo browser is currently attempting to load.  If returning true
            ///    caller can listen for promoBrowserLoadComplete to take action...
            /// \return Whether promo browser is currently loading
            bool isPromoBrowserLoading() { return mPromoLoading; }

            /// \brief If promo browser is currently up, raises it and returns true, otherwise returns false indicating
            /// the promo browser is not currently visible.
            /// \return Whether promo browser is currently visible.
            bool ensurePromoBrowserVisible();

            /// \brief Returns current promo browser context
            /// \return Current promo browser context.
            const PromoBrowserContext currentPromoContext() const { return mContext; }

        signals:
            void promoBrowserLoadComplete(PromoBrowserContext context, bool showedPromo);

        protected slots:
            /// \brief Reaction for when the user presses X in window titlebar.
            /// Records telemetry and calls closePromoDialog.
            void onUserClosePromoDialog();

            /// \brief Reaction when the user presses the "view in store" button in
            /// the dialog that tells the user there are no free trial offers available.
            /// Only applies when the user attempts to view a free trial offer but there
            /// are no offers configured for the given product.
            void onViewProductInStore();

            /// \brief Closes the promo window.
            void closePromoDialog();

            /// \brief Closes the "no promos available" dialog.
            void closeNoPromoDialog();
            
            /// \brief Minimizes the promo window.
            void hide();

            /// \brief Called when the web page is loaded.
            ///
            /// Only shows the promo window if the web page loaded successfully.
            /// \param httpCode The HTTP code returned by the server.
            /// \param loadSuccessful The web page's load result: Success = true.
            void promoDialogLoadResult(const int httpCode, const bool loadSuccessful);

            void onShowMinimized();

        private:
            /// \brief Checks to see if a promo window can be shown. This is 
            /// dependent on the what's going on with Origin 
            /// (eg. user saw this promo recently, MOTD is visible...).
            /// \param context Context in which the promo is shown.
            /// \param ent TBD.
            /// \param force Forces the window to show. Bypasses any "should show" checks.
            bool okToShowPromoDialog(const PromoBrowserContext& context, const Engine::Content::EntitlementRef ent, const bool force);

            /// \brief Checks if there's an eacore.ini setting to disable
            /// promos in non-production environments and if so, whether
            /// we're in a non-production environment.
            /// 
            /// The name has two
            /// negatives so it matches the eacore.ini syntax and because
            /// it's ONLY production environment that is unaffected
            bool nonProductionPromosDisabled();

            /// \brief Gets information about when a promo window was last 
            /// shown in that context from the user settings.
            /// \param context Context in which the promo should be shown.
            /// \param setting Returns the date of when promo was last shown
            /// in the given content.
            void settingsInfoFromContext(const PromoBrowserContext& context, const Origin::Services::Setting** setting);

            /// \brief Logs promo traffic if ForcePromos=true in EACore.ini
            void logDebugPromoTraffic(void);

            /// \brief Shows user error message that there were no promos available if promo fails to load for any reason
            void showNoPromosAvailable(void);

            /// \brief Sets the title text for the promo window based on the promo's context.
            void setPromoWindowTitleText();

            /// \brief Disconnects signals and frees up memory associated with the promo request web view.
            /// \sa PromoBrowserViewController::mPendingPromoView
            void cleanupPromoRequestWebView(bool deleteWebView = true);

            UIToolkit::OriginWindow* mPromoWindow;
            UIToolkit::OriginWindow* mNoPromoWindow;

            /// Contains the WebView for the promo manager response.
            /// While the promo manger is displayed and there are no active
            /// promo manager requests, this web view will be the same as the
            /// PromoView associated with the promo window's OriginWindow
            /// object. 
            ///
            /// However, when a new promo manager request is made, a new 
            /// PromoView object is constructed and assigned to this 
            /// pointer. This variable is then used to request and store the
            /// new promo content, which is preferred to use rather than the
            /// OriginWindow's WebView directly because the user may have a
            /// promo dialog already open, in which case if the request were
            /// to fail for some reason, the failed content would be 
            /// shown in the already-visible promo window. We avoid showing
            /// failed content to the user in this case by holding a pointer
            /// to a new PromoView object (via this member variable), 
            /// only setting it as the "content" of the promo OriginWindow if
            /// the request is successful.
            PromoView* mPendingPromoView;

            PromoBrowserContext mContext;
            bool mPromoLoading;
            bool mForcedToShow;

            /// \brief Stores the product ID if the promo manager was invoked for a specific entitlement.
            QString mProductId;
        };
    }
}

#endif

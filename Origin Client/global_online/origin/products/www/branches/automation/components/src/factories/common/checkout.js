/**
 * @file Checkout.js
 */
(function() {
    'use strict';

    var DOWNLOAD_PAGE_ROUTE = 'app.store.wrapper.download',
        GAME_LIBRARY_ROUTE = 'app.game_gamelibrary',
        IFRAME_CONTEXT_NAMESPACE = 'origin-iframe',
        EXTERNAL_WINDOW_CLOSE_PING_TIME = 500,
        PLATFORM_CLIENT = 'CLIENT',
        PLATFORM_WEB = 'WEB';

    function CheckoutFactory($q, NavigationFactory, UrlHelper, ComponentsConfigHelper, LocaleFactory, COMPONENTSCONFIG, UtilFactory, ErrorModalFactory, OriginCheckoutTypesConstant, LocalStorageFactory, SpaCheckoutFactory, OigCheckoutFactory, WishlistFactory, ArrayHelperFactory, PdpFactory, GamesEntitlementFactory) {
        var isOigContext = ComponentsConfigHelper.isOIGContext(),
            subfactory = isOigContext ? OigCheckoutFactory : SpaCheckoutFactory,
            externalWindow,
            externalWindowClose,
            cachedInvoiceSource,
            checkoutPromise;

        /**
         * Handles redirect to external pages outside of the SPA
         * @param  {string} uri The uri of the link to be opened.
         */
        function openLink(uri) {
            NavigationFactory.externalUrl(decodeURIComponent(uri));
        }

        /**
         * Opens a modal on the successful purchase of offer(s). Directs user to download origin
         * or download the games depending.
         * @param  {Array} offerIds List of offerIds that you want to confirm the user is going to download
         */
        function openCheckoutConfirmation(offerIds) {
            subfactory.openCheckoutConfirmation(offerIds);
        }

        /**
         * Handles the 'downloadOrigin' postmessage event. Redirects user to download page.
         */
        function downloadOrigin() {
            subfactory.close();
            NavigationFactory.goToState(DOWNLOAD_PAGE_ROUTE);
        }

        /**
         * Handles the 'downloadGame' postMessage event. Starts the download of a given offer and redirects user to Game Library.
         * @param {string}      offerId     The offerId of the product to be downloaded.
         */
        function downloadGame(offerId) {
            // start game download
            Origin.client.games.startDownload(offerId);

            // check we are in the client
            if(Origin.client.isEmbeddedBrowser()){
                // redirect to game library
                NavigationFactory.goToState(GAME_LIBRARY_ROUTE);
                subfactory.close();
            }
        }

        /**
         * Handles the 'backToGame' postMessage event.  Closes OIG and the checkout window.
         */
        function backToGame() {
            subfactory.close();
        }

        /**
         * Handle successful purchase of a subscription offer
         * @param  {string} checkoutType type of checkout, to determine if this is for subs
         */
        function handleCheckoutSuccess(checkoutType) {
            if (Origin.client.isEmbeddedBrowser() && isOigContext) {
                Origin.client.oig.purchaseComplete();
            }
            if (checkoutType === OriginCheckoutTypesConstant.SUBS) {
                // reset nux to first stage
                LocalStorageFactory.delete('storeAccessNuxStage');
                Origin.events.fire(Origin.events.DIRTYBITS_SUBSCRIPTION);
                subfactory.showSubsNux();
            }
        }

        /**
         * Build invoiceSource for the default checkout type.
         * @return {string}        invoiceSource string
         */
        function buildInvoiceSource(invoiceSource) {
            if (invoiceSource && _.isString(invoiceSource)) {
                return invoiceSource;
            } else {
                var base = ComponentsConfigHelper.getSetting('invoiceSourceBase'),
                    platform = Origin.client.isEmbeddedBrowser() ? PLATFORM_CLIENT : PLATFORM_WEB,
                    country = Origin.locale.countryCode();
                return  [base, platform, country].join('-').toUpperCase();
            }
        }

        /**
         * Send marketing telemetry event
         * @param {Object} args Object container for the parameters to call sendMarketingEvent in jssdk
         */
        function sendMarketingEvent(args) {
            var params = _.assign({ isCheckout: true }, args.parameters);
			Origin.telemetry.sendMarketingEvent(args.category, args.action, args.label, args.value, params, args.pinParameters);
        }

        /**
         * Send custom dimension telemetry event
         * @param {Object} args Object container for the parameters to call setCustomDimension in jssdk
         */
        function setCustomDimension(args) {
            Origin.telemetry.setCustomDimension(args.dimension, args.data, { isCheckout: true });
        }

        /**
         * Send transaction telemetry event
         * @param {Object} args Object container for the parameters to call sendTransactionEvent in jssdk
         */
        function sendTransactionEvent(args) {
            var receipt = args.receipt;
            var items = [];
            var txId = receipt.orderNumber;
            var revenue = parseFloat(Origin.utils.getProperty(receipt, ['totalPrice'])) || 0;
            var tax = Origin.utils.getProperty(receipt, ['taxInfo', 'tax']) || 0;
            var currency = Origin.utils.getProperty(receipt, ['currency']) || 'USD';

            _.forEach(receipt.products, function(item) {
                var type = Origin.utils.getProperty(item, ['type']);
                var category = type || 'Unknown';
                var unitPrice = Origin.utils.getProperty(item, ['totalPriceWithoutTax']) || 0;
                var revenueModel = category;
                items.push(new Origin.telemetry.TransactionItem(
                   item.offerId,
                   category,
                   category,
                   item.name,
                   unitPrice,
                   revenueModel
                ));
            });

            Origin.telemetry.sendTransactionEvent(txId, null, currency, revenue, tax, items);
        }

        function combineOfferIds(){
            var offerIds = ArrayHelperFactory.concat.apply(this, arguments);
            return offerIds;
        }
        /**
         * Gets a list of lower edition offer IDs by a list of product object containing offer ID
         * @param {object[]} products
         * @returns {Promise<string[]>}
         */
        function getLowerEditionOfferIdsByProducts(products){
            var ocdResolvePromises = _.map(products, function(product){
                return PdpFactory.getLowerEditionBaseGameOfferIdsByOffer(product.offerId);
            });
            return Promise.all(ocdResolvePromises).then(_.spread(combineOfferIds));
        }

        /**
         * Update wish by removing any lower edition that has just been purchased
         *
         * @param {object} args
         */
        function updateWishlist(checkoutType, args){
            //Don't update wishlist if gifting
            if (checkoutType === OriginCheckoutTypesConstant.GIFT){
                return;
            }
            var receipt = args.receipt,
                products = receipt.products || [],
                wishlist = WishlistFactory.getWishlist(Origin.user.userPid()),
                wishlistOffersPromise = wishlist.getOfferList(),
                lowerEditionSiblingOfferIdPromise = getLowerEditionOfferIdsByProducts(products);
            Promise.all([wishlistOffersPromise, lowerEditionSiblingOfferIdPromise]).then(_.spread(function(wishlistOfferList, lowerEditionSiblingOfferIds){
                var wishlistOfferIds = _.pluck(wishlistOfferList, 'id');
                _.forEach(lowerEditionSiblingOfferIds, function(offerId){
                    if (wishlistOfferIds.indexOf(offerId) > -1){
                        wishlist.removeOffer(offerId);
                    }
                });
            }));
        }

        /**
         * Build Checkout URL
         * @param  {string} type           checkout type - "subs" or "default"
         * @param  {string} checkoutParams Optional parameters to use when building the checkout URL
         * @return {string}                checkout URL
         */
        function getCheckoutUrl(type, checkoutParams) {
            var params,
                partnerIdObject,
                endPoint;

            params = {
                gameId: ComponentsConfigHelper.getSetting('gameId')[type],
                cartName: ComponentsConfigHelper.getCartName(),
                locale: Origin.locale.locale(),
                currency: LocaleFactory.getCurrency(Origin.locale.countryCode()),
                virtualCurrency: checkoutParams.virtualCurrency,
                countryCode: Origin.locale.countryCode(),
                txnToken: checkoutParams.txnToken,
                category: checkoutParams.category,
                invoiceSource: checkoutParams.invoiceSource || buildInvoiceSource(checkoutParams.invoiceSource)
            };
            partnerIdObject = {
                '{partnerId}': ComponentsConfigHelper.getSetting('partnerId')
            };
            endPoint = UrlHelper.buildConfigUrl('checkout', type, partnerIdObject);

            if (type === OriginCheckoutTypesConstant.CHAINED) {
                params.offerId = checkoutParams.offerId;
            }

            // cache invoiceSource for use in third party payment flow
            cachedInvoiceSource = params.invoiceSource;

            return UrlHelper.buildParameterizedUrl(endPoint, params);
        }

        /**
         * Validates the given checkout type.
         * @param   {string} type The checkout type
         * @returns {string} The validated checkout type.
         */
        function validateCheckoutType(type) {
            return _.isString(type) ? type : OriginCheckoutTypesConstant.DEFAULT;
        }

        /**
         * Validates the set of checkout params.
         * @param   {Object} params The parameter object to validate
         * @returns {checkoutParams} The validated checkout parameters.
         */
        function validateCheckoutParams(params) {
            params = params || {};

            params.txnToken         = _.isString(params.txnToken) ? params.txnToken : undefined;
            params.virtualCurrency  = _.isString(params.virtualCurrency) ? params.virtualCurrency : undefined;
            params.category         = _.isString(params.category) ? params.category : undefined;
            params.invoiceSource    = _.isString(params.invoiceSource) ? params.invoiceSource : undefined;
            params.offerId          = _.isString(params.offerId) ? params.offerId : undefined;

            return params;
        }

        // Remove references to previous payment provider redirects
        function cleanUpPreviousRedirects() {
            externalWindow = undefined;
            if (externalWindowClose) {
                clearInterval(externalWindowClose);
            }
            window.paymentProviderCompleteCallback = undefined;
        }

        /**
         * @typedef checkoutParams
         * @type {object}
         * @property {string} txnToken        Transaction token with which to open the checkout flow. Ex. Paypal redirect txnToken.
         * @property {string} virtualCurrency The three letter virtual currency code (if any), i.e. '_BW'
         * @property {string} category        The category ID to use during Lockbox topup flow checkout
         * @property {string} invoiceSource   String that identifies source of transaction (the name of the game integrator)
         */

        /**
         * Opens the checkout iframe which start checkout process. Returns a promise that resolves when checkout flow has completed successfully.
         * Rejects the promise if checkout flow has errored out in any way.
         * @param   {string}         type                Type of checkout modal to open
         * @param   {Object} params              Optional parameters to use when building the checkout URL
         * @returns {Promise}        Promise that resolves when checkout flow send 'paymentsuccess' event. Rejects when checkout sends 'error' event.
         */
        function checkout(type, params) {
            var iFrameSourceUrl,
                iFrameOriginUrl;

            Origin.performance.endTimeMeasureAndSendTelemetry('COMPONENTSPERF:checkoutCart');
            Origin.performance.beginTime('COMPONENTSPERF:loadLockBox');

            cleanUpPreviousRedirects();

            type = validateCheckoutType(type);
            params = validateCheckoutParams(params);
            iFrameSourceUrl = getCheckoutUrl(type, params);
            iFrameOriginUrl = UrlHelper.buildConfigUrl('checkout');
            checkoutPromise = $q.defer();

            subfactory.checkout(type, iFrameSourceUrl, iFrameOriginUrl);

            return checkoutPromise.promise
                .then(_.partial(GamesEntitlementFactory.retrieveEntitlements, true))
                .then(_.partial(handleCheckoutSuccess, type));
        }

        /**
         * Opens the direct checkout directive for the given offer.  Only used when purchasing an item priced in virtual currency when the user has a sufficient balance.
         * @param   {string} offerId  The offer ID to purchase
         * @returns {Promise}
         */
        function checkoutUsingBalance(offerId) {
            Origin.performance.endTimeMeasureAndSendTelemetry('COMPONENTSPERF:checkoutCart');

            subfactory.checkoutUsingBalance(offerId);

            checkoutPromise = $q.defer();
            return checkoutPromise.promise;
        }

        /**
         * Prepares the third party payment provider flow by opening a blank external window.
         * Note, this is called immediately by lockbox so that the browser doesn't block the popup.
         * Lockbox will then take a few moments to generate the payment provider redirect uri, at which
         * point paymentProviderRedirectReady will be called to load the payment provider.
         */
        function paymentProviderRedirectPrepare() {
            // Open external window
            externalWindow = window.open(UrlHelper.getWindowOrigin() + COMPONENTSCONFIG.checkout.loading);
            externalWindow.Origin = externalWindow.Origin || {};
            externalWindow.Origin.loadingTitle = UtilFactory.getLocalizedStr(false, IFRAME_CONTEXT_NAMESPACE, 'loading-title');
            externalWindow.Origin.loadingMessage = UtilFactory.getLocalizedStr(false, IFRAME_CONTEXT_NAMESPACE, 'loading-message');
            if(externalWindow.Origin.renderText) {
                externalWindow.Origin.renderText();
            }
        }

        /*
        * Loads the payment provider external window with the correct uri.
        * Closes the current checkout iframe modal and replaces it with a message to the user to
        * complete the flow in the external window.
        * Sets a interval to ping the external window and see if the user has manually closed it.
        * On return, cleans up the interval, closes the dialog and opens a new checkout iframe with optional return txnToken.
        * @param {string}   type        The type of the checkout flow (default/subs etc.)
        * @param {uri}      uri         The uri of the payment provider that is to be opened
        */
        function paymentProviderRedirectReady(type, uri) {

            var callbackRedirectUrl = UrlHelper.getWindowOrigin() + COMPONENTSCONFIG.checkout.paymentProviderRedirect,
                paymentProviderUrl = UrlHelper.buildParameterizedUrl(decodeURIComponent(uri), {redirectUri: callbackRedirectUrl});

            /**
             * Callback function to be called when the payment provider has returned or errored out
             * @param txnToken The transaction token passed back by the payment provider
             */
            function paymentProviderComplete (txnToken) {
                subfactory.closeRedirectMessage();

                cleanUpPreviousRedirects();

                checkout(type, {invoiceSource: cachedInvoiceSource, txnToken: txnToken})
                    .catch(ErrorModalFactory.showDialog);
            }

            /**
             * Checks to see if the external payment provider window has been manually closed.
             * Helper method for paymentProviderRedirect. Returns a function to be called on an interval that
             * will check if that window has been closed and if so calls a callback.
             * @return {function}   The function to be called on an interval
             */
            function listenForExternalWindowClose(externalWindow) {
                return function () {
                    if(externalWindow && externalWindow.closed && _.isFunction(window.paymentProviderCompleteCallback)) {
                        window.paymentProviderCompleteCallback();
                    }
                };
            }

            window.paymentProviderCompleteCallback = paymentProviderComplete;

            // If an external window was successfully opened (not popup blocked), redirect that window and set focus listener
            if (externalWindow) {
                subfactory.showRedirectMessage();

                // set location of the external window to the payment redirect url
                externalWindow.location.href = paymentProviderUrl;

                // Listen for the external window to be manually closed so we can reopen checkout
                externalWindowClose = setInterval(listenForExternalWindowClose(externalWindow), EXTERNAL_WINDOW_CLOSE_PING_TIME);
            } else {
                // Show the redirect message modal that opens the external window
                subfactory.showRedirectMessageWithUri(type, uri);
            }

            //Clean up SPA
            if (!isOigContext) {
                subfactory.close();
            }
        }

        /**
         * In the event where lockbox has requested we open a payment provider redirect window, but can't
         * generate a redirect url for that payment provider, we need to close the external window.
         */
        function paymentProviderRedirectCancel() {
            if (externalWindow) {
                externalWindow.close();
            }
            externalWindow = undefined;
        }

        /**
         * Upon successful purchase of the offer we need to refresh entitlements and let any interested components
         * know that checkout was successfully completed (ex. bundle page needs to clear the cart).
         */
        function onPaymentSuccess() {
            checkoutPromise.resolve();
        }

        /**
         * Upon checkout error fire error event with associated error so that the checkout promise rejects.
         * @param  {string} error The error
         */
        function handleError(error) {
            checkoutPromise.reject(error);
        }

        return {
            close: subfactory.close,
            openLink: openLink,
            paymentProviderRedirectPrepare: paymentProviderRedirectPrepare,
            paymentProviderRedirectReady: paymentProviderRedirectReady,
            paymentProviderRedirectCancel: paymentProviderRedirectCancel,
            downloadOrigin: downloadOrigin,
            downloadGame: downloadGame,
            backToGame: backToGame,
            checkout: checkout,
            updateWishlist: updateWishlist,
            checkoutUsingBalance: checkoutUsingBalance,
            openCheckoutConfirmation: openCheckoutConfirmation,
            onPaymentSuccess: onPaymentSuccess,
            handleError: handleError,
            sendMarketingEvent: sendMarketingEvent,
            sendTransactionEvent: sendTransactionEvent,
            setCustomDimension: setCustomDimension
        };
    }


    /**
     * @ngdoc service
     * @name origin-components.factories.CheckoutFactory

     * @description
     *
     * Checkout Helper
     */
    angular.module('origin-components')
        .constant('OriginCheckoutTypesConstant', {
            DEFAULT: 'default',
            DEFAULT_OIG: 'default-oig',
            SUBS: 'subs',
            GIFT: 'gift',
            CHAINED: 'chained',
            TOPUP: 'topup',
            TOPUP_OIG: 'topup-oig'
        })
        .factory('CheckoutFactory', CheckoutFactory);
}());

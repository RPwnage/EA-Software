/**
 * @file oigcheckout.js
 */
(function() {
    'use strict';

    var PAGE_CONTENT_CLASSNAME = '.origin-oig-checkout',
        SUBS_SUCCESS_REDIRECT_ROUTE = 'app.oig-subscription-nux',
        PAYMENT_PROVIDER_REDIRECT_ELEMENT_NAME = 'origin-dialog-content-checkoutredirect';


    function OigCheckoutFactory($compile, $rootScope, $state) {

        /**
         * Close the modal.  In OIG, this closes the SPA window.
         */
        function closeModal() {
            if (Origin.client.isEmbeddedBrowser()) {
                Origin.client.oig.closeIGO();
            }
        }

        /**
         * Opens a modal on the successful purchase of offer(s). Directs user to download origin
         * or download the games depending.
         * @param  {Array} offerIds List of offerIds that you want to confirm the user is going to download
         */
        function openCheckoutConfirmation(offerIds) {
            $state.go('app.oig-checkout-confirmation', {offerIds: offerIds});
        }

        /**
         * Show the subscription NUX
         */
        function showSubsNux() {
            // Use $state.go to avoid routing the main SPA
            $state.go(SUBS_SUCCESS_REDIRECT_ROUTE);
        }

        /**
         * Opens the checkout iframe which start checkout process.
         * @param {string} type            Type of checkout modal to open
         * @param {string} iFrameSourceUrl iframe source URL
         * @param {string} iFrameOriginUrl iframe origin URL
         */
        function checkout(type, iFrameSourceUrl, iFrameOriginUrl) {
            var directive = '<origin-iframe type="' + type + '" source-url="'+iFrameSourceUrl+'" origin-url="'+iFrameOriginUrl+'"></origin-iframe>',
                modal = angular.element($compile(directive)($rootScope)),
                contentElement = angular.element(PAGE_CONTENT_CLASSNAME);

            contentElement.empty();
            contentElement.append(modal);
            $rootScope.$digest();
        }

        /**
         * Opens the direct checkout directive for the given offer.  Only used when purchasing an item priced in virtual currency when the user has a sufficient balance.
         * @param   {string} offerId  The offer ID to purchase
         */
        function checkoutUsingBalance(offerId) {
            var directive = '<origin-dialog-content-checkoutusingbalance offerid="' + offerId + '"></origin-dialog-content-checkoutusingbalance>',
                modal = angular.element($compile(directive)($rootScope)),
                contentElement = angular.element(PAGE_CONTENT_CLASSNAME);

            contentElement.empty();
            contentElement.append(modal);
        }

        /**
         * Shows the checkout redirect message
         */
        function showRedirectMessage() {
            var modal = angular.element($compile('<' + PAYMENT_PROVIDER_REDIRECT_ELEMENT_NAME + '></' + PAYMENT_PROVIDER_REDIRECT_ELEMENT_NAME + '>')($rootScope)),
                contentElement = angular.element(PAGE_CONTENT_CLASSNAME);

            contentElement.empty();
            contentElement.append(modal);
        }

        /**
         * Shows the redirect message with a CTA that opens the external provider URI in a new tab
         */
        function showRedirectMessageWithUri(type, uri) {
            var directive = '<' + PAYMENT_PROVIDER_REDIRECT_ELEMENT_NAME + ' continue-to-payment-provider="true" type="' + type + '" redirect-uri="' + uri + '"></' + PAYMENT_PROVIDER_REDIRECT_ELEMENT_NAME + '>',
                modal = angular.element($compile(directive)($rootScope)),
                contentElement = angular.element(PAGE_CONTENT_CLASSNAME);

            contentElement.empty();
            contentElement.append(modal);
        }

        /**
         * Closes the checkout redirect message
         */
        function closeRedirectMessage() {
            angular.element(PAGE_CONTENT_CLASSNAME).empty();
        }

        // The list of functions returned by OigCheckoutFactory and SpaCheckoutFactory should match exactly!
        return {
            close: closeModal,
            openCheckoutConfirmation: openCheckoutConfirmation,
            showSubsNux: showSubsNux,
            checkout: checkout,
            checkoutUsingBalance: checkoutUsingBalance,
            showRedirectMessage: showRedirectMessage,
            showRedirectMessageWithUri: showRedirectMessageWithUri,
            closeRedirectMessage: closeRedirectMessage
        };
    }


    /**
     * @ngdoc service
     * @name origin-components.factories.OigCheckoutFactory

     * @description
     *
     * OIG Checkout Helper
     */
    angular.module('origin-components')
        .factory('OigCheckoutFactory', OigCheckoutFactory);
}());

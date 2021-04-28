/**
 * @file spacheckout.js
 */
(function() {
    'use strict';

    var PAGE_CONTENT_CLASSNAME = '.origin-content',
        SUBS_SUCCESS_REDIRECT_ROUTE = 'app.home_loggedin',
        PAYMENT_PROVIDER_REDIRECT_ELEMENT_NAME = 'origin-dialog-content-checkoutredirect',
        PAYMENT_REDIRECT_MODAL_SIZE = 'medium',
        EXTERNAL_WINDOW_CLOSE_PING_TIME = 500,
        CHECKOUT_USING_BALANCE_DIALOG_ID = 'origin-dialog-content-checkoutusingbalance-id';


    function SpaCheckoutFactory($compile, $rootScope, DialogFactory, $timeout, AppCommFactory, storeAccessNux) {

        var shouldShowRedirectMessage = true;

        /**
         * Close the modal.
         */
        function closeModal() {
            AppCommFactory.events.fire('checkout:close');
        }

        /**
         * Opens a modal on the successful purchase of offer(s). Directs user to download origin
         * or download the games depending.
         * @param  {Array} offerIds List of offerIds that you want to confirm the user is going to download
         */
        function openCheckoutConfirmation(offerIds) {
            DialogFactory.openDirective({
                contentDirective: 'origin-dialog-content-checkoutconfirmation',
                id: 'checkout-confirmation',
                name: 'origin-dialog-content-checkoutconfirmation',
                size: 'large',
                data: {
                    id: 'checkout-confirmation',
                    'offers': offerIds
                }
            });
        }

        /**
         * Launch the new user experince
         */
        function launchNux() {
            DialogFactory.openDirective({
                id: storeAccessNux.id,
                name: 'origin-dialog-content-templateloader',
                size: 'large',
                data: {
                    template: storeAccessNux.template,
                    modal: storeAccessNux.id
                }
            });
        }

        /**
         * Show the subscription NUX
         */
        function showSubsNux() {
            $timeout(function(){
                AppCommFactory.events.once('uiRouter:stateChangeAnimationComplete', launchNux);
            }, 1000, false);
            closeModal();
            AppCommFactory.events.fire('uiRouter:go', SUBS_SUCCESS_REDIRECT_ROUTE);
        }

        /**
         * Opens the checkout iframe which start checkout process.
         * @param {string} type            Type of checkout modal to open
         * @param {string} iFrameSourceUrl iframe source URL
         * @param {string} iFrameOriginUrl iframe origin URL
         */
        function checkout(type, iFrameSourceUrl, iFrameOriginUrl) {
            var modal = angular.element($compile('<origin-iframe-modal type="' + type + '" source-url="'+iFrameSourceUrl+'" origin-url="'+iFrameOriginUrl+'"></origin-iframe-modal>')($rootScope));
            angular.element(PAGE_CONTENT_CLASSNAME).append(modal);
            $rootScope.$digest();
        }

        /**
         * Opens the direct checkout directive for the given offer.  Only used when purchasing an item priced in virtual currency when the user has a sufficient balance.
         * @param   {string} offerId  The offer ID to purchase
         */
        function checkoutUsingBalance(offerId) {
            DialogFactory.openDirective({
                id: CHECKOUT_USING_BALANCE_DIALOG_ID,
                name: 'origin-dialog-content-checkoutusingbalance',
                size: 'large',
                data: {
                    offerId: offerId,
                    dialogId: CHECKOUT_USING_BALANCE_DIALOG_ID
                }
            });
        }

        /**
         * Shows the checkout redirect message
         */
        function delayedShowRedirectMessage() {
            window.removeEventListener('focus', delayedShowRedirectMessage);

            // Using setTimeout because we need to make sure this isn't the case where the user
            // has manually closed the external tab. In that case we can't open the dialog message before we
            // try to open checkout otherwise you get this flashing open/close problem. EXTERNAL_WINDOW_CLOSE_PING_TIME + 1 because
            // of the pigeon hole principle. CPSC 221 ftw.
            setTimeout(function () {
                if(shouldShowRedirectMessage) {
                    DialogFactory.openDirective({
                        id: PAYMENT_PROVIDER_REDIRECT_ELEMENT_NAME,
                        name: PAYMENT_PROVIDER_REDIRECT_ELEMENT_NAME,
                        size: PAYMENT_REDIRECT_MODAL_SIZE,
                        data: {}
                    });
                }
            }, EXTERNAL_WINDOW_CLOSE_PING_TIME + 1);
        }

        /**
         * Sets up the redirect message focus event
         */
        function showRedirectMessage() {
            // If the window regains focus we should show the redirect message
            window.addEventListener('focus', delayedShowRedirectMessage);
        }

        /**
         * Shows the redirect message with a CTA that opens the external provider URI in a new tab
         */
        function showRedirectMessageWithUri(type, uri) {
            DialogFactory.openDirective({
                id: PAYMENT_PROVIDER_REDIRECT_ELEMENT_NAME,
                name: PAYMENT_PROVIDER_REDIRECT_ELEMENT_NAME,
                size: PAYMENT_REDIRECT_MODAL_SIZE,
                data: {
                    'continue-to-payment-provider': true,
                    'type': type,
                    'redirect-uri': uri
                }
            });
        }

        /**
         * Closes the checkout redirect message
         */
        function closeRedirectMessage() {
            window.removeEventListener('focus', delayedShowRedirectMessage);
            shouldShowRedirectMessage = false;

            DialogFactory.close(PAYMENT_PROVIDER_REDIRECT_ELEMENT_NAME);
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
     * @name origin-components.factories.SpaCheckoutFactory

     * @description
     *
     * SPA Checkout Helper
     */
    angular.module('origin-components')
        .factory('SpaCheckoutFactory', SpaCheckoutFactory);
}());

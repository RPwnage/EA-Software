/* ReloginFactory
 *
 * manages the relogin flow for the app
 * relogin needed when auto-login of the jssdk fails
 * @file factories/relogin.js
 */
(function() {
    'use strict';

    var LOST_AUTH_RETRY_MAX_ATTEMPTS = 3;
    var SEC_TO_MS = 1000;

    function ReloginFactory($timeout, AuthFactory, AppCommFactory, LogFactory) {

        var reloginFlowActive = false,
            lostAuthRetryCount = 0;


        function resetReloginFlow() {
            lostAuthRetryCount = 0;
            reloginFlowActive = false;
            hideSpinner();
        }

        function onAuthChange(loginObj) {
            if (loginObj && loginObj.isLoggedIn) {
                resetReloginFlow();
            }
        }

        function logoutAndHideSpinner() {
            AuthFactory.logout();

            // Since we're done automatically retrying authentication, hide
            // the spinner so the user can interact with the client SPA
            hideSpinner();

            // :TODO: in the SPA, we need to communicate that the user was
            // logged out.
            // Messaging to the user will be handled by:
            // https://confluence.ea.com/display/EBI/Origin+X+-+Notifications+PRD#OriginX-NotificationsPRD-NotificationsPRD-Automaticallyinitiatedflows
        }

        function hideSpinner() {
            AppCommFactory.events.fire('setBlockwaiting', false);
        }

        function showSpinner() {
            AppCommFactory.events.fire('setBlockwaiting', true);
        }

        function tryJSSDKlogin() {
            return Origin.auth.login(Origin.defines.login.APP_INITIAL_LOGIN);
        }

        function reloginFailed() {
            if (Origin.client.isEmbeddedBrowser()) {
                Origin.auth.requestSidRefresh()
                    .then(tryJSSDKlogin)    //need separate function, otherwise it will pass the response of requestSidRefresh thru to Origin.auth.login
                    .catch(logoutAndHideSpinner);
            } else {
                logoutAndHideSpinner();
            }
        }

        function reloginRetryAttempt() {
            var timeoutLength;

            function loginAttempt() {
                AuthFactory.relogin().catch(reloginRetryAttempt);
            }

            // Note that dataManager has it's own one-time retry attempt to
            // login (for failed authenticated requests). However, any auth
            // requests to dataManager will trigger this retry flow (one for
            // each request). So we only want to trigger our retry attempts
            // if and only if we haven't maxed our retry limit.
            if (lostAuthRetryCount === LOST_AUTH_RETRY_MAX_ATTEMPTS) {
                reloginFailed();
            } else if (lostAuthRetryCount < LOST_AUTH_RETRY_MAX_ATTEMPTS) {
                showSpinner();

                // Exponential back-off based on powers of two. This is
                // the same backoff methodology the client uses. See the
                // client's LoginRegistrationSession::startFallbackRenewalTimer
                // for reference.
                // :TODO: may want to control relogin flow with a boolean where
                // it gets set to true when first entered. Then add a catch()
                // to Origin.auth.login to continue with back-off logic for
                // failures. This will prevent other failed requests from
                // incrementing our counter while we're doing a back-off retry.
                // Basically any failed requests while we're performing back-off
                // will expedite the amount of time that we wait to retry the
                // auth of the user, which isn't what we want.
                timeoutLength = Math.pow(2, lostAuthRetryCount) * SEC_TO_MS;
                $timeout(loginAttempt, timeoutLength);
            }
            ++lostAuthRetryCount;
        }

        /**
         * triggered when APP relogin is needed by JSSDK
         * @method
         * @private
         */
        function onReloginNeeded() {
            LogFactory.log('relogin needed');

            if (!reloginFlowActive) {
                reloginRetryAttempt();
            }

            reloginFlowActive = true;
        }

        function onAuthReadyError(error) {
            LogFactory.error('ReloginFactory - onAuthReadyError:', error.message);
        }

        function init() {
            AuthFactory.waitForAuthReady()
                .then(onAuthChange)
                .catch(onAuthReadyError);
        }

        AuthFactory.events.on('myauth:relogin', onReloginNeeded);
        AuthFactory.events.on('myauth:change', onAuthChange);
        //fired after retry attempt succeeds
        AuthFactory.events.on('myauth:retried', onAuthChange);

        return {
            init: init
        };
    }

    /**
     * @ngdoc service
     * @name originApp.factories.ReloginFactory
     * @requires origin-components.factories.AppCommFactory
     * @description
     *
     * Auth factory
     */
    angular.module('originApp')
        .factory('ReloginFactory', ReloginFactory);
}());
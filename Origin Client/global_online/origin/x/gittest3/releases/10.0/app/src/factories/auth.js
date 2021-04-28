/* AuthFactory
 *
 * manages the authenticaton flow for the app
 * @file factories/auth.js
 */
(function() {
    'use strict';

    var APP_TEST_FOR_LOGIN_URL = 'https://accounts.int.ea.com/connect/auth?client_id=ORIGIN_PC&response_type=token&redirect_uri=nucleus:rest&prompt=none'; //&state=3D138r5719ru3e1';
    var APP_LOGIN_URL = 'https://accounts.int.ea.com/connect/auth?response_type=token&scope=basic.identity+basic.persona+signin+offline&client_id=ORIGIN_PC&locale={locale}';
    var APP_LOGOUT_URL = 'https://accounts.int.ea.com/connect/logout?client_id=ORIGIN_PC';
    var APP_LOGINOUT_HTML = '/views/login.html'; //shared by both login and logout
    var LOST_AUTH_RETRY_MAX_ATTEMPTS = 3;
    var SEC_TO_MS = 1000;

    function AuthFactory($http, $timeout, AppCommFactory, LogFactory, UserDataFactory) {

        /**
         *  events emitted by this service<br>
         *  auth:appLoginOut, true/false        emitted on APP login or out<br>
         *  auth:jssdkLogin                     emitted on JSSDK login<br>
         *  auth:jssdkLogout                    emitted on JSSDK logout<br>
         *  @namespace AuthFactory
         */
        var appLoggedInEADP = false, //for when we've successfully logged into OAuth server but may not yet logged into JSSDK
            appLoggedIn = false, //for when we've successfully logged into JSSDK and retrieved userPID, etc.
            _initialized = false,
            initPromise,
            _resolve,
            reloginFlowActive = false,
            lostAuthRetryCount = 0;

        //binds below are needed because when the event gets triggered, "this" may not be AuthFactory


        /**
         * Resolves the promise and fires the event when we
         * have finished initializing the AuthFactory
         * @memberof AuthFactory
         * @method
         * @private
         */
        function initialized(isLoggedIn) {
            var authObj = {
                'isLoggedIn': isLoggedIn
            };
            _initialized = true;
            _resolve(true);
            UserDataFactory.setAuthReady(authObj);
            AppCommFactory.events.fire('auth:ready',authObj);
        }

        function resetReloginFlow() {
            lostAuthRetryCount = 0;
            reloginFlowActive = false;
            hideSpinner();
        }

        /**
         * triggered after successful login to JSSDK after APP login
         * @memberof AuthFactory
         * @method
         * @private
         */
        function onFirstTimeJSSDKlogin() {
            LogFactory.log('AuthFactory: onFirstTimeJSSDKlogin');
            appLoggedIn = true;
            resetReloginFlow();
            initialized(true);
        }


        /**
         * called after successful login to EADP OAuth
         * @memberof AuthFactory
         * @method
         * @private
         */
        function onLoggedInEADP() {
            LogFactory.log('AuthFactory: onLoggedInEADP');

            appLoggedInEADP = true;

            //listen for the JSSDK login
            Origin.events.on(Origin.events.AUTH_SUCCESSRETRY, onLoggedInJSSDK);
            Origin.events.on(Origin.events.AUTH_LOGGEDOUT, onLoggedOutJSSDK);

            Origin.auth.login()
                .then(onFirstTimeJSSDKlogin)
                .catch(function(error) {
                    LogFactory.log(error.stack);
                    LogFactory.log('AppAuth: JSSDK Login failed');
                });

            window.loginCompleteCallback = null;
        }

        /**
         * triggered after APP logout + JSSDK logout
         * @memberof AuthFactory
         * @method
         * @private
         */
        function onLoggedOutApp() {
            LogFactory.log('AuthFactory: onLoggedOutApp');
            appLoggedIn = false;
            AppCommFactory.events.fire('auth:change', {
                'isLoggedIn': false
            });
        }

        /**
         * called after logged out from EADP (OAuth) servers
         * @memberof AuthFactory
         * @method
         * @private
         */
        function onLoggedOutEADP() {
            LogFactory.log('AuthFactory: onLoggedOutEADP');
            appLoggedInEADP = false;
            //hook up to listen to it for JSSDK to logout
            AppCommFactory.events.once('auth:jssdkLogout', onLoggedOutApp);
            Origin.auth.logout();
            window.loginCompleteCallback = null;
        }


        /**
         * listens to login event from JSSDK
         * @memberof AuthFactory
         * @method
         * @private
         */
        function onLoggedInJSSDK() {
            LogFactory.log('AuthFactory: onLoggedInJSSDK');
            resetReloginFlow();
            // this will run first time, in which case it isn't
            // technically a change, we only want to signal a change
            // after the user has finished initializing
            if (_initialized) {
                AppCommFactory.events.fire('auth:change', {
                    'isLoggedIn': true
                });
            }
            AppCommFactory.events.fire('auth:jssdkLogin');
        }

        /**
         * listens to logout event from JSSDK
         * @memberof AuthFactory
         * @method
         * @private
         */
        function onLoggedOutJSSDK() {
            LogFactory.log('AuthFactory: onLoggedOutJSSDK');
            AppCommFactory.events.fire('auth:change', {
                'isLoggedIn': false
            });
            AppCommFactory.events.fire('auth:jssdkLogout');
        }

        /**
         * initiate logout from EADP OAuth servers
         * @memberof AuthFactory
         * @method AuthFactory.logout
         */
        function logoutApp() {
            var authLogin = null;
            //if the bridge is available, then just go directly to logging out of the jssdk
            //since we didn't log in to the app
            if (Origin.bridgeAvailable()) {
                onLoggedOutEADP();
            } else {
                var oauth = APP_LOGOUT_URL;
                oauth += '&redirect_uri=';

                //IE10 doesn't support window.location.origin
                if (typeof window.location.origin === 'undefined') {
                    window.location.origin = window.location.protocol + '//' + window.location.host;
                }
                var loc = window.location.origin + window.location.pathname;
                //need to strip out any html reference in the location
                var dir = loc.substring(0, loc.lastIndexOf('/'));

                oauth += dir + APP_LOGINOUT_HTML;

                authLogin = window.open(oauth, '', 'top=100,width=' + 752 + ',height=' + 407 + ',location=no,menubar=no,resizable=no,scrollbars=yes,status=no,titlebar=no,toolbar=no', false);
                if (authLogin) {
                    authLogin.focus();
                }

                window.loginCompleteCallback = onLoggedOutEADP;
            }
        }

        function logoutAndHideSpinner() {
            logoutApp();

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

        function reloginFailed() {
            if (Origin.bridgeAvailable()) {
                Origin.auth.requestSidRefresh().then(Origin.auth.login).catch(logoutAndHideSpinner);
            } else {
                logoutAndHideSpinner();
            }
        }

        function reloginRetryAttempt() {
            var timeoutLength;

            function loginAttempt() {
                Origin.auth.login().catch(reloginRetryAttempt);
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
         * @memberof AuthFactory
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

        /**
         * passed as a param into {@link AuthFactory.testForLoggedIn}
         * @memberof AuthFactory
         * @typedef authResponseObject
         * @type {object}
         * @property {string} access_token if successful, undefined if unsuccessful
         * @property {string} error if unsuccessful, undefined if successful
         */


        /**
         * response from bouncing off of EADP server to see if we're already logged in
         * @memberof AuthFactory
         * @method
         * @private
         * @param {AuthFactory.authResponseObject} data access_token or error
         */
        function testForLoggedIn(data) {
            LogFactory.log('AuthFactory: testForLoggedIn');
            var isUserLoggedIn = false;
            if (data) {
                /* jshint camelcase: false */
                if (typeof(data.access_token) !== 'undefined') {
                    LogFactory.log('AuthFactory: User is logged in');
                    isUserLoggedIn = true;
                    onLoggedInEADP();
                } else if (typeof(data.error) !== 'undefined') {
                    LogFactory.log('AuthFactory: login error', data.error);
                } else {
                    LogFactory.log('AuthFactory: no token, or error:', data);
                }
            } else {
                // no user has logged in
                LogFactory.log('AuthFactory: app login response is empty');
            }

            // user is anonymous, either by error
            // or some other means
            if (!isUserLoggedIn) {
                LogFactory.log('AuthFactory: User is anonymous');

                //"testForLoggedIn" will get called as a result of an $http.get call which will trigger a digest cycle.
                //"initialized" is a function that fires a signal that different directives listen to.
                //Because intialized sometimes fires an event outside of angular events, the directives that listens to it
                //will need to trigger a digest to update bindings.
                //so we make sure "initialized" is called outside of the digest cycle with a timeout
                setTimeout(initialized, 0, false, false);
            }
        }

        /**
         * called when testforLogin request fails
         * @member of AuthFactory
         * @private
         * @param error error passed back from $http.get failure
         */
        function handleTestForLoggedInFailure(error) {
            LogFactory.log('AuthFactory: testforLoggedInFailure', error);
            LogFactory.log('AuthFactory: User is anonymous');
            initialized(false);
        }

        /**
         * triggered on page load to test if already logged in by bouncing off of EADP OAuthserver
         * @memberof AuthFactory
         * @method
         * @private
         */
        function testForAlreadyLoggedIn() {
            LogFactory.log('AuthFactory: testForAlreadyLoggedIn');

            var configObj = {
                withCredentials: true
            };

            $http.get(APP_TEST_FOR_LOGIN_URL, configObj)
                .success(testForLoggedIn)
                .error(handleTestForLoggedInFailure);
        }

        function loginApp() {
            var oauth = APP_LOGIN_URL,
                authLogin = null;
            oauth += '&redirect_uri=';
            oauth = oauth.replace('{locale}', Origin.locale.locale());
            //IE10 doesn't support window.location.origin
            if (typeof window.location.origin === 'undefined') {
                window.location.origin = window.location.protocol + '//' + window.location.host;
            }
            var loc = window.location.origin + window.location.pathname;
            //need to strip out any html reference in the location
            var dir = loc.substring(0, loc.lastIndexOf('/'));

            oauth += dir + APP_LOGINOUT_HTML;

            authLogin = window.open(oauth, '', 'top=100,width=' + 752 + ',height=' + 407 + ',location=no,menubar=no,resizable=no,scrollbars=yes,status=no,titlebar=no,toolbar=no', false);
            if (authLogin) {
                authLogin.focus();
            }
            window.loginCompleteCallback = onLoggedInEADP;
        }


        AppCommFactory.events.on('auth:login', loginApp);
        AppCommFactory.events.on('auth:logout', logoutApp);
        return {

            /**
             * initialization of the service - triggers JSSDK login
             * @memberof AuthFactory
             * @method AuthFactory.init
             */
            init: function() {

                if (!initPromise) {
                    initPromise = new Promise(function(resolve) {

                        // store the resolve and reject functions
                        _resolve = resolve;

                        //listen to the changes in online state
                        Origin.events.on(Origin.events.CLIENT_ONLINESTATECHANGED, function(status) {
                            AppCommFactory.events.fire('auth:clientOnlineStateChanged', status);
                        });

                        Origin.events.on(Origin.events.AUTH_FAILEDCREDENTIAL, onReloginNeeded);

                        //if this is for embedded browser, we're already logged in so trigger events that would have occurred if user clicked on "Login" on the external browser
                        if (Origin.bridgeAvailable()) {
                            onLoggedInEADP();
                        } else {
                            //test and see if we're already logged into the App (user may have just reloaded the page)
                            LogFactory.log('Testing to see if logged in already');
                            testForAlreadyLoggedIn();
                        }
                    });
                }

                return initPromise;

            },

            /**
             * initiate login to EADP OAuth servers
             * @memberof AuthFactory
             * @method AuthFactory.login
             */
            login: loginApp,

            /**
             * initiate logout from EADP OAuth servers
             * @memberof AuthFactory
             * @method AuthFactory.logout
             */
            logout: logoutApp,

            /**
             * returns whether logged into EADP OAuth server
             * @memberof AuthFactory
             * @method AuthFactory.isAppLoggedInEADP
             */
            isAppLoggedInEADP: function() {
                return appLoggedInEADP;
            },

            /**
             * returns whether fully logged into APP (EADP OAuthServer + JSSDK)
             * @memberof AuthFactory
             * @method AuthFactory.isAppLoggedIn
             */
            isAppLoggedIn: function() {
                return appLoggedIn;
            },

            /**
             * returns whether Origin client is online, valid only in embedded browser, always return true in external browser
             * @memberof AuthFactory
             * @method AuthFactory.isClientOnline
             */
            isClientOnline: function() {
                //we don't have clientActions right now, just assume  true
                if (!Origin.client) {
                    return true;
                }
                return Origin.client.isOnline();
            }
        };
    }

    /**
     * @ngdoc service
     * @name originApp.factories.AuthFactory
     * @requires $http
     * @requires origin-components.factories.AppCommFactory
     * @description
     *
     * Auth factory
     */
    angular.module('originApp')
        .factory('AuthFactory', AuthFactory);
}());
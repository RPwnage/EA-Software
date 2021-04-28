/* AuthFactory
 *
 * manages the authenticaton flow
 * @file common/auth.js
 */
(function() {
    'use strict';

    function AuthFactory($http, $timeout, AppCommFactory, ComponentsLogFactory, SubscriptionFactory) {

        /**
         *  events emitted by this service<br>
         *  auth:appLoginOut, true/false        emitted on APP login or out<br>
         *  auth:jssdkLogin                     emitted on JSSDK login<br>
         *  auth:jssdkLogout                    emitted on JSSDK logout<br>
         *  @namespace AuthFactory
         */
        var myEvents = new Origin.utils.Communicator(),
            authConfig = {},
            appLoggedInEADP = false, //for when we've successfully logged into OAuth server but may not yet logged into JSSDK
            appLoggedIn = false, //for when we've successfully logged into JSSDK and retrieved userPID, etc.
            _initialized = false,
            initPromise,
            _resolve,
            authLogin = null,
            pollInterval = null,
            loginPromise = null,
            loginSuccessCallback = null; // for when we've logged in using loginWithCallback;

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
                'isLoggedIn': isLoggedIn,
                'isOnline': isClientOnline()
            };

            _initialized = true;
            _resolve(true);
            myEvents.fire('myauth:ready', authObj);
        }

        /**
         * triggered after successful login to JSSDK after APP login, "first time" as opposed to auto-login due to expired/invalid access_token
         * @memberof AuthFactory
         * @method
         * @private
         */
        function onFirstTimeJSSDKlogin() {
            ComponentsLogFactory.log('AuthFactory: onFirstTimeJSSDKlogin');
            appLoggedIn = true;
            onLoggedInJSSDK();
            //hook up to listen to future JSSDK logins
            Origin.events.on(Origin.events.AUTH_SUCCESS_LOGIN, onLoggedInJSSDK);
            initialized(true);
        }

        /**
         * called after successful login to EADP OAuth
         * @memberof AuthFactory
         * @method
         * @private
         */
        function onLoggedInEADP() {
            ComponentsLogFactory.log('AuthFactory: onLoggedInEADP');

            appLoggedInEADP = true;

            //listen for JSSDK login post being offline
            Origin.events.on(Origin.events.AUTH_SUCCESS_POST_OFFLINE, onLoggedInJSSDKpostOffline);
            //listen for JSSDK login retry success
            Origin.events.on(Origin.events.AUTH_SUCCESS_RETRY, onLoggedInJSSDKretry);

            Origin.events.on(Origin.events.AUTH_LOGGEDOUT, onLoggedOutJSSDK);

            Origin.telemetry.sendLoginEvent('login', 'SPA', 'success', 'normal');

            Origin.auth.login() //Origin.defines.login.APP_INITIAL_LOGIN)
                .then(SubscriptionFactory.retrieveSubscriptionInfo) //grab subs info before emitting that we're "logged in"
                .then(onFirstTimeJSSDKlogin)
                .catch(function(error) {
                    ComponentsLogFactory.log(error.stack);
                    ComponentsLogFactory.log('AppAuth: JSSDK Login failed');
                });

            window.loginCompleteCallback = null;

            runLoginSuccessCallback();
        }

        /**
         * triggered after APP logout + JSSDK logout
         * @memberof AuthFactory
         * @method
         * @private
         */
        function onLoggedOutApp() {
            ComponentsLogFactory.log('AuthFactory: onLoggedOutApp');
            Origin.telemetry.sendLoginEvent('logout', 'SPA', 'success', 'normal');

            SubscriptionFactory.clearSubscriptionInfo();
            appLoggedIn = false;

            Origin.events.off(Origin.events.AUTH_SUCCESS_LOGIN, onLoggedInJSSDK);
            Origin.events.off(Origin.events.AUTH_SUCCESS_POST_OFFLINE, onLoggedInJSSDKpostOffline);
            Origin.events.off(Origin.events.AUTH_SUCCESS_RETRY, onLoggedInJSSDKretry);
            Origin.events.off(Origin.events.AUTH_LOGGEDOUT, onLoggedOutJSSDK);

            myEvents.fire('myauth:change', {
                'isLoggedIn': false,
                'isOnline': isClientOnline()
            });
        }

        function clearLoginCompleteCallback() {
            window.loginCompleteCallback = null;
        }

        function handleLogoutError(error) {
            ComponentsLogFactory.error('[AuthFactory:Logout]', error.message);
        }

        /**
         * called after logged out from EADP (OAuth) servers
         * @memberof AuthFactory
         * @method
         * @private
         */
        function onLoggedOutEADP() {
            ComponentsLogFactory.log('AuthFactory: onLoggedOutEADP');
            appLoggedInEADP = false;
            Origin.auth.logout()
                .then(clearLoginCompleteCallback)
                .catch(handleLogoutError);

        }

        /**
         * listens to login event from JSSDK
         * @memberof AuthFactory
         * @method
         * @private
         */
        function onLoggedInJSSDK() {
            var onlineObj = {
                'isLoggedIn': true,
                'isOnline': isClientOnline()
            };

            ComponentsLogFactory.log('AuthFactory: onLoggedInJSSDK');
            //for initial login, this function is triggered from JSSDK login event, and is also called from onFirstTimeJSSDKLogin
            //so we only want to send telemetry once
            Origin.telemetry.sendLoginEvent('login', 'nucleus','success', 'normal');

            // this will run first time, in which case it isn't
            // technically a change, we only want to signal a change
            // after the user has finished initializing, and app is considered logged in
            if (_initialized && appLoggedIn) {
                //we we come back online after being offline, we don't want to consider that an auth change, just want to
                //keep that transparent outside this factory
                myEvents.fire('myauth:change', onlineObj);
            }
        }

        function onLoggedInJSSDKpostOffline() {
            var onlineObj = {
                'isLoggedIn': true,
                'isOnline': true
            };

            ComponentsLogFactory.log('AuthFactory: onLoggedInJSSDKpostOffline');

            if (_initialized && appLoggedIn) {
                myEvents.fire('myauth:clientonlinestatechanged', onlineObj);
            }
        }

        function onLoggedInJSSDKretry() {
            var onlineObj = {
                'isLoggedIn': true,
                'isOnline': isClientOnline()
            };

            ComponentsLogFactory.log('AuthFactory: onLoggedInJSSDKretry');

            if (_initialized && appLoggedIn) {
                myEvents.fire('myauth:retried', onlineObj);
            }
        }

        /**
         * listens to logout event from JSSDK
         * @memberof AuthFactory
         * @method
         * @private */
        function onLoggedOutJSSDK() {
            ComponentsLogFactory.log('AuthFactory: onLoggedOutJSSDK');
            Origin.telemetry.sendLoginEvent('logout', 'nucleus', 'success', 'normal');

            //eventually we'll want to determine why we logged out of jssdk, but for now, just logout to the App
            onLoggedOutApp();
        }

        /*
         * listens for client online/offline state change
         * @memeber of AuthFactory
         * @method
         * @private
         */
        function onClientOnlineStateChangedJSSDK(online) {
            //people really only need to know if gone offline, coming back online, they should rely on waitForAuthReady
            var onlineObj = {
                'isLoggedIn': true,
                'isOnline': online
            };

            //for going offline, just pass thru the event
            //for coming back online, onBackOnlineJSSDK will get called once JSSDK login finishes AFTER coming back online
            if (!online) {
                //for going offline case, there's no "wait"
                myEvents.fire('myauth:clientonlinestatechangeinitiated', onlineObj);
                myEvents.fire('myauth:clientonlinestatechanged', onlineObj);
            } else {
                //let people know that the onlinestate is changing, clientonelinestatechanged will get fired AFTER jssdk login finishes
                myEvents.fire('myauth:clientonlinestatechangeinitiated', onlineObj);
            }
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
            if (Origin.client.isEmbeddedBrowser()) {
                onLoggedOutEADP();
            } else {
                var oauth = authConfig.logoutUrl;
                oauth += '&redirect_uri=';

                //IE10 doesn't support window.location.origin
                if (typeof window.location.origin === 'undefined') {
                    window.location.origin = window.location.protocol + '//' + window.location.host;
                }
                var loc = window.location.origin + window.location.pathname;
                //need to strip out any html reference in the location
                var dir = loc.substring(0, loc.lastIndexOf('/'));

                oauth += dir + authConfig.logInOutHtml;

                authLogin = window.open(oauth, '', 'top=100,width=' + 752 + ',height=' + 407 + ',location=no,menubar=no,resizable=no,scrollbars=yes,status=no,titlebar=no,toolbar=no', false);
                if (authLogin) {
                    authLogin.focus();
                }

                window.loginCompleteCallback = onLoggedOutEADP;
            }
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
            var isUserLoggedIn = false;
            if (data) {
                /* jshint camelcase: false */
                if (typeof(data.access_token) !== 'undefined') {
                    ComponentsLogFactory.log('AuthFactory: User is logged in');
                    isUserLoggedIn = true;
                    onLoggedInEADP();
                } else if (typeof(data.error) !== 'undefined') {
                    ComponentsLogFactory.log('AuthFactory: login error', data.error);
                } else {
                    ComponentsLogFactory.log('AuthFactory: no token, or error:', data);
                }
            } else {
                // no user has logged in
                ComponentsLogFactory.log('AuthFactory: app login response is empty');
            }

            // user is anonymous, either by error
            // or some other means
            if (!isUserLoggedIn) {
                ComponentsLogFactory.log('AuthFactory: User is anonymous');

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
            ComponentsLogFactory.log('AuthFactory: testforLoggedInFailure', error);
            ComponentsLogFactory.log('AuthFactory: User is anonymous');
            initialized(false);
        }

        /**
         * triggered on page load to test if already logged in by bouncing off of EADP OAuthserver
         * @memberof AuthFactory
         * @method
         * @private
         */
        function testForAlreadyLoggedIn() {
            ComponentsLogFactory.log('AuthFactory: testForAlreadyLoggedIn');

            var configObj = {
                withCredentials: true
            };

            $http.get(authConfig.testForLoginUrl, configObj)
                .success(testForLoggedIn)
                .error(handleTestForLoggedInFailure);
        }

        /**
         * clear the login window global vars
         * @memberof AuthFactory
         * @method
         * @private
         */
        function resetLoginWindowVars() {
            pollInterval = null;
            authLogin = null;
        }

        /**
         * poll for login window closing, fires an event
         * @memberof AuthFactory
         * @method
         * @private
         */
        function pollWindowClose() {
            try {
                if (authLogin === null || authLogin.closed) {
                    window.clearInterval(pollInterval);
                    resetLoginWindowVars();
                    myEvents.fire('myauth:loginclosed');
                }
            } catch (e) {
                //some error occurred, assume window was closed
                console.log('pollError:', e);
                myEvents.fire('myauth:loginclosed');
            }
        }

        /**
         * manually close the login window -- used when login is clicked multiple times to close previous login window
         * @memberof AuthFactory
         * @method
         * @private
         */
        function closeLoginWindow() {
            window.clearInterval(pollInterval);
            authLogin.close();
            resetLoginWindowVars();
        }

        function loginWithCallbackApp(callback) {
            loginSuccessCallback = callback;

            if (appLoggedIn) {
                runLoginSuccessCallback();
            }
            else {
                doLogin();
            }
        }

        function runLoginSuccessCallback() {
            if (typeof loginSuccessCallback === 'function') {
                loginSuccessCallback();
            }
            clearLoginSuccessCallback();
        }

        function clearLoginSuccessCallback() {
            loginSuccessCallback = null;
        }

        function loginApp() {
            clearLoginSuccessCallback();
            doLogin();
        }

        function doLogin() {
            var oauth = authConfig.loginUrl,
                local = Origin.locale.locale();

            oauth = oauth.replace('{loginlocale}', local);
            oauth += '&redirect_uri=';

            //IE10 doesn't support window.location.origin
            if (typeof window.location.origin === 'undefined') {
                window.location.origin = window.location.protocol + '//' + window.location.host;
            }
            var loc = window.location.origin + window.location.pathname;
            //need to strip out any html reference in the location
            var dir = loc.substring(0, loc.lastIndexOf('/'));

            oauth += dir + authConfig.logInOutHtml;

            //there was a previous login window opened and it wasn't closed
            //close the previous one so we only have one login window open
            if (authLogin && pollInterval) {
                //closing previous window
                closeLoginWindow();
            }

            authLogin = window.open(oauth, '', 'top=100,width=' + 752 + ',height=' + 407 + ',location=no,menubar=no,resizable=no,scrollbars=yes,status=no,titlebar=no,toolbar=no', false);
            if (authLogin) {
                authLogin.focus();

                //set up to poll for window being closed
                pollInterval = window.setInterval(pollWindowClose, 1000);
            }
            window.loginCompleteCallback = onLoggedInEADP;
        }

        function relogin() {
            return Origin.auth.login(Origin.defines.login.APP_RETRY_LOGIN);
        }

        //notify the app that relogin is needed
        function onReloginNeeded() {
            myEvents.fire('myauth:relogin');
        }

        function init(config) {
            if (!initPromise) {
                initPromise = new Promise(function(resolve) {

                    authConfig = config;

                    // store the resolve and reject functions
                    _resolve = resolve;

                    //listen to the changes in online state
                    Origin.events.on(Origin.events.CLIENT_ONLINESTATECHANGED, onClientOnlineStateChangedJSSDK);

                    //listen for failed attempt to relogin
                    Origin.events.on(Origin.events.AUTH_FAILED_CREDENTIAL, onReloginNeeded);

                    //if this is for embedded browser, we're already logged in so trigger events that would have occurred if user clicked on "Login" on the external browser
                    if (Origin.client.isEmbeddedBrowser()) {
                        onLoggedInEADP();
                    } else {
                        //test and see if we're already logged into the App (user may have just reloaded the page)
                        testForAlreadyLoggedIn();
                    }
                });
            }

            return initPromise;
        }

        /**
         * have the request to login return a promise
         */
        function promiseLogin() {
            //already logged in?
            if (appLoggedIn) {
                console.log('+++++++++++++++++++++++already loggedin');
                return Promise.resolve({
                        'isLoggedIn': appLoggedIn,
                        'isOnline': isClientOnline()
                    });
            } else {
                //check and see if login is already in progress, if so, just return the same promise
                if (!loginPromise) {
                    loginPromise = new Promise(function(resolve, reject) {

                        function respondLoggedIn() {
                            myEvents.off('myauth:ready', respondLoggedIn);

                            resolve({
                                'isLoggedIn': appLoggedIn,
                                'isOnline': isClientOnline()
                            });
                            loginPromise = null;
                        }

                        function checkAborted() {
                            //window may have been closed when login to EADP succeeded, so can't automatically reject
                            //if actually logged in, then myauth:ready will fire and resole the promise, so don't
                            if (!appLoggedInEADP) {
                                reject('login aborted');
                                loginPromise = null;
                            }
                        }

                        myEvents.on('myauth:ready', respondLoggedIn);   //listen for login success
                        myEvents.once('myauth:loginclosed', checkAborted); //listen for login window closed

                        //start the login
                        loginApp();
                    });
                }
                return loginPromise;
            }
        }

        function waitForAuthReady() {
            return new Promise(function(resolve) {

                function respondAuthReady() {
                    resolve({
                        'isLoggedIn': appLoggedIn,
                        'isOnline': isClientOnline()
                    });
                }

                if (_initialized) {
                    respondAuthReady();
                } else {
                    myEvents.on('myauth:ready', respondAuthReady);
                }
            });
        }

        function isClientOnline() {
            //we don't have clientActions right now, just assume  true
            if (!Origin.client) {
                return true;
            }
            return Origin.client.onlineStatus.isOnline();
        }

        myEvents.on('auth:loginWithCallback', loginWithCallbackApp);
        myEvents.on('auth:login', loginApp);
        myEvents.on('auth:logout', logoutApp);

        return {
            events: myEvents,

            /**
             * initialization of the service - triggers JSSDK login
             * @memberof AuthFactory
             * @method AuthFactory.init
             */
            init: init,
            /**
             * initiate login to EADP OAuth servers
             * @memberof AuthFactory
             * @method AuthFactory.login
             */
            login: loginApp,


            /**
             * return a promise that resolves when login is completed; if login is aborted returns a reject
             * @memberof AuthFactory
             * @method AuthFactory.promiseLogin
             */
            promiseLogin: promiseLogin,

            /**
             * initiate logout from EADP OAuth servers
             * @memberof AuthFactory
             * @method AuthFactory.logout
             */
            logout: logoutApp,


            /**
             * initiate a relogin to the jssdk
             * @memberof AuthFactory
             * @method AuthFactory.relogin
             */
            relogin: relogin,

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
            isClientOnline: isClientOnline,

            /**
             * returns a promise which will resole when Auth initialization has been completed
             * @memberof AuthFactory
             * @method AuthFactory.waitForAuthReady
             */
            waitForAuthReady: waitForAuthReady
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function AuthFactorySingleton($http, $timeout, AppCommFactory, ComponentsLogFactory, SubscriptionFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('AuthFactory', AuthFactory, this, arguments);
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
    angular.module('origin-components')
        .factory('AuthFactory', AuthFactorySingleton);
}());
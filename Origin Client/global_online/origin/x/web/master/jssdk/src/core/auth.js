/*jshint unused: false */
/*jshint strict: false */
define([
    'core/events',
    'core/utils',
    'core/logger',
    'core/urls',
    'core/dataManager',
    'core/errorhandler',
    'core/user',
    'core/defines',
    'core/dirtybits',
    'modules/client/client'
], function(events, utils, logger, urls, dataManager, errorhandler, user, defines, dirtybits, client) {
    /**
     * Contains authentication related methods
     * @module module:auth
     * @memberof module:Origin
     */
    var VALUE_CLEARED = '',
        REST_ERROR_SUCCESS = 0;

    function triggerEventAuthSuccess(loginType) {
        logger.log('triggered AuthSuccess', loginType);

        if (!loginType || loginType === defines.login.APP_INITIAL_LOGIN) {
            events.fire(events.AUTH_SUCCESS_LOGIN);
        } else if (loginType === defines.login.AUTH_INVALID) {
            events.fire(events.priv.AUTH_SUCCESS_POST_AUTHINVALID);
        } else if (loginType === defines.login.POST_OFFLINE) {
            events.fire(events.AUTH_SUCCESS_POST_OFFLINE);
        } else if (loginType === defines.login.APP_RETRY_LOGIN) {
            events.fire(events.AUTH_SUCCESS_RETRY);
        } else {
            //fallback
            events.fire(events.AUTH_SUCCESS_LOGIN);
        }
    }

    function triggerEventAuthFailed(loginType) {
        logger.log('triggered AuthFailed', loginType);

        //notify the APP that APP relogin is needeed
        events.fire(events.AUTH_FAILED_CREDENTIAL);

        //also notify specific type of failure
        if (loginType) {
            if (loginType === defines.login.AUTH_INVALID) {
                events.fire(events.priv.AUTH_FAILED_POST_AUTHINVALID); //this is to notify dataManager that refresh failed
            } else if (loginType === defines.login.POST_OFFLINE) {
                events.fire(events.AUTH_FAILED_POST_OFFLINE);
            } else if (loginType === defines.login.APP_RETRY_LOGIN) {
                events.fire(events.AUTH_FAILED_RETRY);
            }
        }
    }

    function triggerEventAuthLoggedOut() {
        events.fire(events.AUTH_LOGGEDOUT);
    }

    function triggerEventAuthUserPidRetrieved() {
        events.fire(events.AUTH_USERPIDRETRIEVED);
    }

    function setUserInfo(response) {
        if (utils.isChainDefined(response, ['personas', 'persona', 0, 'personaId'])) {
            //this assumes that we retrieve the information from the first persona we find
            user.setPersonaId(response.personas.persona[0].personaId);
            logger.log('personaId:', user.publicObjs.personaId());

            if (utils.isChainDefined(response, ['personas', 'persona', 0, 'displayName'])) {
                user.setOriginId(response.personas.persona[0].displayName);
                logger.log('originId:', user.publicObjs.originId());
            } else {
                logger.warn('part of originId not defined');
                user.clearOriginId();
            }
        } else {
            logger.warn('part of persona not defined');
            user.clearPersonaId();
        }
    }

    function processLoginError(msg) {
        user.clearAccessToken();
        return errorhandler.promiseReject(msg);
    }

    function processLoginResponse(data) {
        /* jshint camelcase: false */
        if (data) {
            if (data.access_token) {
                logger.log('aT: ' + data.access_token);
                user.setAccessToken(data.access_token);
                return data.access_token;
            } else if (data.error) {
                var msg = '';
                if (data.error === 'login_required') {
                    //need to emit signal to integrator that login is needed
                    msg = 'login error: login_required';
                } else {
                    msg = 'login error: ' + data.error;
                }

                return processLoginError(msg);
            } else {
                return processLoginError('login error: unknown data received:' + data);


            }
        } else {
            //need to emit signal to integrator to RE-LOGIN
            // no user has logged in
            return processLoginError('login error: response is empty');
        }
        /* jshint camelcase: true */
    }

    function onClientOnlineStateChanged(online) {
        logger.log('jssdK: onClientOnlineStateChanged:', online);
        //if going online, then need to trigger logging into jssdk
        if (online && user.publicObjs.accessToken() === VALUE_CLEARED) {
            //if we aren't already logged in, then...
            login(defines.login.POST_OFFLINE);
        } else {
            user.clearAccessToken();
            dirtybits.disconnect();
        }
    }


    function retrieveUserPersonaFromServer() {
        var endPoint = urls.endPoints.userPersona;
        var auth = 'Bearer ' + user.publicObjs.accessToken();
        var config = {
            atype: 'GET',
            headers: [],
            parameters: [{
                'label': 'userId',
                'val': user.publicObjs.userPid()
            }],
            reqauth: false, //set these to false so that dataREST doesn't automatically trigger a relogin
            requser: false
        };

        //in the offline case we don't want anything extra in the header, otherwise, it will force an OPTIONS call which will fail if offline
        dataManager.addHeader(config, 'Authorization', auth);
        dataManager.addHeader(config, 'X-Expand-Results', true);

        return dataManager.dataREST(endPoint, config);
    }

    function processUserPersonaResponse(loginType) {
        return function(response) {
            setUserInfo(response);

            //should it be ok that personaId and/or originId would be empty?
            triggerEventAuthSuccess(loginType);
        };
    }

    function retrieveUserPidFromServer() {
        var endPoint = urls.endPoints.userPID;
        var auth = 'Bearer ' + user.publicObjs.accessToken();
        var config = {
            atype: 'GET',
            headers: [],
            parameters: [],
            reqauth: false, //set these to false so that dataREST doesn't automatically trigger a relogin
            requser: false
        };

        dataManager.addHeader(config, 'Authorization', auth);
        dataManager.addHeader(config, 'X-Include-UnderAge', true);

        return dataManager.dataREST(endPoint, config);
    }

    function processUserPidResponse(response) {
        if (utils.isChainDefined(response, ['pid', 'pidId'])) {

            if (JSON.parse(response.pid.underagePid) || JSON.parse(response.pid.anonymousPid)) {
                user.setUnderAge(true);
            }

            logger.log('userPid:', response.pid.pidId);
            user.setUserPid(response.pid.pidId);

            if (utils.isChainDefined(response, ['pid', 'dob'])) {
                user.setDob(response.pid.dob);
            } else {
                logger.warn('user dob not defined');
            }

            triggerEventAuthUserPidRetrieved();
        } else {
            return errorhandler.promiseReject('part of userPid not defined');
        }
    }

    function logoutEmbedded() {
        if (client.isEmbeddedBrowser()) {
            client.user.requestLogout();
        }
    }

    function authFailed(loginType) {
        return function() {
            user.clearOriginId();
            user.clearPersonaId();
            user.clearUserPid();
            triggerEventAuthFailed(loginType);
        };
    }

    function getPersonaInfo(loginType) {
        return retrieveUserPersonaFromServer()
            .then(processUserPersonaResponse(loginType));
    }

    function setupPersonaAndConnectDirtyBits(loginType) {
        return function() {
            //run these two in parallel for performance since there is no dependencies
            return Promise.all([getPersonaInfo(loginType), dirtybits.connect()]);
        };
    }

    /**
     * login the user to the Origin JSSDK
     * @method
     */
    function login(loginType) {

        //if offline, retrieve user information across the bridge
        if (client.isEmbeddedBrowser() && !client.onlineStatus.isOnline()) {
            return client.user.offlineUserInfo()
                .then(function(userInfo) {
                    console.log('Offline User Info:', userInfo);
                    user.setUserPid(userInfo.nucleusId);
                    triggerEventAuthUserPidRetrieved();

                    user.setPersonaId(userInfo.personaId);
                    user.setOriginId(userInfo.originId);
                    user.setDob(userInfo.dob);
                    triggerEventAuthSuccess(loginType);
                });
        } else {

            logger.log('LOGIN: online');
            var endPoint = urls.endPoints.connectAuth;
            var config = {
                atype: 'GET',
                headers: [],
                parameters: [],
                reqauth: false, //set these to false so that dataREST doesn't automatically trigger a relogin
                requser: false,
                withCredentials: true
            };

            return dataManager.dataREST(endPoint, config)
                .then(processLoginResponse)
                .then(retrieveUserPidFromServer)
                .then(processUserPidResponse)
                .then(setupPersonaAndConnectDirtyBits(loginType))
                .catch(errorhandler.logAndCleanup('AUTH:login FAILED', authFailed(loginType)));
        }
    }

    /**
     * Returns a promise that resolves if the client SID cookie renewal was successful.
     * Requests a session ID (SID) cookie refresh from the embedded client.
     * accounts.ea.com uses the SID for authentication for several requests. Note that
     * a successful renewal request extends the expiration of the current SID value
     * and doesn't necessarily return a new value.
     */
    function requestSidRefresh() {
        return new Promise(function(resolve, reject) {
            // Define our response handler. HTTP 200 indicates success,
            // otherwise it's a failure.
            var onSidRenewalResponse = function(restError, httpStatus) {

                var response = {
                    rest: restError,
                    http: httpStatus
                };

                // We must wrap our Qt::disconnect attempt in a try-catch. If
                // the disconnect fails, an exception is thrown!
                try {
                    events.off(events.CLIENT_SIDRENEWAL, onSidRenewalResponse);

                    if (REST_ERROR_SUCCESS === restError) {
                        resolve(response);
                    } else {
                        reject(Error(response));
                    }
                } catch (e) {
                    reject(Error('bridge disconnect failure'));
                }
            };

            // We must wrap our Qt::connect attempt in a try-catch. If
            // the connect fails, an exception is thrown!
            try {
                // Check that the embedded client is present and that we
                // can actually make the SID renewal request. Note that
                // the Origin embedded client exposes the OriginUser object.
                if (client.isEmbeddedBrowser()) {
                    // Connect to the response before we issue the request
                    events.on(events.CLIENT_SIDRENEWAL, onSidRenewalResponse);

                    // Call the embedded client to perform the sid refresh
                    client.user.requestSidRefresh();
                } else {
                    reject(Error('bridge connect failure'));
                }
            } catch (e) {
                reject(Error('bridge connect failure'));
            }
        });
    }

    //triggering  login (event is private within jssdk)
    function onTriggerLogin(loginType) {
        login(loginType);
    }

    events.on(events.CLIENT_ONLINESTATECHANGED, onClientOnlineStateChanged);
    events.on(events.priv.AUTH_TRIGGERLOGIN, onTriggerLogin);

    return /** @lends module:Origin.module:auth */ {
        /**
         * is a user logged in the Origin JSSDK
         * @method
         * @return {boolean} responsename true if the user is logged in, false if the user is not
         */
        isLoggedIn: function() {
            var loggedIn = false;
            if (client.isEmbeddedBrowser() && !client.onlineStatus.isOnline()) {
                loggedIn = user.publicObjs.userPid().length !== 0;
            } else {
                loggedIn = (user.publicObjs.accessToken().length !== 0 && user.publicObjs.userPid().length !== 0);
            }
            return loggedIn;
        },

        /**
         * is a user online or offline in the Origin JSSDK
         * @method
         * @return {boolean} responsename true if the user is online, false if the user is not
         */
        isOnline: function() {
            if (client.isEmbeddedBrowser()) {
                return client.onlineStatus.isOnline();
            }
            return true;
        },

        /**
         * login the user to the Origin JSSDK
         * @method
         */
        login: login,

        /**
         * logout the user from the Origin JSSDK
         * @method
         */
        //for now just clear out the accessToken and userPid
        logout: function() {
            //IF embedded, send logout across the bridge
            logoutEmbedded();
            user.clearUserAuthInfo();
            return dirtybits.disconnect().then(triggerEventAuthLoggedOut);
        },

        /**
         * Returns a promise that resolves if the client SID cookie renewal was successful.
         * @method
         * @return {promise} responsename The promise resolves when the SID Cookie renewal attempt is completed
         */
        requestSidRefresh: requestSidRefresh
    };
});
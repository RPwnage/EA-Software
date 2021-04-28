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
    'modules/client/client'
], function(events, utils, logger, urls, dataManager, errorhandler, user, defines, client) {

    var VALUE_CLEARED = '',
        REST_ERROR_SUCCESS = 0;

    function triggerEventAuthSuccessRetry() {
        events.fire(events.AUTH_SUCCESSRETRY);
    }

    function triggerEventAuthFailedCredential() {
        events.fire(events.AUTH_FAILEDCREDENTIAL);
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
                logger.log('response, originId:', response.personas.persona[0].displayName);
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

    function processUserPidResponse(response) {
        if (utils.isChainDefined(response, ['pid', 'pidId'])) {

            if (JSON.parse(response.pid.underagePid) || JSON.parse(response.pid.anonymousPid)) {
                user.setUnderAge(true);
            }

            logger.log('userPid:', response.pid.pidId);
            user.setUserPid(response.pid.pidId);

            if (utils.isChainDefined(response, ['pid', 'dob'])) {
                logger.log('userDob:', response.pid.dob);
                user.setDob(response.pid.dob);
            } else {
                logger.warn('user dob not defined');
            }

            triggerEventAuthUserPidRetrieved();
        } else {
            return errorhandler.promiseReject('part of userPid not defined');
        }
    }

    function processUserPersonaResponse(response) {
        setUserInfo(response);

        //should it be ok that personaId and/or originId would be empty?
        triggerEventAuthSuccessRetry();
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
            login();
        } else {
            user.clearAccessToken();
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

    function logoutEmbedded() {
        if (utils.bridgeAvailable() && (typeof OriginUser !== 'undefined')) {
            OriginUser.requestLogout();
        }
    }

    function authFailed() {
        user.clearOriginId();
        user.clearPersonaId();
        user.clearUserPid();
        triggerEventAuthFailedCredential();
    }

    /**
     * login the user to the Origin JSSDK
     * @method
     */
    function login() {

        //if offline, retrieve user information across the bridge
        if (typeof OriginOnlineStatus !== 'undefined' && !OriginOnlineStatus.onlineState) {
            return new Promise(function(resolve, reject) {
                logger.log('LOGIN: offline');
                //retrieve the userPID
                if (typeof OriginUser !== 'undefined') {
                    var userInfo = OriginUser.offlineUserInfo();
                    console.log('Offline User Info:', userInfo);
                    user.setUserPid(userInfo.nucleusId);
                    triggerEventAuthUserPidRetrieved();

                    user.setPersonaId(userInfo.personaId);
                    user.setOriginId(userInfo.originId);
                    user.setDob(userInfo.dob);
                    triggerEventAuthSuccessRetry();
                    resolve();
                } else {
                    triggerEventAuthFailedCredential();
                    reject(new Error('OriginUser undefined'));
                }
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
                .then(retrieveUserPersonaFromServer)
                .then(processUserPersonaResponse, errorhandler.logAndCleanup('AUTH:login FAILED', authFailed));
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
                    OriginUser.sidRenewalResponseProxy.disconnect(onSidRenewalResponse);

                    if (REST_ERROR_SUCCESS === restError) {
                        resolve(response);
                    } else {
                        reject(Error(response));
                    }
                } catch(e) {
                    reject(Error('bridge disconnect failure'));
                }
            };

            // We must wrap our Qt::connect attempt in a try-catch. If
            // the connect fails, an exception is thrown!
            try {
                // Check that the embedded client is present and that we
                // can actually make the SID renewal request. Note that 
                // the Origin embedded client exposes the OriginUser object.
                if (utils.bridgeAvailable() && typeof OriginUser !== 'undefined') {
                    // Connect to the response before we issue the request
                    OriginUser.sidRenewalResponseProxy.connect(onSidRenewalResponse);

                    // Call the embedded client to perform the sid refresh
                    OriginUser.requestSidRefresh();
                }
            } catch(e) {
                reject(Error('bridge connect failure'));
            }
        });
    }

    events.on(events.CLIENT_ONLINESTATECHANGED, onClientOnlineStateChanged);
    events.on(events.priv.AUTH_TRIGGERLOGIN, login);

    //public functions
    /** @namespace
     * @memberof Origin
     * @alias auth
     */
    return {
        /**
         * is a user logged in the Origin JSSDK
         * @method
         * @return {boolean} true if the user is logged in, false if the user is not
         */
        isLoggedIn: function() {
            var loggedIn = false;
            if (typeof OriginOnlineStatus !== 'undefined' && !OriginOnlineStatus.onlineState) {
                loggedIn = user.publicObjs.userPid().length !== 0;
            } else {
                loggedIn = (user.publicObjs.accessToken().length !== 0 && user.publicObjs.userPid().length !== 0);
            }
            return loggedIn;
        },

        /**
         * is a user online or offline in the Origin JSSDK
         * @method
         * @return {boolean} true if the user is online, false if the user is not
         */
        isOnline: function() {
            if (typeof OriginOnlineStatus !== 'undefined') {
                return OriginOnlineStatus.onlineState;
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
            triggerEventAuthLoggedOut();
        },

        /**
         * Returns a promise that resolves if the client SID cookie renewal was successful.
         */
        requestSidRefresh: requestSidRefresh
    };
});

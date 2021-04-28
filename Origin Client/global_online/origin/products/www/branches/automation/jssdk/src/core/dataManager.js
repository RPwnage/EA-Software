/*jshint unused: false */
/*jshint strict: false */
define([
    'promise',
    'xml2json',
    'core/logger',
    'core/events',
    'core/user',
    'core/defines',
    'core/errorhandler',
    'core/utils',
    'modules/client/onlinestatus'
], function(Promise, xml2json, logger, events, user, defines, errorhandler, utils, onlinestatus) {
    /**
     * Contains authentication related methods
     * @module module:dataManager
     * @memberof module:Origin
     * @private
     */
    var queue = [];
    var HTTP_REQUEST_TIMEOUT = 30000; //milliseconds

    function djb2Code(str) {
        var chr,
            hash = 5381,
            len = str.length / 2;

        for (var i = str.length-1; i >= len; i--) {
            chr = str.charCodeAt(i);
            hash = hash + chr;
        }
        return hash;
    }

    function computeHashIndex(urlpath) {
        var hashVal = djb2Code(urlpath);

        hashVal = hashVal % 4;
        return hashVal;
    }

    function httpRequest(endPoint, config) {
        var exp;
        //swap out the parameters in the URL
        config.parameters = config.parameters || [];
        for (var i = 0, j = config.parameters.length; i < j; i++) {
            exp = new RegExp('{' + config.parameters[i].label + '}', 'g');
            endPoint = endPoint.replace(exp, config.parameters[i].val);
        }

        //append params
        if (typeof config.appendparams !== 'undefined') {
            for (i = 0, j = config.appendparams.length; i < j; i++) {
                if (i === 0) {
                    endPoint += '?';
                } else {
                    endPoint += '&';
                }

                endPoint += config.appendparams[i].label + '=' + config.appendparams[i].val;
            }
        }

        //now take the path portion of the endpoint and hash it
        if ((endPoint.indexOf('api{num}') > 0) || (endPoint.indexOf('data{num}') > 0)) {
            var origincom = endPoint.indexOf('origin.com/'),
                portion2beHashed;

            portion2beHashed = endPoint.substr(origincom+11);
            var hashIndex = computeHashIndex(portion2beHashed) + 1;

            endPoint = endPoint.replace('{num}', hashIndex);
        }

        //endPoint = encodeURI(endPoint);
        return new Promise(function(resolve, reject) {
            var req,
                data,
                responseHeaders ='';

            function requestSuccess(data, parseJSON, responseHeaders) {
                var result = '';
                if (data.length) {

                    if (parseJSON) {
                        try {
                            result = JSON.parse(data);
                        } catch (error) {
                            logger.error('ERROR:dataManager could not parse JSON', endPoint);
                            result = {};
                            // its not json
                        }
                    } else {
                        result = data;
                    }
                }

                if (config.responseHeader) {
                    resolve({headers:responseHeaders,
                             data: result});

                } else {
                    resolve(result);
                }
            }

            function requestError(status, textStatus, response) {
                var msg = 'httpRequest ERROR: ' + endPoint + ' ' + status + ' (' + textStatus + ')',
                    error = new Error(msg),
                    errResponse = {};

                error.status = status;
                error.endPoint = endPoint;

                if (response && response.length > 0) {
                    try {
                        errResponse = JSON.parse(response);
                    } catch (error) {
                        logger.error('ERROR:dataManager requestError could not parse response JSON', response);
                        errResponse = {};
                        // its not json
                    }
                }
                error.response = errResponse;
                reject(error);
            }

            function parseXml(xml) {
                var dom = null;
                if (window.DOMParser) {
                    try {
                        dom = (new DOMParser()).parseFromString(xml, 'text/xml');
                    } catch (e) {
                        dom = null;
                    }
                }
                //TODO: Need to handle IE
                /*else if (window.ActiveXObject) {
                    try {
                        dom = new ActiveXObject('Microsoft.XMLDOM');
                        dom.async = false;
                        if (!dom.loadXML(xml)) // parse error ..

                            window.alert(dom.parseError.reason + dom.parseError.srcText);
                    } catch (e) {
                        dom = null;
                    }
                } */
                else {
                    logger.error('cannot parse xml string!', endPoint);
                }
                return dom;
            }

            function isJSON(str) {
                try {
                    JSON.parse(str);
                } catch (e) {
                    return false;
                }
                return true;
            }

            if (endPoint === '') {
                requestError(-1, 'empty endPoint', '');
            } else {
                req = new XMLHttpRequest();
                data = config.body || '';

                req.open(config.atype, endPoint, true /*async*/ );
                req.timeout = HTTP_REQUEST_TIMEOUT;
                //add request headers
                for (var i = 0, j = config.headers.length; i < j; i++) {
                    req.setRequestHeader(config.headers[i].label, config.headers[i].val);
                }
                if (config.withCredentials) {
                    req.withCredentials = true;
                }

                req.onload = function() {
                    var contentType,
                        parsedXml,
                        jsonresp;

                    // initialize to raw response (assumed to be JSON)
                    jsonresp = req.response;

                    // do convert
                    if (!config.noconvert) {
                        contentType = req.getResponseHeader('content-type');

                        // no 'content-type' specified
                        if (contentType === null) {
                            // try to convert if it is not JSON
                            if( !isJSON(req.response) ) {
                                // see if it is XML
                                parsedXml = parseXml(req.response);
                                if (parsedXml !== null) {
                                    jsonresp = xml2json(parsedXml, '');
                                }
                            }
                        }
                        //if the response is in xml, we need to convert it to JSON
                        else
                        if (contentType.indexOf('xml') > -1) {
                            parsedXml = parseXml(req.response);
                            jsonresp = xml2json(parsedXml, '');
                        }
                    }
                    //202 is success, but pending (response we get from client local host)
                    if (req.status === 200 || req.status === 202) {
                        if (config.responseHeader) {
                            responseHeaders = req.getAllResponseHeaders();
                        }

                        requestSuccess(jsonresp, !config.noconvert, responseHeaders);
                    } else {
                        requestError(req.status, req.statusText, jsonresp);
                    }
                };

                // Handle network errors
                req.onerror = function() {
                    requestError('-1', 'XHR_Network_Error', '');
                };

                req.ontimeout = function() {
                    requestError('-2', 'XHR_Timed_Out', '');
                };

                req.send(data);
            }
        });
    }

    function objKeyMatches(obj1, obj2, key, blockStr) {
        if (obj1[key] === obj2[key]) {
            return true;
        }
        return false;
    }

    /**
     * Compare two configs for xhr requests to see if they match
     * @method
     * @param {Object} config1
     * @param {Object} config2
     * @return {Boolean}
     * @private
     */
    function configMatches(config1, config2) {
        var matches = false,
            k;

        if (objKeyMatches(config1, config2, 'atype', 'root') &&
            objKeyMatches(config1, config2, 'reqauth', 'root') &&
            objKeyMatches(config1, config2, 'requser', 'root') &&
            (config1.headers.length === config2.headers.length)) {
            for (k = 0; k < config1.headers.length; k++) {
                if (!objKeyMatches(config1.headers[k], config2.headers[k], 'label', 'headers') ||
                    !objKeyMatches(config1.headers[k], config2.headers[k], 'val', 'headers')) {
                    break;
                }
                matches = true;
            }
        }

        //still matches
        if (matches) {
            if (config1.parameters.length !== config2.parameters.length) {
                //logger.log ('config params lengths differ');
                matches = false;
            } else {
                for (k = 0; k < config1.parameters.length; k++) {
                    if (!objKeyMatches(config1.parameters[k], config2.parameters[k], 'label', 'params') ||
                        !objKeyMatches(config1.parameters[k], config2.parameters[k], 'val', 'params')) {
                        matches = false;
                        break;
                    }
                }
            }
        }

        if (matches) {
            //if the body is different consider different requests
            if(config1.body !== config2.body) {
                matches = false;
            } else if (typeof config1.appendparams !== typeof config2.appendparams) {
                matches = false;
                //logger.log('appendparams def/undef mismatch');
            } else if (typeof config1.appendparams !== 'undefined' && typeof config2.appendparams !== 'undefined') {
                if (config1.appendparams.length !== config2.appendparams.length) {
                    //logger.log ('config append param lengths differ');
                    matches = false;
                } else {
                    for (k = 0; k < config1.appendparams.length; k++) {
                        if (!objKeyMatches(config1.appendparams[k], config2.appendparams[k], 'label', 'appendparams') ||
                            !objKeyMatches(config1.appendparams[k], config2.appendparams[k], 'val', 'appendparams')) {
                            matches = false;
                            break;
                        }
                    }
                }

            }
        }
        return matches;
    }



    function deQueue(promise) {
        var dequeued = false,
            i = 0;

        if (queue.length > 0) {
            i = 0;
            for (i = 0; i < queue.length; i++) {
                if (queue[i].promise === promise) {
                    queue.splice(i, 1);
                    dequeued = true;
                    break;
                }
            }
        }
        if (dequeued === false) {
            logger.error('dequeue failed', promise);
        }
    }
    /**
     * dequeues the promise and passes on object
     * @param  {object} promise the promise to dequeue
     * @return {function} a function that returns the response
     */
    function deQueueAndPassResponse(promise) {
        return function(response) {
            deQueue(promise);
            return response;
        };
    }

    /**
     * dequeues the promise and passes on the error message
     * @param  {object} promise the promise to dequeue
     * @return {function} a function that returns the error
     */
    function deQueueAndPassFailure(promise) {
        return function(error) {
            deQueue(promise);
            return Promise.reject(error);
        };
    }

    function enQueue(baseUrl, config, outstanding) {
        var promise,
            endpoint = baseUrl,
            autoDequeue = true,
            configmatch = false,
            q = {};

        if (typeof config.autoDequeue !== 'undefined') {
            autoDequeue = config.autoDequeue;
        }

        config.parameters = config.parameters || [];
        for (var i = 0, j = config.parameters.length; i < j; i++) {
            endpoint = endpoint.replace(
                '{' + config.parameters[i].label + '}',
                encodeURIComponent(config.parameters[i].val)
            );
        }

        //look for it in the queue
        if (queue.length > 0) {
            for (i = 0; i < queue.length; i++) {
                if (queue[i].baseUrl === endpoint) { //check endpoint first
                    configmatch = configMatches(queue[i].config, config);
                    if (configmatch &&
                        queue[i].outstanding === outstanding) {
                        promise = queue[i].promise;
                        break;
                    }
                }
            }
        }

        if (typeof promise === 'undefined') {
            if (config.reqauth === true || config.requser === true) {
                promise = dataRESTauth(baseUrl, config);
            } else {
                promise = dataREST(baseUrl, config);
            }

            q = {};
            q.baseUrl = endpoint;
            q.config = config;
            q.outstanding = outstanding;
            q.promise = promise;
            queue.push(q);
            //we automatically dequeue the promise here unless we explicitly turn it off
            if (autoDequeue) {
                promise = promise.then(deQueueAndPassResponse(promise), deQueueAndPassFailure(promise));
            }

        }

        return promise;
    }

    function handleDataRestAuthError(endPoint, config) {
        return function(error) {
            //if this was an auth/user request then send back response to retry later
            //and then initiate a relogin
            var triggerRelogin = false,
                errorObj,
                cause,
                field;

            // if we are not currently logged in, then don't trigger a relogin
            var loggedIn = (user.publicObjs.accessToken().length !== 0 && user.publicObjs.userPid().length !== 0);
            if (!loggedIn) {
                return Promise.reject(error);
            }

            /*
             * Helpers
             */
            function getCauseAndField(response) {
                var cause, field;

                // unowned DLC
                cause = utils.getProperty(response, ['failure', 'cause']);
                field = utils.getProperty(response, ['failure', 'field']);



                /*
                 * Origin services should return the following error structure, e.g.,
                 *
                 * XML:
                 * <error code="10062" seq="13467605732696">
                 *     <failure value="" field="authToken" cause="MISSING_VALUE"/>
                 * </error>
                 *
                 * JSON:
                 * {error: {"code":"10062","seq":"13436458303827","failure":{"value":"","field":"authToken","cause":"MISSING_VALUE"}}}
                 *
                 *
                 * IMPORTANT: If you find an Origin service that returns an error structure other than the above, please log a bug to
                 *            the Origin Server Team.
                 *
                 *
                 *  The following case should handle error responses for all Origin services:
                 *  atom
                 *  avatars
                 *  ec2
                 *  ec2 proxy (supercarp)
                 *  xsearch
                 *  gifting
                 */
                if (cause === null && field === null) {
                    cause = utils.getProperty(response, ['error', 'failure', 'cause']);
                    field = utils.getProperty(response, ['error', 'failure', 'field']);
                }

                /*
                 * Non-Origin services
                 */

                // achievements, chat, friends, groups
                if (cause === null && field === null) {
                    cause = utils.getProperty(response, ['error', 'name']);
                }

                // gateway
                if (cause === null && field === null) {
                    cause = utils.getProperty(response, ['error']);
                }

                //EADP social (friend recommendation)
                if (cause === null && field === null) {
                  cause = utils.getProperty(response, ['message']);
                  field = utils.getProperty(response, ['code']);
                }

                return {
                    cause: cause,
                    field: field
                };
            }

            function shouldTriggerReloginAchievements(cause) {
                // modified, missing and expired access_token (401)
                if (cause && cause === 'AUTHORIZATION_REQUIRED') {
                    return true;
                }

                return false;
            }

            function shouldTriggerReloginFriendRecommendation(cause, field) {
                return field && field === 10000  && cause && cause === 'Authentication failed. Check your auth token.';
            }

            function shouldTriggerReloginAtom(cause, field) {
                if (cause && field) {
                    // missing access_token (403)
                    if (cause === 'TOKEN_USERID_INCONSISTENT' && field === 'authToken') {
                        return true;
                    }
                    else if (cause === 'MISSING_AUTHTOKEN' && field === 'authToken') { // atomUsers
                        return true;
                    }
                    // expired access_token (403)
                    else if (cause === 'invalid_token' && field === 'authToken') {
                        return true;
                    }
                    // modified access_token (403)
                    else if (cause === '500 Internal Server Error' && field === 'authToken') {
                        return true;
                    }
                }

                return false;
            }

            function shouldTriggerReloginAvatars(cause, field) {
                if (cause && field) {
                    // modified or expired access_token (401)
                    if (cause === 'INVALID_VALUE' && field === 'AuthToken') {
                        return true;
                    }
                    // missing access_token (401)
                    else if (cause === 'MISSING_VALUE' && field === 'token') {
                        return true;
                    }
                }

                return false;
            }

            function shouldTriggerReloginChat(cause) {
                if (cause) {
                    // modified, missing and expired access_token (401)
                    if (cause === 'AUTHTOKEN_INVALID') {
                        return true;
                    }
                }

                return false;
            }

            function shouldTriggerReloginEC2(cause, field) {
                if (cause && field) {
                    // missing access_token (401)
                    if (cause === 'MISSING_VALUE' && field === 'authToken') {
                        return true;
                    }
                    // modified or expired access_token (403)
                    else if (cause === 'INVALID_AUTHTOKEN' && field === 'authToken') {
                        return true;
                    }
                }

                return false;
            }

            function shouldTriggerReloginEC2Proxy(cause, field) {
                if (cause && field) {
                    // missing access_token (401)
                    if (cause === 'MISS_VALUE' && field === 'AuthToken') {
                        return true;
                    }
                    // modified or expired access_token (401)
                    else if (cause === 'AUTHTOKEN_USERID_UN_CONSISTENT' && field === 'AuthToken') {
                        return true;
                    }
                }

                return false;
            }

            function shouldTriggerReloginEC2ConsolidatedEntitlements(cause, field) {
                if (cause && field) {
                    // missing access_token (401)
                    if (cause === 'MISSING_VALUE' && field === 'authToken') {
                        return true;
                    }
                    // modified or expired access_token (403)
                    else if (cause === 'AUTHTOKEN_USERID_INCONSISTENT' && field === 'authToken') {
                        return true;
                    }
                }

                return false;
            }

            function shouldTriggerReloginFriends(cause) {
                // modified, missing and expired access_token (400)
                if (cause && cause === 'AUTHTOKEN_INVALID') {
                    return true;
                }

                return false;
            }

            function shouldTriggerReloginGateway(cause) {
                if (cause) {
                    // missing access_token (400)
                    if (cause === 'invalid_oauth_info') {
                        return true;
                    }
                    // modified and expired access_token (403)
                    else if (cause === 'invalid_access_token') {
                        return true;
                    }
                }

                return false;
            }

            function shouldTriggerReloginGifting(cause, field) {
                if (cause && field) {
                    // missing access_token (403)
                    if (cause === 'TOKEN_USERID_INCONSISTENT' && field === 'authToken') {
                        return true;
                    }
                    // expired and modified access_token (403)
                    else if (cause === 'invalid_token' && field === 'authToken') {
                        return true;
                    }
                }

                return false;
            }

            function shouldTriggerReloginGroups(cause) {
                if (cause) {
                    // modified, missing and expired access_token (401)
                    if (cause === 'AUTHTOKEN_INVALID') {
                        return true;
                    }
                }

                return false;
            }

            function shouldTriggerReloginXSearch(cause, field) {
                if (cause && field) {
                    // missing access_token (401)
                    if (cause === 'MISSING_AUTHTOKEN' && field === 'authToken') {
                        return true;
                    }
                    // modified access_token (403)
                    else if (cause === '500 Internal Server Error' && field === 'authToken') {
                        return true;
                    }
                    // expired access_token (403)
                    else if (cause === 'invalid_token' && field === 'authToken') {
                        return true;
                    }
                }

                return false;
            }

            /*
             * Determine whether to relogin
             */
            errorObj = getCauseAndField(error.response);
            cause = errorObj.cause;
            field = errorObj.field;

            if (error.status === defines.http.ERROR_400_BAD_REQUEST) {

                triggerRelogin = shouldTriggerReloginGateway(cause, field) ||
                                 shouldTriggerReloginFriends(cause, field);
            }
            else if (error.status === defines.http.ERROR_401_UNAUTHORIZED) {
                //a temporary hack to get around the issue that EC2 isn't allowing /public endpoints
                //we'll get back a 401 if we don't own the offer but we don't want to re-initiate
                //a login in the case.  so just allow it to fail
                if (!config.dontRelogin) {
                    //for unowned DLC, we can get back a 401 from a private offer request and the parent offerId isn't 1102/1103
                    //in that case, we don't want to trigger a relogin
                    //when access_token is invalid, we get back in the response:
                    //{"code":10053,"seq":"10074974195346","failure":{"cause":"INVALID_VALUE","field":"AuthToken","value":"QVQwOjEuMDozLjA6NjA6UEtPSnV1bW9CVzQ3UE41eWxUVFl1NzFjRzdnQ204ODczUHo6OTAwMDQ6bXE3bDZ"}}"
                    if (cause && cause === 'INVALID_VALUE' && field && field === 'AuthToken') {
                        triggerRelogin = true;
                    }

                    triggerRelogin = triggerRelogin ||
                                     shouldTriggerReloginEC2(cause, field) ||
                                     shouldTriggerReloginEC2Proxy(cause, field) ||
                                     shouldTriggerReloginEC2ConsolidatedEntitlements(cause, field) ||
                                     shouldTriggerReloginXSearch(cause, field) ||
                                     shouldTriggerReloginAvatars(cause, field) ||
                                     shouldTriggerReloginAchievements(cause, field) ||
                                     shouldTriggerReloginGroups(cause, field) ||
                                     shouldTriggerReloginChat(cause, field) ||
                                     shouldTriggerReloginFriendRecommendation(cause, field);
                }
            } else if (error.status === defines.http.ERROR_403_FORBIDDEN && !config.dontRelogin) {

                // check for empty response
                if (error.response.length === 0) {
                    triggerRelogin = true;
                }
                else {
                    triggerRelogin = shouldTriggerReloginAtom(cause, field) ||
                                     shouldTriggerReloginEC2(cause, field) ||
                                     shouldTriggerReloginEC2ConsolidatedEntitlements(cause, field) ||
                                     shouldTriggerReloginGateway(cause, field) ||
                                     shouldTriggerReloginXSearch(cause, field) ||
                                     shouldTriggerReloginGifting(cause, field) ||
                                     shouldTriggerReloginFriendRecommendation(cause, field);
                }
            }
            // The following will not execute due to the early exit above when not logged in
            /* else if (error.status === defines.http.ERROR_404_NOTFOUND) {
                if (config.reqauth && user.publicObjs.accessToken().length === 0) {
                    triggerRelogin = true;
                } else if (config.requser && user.publicObjs.userPid().length === 0) {
                    triggerRelogin = true;
                }
            }*/

            if (triggerRelogin) {
                return new Promise(function(resolve, reject) {
                    var loginSucceeded, loginFailed;
                    loginSucceeded = function() {
                        events.off(events.priv.AUTH_FAILED_POST_AUTHINVALID, loginFailed);
                        //AUTH_SUCCESS_POST_AUTHINVALID.off already because of events.once
                        //Login was successful, replace access token in headers
                        for (var i = 0; i < config.headers.length; i++) {
                            // if authHint is available, use that thing
                            if (config.hasOwnProperty('authHint')) {
                                if (config.headers[i].label === config.authHint.property) {
                                    config.headers[i].val = config.authHint.format.replace('{token}', user.publicObjs.accessToken());
                                    break;
                                }
                            } else if (config.headers[i].label === 'Authorization') {
                                config.headers[i].val = 'Bearer ' + user.publicObjs.accessToken();
                                break;
                            } else if (config.headers[i].label === 'AuthToken') {
                                config.headers[i].val = user.publicObjs.accessToken();
                                break;
                            }
                        }
                        //Retry the original request with the new auth token
                        httpRequest(endPoint, config).then(function(response) {
                            logger.log('dataRESTauth: reauth and sucess');
                            resolve(response);
                        }, function(error) {
                            logger.log('dataRESTauth: reauth and failure');
                            reject(error);
                        });
                    };
                    loginFailed = function() {
                        events.off(events.priv.AUTH_SUCCESS_POST_AUTHINVALID, loginSucceeded);
                        //AUTH_FAILED_POST_AUTHINVALID.off already because of events.once
                        error.message = 'OJSSDK_ERR_AUTH_RETRY_WHEN_READY';
                        reject(error);
                    };
                    //Trigger a login. As a result one of the above two functions will be called,
                    //closing out this promise
                    events.once(events.priv.AUTH_SUCCESS_POST_AUTHINVALID, loginSucceeded);
                    events.once(events.priv.AUTH_FAILED_POST_AUTHINVALID, loginFailed);
                    events.fire(events.priv.AUTH_TRIGGERLOGIN, defines.login.AUTH_INVALID);
                });
            } else {
                return Promise.reject(error);
            }
        };
    }

    /* declared in constants.js */
    function dataRESTauth(endPoint, config) {
        return httpRequest(endPoint, config).catch(handleDataRestAuthError(endPoint, config));
    }

    /**
     * @method
     */
    function dataREST(endPoint, config) {
        return httpRequest(endPoint, config);
    }

    /**
     * @method
     */
    function validateDataObject(objectContract, object) {
        var required = objectContract.required || [],
            optional = objectContract.optional || [],
            validDataSet = required.concat(optional),
            objectKeys = Object.keys(object),
            validatedObject = {},
            prop = '';

        // remove properties not defined in object contract
        for (prop in object) {
            if (validDataSet.indexOf(prop) !== -1) {
                validatedObject[prop] = object[prop];
            }
        }

        // ensure required properties are present in data object
        for (prop in required) {
            if (!validatedObject.hasOwnProperty(required[prop])) // and is not null
            {
                return false;
            }
        }

        return validatedObject;
    }


    return /** @lends module:Origin.module:dataManager */ {

        /**
         * @method
         */
        dataRESTauth: dataRESTauth,

        /**
         * @method
         */
        dataREST: dataREST,

        /**
         * Check and see if request already exists, if so return promise, otherwise, generate request, add to the queue, and return the associated promise
         * @method
         * @param {String} baseUrl
         * @param {Object} config
         * @param {String} outstanding outstanding request identifier (e.g. Last-Modified-Date)
         * @return {promise} promise to an xhr response
         * @private
         */
        enQueue: enQueue,

        /**
         * Remove the request(promise) from the queue
         * @method
         * @param {Object} promise
         * @private
         */
        deQueue: deQueue,

        /**
         * for online requests, add the additional header
         * @method
         * @param {string} label label to use
         * @param {string} val value of the label
         */
        addHeader: function(config, label, val) {
            config = config || {};
            config.headers = config.headers || [];
            //use onlinestatus instead of bringing in all of client to avoid circular dependency
            if (onlinestatus.isOnline()) {
                config.headers.push({
                    'label': label,
                    'val': val
                });
            }
        },

        /**
         * Put a body on a request
         * @param {Object} config     the config object
         * @param {Object} bodyObject the data that you want in the body. This will replace what is currently in the body.
         */
        addBody: function(config, bodyObject) {
            config = config || {};
            config.body = bodyObject;
        },
        /**
         * Replacements for parameters within endpoint
         * @method
         * @param {Object} config config object
         * @param {String} label label to use
         * @param {String} val value of the label
         */
        addParameter: function(config, label, val) {
            config = config || {};
            config.parameters = config.parameters || [];
            config.parameters.push({
                'label': label,
                'val': val
            });
        },

        /**
         * Query parameters appended to endpoint
         * @method
         * @param {Object} config config object
         * @param {String} label label to use
         * @param {String} val value of the label
         */
        appendParameter: function(config, label, val) {
            config = config || {};
            config.appendparams = config.appendparams || [];
            config.appendparams.push({
                'label': label,
                'val': val
            });
        },

        /**
         * Adds hinting for auth token header, to allow retry replacement
         * @param {Object} config config object
         * @param {String} property authentication token name
         * @param {String} format authentication token string (use {token} as placeholder)
         */
        addAuthHint: function(config, property, format) {
            config.authHint = {property: property, format: format};
        },

        validateDataObject: validateDataObject

    };

});

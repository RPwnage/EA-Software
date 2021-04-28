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
    'modules/client/client'
], function(Promise, xml2json, logger, events, user, defines, errorhandler, utils, client) {
    /**
     * Contains authentication related methods
     * @module module:dataManager
     * @memberof module:Origin
     * @private
     */
    var queue = [];
    var HTTP_REQUEST_TIMEOUT = 30000; //milliseconds

    function httpRequest(endPoint, config) {
        //swap out the parameters in the URL
        for (var i = 0, j = config.parameters.length; i < j; i++) {
            endPoint = endPoint.replace(
                '{' + config.parameters[i].label + '}',
                config.parameters[i].val
            );
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

                    if (req.status === 200) {
                        if (config.responseHeader) {
                            responseHeaders = req.getAllResponseHeaders();
                        }

                        //if the response is in xml, we need to convert it to JSON
                        contentType = req.getResponseHeader('content-type');

                        //if 'config.nocovert' is set to true, then we just process the raw response as is
                        if (config.noconvert) {
                            requestSuccess(req.response, false, responseHeaders);
                        } else
                        if (!config.noconvert && contentType !== null && contentType.indexOf('xml') > -1) {
                            parsedXml = parseXml(req.response);
                            jsonresp = xml2json(parsedXml, '');
                            requestSuccess(jsonresp, true, responseHeaders);
                        } else {
                            requestSuccess(req.response, true, responseHeaders);
                        }
                    } else {
                        requestError(req.status, req.statusText, req.response);
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
            if (typeof config1.appendparams !== typeof config2.appendparams) {
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
                cause,
                field;

            if (error.status === defines.http.ERROR_401_UNAUTHORIZED) {
                //a temporary hack to get around the issue that EC2 isn't allowing /public endpoints
                //we'll get back a 401 if we don't own the offer but we don't want to re-initiate
                //a login in the case.  so just allow it to fail
                if (!config.dontRelogin) {
                    //for unowned DLC, we can get back a 401 from a private offer request and the parent offerId isn't 1102/1103
                    //in that case, we don't want to trigger a relogin
                    //when access_token is invalid, we get back in the response:
                    //{"code":10053,"seq":"10074974195346","failure":{"cause":"INVALID_VALUE","field":"AuthToken","value":"QVQwOjEuMDozLjA6NjA6UEtPSnV1bW9CVzQ3UE41eWxUVFl1NzFjRzdnQ204ODczUHo6OTAwMDQ6bXE3bDZ"}}"
                    cause = utils.getProperty(error.response, ['failure', 'cause']);
                    field = utils.getProperty(error.response, ['failure', 'field']);

                    if (cause && cause === 'INVALID_VALUE' && field && field === 'AuthToken') {
                        triggerRelogin = true;
                    }
                }
            } else if (error.status === defines.http.ERROR_403_FORBIDDEN && !config.dontRelogin) {
                triggerRelogin = true;
            } else if (error.status === defines.http.ERROR_404_NOTFOUND) {
                if (config.reqauth && user.publicObjs.accessToken().length === 0) {
                    triggerRelogin = true;
                } else if (config.requser && user.publicObjs.userPid().length === 0) {
                    triggerRelogin = true;
                }
            }

            if (triggerRelogin) {
                return new Promise(function(resolve, reject) {
                    var loginSucceeded, loginFailed;
                    loginSucceeded = function() {
                        events.off(events.priv.AUTH_FAILED_POST_AUTHINVALID, loginFailed);
                        //AUTH_SUCCESS_POST_AUTHINVALID.off already because of events.once
                        //Login was successful, replace access token in headers
                        for (var i = 0; i < config.headers.length; i++) {
                            if (config.headers[i].label === 'AuthToken') {
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
            if (client.onlineStatus.isOnline()) {
                config.headers.push({
                    'label': label,
                    'val': val
                });
            }
        }
    };

});
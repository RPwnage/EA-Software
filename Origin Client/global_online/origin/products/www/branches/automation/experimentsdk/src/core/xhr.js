/*jshint unused: false */
/*jshint strict: false */
define([
], function() {
    /**
     * Contains xhr methods
     * @module module:xhr
     * @memberof module:Experiment
     * @private
     */
    var queue = [];
    var HTTP_REQUEST_TIMEOUT = 30000; //milliseconds

    function httpRequest(endPoint, config) {
        var HTTP_REQUEST_TIMEOUT = 30000;
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
                            console.error('ERROR:dataManager could not parse JSON', endPoint);
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
                        console.error('ERROR:dataManager requestError could not parse response JSON', response);
                        errResponse = {};
                        // its not json
                    }
                }
                error.response = errResponse;
                reject(error);
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
                        jsonresp;

                    // initialize to raw response (assumed to be JSON)
                    jsonresp = req.response;

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


    return /** @lends module:Experiment.module:xhr */ {

        /**
         * @method
         * @param {String} endPoint
         * @param {Object} config
         */
        httpRequest: httpRequest
    };

});
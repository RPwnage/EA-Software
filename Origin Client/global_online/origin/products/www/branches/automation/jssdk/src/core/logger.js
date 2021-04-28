/*jshint unused: false */
/*jshint strict: false */
/*jshint undef: false */
define(['core/telemetry'], function(telemetry) {
    /**
     * A wrapper for console log
     * @module module:log
     * @memberof module:Origin
     * @private
     */
    var jssdkPrefix = '[JSSDK]',
        jssdkColor = 'background: #000077; color: #cccccc',
        //normally we would pass this down via overrides, but the logging bindings happen on jssdk parse
        //so we don't have chance to do so.
        showlogging = window.location.href.indexOf('showlogging=true') > -1;


    function handleLoggingError(err) {
        console.error(err.message);
    }


    //this should eventually be separated out into its own module and sendError should be pulled out of logging too.
    function buildAndSendErrorData(errorArgs) {
        var date = new Date(),
            data = {
                errorMessage: errorArgs[0],
                url: window.location.href
            },
            errDescription = '',
            errorObject;

        // if an error object was passed, extract important info from it
        if (errorArgs.length > 1) {
            errorObject = errorArgs[1];

            if (typeof errorObject === 'object') {
                data.errorDescription = errorObject.message;
                data.errorStack = errorObject.stack;
                data.errorStatus = errorObject.status;
                if (errorObject.response && errorObject.response.error && errorObject.response.error.failure && errorObject.response.error.failure.cause) {
                    data.errorCause = errorObject.response.error.failure.cause;
                }
            }
        }

        // include the timestamp only for GA
        data.time = date.toUTCString();
        errDescription = JSON.stringify(data);
        delete data.time;

        telemetry.sendErrorEvent(data.errorMessage, errDescription, data);
    }

    function justSendError() {
        var args = [];
        //convert arguments to a real array
        args = Array.prototype.slice.call(arguments);

        buildAndSendErrorData(args);
    }

    function logAndSendError(bindFn) {
        return function() {
            var args = Array.prototype.slice.call(arguments);

            bindFn.apply(console, arguments);

            buildAndSendErrorData(args);
        };
    }

    function getBindFunction(type, prefix, color, isChrome) {
        if (showlogging) {
            if (typeof OriginGamesManager !== 'undefined') {
                return window.console[type].bind(console, '%c' + prefix, color);
            } else if (isChrome) {
                return window.console[type].bind(console, '%c' + prefix, color);
            } else {
                return window.console[type].bind(console);
            }
        } else {
            return function() {};
        }
    }

    /**
     * @param {string} msg The string you want to log
     * @return {promise}
     */
    function logMessage(type, prefix, color) {

        var isChrome = navigator && navigator.userAgent && (navigator.userAgent.indexOf('Chrome') !== -1),
            bindFn;

        //special case handling of just sending error to telemetry, and not logging
        //need to do this for now since buildAndSendErrorData is in this file
        //should eventually move it out
        if (type === 'senderror') {
            return justSendError;
        } else {
            //if a browser doesn't support a particular type of console messaging just default to log
            if (typeof console[type] === 'undefined') {
                type = 'log';
            }

            bindFn = getBindFunction(type, prefix, color, isChrome);

            if (type === 'error') {
                return logAndSendError(bindFn);
            } else if (type === 'senderror') {
                return justSendError;
            } else {
                return bindFn;
            }
        }
    }



    function logMessageJSSDK(type) {
        return logMessage(type, jssdkPrefix, jssdkColor);
    }


    /**
     * @namespace
     * @memberof privObjs
     * @private
     * @alias log
     */
    return /** @lends module:Origin.module:log */ {
        log: logMessageJSSDK('log'),
        info: logMessageJSSDK('info'),
        warn: logMessageJSSDK('warn'),
        error: logMessageJSSDK('error'),
        debug: logMessageJSSDK('debug'),
        publicObjs: {
            /**
             * @method
             * @param {string} msg The string you want to log
             * @return {promise}
             */
            message: logMessage
        }
    };
});
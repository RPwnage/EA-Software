/*jshint unused: false */
/*jshint strict: false */
/*jshint undef: false */
define(['core/telemetry', 'stacktrace'], function(telemetry, printStackTrace) {
    /**
     * A wrapper for console log
     * @module module:log
     * @memberof module:Origin
     * @private
     */
    var jssdkPrefix = '[JSSDK]',
        jssdkColor = 'background: #000077; color: #cccccc';


    function buildAndSendErrorData(errorMessage, stacktrace) {
        var date = new Date(),
            data = {
                url: window.location.href,
                errorMessage: errorMessage[0],
                stackTrace: stacktrace,
                time: date.toUTCString()
            },
            errDescription = '',
            errMessageConcat = '';


        if (errorMessage.length > 1) {
            errorMessage.shift();
            errMessageConcat = errorMessage.toLocaleString();
            data.stackTrace = errMessageConcat + data.stackTrace;
        }

        errDescription = JSON.stringify(data);
        telemetry.sendErrorEvent(data.errorMessage, errDescription, data);
    }

    function logAndSendError(bindFn) {
        return function() {
            var args = [];

            //do this first because we muck with arguments in buildAndSendErrorData
            bindFn.apply(console, arguments);
            console.log.bind(console, 't\t' + printStackTrace()[4] + '\n');

            //convert arguments to a real array
            args = Array.prototype.slice.call(arguments);
            buildAndSendErrorData(args, printStackTrace()[4]);
        };
    }

    function getBindFunction(type, prefix, color, isChrome) {
        if (typeof OriginGamesManager !== 'undefined') {
            return console[type].bind(console, '%c' + prefix, color);
        } else if (isChrome) {
            return console[type].bind(console, '%c' + prefix, color);
        } else {
            return console[type].bind(console);
        }
    }

    /**
     * @param {string} msg The string you want to log
     * @return {promise}
     */
    function logMessage(type, prefix, color) {

        if (typeof STRIPCONSOLE === 'undefined') {
            STRIPCONSOLE = false;
        }

         if(!STRIPCONSOLE){
            var isChrome = navigator && navigator.userAgent && (navigator.userAgent.indexOf('Chrome') !== -1),
                bindFn;

            //if a browser doesn't support a particular type of console messaging just default to log
            if(typeof console[type] === 'undefined') {
                type = 'log';
            }

            bindFn = getBindFunction (type, prefix, color, isChrome);

            if (type === 'error') {
                return logAndSendError(bindFn);
            } else

            //if we are embedded we need to manually get the line numbers and paste that
            if (typeof OriginGamesManager !== 'undefined') {
                return function() {
                    bindFn.apply(console, arguments);
                    console.log.bind(console, '\t\t' + printStackTrace()[4] + '\n');
                };
            } else {
                return bindFn;
            }

        } else {
            //return empty function if we stripconsole
            return function() {};
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
    return /** @lends module:Origin.module:log */{
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
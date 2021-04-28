/*jshint unused: false */
/*jshint strict: false */
/*jshint undef: false */
define(['stacktrace'], function(printStackTrace) {

    var jssdkPrefix = '[JSSDK]',
        jssdkColor = 'background: #000077; color: #cccccc';


    function buildSendErrorData(error, exception, cause) {
        var errorMessage, stackTrace;

        if (typeof exception !== 'undefined') {
            errorMessage = exception.toString();
            stackTrace = printStackTrace({
                e: exception
            });
        } else {
            errorMessage = error;
            stackTrace = printStackTrace();
        }

        var date = new Date();
        var data = {
            fqdn: window.location.href,
            errorMessage: errorMessage,
            stackTrace: stackTrace,
            cause: (cause || ''),
            time: date.toUTCString()
        };

        logMessageJSSDK('error')('ERROR: ' + errorMessage);

        var exDescription = JSON.stringify(data);

        ga('send', 'event', 'error', data.errorMessage, exDescription);
        ga('send', 'exception', {
            'exDescription': exDescription,
            'exFatal': false
        });
    }

    /**
     * @param {Exception} exception The exception object
     * @param {string} description of the cause of the exception
     * @return {void}
     */
    function logException(exception, cause) {
        //TODO: need to revisit this: Sometimes, this function is called but the object is not a standard 'Error object'. So we check for a stacktrace
        // and process the error differently if theres no stacktrace
        if (exception.stack) {
            logMessageJSSDK('error')(exception.stack);
            // buildSendErrorData('', exception, cause);
        } else {
            buildSendErrorData(cause);
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
            var isChrome = navigator && navigator.userAgent && (navigator.userAgent.indexOf('Chrome') !== -1);

            //if a browser doesn't support a particular type of console messaging just default to log
            if(typeof console[type] === 'undefined') {
                type = 'log';
            }

            //if we are embedded we need to manually get the line numbers and paste that
            if (typeof OriginGamesManager !== 'undefined') {
                return function() {
                    console[type].bind(console, '%c' + prefix, color).apply(console, arguments);
                    console.log.bind(console)('\t\t' + printStackTrace()[4] + '\n');
                };
            } else if (isChrome) {
                return console[type].bind(console, '%c' + prefix, color);
            } else {
                return console[type].bind(console);
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
    return {
        log: logMessageJSSDK('log'),
        info: logMessageJSSDK('info'), 
        warn: logMessageJSSDK('warn'),
        error: logMessageJSSDK('error'),
        debug: logMessageJSSDK('debug'),
        publicObjs: {
            /**
             * @method
             * @param {Exception} exception The exception object
             * @param {string} description of the cause of the exception
             * @return {void}
             */
            exception: logException,
            /**
             * @method
             * @param {string} msg The string you want to log
             * @return {promise}
             */
            message: logMessage
        }
    };
});
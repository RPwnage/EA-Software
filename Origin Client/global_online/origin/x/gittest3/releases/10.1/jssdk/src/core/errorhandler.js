/*jshint unused: false */
/*jshint strict: false */

define([
    'core/utils',
    'core/logger'
], function(utils, logger) {

    return {
        promiseReject: function(msg, extraProperties) {
            var error = new Error(msg);
            if (extraProperties) {
                utils.mix(error, extraProperties);
            }
            return Promise.reject(error);
        },
        logAndCleanup: function(msg, customAction) {
            return function(error) {
                var output = error.message;
                
                //if there is a stack lets use that as output instead
                if(error.stack) {
                    output = error.stack;
                }

                if (customAction) {
                    customAction(error);
                }
                logger.error('[' + msg + ']', output);
                return Promise.reject(error);
            };
        }
    };
});
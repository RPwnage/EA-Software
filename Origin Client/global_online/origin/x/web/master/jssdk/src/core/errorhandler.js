/*jshint unused: false */
/*jshint strict: false */

define([
    'core/utils',
    'core/logger'
], function(utils, logger) {
    /**
     * Some private errorhandler utility functions for jssdk
     * @module module:errorhandler
     * @memberof module:Origin
     * @private
     */
    return /** @lends module:Origin.module:errorhandler */ {
        /**
         * sets up an error object with some custom properties and returns the object from Promise.reject();
         * @param  {object} msg             The message used for the error
         * @param  {object} extraProperties Any extra properties we want to add to the error object
         * @return {promise}                responsename The promise returned from Promise.reject()
         */
        promiseReject: function(msg, extraProperties) {
            var error = new Error(msg);
            if (extraProperties) {
                utils.mix(error, extraProperties);
            }
            return Promise.reject(error);
        },
        /**
         * Logs an error before passing on the reject in the Promise chain
         * @param  {object} msg          A message that is prepended to the actual error message
         * @param  {function} customAction A function that is run when we hit this error
         * @return {promise}              responsename The promise returned from Promise.reject()
         */
        logAndCleanup: function(msg, customAction) {
            return function(error) {
                var output = error.message;

                //if there is a stack lets use that as output instead
                if (error.stack) {
                    output = error.stack;
                }

                if (customAction) {
                    customAction(error);
                }
                logger.error('[' + msg + ']', output);
                return Promise.reject(error);
            };
        },
        /**
         * Logs the message from the error object to console in standard style
         * @param  {Error} error Error object
         */
        logErrorMessage: function(error) {
            logger.error(error.message);
        }
    };
});
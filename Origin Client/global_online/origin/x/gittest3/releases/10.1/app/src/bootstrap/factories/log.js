/**
 * factory for an error logging service
 * @file factories/log.js
 */
(function() {
    'use strict';

    var bootstrapPrefix = '[BOOTSTRAP]',
        bootstrapColor = 'background: #003300; color: #cccccc';

    function LogFactory() {
        function logMessage(type) {
            return Origin.log.message(type, bootstrapPrefix, bootstrapColor);
        }

        return {
            /**
             * @function
             * @description
             *
             * uses console.log with the prefix [BOOTSTRAP]
             */
            log: logMessage('log'),
            /**
             * @function
             * @description
             *
             * uses console.warn with the prefix [BOOTSTRAP]
             */
            warn: logMessage('warn'),
            /**
             * @function
             * @description
             *
             * uses console.error with the prefix [BOOTSTRAP]
             */
            error: logMessage('error'),
            /**
             * @function
             * @description
             *
             * uses console.debug with the prefix [BOOTSTRAP]
             */
            debug: logMessage('debug'),
            /**
             * @function
             * @description
             *
             * uses console.info with the prefix [BOOTSTRAP]
             */
            info: logMessage('info')
        };
    }
    /**
     * @ngdoc service
     * @name bootstrap.factories.LogFactory
     * @description
     *
     * LogFactory
     */
    angular.module('bootstrap')
        .factory('LogFactory', LogFactory);

}());
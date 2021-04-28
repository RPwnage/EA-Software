/**
 * factory for an error logging service
 * @file factories/log.js
 */
(function() {
    'use strict';

    var appPrefix = '[APP]',
        appColor = 'background: #007700; color: #cccccc';

    function LogFactory() {
        function logMessage(type) {
            return Origin.log.message(type, appPrefix, appColor);
        }

        return {
            /**
             * @function
             * @description
             *
             * uses console.log with the prefix [APP]
             */
            log: logMessage('log'),
            /**
             * @function
             * @description
             *
             * uses console.warn with the prefix [APP]
             */
            warn: logMessage('warn'),
            /**
             * @function
             * @description
             *
             * uses console.error with the prefix [APP]
             */
            error: logMessage('error'),
                        /**
             * @function
             * @description
             *
             * uses console.debug with the prefix [APP]
             */
            debug: logMessage('debug'),
                        /**
             * @function
             * @description
             *
             * uses console.info with the prefix [APP]
             */
            info: logMessage('info')
        };
    }
    /**
     * @ngdoc service
     * @name originApp.factories.LogFactory
     * @requires $log
     * @description
     *
     * LogFactory
     */
    angular.module('originApp')
        .factory('LogFactory', LogFactory);

}());
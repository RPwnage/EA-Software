/**
 * factory for an error logging service
 * @file factories/log.js
 */
(function() {
    'use strict';

    var componentsPrefix = '[COMPONENTS]',
        componentsColor = 'background: #006666; color: #cccccc';


    function ComponentsLogFactory() {
        function logMessage(type) {
            return Origin.log.message(type, componentsPrefix, componentsColor);
        }

        return {
            /**
             * @function
             * @description
             *
             * uses console.log with the prefix [COMPONENTS]
             */
            log: logMessage('log'),
            /**
             * @function
             * @description
             *
             * uses console.warn with the prefix [COMPONENTS]
             */
            warn: logMessage('warn'),
            /**
             * @function
             * @description
             *
             * uses console.error with the prefix [COMPONENTS]
             */
            error: logMessage('error'),
            /**
             * @function
             * @description
             *
             * uses console.debug with the prefix [COMPONENTS]
             */
            debug: logMessage('debug'),
            /**
             * @function
             * @description
             *
             * uses console.info with the prefix [COMPONENTS]
             */
            info: logMessage('info')
        };
    }
    /**
     * @ngdoc service
     * @name origin-components.factories.ComponentsLogFactory
     * @description
     *
     * ComponentsLogFactory
     */
    angular.module('origin-components')
        .factory('ComponentsLogFactory', ComponentsLogFactory);

}());

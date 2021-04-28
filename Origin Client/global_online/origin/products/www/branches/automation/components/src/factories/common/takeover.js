/**
 * @file common/takeover.js
 */
(function () {
    'use strict';
    var LOCAL_STORAGE_NAMESPACE = 'OriginTakeoverHelper';
    function OriginTakeoverHelper(LocalStorageFactory) {

        /**
         * Get namespace with takeover id
         * @param {string} id takeover id
         * @returns {string} namespace
         */
        function getTakeoverNamespace(id){
            return [LOCAL_STORAGE_NAMESPACE, _.kebabCase(id)].join('_');
        }

        /**
         * Dismiss a takeover by ID
         * @param {string} id takeover id
         */
        function dismissTakeover(id){
            var namespace = getTakeoverNamespace(id),
                dismissed = true;
            LocalStorageFactory.set(namespace, dismissed);
        }

        /**
         * Check if a takeover is dismissed
         * @param id
         * @returns {Boolean} isDismissed true if takeover is dismissed
         */
        function isTakeoverDismissed(id){
            var namespace = getTakeoverNamespace(id),
                isDismissed = LocalStorageFactory.get(namespace, false);
            return isDismissed;
        }

        return {
            dismissTakeover: dismissTakeover,
            isTakeoverDismissed: isTakeoverDismissed
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function OriginTakeoverHelperSingleton(LocalStorageFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('OriginTakeoverHelper', OriginTakeoverHelper, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OriginTakeoverHelper
     * @description
     *
     * Takeover helper factories
     *
     */
    angular.module('origin-components')
        .factory('OriginTakeoverHelper', OriginTakeoverHelperSingleton);

})();

/**
 * @file common/localstorage.js
 */

(function() {
    'use strict';

    var SEPARATOR = '_';

    function LocalStorageFactory(ComponentsLogFactory) {
        /**
         * Generates local storage key for the current user.
         * @return {string}
         */
        function prefixKey(key) {
            return key + SEPARATOR + Origin.user.userPid();
        }

        /**
         * Saves given object in the current user's storage as JSON string.
         * @param {string} key - storage key to write to
         * @param {mixed} data - data to save
         * @return {void}
         */
        function set(key, data) {
            var name = prefixKey(key),
                value = JSON.stringify(data);

            localStorage[name] = value;
        }

        /**
         * Reads data from local storage
         * @param {string} key - storage key to read from
         * @param {mixed} defaultValue - *optional* value to return if nothing is found in the storage
         * @return {mixed}
         */
        function get(key, defaultValue) {
            var name = prefixKey(key), value = defaultValue;

            if (typeof localStorage[name] !== "undefined") {
                try {
                    value = JSON.parse(localStorage[name]);
                } catch (error) {
                    // We don't really care about read errors: just use default value
                    ComponentsLogFactory.error('[local-storage-factory get(' + name + ')] error', error.message);
                }
            }

            return value;
        }

        function deleteKey(key) {
            var name = prefixKey(key);
            try {
                localStorage.removeItem(name);
            } catch (error) {
                ComponentsLogFactory.error('[local-storage-factory delete(' + name + ')] error', error.message);
            }
        }

        return {
            set: set,
            get: get,
            delete: deleteKey
        };
    }

    angular.module('origin-components')
        .factory('LocalStorageFactory', LocalStorageFactory);
}());

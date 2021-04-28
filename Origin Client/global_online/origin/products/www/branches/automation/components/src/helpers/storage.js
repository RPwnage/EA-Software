/**
 * @file common/storage.js
 */

(function () {
    'use strict';

    var SEPARATOR = '_';

    function createStorageFactory(storageLocation) {

      return function(ComponentsLogFactory, JsonSerializerFactory) {

          /**
             * Generates storage key for the current user.
             * @return {string}
             */
            function prefixKey(key) {
                return _.compact([key, Origin.user.userPid()]).join(SEPARATOR);
            }

            /**
             * Saves given object in the current user's storage as JSON string.
             * @param {string} key - storage key to write to
             * @param {mixed} data - data to save
             * @param {boolean} useRawKey - *optional* for non user specific key
             * @return {void}
             */
            function set(key, data, useRawKey) {

                var name = useRawKey ? key : prefixKey(key),
                    value = JsonSerializerFactory.serialize(name, data);

                try {
                    storageLocation.setItem(name, value);

                } catch (error) {
                    ComponentsLogFactory.error('['+storageLocation +'-factory set(' + name + ')] error', error);
                }
            }

            /**
             * Reads data from storage
             * @param {string} key - storage key to read from
             * @param {mixed} defaultValue - *optional* value to return if nothing is found in the storage
             * @param {boolean} useRawKey - *optional* for non user specific key
             * @return {mixed}
             */
            function get(key, defaultValue, useRawKey) {
                var name = useRawKey ? key : prefixKey(key),
                    value = defaultValue,
                    response = null;

                try {
                    response = storageLocation.getItem(name);
                } catch (error) {
                    ComponentsLogFactory.error('['+storageLocation +'-factory get(' + name + ')] error', error);
                    value = response;
                }

                if (!_.isEmpty(response)) {
                    value = JsonSerializerFactory.deserialize(name, response);
                }

                return value;
            }

            /**
             * Deletes data reference by a key in storage
             * @param {string} key - storage key
             * @param {boolean} useRawKey - *optional* for non user specific key
             */
            function deleteKey(key, useRawKey) {
                var name = useRawKey ? key : prefixKey(key);

                storageLocation.removeItem(name);
            }

            return {
                set: set,
                get: get,
                delete: deleteKey
            };

        };

    }

    angular.module('origin-components')
        .factory('LocalStorageFactory', ['ComponentsLogFactory', 'JsonSerializerFactory', createStorageFactory(window.localStorage)])
        .factory('SessionStorageFactory', ['ComponentsLogFactory', 'JsonSerializerFactory',createStorageFactory(window.sessionStorage)]);
}());

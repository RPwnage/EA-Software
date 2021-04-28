/**
 * @file common/serializer.js
 */
(function() {
    'use strict';

    function ObjectSerializerFactory() {
        var PROPERTIES_SEPARATOR = ';',
            KEY_AND_VALUE_SEPARATOR = '=';

        /**
         * Deserializes string into an object
         */
        function deserialize(string) {
            var serializedProperties = [],
                object = {},
                keyValuePair = [],
                i = 0;

            if (string) {
                serializedProperties = string.split(PROPERTIES_SEPARATOR);

                for (i = 0; i < serializedProperties.length; i++) {
                    keyValuePair = serializedProperties[i].split(KEY_AND_VALUE_SEPARATOR);
                    object[keyValuePair[0]] = keyValuePair[1];
                }
            }

            return object;
        }

        /**
         * Serializes object into a string
         */
        function serialize(object) {
            var serializedProperties = [];

            for (var p in object) {
                if (object.hasOwnProperty(p)) {
                    serializedProperties.push(p + KEY_AND_VALUE_SEPARATOR + object[p]);
                }
            }

            return serializedProperties.join(PROPERTIES_SEPARATOR);
        }

        return {
            /**
             * Deserializes string into an object
             * @param {string} string serialized data
             * @return {Object}
             */
            deserialize: deserialize,

            /**
             * Serializes object into a string
             * @param {Object} object object to be serialized
             * @return {string}
             */
            serialize: serialize
        };

    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ObjectSerializerFactory

     * @description
     *
     * ObjectSerializerFactory
     */
    angular.module('origin-components')
        .factory('ObjectSerializerFactory', ObjectSerializerFactory);

}());
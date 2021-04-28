/**
 * @file common/serializer.js
 */
(function() {
    'use strict';

    function ArraySerializerFactory() {
        var PROPERTIES_SEPARATOR = ';';

        /**
         * Deserializes string into an array
         */
        function deserialize(string) {
            if (string) {
                return string.split(PROPERTIES_SEPARATOR);
            } else {
                return [];
            }
        }

        /**
         * Serializes array into a string
         */
        function serialize(array) {
            return array.join(PROPERTIES_SEPARATOR);
        }

        return {
            /**
             * Deserializes string into an array
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
     * @name origin-components.factories.ArraySerializerFactory

     * @description
     *
     * ArraySerializerFactory
     */
    angular.module('origin-components')
        .factory('ArraySerializerFactory', ArraySerializerFactory);

}());
/**
 * @file common/json-serializer.js
 */
(function() {
    'use strict';

    function JsonSerializerFactory(ComponentsLogFactory) {

        /**
         * Serialize a value for storage
         * @param  {String} name the key name
         * @param  {Mixed} data the data to serialize
         * @return {String} the serialized data
         */
        function serialize(name, data) {
            var value;

            try {
                value = JSON.stringify(data);
            } catch (error) {
                ComponentsLogFactory.log('[json-serializer serialize(' + name + ')] WARN - ', error);
            }
            return value;
        }

        /**
         * Deserialize a value from storage
         * @param  {String} name the key name
         * @param  {String} data the data to deserialize
         * @return {Mixed} deserialized data structure
         */
        function deserialize(name, data) {
            var value;

            try {
                value = JSON.parse(data);
            } catch (error) {
                ComponentsLogFactory.log('[local-storage-factory deserialize(' + name + ')] WARN - ', error);
            }

            return value;
        }

        return {
            deserialize: deserialize,
            serialize: serialize
        };

    }

    /**
     * @ngdoc service
     * @name origin-components.factories.JsonSerializerFactory

     * @description
     *
     * JsonSerializerFactory
     */
    angular.module('origin-components')
        .factory('JsonSerializerFactory', JsonSerializerFactory);

}());
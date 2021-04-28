/**
 * @file object-helper.js
 */
(function() {
    'use strict';

    function ObjectHelperFactory() {
        /**
         * Converts native object into native array by dropping property names and keeping property values.
         * Helps when we need to apply Array's map/filter functions to an object
         *
         * @param {Object} object - Javascript object to be converted
         * @return {Array}
         */
        function toArray(object) {
            return Object.keys(object).map(function (i) {
                return object[i];
            });
        }

        /**
         * Maps a function over the object's properties preserving the keys.
         *
         * @param {function} func - function to apply to each property
         * @param {Object} object - Javascript object to be converted
         * @return {Array}
         */
        function map(func, object) {
            var result = {};

            for (var i in object) {
                if (object.hasOwnProperty(i)) {
                    result[i] = func(object[i]);
                }
            }

            return result;
        }

        /**
         * Filters object's properties preserving the keys. Removes any property that doesn't pass the filtering function.
         *
         * @param {function} func - function to apply to each property
         * @param {Object} object - Javascript object to be converted
         * @return {Array}
         */
        function filter(func, object) {
            var result = {};

            for (var i in object) {
                if (object.hasOwnProperty(i) && func(object[i])) {
                    result[i] = object[i];
                }
            }

            return result;
        }

        /**
         * Calculates the length of a Javascript object
         *
         * @param {Object} object - Javascript object
         * @return {Array}
         */
        function length(object) {
            return Object.keys(object).length;
        }

        /**
         * Generates getter function that given an object yields its property value.
         * Used for creating filter/promise piplines
         *
         * @param {stromg} propertyName - property name
         * @return {function}
         */
        function getProperty(propertyName) {
            return function (object) {
                return object[propertyName];
            };
        }

        /**
         * Generates setter function that given an object sets its property to certain value.
         * Used for creating filter/promise piplines
         *
         * @param {string} propertyName - property name
         * @param {mixed} value - desired property value
         * @return {function}
         */
        function setProperty(propertyName, value) {
            return function (object) {
                object[propertyName] = value;

                return value;
            };
        }

        return {
            getProperty: getProperty,
            setProperty: setProperty,
            toArray: toArray,
            map: map,
            filter: filter,
            length: length,
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ObjectHelperFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ObjectHelperFactory', ObjectHelperFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ObjectHelperFactory
     * @description
     *
     * Helpers for manipulating objects
     */
    angular.module('origin-components')
        .factory('ObjectHelperFactory', ObjectHelperFactorySingleton);
}());
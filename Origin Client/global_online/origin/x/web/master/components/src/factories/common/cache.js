/**
 * @file cache.js
 */
(function() {
    'use strict';

    function CacheFactory() {
        /**
         * Default key generation strategy: convertion to a JSON string
         * @param {mixed} value value to generate unique cache key from
         * @return {string}
         */
        function stringify(value) {
            return JSON.stringify(value);
        }

        /**
         * Default cache expiration strategy: cache doesn't expire
         * @return {boolean}
         */
        function alwaysGood() {
            return false;
        }

        /**
         * Cache storage object
         * @class CacheStorage
         * @return {boolean}
         */
        function CacheStorage(keyConstructor, expiryChecker) {
            this.values = {};
            this.expiryChecker = (expiryChecker === undefined) ? alwaysGood : expiryChecker;
        }

        /**
         * Gets data from cache based on its unique key.
         * Returns "undefined" if the data is not found or cache is expired for the given key.
         * @param {string} key unique cache key
         * @return {mixed}
         */
        CacheStorage.prototype.get = function (key) {
            if (!this.values.hasOwnProperty(key) || this.expiryChecker(this.values[key])) {
                return undefined;
            }

            return this.values[key];
        };

        /**
         * Adds cached data
         * @param {string} key unique cache key
         * @param {mixed} value data to cache
         * @return {void}
         */
        CacheStorage.prototype.set = function (key, value) {
            this.values[key] = value;
        };

        /**
         * Wraps a data providing function into a closure providing
         * caching for the function's results
         * @param {function} dataGetter original function
         * @param {CacheStorage} cacheStorage cache storage object
         * @param {function} constructKey cache object
         * @return {function}
         */
        function withCache(dataGetter, cacheStorage, constructKey) {
            return function () {
                var args = Array.prototype.slice.call(arguments),
                    key = constructKey(args),
                    value = cacheStorage.get(key);

                if (value === undefined) {
                    value = dataGetter.apply(undefined, args);
                    cacheStorage.set(key, value);
                }

                return value;
            };
        }

        /**
         * Public API for decorating a function with cache layer
         * @param {string} key unique cache key
         * @param {mixed} value data to cache
         * @return {void}
         */
        function decorate(func, keyConstructor, expiryChecker) {
            var constructKey = (keyConstructor === undefined) ? stringify : keyConstructor,
                storage = new CacheStorage(expiryChecker);

            return withCache(func, storage, constructKey);
        }

        return {
            decorate: decorate
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function CacheFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('CacheFactory', CacheFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.CacheFactory

     * @description
     *
     * Generic cache decorator
     */
    angular.module('origin-components')
        .factory('CacheFactory', CacheFactorySingleton);
}());
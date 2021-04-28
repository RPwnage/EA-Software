/**
 * @file cache.js
 */
(function() {
    'use strict';

    function CacheFactory(DateHelperFactory) {
        /**
         * The default time to live in seconds - approximately 5 minutes
         * @type {Number}
         */
        var defaultTtl = 300,
            refreshArgumentName = 'cacheFactory:forceCacheRefresh';

        /**
         * Default key generation strategy: conversion to a JSON string
         * @param {mixed} args value to generate unique cache key from
         * @return {string}
         */
        var stringify = function(args) {
            return JSON.stringify(_.without(args, refreshArgumentName));
        };

        /**
         * Default cache expiration strategy: cache reacts to an optional ttl
         * @return {boolean}
         */
        var isExpired = function(date) {
            return DateHelperFactory.isInThePast(date);
        };

        /**
         * Cache storage object
         * @class CacheStorage
         * @param {Function} expiryChecker an optional expiry date handling strategy
         * @return {boolean}
         */
        function CacheStorage(expiryChecker) {
            /**
             * Some type hints for the getter when unserializing the data
             * @type {Object}
             */
            this.types = {
                'pojo': 'pojo'
            };

            /**
             * The storage field names
             * @type {String}
             */
            this.dataField = 'data';
            this.expiryField = 'expiry';
            this.typeField = 'type';

            /**
             * The cache bag
             * @type {Object}
             */
            this.values = {};

            this.expiryChecker = _.isFunction(expiryChecker) ? expiryChecker : isExpired;
        }

        CacheStorage.prototype.fetch = function(key) {
            var self = this;

            return _.get(self.values, [key, self.dataField]);
        };


        /**
         * Gets data from cache based on its unique key.
         * @param {string} key unique cache key
         * @return {mixed} data or undefined on a cache miss
         */
        CacheStorage.prototype.get = function(key) {
            var self = this;

            if (!_.has(self.values, [key, self.expiryField]) ||
                !_.has(self.values, [key, self.dataField]) ||
                !_.has(self.values, [key, self.typeField]) ||
                self.expiryChecker(_.get(self.values, [key, self.expiryField]))
            ) {
                return;
            }

            return self.fetch(key);
        };

        /**
         * Update or insert synchronous javascript data into the cache bag
         * @param {String} key the cache key identifier
         * @param {Mixed} data synchronous data
         * @param {Date} expiry the cache expiry time
         * @param {String} type the type name in this types
         */
        CacheStorage.prototype.upsert = function(key, data, expiry, type) {
            var self = this;

            self.values[key] = {};
            self.values[key][self.dataField] = data;
            self.values[key][self.expiryField] = expiry;
            self.values[key][self.typeField] = type;
        };

        /**
         * Adds cached data to storage
         * @param {string} key unique cache key
         * @param {mixed} value data to cache
         * @param {Number} ttl time offset in seconds
         * @return {void}
         */
        CacheStorage.prototype.set = function(key, value, ttl) {
            var generationTime = new Date(),
                timeToLive = _.isNumber(ttl) ? ttl : defaultTtl,
                self = this;

            generationTime.setSeconds(generationTime.getSeconds() + timeToLive);

            self.upsert(key, value, generationTime, self.types.pojo);

            return value;
        };

        /**
         * Remove a key from the cache
         * @param {string} key unique cache key
         */
        CacheStorage.prototype.remove = function(key) {
            var self = this;

            delete self.values[key];
        };

        /**
         * Wraps a data providing function into a closure providing
         * caching for the function's results
         * @param {Function} dataGetter the getter function if the data is not cached
         * @param {CacheStorage} cacheStorage cache storage object
         * @param {Function} constructKey the cache key algo
         * @return {function}
         */
        function withCache(dataGetter, ttl, cacheStorage, constructKey) {
            /**
             * This function awaits arguments to be passed to the data getter
             * @param {...string} var_args
             * @return {Mixed} the data
             */
            return function () {
                var args, key, value;

                args = Array.prototype.slice.call(arguments);
                key = constructKey(args);

                if(args.indexOf(refreshArgumentName) > -1) {
                    cacheStorage.remove(key);
                    _.pull(args, refreshArgumentName);
                }

                value = cacheStorage.get(key);

                if (_.isUndefined(value)) {
                    value = dataGetter.apply(undefined, args);
                    return cacheStorage.set(key, value, ttl);
                }

                return value;
            };
        }

        /**
         * Public API for decorating a function with cache layer
         * @param {Function} dataGetter the data generator callback
         * @param {Number} ttl the optional time to live value in seconds
         * @param {Function} keyConstructor an optional key construction algorithm - will be passed dataGetter's args
         * @param {Function} expiryChecker an optional expiry checker algorithm. Handler expects a boolean return (true if expired).
         * @return {Function}
         */
        function decorate(dataGetter, ttl, keyConstructor, expiryChecker) {
            var constructKey = _.isFunction(keyConstructor) ? keyConstructor : stringify,
                storage = new CacheStorage(expiryChecker);

            return withCache(dataGetter, ttl, storage, constructKey);
        }

        /**
         * Pass the refresh tokey to any request to evict the old cache record.
         * @param {Boolean} refresh set to true to refresh the content
         * @return {string} The custom
         */
        function refresh(isRefresh) {
            return (isRefresh) ? refreshArgumentName : undefined;
        }

        return {
            decorate: decorate,
            refresh: refresh
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function CacheFactorySingleton(DateHelperFactory, SingletonRegistryFactory) {
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
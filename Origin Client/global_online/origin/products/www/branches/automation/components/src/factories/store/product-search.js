/**
 * @file store/product-search.js
 */
/* global Promise */
(function() {
    'use strict';

    function ProductSearchFactory(StoreMockDataFactory, CacheFactory) {
        /**
         * Resolves filtering conditions into the list of offer IDs
         * @param {Object} conditions filtering conditions
         * @return {Promise}
         */
        function findOfferIds(conditions) {
            return Promise.resolve(conditions).then(StoreMockDataFactory.getEditionOfferIds);
        }

        return {
            findOfferIds: CacheFactory.decorate(findOfferIds)
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ProductSearchFactorySingleton(StoreMockDataFactory, CacheFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ProductSearchFactory', ProductSearchFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ProductSearchFactory

     * @description
     *
     * Store product factory
     */
    angular.module('origin-components')
        .factory('ProductSearchFactory', ProductSearchFactorySingleton);
}());
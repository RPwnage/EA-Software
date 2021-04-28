/**
 * @file store/product.js
 */
(function() {
    'use strict';

    function ProductFactory(ObjectHelperFactory, GamesDataFactory, ObservableFactory, ObserverFactory, OfferTransformer, AuthFactory, SubscriptionFactory) {
        var collection = ObservableFactory.observe({}),
            toArray = ObjectHelperFactory.toArray,
            forEach = ObjectHelperFactory.forEach,
            filter = ObjectHelperFactory.filter;

        /**
         * Checks if a collection has an object with the key specified
         * @param {string} offerId key
         * @return {boolean}
         */
        function isInCollection(offerId) {
            return collection.data.hasOwnProperty(offerId);
        }

        /**
         * Checks if the offer ID is not known to the collection
         * @param {string} offerId key
         * @return {boolean}
         */
        function isNotInCollection(offerId) {
            return !isInCollection(offerId);
        }

        function reloadCollection() {
            loadData(_.keys(collection.data), true);
        }

        /**
         * Adds empty model objects to the collection for the offers currently being loaded
         *
         * @param {Array} offerIds - offer ID's to mark
         * @return {void}
         */
        function createModels(offerIds) {
            forEach(function (offerId) {
                collection.data[offerId] = {};
            }, offerIds);
        }

        /**
         * Loads data for given offer ID's (promised or otherwise) from the main data pipeline
         * @param {Promise|Object|Array} offerIds - offer ID's to load the data for
         * @return {Promise}
         */
        function loadData(offerIds, force) {
            if(!force) {
                offerIds = toArray(filter(isNotInCollection, offerIds));
            }

            if (offerIds.length > 0) {
                createModels(offerIds);

                GamesDataFactory
                    .getCatalogInfo(offerIds)
                    .then(OfferTransformer.transformOffer)
                    .then(updateCollection);
            }
        }

        /**
         * Updates data in the repository
         * @param {Object} newData - new models
         */
        function updateCollection(newData) {
            forEach(function (item) {
                collection.data[item.offerId] = item;
            }, newData);

            collection.commit();
        }

        /**
         * Returns observer focused on a single model or a collection of models.
         * @param {string} offerIds offer ID(s)
         * @return {Observer}
         */
        function get(offerIds) {
            var observer = ObserverFactory.create(collection);

            loadData(toArray(offerIds));

            // Collection is a hash-map of model objects indexed by offer ID's.
            if (typeof offerIds === 'string') {
                return observer.getProperty(offerIds).defaultTo({});
            } else {
                return observer.pick(offerIds);
            }
        }

        //setup a listeners for any entitlement updates
        GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', reloadCollection);
        SubscriptionFactory.events.on('SubscriptionFactory:subscriptionUpdate', reloadCollection);
        AuthFactory.events.on('myauth:change', reloadCollection);

        return {
            get: get
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ProductFactory

     * @description
     *
     * Store product factory
     */
    angular.module('origin-components')
        .factory('ProductFactory', ProductFactory);
}());

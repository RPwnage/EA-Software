/**
 * @file store/product.js
 */
/* global Promise */
(function() {
    'use strict';

    var DEFAULT_PLATFORM = 'PCWIN',
        CATALOG_DATA_MAP = {
            offerId: 'offerId',
            displayName: ['i18n', 'displayName'],
            shortDescription: ['i18n', 'shortDescription'],
            longDescription: ['i18n', 'longDescription'],
            packArt: ['i18n', 'packArtLarge'],
            purchasable: ['countries', 'isPurchasable'],
            downloadable: 'downloadable',
            releaseDate: ['platforms', DEFAULT_PLATFORM, 'releaseDate'],
            downloadStartDate: ['platforms', DEFAULT_PLATFORM, 'downloadStartDate'],
            useEndDate: ['platforms', DEFAULT_PLATFORM, 'useEndDate'],
            oth: 'oth',
            trial: 'trial',
            demo: 'demo',
            eulaLink : ['i18n', 'eulaURL'],
            gameRatingDesc : 'gameRatingDesc',
            gameRatingDescriptionLong : 'gameRatingDescriptionLong',
            gameRatingPendingMature : 'gameRatingPendingMature',
            gameRatingReason : 'gameRatingReason',
            gameRatingTypeValue : 'gameRatingTypeValue',
            gameRatingUrl : 'gameRatingUrl',
            gameRatingSystemIcon: ['i18n', 'ratingSystemIcon'],
            subscriptionAvailable : 'vault',
            platforms : 'platforms'
        };

    function ProductFactory(ObjectHelperFactory, GamesDataFactory, ObservableFactory, StoreMockDataFactory, ObserverFactory, CacheFactory, SubscriptionFactory, LocaleFactory) {
        var collection = ObservableFactory.observe({}),
            transformWith = ObjectHelperFactory.transformWith,
            forEach = ObjectHelperFactory.forEach,
            merge = ObjectHelperFactory.merge,
            filter = ObjectHelperFactory.filter,
            mapWith = ObjectHelperFactory.mapWith,
            map = ObjectHelperFactory.map,
            getProperty = ObjectHelperFactory.getProperty,
            toArray = ObjectHelperFactory.toArray,
            forEachWith = ObjectHelperFactory.forEachWith;

        /**
         * @todo remove this when the rest of the services are available
         *
         * Adds information otherwise missing in other data sources
         * @param {Object} game model data to augment
         * @return {Object}
         */
        function mockMissingData(game) {
            var data = StoreMockDataFactory.getMock(game.offerId);
            return merge(game, data);
        }

        /**
         * Adds entitlement information to the product model object
         * @param {Object} game model data to augment
         * @return {Object}
         */
        function loadEntitlementsInfo(game) {
            game.isOwned = GamesDataFactory.ownsEntitlement(game.offerId);
            game.isInstalled = GamesDataFactory.isInstalled(game.offerId);
            game.isSubscription = game.subscriptionAvailable && SubscriptionFactory.userHasSubscription();
            return game;
        }

        /**
         * Adds prices and discounts to the product model objects
         * @param {Object} games collection of models to augment
         * @return {Function}
         */
        function addPriceInfo(games) {
            return function (dataItem) {
                if (!games.hasOwnProperty(dataItem.offerId)) {
                    throw new Error('[ProductFactory]: Cannot find model for the price data being loaded');
                }

                var game = games[dataItem.offerId],
                    localCurrency = LocaleFactory.getCurrency(Origin.locale.countryCode()),
                    prices = getProperty(['rating', localCurrency])(dataItem);

                if (!!prices) {
                    game.price = prices.finalTotalPrice || 0;
                    game.strikePrice = prices.originalTotalPrice || 0;
                    game.savings = prices.totalDiscountRate || 0;
                }
            };
        }

        /**
         * Wraps incoming data objects into a native JS array.
         * @param {Object|Array} data
         * @return {Function}
         */
        function wrapInArray(data) {
            return Array.isArray(data) ? data : [data];
        }

        /**
         * Loads prices and discounts information from the Ratings service
         * @param {Object|Array} games collection of models to augment
         * @return {Promise}
         */
        function loadPrices(games) {
            var offerIds = toArray(map(getProperty('offerId'), games));

            return GamesDataFactory.getPrice(offerIds)
                .then(wrapInArray)
                .then(forEachWith(addPriceInfo(games)))
                .then(function () {
                    return games;
                });
        }

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

        /**
         * Updates model with entitlements data. Used for handling underlying factories' events
         * @param {Object} model model/data transfer object
         * @return {void}
         */
        function updateEntitlementsInfo(model) {
            var offerId = model.offerId;

            if (isInCollection(offerId)) {
                collection.data[offerId] = merge(collection.data[offerId], loadEntitlementsInfo(model));
                collection.commit();
            }
        }

        /**
         * Grabs new entitlements data and modifies the current model accordingly
         * @return {void}
         */
        function onEntitlementChange(event) {
            var affectedModels = (event.length > 0) ? event : collection.data;

            forEach(updateEntitlementsInfo, affectedModels);
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
        function loadData(offerIds) {
            var offerIdsToLoad = toArray(filter(isNotInCollection, offerIds));

            if (offerIdsToLoad.length > 0) {
                createModels(offerIdsToLoad);

                GamesDataFactory.getCatalogInfo(offerIdsToLoad)
                    .then(mapWith(transformWith(CATALOG_DATA_MAP)))
                    .then(mapWith(loadEntitlementsInfo))
                    .then(loadPrices)
                    .then(mapWith(mockMissingData))
                    .then(updateCollection);
            }
        }

        /**
         * Updates data in the repository
         * @param {Object} newData - new models
         * @return {viod}
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

        /**
         * Resolves filtering conditions into the list of offer IDs
         * @param {Object} conditions filtering conditions
         * @return {Promise}
         */
        var findOfferIds = CacheFactory.decorate(function (conditions) {
            return Promise.resolve(conditions).then(StoreMockDataFactory.getEditionOfferIds);
        });

        /**
         * Returns a list of observable models filtered by given conditions
         * @param {Array} offerIds offer IDs
         * @return {Observer}
         */
        function filterBy(conditions) {
            findOfferIds(conditions).then(loadData);

            return ObserverFactory.create(collection).filterBy(conditions);
        }

        //setup a listeners for any entitlement updates
        GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', onEntitlementChange);

        return {
            get: get,
            filterBy: filterBy,
            findOfferIds: findOfferIds
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ProductFactorySingleton(ObjectHelperFactory, GamesDataFactory, ObservableFactory, StoreMockDataFactory, ObserverFactory, CacheFactory, SubscriptionFactory, LocaleFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ProductFactory', ProductFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ProductFactory

     * @description
     *
     * Store product factory
     */
    angular.module('origin-components')
        .factory('ProductFactory', ProductFactorySingleton);
}());

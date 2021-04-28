/**
 *  * @file factories/common/ocd-path.js
 */
(function () {
    'use strict';


    function OcdPathFactory(OfferTransformer, GamesDataFactory, ObservableFactory, ObserverFactory, ObjectHelperFactory, AuthFactory, SubscriptionFactory, OcdPathFactoryConstant) {
        var collection = ObservableFactory.observe({});
        var toArray = ObjectHelperFactory.toArray;

        /**
         * unwrap from {offerId: offer}
         * @param {Object} object {offerId: offer}
         * @returns {Object} offer
         */
        function unwrap(object) {
            return _.values(object)[0];
        }

        function transform(offer, ocdResponse) {
            if (ocdResponse) {
                if (offer && !_.isEmpty(offer)) {
                    return OfferTransformer.transformOffer(offer)
                        .then(unwrap)
                        .then(_.curry(transformWithOcd)(ocdResponse));
                } else {
                    return transformWithOcd(ocdResponse, handleError());
                }
            }
            return handleError();
        }

        function transformWithOcd(ocdResponse, offer) {
            var gamehub = ocdResponse.gamehub;
            if (gamehub) {
                offer.siblings = gamehub.siblings;
                offer.children = gamehub.children;
            }
            return offer;
        }

        function handleError() {
            return {error: OcdPathFactoryConstant.offerNotFoundError};
        }

        /**
         * Gets ocd data by path and retains first purchasable offer
         * @param ocdPath
         * @returns {Object}
         */
        function getOfferFromPath(ocdPath) {

            return Promise.all([GamesDataFactory.getCatalogByPath(ocdPath), GamesDataFactory.getOcdByPath(ocdPath)])
                .then(_.spread(transform))
                .catch(handleError);
        }


        /**
         * Initializes Observable collection
         * @param srcCollection src to iterative over.
         * @param collection to initialize
         */
        function initializeCollection(srcCollection) {
            _.forEach(srcCollection, function (ocdPath) {
                if(!collection.data[ocdPath]) {
                    collection.data[ocdPath] = {};
                }
            });
        }

        /**
         * Builds an array of purchasable offer from ocdPath promises
         * @param ocdPathsArray
         * @returns {Array} array of promises
         */
        function getOfferFromPathPromises(ocdPathsArray) {
            return _.map(ocdPathsArray, function (ocdPath) {
                return getOfferFromPath(ocdPath);
            });
        }

        function updateCollection(ocdPathsArray, result){
            _.forEach(ocdPathsArray, function (ocdPath, i) {
                collection.data[ocdPath] = result[i];
            });

            collection.commit();
        }

        /**
         * Filters paths from ocdPathsArray that exist in collection data
         *
         * @param  {string[]} ocdPathsArray Array of OCD paths
         * @return {string[]}
         */
        function filterCollectionPaths(ocdPathsArray) {
            return _.filter(ocdPathsArray, function(path) {
                return !collection.data.hasOwnProperty(path);
            });
        }

        /**
         * Retrieves purchasable offer(s) for give  OCD path(s) via observables
         *
         * @param {string[]} ocdPaths Array of OCD paths
         * @param {boolean} force     If true, forces retrieval from OCD rather than cached collection data
         * @returns {*}
         */
        function getOffersFromPaths(ocdPaths, force) {
            var observer = ObserverFactory.create(collection);
            var ocdPathsArray = toArray(ocdPaths);
            if(!force){
                ocdPathsArray = filterCollectionPaths(ocdPathsArray);
            }

            if(ocdPathsArray.length > 0 ) {
                initializeCollection(ocdPathsArray);

                Promise.all(getOfferFromPathPromises(ocdPathsArray))
                    .then(_.curry(updateCollection)(ocdPathsArray));
            }
            // Collection is a hash-map of model objects indexed by offer ID's.
            if (_.isString(ocdPaths)) {
                return observer.getProperty(ocdPaths).defaultTo({});
            } else {
                return observer.pick(ocdPaths);
            }
        }

        /**
         * Returns collection data for paths in ocdPaths array
         *
         * @param  {string[]} ocdPaths array of OCD path strings
         * @return {object}            collection of offer data, keyed by OCD path
         */
        function returnCollectionData(ocdPaths) {
            return _.pick(collection.data, toArray(ocdPaths));
        }

        /**
         * Retrieves purchasable offer(s) for give  OCD path(s) as a Promise
         *
         * @param {string[]} ocdPaths Array of OCD paths
         * @param {boolean} force     If true, forces retrieval from OCD rather than cached collection data
         * @returns {Promise}          Resolves to offer collection object
         */
        function getOffersFromPathsPromise(ocdPaths, force) {
            var ocdPathsArray = toArray(ocdPaths);
            if(!force){
                ocdPathsArray = filterCollectionPaths(ocdPathsArray);
            }

            if(ocdPathsArray.length > 0 ) {
                initializeCollection(ocdPathsArray);

                return Promise.all(getOfferFromPathPromises(ocdPathsArray))
                    .then(_.curry(updateCollection)(ocdPathsArray))
                    .then(_.partial(returnCollectionData, ocdPaths));
            }
            else {
                return Promise.resolve(returnCollectionData(ocdPaths));
            }
        }

        function reloadCollection() {
            getOffersFromPaths(_.keys(collection.data), true);
        }

        AuthFactory.events.on('myauth:change', reloadCollection);
        SubscriptionFactory.events.on('SubscriptionFactory:subscriptionUpdate', reloadCollection);
        GamesDataFactory.events.on('games:extraContentEntitlementsUpdate', reloadCollection);

        return {
            get: getOffersFromPaths,
            promiseGet: getOffersFromPathsPromise
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function OcdPathFactorySingleton(OfferTransformer, GamesDataFactory, ObservableFactory, ObserverFactory, ObjectHelperFactory, AuthFactory, SubscriptionFactory, OcdPathFactoryConstant, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('OcdPathFactory', OcdPathFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OcdPathFactory
     * @description maps an ocd path to an offer via Observable pattern
     *
     * OcdPathFactory
     */
    angular.module('origin-components')
        .constant('OcdPathFactoryConstant', {
            offerNotFoundError : 'OFFER_NOT_FOUND'
        })
        .factory('OcdPathFactory', OcdPathFactorySingleton);
}());

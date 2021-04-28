/**
 * @file game/gamespricing.js
 */
(function() {
    'use strict';

    function GamesPricingFactory(GamesCatalogFactory, ComponentsLogFactory) {
        var myEvents = new Origin.utils.Communicator();
            //anonymousPrice = {};
            //userPrice = {};

        function handleGetPriceError(error) {
            ComponentsLogFactory.error('getPrice failed:', error.message);
        }

        function handlePolymorphicResponse(priceList) {
            if (priceList.length === 1) {
                return priceList[0];
            } else {
                return priceList;
            }
        }

        function getPrice(offerIdList) {
            var offerList = [],
                //TODO: remove once we're able to retrieve it properly
                country = 'USA',
                currencies = ['USD'];

            if (Array.isArray(offerIdList)) {
                offerList = offerIdList;
            } else {
                offerList = [offerIdList];
            }

            //TODO: set country and currencies properly
            return Origin.games.getPrice(offerList, country, currencies)
                .then(handlePolymorphicResponse)
                .catch(handleGetPriceError);
        }


        function getBundlePrice(bundleId, offerIdList) {
            //TODO: remove once we're able to retrieve it properly
            var country = 'USA',
                currencies = ['USD'];

            if (!Array.isArray(offerIdList)) {
                ComponentsLogFactory.warn ('getBundlePrice expects offer list, not a single offer');
            }

            return Origin.games.getBundlePrice(bundleId, offerIdList, country, currencies)
                .catch(handleGetPriceError);
        }

        return {
            events: myEvents,

            /**
             * Get the price information
             * @param {string[]} a list of offerIds
             * @return {Promise} a promise for a list of pricing info
             * @method getPrice
             */
            getPrice: getPrice,

            /**
             * Get the price information for a bundle
             * @param {string} bundleId
             * @param {string[]} a list of offerIds in the bundle
             * @return {Promise} a promise for a list of pricing info
             * @method getBundlePrice
             */
            getBundlePrice: getBundlePrice
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesPricingFactorySingleton(GamesCatalogFactory, ComponentsLogFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesPricingFactory', GamesPricingFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesOcdFactory

     * @description
     *
     * GamesOcdFactory
     */
    angular.module('origin-components')
        .factory('GamesPricingFactory', GamesPricingFactorySingleton);
}());
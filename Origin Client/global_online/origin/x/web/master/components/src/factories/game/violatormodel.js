/**
 * @file game/violatormodel.js
 */
(function(){

    'use strict';

    function GameViolatorModelFactory(GamesDataFactory, ObjectHelperFactory) {
        var getProperty = ObjectHelperFactory.getProperty,
            transformWith = ObjectHelperFactory.transformWith,
            mapWith = ObjectHelperFactory.mapWith,
            currentPlatform = Origin.utils.os();

        /**
         * Get the Catalog model used for violators
         * @param  {string} offerId The oferId to download
         * @return {Promise.<Object, Error>} a promise with the model info
         */
        function getCatalog(offerId) {
            return GamesDataFactory.getCatalogInfo([offerId])
                .then(getProperty(offerId))
                .then(transformWith({
                    'beta': 'beta',
                    'trial': 'trial',
                    'demo': 'demo',
                    'downloadable': 'downloadable',
                    'purchasable': 'purchasable',
                    'preloadDate': ['platforms', currentPlatform, 'downloadStartDate'],
                    'releaseDate': ['platforms', currentPlatform, 'releaseDate'],
                    'originDisplayType': 'originDisplayType',
                    'offerId': 'offerId'
                }));
        }

        /**
         * Get every extra content model
         * @param  {string} parentOfferId The base game offer Id to use
         * @param  {string} filter        the filer enum to use (OWNEDONLY, UNOWNEDONLY)
         * @return {Promise.<Object{}, Error} resolves to an object of objects
         */
        function getExtraContent(parentOfferId, filter) {
            return GamesDataFactory.retrieveExtraContentCatalogInfo(parentOfferId, filter)
                .then(mapWith(transformWith({
                    'downloadable': 'downloadable',
                    'purchasable': 'purchasable',
                    'releaseDate': ['platforms', currentPlatform, 'releaseDate'],
                    'originDisplayType': 'originDisplayType',
                    'offerId': 'offerId'
                })));
        }

        /**
         * Get client model - passthru
         * @param {string} offerId the offerId
         * @return {Promise.<Object, Error>} resolves to an object
         */
        function getClient(offerId) {
            return Promise.resolve(GamesDataFactory.getClientGame(offerId));
        }

        /**
         * Get entitlement model - passthru
         * @param {string} offerId the offerId
         * @return {Promise<Object, Error>} resolves to an object
         */
        function getEntitlement(offerId) {
            return Promise.resolve(GamesDataFactory.getEntitlement(offerId));
        }

        /**
         * Get the game usage model for violators
         * @param  {string} offerId The offerId to analyze
         * @return {Promise.<Object, Error>} resolves to a game usage info model
         */
        function getGameUsage(offerId) {
            return GamesDataFactory.getGameUsageInfo(offerId)
                .then(transformWith({
                    'lastPlayedTime': 'lastPlayedTime'
                }));
        }

        /**
         * Get Origin Catalog Database (OCD) record
         * @param {string} offerId the offerId to analyze
         * @return {Promise.<violatorContent, Error>} resolves to an OCD response
         */
        function getOcd(offerId) {
            return GamesDataFactory.getOcdByOfferId(offerId);
        }

        return {
            getCatalog: getCatalog,
            getExtraContent: getExtraContent,
            getGameUsage: getGameUsage,
            getClient: getClient,
            getEntitlement: getEntitlement,
            getOcd: getOcd
        };
    }

    /**
    * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
    * @param {object} SingletonRegistryFactory the factory which manages all the other factories
    */
    function GameViolatorModelFactorySingleton(GamesDataFactory, ObjectHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GameViolatorModelFactory', GameViolatorModelFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GameViolatorModelFactory
     *
     * @description
     *
     * GameViolatorModelFactory
     */
    angular.module('origin-components')
        .factory('GameViolatorModelFactory', GameViolatorModelFactorySingleton);
})();
/**
 * @file violator/automatic/basegame.js
 */
(function(){

    'use strict';

    function ViolatorBaseGameFactory(ViolatorModelFactory, PlatformRefiner, EntitlementStateRefiner) {
        /**
         * Test update available status
         * @param {string} offerId the offer id to use
         * @return {Promise.<Boolean, Error>} Resolves to a boolean
         */
        function updateAvailable(offerId) {
            return ViolatorModelFactory.getClient(offerId)
                .then(EntitlementStateRefiner.isUpdateAvailable);
        }

        /**
         * Test game entitlement expiry - game has been revoked from the user's entitlements list
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function gameExpires(offerId) {
            return ViolatorModelFactory.getCatalogAndEntitlementData(offerId)
                .then(_.spread(EntitlementStateRefiner.isBaseGameExpiryImminentInDays(90)));
        }

        /**
         * Test if the game has a downloadOverride
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function downloadOverride(offerId) {
            return ViolatorModelFactory.getClient(offerId)
                .then(EntitlementStateRefiner.isDownloadOverride);
        }

        return {
            updateAvailable: updateAvailable,
            gameExpires: gameExpires,
            downloadOverride: downloadOverride
        };
    }

    /**
    * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
    * @param {object} SingletonRegistryFactory the factory which manages all the other factories
    */
    function ViolatorBaseGameFactorySingleton(ViolatorModelFactory, PlatformRefiner, EntitlementStateRefiner, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ViolatorBaseGameFactory', ViolatorBaseGameFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ViolatorBaseGameFactory

     * @description
     *
     * ViolatorBaseGameFactory -
     */
    angular.module('origin-components')
        .factory('ViolatorBaseGameFactory', ViolatorBaseGameFactorySingleton);
})();
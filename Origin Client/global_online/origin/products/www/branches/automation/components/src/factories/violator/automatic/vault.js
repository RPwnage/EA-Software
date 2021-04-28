/**
 * @file factories/violator/automatic/vault
 */
(function(){

    'use strict';

    function ViolatorVaultFactory(ViolatorModelFactory, VaultRefiner, EntitlementStateRefiner) {
        /**
         * Test criteria for offline play of vault content expiring
         * @param  {string} offerId  the offerid to test
         * @return {Promise.<Boolean, Error>}  resolves to a boolean
         */
        function offlinePlayExpiring(offerId) {
            return ViolatorModelFactory.getClient(offerId)
                .then(VaultRefiner.isOfflinePlayExpiryImminent(5));
        }

        /**
         * Test criteria for offline play of vault content expired
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function offlinePlayExpired(offerId) {
            return ViolatorModelFactory.getClient(offerId)
                .then(VaultRefiner.isOfflinePlayExpired);
        }

        /**
         * Test criteria for offline trial
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function offlineTrial(offerId) {
            return Promise.all([
                    ViolatorModelFactory.getCatalog(offerId),
                    ViolatorModelFactory.getClientOnline()
                ])
                .then(_.spread(VaultRefiner.isOfflineTrial));
        }

        /**
         * Test if an Early Access trial awaiting activation
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function trialNotActivated(offerId) {
            return ViolatorModelFactory.getTrialTime(offerId)
                .then(VaultRefiner.isTrialAwaitingActivation);
        }

        /**
         * Test if an Early Access trial is in progress
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function trialNotExpired(offerId) {
            return ViolatorModelFactory.getTrialTime(offerId)
                .then(VaultRefiner.isTrialInProgress);
        }

        /**
         * Test if an Early Access trial is in progress and not released
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function trialNotExpiredNotReleased(offerId) {
            return Promise.all([
                    ViolatorModelFactory.getTrialTime(offerId),
                    ViolatorModelFactory.isReleased(offerId)
                ])
                .then(_.spread(VaultRefiner.isTrialInProgressNotReleased));
        }

        /**
         * Test if an Early Access trial is in progress and released
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function trialNotExpiredReleased(offerId) {
            return Promise.all([
                    ViolatorModelFactory.getTrialTime(offerId),
                    ViolatorModelFactory.isReleased(offerId)
                ])
                .then(_.spread(VaultRefiner.isTrialInProgressReleased));
        }

        /**
         * Test if an Early Access trial has expired
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function trialExpired(offerId) {
            return ViolatorModelFactory.getTrialTime(offerId)
                .then(VaultRefiner.isTrialExpired);
        }

        /**
         * Test if an Early Access trial has expired, not released and is not hidden in the game library
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function trialExpiredAndVisibleNotReleased(offerId) {
            return Promise.all([
                    ViolatorModelFactory.getTrialTime(offerId),
                    ViolatorModelFactory.isHidden(offerId),
                    ViolatorModelFactory.isReleased(offerId)
                ])
                .then(_.spread(VaultRefiner.isTrialExpiredAndVisibleNotReleased));
        }

        /**
         * Test if an Early Access trial has expired, not released and is hidden in the game library
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function trialExpiredAndHiddenNotReleased(offerId) {
            return Promise.all([
                    ViolatorModelFactory.getTrialTime(offerId),
                    ViolatorModelFactory.isHidden(offerId),
                    ViolatorModelFactory.isReleased(offerId)
                ])
                .then(_.spread(VaultRefiner.isTrialExpiredAndHiddenNotReleased));
        }

        /**
         * Test if an Early Access trial has expired, released and is not hidden in the game library
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function trialExpiredAndVisibleReleased(offerId) {
            return Promise.all([
                    ViolatorModelFactory.getTrialTime(offerId),
                    ViolatorModelFactory.isHidden(offerId),
                    ViolatorModelFactory.isReleased(offerId)
                ])
                .then(_.spread(VaultRefiner.isTrialExpiredAndVisibleReleased));
        }

        /**
         * Test if an Early Access trial has expired, released and is hidden in the game library
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function trialExpiredAndHiddenReleased(offerId) {
            return Promise.all([
                    ViolatorModelFactory.getTrialTime(offerId),
                    ViolatorModelFactory.isHidden(offerId),
                    ViolatorModelFactory.isReleased(offerId)
                ])
                .then(_.spread(VaultRefiner.isTrialExpiredAndHiddenReleased));
        }

        /**
         * Test if Origin Access subscription has expired for the game
         * @param {string} offerId the offerid to test
         * @return {Promise<Boolean, Error>} resolves to a boolean
         */
        function subscriptionExpired(offerId) {
            return Promise.all([
                    ViolatorModelFactory.getEntitlement(offerId),
                    ViolatorModelFactory.getSubscriptionActive()
                ])
                .then(_.spread(EntitlementStateRefiner.isVaultGameExpired));
        }

        /**
         * Test if the game qualifies for an updgrade to a better vault edition
         * @param {string} offerId the offerid to test
         * @return {Promise<Boolean, Error>} resolves to a boolean
         */
        function vaultGameUpgrade(offerId) {
            return Promise.all([
                    ViolatorModelFactory.getSubscriptionActive(),
                    ViolatorModelFactory.isVaultEditionOwned(offerId),
                    ViolatorModelFactory.isMissingBasegameVaultEditionOrDLC(offerId),
                    ViolatorModelFactory.isUpgradeableVaultGame(offerId)
                ])
                .then(_.spread(VaultRefiner.isGameEligibleForUpgrade));
        }

        return {
            offlinePlayExpiring: offlinePlayExpiring,
            offlinePlayExpired: offlinePlayExpired,
            trialNotActivated: trialNotActivated,
            trialNotExpired: trialNotExpired,
            trialNotExpiredNotReleased: trialNotExpiredNotReleased,
            trialNotExpiredReleased: trialNotExpiredReleased,
            trialExpired: trialExpired,
            trialExpiredAndVisibleNotReleased: trialExpiredAndVisibleNotReleased,
            trialExpiredAndHiddenNotReleased: trialExpiredAndHiddenNotReleased,
            trialExpiredAndVisibleReleased: trialExpiredAndVisibleReleased,
            trialExpiredAndHiddenReleased: trialExpiredAndHiddenReleased,
            subscriptionExpired: subscriptionExpired,
            offlineTrial: offlineTrial,
            vaultGameUpgrade: vaultGameUpgrade
        };
    }

    /**
    * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
    * @param {object} SingletonRegistryFactory the factory which manages all the other factories
    */
    function ViolatorVaultFactorySingleton(ViolatorModelFactory, VaultRefiner, EntitlementStateRefiner, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ViolatorVaultFactory', ViolatorVaultFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ViolatorVaultFactory

     * @description
     *
     * ViolatorVaultFactory
     * This Factory supports vault specific game states
     */
    angular.module('origin-components')
        .factory('ViolatorVaultFactory', ViolatorVaultFactorySingleton);
})();
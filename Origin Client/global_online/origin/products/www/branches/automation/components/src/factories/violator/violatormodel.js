/**
 * @file game/violatormodel.js
 */
(function(){

    'use strict';

    function ViolatorModelFactory(GamesDataFactory, ObjectHelperFactory, SubscriptionFactory, AuthFactory, GamesTrialFactory, EntitlementStateRefiner, GamesUsageFactory, OwnershipFactory, VaultRefiner, PlatformRefiner) {
        var getProperty = ObjectHelperFactory.getProperty,
            getClient = GamesDataFactory.getClientGamePromise,
            getExtraContent = GamesDataFactory.retrieveExtraContentCatalogInfo,
            getOcd = GamesDataFactory.getOcdByOfferId,
            getLastPlayedTimeByOfferId = GamesUsageFactory.getLastPlayedTimeByOfferId;

        /**
         * Get the Catalog model used for violators
         * @param  {string} offerId The oferId to download
         * @return {Promise.<Object, Error>} a promise with the model info
         */
        function getCatalog(offerId) {
            return GamesDataFactory.getCatalogInfo([offerId])
                .then(getProperty(offerId));
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
         * Get Catalog and entitlement data
         * @param {string} offerId the offer Id
         * @return {Promise.<Object, Error>}
         */
        function getCatalogAndEntitlementData(offerId) {
            return Promise.all([
                getCatalog(offerId),
                getEntitlement(offerId)
            ]);
        }

        /**
         * Get the client and entitlement data
         * @param  {Object} ownedContentItem an owned content item
         * @return {Promise.<Object[], Error>} an object array [clientData, entitlementData]
         */
        function getClientAndEntitlementData(ownedContentItem){
            return Promise.all([
                getClient(ownedContentItem.offerId),
                getEntitlement(ownedContentItem.offerId)
            ]);
        }

        /**
         * Get the client and baseGameLastPlayed in a promise context
         * @param {Object} ownedContentItem an owned content item
         * @param {Object} baseGameLastPlayed last played Date
         * @return {Promise.<Object[], Error>} an object array [clientData, entitlementData]
         */
        function getClientAndBaseGameLastPlayedData(ownedContentItem, baseGameLastPlayed){
            return Promise.all([
                getClient(ownedContentItem.offerId),
                baseGameLastPlayed
            ]);
        }

        /**
         * Get the OA early accesss trial times from the catalog and subs service.
         * Also cache the result for offline use cases.
         * @param  {string} offerId the offerId
         * @return {Promise<Object, error>} the trial time info
         */
        function getTrialTime(offerId) {
            return GamesTrialFactory.getTrialTimeByOfferId(offerId);
        }

        /**
         * check to see if subscription is active
         * @return {Promise<boolean, error>} the subscription active value
         */
        function getSubscriptionActive() {
            return Promise.resolve(SubscriptionFactory.isActive());
        }

        /**
         * Check to see if client is online
         * @return {Promise<boolean, error>} the client online state
         */
        function getClientOnline() {
            return Promise.resolve(AuthFactory.isClientOnline());
        }

        function handleIsUpgradeableError() {
            return Promise.resolve(false);
        }

        /**
         * Check if the offer is upgradeable
         * @param {string} offerId the offerId to analyze
         * @return {Promise<boolean, error>} true if upgradeable
         */
        function isUpgradeableVaultGame(offerId) {
            return getCatalog(offerId)
                    .then(VaultRefiner.isUpgradeable)
                    .catch(handleIsUpgradeableError);
        }

        function getVaultCatalog(catalogInfo) {
            return _.first(_.values(catalogInfo));
        }

        /**
         * Given the catalog info, retrieve the offer id for the vault edition
         * @param  {Object} catalogInfo the catalog info
         * @return {Mixed} offer id
         */
        function resolveVaultOfferId(catalogInfo) {
            var path = _.get(catalogInfo, ['vaultInfo', 'path']);

            if(path && (path !== _.get(catalogInfo, 'offerPath'))) {
                return GamesDataFactory.getCatalogByPath(path).then(getVaultCatalog);
            } else {
                return catalogInfo;
            }
        }

        function isVaultBundleContentsOwned(catalogInfo) {
            var isOwned = true,
                offerIds = _.get(catalogInfo, 'bundleOffers');

            if(_.size(offerIds) > 0) {
                _.forEach(offerIds, function(offerId) {
                    if(!GamesDataFactory.ownsEntitlement(offerId)){
                        isOwned = false;
                    }
                });

                return isOwned;
            } else {
                return false;
            }
        }

        function checkVaultGameOwnership(catalogInfo) {
            return _.get(catalogInfo, 'isOwned') === true || isVaultBundleContentsOwned(catalogInfo);
        }

        function handleVaultOwnershipError() {
            return Promise.resolve(false);
        }

        /**
         * Check if the Vault edition is owned
         * @param {string} offerId the offerId to analyze
         * @return {Promise<boolean, error>} true if owned
         */
        function isVaultEditionOwned(offerId) {
            return getCatalog(offerId)
                .then(resolveVaultOfferId)
                .then(OwnershipFactory.loadEntitlementsInfo)
                .then(checkVaultGameOwnership)
                .catch(handleVaultOwnershipError);
        }

        /**
         * Chek if the vault edition or DLCs are missing
         * @param {offerId} offerId [description] for offerId to analyze
         * @return {Promise<boolean>} true if missing
         */
        function isMissingBasegameVaultEditionOrDLC(offerId) {
            return GamesDataFactory.isMissingBasegameVaultEditionOrDLC(offerId);
        }

        /**
         * Chek if the game is hidden
         * @param {string} offerId the offerId
         * @return {Promise<boolean, error>} the game hidden state
         */
        function isHidden(offerId) {
            return Promise.resolve(GamesDataFactory.isHidden(offerId));
        }

        function determineReleaseStatus(catalogInfo) {
            var currentPlatform = Origin.utils.os();
            if (PlatformRefiner.isAvailableOnPlatform(currentPlatform)(catalogInfo)) {
                return PlatformRefiner.isReleased(currentPlatform)(catalogInfo);
            } else {
                return PlatformRefiner.isReleasedOnAnyPlatform()(catalogInfo);
            }
        }

        function handleCatalogError() {
            return Promise.resolve(false);
        }

        /**
         * Check if the game is released
         * @param {string} offerId the offerId
         * @return {Promise<boolean, error>} the game released state
         */
        function isReleased(offerId) {
            return getCatalog(offerId)
                .then(determineReleaseStatus)
                .catch(handleCatalogError);
        }

        return {
            getCatalog: getCatalog,
            getExtraContent: getExtraContent,
            getLastPlayedTimeByOfferId: getLastPlayedTimeByOfferId,
            getClient: getClient,
            getEntitlement: getEntitlement,
            getOcd: getOcd,
            getCatalogAndEntitlementData: getCatalogAndEntitlementData,
            getClientAndEntitlementData: getClientAndEntitlementData,
            getClientAndBaseGameLastPlayedData: getClientAndBaseGameLastPlayedData,
            getTrialTime: getTrialTime,
            getSubscriptionActive: getSubscriptionActive,
            getClientOnline: getClientOnline,
            isVaultEditionOwned: isVaultEditionOwned,
            isMissingBasegameVaultEditionOrDLC: isMissingBasegameVaultEditionOrDLC,
            isHidden: isHidden,
            isReleased: isReleased,
            isUpgradeableVaultGame: isUpgradeableVaultGame
        };
    }

    /**
    * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
    * @param {object} SingletonRegistryFactory the factory which manages all the other factories
    */

    function ViolatorModelFactorySingleton(GamesDataFactory, ObjectHelperFactory, SubscriptionFactory, AuthFactory, GamesTrialFactory, EntitlementStateRefiner, GamesUsageFactory, OwnershipFactory, VaultRefiner, PlatformRefiner, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ViolatorModelFactory', ViolatorModelFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ViolatorModelFactory
     *
     * @description
     *
     * ViolatorModelFactory
     */
    angular.module('origin-components')
        .factory('ViolatorModelFactory', ViolatorModelFactorySingleton);
})();
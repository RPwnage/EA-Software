/**
 * @file violator/automatic/dlc.js
 */
(function(){

    'use strict';

    var EXTRA_CONTENT_CACHE_TIMEOUT_SECONDS = 86400;

    function ViolatorDlcFactory(ObjectHelperFactory, FunctionHelperFactory, ViolatorModelFactory, GameRefiner, EntitlementStateRefiner, PlatformRefiner, DateHelperFactory, CacheFactory) {
        var or = FunctionHelperFactory.or,
            and = FunctionHelperFactory.and,
            filterBy = ObjectHelperFactory.filterBy,
            toArray = ObjectHelperFactory.toArray,
            map = ObjectHelperFactory.map,
            mapWith = ObjectHelperFactory.mapWith,
            hasTruth = ObjectHelperFactory.hasTruth,
            isValidDate = DateHelperFactory.isValidDate,
            //TODO: we should be able to make this call less heavy by not dirty checking private unowned EC
            //whenever we reload the game library. For now, run it up to hourly as the only content relying on it is a
            //nag to buy DLC.
            getExtraContent = function() {
                return ViolatorModelFactory.getExtraContent.apply(undefined, arguments);
            },
            cachedGetExtraContent = CacheFactory.decorate(getExtraContent, EXTRA_CONTENT_CACHE_TIMEOUT_SECONDS);

        /**
         * Validate all of the DLC responses
         * -game is a full game
         * -extra content collection has cardinality > 0
         * @param {Boolean} isBaseGame true if the game is a full game
         * @param {Object} extraContent the extra content collection
         * @return {Boolean}
         */
        function isValidDlcResponse(isBaseGame, extraContent) {
            return (isBaseGame && hasTruth(extraContent)) ? true : false;
        }

        /**
         * is an Addon or expansion ready for install?
         * @param {Boolean} isValidParent true if the parent game is a downloadable base game
         * @param {Object} ownedContent owned content collection
         * @return {Boolean}
         */
        function isNewDlcReadyForInstall(isValidParent, ownedContent) {
            if (!isValidParent) {
                return false;
            }

            var itemsAwaitingDownloadOrInstall;

            itemsAwaitingDownloadOrInstall = map(function(ownedContentItem) {
                return ViolatorModelFactory.getClientAndEntitlementData(ownedContentItem)
                    .then(_.spread(EntitlementStateRefiner.isAddonOrExpansionAwatingDownloadInDays(28)));
            }, ownedContent);

            return Promise.all(toArray(itemsAwaitingDownloadOrInstall))
                .then(hasTruth);
        }

        /**
         * Filter on new dlc installed
         * -Game is a full game of full game plus expansion
         * -Game has not been played since new dlc was installed
         * -Addon or Expansion is owned
         * -Addon or expansion is in the installed state
         * -Addon or expansion was installed up to 7 days ago
         * @param {Boolean} isValidParent true if the parent game is a downloadable base game
         * @param {Object} ownedContent the owned content list
         * @param {Date} baseGameLastPlayed the last played time
         * @return {Promise.<Boolean, Error>} Resolves to a boolean
         */
        function isNewDlcInstalled(isValidParent, ownedContent, baseGameLastPlayed) {
            if (!isValidParent || !isValidDate(baseGameLastPlayed)) {
                return false;
            }

            var itemsRecentlyInstalled;

            itemsRecentlyInstalled = map(function(ownedContentItem) {
                return ViolatorModelFactory.getClientAndBaseGameLastPlayedData(ownedContentItem, baseGameLastPlayed)
                    .then(_.spread(EntitlementStateRefiner.isAddonOrExpansionRecentlyInstalledInDays(7)));
            }, ownedContent);

            return Promise.all(toArray(itemsRecentlyInstalled))
                .then(hasTruth);
        }

        /**
         * Test for new dlc available (DLC is a marketing alias for the Addon type)
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function newDlcAvailable(offerId) {
            return Promise.all([
                    ViolatorModelFactory.getCatalog(offerId)
                        .then(GameRefiner.isBaseGame),
                    cachedGetExtraContent(offerId, 'UNOWNEDONLY')
                        .then(mapWith(and([
                            GameRefiner.isDownloadable,
                            GameRefiner.isPurchasable,
                            GameRefiner.isAddon,
                            PlatformRefiner.isRecentlyReleasedInDays(Origin.utils.os(), 7)
                        ])))
                        .then(toArray)
                ])
                .then(_.spread(isValidDlcResponse));
        }

        /**
         * Test criteria for new expansion available
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function newDlcExpansionAvailable(offerId) {
            return Promise.all([
                    ViolatorModelFactory.getCatalog(offerId)
                        .then(GameRefiner.isBaseGame),
                    cachedGetExtraContent(offerId, 'UNOWNEDONLY')
                        .then(mapWith(and([
                            GameRefiner.isDownloadable,
                            GameRefiner.isPurchasable,
                            GameRefiner.isExpansion,
                            PlatformRefiner.isRecentlyReleasedInDays(Origin.utils.os(), 7)
                        ])))
                        .then(toArray)
                ])
                .then(_.spread(isValidDlcResponse));
        }

        /**
         * Tests for new addons ready for install
         * @param {string} offerId the offer id to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function newDlcReadyForInstall(offerId) {
            return Promise.all([
                ViolatorModelFactory.getCatalog(offerId)
                    .then(and([GameRefiner.isBaseGame, GameRefiner.isDownloadable])),
                ViolatorModelFactory.getExtraContent(offerId, 'OWNEDONLY')
                    .then(filterBy(and([GameRefiner.isDownloadable, GameRefiner.isAddon])))
            ]).then(_.spread(isNewDlcReadyForInstall));
        }

        /**
         * Tests for new expansions ready for install
         * @param {string} offerId the offer id to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function newDlcExpansionReadyForInstall(offerId) {
            return Promise.all([
                ViolatorModelFactory.getCatalog(offerId)
                    .then(and([GameRefiner.isBaseGame, GameRefiner.isDownloadable])),
                ViolatorModelFactory.getExtraContent(offerId, 'OWNEDONLY')
                    .then(filterBy(and([GameRefiner.isDownloadable, GameRefiner.isExpansion])))
            ]).then(_.spread(isNewDlcReadyForInstall));
        }

        /**
         * Test criteria for newly installed DLC
         * @param {string} offerId he offerid to test
         * @return {Promise.<Boolean, Error>} [description]
         */
        function newDlcInstalled(offerId) {
            return Promise.all([
                ViolatorModelFactory.getCatalog(offerId)
                    .then(and([GameRefiner.isBaseGame, GameRefiner.isDownloadable])),
                ViolatorModelFactory.getExtraContent(offerId, 'OWNEDONLY')
                    .then(filterBy(GameRefiner.isDownloadable))
                    .then(filterBy(or([GameRefiner.isAddon, GameRefiner.isExpansion]))),
                ViolatorModelFactory.getLastPlayedTimeByOfferId(offerId)
            ]).then(_.spread(isNewDlcInstalled));
        }

        return {
            newDlcAvailable: newDlcAvailable,
            newDlcExpansionAvailable: newDlcExpansionAvailable,
            newDlcReadyForInstall: newDlcReadyForInstall,
            newDlcExpansionReadyForInstall: newDlcExpansionReadyForInstall,
            newDlcInstalled: newDlcInstalled
        };
    }

    /**
    * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
    * @param {object} SingletonRegistryFactory the factory which manages all the other factories
    */
    function ViolatorDlcFactorySingleton(ObjectHelperFactory, FunctionHelperFactory, ViolatorModelFactory, GameRefiner, EntitlementStateRefiner, PlatformRefiner, DateHelperFactory, CacheFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ViolatorDlcFactory', ViolatorDlcFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ViolatorDlcFactory

     * @description
     *
     * ViolatorDlcFactory
     * Tools for evaluating dlc states related to the base game
     */
    angular.module('origin-components')
        .factory('ViolatorDlcFactory', ViolatorDlcFactorySingleton);
})();
/**
 * @file game/violatorautomatic.js
 */
(function(){

    'use strict';

    function GameAutomaticViolatorTestsFactory(ObjectHelperFactory, FunctionHelperFactory, GameViolatorModelFactory, GameViolatorFilterFactory) {
        var gameFilters = GameViolatorFilterFactory,
            gameModel = GameViolatorModelFactory,
            or = FunctionHelperFactory.or,
            filterBy = ObjectHelperFactory.filterBy,
            toArray = ObjectHelperFactory.toArray,
            map = ObjectHelperFactory.map;

        /**
         * Get Catalog and entitlement data
         * @param {string} offerId the offer Id
         * @return {Promise.<Object, Error}
         */
        function getCatalogAndEntitlementData(offerId) {
            return Promise.all([
                    gameModel.getCatalog(offerId),
                    gameModel.getEntitlement(offerId)
            ]);
        }

        /**
         * Get the client and entitlement data
         * @param  {Object} ownedContentItem an owned content item
         * @return {Promise.<Object[], Error>} an object array [clientData, entitlementData]
         */
        function getClientAndEntitlementData(ownedContentItem){
            return Promise.all([
                gameModel.getClient(ownedContentItem.offerId),
                gameModel.getEntitlement(ownedContentItem.offerId)
            ]);
        }

        /**
         * Test if the extra content collection has a cardinality > 0
         * @param  {Object}  extraContent the exctra content collection to test
         * @return {Boolean}
         */
        function hasExtraContent(extraContent) {
            var index;

            if(typeof(extraContent) === 'object') {
                for (index in extraContent) {}
                if(index) {
                    return true;
                }
            }

            return false;
        }

        /**
         * Validate all of the DLC responses
         * -game is a full game
         * -extra content collection has cardinality > 0
         * @param  {Array} data [bool, obj]
         * @return {Boolean}
         */
        function isValidDlcResponse(data) {
            var isFullGame = data[0];
            var extraContent = data[1];

            return (isFullGame && hasExtraContent(extraContent)) ? true : false;
        }

        /**
         * Ensure that given list has at least one true element
         * @param {Array} list a list of booleans [true,false,...]
         * @return {Boolean}
         */
        function hasTruth(list) {
            return (list.indexOf(true) > -1);
        }

        /**
         * is an Addon or expansion ready for install?
         * @param {Object[]} data [isFullgame, ownedContent]
         * @return {Boolean}
         */
        function isNewDlcReadyForInstall(data) {
            var isFullGame = data[0],
                ownedContent = data[1],
                itemsAwaitingDownloadOrInstall;

            if (!isFullGame) {
                return false;
            }

            itemsAwaitingDownloadOrInstall = map(function(ownedContentItem) {
                return getClientAndEntitlementData(ownedContentItem)
                    .then(gameFilters.isAddonOrExpansionAwatingDownload);
            }, ownedContent);

            return Promise.all(toArray(itemsAwaitingDownloadOrInstall))
                .then(hasTruth);
        }

        /**
         * Get the client and baseGameLastPlayed in a promise context
         * @param  {Object} ownedContentItem an owned content item
         * @return {Promise.<Object[], Error>} an object array [clientData, entitlementData]
         */
        function getClientAndBaseGameLastPlayedData(ownedContentItem, baseGameLastPlayed){
            return Promise.all([
                gameModel.getClient(ownedContentItem.offerId),
                baseGameLastPlayed
            ]);
        }

        /**
         * Filter on new dlc installed
         * -Game is a full game of full game plus expansion
         * -Game has not been played since new dlc was installed
         * -Addon or Expansion is owned
         * -Addon or expansion is in the installed state
         * -Addon or expansion was installed up to 7 days ago
         * @param  {Object[]} data [catalogData, ownedContent, gameUsage]
         * @return {[type]}      [description]
         */
        function newDlcInstalledFilter(data){
            var isFullGame = data[0],
                ownedContent = data[1],
                gameUsage = data[2],
                itemsRecentlyInstalled,
                baseGameLastPlayed = (gameUsage.lastPlayedTime) ? new Date(gameUsage.lastPlayedTime) : undefined;

            if (!isFullGame) {
                return false;
            }

            itemsRecentlyInstalled = map(function(ownedContentItem) {
                return getClientAndBaseGameLastPlayedData(ownedContentItem, baseGameLastPlayed)
                    .then(gameFilters.isAddonOrExpansionRecentlyInstalled);
            }, ownedContent);

            return Promise.all(toArray(itemsRecentlyInstalled))
                .then(hasTruth);
        }

        /**
         * Test preload on status
         * @param  {string} offerId the offerId to test
         * @return {Promise.<Boolean, Error>} Resolves to a boolean
         */
        function preloadOn(offerId) {
            return gameModel.getCatalog(offerId)
                .then(gameFilters.isPreloadImminent);
        }

        /**
         * Test preload available status
         * @param {string} offerId the offerId to test
         * @return {Promise.<Boolean, Error>} Resolves to a boolean
         */
        function preloadAvailable(offerId) {
            return gameModel.getClient(offerId)
                .then(gameFilters.isPreloadAvailable);
        }

        /**
         * Test update available status
         * @param {string} offerId the offer id to use
         * @return {Promise.<Boolean, Error>} Resolves to a boolean
         */
        function updateAvailable(offerId) {
            return gameModel.getClient(offerId)
                .then(gameFilters.isUpdateAvailable);
        }

        /**
         * Test releases on status
         * @param {string} offerId the offerId to test
         * @return {Promise.<Boolean, Error>} Resolves to a boolean
         */
        function releasesOn(offerId) {
            return gameModel.getCatalog(offerId)
                .then(gameFilters.isReleaseDateImminent);
        }

        /**
         * Test game expires status
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function gameExpires(offerId) {
            return getCatalogAndEntitlementData(offerId)
                .then(gameFilters.isGameExpiryImminent);
        }

        /**
         * Test criteria for trial not activated
         * -no entitlement termination date set
         * -game is a trial
         * @param  {string} offerId  the offerid to test
         * @return {Promise.<Boolean, Error>}  resolves to a boolean
         */
        function trialNotActivated(offerId) {
            return getCatalogAndEntitlementData(offerId)
                .then(gameFilters.isTrialAwaitingActivation);
        }

        /**
         * Check if trial is not expired
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function trialNotExpired(offerId) {
            return getCatalogAndEntitlementData(offerId)
                .then(gameFilters.isTrialInProgress);
        }

        /**
         * Test if a trial is expired
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function trialExpired(offerId) {
            return getCatalogAndEntitlementData(offerId)
                .then(gameFilters.isTrialExpired);
        }

        /**
         * Test for new dlc available (DLC is a marketing alias for the Addon type)
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function newDlcAvailable(offerId) {
            return Promise.all([
                    gameModel.getCatalog(offerId)
                        .then(or([gameFilters.isFullGame, gameFilters.isFullGamePlusExpansion])),
                    gameModel.getExtraContent(offerId, 'UNOWNEDONLY')
                        .then(filterBy(gameFilters.isDownloadable))
                        .then(filterBy(gameFilters.isPurchasable))
                        .then(filterBy(gameFilters.isAddonRecentlyReleased))
                ])
                .then(isValidDlcResponse);
        }

        /**
         * Test criteria for new expansion available
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function newDlcExpansionAvailable(offerId){
            return Promise.all([
                    gameModel.getCatalog(offerId)
                        .then(or([gameFilters.isFullGame, gameFilters.isFullGamePlusExpansion])),
                    gameModel.getExtraContent(offerId, 'UNOWNEDONLY')
                        .then(filterBy(gameFilters.isDownloadable))
                        .then(filterBy(gameFilters.isPurchasable))
                        .then(filterBy(gameFilters.isExpansionRecentlyReleased))
                ])
                .then(isValidDlcResponse);
        }

        /**
         * Tests for new dlc ready for install
         * @param {string} offerId the offer id to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function newDlcReadyForInstall(offerId) {
            return Promise.all([
                gameModel.getCatalog(offerId)
                    .then(or([gameFilters.isFullGame, gameFilters.isFullGamePlusExpansion])),
                gameModel.getExtraContent(offerId, 'OWNEDONLY')
                    .then(filterBy(gameFilters.isDownloadable))
                    .then(filterBy(or([gameFilters.isAddon, gameFilters.isExpansion])))
            ]).then(isNewDlcReadyForInstall);
        }


        /**
         * Test criteria for newly installed DLC
         * @param {string} offerId he offerid to test
         * @return {Promise.<Boolean, Error>} [description]
         */
        function newDlcInstalled(offerId) {
            return Promise.all([
                gameModel.getCatalog(offerId)
                    .then(or([gameFilters.isFullGame, gameFilters.isFullGamePlusExpansion])),
                gameModel.getExtraContent(offerId, 'OWNEDONLY')
                    .then(filterBy(gameFilters.isDownloadable))
                    .then(filterBy(or([gameFilters.isAddon, gameFilters.isExpansion]))),
                gameModel.getGameUsage(offerId)
            ]).then(newDlcInstalledFilter);
        }

        return {
            preloadOn: preloadOn,
            preloadAvailable: preloadAvailable,
            releasesOn: releasesOn,
            updateAvailable: updateAvailable,
            gameExpires: gameExpires,
            trialNotActivated: trialNotActivated,
            trialNotExpired: trialNotExpired,
            trialExpired: trialExpired,
            newDlcAvailable: newDlcAvailable,
            newDlcExpansionAvailable: newDlcExpansionAvailable,
            newDlcReadyForInstall: newDlcReadyForInstall,
            newDlcInstalled: newDlcInstalled
        };
    }

    /**
    * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
    * @param {object} SingletonRegistryFactory the factory which manages all the other factories
    */
    function GameAutomaticViolatorTestsFactorySingleton(ObjectHelperFactory, FunctionHelperFactory, GameViolatorModelFactory, GameViolatorFilterFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GameAutomaticViolatorTestsFactory', GameAutomaticViolatorTestsFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GameAutomaticViolatorTestsFactory

     * @description
     *
     * GameAutomaticViolatorTestsFactory
     */
    angular.module('origin-components')
        .factory('GameAutomaticViolatorTestsFactory', GameAutomaticViolatorTestsFactorySingleton);
})();
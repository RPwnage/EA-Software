/**
 * @file game/violatorfilters.js
 */
(function(){

    'use strict';

    function GameViolatorFilterFactory(DateHelperFactory) {
        var addDays = DateHelperFactory.addDays,
            isInTheFuture = DateHelperFactory.isInTheFuture,
            isInThePast = DateHelperFactory.isInThePast,
            isValidDate = DateHelperFactory.isValidDate;

        /**
         * Is there a future preload event for the given game
         * @param {Object} catalogData the catalog data model
         * @return {Boolean}
         */
        function isPreloadImminent(catalogData) {
            return (
                !catalogData.beta &&
                !catalogData.trial &&
                !catalogData.demo &&
                catalogData.downloadable &&
                isInTheFuture(catalogData.preloadDate)
            ) ? true: false;
        }

        /**
         * Is a Game ready to preload?
         * @param {Object} clientData the client data model
         * @return {Boolean}
         */
        function isPreloadAvailable(clientData) {
            return (clientData.releaseStatus &&
                clientData.releaseStatus === 'PRELOAD'
            ) ? true : false;
        }

        /**
         * Is there a future release date set for this game?
         * @param {Object} catalogData the catalog data model
         * @return {Boolean}
         */
        function isReleaseDateImminent(catalogData) {
            if( isValidDate(catalogData.preloadDate) &&
                isValidDate(catalogData.releaseDate) &&
                catalogData.preloadDate === catalogData.releaseDate) {
                return (isInTheFuture(catalogData.releaseDate));
            } else if(isValidDate(catalogData.releaseDate)) {
                return (isInTheFuture(catalogData.releaseDate));
            }

            return false;
        }

        /**
         * Is the game in an update state?
         * @param {Object} clientData the client data model
         * @return {Boolean}
         */
        function isUpdateAvailable(clientData) {
            return (clientData &&
                clientData.updateAvailable &&
                clientData.updateAvailable === true
            ) ? true : false;
        }

        /**
         * Is a non trial game expiring in the future?
         * @param {Object[]} data [catalogData, entitlementData]
         * @return {Boolean}
         */
        function isGameExpiryImminent(data) {
            var catalogData = data[0],
                entitlementData = data[1];

            return (
                catalogData &&
                entitlementData &&
                !catalogData.trial &&
                entitlementData.terminationDate &&
                isInTheFuture(entitlementData.terminationDate)
            ) ? true : false;
        }

        /**
         * Is a trial game awaiting activation?
         * @param {Object[]} data [catalogData, entitlementData]
         * @return {Boolean}
         */
        function isTrialAwaitingActivation(data) {
            var catalogData = data[0],
                entitlementData = data[1];

            return (
                catalogData &&
                entitlementData &&
                catalogData.trial &&
                !entitlementData.terminationDate
            ) ? true : false;
        }

        /**
         * Is a game trial in progress?
         * @param {Object[]} data [catalogData, entitlementData]
         * @return {Boolean}
         */
        function isTrialInProgress(data) {
            var catalogData = data[0],
                entitlementData = data[1];

            return (
                catalogData &&
                entitlementData &&
                catalogData.trial &&
                entitlementData.terminationDate &&
                isInTheFuture(entitlementData.terminationDate)
            ) ? true : false;
        }

        /**
         * Is the game trial expired?
         * @param {Object[]} data [catalogData, entitlementData]
         * @return {Boolean}
         */
        function isTrialExpired(data) {
            var catalogData = data[0],
                entitlementData = data[1];

            return (
                catalogData &&
                entitlementData &&
                catalogData.trial &&
                entitlementData.terminationDate &&
                isInThePast(entitlementData.terminationDate)
            ) ? true : false;
        }

        /**
         * is a catalog item purchasable
         * @param  {Object} catalogData the catalog data model
         * @return {Boolean}
         */
        function isPurchasable(catalogData) {
            return catalogData && catalogData.purchasable ? true : false;
        }

        /**
         * is a catalog item downloadable
         * @param  {Object} catalogData the catalog data model
         * @return {Boolean}
         */
        function isDownloadable(catalogData) {
            return catalogData && catalogData.downloadable ? true : false;
        }

        /**
         * is catalog item a full game?
         * @param {Object} catalogData the catalog data model
         * @return {Boolean}
         */
        function isFullGame(catalogData) {
            return catalogData && (catalogData.originDisplayType === 'Full Game') ? true : false;
        }

        /**
         * is catalog item a full game with expansions?
         * @param {Object} catalogData the catalog data model
         * @return {Boolean}
         */
        function isFullGamePlusExpansion(catalogData) {
            return catalogData && (catalogData.originDisplayType === 'Full Game Plus Expansion') ? true : false;
        }

        /**
         * is the catalog data an addon?
         * @param  {object} catalogData the catalog data model
         * @return {Boolean}             [description]
         */
        function isAddon(catalogData) {
            return catalogData && catalogData.originDisplayType === 'Addon' ? true : false;
        }

        /**
         * is the catalog data an expansion?
         * @param  {object} catalogData the catalog data model
         * @return {Boolean}             [description]
         */
        function isExpansion(catalogData) {
            return catalogData && catalogData.originDisplayType === 'Expansion' ? true : false;
        }

        /**
         * Is a game addon released within the last 7 days
         * @param  {Object} catalogData the catalog data model
         * @return {Boolean}
         */
        function isAddonRecentlyReleased(catalogData) {
            return (catalogData &&
                catalogData.originDisplayType === 'Addon' &&
                isInTheFuture(addDays(catalogData.releaseDate, 7))
            ) ? true: false;
        }

        /**
         * Is a game expansion released within the last 28 days
         * @param  {Object} catalogData the catalog data model
         * @return {Boolean}
         */
        function isExpansionRecentlyReleased(catalogData) {
            return (catalogData &&
                catalogData.originDisplayType === 'Expansion' &&
                isInTheFuture(addDays(catalogData.releaseDate, 28))
            ) ? true: false;
        }

        /**
         * Is an expansion awaiting download/install and granted within the last 7 days
         * @param  {Object[]}  data [clientData, entitlementData]
         * @return {Boolean}
         */
        function isAddonOrExpansionAwatingDownload(data) {
            var clientData = data[0],
                entitlementData = data[1];

            if ((clientData.downloadable || clientData.installable) &&
                entitlementData.grantDate &&
                isInTheFuture(addDays(entitlementData.grantDate, 7))) {
                return true;
            }
        }

        /**
         * Test if an addon or expansion has been installed in the last 7 days
         * @param {Object[]} data [clientData, baseGameLastPlayed]
         * @return {Boolean}
         */
        function isAddonOrExpansionRecentlyInstalled(data) {
            var clientData = data[0],
                baseGameLastPlayed = data[1];

            if( clientData.installed &&
                isValidDate(clientData.initialInstalledTimestamp) &&
                clientData.initialInstalledTimestamp > baseGameLastPlayed &&
                isInTheFuture(addDays(clientData.initialInstalledTimestamp, 7))
            ) {
                return true;
            }

            return false;
        }

        return {
            isPreloadImminent: isPreloadImminent,
            isPreloadAvailable: isPreloadAvailable,
            isUpdateAvailable: isUpdateAvailable,
            isReleaseDateImminent: isReleaseDateImminent,
            isGameExpiryImminent: isGameExpiryImminent,
            isTrialAwaitingActivation: isTrialAwaitingActivation,
            isTrialInProgress: isTrialInProgress,
            isTrialExpired: isTrialExpired,
            isPurchasable: isPurchasable,
            isFullGame: isFullGame,
            isFullGamePlusExpansion: isFullGamePlusExpansion,
            isDownloadable: isDownloadable,
            isAddonRecentlyReleased: isAddonRecentlyReleased,
            isExpansionRecentlyReleased: isExpansionRecentlyReleased,
            isAddon: isAddon,
            isExpansion: isExpansion,
            isAddonOrExpansionAwatingDownload: isAddonOrExpansionAwatingDownload,
            isAddonOrExpansionRecentlyInstalled: isAddonOrExpansionRecentlyInstalled
        };
    }

    /**
    * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
    * @param {object} SingletonRegistryFactory the factory which manages all the other factories
    */
    function GameViolatorFilterFactorySingleton(DateHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GameViolatorFilterFactory', GameViolatorFilterFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GameViolatorFilterFactory

     * @description
     *
     * GameViolatorFilterFactory
     */
    angular.module('origin-components')
        .factory('GameViolatorFilterFactory', GameViolatorFilterFactorySingleton);
})();
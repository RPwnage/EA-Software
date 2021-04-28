/**
 * @file game/gamescatalog.js
 */
(function() {
    'use strict';

    function GamesUsageFactory(AppCommFactory, ComponentsLogFactory, GamesCatalogFactory, GamesClientFactory, ObjectHelperFactory) {
        var getProperty = ObjectHelperFactory.getProperty,
            lastPlayedTimesPromise;

        function handlegetAllLastPlayedTimesError() {
            resetLastPlayedPromise();
            return Promise.resolve([]);
        }

        /**
         * Get all the last gameplay times and master title ids for the current user as a list.
         * The promise object is cached (assigned to a var) to avoid repeated calls to the endpoint.
         * The cached promise cleared when the user navigates to another route or finished playing a game --
         * there's no need to update "last played" times when the user is in a game or AFK.
         * @return {Object} the consolidated list of last played games and their times
         */
        function getAllLastPlayedTimes() {
            if (!lastPlayedTimesPromise) {
                lastPlayedTimesPromise = Origin.atom.atomGameLastPlayed().catch(handlegetAllLastPlayedTimesError);
            }
            return lastPlayedTimesPromise;
        }

        /**
         * Find the play time in the model based on the catalog info master title id
         * @param {Object} catalogInfo the offer Id to look up
         * @param {Object} allUsageInfo the usage model from atom
         * @return {Date} the last played timestamp
         */
        function findLastPlayedTime(catalogInfo, allUsageInfo) {
            if(!catalogInfo || !allUsageInfo || !_.get(catalogInfo, ['masterTitleId'])) {
                return;
            }

            var masterTitleId = _.get(catalogInfo, ['masterTitleId']),
                match = _.head(_.where(allUsageInfo, {'masterTitleId': masterTitleId}));

            if(_.has(match, 'timestamp')) {
                return new Date(_.get(match, 'timestamp'));
            }

            return;
        }

        /**
         * Get the last played date by offerId
         * @param {string} offerId the offerId to find
         * @return {Promise.<Object, Error>} resolves to an object containing play time and the offerId
         */
        function getLastPlayedTimeByOfferId(offerId) {
            return Promise.all([
                    GamesCatalogFactory.getCatalogInfo([offerId])
                        .then(getProperty(offerId)),
                    getAllLastPlayedTimes()
                ])
                .then(_.spread(findLastPlayedTime));
        }

        /**
         * Clear lastPlayedTimesPromise so new last-played data will be loaded from atom service
         */
        function resetLastPlayedPromise() {
            lastPlayedTimesPromise = null;
        }

        // Clear lastPlayedTimesPromise upon page navigation or gameplay ended (SPA doesn't get "game ended" client events)
        AppCommFactory.events.on('uiRouter:stateChangeSuccess', resetLastPlayedPromise);
        GamesClientFactory.events.on('GamesClientFactory:finishedPlaying', resetLastPlayedPromise);

        return {
            getLastPlayedTimeByOfferId: getLastPlayedTimeByOfferId,
            getAllLastPlayedTimes: getAllLastPlayedTimes
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesUsageFactorySingleton(AppCommFactory, ComponentsLogFactory, GamesCatalogFactory, GamesClientFactory, ObjectHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesUsageFactory', GamesUsageFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesUsageFactory
     * @description
     *
     * GamesUsageFactory
     */
    angular.module('origin-components')
        .factory('GamesUsageFactory', GamesUsageFactorySingleton);
}());

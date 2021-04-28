/**
 * @file store/rating-system.js
 */
(function() {
    'use strict';

    function RatingSystemFactory(UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-store-game-rating'; // <-- CQ defaults defined in that directive's ngdocs

        /**
         * Based on rating descriptor string (and optionally, rating system), get the complete localization key
         * @param {string} descriptor Value of descriptor, e.g. "Strong Language"
         * @param {string} ratingSystem Name of respective rating system, e.g. "ESRB"
         * @return {string} Constructed localization key, e.g. "esrb-fr-strong-language"
         */
        function getRatingDescriptorLocalizationKey(descriptor, ratingSystem) {
            if (descriptor && ratingSystem) {
                return _.kebabCase([ratingSystem, descriptor].join('-'));
            }
            return '';
        }

        /**
         * Provided a game rating system & descriptor, return translated version from CQ5 defaults if available
         * @param descriptor Value of descriptor, e.g. "Strong Language"
         * @param ratingSystem Name of respective rating system, e.g. "ESRB"
         * @returns {string} Translated descriptor based on user's locale, e.g. "Lenguaje fuerte"
         */
        function getTranslatedGameRating(descriptor, ratingSystem) {
            var lookupKey = getRatingDescriptorLocalizationKey(descriptor, ratingSystem);
            return lookupKey ? UtilFactory.getLocalizedStr(undefined, CONTEXT_NAMESPACE, lookupKey) : '';
        }

        /**
         * Update a given game with game rating descriptors in the current language
         * @param game Catalog data of the game
         */
        function updateGameWithLocalizedRatings(game) {
            if (!_.isEmpty(game)) {
                var longDescriptor = game.gameRatingDescriptionLong;
                var gameRating = game.gameRatingTypeValue;
                var ratingSystem = _.get(game, 'gameRatingType');

                if (ratingSystem === 'ESRB') { // Remove this ESRB check if other rating systems must be localized

                    if (longDescriptor) {
                        var newLongDescriptor = getTranslatedGameRating(longDescriptor, ratingSystem);
                        if (newLongDescriptor) {
                            game.gameRatingDescriptionLong = newLongDescriptor;
                        }
                    }
                    if (gameRating) {
                        var newRatingDescriptor = getTranslatedGameRating(gameRating, ratingSystem);
                        if (newRatingDescriptor) {
                            game.gameRatingTypeValue = newRatingDescriptor;
                        }
                    }

                    if (!_.isEmpty(game.gameRatingDesc)) {
                        _.forEach(game.gameRatingDesc, function (descriptor, index) {
                            var newDescriptor = getTranslatedGameRating(descriptor, ratingSystem);
                            if (newDescriptor) {
                                game.gameRatingDesc[index] = newDescriptor;
                            }
                        });
                    }
                }
            }
        }

        /**
         * Update a collection of games with game rating descriptors in the current language
         * @param games Collection of game models
         * @return {object} Updated collection of games
         */
        function updateGamesWithLocalizedRatings(games) {
            _.forEach(games, updateGameWithLocalizedRatings);
            return games;
        }

        return {
            getTranslatedGameRating: getTranslatedGameRating,
            updateGameWithLocalizedRatings: updateGameWithLocalizedRatings,
            updateGamesWithLocalizedRatings: updateGamesWithLocalizedRatings
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    // This needs to be a singleton for the popout/OIG windows to work properly please don't remove this singleton without proper testing of these features
    function RatingSystemFactorySingleton(UtilFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('RatingSystemFactory', RatingSystemFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.RatingSystemFactory

     * @description
     *
     * Store Rating System factory
     */
    angular.module('origin-components')
        .factory('RatingSystemFactory', RatingSystemFactorySingleton);
})();

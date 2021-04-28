/**
 * @file game/gamesocd.js
 */
(function () {
    'use strict';

    function GamesOcdFactory(ObjectHelperFactory, ComponentsLogFactory, CacheFactory) {


        /**
         * Log any errors that occur with the service
         * @param  {Object} error the error object
         * @return {Object} an empty object
         */
        function catchOcdError(error) {
            ComponentsLogFactory.error('[GAMESOCDFACTORY:catchOcdError]:', error);

            return {};
        }

        /**
         * OCD requires a unique locale format to other services in the format "en-gb.gbr"
         * @return {string} ocd's specific language code format for HTTP requests
         */
        function getDefaultLocale() {
            var urlSafeLocale = Origin.locale.locale().toLowerCase().replace('_', '-'),
                threeLetterCountryCode = Origin.locale.threeLetterCountryCode().toLowerCase();

            return [urlSafeLocale, threeLetterCountryCode].join('.');
        }

        /**
         * Sanitize the locale override string if provided
         * @param  {mixed} locale string
         * @return {string} the sanitized locale
         */
        function sanitizeLocale(locale) {
            return !_.isEmpty(locale) && _.isString(locale) ? locale : getDefaultLocale();
        }


        /**
         * retrieves the raw OCD record for a game tree path
         * @param {string} ocdPath the ocdPath attribute eg. /content/web/app/games/battlefield/battlefield-4/standard-edition
         * @param {string} locale the locale in ocd format eg. en-gb.gbr
         * @return {Promise.<Object, Error>}
         */
        function getOcdByPath(OCDPath, locale) {
            if (!OCDPath) {
                ComponentsLogFactory.error('[GAMESOCDFACTORY:getOcdByPath]: OCDPath not provided');
                return Promise.resolve({});
            }

            return Origin.games.getOcdByPath.apply(undefined, [sanitizeLocale(locale), OCDPath])
                .then(ObjectHelperFactory.getProperty('data'))
                .catch(catchOcdError);
        }

        return {
            getOcdByPath: CacheFactory.decorate(getOcdByPath)
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesOcdFactorySingleton(ObjectHelperFactory, ComponentsLogFactory, CacheFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesOcdFactory', GamesOcdFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesOcdFactory

     * @description
     *
     * This adapter adds OCD functionality to the games data factory. Please access the methods through GamesDataFactory.
     */
    angular.module('origin-components')
        .factory('GamesOcdFactory', GamesOcdFactorySingleton);
}());
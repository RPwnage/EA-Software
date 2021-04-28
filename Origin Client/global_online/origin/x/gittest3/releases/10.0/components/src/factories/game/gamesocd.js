/**
 * @file game/gamesocd.js
 */
(function() {
    'use strict';

    function GamesOcdFactory(ComponentsLogFactory) {
        var myEvents = new Origin.utils.Communicator(),
            ocdByOffer = {},
            ocdByPath = {},
            DEFAULT_EXPIRATION_MS = 300; //5 seconds

        function getResponseExpiration(responseHeader) {
            var maxage,
                expiration;

            if (responseHeader) {
                maxage = /max-age=\d+/i.exec(responseHeader);
                expiration = /\d+/.exec(maxage)[0];
                expiration = parseInt(expiration);
            } else {
                //just set the expiration to default 5 min out
                expiration = DEFAULT_EXPIRATION_MS;
            }
            return expiration;
        }

        function isOcdOfferCached(offerId, locale) {
            var cacheExpire = 0;
            if (ocdByOffer[offerId] && ocdByOffer[offerId].locale === locale) {
                return ocdByOffer[offerId].expiration;
            }
            return cacheExpire;
        }

        function handleOcdByOfferResponse(offerId, locale) {
            return function(response) {
                var expiration;

                ocdByOffer[offerId] = {
                    offerId: offerId,
                    locale: locale,
                    data: response.data
                };
                expiration = getResponseExpiration(response.headers);
                expiration = Date.now() + expiration;
                ocdByOffer[offerId].expiration = expiration;
                return ocdByOffer[offerId].data;
            };
        }

        function catchOcdByOfferIdError(error) {
            ComponentsLogFactory.error('[GAMESOCDFACTORY:catchOcdByOfferIdError]:', error.status, ', ', error.message, ',', error.stack);
            return {};
        }

        function getOcdByOfferId(offerId, locale) {
            var promise = null,
                setLocale,
                cacheExpire;

            if (!offerId) {
                ComponentsLogFactory.error('[GAMESOCDFACTORY:getOcdByOfferId]: offerId is null');
                promise = Promise.resolve({});
            } else {
                setLocale = locale ? locale : Origin.locale.locale();
                cacheExpire = isOcdOfferCached(offerId, locale);

                if (Date.now() < cacheExpire) {
                    promise = Promise.resolve(ocdByOffer[offerId].data);
                } else {
                    promise = Origin.games.getOcdByOfferId(offerId, setLocale)
                        .then(handleOcdByOfferResponse(offerId, setLocale))
                        .catch(catchOcdByOfferIdError);
                }
            }
            return promise;
        }

        function getPathKey(franchise, game, edition) {
            var key = franchise;
            if (game) {
                key += '|' + game;
                if (edition) {
                    key += '|' + edition;
                }
            }
            return key;
        }

        function isOcdPathCached(pathKey, locale) {
            var cacheExpire = 0;
            if (ocdByPath[pathKey] && ocdByPath[pathKey].locale === locale) {
                return ocdByPath[pathKey].expiration;
            }
            return cacheExpire;
        }

        function handleOcdByPathResponse(locale, franchise, game, edition) {
            return function(response) {
                var expiration,
                    key = getPathKey(franchise, game, edition);

                ocdByPath[key] = {
                    locale: locale,
                    data: response.data
                };
                expiration = getResponseExpiration(response.headers);
                expiration = Date.now() + expiration;
                ocdByPath[key].expiration = expiration;
                return ocdByPath[key].data;
            };
        }


        function catchOcdByPathError(error) {
            ComponentsLogFactory.error('[GAMESOCDFACTORY:catchOcdByPathError]:', error.status, ', ', error.message, ',', error.stack);
            return {};
        }

        function getOcdByPath(locale, franchise, game, edition) {
            var promise = null,
                key,
                cacheExpire;

            if (!franchise) {
                ComponentsLogFactory.error('[GAMESOCDFACTORY:getOcdByPath]: franchis is null');
                promise = Promise.resolve({});
            } else {
                key = getPathKey(franchise, game, edition);
                cacheExpire = isOcdPathCached(key, locale);

                if (Date.now() < cacheExpire) {
                    promise = Promise.resolve(ocdByPath[key].data);
                } else {
                    promise = Origin.games.getOcdByPath(locale, franchise, game, edition)
                        .then(handleOcdByPathResponse(locale, franchise, game, edition))
                        .catch(catchOcdByPathError);
                }
            }
            return promise;
        }

        return {

            events: myEvents,

            /*
             * retrieves the OCD record for a given offerId
             * @param {string} offerId
             * @return {object} ocd response
             */
            getOcdByOfferId: getOcdByOfferId,

            /*
             * retrieves the OCD record for a game tree path
             * @param {string} franchise
             * @param {string} game - optional
             * @param {string} edition - optional
             * @return {object} ocd response
             */
            getOcdByPath: getOcdByPath,

        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesOcdFactorySingleton(ComponentsLogFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesOcdFactory', GamesOcdFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesOcdFactory

     * @description
     *
     * GamesOcdFactory
     */
    angular.module('origin-components')
        .factory('GamesOcdFactory', GamesOcdFactorySingleton);
}());
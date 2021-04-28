/**
 * @file game/gamesocd.js
 */
(function() {
    'use strict';

    function GamesOcdFactory(CQTargetingFactory, ObjectHelperFactory, ComponentsLogFactory) {
        var myEvents = new Origin.utils.Communicator(),
            ocdByOffer = {},
            ocdByPath = {},
            DEFAULT_EXPIRATION_MS = 300; //5 seconds

        function getResponseExpiration(responseHeader) {
            var maxage,
                expiration = DEFAULT_EXPIRATION_MS;

            if (responseHeader) {
                maxage = /max-age=\d+/i.exec(responseHeader);
                if (maxage) {
                    expiration = /\d+/.exec(maxage)[0];
                    expiration = parseInt(expiration);
                } else {
                    ComponentsLogFactory.error('[GamesOcdFactory][getResponseExpiration] invalid/missing max age header');
                }
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

        /**
         * helper function to see a if a requested directive name matches a directive name exactly or
         * if it matches the name and an '_'. Example: origin-game-tile matches origin-game tile or
         * it matches origin-game-tile with origin-game-tile_01 -- the suffixes are used between different variations
         * @param  {string} directiveName          a directive name in the json object
         * @param  {string} requestedDirectiveName the directive name we are looking for
         * @return {boolean}                       true if we find a match/false if we don't
         */
        function directiveNameMatches(directiveName, requestedDirectiveName) {
            return (directiveName.startsWith(requestedDirectiveName + '_') || (directiveName === requestedDirectiveName)) ? true : false;
        }

        /**
         * filter a json object(from OCD) and apply targeting rules
         * @param  {object} directives             the directives from an ocd response
         * @param  {string} requestedDirectiveName the directive we are looking for
         * @return {object}                        the requested directive to show
         */
        function removeUnMatchedCQTargets(directives, requestedDirectiveName) {
            return function() {
                var filteredDirective = directives[requestedDirectiveName];

                ObjectHelperFactory.forEach(function(directiveObject, directiveName) {
                    if (directiveNameMatches(directiveName, requestedDirectiveName) &&
                        CQTargetingFactory.targetingItemMatches(directiveObject['cq-targeting'])) {

                        //set the directive to return and delete the targeting object since its no longer needed
                        filteredDirective = directiveObject;
                        delete filteredDirective['cq-targeting'];
                    }
                }, directives);

                return filteredDirective || {};
            };
        }

        /**
         * given a object of components, remove the unused ones when the targeting data feed is ready
         * @param  {object} directives             the directives from an ocd response
         * @param  {string} requestedDirectiveName the directive we are looking for
         * @return {promise}                       resolved with the directive we are looking for or an empty object
         */
        function applyCQTargeting(directives, requestedDirectiveName) {
            return CQTargetingFactory.waitForDataSourcesReady()
                .then(removeUnMatchedCQTargets(directives, requestedDirectiveName));
        }

        function parseOutComponents(directiveName) {
            return function(response) {
                var components = Origin.utils.getProperty(response, ['gamehub', 'components']);
                return components ? applyCQTargeting(components, directiveName) : Promise.resolve({});

            };
        }

        function catchOcdDirectiveByOfferIdError(error) {
            ComponentsLogFactory.error('[GAMESOCDFACTORY:catchOcdDirectiveByOfferError]:', error.status, ', ', error.message, ',', error.stack);
            return {};
        }

        function getOcdDirectiveByOfferId(directiveName, offerId, locale) {
            return getOcdByOfferId(offerId, locale)
                .then(parseOutComponents(directiveName))
                .catch(catchOcdDirectiveByOfferIdError);
        }

        function catchOcdDirectiveByPathError(error) {
            ComponentsLogFactory.error('[GAMESOCDFACTORY:catchOcdDirectiveByPathError]:', error.status, ', ', error.message, ',', error.stack);
            return {};
        }

        function getOcdDirectiveByPath(directiveName, locale, franchise, game, edition) {
            return getOcdByPath(locale, franchise, game, edition)
                .then(parseOutComponents(directiveName))
                .catch(catchOcdDirectiveByPathError);
        }

        return {

            events: myEvents,

            /*
             * retrieves the raw OCD record for a given offerId
             * @param {string} offerId
             * @return {object} ocd response
             */
            getOcdByOfferId: getOcdByOfferId,

            /*
             * retrieves the raw OCD record for a game tree path
             * @param {string} franchise
             * @param {string} game - optional
             * @param {string} edition - optional
             * @return {object} ocd response
             */
            getOcdByPath: getOcdByPath,

            /*
             * retrieves and returns the OCD offer info for the specified directive given an offerId
             * @param {string} directive name
             * @param {string} offerId
             * @return {object} ocd offer info for that directive
             */
            getOcdDirectiveByOfferId: getOcdDirectiveByOfferId,

            /*
             * retrieves and returns the OCD offer info for the specified directive given a path
             * @param {string} directive name
             * @param {string} franchise
             * @param {string} game - optional
             * @param {string} edition - optional
             * @return {object} ocd info for that directive given the path
             */
            getOcdDirectiveByPath: getOcdDirectiveByPath
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesOcdFactorySingleton(CQTargetingFactory, ObjectHelperFactory, ComponentsLogFactory, SingletonRegistryFactory) {
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
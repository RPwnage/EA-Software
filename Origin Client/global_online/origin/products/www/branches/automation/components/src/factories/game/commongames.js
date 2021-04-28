/**
 * @file game/commongames.js
 */
(function() {
    'use strict';

    function CommonGamesFactory(RosterDataFactory, GamesDataFactory, ComponentsLogFactory) {
        var SINGLE_PLAYER_ID = 'SinglePlayerId',
            commonGamesCache = {};

        /**
         * wraps an object or primitive into an Array
         * @param  {object} data the data to wrap
         * @return {object[]} an array of items if its not already an array
         */
        function toArray(data) {
            return Array.isArray(data) ? data : [data];
        }

        /**
         * gets a set of nucleusIds objects and builds arrays of strings to pass into the jssdk request
         * if we have 30 nucleus ids we need to group them in sets of 5
         * @param  {string[]} friendsIdArray nucleus ids objects from RosterData
         * @return {object} list of cached nucleus ids plus nucleus ids to retrieve from server, grouped in sets of 5
         */
        function buildNucleusIdsStrings(friendsIdArray) {
            var MAX_IDS_IN_REQUEST = 5,
                incomingNucleusIds = Object.keys(friendsIdArray),
                cachedNucleusIds = Object.keys(commonGamesCache),
                requestNucleusIds = _.difference(incomingNucleusIds, cachedNucleusIds),
                nucleusIdsSets = [];


            while (requestNucleusIds.length) {
                nucleusIdsSets.push(requestNucleusIds.splice(0, MAX_IDS_IN_REQUEST).join(','));
            }

            return {'cached' : cachedNucleusIds,  'request' : nucleusIdsSets};
        }

        /**
         * logs out any server error and treats it like an empty response
         * @param  {Error} error the error object
         */
        function handleRetrieveFromServerError(error) {
            ComponentsLogFactory.error('[CommonGamesFactory:handleRetrieveFromServerError]', error);
        }


        /**
         * get the cached list of common games given a friend nucleus Id
         * @param  {string} the nucleus Id of the friend 
         */
        function retrieveCachedCommonGames(nucleusId) {
            return new Promise(function(resolve) {
                resolve(commonGamesCache[nucleusId]);
            });
        }

        /**
         * builds a list of promises to resolve friends common games 
         * @param  {object} list of cached nucleus ids plus nucleus ids to retrieve from server, grouped in sets of 5
         * @return {promise}  promise resolves with the jssdk or cached results
         */
        function buildCommonGamesPromises(nucleusIds) {
            var promiseArray = [],
                requestNucleusIds = nucleusIds.request,
                cachedNucleusIds = nucleusIds.cached;

            requestNucleusIds.forEach(function(requestIds) {
                promiseArray.push(Origin.atom.atomCommonGames(requestIds).catch(handleRetrieveFromServerError));
            });

            cachedNucleusIds.forEach(function(cachedNucleusId) {
                promiseArray.push(retrieveCachedCommonGames(cachedNucleusId));
            });

            return Promise.all(promiseArray);
        }

        /**
         * gets owned games for other users, if we already have it just return the cache version
         * if we are in progress, return the promise of the request in progress
         * otherwise make a new request
         * @return {promise} promise resolves with an object with master title as keys and nucleus ids as values
         */
        function getOwnedGamesForOtherUsers() {
            return RosterDataFactory.getRoster('FRIENDS')
                .then(buildNucleusIdsStrings)
                .then(buildCommonGamesPromises)
                .then(concatResults)
                .then(cacheResults)
                .then(transformResults);
        }

        /**
         * assigns a user ids to a master title key in a map
         */
        function getGameToMasterTitleIdMap(game, userId) {
            var gameToMasterTitleIdMap = {};

            //take the list of games for a user and assign it to a new map with mastertitle id and multiplayer id as the keys
            game.forEach(function(gameItem) {
                var masterTitleId = gameItem.masterTitleId,
                    multiPlayerId = gameItem.multiPlayerId || SINGLE_PLAYER_ID;

                gameToMasterTitleIdMap[masterTitleId] = gameToMasterTitleIdMap[masterTitleId] || {};
                gameToMasterTitleIdMap[masterTitleId][multiPlayerId] = gameToMasterTitleIdMap[masterTitleId][multiPlayerId] || {};
                gameToMasterTitleIdMap[masterTitleId][multiPlayerId][userId] = true;
            });

            return gameToMasterTitleIdMap;
        }

        /**
         * raw response is arrays of mastertitle ids indexed by nucleus ids, this function flips that so we index by master titleids to
         * array of nucleus ids
         * @param  {objects[]} results the raw results from the jssdk
         * @return {objects[]} the transformed results
         **/
        function transformResults(results) {
            var transformedResults = {};
            results.forEach(function(item) {
                if (item.games) {
                    item.games.game = toArray(item.games.game);
                    //mix the results for an individual user to our new transformed results object (indexed by mastertitle id)
                    Origin.utils.mix(transformedResults, getGameToMasterTitleIdMap(item.games.game, item.userId));
                }
            });

            return transformedResults;
        }


        function cacheResults(results) {
            // Cache the results
            results.forEach(function(commonGamesPerUserId) {
                commonGamesCache[commonGamesPerUserId.userId] = {'users' : {'user' : [commonGamesPerUserId]}};
            });
            return results;
        }

        /**
         * concatenates the result of the multiple calls we made to get other users master title ids
         * @param  {object[]} results of the promise all
         * @return {object[]} the results but in one array
         */
        function concatResults(results) {
            var fullResults = [];

            //lets loop through the results of the multiple calls we made and concatenate them together
            results.forEach(function(nucleusIdSet) {
                if (!!nucleusIdSet && !! nucleusIdSet.users) {
                    //because this object is converted to JSON (original response is XML), users.user can be an array or a single item
                    nucleusIdSet.users.user = toArray(nucleusIdSet.users.user);
                    fullResults = fullResults.concat(nucleusIdSet.users.user);                    
                }
            });

            return fullResults;
        }

        /**
         * builds a list of users who have compatible offers with the offer Id
         * @param  {string} offerId the offer id
         * @param  {string} platform the platform we want to check against
         * @return {function} a function that returns an array of nucleus ids
         */
        function getListOfCompatibleOffers(offerId, platform) {
            return function(masterTitleToUsersMap) {
                //use current if none is passed in
                if (!platform) {
                    platform = Origin.utils.os();
                }
                return GamesDataFactory.getCatalogInfo([offerId])
                    .then(function(catalogInfo) {
                        //we need the catalog to get the multiplayer and master title for the requested offer
                        var multiPlayerId = (Origin.utils.getProperty(catalogInfo, [offerId, 'platforms', platform, 'multiPlayerId']) || SINGLE_PLAYER_ID),
                            masterTitleId = catalogInfo[offerId].masterTitleId,
                            //then index into the map by mastertitle id and multiplayer
                            foundUsers = masterTitleId && masterTitleToUsersMap[masterTitleId] && masterTitleToUsersMap[masterTitleId][multiPlayerId];

                        //if we found users return the users as an array of nucleus ids
                        return foundUsers ? Object.keys(masterTitleToUsersMap[masterTitleId][multiPlayerId]) : [];

                    });

            };
        }

        /**
         * get the users who own a particular offer
         * @param  {string} offerId the offer id
         * @return {promise}        promise resolves with a array of nucleus ids
         */
        function getUsersWhoOwn(offerId, platform) {
            return getOwnedGamesForOtherUsers()
                .then(getListOfCompatibleOffers(offerId, platform));
        }

        /**
         * get the list of games in common
         * @param  {string[]} nuclsusIds to check for common games
         * @return {promise}        promise resolves with a array of common games
         */
        function getCommonGames(nucleusIds) {
            nucleusIds = Object.assign({}, nucleusIds);
            nucleusIds = _.invert(nucleusIds);

            return buildCommonGamesPromises(buildNucleusIdsStrings(nucleusIds))
                .then(concatResults)
                .then(transformResults);
        }

        return {
            getUsersWhoOwn: getUsersWhoOwn,
            getCommonGames: getCommonGames
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function CommonGamesFactorySingleton(RosterDataFactory, GamesDataFactory, ComponentsLogFactory, AuthFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('CommonGamesFactory', CommonGamesFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.CommonGamesFactory
     
     * @description
     *
     * CommonGamesFactory
     */
    angular.module('origin-components')
        .factory('CommonGamesFactory', CommonGamesFactorySingleton);
}());
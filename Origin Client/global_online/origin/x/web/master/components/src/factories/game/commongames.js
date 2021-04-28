/**
 * @file game/violator.js
 */
(function() {
    'use strict';

    function CommonGamesFactory(RosterDataFactory, GamesDataFactory, ComponentsLogFactory) {
        var otherUsersGames = {},
            retrieveCommonGamesPromise = null,
            commonGamesDataRetrieved = false,
            MAX_IDS_IN_REQUEST = 5;


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
         * @param  {string[]} nucleusIdsObjects nucleus ids objects from RosterData
         * @return {string[]} n number of comma deliminated strings
         */
        function buildNucleusIdsStrings(nucleusIdsObjects) {
            var incomingNucleusIds = Object.keys(nucleusIdsObjects),
                nucleusIdsSets = [];

            while (incomingNucleusIds.length) {
                nucleusIdsSets.push(incomingNucleusIds.splice(0, MAX_IDS_IN_REQUEST).join(','));
            }

            return nucleusIdsSets;
        }

        /**
         * logs out any server error and treats it like an empty response
         * @param  {Error} error the error object
         * @return {object} empty object
         */
        function handleRetrieveFromServerError(error) {
            ComponentsLogFactory.error(error);
            return {};
        }

        /**
         * clears the in progress promise
         */
        function clearInRetrievalInProgress(results) {
            retrieveCommonGamesPromise = null;
            commonGamesDataRetrieved = true;
            return results;
        }
        /**
         * gets the other users game information
         * @param  {string[]} friendsIdArray array of comma delminated strings, since the jssdk call can only take 5 nucleus useres at a time
         * @return {promise}  promise resolves with the jssdk results
         */
        function retrieveGamesOwnedForOtherUsersFromServer(friendsIdArray) {
            var promiseArray = [];
            otherUsersGames = {};
            friendsIdArray.forEach(function(friendIds) {
                promiseArray.push(Origin.atom.atomCommonGames(friendIds).catch(handleRetrieveFromServerError));
            });

            return promiseArray.length ? Promise.all(promiseArray) : Promise.resolve([]);
        }

        /**
         * gets owned games for other users, if we already have it just return the cache version
         * if we are in progress, return the promise of the request in progress
         * otherwise make a new request
         * @return {promise} promise resolves with an object with master title as keys and nucleus ids as values
         */
        function getOwnedGamesForOtherUsers() {
            if (commonGamesDataRetrieved) {
                return Promise.resolve(otherUsersGames);
            }

            if (retrieveCommonGamesPromise) {
                return retrieveCommonGamesPromise;
            }

            return RosterDataFactory.getRoster('FRIENDS')
                .then(buildNucleusIdsStrings)
                .then(retrieveGamesOwnedForOtherUsersFromServer)
                .then(concatResults)
                .then(transformResults)
                .then(storeResults)
                .then(clearInRetrievalInProgress);
        }

        /**
         * assigns a user ids to a master title key in a map
         */
        function getGameToMasterTitleIdMap(game, userId) {
            var gameToMasterTitleIdMap = {};

            //take the list of games for a user and assign it to a new map with mastertitle id and multiplayer id as the keys
            game.forEach(function(gameItem) {
                var masterTitleId = gameItem.masterTitleId,
                    multiPlayerId = gameItem.multiPlayerId;

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

        /**
         * cache the transformed objects
         * @param  {object} transformedObjects cache the transformed objects
         * @return {object} just pass through the transformed objects
         */
        function storeResults(transformedObjects) {
            Origin.utils.mix(otherUsersGames, transformedObjects);
            return transformedObjects;
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
                //because this object is converted to JSON (original response is XML), users.user can be an array or a single item
                nucleusIdSet.users.user = toArray(nucleusIdSet.users.user);
                fullResults = fullResults.concat(nucleusIdSet.users.user);
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
                        var multiPlayerId = Origin.utils.getProperty(catalogInfo, [offerId, 'platforms', platform, 'multiPlayerId']),
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

        return {
            getUsersWhoOwn: getUsersWhoOwn
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function CommonGamesFactorySingleton(RosterDataFactory, GamesDataFactory, ComponentsLogFactory, SingletonRegistryFactory) {
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
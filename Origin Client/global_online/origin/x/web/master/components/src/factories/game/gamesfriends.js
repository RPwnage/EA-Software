/**
 * @file game/gamesocd.js
 */

(function() {
    'use strict';

    function GamesFriendsFactory(RosterDataFactory, CommonGamesFactory, GamesDataFactory) {

        /*jshint latedef: nofunc */

        var friendsListTextMap = {
            friendsPlaying: {
                popoutTitleId: 'popoutfriendsplayinggame',
                descriptionTextId: 'friendsplayingdescription',
                showBeacon: true
            },
            friendsPlayed: {
                popoutTitleId: 'popoutfriendsplayedgame',
                descriptionTextId: 'friendsplayeddescription',
                showBeacon: true
            },
            friendsOwned: {
                popoutTitleId: 'popoutfriendsownedgame',
                descriptionTextId: 'friendsowneddescription',
                showBeacon: false
            }
        };

        var requestFriendTypeMap = {
            friendsPlayedOrPlaying: getFriendsPlayedOrPlaying,
            friendsOwned: getCommonGames
        };

        /**
         * extract the catalogInfo from the array
         * @param  {string} offerId the offer id
         * @return {object}         a catalog info object
         */
        function extractSingleResponse(offerId) {
            return function(results) {
                return results[offerId];
            };
        }

        /**
         * returns a object
         * @param  {string} type     the type of friends list this is, (FRIENDS_PLAYING, FRIENDS_PLAYED, FRIENDS_OWNED)
         * @param  {string[]} list   array of nucleus ids
         * @param  {string} gameName the name of the associated game
         * @return {object}          object with the three params
         */
        function buildReturnObject(type, list, gameName) {
            var returnObject = {
                list: list,
                gameName: gameName
            };
            Origin.utils.mix(returnObject, friendsListTextMap[type]);

            return returnObject;
        }

        /**
         * returns a promise that will resolve with the friends played, or friends playing list
         * @param  {string} offerId the offer id
         * @param  {string} gameName the name of the associated game
         * @return {promise} resolves with a friends list
         */
        function getFriendsPlayedOrPlaying(offerId, gameName) {
            var friendsPlaying = RosterDataFactory.friendsWhoArePlaying(offerId),
                friendsPlayed = RosterDataFactory.getFriendsWhoPlayed(offerId);

            return Promise.resolve(friendsPlaying.length ? buildReturnObject('friendsPlaying', friendsPlaying, gameName) : buildReturnObject('friendsPlayed', friendsPlayed, gameName));
        }

        /**
         * returns a promise that will resolve with the friends owned list
         * @param  {string} offerId the offer id
         * @param  {string} gameName the name of the associated game
         * @return {promise} resolves with a friends list
         */
        function getCommonGames(offerId, gameName) {
            return CommonGamesFactory
                .getUsersWhoOwn(offerId)
                .then(function(list) {
                    return buildReturnObject('friendsOwned', list, gameName);
                });
        }

        /**
         * returns a promise that will resolve with the friends owned list, the friends played, or friends playing list
         * @param  {string} offerId the offer id
         * @return {promise} resolves with a friends list
         */
        function getFriends(offerId, requestedFriendType) {
            return function(catalogInfo) {
                //if we want a specific kind of list (like in a programmable tile)
                if (requestedFriendType) {
                    return requestFriendTypeMap[requestedFriendType](offerId, catalogInfo.i18n.displayName);
                }
                return GamesDataFactory.hasPreload(offerId) ? getCommonGames(offerId, catalogInfo.i18n.displayName) : getFriendsPlayedOrPlaying(offerId, catalogInfo.i18n.displayName);

            };
        }

        /**
         * returns a promise that will resolve with the friends owned list, the friends played, or friends playing list
         * @param  {string} offerId the offer id
         * @return {promise} resolves with a friends list
         */
        function getFriendsListByOfferId(offerId, requestedFriendType) {
            return GamesDataFactory.getCatalogInfo([offerId])
                .then(extractSingleResponse(offerId))
                .then(getFriends(offerId, requestedFriendType));
        }

        return {
            /**
             * gets a list of friends depending on the game state
             * @param  {string} offerId the offer id
             * @return {promise} resolves with a friends list
             * @method
             */
            getFriendsListByOfferId: getFriendsListByOfferId
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesFriendsFactorySingleton(RosterDataFactory, CommonGamesFactory, GamesDataFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesFriendsFactory', GamesFriendsFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesFriendsFactory
     
     * @description
     *
     * GamesFriendsFactory
     */
    angular.module('origin-components')
        .factory('GamesFriendsFactory', GamesFriendsFactorySingleton);
}());
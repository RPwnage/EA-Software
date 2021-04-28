/**
 * @file game/friendsplaying/text.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-friendsplaying-text';

    function OriginGameFriendsplayingTextCtrl($scope, GamesFriendsFactory, RosterDataFactory, ComponentsLogFactory, UtilFactory) {
        var self = this;

        /**
         * populate the friends info scope property
         * @param  {object} friendsInfo object containing the text ids, game names, and friends list
         * @return {void}
         */
        function updateFriendsList(friendsInfo) {
            $scope.friendsInfo = friendsInfo;
            $scope.friendDescription = UtilFactory.getLocalizedStr($scope.friendsText, CONTEXT_NAMESPACE, friendsInfo.descriptionTextId, {
                '%friends%': friendsInfo.list.length
            }, friendsInfo.list.length);
            $scope.$digest();
        }

        /**
         * log out any errors
         * @param  {Error} error error object
         * @return {void}
         **/
        function handleGetListOfFriendsError(error) {
            ComponentsLogFactory.error('[origin-game-friendsplaying-text]', error.message);
        }

        /**
         * grab the friends list from the factory
         * @return {object} object containing friends info
         */
        function populateFriendsList() {
            GamesFriendsFactory.getFriendsListByOfferId($scope.offerId, $scope.requestedFriendType)
                .then(updateFriendsList)
                .catch(handleGetListOfFriendsError);
        }

        function onDestroy() {
            RosterDataFactory.events.off('socialFriendsPlayingListUpdated:' + $scope.offerId, populateFriendsList, self);
        }

        /**
         * Clean up
         * @method onDestroy
         * @return {void}
         */
        if ($scope.offerId) {
            RosterDataFactory.events.on('socialFriendsPlayingListUpdated:' + $scope.offerId, populateFriendsList, self);
            $scope.$on('$destroy', onDestroy);
            populateFriendsList();
        }


    }

    function originGameFriendsplayingText(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                text: '@',
                friendsText: '@friendstext',
                requestedFriendType: '@requestedfriendtype'
            },
            controller: 'OriginGameFriendsplayingTextCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/friendsplaying/views/text.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameFriendsplayingText
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer ID
     * @param {LocalizedText} text text prepended to the friends text
     * @param {LocalizedText} friendstext text describing how many friends are playing
     * @description
     *
     * friends playing game text
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-friendsplaying-text
     *             offerid="OFB-EAST:109552154" text="Now Available"
     *             friendsplayingdescription="You gots friends playing." friendsplayeddescription="Your friend played.">
     *         </origin-game-friendsplaying-text>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGameFriendsplayingTextCtrl', OriginGameFriendsplayingTextCtrl)
        .directive('originGameFriendsplayingText', originGameFriendsplayingText);
}());
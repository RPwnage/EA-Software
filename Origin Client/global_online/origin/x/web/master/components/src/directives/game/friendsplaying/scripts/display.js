/**
 * @file game/friendsplaying/display.js
 */
(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-game-friends-display';

    /**
     * Friends Playing Game Ctrl
     * @controller originGameFriendsplayingDisplayCtrl
     */
    function OriginGameFriendsplayingDisplayCtrl($scope, GamesFriendsFactory, RosterDataFactory, ComponentsLogFactory, UtilFactory) {

        var self = this;



        /**
         * populate the friends info scope property
         * @param  {object} friendsInfo object containing the text ids, game names, and friends list
         * @return {void}
         */
        function updateFriendsList(friendsInfo) {
            $scope.friendsInfo = friendsInfo;
            $scope.popoutTitleStr = UtilFactory.getLocalizedStr($scope.popoutTitle, CONTEXT_NAMESPACE, friendsInfo.popoutTitleId, {
                '%game%': friendsInfo.gameName
            });
            $scope.$digest();
        }
        /**
         * log out any errors
         * @param  {Error} error error object
         * @return {void}
         **/
        function handleGetListOfFriendsError(error) {
            ComponentsLogFactory.error('[origin-game-friends-display]', error.message);
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

        /**
         * Clean up
         * @method onDestroy
         * @return {void}
         */
        function onDestroy() {
            RosterDataFactory.events.off('socialFriendsPlayingListUpdated:' + $scope.offerId, populateFriendsList, self);
        }

        if ($scope.offerId) {
            RosterDataFactory.events.on('socialFriendsPlayingListUpdated:' + $scope.offerId, populateFriendsList, self);
            $scope.$on('$destroy', onDestroy);

            populateFriendsList();
        }
    }

    /**
     * Friends Playing Game directive
     * @directive originGameFriendsplayingDisplay
     */
    function originGameFriendsplayingDisplay(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                popoutTitle: '@popouttitle',
                requestedFriendType: '@requestedfriendtype'
            },
            controller: 'OriginGameFriendsplayingDisplayCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/friendsplaying/views/display.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameFriendsplayingDisplay
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer id
     * @param {LocalizedText} popouttitle string used in the title
     *
     * friends playing display
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-friendsplaying-display offerid="OFB-EAST:109552154" userid="1000080156081"></origin-game-friendsplaying-display>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGameFriendsplayingDisplayCtrl', OriginGameFriendsplayingDisplayCtrl)
        .directive('originGameFriendsplayingDisplay', originGameFriendsplayingDisplay);
}());
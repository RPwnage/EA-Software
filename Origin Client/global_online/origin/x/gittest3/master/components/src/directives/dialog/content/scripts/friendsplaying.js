/**
 * @file dialog/content/friendsplaying.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-friendsplaying';

    function OriginDialogContentFriendsplayingCtrl($scope, RosterDataFactory, GamesDataFactory, UtilFactory, ComponentsLogFactory) {

        /**
         * Update your list of friends and then display them
         * @param {Array} list - your list of friends
         * @return {void}
         */
        function updateFriends(list) {
            $scope.friends = list;
            $scope.$digest();
        }

        /**
         * Store the product info
         * @param {Object} data - the data to store
         * @return {void}
         */
        function storeProductInfo(data) {
            $scope.friendsPlayingGame = UtilFactory.getLocalizedStr($scope.friendsPlayingGameStr, CONTEXT_NAMESPACE, 'friendsplayinggame', {
                '%game%': data[$scope.offerId].displayName
            });
            $scope.$digest();
        }

        /**
        * Clean up after yoself
        * @method onDestroy
        * @return {void}
        */
        function onDestroy() {
            RosterDataFactory.events.off('socialFriendsPlayingListUpdated:' + $scope.offerId, updateFriends);
        }

        /**
         * Get the list of friends and then display them. Subscribe to any changes
         * @return {void}
         */
        this.initFriendsPlaying = function() {
            $scope.friends = RosterDataFactory.friendsWhoArePlaying($scope.offerId);
            RosterDataFactory.events.on('socialFriendsPlayingListUpdated:' + $scope.offerId, updateFriends);
            GamesDataFactory.getCatalogInfo([$scope.offerId])
                .then(storeProductInfo)
                .catch(function(error) {
                    ComponentsLogFactory.error('[origin-dialog-content-friendsplaying GamesDataFactory.getCatalogInfo] error', error.stack);
                });
        };

        // listen to events
        $scope.$on('$destroy', onDestroy);

    }

    function originDialogContentFriendsplaying(ComponentsConfigFactory) {

        function originDialogContentFriendsplayingLink(scope, elem, attrs, ctrl) {
            ctrl.initFriendsPlaying();
        }

        return {
            restrict: 'E',
            scope: {
                friendsPlayingGameStr: '@friendsplayinggame',
                offerId: '@offerid'
            },
            controller: 'OriginDialogContentFriendsplayingCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/friendsplaying.html'),
            link: originDialogContentFriendsplayingLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentFriendsplaying
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} friendsplayinggame messaging for friends playing a game
     * @param {string} offerid the offer id of the game
     * @description
     *
     * friends played dialog
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-friendsplaying
     *             friendsplayinggame="Friends Playing %game%"
     *             offerid="OFB-EAST:52735">
     *         </origin-dialog-content-friendsplaying>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginDialogContentFriendsplayingCtrl', OriginDialogContentFriendsplayingCtrl)
        .directive('originDialogContentFriendsplaying', originDialogContentFriendsplaying);

}());
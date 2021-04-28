/**
 * @file dialog/content/friendsplayed.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-friendsplayed';

    function OriginDialogContentFriendsplayedCtrl($scope, RosterDataFactory, GamesDataFactory, UtilFactory, ComponentsLogFactory) {
        /**
         * Store the product info
         * @param {Object} data - the data to store
         * @return {void}
         */
        function storeProductInfo(data) {
            $scope.friendsPlayingGame = UtilFactory.getLocalizedStr($scope.friendsPlayedGameStr, CONTEXT_NAMESPACE, 'friendsplayedgame', {
                '%game%': data[$scope.offerId].displayName
            });
            $scope.$digest();
        }

        /**
         * Get the list of friends and then display them. Subscribe to any changes
         * @return {void}
         */
        this.initFriendsPlaying = function() {
            $scope.friends = RosterDataFactory.getFriendsWhoPlayed($scope.offerId);
            GamesDataFactory.getCatalogInfo([$scope.offerId])
                .then(storeProductInfo)
                .catch(function(error) {
                    ComponentsLogFactory.error('[origin-dialog-content-friendsplayed GamesDataFactory.getCatalogInfo] error', error.stack);
                });
        };

    }

    function originDialogContentFriendsplayed(ComponentsConfigFactory) {

        function originDialogContentFriendsplayedLink(scope, elem, attrs, ctrl) {
            ctrl.initFriendsPlaying();
        }

        return {
            restrict: 'E',
            scope: {
                friendsPlayedGameStr: '@friendsplayedgame',
                offerId: '@offerid'
            },
            controller: 'OriginDialogContentFriendsplayedCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/friendsplayed.html'),
            link: originDialogContentFriendsplayedLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentFriendsplayed
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} friendsplayedgame messaging for friends that played
     * @param {string} offerid the offer id of the game
     * @description
     *
     * friends played dialog
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-friendsplayed
     *            friendsplayinggame="Friends That Played %game%"
     *            offerid="OFB-EAST:39130"
     *         ></origin-dialog-content-friendsplayed>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginDialogContentFriendsplayedCtrl', OriginDialogContentFriendsplayedCtrl)
        .directive('originDialogContentFriendsplayed', originDialogContentFriendsplayed);

}());
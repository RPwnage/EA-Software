/**
 * @file game/icons.js
 */
(function() {

    'use strict';

    function OriginGameIconsCtrl($scope, RosterDataFactory, ObserverFactory, GamesDataFactory) {
        if ($scope.offerId) {

            // Create observer of the roster data friends playing observable, and attach to $scope.friendsPlaying
            RosterDataFactory.getFriendsPlayingObservable($scope.offerId)
                .then(function(observable) {
                    ObserverFactory.create(observable)
                        .getProperty('list')
                        .attachToScope($scope, 'friendsPlaying');
                });

            // get the game details
            $scope.game = GamesDataFactory.getClientGame($scope.offerId);
        }

    }

    function originGameIcons(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid'
            },
            controller: 'OriginGameIconsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/views/icons.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameIcons
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offerid of the game you want to interact with
     * @description
     *
     * game tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-icons
     *             offerid="OFB-EAST:109549060"
     *         ></origin-game-icons>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGameIconsCtrl', OriginGameIconsCtrl)
        .directive('originGameIcons', originGameIcons);
}());
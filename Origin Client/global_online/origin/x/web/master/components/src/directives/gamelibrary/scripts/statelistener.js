/**
 * @file /ownedgamecount.js
 */

(function() {
    'use strict';

    function OriginGamelibraryStatelistenerCtrl($scope, $controller, GamesDataFactory, AuthFactory) {
        function getOwnedGamesCount() {
            return GamesDataFactory.baseGameEntitlements().length;
        }

        function handleGamesReady() {
            $scope.gamesCount = getOwnedGamesCount();
            //reset it based on logged in/out state, so if !loggedIn, we want isLoading to be true so that
            //when we do login, we'll wait until entitlements are loaded
            $scope.isLoading = !AuthFactory.isAppLoggedIn();
        }

        function onUpdate() {
            handleGamesReady();
            $scope.$digest();
        }

        $scope.hasGames = function() {
            return $scope.gamesCount !== undefined && $scope.gamesCount > 0;
        };

        $scope.gamesCount = undefined;
        $scope.isLoading = true;

        GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', onUpdate, this);

        $scope.$on('$destroy', function() {
            GamesDataFactory.events.off('games:baseGameEntitlementsUpdate', onUpdate);
        });

        if (GamesDataFactory.initialRefreshCompleted()) {
            handleGamesReady();
        }

        $controller('OriginAuthstatelistenerCtrl', {
            $scope: $scope
        });
    }

    function originGamelibraryStatelistener() {
        return {
            restrict: 'A',
            controller: 'OriginGamelibraryStatelistenerCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryStatelistener
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     * A mix-in to fetch the count of owned games, online and offline and loading progress in order to show
     * marketing language to new users instead of an empty games library
     * injects $scope params
     * bool: isLoading
     * fn: hasGames() - returns bool
     * bool: isOnline
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <ANY ng-if="isLoading && hasGames() && isOnline" origin-gamelibrary-statelistener></ANY>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryStatelistenerCtrl', OriginGamelibraryStatelistenerCtrl)
        .directive('originGamelibraryStatelistener', originGamelibraryStatelistener);
}());
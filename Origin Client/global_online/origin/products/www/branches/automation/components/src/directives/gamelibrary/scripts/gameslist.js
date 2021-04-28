/**
 * @file gamelibrary/gameslist.js
 */
(function() {
    'use strict';

    var GAMELIST_WRAPPER_CLASS = '.origin-gameslist-wrapper';
    var GAMELIST_WRAPPER_HIDDEN_CLASS = 'origin-gameslist-wrapper-hidden';
    var FILTER_APPLIED_EVENT = 'OriginGamelibraryLoggedin:filterApplied';

    function OriginGamelibraryGameslistCtrl($scope, $filter, GamesDataFactory, AppCommFactory, EventsHelperFactory, GameLibraryTileSizeFactory) {
        var handles = [];

        var tileSizeObserver = GameLibraryTileSizeFactory.getTileSizeObserver();
        tileSizeObserver.getProperty('tileSize').attachToScope($scope, 'gridSize');

        $scope.$on('$destroy', function() {
            tileSizeObserver.detach();
        });

        $scope.open = true;

        /**
         * Applies the filter to games list
         * @return {Array}
         */
        $scope.getFilteredGames = function() {
            return $filter('filter')($scope.games, $scope.filterBy());
        };

        /**
         * Checks if there is any game in the list after the filtering
         * @return {boolean}
         */
        $scope.isEmpty = function() {

            var filteredGames = $scope.getFilteredGames();
            if(filteredGames) {
                return filteredGames.length === 0;
            }
        };

        /**
         * If the only games on the page are the ones from this list, the list is considered being alone
         * @return {boolean}
         */
        $scope.isAlone = function() {
            var filteredGames = $scope.getFilteredGames();
            if(filteredGames) {
                return filteredGames.length === $scope.games.length;
            }
        };

        $scope.isGamePlayableOrInstallable = function (game) {
            return GamesDataFactory.isPlayable(game.offerId) || GamesDataFactory.isInstallable(game.offerId);
        };

        /**
         * Show / hide the games list
         * @method toggleGamesList
         * @return {void}
         */
        $scope.triggerValue = function() {
            return $scope.open ? '-' : '+';
        };

        function refreshGames() {
            $scope.filteredGames = $scope.getFilteredGames();
            $scope.$digest();
        }

         handles = [
             AppCommFactory.events.on(FILTER_APPLIED_EVENT, refreshGames)
         ];

         function onDestroy () {
             //clear out these scope properties as they were getting held on
             //possibly because they were used inside the ng-repeat
             //setting it to null, released them
             $scope.isGamePlayableOrInstallable = null;
             $scope.isOffline = null;

             EventsHelperFactory.detachAll(handles);
         }

        $scope.$on('$destroy', onDestroy);

    }

    function originGamelibraryGameslist(ComponentsConfigFactory, UtilFactory) {

        function originGamelibraryGameslistLink(scope, element) {

            var wrapper = element.find(GAMELIST_WRAPPER_CLASS),
                child = wrapper.find('.l-origin-gameslist-list');

            /**
             * Toggle the games list
             * @return {void}
             * @method toggle
             */
            scope.toggle = function() {

                // set the wrapper height to this for now
                wrapper.css('height', wrapper.css('height'));
                wrapper[0].offsetHeight; // jshint ignore:line

                if (scope.open) {
                    wrapper.css('height', '0px');
                    element.find(GAMELIST_WRAPPER_CLASS).addClass(GAMELIST_WRAPPER_HIDDEN_CLASS);
                } else {
                    wrapper.css('height', child[0].offsetHeight + 'px');
                    UtilFactory.onTransitionEnd(wrapper, function() {
                        wrapper.css('height', 'auto');
                        element.find(GAMELIST_WRAPPER_CLASS).removeClass(GAMELIST_WRAPPER_HIDDEN_CLASS);
                    });
                }

                scope.open = !scope.open;
            };

            scope.$watchCollection('games', function(newValues) {
                if (newValues) {
                    scope.filteredGames = scope.getFilteredGames();
                }
            });

        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                titleStr: '@',
                games: '=',
                id: '@',
                filterBy: '&'
            },
            link: originGamelibraryGameslistLink,
            controller: 'OriginGamelibraryGameslistCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/gameslist.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryGameslist
     * @restrict E
     * @element ANY
     * @scope
     * @description game library anonymous container
     *
     * @param {LocalizedString} title-str the title header text
     * @param {Object} games array containing user's games
     * @param {LocalizedString} id identifier of type of games section
     * @param {function} filter-by function that filters by certain criteria
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-gameslist></origin-gamelibrary-gameslist>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryGameslistCtrl', OriginGamelibraryGameslistCtrl)
        .directive('originGamelibraryGameslist', originGamelibraryGameslist);
}());

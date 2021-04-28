/**
 * @file gamelibrary/gameslist.js
 */
(function() {
    'use strict';

    function OriginGamelibraryGameslistCtrl($scope) {
        console.time($scope.title);
        this.gamesListVisible = true;
        $scope.triggerValue = '-';

        /**
         * Show / hide the games list
         * @method toggleGamesList
         * @return {void}
         */
        this.toggleTriggerValue = function() {
            $scope.triggerValue = $scope.triggerValue === '-' ? '+' : '-';
        };

    }

    function originGamelibraryGameslist(ComponentsConfigFactory, UtilFactory) {

        function originGamelibraryGameslistLink(scope, element, attrs, ctrl) {

            var wrapper = element.find('.origin-gameslist-wrapper'),
                child = wrapper.find('.origin-gameslist-list'),
                last,
                games,
                count,
                intervalId,
                watcher;

            /**
             * Toggle the games list
             * @return {void}
             * @method toggleGamesList
             */
            scope.toggleGamesList = function() {

                // set the wrapper height to this for now
                wrapper.css('height', wrapper.css('height'));
                wrapper[0].offsetHeight; // jshint ignore:line

                ctrl.toggleTriggerValue();
                if (ctrl.gamesListVisible) {
                    wrapper.css('height', '0px');
                } else {
                    wrapper.css('height', child[0].offsetHeight + 'px');
                    UtilFactory.onTransitionEnd(wrapper, function() {
                        wrapper.css('height', 'auto');
                    });
                }
                ctrl.gamesListVisible = !ctrl.gamesListVisible;
            };

            /**
             * called when the gamesList has changed after the initial render
             */
            function onGamesListChanged() {
                showGames();
            }

            /**
             * Show an individual game by adding a class
             * to the game.
             * @return {void}
             * @method showGame
             */
            function showGame() {
                angular.element(games[last]).addClass('origin-gameslist-item-isvisible');
                last++;
                if (last === count) {
                    clearInterval(intervalId);
                    intervalId = null;

                    //TODO: revisit this
                    //after we've laid out the games list initially, we need to know when the list has changed so that we can
                    //make the new tile visible.
                    //calling showGames again for one item changing works for now, but is inefficient
                    if (typeof watcher === 'function') {
                        watcher(); // need to cleanup previous watcher
                    }

                    watcher = scope.$watch('games', onGamesListChanged);
                }
            }

            /**
             * Add classes to games in a sequence to create a cool animation
             * @return {void}
             * @method showGames
             */
            function showGames(e) {
                        console.timeEnd(scope.title);

                last = 0;
                games = element.find('.origin-gameslist-item');
                count = games.length;
                if (count) {
                    intervalId = setInterval(showGame, 100);
                }

                if (e) {
                    e.stopPropagation();
                }


                return false;
            }

            //PJ: Do we need a scope.$off ??
            scope.$on('postrepeat:last', showGames);
        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                title: '@',
                games: '=',
                id: '@'
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
     * @description
     *
     * game library anonymous container
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
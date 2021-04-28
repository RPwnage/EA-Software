/**
 * @file tilelist/tilelist.js
 */
(function() {
    'use strict';

    var STAGGER_DELAY = 25;

    function originTileList($timeout, ComponentsConfigFactory) {

        function originTileListLink(scope, elm) {
            var itemsList,
                totalItems,
                timer,
                loading = false,
                currentItem = 0;


            function showTile() {
                totalItems = itemsList.length;
                itemsList[currentItem].classList.add('origin-tilelist-item-isvisible');
                currentItem++;

                if (currentItem < totalItems) {
                    $timeout(showTile, STAGGER_DELAY);
                } else {
                    loading = false; // no more tiles to display
                }

            }

            function showTiles(event) {
                itemsList = elm.find('.origin-tilelist-item');
                totalItems = itemsList.length;

                if (totalItems && !loading) {
                    loading = true;
                    timer = $timeout(showTile, STAGGER_DELAY);
                }

                if (event) {
                    event.stopPropagation();
                }
            }

            scope.$on('postrepeat:last', showTiles);

            scope.$on('$destroy', function () {
                if (timer) {
                    $timeout.cancel(timer);
                }
            });
        }

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            link: originTileListLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('tilelist/views/tilelist.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originTileList
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Generic "tile" list which will render a list of items with transcluded template within
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-tile-list>
     *            <li class="origin-tilelist-item" ng-repeat="item in games" origin-postrepeat>
     *                 <origin-store-game-tile offer-id="{{item.offerId}}" type="responsive"></origin-store-game-tile>
     *            </li>
     *          </origin-tile-list>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originTileList', originTileList);
}());

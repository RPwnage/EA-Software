/**
 * @file tilelist/tilelist-item.js
 */
(function() {
    'use strict';

    function originTileListItem(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                item: '='
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('tilelist/views/tilelist-item.html'),
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originTileListItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} item Individual item model to bind to
     * @description
     *
     * List item to be repeated inside an originTileList directive.
     * Any directive can go inside it to be transcluded in as the item's view
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-tile-list>
     *            <li class="origin-tilelist-item" ng-repeat="item in games" origin-postrepeat>
     *                 <origin-store-game-tile ocd-path="{{item.path}}" type="responsive"></origin-store-game-tile>
     *            </li>
     *          </origin-tile-list>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .directive('originTileListItem', originTileListItem);
}());

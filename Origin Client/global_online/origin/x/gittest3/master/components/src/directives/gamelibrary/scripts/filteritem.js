/**
 * @file filter/filteritem.js
 */
(function() {
    'use strict';

    var ALWAYS_VISIBLE = 'always';

    function originGamelibraryFilterItem(ComponentsConfigFactory, GamesFilterFactory) {
        function originGamelibraryFilterLink(scope, element, attributes) {
            scope.filterId = attributes.filterId;
            scope.model = GamesFilterFactory.getFilter(scope.filterId);

            /**
             * Returns filter item visibility based on the current state:
             * some filter items are only available to online users.
             * @return {boolean}
             */
            scope.isVisible = function () {
                return attributes.visibility === ALWAYS_VISIBLE || Origin.client.isOnline;
            };

            /**
             * Filter items with no game matching are disabled.
             * @return {boolean}
             */
            scope.isEnabled = function () {
                return scope.model.itemsCount > 0;
            };
        }

        return {
            restrict: 'E',
            controller: 'OriginGamelibraryFilterCtrl',
            scope: true,
            link: originGamelibraryFilterLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/filteritem.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryFilterItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} filterId filter identifier as provided by the GamesFilterFactory
     * @param {string} visibility flag indicating whether the filter is visible or not for the current user state (online/always)
     * @description
     *
     * Game library filter panel item.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-filter-item filter-id="isSomething" visibility="always"></origin-gamelibrary-filter-item>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originGamelibraryFilterItem', originGamelibraryFilterItem);
}());

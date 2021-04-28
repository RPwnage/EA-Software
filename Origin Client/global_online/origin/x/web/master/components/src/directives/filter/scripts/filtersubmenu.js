/**
 * @file filter/filtersubmenu.js
 */
(function() {
    'use strict';

    function OriginFilterSubmenuCtrl($scope) {
        /**
         * Toggles open/closed state.
         * @return {void}
         */
        $scope.toggle = function () {
            $scope.open = !$scope.open;
        };

        $scope.open = true;
    }

    function originFilterSubmenu(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            controller: 'OriginFilterSubmenuCtrl',
            scope: {
                label: '@',
                onToggle: '&'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('filter/views/filtersubmenu.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originFilterSubmenu
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} label element label
     * @param {boolean} enabled flag indicating whether the user can click on the element or not
     * @param {function} onToggle function to call when the element state has changed
     * @description
     *
     * Filter panel item that can be toggled on and off.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-filter-submenu label="Something" on-toggle="update()"></origin-filter-submenu>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .controller('OriginFilterSubmenuCtrl', OriginFilterSubmenuCtrl)
        .directive('originFilterSubmenu', originFilterSubmenu);
}());

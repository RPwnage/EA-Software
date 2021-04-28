/**
 * @file filter/filteroption.js
 */
(function() {
    'use strict';

    function OriginFilterOptionCtrl($scope) {
        /**
         * Toggles active/inactive state. Doesn't affect disabled items
         * @return {void}
         */
        $scope.toggle = function () {
            if ($scope.isEnabled()) {
                $scope.isActive = !$scope.isActive;
            }
        };

        $scope.$watch('isActive', function (newValue, oldValue) {
            if (newValue !== oldValue) {
                $scope.onToggle();
            }
        });

        $scope.isActive = $scope.isActive || false;
        $scope.isVisible = $scope.isVisible || true;
        $scope.isEnabled = $scope.isEnabled || true;
    }

    function originFilterOption(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginFilterOptionCtrl',
            scope: {
                label: '@',
                isActive: '=?',
                isVisible: '&',
                isEnabled: '&',
                onToggle: '&'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('filter/views/filteroption.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originFilterOption
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} label element label
     * @param {boolean} isActive filter state
     * @param {boolean} isVisible flag to hide/show the element
     * @param {boolean} isEnabled flag indicating whether the user can click on the element or not
     * @param {function} onToggle function to call when the element state has changed
     * @description
     *
     * Filter panel item that can be toggled on and off.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-filter-oprion label="Something" is-active="filters.something.active" on-toggle="update()"></origin-filter-option>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .controller('OriginFilterOptionCtrl', OriginFilterOptionCtrl)
        .directive('originFilterOption', originFilterOption);
}());

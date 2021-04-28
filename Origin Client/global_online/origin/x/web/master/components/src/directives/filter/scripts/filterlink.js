/**
 * @file filter/filterlink.js
 */
(function() {
    'use strict';

    function OriginFilterLinkCtrl($scope) {
        /**
         * Execute callback function when the user clicks the link
         * @return {void}
         */
        $scope.click = function () {
            if ($scope.isEnabled()) {
                $scope.onClick();
            }
        };
    }

    function originFilterLink(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginFilterLinkCtrl',
            scope: {
                label: '@',
                isEnabled: '&',
                onClick: '&'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('filter/views/filterlink.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originFilterLink
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} label element label
     * @param {boolean} enabled flag indicating whether the user can click on the element or not
     * @param {function} onClick action to perform when the element is clicked on
     * @description
     *
     * Simple stateless filter element
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-filter-link label="Clear" enabled="canClearFilters()" action="clearAllFilters()"></origin-filter-link>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .controller('OriginFilterLinkCtrl', OriginFilterLinkCtrl)
        .directive('originFilterLink', originFilterLink);
}());

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
        $scope.action = function () {
            if ($scope.isEnabled()) {
                $scope.actionCallback();
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
                actionCallback: '&'
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
     * @param {function} action-callback action to perform when the element is clicked on
     * @description
     *
     * Simple stateless filter element. This directive is a partial not directly merchandised.
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

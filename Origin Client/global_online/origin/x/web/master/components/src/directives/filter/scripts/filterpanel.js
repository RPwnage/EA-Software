/**
 * @file filter/filterpanel.js
 */
(function() {
    'use strict';

    function originFilterPanel(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                isVisible: '&'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('filter/views/filterpanel.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originFilterPanel
     * @restrict E
     * @element ANY
     * @scope
     * @param {boolean} isVisible flag indicating whether the panel is open or closed
     * @description
     *
     * Slide-out panel for filters
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-filterpanel is-visible="isFilterPanelVisible"></origin-filterpanel>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originFilterPanel', originFilterPanel);
}());

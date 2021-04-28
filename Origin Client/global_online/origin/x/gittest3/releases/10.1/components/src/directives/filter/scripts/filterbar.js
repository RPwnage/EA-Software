/**
 * @file filter/filterbar.js
 */
(function() {
    'use strict';

    function originFilterBar(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('filter/views/filterbar.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originFilterBar
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Horisontal area displaying active filters
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-filterbar></origin-filterbar>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originFilterBar', originFilterBar);
}());
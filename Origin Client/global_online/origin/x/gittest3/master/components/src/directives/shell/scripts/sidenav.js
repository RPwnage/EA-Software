/**
 * @file shell/scripts/sidenav.js
 */
(function() {
    'use strict';

    function originShellSideNav(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/sidenav.html'),
            transclude: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellSideNav
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * side nav
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-side-nav></origin-shell-side-nav>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originShellSideNav', originShellSideNav);
}());
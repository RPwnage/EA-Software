/**
 * @file shell/scripts/tertiary.js
 */
(function() {
    'use strict';

    function originShellTertiaryMenu(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/tertiary.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellTertiaryMenu
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Tertiary menu
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-shell-tertiary-menu>
     *         <origin-shell-menu-item>
     *             <origin-shell-menu-item-link titlestr="Home" href="#" icon="home"></origin-shell-menu-item-link>
     *         </origin-shell-menu-item>
     *     </origin-shell-tertiary-menu>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originShellTertiaryMenu', originShellTertiaryMenu);
}());
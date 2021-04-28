/**
 * @file shell/scripts/footermenu.js
 */
(function() {
    'use strict';

    function originShellFooterMenu(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/footermenu.html'),
            transclude: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellFooterMenu
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * footer menu
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-footer-menu>
     *             <origin-shell-menu-item>
     *                 <origin-shell-menu-item-link titlestr="Get Started" href="#/getstarted"></origin-shell-menu-item-link>
     *             </origin-shell-menu-item>
     *         </origin-shell-footer-menu>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originShellFooterMenu', originShellFooterMenu);
}());
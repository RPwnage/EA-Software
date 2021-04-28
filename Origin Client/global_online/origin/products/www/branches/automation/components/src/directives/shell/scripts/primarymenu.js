/**
 * @file shell/scripts/primarymenu.js
 */
(function() {
    'use strict';

    function originShellPrimaryMenu(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/primarymenu.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellPrimaryMenu
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * primary menu
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-shell-primary-menu>
     *         <origin-shell-menu-item>
     *             <origin-shell-menu-item-link titlestr="Home" href="#" icon="home"></origin-shell-menu-item-link>
     *         </origin-shell-menu-item>
     *     </origin-shell-primary-menu>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originShellPrimaryMenu', originShellPrimaryMenu);
}());
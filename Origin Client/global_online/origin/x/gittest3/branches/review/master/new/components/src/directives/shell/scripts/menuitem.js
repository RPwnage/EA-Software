/**
 * @file shell/scripts/menuitem.js
 */
(function() {
    'use strict';

    function originShellMenuItem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/menuitem.html'),
            replace: true,
            transclude: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellMenuItem
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * menu item
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-menu-item>
     *             <origin-shell-menu-item-link titlestr="Home" href="#" icon="home"></origin-shell-menu-item-link>
     *         </origin-shell-menu-item>
     *     </file>
     * </example>
     *     
     */
    angular.module('origin-components')
        .directive('originShellMenuItem', originShellMenuItem);
}());
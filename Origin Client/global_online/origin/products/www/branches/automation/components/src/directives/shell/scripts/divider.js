
/**
 * @file /shell/scripts/divider.js
 */
(function(){
    'use strict';
    function originShellDivider() {
        return {
            restrict: 'E',
            template: '<li ng-transclude class="origin-shell-divider"></li>',
            transclude: true,
            replace: true
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellDivider
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-shell-divider></origin-shell-divider>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originShellDivider', originShellDivider);
}());

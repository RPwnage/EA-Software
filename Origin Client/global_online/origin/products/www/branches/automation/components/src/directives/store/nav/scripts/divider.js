/**
 * @file store/scripts/nav/divider.js
 */
(function(){
    'use strict';

    function originStoreNavDivider() {
        return {
            restrict: 'E',
            replace: true,
            template: '<li class="origin-storenav-divider otkmenu-context-divider"></li>'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreNavDivider
     * @restrict E
     * @element ANY
     * @scope
     * @description Store nav Divider
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-nav-divider><origin-store-nav-item>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreNavDivider', originStoreNavDivider);
}());
/**
 * @file home/page/anonymous.js
 */
(function() {
    'use strict';

    function originHomePageAnonymous() {
        return {
            restrict: 'E',
            transclude: true,
            template: '<div class="origin-home origin-home-anon" ng-transclude></div>'
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomePageAnonymous
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * home anonymous panel container
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-page-anonymous></origin-home-page-anonymous>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomePageAnonymous', originHomePageAnonymous);
}());
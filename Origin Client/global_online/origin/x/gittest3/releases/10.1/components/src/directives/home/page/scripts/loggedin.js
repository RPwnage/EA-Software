/**
 * @file home/page/loggedin.js
 */
(function() {
    'use strict';

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomePageLoggedin
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * home panel container
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-page-loggedin></origin-home-page-loggedin>
     *     </file>
     * </example>
     */
    function originHomePageLoggedin() {
        return {
            restrict: 'E',
            transclude: true,
            template: '<div class="origin-home" ng-transclude></div>'
        };
    }

    angular.module('origin-components')
        .directive('originHomePageLoggedin', originHomePageLoggedin);
}());
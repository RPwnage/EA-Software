/**
 * @file home/page/anonymous.js
 */
(function() {
    'use strict';

    function originHomePageAnonymous(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/page/views/anonymous.html')
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
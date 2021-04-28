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
    function originHomePageLoggedin(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/page/views/loggedin.html')
        };
    }

    angular.module('origin-components')
        .directive('originHomePageLoggedin', originHomePageLoggedin);
}());
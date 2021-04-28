/**
 * @file home/newuser/area.js
 */
(function() {
    'use strict';


    function originHomeNewuserArea(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/newuser/views/area.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeNewuserArea
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * new user area
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-newuser-area></origin-home-newuser-area>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeNewuserArea', originHomeNewuserArea);
}());
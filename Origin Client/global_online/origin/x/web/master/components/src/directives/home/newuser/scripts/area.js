/**
 * @file home/newuser/area.js
 */
(function() {
    'use strict';

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
    function originHomeNewuserArea(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                anonymous: '@anonymous',
            }, //I too, live dangerously.
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/newuser/views/area.html')
        };
    }

    angular.module('origin-components')
        .directive('originHomeNewuserArea', originHomeNewuserArea);
}());
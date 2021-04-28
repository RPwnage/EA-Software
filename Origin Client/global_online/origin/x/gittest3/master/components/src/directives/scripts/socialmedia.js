/**
 * @file socialmedia.js
 */
(function() {
    'use strict';

    function originSocialmedia(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/socialmedia.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialmedia
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Social Media directive to hold buttons
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-socialmedia></origin-socialmedia>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originSocialmedia', originSocialmedia);
}());
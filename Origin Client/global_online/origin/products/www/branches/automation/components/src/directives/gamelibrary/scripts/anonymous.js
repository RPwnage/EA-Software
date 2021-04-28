/**
 * @file gamelibrary/anonymous.js
 */
(function() {
    'use strict';

    function originGamelibraryAnonymous(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/anonymous.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryAnonymous
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * game library anonymous container
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-anonymous></origin-gamelibrary-anonymous>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originGamelibraryAnonymous', originGamelibraryAnonymous);
}());
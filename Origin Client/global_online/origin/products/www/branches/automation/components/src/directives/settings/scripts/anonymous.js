/**
 * @file Settings/anonymous.js
 */
(function() {
    'use strict';

    function originSettingsAnonymous(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/anonymous.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettingsAnonymous
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
     *         <origin-Settings-anonymous
     *         </origin-Settings-anonymous>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originSettingsAnonymous', originSettingsAnonymous);
}());
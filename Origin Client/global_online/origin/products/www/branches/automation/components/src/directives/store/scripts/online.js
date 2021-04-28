/**
 * @file store/online.js
 */
(function() {
    'use strict';

    function originStoreOnline(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/online.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreOnline
     * @restrict E
     * @element ANY
     * @scope
     *
     * store online container
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-online></origin-store-online>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originStoreOnline', originStoreOnline);
}());
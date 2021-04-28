
/**
 * @file store/scripts/viewall.js
 */
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStoreViewall(ComponentsConfigFactory) {
        /**
        * The directive link
        */
        function originStoreViewallLink(scope) {
            // Add Scope functions and call the controller from here
            scope.viewAllHref = '#/search?searchString=' + scope.searchString;
        }
        return {
            restrict: 'E',
            scope: false,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/viewall.html'),
            link: originStoreViewallLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreViewall
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-viewall />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreViewall', originStoreViewall);
}());

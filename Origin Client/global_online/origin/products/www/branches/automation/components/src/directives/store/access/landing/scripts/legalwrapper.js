
/** 
 * @file store/access/landing/scripts/legalwrapper.js
 */ 
(function(){
    'use strict';
    function originStoreAccessLandingLegalwrapper(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/landing/views/legalwrapper.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessLandingLegalwrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-landing-legalwrapper />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessLandingLegalwrapper', originStoreAccessLandingLegalwrapper);
}()); 

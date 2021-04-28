/**
 * @file store/pdp/termsandconditions.js
 */
(function(){
    'use strict';

    function originStorePdpTermsandconditionsCtrl($scope, PdpUrlFactory, ProductFactory, ObjectHelperFactory) {
        var takeHead = ObjectHelperFactory.takeHead;

        ProductFactory
            .filterBy(PdpUrlFactory.getCurrentEditionSelector())
            .addFilter(takeHead)
            .attachToScope($scope, 'model');
    }

    function originStorePdpTermsandconditions(ComponentsConfigFactory) {
        return {
            replace: true,
            restrict: 'E',
            scope: {},
            controller: 'originStorePdpTermsandconditionsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/termsandconditions.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpTermsandconditions
     * @restrict E
     * @element ANY
     * @scope
     * @description PDP Terms and Conditions block, retrieved from catalog
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-termsandconditions></origin-store-pdp-termsandconditions>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('originStorePdpTermsandconditionsCtrl', originStorePdpTermsandconditionsCtrl)
        .directive('originStorePdpTermsandconditions', originStorePdpTermsandconditions);
}());
/**
 * @file store/pdp/systemrequirements.js
 */
(function(){
    'use strict';

    function originStorePdpSystemrequirementsCtrl($scope, PdpUrlFactory, ProductFactory, ObjectHelperFactory) {
        var takeHead = ObjectHelperFactory.takeHead;

        ProductFactory
            .filterBy(PdpUrlFactory.getCurrentEditionSelector())
            .addFilter(takeHead)
            .attachToScope($scope, 'model');
    }

    function originStorePdpSystemrequirements(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {},
            controller: 'originStorePdpSystemrequirementsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/systemrequirements.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSystemrequirements
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * PDP system requirements block, retrieved from catalog
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-systemrequirements></origin-store-pdp-systemrequirements>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('originStorePdpSystemrequirementsCtrl', originStorePdpSystemrequirementsCtrl)
        .directive('originStorePdpSystemrequirements', originStorePdpSystemrequirements);
}());

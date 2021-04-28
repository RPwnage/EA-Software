/**
 * @file store/pdp/scripts/overview.js
 */
(function(){
    'use strict';

    function OriginStorePdpOverviewCtrl($scope, PdpUrlFactory, ProductFactory, ObjectHelperFactory) {
        var takeHead = ObjectHelperFactory.takeHead;

        ProductFactory
            .filterBy(PdpUrlFactory.getCurrentEditionSelector())
            .addFilter(takeHead)
            .attachToScope($scope, 'model');
    }

    function originStorePdpOverview(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/overview.html'),
            controller: 'OriginStorePdpOverviewCtrl'
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpOverview
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * PDP overview blocks, retrieved from catalog
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-overview></origin-store-pdp-overview>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpOverviewCtrl', OriginStorePdpOverviewCtrl)
        .directive('originStorePdpOverview', originStorePdpOverview);
}());

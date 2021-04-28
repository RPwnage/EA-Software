/**
 * @file store/pdp/scripts/overview-item.js
 */
(function(){
    'use strict';

    function originStorePdpOverviewItem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                title: '@',
                message: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/overview-item.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpOverviewItem
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Single PDP overview block
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-overview-item title="Genre" message="{{ model.genre }}"></origin-store-pdp-overview-item>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpOverviewItem', originStorePdpOverviewItem);
}());

/**
 * @file store/pdp/scripts/details.js
 */
(function(){
    'use strict';

    function originStorePdpDetails(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {},
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/details.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpDetails
     * @restrict E
     * @element ANY
     * @scope
     * @description PDP details blocks,retrieved from cq5
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-details></origin-store-pdp-details>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpDetails', originStorePdpDetails);
}());

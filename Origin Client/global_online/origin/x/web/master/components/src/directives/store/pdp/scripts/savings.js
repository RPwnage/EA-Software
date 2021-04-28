/**
 * @file store/pdp/scripts/savings.js
 */
(function(){
    'use strict';

    function originStorePdpSavings(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                savings: '@',
                strikePrice: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/savings.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSavings
     * @restrict E
     * @element ANY
     * @scope
     * @description PDP Savings block
     * @param {string} savings string
     * @param {string} strike-price strike price
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-savings savings="25%" strike-price="$99.99"></origin-store-pdp-savings>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpSavings', originStorePdpSavings);
}());

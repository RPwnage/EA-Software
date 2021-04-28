/**
 * @file store/pdp/scripts/savings.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-hero';

    function OriginStorePdpSavingsCtrl($scope, UtilFactory) {

       function init() {

           $scope.pdpSavingStrings = {};

           $scope.$watch('model.savings', function() {
               $scope.pdpSavingStrings.saveText = UtilFactory.getLocalizedStr($scope.saveText, CONTEXT_NAMESPACE, 'save-text', {
                   '%savings%': $scope.model.savings
               });
           });

           $scope.pdpSavingStrings.originAccessDiscountApplied = UtilFactory.getLocalizedStr($scope.oaDiscountApplied, CONTEXT_NAMESPACE, 'oa-discount-applied');
       }

        $scope.$watchOnce('ocdDataReady', init);
    }

    function originStorePdpSavings(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/savings.html'),
            controller: OriginStorePdpSavingsCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSavings
     * @restrict E
     * @element ANY
     * @scope
     * @description PDP Savings block
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-savings></origin-store-pdp-savings>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpSavingsCtrl', OriginStorePdpSavingsCtrl)
        .directive('originStorePdpSavings', originStorePdpSavings);
}());

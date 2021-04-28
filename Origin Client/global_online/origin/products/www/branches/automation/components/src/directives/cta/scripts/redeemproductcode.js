/**
 * @file cta/redeemproductcode.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-cta-redeem-product-code';

    function OriginCtaRedeemProductCodeCtrl($scope, UtilFactory) {
        $scope.type = $scope.type || 'primary';
        $scope.btnText = UtilFactory.getLocalizedStr($scope.btnText, CONTEXT_NAMESPACE, 'description');

        $scope.onBtnClick = function() {
            Origin.client.games.redeemProductCode();
        };

    }

    function originCtaRedeemProductCode(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                btnText: '@description',
                type: '@'
            },
            controller: 'OriginCtaRedeemProductCodeCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaRedeemProductCode
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} description *Update in Defaults
     * RedeemProductCode (takes you to redeem game popup) call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-redeem-product-code description="Redeem Produce Code" type="primary"></origin-cta-redeem-product-code>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaRedeemProductCodeCtrl', OriginCtaRedeemProductCodeCtrl)
        .directive('originCtaRedeemProductCode',  originCtaRedeemProductCode);
}());
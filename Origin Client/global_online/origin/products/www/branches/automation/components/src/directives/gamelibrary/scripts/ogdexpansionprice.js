/**
 * @file /gamelibrary/scripts/ogdexpansionprice.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-ogdexpansionprice';

    function originGamelibraryOgdexpansionpriceCtrl($scope, UtilFactory, SubscriptionFactory, ProductFactory, ProductInfoHelper) {

        $scope.saveLoc = UtilFactory.getLocalizedStr($scope.savestr, CONTEXT_NAMESPACE, 'savestr');
        $scope.originAccessDiscountAppliedLoc = UtilFactory.getLocalizedStr($scope.originAccessDiscountAppliedStr, CONTEXT_NAMESPACE, 'originaccessdiscountappliedstr');

        $scope.isSubscriber = SubscriptionFactory.userHasSubscription();

        $scope.isFree = function(model) {
            return ProductInfoHelper.isFree(model);
        };

        ProductFactory.get($scope.offerId).attachToScope($scope, 'model');
    }

    function originGamelibraryOgdexpansionprice(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@',
                savestr: "@savestr",
                originAccessDiscountAppliedStr: "@originaccessdiscountappliedstr"
            },
            controller: 'originGamelibraryOgdexpansionpriceCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdexpansionprice.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdexpansionprice
     * @restrict E
     * @element ANY
     * @scope
     * @param {OfferId} offer-id the offer id
     * @param {LocalizedString} savestr Save
     * @param {LocalizedString} originaccessdiscountappliedstr Origin Access Discount Applied
     * @description
     *
     * Shows Price for an Offer
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-ogdexpansionprice originaccessdiscountappliedstr="Discount applied" savestr="Save" ownedstr="You own this" offer-id="OFR.123"></origin-gamelibrary-ogdexpansionprice>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('originGamelibraryOgdexpansionpriceCtrl', originGamelibraryOgdexpansionpriceCtrl)
        .directive('originGamelibraryOgdexpansionprice', originGamelibraryOgdexpansionprice);
}());

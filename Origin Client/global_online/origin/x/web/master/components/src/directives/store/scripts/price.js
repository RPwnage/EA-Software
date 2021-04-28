/**
 * @file /store/scripts/price.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-price',
        HideOwnedStateEnumeration = {
            "true": "true",
            "false": "false"
        };

    function originStorePriceCtrl($scope, UtilFactory, ProductFactory, SubscriptionFactory) {

        $scope.isSubscriber = false;
        $scope.isFree = false;
        $scope.model = {};

        $scope.strings = {
            ownedText: UtilFactory.getLocalizedStr($scope.ownedText, CONTEXT_NAMESPACE, 'owned'),
            orText: UtilFactory.getLocalizedStr($scope.orText, CONTEXT_NAMESPACE, 'or'),
            freeText: UtilFactory.getLocalizedStr($scope.freeText, CONTEXT_NAMESPACE, 'free'),
            withText: UtilFactory.getLocalizedStr($scope.withText, CONTEXT_NAMESPACE, 'with'),
            zeroPriceText: UtilFactory.getLocalizedStr($scope.zeroPriceText, CONTEXT_NAMESPACE, 'zeroprice')
        };

        $scope.isFree = function() {
            return $scope.model.oth || $scope.model.demo || $scope.model.trial;
        };

        $scope.isSubscriber = function() {
            return SubscriptionFactory.userHasSubscription();
        };

        $scope.isOwnedTextVisible = function() {
            return String($scope.hideOwnedState).toLowerCase() !== HideOwnedStateEnumeration["true"];
        };

        function updateModel(newData) {
            $scope.model = newData;
            $scope.strings.saveText = UtilFactory.getLocalizedStr($scope.saveText, CONTEXT_NAMESPACE, 'save', {
                '%savings%': newData.savings
            });
        }

        $scope.$watch('offerId', function(newValue) {
            if (newValue) {
                ProductFactory.get(newValue).attachToScope($scope, updateModel);
            }
        });
    }

    function originStorePrice(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@',
                ownedText: "@",
                orText: "@",
                saveText: "@",
                freeText: "@",
                withText: "@",
                zeroPriceText: "@",
                hideOwnedState: "@"
            },
            controller: 'originStorePriceCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/price.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePrice
     * @restrict E
     * @element ANY
     * @scope
     * @param {OfferId} offer-id the offer id
     * @param {LocalizedString|OCD} owned-text The "You Own This" text
     * @param {LocalizedString|OCD} or-text The "or [price]" text
     * @param {LocalizedString|OCD} free-text The "FRR" text
     * @param {LocalizedString|OCD} with-text The "with [origin access]" text
     * @param {LocalizedString|OCD} save-text (optional) override text for the Savings indicator
     * @param {LocalizedString|OCD} zero-price-text The "$0.00" text
     * @param {HideOwnedStateEnumeration} hide-owned-state boolean to show whether we want to show the "owned text"
     * @description
     *
     * Shows Price for an Offer
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-price offer-id="OFR.123"></origin-store-price>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('originStorePriceCtrl', originStorePriceCtrl)
        .directive('originStorePrice', originStorePrice);
}());

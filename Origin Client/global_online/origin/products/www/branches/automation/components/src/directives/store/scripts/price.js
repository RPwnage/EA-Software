/**
 * @file /store/scripts/price.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-price';

    var BooleanEnumeration = {
            "true": "true",
            "false": "false"
        };
    var LayoutOrientation = {
        "horizontal": "horizontal",
        "vertical": "vertical"
    };

    function originStorePriceCtrl(
        $scope,
        ProductFactory,
        SubscriptionFactory,
        UtilFactory,
        ProductInfoHelper
    ) {

        $scope.isSubscriber = SubscriptionFactory.userHasSubscription(); // SubscriptionFactory.bUserHasSubscription is only set once per Origin session (i.e., on login)
        $scope.isFree = false;
        $scope.isOwnedTextVisible = String($scope.hideOwnedState).toLowerCase() !== BooleanEnumeration.true;
        $scope.model = {};
        $scope.showFreeText = false;
        $scope.showPrice = false;
        $scope.showFormattedPrice = false;
        $scope.showSaveText = false;
        $scope.showFormattedStrikePrice = false;
        $scope.showIncludedWithText = false;
        $scope.showAccessLogo = false;
        $scope.showOwnedText = false;
        $scope.suppressAccessText = $scope.suppressAccessText === BooleanEnumeration.true || false;

        $scope.strings = {
            ownedText: UtilFactory.getLocalizedStr($scope.ownedText, CONTEXT_NAMESPACE, 'owned-text'),
            orText: UtilFactory.getLocalizedStr($scope.orText, CONTEXT_NAMESPACE, 'or-text'),
            freeText: UtilFactory.getLocalizedStr($scope.freeText, CONTEXT_NAMESPACE, 'free-text'),
            withText: UtilFactory.getLocalizedStr($scope.withText, CONTEXT_NAMESPACE, 'with-text'),
            included: UtilFactory.getLocalizedStr($scope.included, CONTEXT_NAMESPACE, 'included')
        };

        function updateModel(newData) {
            $scope.model = newData;
            $scope.strings.saveText = UtilFactory.getLocalizedStr($scope.saveText, CONTEXT_NAMESPACE, 'save-text', {
                '%savings%': newData.savings
            });

            $scope.isFree = ProductInfoHelper.isFree(newData);
            $scope.ownsThroughOAX = $scope.model.vaultEntitlement;
            $scope.showFreeText = $scope.isFree && (!$scope.model.isOwned || $scope.ownsThroughOAX);
            $scope.showPrice = ((!$scope.model.isUpgradeable && !$scope.model.subscriptionAvailable) || $scope.suppressAccessText) && !$scope.model.isOwned && !$scope.isFree;
            $scope.showFormattedPrice = $scope.model.formattedPrice;
            $scope.showSaveText = $scope.strings.saveText && $scope.model.strikePrice !== $scope.model.price;
            $scope.showFormattedStrikePrice = $scope.model.strikePrice && $scope.model.strikePrice !== $scope.model.price;
            $scope.showIncludedWithText = !$scope.isSubscriber && !$scope.model.isOwned && !$scope.isFree && ($scope.model.subscriptionAvailable || $scope.model.isUpgradeable) &&!$scope.suppressAccessText;
            $scope.showOwnedText = (($scope.model.isOwned && !$scope.ownsThroughOAX ) || ($scope.ownsThroughOAX && !$scope.isFree)) && $scope.isOwnedTextVisible;
            $scope.showAccessLogo = !$scope.model.isOwned && $scope.isSubscriber && !$scope.isFree && ($scope.model.subscriptionAvailable || $scope.model.isUpgradeable) && !$scope.showOwnedText &&!$scope.suppressAccessText;
            // Two layouts for pricing info: horizontal/vertical
            $scope.pricingLayoutHorizontal = LayoutOrientation.horizontal === $scope.pricingLayout;

            $scope.$emit('priceVisible', priceVisible());
        }

        function priceVisible() {
            return $scope.showFreeText ||
                ($scope.showPrice &&
                    ($scope.showFormattedPrice || $scope.showSaveText || $scope.showFormattedStrikePrice)) ||
                $scope.showIncludedWithText ||
                $scope.showAccessLogo ||
                $scope.showOwnedText;
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
                ownedText: '@',
                orText: '@',
                saveText: '@',
                freeText: '@',
                withText: '@',
                hideOwnedState: '@',
                included: '@',
                suppressAccessText: '@',
                pricingLayout: '@'
            },
            controller: 'originStorePriceCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/price.html')
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
     * @param {BooleanEnumeration} hide-owned-state boolean to show whether we want to show the "owned text"
     * @param {LocalizedString|OCD} included The "Included" with access text
     * @param {BooleanEnumeration} suppress-access-text suppress access text and show pricing instead
     *
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

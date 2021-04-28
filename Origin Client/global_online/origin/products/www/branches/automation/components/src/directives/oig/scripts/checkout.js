/**
 * @file /oig/scripts/checkout.js
 */
(function() {
    'use strict';

    function OriginOigCheckoutCtrl($stateParams, CartModel, CheckoutFactory, PurchaseFactory, GamesCommerceFactory, GamesDataFactory, GameRefiner, ComponentsLogFactory, OriginCheckoutTypesConstant) {
        var offerIds = $stateParams.offerId ? $stateParams.offerId.split(',') : [];

        function checkForSubscriptionOffer(catalog) {
            var subscriptionOffer = '';
            _.forEach(catalog, function(catalogInfo) {
                if (GameRefiner.isSubscription(catalogInfo)) {
                    subscriptionOffer = Promise.resolve(catalogInfo.offerId);
                    return false;
                }
            });
            return subscriptionOffer;
        }

        function startPurchaseFlow(subscriptionOffer) {
            if (subscriptionOffer && offerIds.length === 1) {
                // Use the subscription flow if there is only one offer to be purchased and it's a subscription offer.
                // The addon flow will account for additional offers by using the standard or 'chained' flow.
                PurchaseFactory.buy(subscriptionOffer, {checkoutType: OriginCheckoutTypesConstant.SUBS});
            } else {
                PurchaseFactory.buyAddons(offerIds);
            }
        }

        function handleError(error) {
            ComponentsLogFactory.error('[OIG Checkout] error: ', error);
        }

        if (offerIds.length > 0) {
            GamesCommerceFactory.checkForCheckoutMessage($stateParams.profile, $stateParams.masterTitleId)
                .then(_.partial(GamesDataFactory.getCatalogInfo, offerIds))
                .then(checkForSubscriptionOffer)
                .then(startPurchaseFlow)
                .catch(handleError);
        } else {
            ComponentsLogFactory.error('[OIG Checkout] No offer ID passed');
        }
    }

    function originOigCheckout(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {},
            controller: 'OriginOigCheckoutCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/oig/views/checkout.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originOigCheckout
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Addon store for in-game purchases
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-oig-checkout></origin-oig-checkout>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginOigCheckoutCtrl', OriginOigCheckoutCtrl)
        .directive('originOigCheckout', originOigCheckout);
}());

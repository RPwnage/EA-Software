/**
 * @file bundlepurchase.js
 */
(function(){
    'use strict';

    function BundlePurchaseFactory(AuthFactory, OwnershipFactory, CheckoutFactory, StoreBundleFactory, PurchasePreventionConstant, PurchasePreventionFactory, PageThinkingFactory, AppCommFactory, DialogFactory, ObjectHelperFactory, CartModel, ErrorModalFactory) {
        /**
         * Removes the owned items from the bundle
         *
         * @param {Catalog[]} bundleProducts array of catalog data object
         * @return {promise} promise resolve to array of unowned products
         */
        function removeOwnedFromBundle(bundleProducts) {
            var ownedProducts = OwnershipFactory.getOutrightOwnedProducts(bundleProducts);
            StoreBundleFactory.removeAll(ownedProducts);
            
            completePurchase(OwnershipFactory.getOutrightUnownedProducts(bundleProducts));
        }

        function endPerformaceTimeOnError(error) {
            Origin.performance.endTime('COMPONENTSPERF:checkoutCart');
            return Promise.reject(error);
        }

        function completePurchase(bundleProducts) {
            Origin.performance.beginTime('COMPONENTSPERF:checkoutCart');

            CartModel.addOfferList(extractOfferIds(bundleProducts))
                .then(_.partial(CheckoutFactory.checkout, undefined, undefined))
                .then(StoreBundleFactory.clear) // Clear cart upon successful checkout
                .catch(endPerformaceTimeOnError)
                .catch(ErrorModalFactory.showDialog);
        }

        function extractOfferIds(bundleProducts) {
            return _.map(bundleProducts, function(product) {
                return product.offerId;
            });
        }

        /**
         * Callback for the dialog checks to see the users response and executes that action
         * @param  {Object} result  The data coming back from the dialog
         */
        function handlePurchasePreventionChoice(result, bundleProducts) {
            // Probably need to check the id in the result and we will need a destroy
            if (result.accepted) {
                removeOwnedFromBundle(bundleProducts);
            }
        }

        /**
         * Make sure all information needed for purchase is in place, then initiate purchase.
         * ie. get catalog info, make sure entitlements are loaded and ownership data is set
         */
        function setupPurchase(offerIds) {

            OwnershipFactory.getGameDataWithEntitlements(offerIds)
                .then(ObjectHelperFactory.toArray)
                .then(initiatePurchase)
                .then(PageThinkingFactory.unblockUIPromise)
                .catch(PageThinkingFactory.unblockUIPromiseOnError)
                .catch(ErrorModalFactory.showDialog);
        }

        /**
         * Checks the entitlements for the list of products provided.
         * @param  {Array} bundleProducts An array of catalog objects
         * @return {boolean}                True if the user owns some of the products.
         */
        function doesUserOwnAny(bundleProducts) {
            var ownedProducts = OwnershipFactory.getOutrightOwnedProducts(bundleProducts);
            return ownedProducts.length > 0;
        }

        /**
         * Starts purchase prevention or completes purchase
         * @param  {Array} bundleProducts List of products in the bundle
         */
        function initiatePurchase(bundleProducts) {
            var productWithoutBaseGames = PurchasePreventionFactory.getProductsWithUnownedBaseGames(bundleProducts);

            if (doesUserOwnAny(bundleProducts)) {
                DialogFactory.open({
                    id: PurchasePreventionConstant.BUNDLE_DIALOG_ID,
                    xmlDirective: PurchasePreventionFactory.getBundlePurchasePreventionDirectiveXML(extractOfferIds(bundleProducts)),
                    size: 'large'
                });
                AppCommFactory.events.once('dialog:hide', _.partialRight(handlePurchasePreventionChoice, bundleProducts));
            } else if (productWithoutBaseGames.length) {
                var productsWithBaseGameNotInCart = PurchasePreventionFactory.getProductsWithBaseGameNotInCart(bundleProducts);

                if (productsWithBaseGameNotInCart.length) {
                    DialogFactory.open({
                        id: PurchasePreventionConstant.BUNDLE_DLC_DIALOG_ID,
                        xmlDirective: PurchasePreventionFactory.getDLCPurchasePreventionModal(),
                        size: 'large'
                    });
                } else {
                    completePurchase(bundleProducts);
                }
            } else {
                completePurchase(bundleProducts);
            }
        }

        function buy(offerIds) {
            AuthFactory.promiseLogin()
                .then(PageThinkingFactory.blockUIPromise)
                .then(_.partial(setupPurchase, offerIds));
        }

        return {
            buy: buy
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.BundlePurchaseFactory
     * @description  Implements bundle purchase related actions
     */
    angular.module('origin-components')
        .factory('BundlePurchaseFactory', BundlePurchaseFactory);
}());
/**
 * @file store/bundle/cart.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-bundle-main';

    function OriginStoreBundleCartCtrl($scope, BundlePurchaseFactory, CartModel, CurrencyHelper, StoreBundleFactory) {
        StoreBundleFactory.getAll($scope.bundleId).attachToScope($scope, 'bundle');

        /**
         * Click handles for the buy button initiates the buy or purchase prevention flow.
         */
        $scope.buyNow = function() {
            if ($scope.bundle.length) {
                BundlePurchaseFactory.buy($scope.bundle);
            }
        };

        function cartPriceUpdate(cartResponse) {

            var total = cartResponse.totalPriceWithoutTax,
                discount = cartResponse.totalDiscountAmount,
                subTotal = total + discount,
                prices = [
                    CurrencyHelper.formatCurrency(subTotal),
                    CurrencyHelper.formatCurrency(total),
                    CurrencyHelper.formatCurrency(discount)
                ];

            function checkPriceValidity(price) {
                return price ? price : '--';
            }

            Promise.all(prices).then(function(result){
                $scope.bundlePrice = {
                    'subTotal': subTotal,
                    'total': total,
                    'discount': discount,
                    'subTotalPrice': checkPriceValidity(result[0]),
                    'totalPrice': checkPriceValidity(result[1]),
                    'discountPrice': checkPriceValidity(result[2])
                };
                $scope.$digest();
            });
        }

        function cartPriceZero() {
            $scope.bundlePrice = {'subTotal': 0, 'total': 0, 'discount': 0};
        }

        $scope.$watchCollection('bundle', function(newvalue){
            // bundle has been updated, refresh and get cart, then update bundlePrice object data
            if (newvalue.length > 0) {
                CartModel.addOfferList(newvalue)
                    .then(CartModel.getCart())
                    .then(cartPriceUpdate)
                    .catch(cartPriceZero);
            } else {
                cartPriceZero();
            }
        });
    }

    function originStoreBundleCart(ComponentsConfigFactory, UtilFactory) {
        function originStoreBundleCartLink(scope) {
            scope.bundlePrice = {'subTotal': 0, 'total': 0, 'discount': 0};
            scope.strings = {
                yourBundleText: UtilFactory.getLocalizedStr(scope.yourBundleText, CONTEXT_NAMESPACE, 'your-bundle-text'),
                buyNowText: UtilFactory.getLocalizedStr(scope.buyNowText, CONTEXT_NAMESPACE, 'buy-now-text'),
                subtotalText: UtilFactory.getLocalizedStr(scope.subtotalText, CONTEXT_NAMESPACE, 'subtotal-text'),
                totalText: UtilFactory.getLocalizedStr(scope.totalText, CONTEXT_NAMESPACE, 'total-text'),
                discountText: UtilFactory.getLocalizedStr(scope.discountText, CONTEXT_NAMESPACE, 'discount-text'),
                emptyBundleText: UtilFactory.getLocalizedStr(scope.emptyBundleText, CONTEXT_NAMESPACE, 'empty-bundle-text')
            };
        }
        return {
            restrict: 'E',
            link: originStoreBundleCartLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/bundle/views/cart.html'),
            controller: 'OriginStoreBundleCartCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBundleCart
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Store bundle cart: collection of items currently in the bundle
     *
     * @example
     * <origin-store-row>
     *     <origin-store-bundle-cart></origin-store-bundle-cart>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('OriginStoreBundleCartCtrl', OriginStoreBundleCartCtrl)
        .directive('originStoreBundleCart', originStoreBundleCart);
}());

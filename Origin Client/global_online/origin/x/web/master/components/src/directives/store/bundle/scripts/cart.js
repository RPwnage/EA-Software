/**
 * @file store/bundle/cart.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-bundle';

    function OriginStoreBundleCartCtrl($scope, UtilFactory, StoreBundleFactory, AuthFactory, DialogFactory) {
        $scope.strings = {
            yourBundleText: UtilFactory.getLocalizedStr($scope.yourBundleText, CONTEXT_NAMESPACE, 'yourbundle'),
            buyNowText: UtilFactory.getLocalizedStr($scope.buyNowText, CONTEXT_NAMESPACE, 'buynow'),
            subtotalText: UtilFactory.getLocalizedStr($scope.subtotalText, CONTEXT_NAMESPACE, 'subtotal'),
            totalText: UtilFactory.getLocalizedStr($scope.totalText, CONTEXT_NAMESPACE, 'total'),
            discountText: UtilFactory.getLocalizedStr($scope.discountText, CONTEXT_NAMESPACE, 'discount')
        };

        // @todo remove mock data
        $scope.getSubtotal = function () {
            return '$59.99';
        };

        $scope.getTotal = function () {
            return '$40.20';
        };

        $scope.getDiscount = function () {
            return 'Save 33%';
        };
        // @todo end of mocks

        /**
         * Returns the items in the cart
         * @return {Object}
         */
        $scope.getMyItems = function () {
            return $scope.bundle;
        };

        /**
         * Returns the number of items in the cart
         * @return {integer}
         */
        $scope.countMyItems = function () {
            return $scope.getMyItems().length;
        };

        StoreBundleFactory.getAll($scope.bundleId).attachToScope($scope, 'bundle');

        // Handle click on Buy Now button.  Forces login if not already authed.
        $scope.buyNow = function () {
            AuthFactory.events.fire('auth:loginWithCallback', function(){
                DialogFactory.openAlert({
                    id: 'web-bundle-purchase-flow-warning',
                    title: 'Purchase Bundle',
                    description: 'When the cart service is ready, you will be able to perform this action.',
                    rejectText: 'OK'
                });
            });
        };
    }

    function originStoreBundleCart(ComponentsConfigFactory) {
        function originStoreBundleCartLink() {

        }

        return {
            restrict: 'E',
            scope: {
                bundleId: '@',
                yourBundleText: '@',
                buyNowText: '@',
                subtotalText: '@',
                totalText: '@',
                discountText: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/bundle/views/cart.html'),
            link: originStoreBundleCartLink,
            controller: 'OriginStoreBundleCartCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBundleCart
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} bundle-id the bundle id
     * @param {LocalizedString} your-bundle-text The 'your bundle' text override
     * @param {LocalizedString} buy-now-text The 'buy now' text override
     * @param {LocalizedString} subtotal-text The 'subtotal' text override
     * @param {LocalizedString} total-text The 'total' text override
     * @param {LocalizedString} discount-text The 'discount' text override
     * @description
     *
     * Store bundle cart: collection of items currently in the bundle
     *
     * @example
     * <origin-store-row>
     *     <origin-store-bundle-cart bundle-id="{{ activeBundleId }}">
     *     </origin-store-bundle-cart>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('OriginStoreBundleCartCtrl', OriginStoreBundleCartCtrl)
        .directive('originStoreBundleCart', originStoreBundleCart);
}());

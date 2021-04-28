/**
 * @file store/bundle/cart-item.js
 */
(function(){
    'use strict';

    function OriginStoreBundleCartItemCtrl($scope, ProductFactory, StoreBundleFactory, AppCommFactory) {
        /**
         * Removes current item from the currently active bundle
         * @return {void}
         */
        $scope.removeFromBundle = function () {
            StoreBundleFactory.remove($scope.offerId);
            window.requestAnimationFrame(function(){
                AppCommFactory.events.fire('carousel:resetup');
            });
        };

        var removeWatcher = $scope.$watch('offerId', function (newValue) {
            if (newValue) {
                ProductFactory.get(newValue).attachToScope($scope, 'model');
                removeWatcher();
            }
        });
    }

    function originStoreBundleCartItem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/bundle/views/cart-item.html'),
            controller: 'OriginStoreBundleCartItemCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBundleCartItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {OfferId} offer-id the offer id
     * @description
     *
     * Store bundle item
     *
     * @example
     * <origin-store-bundle-cart>
     *     <origin-store-bundle-cart-item offer-id="{{ offerId }}"></origin-store-bundle-cart-item>
     * </origin-store-bundle-cart>
     */
    angular.module('origin-components')
        .controller('OriginStoreBundleCartItemCtrl', OriginStoreBundleCartItemCtrl)
        .directive('originStoreBundleCartItem', originStoreBundleCartItem);
}());

/**
 * @file productinfo.js
 */
(function() {
    'use strict';

    function ProductInfoHelper(DateHelperFactory, GameRefiner) {
        var isInThePast = DateHelperFactory.isInThePast;
        /**
         * Returns true if the given product is free
         * Note - if offer is oth, it's price is 0, so this logic includes oth
         * @param {Object} product product data as a plain JS object
         * @return {boolean}
         */
        function isFree(product) {
            return product && (GameRefiner.isAlphaTrialDemoOrBeta(product) || product.price === 0);
        }

        function isReleased(product){
            return isInThePast(product.releaseDate);
        }

        /**
         * [isDLC description]
         * @param  {[type]}  product [description]
         * @return {Boolean}         [description]
         */
        function isDLC(product) { // Should probably be in the product
            return product.mdmItemType === 'Expansion Pack' || product.itemType === 'Extra Content';
        }

        /**
         * Returns true if the current user can purchase the given product
         * @param {Object} product product data as a plain JS object
         * @return {boolean}
         */
        function isPurchasable(product) {
            return product.purchasable && (!product.isOwned || product.vaultEntitlement) && !isFree(product);
        }

        /**
         * Returns true if current product can be subscribed
         * @param {Object} product product data as a plain JS object
         * @return {boolean}
         */
        function isSubscriptionAvailable(product) {
            return product.subscriptionAvailable;
        }

        /**
         * Returns true if current product is installed
         * @param {Object} product product data as a plain JS object
         * @return {boolean}
         */
        function isInstalled(product) {
            return product.isInstalled;
        }

        /**
         * Returns true if current product can be downloaded
         * @param {Object} product product data as a plain JS object
         * @return {boolean}
         */
        function isDownloadable(product) {
            return isInThePast(product.releaseDate);
        }

        /**
         * Returns true if current product can be preloaded
         * @param {Object} product product data as a plain JS object
         * @return {boolean}
         */
        function isPreloadable(product) {
            return isInThePast(product.downloadStartDate) || isInThePast(product.releaseDate);
        }

        /**
         * Returns true if current product can be preordered
         * @param {Object} product product data as a plain JS object
         * @return {boolean}
         */
        function isPreorderable(product) {
            return !isInThePast(product.releaseDate);
        }

        return {
            isFree: isFree,
            isReleased: isReleased,
            isPurchasable: isPurchasable,
            isSubscriptionAvailable: isSubscriptionAvailable,
            isInstalled: isInstalled,
            isDownloadable: isDownloadable,
            isPreloadable: isPreloadable,
            isPreorderable: isPreorderable,
            isDLC: isDLC
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ProductInfoHelper
     * @description  Implements purchase related actions
     */
    angular.module('origin-components')
        .factory('ProductInfoHelper', ProductInfoHelper);
}());

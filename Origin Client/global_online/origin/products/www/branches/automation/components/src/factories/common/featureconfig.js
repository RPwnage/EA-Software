/**
 * @file factories/common/featureconfig.js
 */
(function() {
    'use strict';

    var GIFT_CONFIG_KEY = 'gifting';
    var WISHLIST_CONFIG_KEY = 'wishlist';
    var SUBSCRIPTIONS_CONFIG_KEY = 'subscriptions';


    function FeatureConfig(COMPONENTSCONFIG) {

        function isEnabled(key) {
            var currentCountryCode,
                enabled = null;

            if(Origin.client.isEmbeddedBrowser()) {
                if(key === GIFT_CONFIG_KEY) {
                    enabled = Origin.client.info.isGiftingEnabled();
                } else if(key === WISHLIST_CONFIG_KEY) {
                    enabled = Origin.client.info.isWishlistEnabled();
                }

                // null means that the client call to get the enabled state did not succeed
                // due to the function not being available
                if(enabled !== null) {
                    return enabled;
                }
                // else fallback to the configuration on the SPA
            }

            currentCountryCode = Origin.locale.threeLetterCountryCode().toUpperCase();
            return angular.isUndefined(_.get(COMPONENTSCONFIG, [key, 'disabledCountries', currentCountryCode]));
        }

        /**
         * Check if Gifting feature is enabled/disabled for user's storefront
         * @returns {Boolean}
         */
        function isGiftingEnabled() {
            return isEnabled(GIFT_CONFIG_KEY);
        }

        /**
         * Check if Wishlist feature is enabled/disabled for user's storefront
         * @returns {Boolean}
         */
        function isWishlistEnabled() {
            return isEnabled(WISHLIST_CONFIG_KEY);
        }

        /**
         * Check if Subscription feature is enabled/disabled for user's storefront
         * @returns {Boolean}
         */
        function isOriginAccessEnabled() {
            return isEnabled(SUBSCRIPTIONS_CONFIG_KEY);
        }

        return {
            isGiftingEnabled: isGiftingEnabled,
            isWishlistEnabled: isWishlistEnabled,
            isOriginAccessEnabled: isOriginAccessEnabled
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GiftingFactory

     * @description
     *
     * Store product factory
     */
    angular.module('origin-components')
        .factory('FeatureConfig', FeatureConfig);

}());

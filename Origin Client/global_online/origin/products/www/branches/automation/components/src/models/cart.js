/* Cart
 *
 * manages the cart flow
 * @file models/cart.js
 */
(function() {
    'use strict';

    function CartModel(LocaleFactory, ComponentsConfigHelper) {
        var CART_NAME = ComponentsConfigHelper.getCartName();
        var BUNDLE_TYPE = 'NONE_BUNDLE';
        var BUNDLE_PROMOTION_RULE_ID = '';
        var STORE_ID = ComponentsConfigHelper.getSetting('storeId'); // origin-store
        var COUNTRY_CODE = Origin.locale.countryCode();
        var CURRENCY = LocaleFactory.getCurrency(COUNTRY_CODE);
        var NEED_CLEAR_CART = true;
        var CART_ERROR_NO_COUPON = '[Cart Model] No couponCode!';

        /**
         * Retrieve cart response
         * @param  {string} cartName name of cart
         * @return {Promise} A cart response Promise
         */
        function getCart(cartName, currency) {
            var data = {
                cartName: cartName || CART_NAME,
                currency: currency || CURRENCY,
                storeId: STORE_ID,
                countryCode: COUNTRY_CODE
            };
            return Origin.cart.getCart(data);
        }

        /**
         * Clear cart
         * @param  {string}  cartName name of cart
         * @return {Promise} A cart response Promise (will be empty string as
         *                   it returns no response body)
         */
        function clearCart(cartName) {
            var data = {
                cartName: cartName || CART_NAME,
                countryCode: COUNTRY_CODE
            };
            return Origin.cart.clearCart(data);
        }

        /**
         * Helper for setting a single property in the cart
         * @param {string} name      property name
         * @param {string} value     property value
         * @param {string} namespace property namespace
         * @param {string} cartName  name of cart
         */
        function setCartProperty(name, value, namespace, cartName) {
            var property = {};
            property[name] = value;
            return setCartProperties(property, namespace, cartName);
        }

        /**
         * @typedef propObj
         * @type {object}
         * @property {string} name property name
         * @property {string} value property value
         */

        /**
         * Add properties to cart
         * @param {Object} propData key/value pairs of properties to add
         * @param {string} cartName  name of cart
         * @param {string} namespace property namespace to apply to properties
         */
        function setCartProperties(propData, cartName, namespace) {
            var property = [];
            var keys = Object.keys(propData || {});

            for (var i = 0; i < keys.length; i++) {
                property.push({
                    namespace: namespace || 'prop',
                    name: keys[i],
                    value: propData[keys[i]]
                });
            }

            var data = {
                cartName: cartName || CART_NAME,
                property: property
            };

            return Origin.cart.setCartProperties(data);
        }


        /**
         * Builds cart params
         * @param {Array|string} offerIdList
         * @param params
         * @returns {Object}
         */
        function buildCartParams(offerIdList, params) {
            if(!offerIdList) {
                throw new Error('[Cart Model] No offerId!');
            }

            if (!_.isArray(offerIdList)) {
                offerIdList = [offerIdList];
            }

            params = params || {};

            if (params.needClearCart !== false) {
                params.needClearCart = true;
            }

            return {
                offerIdList: offerIdList,
                bundleType: params.bundleType || BUNDLE_TYPE,
                bundlePromotionRuleId: params.bundlePromotionRuleId || BUNDLE_PROMOTION_RULE_ID,
                cartName: CART_NAME,
                currency: params.currency || CURRENCY,
                storeId: STORE_ID,
                needClearCart: params.needClearCart,
                countryCode: COUNTRY_CODE
            };
        }

        /**
         * @typedef cartParams
         * @type {object}
         * @property {boolean} needClearCart        Should the cart be cleared or not.
         * @property {string} currency              The currency code
         * @property {string} bundleType            The type of bundle
         * @property {string} bundlePromotionRuleId The promo rule
         * @property {boolean} useChainedCheckout   If true, the chained checkout cart will be used
         */

        /**
         * Add offers to a cart. May be an Anonymous Cart or a Users cart
         * @param {string[]} offerIdList         List of offers to add to the cart
         * @param {Object} params            Parameters that determine cart behavior
         * @return {Promise} A Cart Promise
         * @throws {Error} If No Offer Id is provided
         * @throws {Error} If Offer Id list isn't an array
         */
        function addOfferList(offerIdList, params) {
            return Origin.cart.addOffer(buildCartParams(offerIdList, params));
        }

        /**
         * Add an offer to a cart. May be an Anonymous Cart or a Users cart
         * @param {string} offerId               The offer to add to the cart
         * @param {Object} params            Parameters that determine behavior of the cart.
         * @return {Promise} A cart response Promise
         */
        function addOffer(offerId, params) {
            return addOfferList([offerId], params);
        }

        /**
         * Add an offer to a cart as well as gift related params. May be an Anonymous Cart or a Users cart
         * @param {string} offerId
         * @param {Number} recipientId
         * @param {string} senderName
         * @param {string} message
         * @param {Object} giftParams
         * @returns {Promise} A cart response Promise
         */
        function addGiftOffer(offerId, recipientId, senderName, message, giftParams) {
            var data = buildCartParams([offerId], giftParams);
            var properties = {
                recipientId: recipientId,
                senderName: senderName,
                message: message
            };

            return Origin.cart.addOffer(data)
                .then(function() {
                    return setCartProperties(properties, data.cartName);
                });

        }

        /**
         * Add a coupon to the users cart
         * @param {string} couponCode The code for the coupon
         * @param {string} cartName   The name of the cart normally one of the provided constants
         * @return {Promise} A Cart Promise
         */
        function addCoupon(couponCode, cartName, currency) {
            if(!couponCode) {
                return Promise.reject(CART_ERROR_NO_COUPON);
            }

            var data = {
                couponCode: couponCode,
                cartName: cartName || CART_NAME,
                currency: currency || CURRENCY,
                storeId: STORE_ID,
                countryCode: COUNTRY_CODE
            };

            return Origin.cart.addCoupon(data);
        }

        /**
         * Remove a offer from the cart by offer entry ID
         * @param {string} offerEntryId The offer to remove
         * @param {string} cartName     The name of the cart normally one of the provided constants
         * @return {Promise} A cart promise
         */
        function removeOfferByOfferEntryId(offerEntryId, cartName, currency) {
            var data = {
                offerEntryId: offerEntryId,
                cartName: cartName || CART_NAME,
                currency: currency || CURRENCY,
                storeId: STORE_ID,
                countryCode: COUNTRY_CODE
            };

            return Origin.cart.removeOffer(data);
        }

        /**
         * Remove a offer from the cart by offer code
         * @param {string} offerId  The offer to remove
         * @param {string} cartName The name of the cart normally one of the provided constants
         * @return {Promise} A Cart Promise
         */
        function removeOfferByOfferId(offerId, cartName, currency) {
            cartName = cartName || CART_NAME;
            currency = currency || CURRENCY;

            return getCart(cartName, currency)
                .then(function(cartResponse){
                    var offerEntries = cartResponse.offerEntry || [],
                        offer = _.filter(offerEntries, {offerId: offerId});

                    if (offer.length > 0 && offer[0].hasOwnProperty('entryId')) {
                        return {cartResponse: cartResponse, offerEntryId: offer[0].entryId};
                    }

                    return {cartResponse: cartResponse, offerEntryId: false};
                })
                .then(function(response){
                    if (response.offerEntryId) {
                        return removeOfferByOfferEntryId(response.offerEntryId, cartName, currency);
                    }

                    return response.cartResponse;
                });
        }

        /**
         * Remove a coupon from the cart by coupon entry ID
         * @param {string} couponId     The coupon to remove
         * @param {string} cartName     The name of the cart normally one of the provided constants
         * @return {Promise} A cart promise
         */
        function removeCouponByEntryId(couponEntryId, cartName, currency) {
            var data = {
                couponEntryId: couponEntryId,
                cartName: cartName || CART_NAME,
                currency: currency || CURRENCY,
                storeId: STORE_ID,
                countryCode: COUNTRY_CODE
            };

            return Origin.cart.removeCoupon(data);
        }

        /**
         * Remove a coupon from the cart by coupon code
         * @param {string} couponCode The coupon to remove
         * @param {string} cartName   The name of the cart normally one of the provided constants
         * @return {Promise} A cart promise
         */
        function removeCouponByCode(couponCode, cartName, currency) {
            cartName = cartName || CART_NAME;
            currency = currency || CURRENCY;

            return getCart(cartName, currency)
                .then(function(cartResponse){
                    var couponEntries = cartResponse.couponEntry || [],
                        coupon = _.filter(couponEntries, {couponCode: couponCode});

                    if (coupon.length > 0 && coupon[0].hasOwnProperty('entryId')) {
                        return {cartResponse: cartResponse, couponEntryId: coupon[0].entryId};
                    }

                    return {cartResponse: cartResponse, couponEntryId: false};
                })
                .then(function(response){
                    if (response.couponEntryId) {
                        return removeCouponByEntryId(response.couponEntryId, cartName, currency);
                    }

                    return response.cartResponse;
                });
        }

        return {
            addOffer: addOffer,
            addGiftOffer: addGiftOffer,
            addOfferList: addOfferList,
            removeOfferByOfferId: removeOfferByOfferId,
            removeOfferByOfferEntryId: removeOfferByOfferEntryId,
            addCoupon: addCoupon,
            removeCouponByCode: removeCouponByCode,
            removeCouponByEntryId: removeCouponByEntryId,
            getCart: getCart,
            clearCart: clearCart,
            setCartProperty: setCartProperty,
            setCartProperties: setCartProperties,

            STANDARD_CART: CART_NAME,
            NEED_CLEAR_CART: NEED_CLEAR_CART
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function CartModelSingleton(LocaleFactory, ComponentsConfigHelper, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('CartModel', CartModel, this, arguments);
    }
    /**
     * @ngdoc service
     * @name originApp.factories.Cart
     * @requires $http
     * @requires origin-components.factories.AppCommFactory
     * @description
     *
     * Cart factory
     */
    angular.module('origin-components')
        .factory('CartModel', CartModelSingleton);
}());

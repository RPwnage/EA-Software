define([
    'promise',
    'core/auth',
    'core/logger',
    'core/user',
    'core/urls',
    'core/dataManager',
    'core/errorhandler',
    'core/anonymoustoken'
], function(Promise, auth, logger, user, urls, dataManager, errorhandler, anonymousToken) {
    /**
     * @module module:cart
     * @memberof module:Origin
     */
    'use strict';
    var CLIENT_ID = 'ORIGIN_JS_SDK',
        CLIENT_SECRET = '68BEdbSaTQEQba8DvV2RGbNl9KRrY5stIN1IDVjPwm1ycRea3u9nlKCcuHD1uQybjQK7s2j4M3E7V1J8',
        ACCEPT_TYPE = 'application/json',
        REQUESTOR_ID = 'Ebisu-Platform',
        subReqNum = 0;

    // expose dataManager functions
    var addParameter = dataManager.addParameter,
        appendParameter = dataManager.appendParameter,
        addHeader = dataManager.addHeader,
        addAuthHint = dataManager.addAuthHint,
        enQueue = dataManager.enQueue,
        validateDataObject = dataManager.validateDataObject;

    /**
     * Object contract for passing offer data to addOffer
     *
     * required:
     * - offerIdList {Array}            array of offer IDs
     * - bundleType {String}            bundle type
     * - bundlePromotionRuleId {String} bundle promotion ID
     * - cartName {String}              cart name
     * - storeId {String}               store ID
     * - currency {String}              cart currency
     *
     * optional:
     * - needClearCart {String}         clear cart before adding offer (true/false)
     */
    var addOfferDataContract = {
        required: ['offerIdList', 'bundleType', 'bundlePromotionRuleId', 'cartName', 'storeId', 'currency', 'countryCode'],
        optional: ['needClearCart']
    };
    /**
     * Add offer to cart
     * @param  {Object} data data object defined by addOfferDataContract
     * @return {Object}             Cart response
     */
    function addOffer(data) {
        return getAuthorization().then(function(token) {
            var authHint = buildAuthHint('Authorization', 'Bearer {token}'),
                addHeaders = {
                    Authorization: token,
                    'client_secret': CLIENT_SECRET
                },
                addParameters = {},
                appendParameters = {
                    storeId: data.storeId,
                    currencyCode: data.currency,
                    needFullCartInfo: 'true',
                    needClearCart: data.needClearCart,
                    countryCode: data.countryCode
                },
                requestBody = {},
                offerList = [];

            for(var i=data.offerIdList.length-1; i >= 0;i--) {
                offerList.push({
                    offerId: data.offerIdList[i],
                    quantity: 1
                });
            }

            requestBody.offers = {offer: offerList};
            requestBody.bundleType = data.bundleType || '';
            requestBody.bundlePromotionRuleId = data.bundlePromotionRuleId || '';

            return makeRequest('addOffer', 'POST', urls.endPoints.cartAddOffer, data, addOfferDataContract, addHeaders, addParameters, appendParameters, authHint, requestBody);
        });
    }

    /**
     * Object contract for passing data to removeOffer
     *
     * required:
     * - offerEntryID {String}  offer entry ID to remove from cart
     * - cartName {String}      cart name
     * - storeId {String}       store ID
     */
    var removeOfferDataContract = {
        required: ['offerEntryId', 'cartName', 'storeId', 'countryCode']
    };
    /**
     * Remove offer from cart
     * @param  {Object} data data object defined by removeOfferDataContract
     * @return {Object}             Cart response
     */
    function removeOffer(data) {
        return getAuthorization().then(function(token) {
            var authHint = buildAuthHint('Authorization', 'Bearer {token}'),
                addHeaders = {
                    Authorization: token,
                    'client_secret': CLIENT_SECRET
                },
                addParameters = {offerEntryId: data.offerEntryId},
                appendParameters = {
                    storeId: data.storeId,
                    needFullCartInfo: 'true',
                    countryCode: data.countryCode
                };

            return makeRequest('removeOffer', 'DELETE', urls.endPoints.cartRemoveOffer, data, removeOfferDataContract, addHeaders, addParameters, appendParameters, authHint);
        });
    }

    /**
     * Object contract for passing data to addCoupon
     *
     * required:
     * - cartName {String}      cart name
     * - storeId {String}       store ID
     * - couponCode {String}    coupon code
     */
    var addCouponDataContract = {
        required: ['couponCode', 'cartName', 'storeId', 'countryCode']
    };
    /**
     * Add coupon to cart
     * @param  {dataContractObject} data data object defined by addCouponDataContract
     * @return {Object}             Cart response
     */
    function addCoupon(data) {
        return getAuthorization().then(function(token) {
            var authHint = buildAuthHint('Authorization', 'Bearer {token}'),
                addHeaders = {
                    Authorization: token,
                    'client_secret': CLIENT_SECRET
                },
                addParameters = {},
                appendParameters = {
                    storeId: data.storeId,
                    currencyCode: data.currency,
                    needFullCartInfo: 'true',
                    countryCode: data.countryCode
                },
                requestBody = {couponCode: data.couponCode};

            return makeRequest('addCoupon', 'POST', urls.endPoints.cartAddCoupon, data, addCouponDataContract, addHeaders, addParameters, appendParameters, authHint, requestBody);
        });
    }

    /**
     * Object contract for passing data to removeCoupon
     *
     * required:
     * - couponEntryID {String}  coupon entry ID to remove from cart
     * - cartName {String}       cart name
     * - storeId {String}        store ID
     * - currency {String}       cart currency
     */
    var removeCouponDataContract = {
        required: ['couponEntryId', 'cartName', 'storeId', 'currency', 'countryCode']
    };
    /**
     * Remove coupon from cart
     * @param  {dataContractObject} data data object defined by removeCouponDataContract
     * @return {Object}             Cart response
     */
    function removeCoupon(data) {
        return getAuthorization().then(function(token) {
            var authHint = buildAuthHint('Authorization', 'Bearer {token}'),
                addHeaders = {
                    Authorization: token,
                    'client_secret': CLIENT_SECRET
                },
                addParameters = {couponEntryId: data.couponEntryId},
                appendParameters = {
                    storeId: data.storeId,
                    needFullCartInfo: 'true',
                    countryCode: data.countryCode
                };

            return makeRequest('removeCoupon', 'DELETE', urls.endPoints.cartRemoveCoupon, data, removeCouponDataContract, addHeaders, addParameters, appendParameters, authHint);
        });
    }

    /**
     * Object contract for passing data to getCart
     *
     * required:
     * - cartName {String}      cart name
     * - storeId {String}       store ID
     * - currency {String}      cart currency
     */
    var getCartDataContract = {
        required: ['cartName', 'storeId', 'currency', 'countryCode']
    };
    /**
     * Get cart
     * @param  {dataContractObject} data data object defined by getCartDataContract
     * @return {Object}             Cart response
     */
    function getCart(data) {
        return getAuthorization().then(function(token) {
            var authHint = buildAuthHint('Authorization', 'Bearer {token}'),
                addHeaders = {
                Authorization: token,
                'client_secret': CLIENT_SECRET
            },
            addParameters = {},
            appendParameters = {
                storeId: data.storeId,
                currency: data.currency,
                needFullCartInfo: 'true',
                countryCode: data.countryCode
            };
            return makeRequest('getCart', 'GET', urls.endPoints.cartGetCart, data, getCartDataContract, addHeaders, addParameters, appendParameters, authHint);
        });

    }

    /**
     * Object contract for passing data to mergeCart
     *
     * required:
     * - anonymousToken {String} raw token of initial anonymous cart
     * - cartName {String}       cart name
     * - sourcePidId {String}    pidId of initial anonymous cart
     */
    var mergeCartDataContract = {
        required: ['cartName', 'sourcePidId', 'anonymousToken', 'countryCode']
    };
    /**
     * Merge carts
     * @param  {dataContractObject} data data object defined by mergeCartDataContract
     * @return {Object}             Cart response
     */
    function mergeCart(data) {
        return getAuthorization().then(function(token) {
            var authHint = buildAuthHint('Authorization', 'Bearer {token}'),
                addHeaders = {
                    Authorization: token,
                    'anonymous_token': data.anonymousToken,
                    'client_secret': CLIENT_SECRET
                },
                addParameters = {},
                appendParameters = {
                    sourcePidId: data.sourcePidId,
                    action: 'merge',
                    needFullCartInfo: 'true',
                    countryCode: data.countryCode
                };

            return makeRequest('mergeCart', 'POST', urls.endPoints.cartOperation, data, mergeCartDataContract, addHeaders, addParameters, appendParameters, authHint);
        });
    }

    /**
     * Object contract for passing data to clearCart
     *
     * required:
     * - cartName {String}        cart name
     */
    var clearCartDataContract = {
        required: ['cartName', 'countryCode']
    };
    /**
     * Clear cart
     * @param  {dataContractObject} data data object defined by clearCartDataContract
     * @return {Object}             Cart response
     */
    function clearCart(data) {
        return getAuthorization().then(function(token) {
            var authHint = buildAuthHint('Authorization', 'Bearer {token}'),
                addHeaders = {
                    Authorization: token,
                    'client_secret': CLIENT_SECRET
                },
                addParameters = {},
                appendParameters = {
                    action: 'close',
                    needFullCartInfo: 'true',
                    countryCode: data.countryCode
                };

            return makeRequest('clearCart', 'POST', urls.endPoints.cartOperation, data, clearCartDataContract, addHeaders, addParameters, appendParameters, authHint);
        });
    }


    /**
     * Object contract for setCartProperties
     *
     * required:
     * - cartName {String}  cart name
     * - property {Array} array of properties objects
     */
    var setCartPropertiesDataContract = {
        required: ['cartName', 'property']
    };
    /**
     * Set Cart Properties
     * @param  {dataContractObject} data data object defined by setCartPropertiesDataContract
     * @return {Object}             Cart response
     */
    function setCartProperties(data) {
        return getAuthorization().then(function(token) {
            var authHint = buildAuthHint('Authorization', 'Bearer {token}'),
                addHeaders = {
                    Authorization: token,
                    'client_secret': CLIENT_SECRET
                },
                addParameters = {},
                appendParameters = {
                    needFullCartInfo: 'true'
                },
                requestBody = {};

            requestBody.property = data.property || {};

            return makeRequest('setCartProperties', 'PUT', urls.endPoints.cartPutProperties, data, setCartPropertiesDataContract, addHeaders, addParameters, appendParameters, authHint, requestBody);
        });
    }

    // private

    /**
     * If the user is logged in returns their access token otherwise returns a anonymous access token.
     * @return {string} The Access token
     */
    function getAuthorization(){
        return new Promise(function(resolve) {
            var authorization = user.publicObjs.accessToken();
            if(authorization){
                resolve('Bearer '  + authorization);
            } else {
                anonymousToken.getAnonymousToken().then(function(token) {
                    resolve('Anon ' + token.getId());
                });
            }
        });
    }

    /**
     * Performs service request
     * @param  {String} requestName      Type of request to make
     * @param  {String} method           Http method - POST, GET, etc
     * @param  {String} endPoint         Service endpoint
     * @param  {Object} data             data object defined by dataContract
     * @param  {Object} dataContract     data contract object
     * @param  {Object} addHeaders       headers to add to request
     * @param  {Object} addParameters    replacement parameters to add to request
     * @param  {Object} appendParameters query string parameters to add to request
     * @param  {Object} authHint         Auth token hinting for use during re-auth/re-try
     * @param  {Object} requestBody      request body object (optional)
     * @return {Object}                  Cart response
     */
    function makeRequest(requestName, method, endPoint, data, dataContract, addHeaders, addParameters, appendParameters, authHint, requestBody) {
        data = validateDataObject(dataContract, data);
        if (data === false) {
            return errorhandler.logAndCleanup('CART:'+requestName+' VALIDATION FAILED');
        }

        var config = initCartConfig(method, data.cartName, data.storeId, data.currency),
            prop;

        addHeaders = addHeaders || {};
        addParameters = addParameters || {};
        appendParameters = appendParameters || {};
        requestBody = requestBody || {};

        for (prop in addHeaders) {
            if(addHeaders.hasOwnProperty(prop)) {
                addHeader(config, prop, addHeaders[prop]);
            }
        }

        for (prop in addParameters) {
            if(addParameters.hasOwnProperty(prop)) {
                addParameter(config, prop, addParameters[prop]);
            }
        }

        for (prop in appendParameters) {
            if(appendParameters.hasOwnProperty(prop)) {
                appendParameter(config, prop, appendParameters[prop]);
            }
        }

        if (addHeaders.hasOwnProperty('Authorization') && addHeaders.Authorization.substring(0, 4) === 'Anon') {
            // this parameter will be removed eventually when the requirement is removed
            // in favor of relying on the Authorizaation header to determine if a request
            // is authenticated or not.
            appendParameter(config, 'isAnonymous', 'true');
            config.reqauth = false;
            config.requser = false;
        }

        config.body = JSON.stringify(requestBody);

        if (authHint && authHint.hasOwnProperty('property') && authHint.hasOwnProperty('format')) {
            addAuthHint(config, authHint.property, authHint.format);
        }

        return enQueue(endPoint, config, subReqNum)
            .then(function(response){
                if (response && response.hasOwnProperty ('cartInfo')) {
                    return response.cartInfo;
                }
                return response;
            })
            .catch(errorhandler.logAndCleanup('CART:'+requestName+' FAILED'));
    }

    /**
     * Initialize request config object
     * @param  {String} type          Http request method (GET, POST, etc)
     * @param  {String cartName       Cart name
     * @param  {String} storeId       Store ID
     * @param  {String} currency      Currency code (CAD, USD, etc)
     * @param  {Boolean} requiresAuth Requires auth
     * @param  {Boolean} requiresUser Requires user
     * @return {Object}               Service request config object
     */
    function initCartConfig(type, cartName, storeId, currency, requiresAuth, requiresUser) {
        requiresAuth = requiresAuth || true;
        requiresUser = requiresUser || true;

        var config = {
                atype: type,
                headers: [{
                    'label': 'accept',
                    'val' : ACCEPT_TYPE
                }, {
                    'label': 'Nucleus-RequestorId',
                    'val': REQUESTOR_ID
                }, {
                    'label': 'X-CART-REQUESTORID',
                    'val': REQUESTOR_ID
                }, {
                    'label': 'client_id',
                    'val': CLIENT_ID
                }],
                parameters: [{
                    'label': 'cartName',
                    'val': cartName
                }, {
                    'label': 'storeId',
                    'val': storeId
                }, {
                    'label': 'currency',
                    'val': currency
                }],

                reqauth: requiresAuth,
                requser: requiresUser
            };

        return config;
    }

    /**
     * Build authHint object
     * @param  {string} property Authentication header name
     * @param  {string} format   Authentication header value format - use {token} as placeholder.
     * @return {object}          authHint object
     */
    function buildAuthHint(property, format) {
        return {property: property, format: format};
    }

    return /** @lends module:Origin.module:cart */{

        /**
         * This is the cart response object.
         *
         * @typedef cartResponse
         * @type {object}
         * @property {object} shippingInfo physical shipping info
         * @property {number} totalNumberOfItems Number of items in the cart
         * @property {string} currency Currency of cart
         * @property {float} totalPriceWithoutTax Total price of cart without tax
         * @property {float} totalDiscountRate Rate of discount
         * @property {float} totalDiscountAmount Dollar value of discount
         * @property {number} checkoutToken Checkout token
         * @property {number} pidId User pidId of cart owner
         * @property {string} cartName Name of cart
         * @property {string} storeId Store ID
         * @property {number} updatedDateTime Update time in seconds
         * @property {number} createdDateTime Created time in seconds
         * @property {object[]} offerEntry List of offer entry objects
         * @property {object[]} discountLineItem List of line item discount objects
         * @property {object[]} couponEntry List of coupon entry objects
         *
         */

        /**
         * This is a data contract object used to ensure that the data that is
         * passed to the method is valid.
         *
         * @typedef dataContractObject
         * @type {object}
         * @property {string[]} required a list of required object properties
         * @property {string[]} optional a list of optional object properties
         */

        /**
         * Add offer to cart<br>
         * <br>
         * Object contract for passing offer data to addOffer:<br>
         * <br>
         * Required:<br>
         * - offerIdList {Array}            array of offer IDs<br>
         * - bundleType {String}            bundle type<br>
         * - bundlePromotionRuleId {String} bundle promotion ID<br>
         * - cartName {String}              cart name<br>
         * - storeId {String}               store ID<br>
         * - currency {String}              cart currency<br>
         * <br>
         * Optional:<br>
         * - needClearCart {String}         clear cart before adding offer (true/false)<br><br>
         *
         * @param {dataContractObject} data data object defined by addOfferDataContract
         * @return {promise<module:Origin.module:cart~cartResponse>} responsename response from cart service
         * @method
         */
        addOffer: addOffer,

        /**
         * Remove offer from cart<br>
         * <br>
         * Object contract for passing data to removeOffer:<br>
         * <br>
         * Required:<br>
         * - cartName {String}      cart name<br>
         * - storeId {String}       store ID<br>
         * - currency {String}      cart currency<br>
         * - offerEntryID {String}  offer Entry ID to remove from cart<br><br>
         *
         * @param {dataContractObject} data data object defined by removeOfferDataContract
         * @return {promise<module:Origin.module:cart~cartResponse>} responsename response from cart service
         * @method
         */
        removeOffer: removeOffer,

        /**
         * Add coupon to cart<br>
         *<br>
         * Object contract for passing data to addCoupon:<br>
         *<br>
         * Required:<br>
         * - cartName {String}      cart name<br>
         * - storeId {String}       store ID<br>
         * - couponCode {String}    coupon code<br><br>
         *
         * @param {dataContractObject} data data object defined by addCouponDataContract
         * @return {promise<module:Origin.module:cart~cartResponse>} responsename response from cart service
         * @method
         */
        addCoupon: addCoupon,

        /**
         * Remove coupon from cart<br>
         * <br>
         * Object contract for passing data to removeCoupon:<br>
         *<br>
         * Required:<br>
         * - couponEntryID {String}  offer Entry ID to remove from cart<br>
         * - cartName {String}       cart name<br>
         * - storeId {String}        store ID<br>
         * - currency {String}       cart currency<br><br>
         *
         * @param {dataContractObject} data data object defined by removeCouponDataContract
         * @return {promise<module:Origin.module:cart~cartResponse>} responsename response from cart service
         * @method
         */
        removeCoupon: removeCoupon,

        /**
         * Get cart<br>
         *<br>
         * Object contract for passing data to getCart:<br>
         *<br>
         * Required:<br>
         * - cartName {String}      cart name<br>
         * - storeId {String}       store ID<br>
         * - currency {String}      cart currency<br><br>
         *
         * @param {dataContractObject} data data object defined by getCartDataContract
         * @return {promise<module:Origin.module:cart~cartResponse>} responsename response from cart service
         * @method
         */
        getCart: getCart,

        /**
         * Merge anonymous cart with authenticated cart<br>
         *<br>
         * Object contract for passing data to mergeCart:<br>
         *<br>
         * Required:<br>
         * - anonymousToken {String} raw token of initial anonymous cart<br>
         * - cartName {String}       cart name<br>
         * - sourcePidId {String}    pidId of initial anonymous cart<br><br>
         *
         * @param {dataContractObject} data data object defined by mergeCartDataContract
         * @return {promise<module:Origin.module:cart~cartResponse>} responsename response from cart service
         * @method
         */
        mergeCart: mergeCart,


        /**
         * Clear cart contents (maintains cart)<br>
         *<br>
         * Object contract for passing data to clearCart:<br>
         *<br>
         * Required:<br>
         * - cartName {String}        cart name<br><br>
         *
         * @param {dataContractObject} data data object defined by clearCartDataContract
         * @return {promise<module:Origin.module:cart~cartResponse>} responsename response from cart service
         * @method
         */
        clearCart: clearCart,

        /**
         * Add properties to cart<br>
         * <br>
         * Object contract for passing property data to setCartProperties:<br>
         * <br>
         * Required:<br>
         * - cartName {String}              cart name<br>
         * - properties {Array}             array of property objects, which include namespace, name, and value<br>
         *
         * @param {dataContractObject} data data object defined by setCartPropertiesDataContract
         * @return {promise<module:Origin.module:cart~cartResponse>} responsename response from cart service
         * @method
         */
        setCartProperties: setCartProperties
    };
});

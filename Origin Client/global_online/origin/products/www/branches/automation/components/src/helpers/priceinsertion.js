/**
 * @file helpers/priceinsertion.js
 */
(function () {
    'use strict';

    var OFFER_ID_REGEX_PATTERN = /%~([a-zA-Z0-9]+[\-\.\:]+[a-zA-Z0-9]+[\-\.\:]+[a-zA-Z0-9]*[\-\.\:]*[a-zA-Z0-9]*)~%/g;

    function PriceInsertionFactory(UtilFactory, GamesPricingFactory, CurrencyHelper, ScopeHelper) {
        /**
         * Return getLocalizedStr as Promise.
         *
         * @param {String} possibleStr - the attribute/value of the
         *   parameter that is passed down as a parameter to the
         *   directive.  This will either be the already translated
         *   string or it will be empty.
         * @param {String} namespace the dictionary entry that contains the default
         *   translations for the translation lookup in case possibleStr is not defined
         * @param {String} key - the English key for the string to
         *   translate.
         * @param {object} params - *optional* this is the optional list
         *   of parameters to substitute the string with.
         * @param {object} choice - *optional* this is the optional number value that
         *   determines if pluralization within the @possibleStr or @key
         *   needs to be resolved.
         *
         * @return {Promise} resolves to the localized string with parameters substituted
         */
        function getLocalizedStrPromise(possibleStr, namespace, key, params, choice) {
            return Promise.resolve(UtilFactory.getLocalizedStr(possibleStr, namespace, key, params, choice));
        }

        /**
         * Extract offerId from string content and return an object with the
         * original string, offerId placeholder, and offerId
         *
         * @param  {string} stringContent a content string that may or may not
         *   contain an offerId placeholder
         *
         * @return {object} data object containing origin string and offerIds
         */
        function extractOfferIdFromString(stringContent) {
            var offerIds = [],
                regexResult;

            while ((regexResult = OFFER_ID_REGEX_PATTERN.exec(stringContent)) !== null) {
                offerIds.push(regexResult[1]);
            }

            return {
                stringContent: stringContent,
                offerIds: offerIds
            };
        }

        /**
         *
         * @param {array} unformattedPriceData
         */
        function formatDiscountRate(unformattedPriceData){
            _.forEach(unformattedPriceData, function (priceObject) {
                var ratingArray = Origin.utils.getProperty(priceObject, ['rating']);
                var rating = _.isArray(ratingArray) && ratingArray[0];
                var discountRate;
                if (rating){
                    discountRate = rating.totalDiscountRate * 100; //Formatted total discount rate into percentage out of 100
                    discountRate = Math.round(discountRate * 100) / 100; //Round to second decimal place
                    priceObject.formattedDiscountRate = discountRate + '%';
                }
            });
        }

        /**
         *
         * @param {array} unformattedPriceData
         * @param {string} priceRatingField
         * @param {string} resultField
         * @returns {promise}
         */
        function formatPrice(unformattedPriceData, priceRatingField, resultField){
            var promises = [];
            _.forEach(unformattedPriceData, function (value) {
                promises.push(CurrencyHelper.formatCurrency(Origin.utils.getProperty(value, ['rating', 0, priceRatingField])));
            });

            return Promise.all(promises).then(function (prices) {
                for (var i = 0; i < unformattedPriceData.length; i++) {
                    unformattedPriceData[i][resultField]  = prices[i];
                }
                return unformattedPriceData;
            });
        }


        /**
         * format the prices returned by the rating service and attach the data to
         * the ratings object
         *
         * @param  {object} unformattedPriceData the unformatted ratings data
         *
         * @return {object} data object containing origin string, offerId, offerId
         *   placeholder, formatted price and offer price
         */
        function formatPrices(unformattedPriceData) {
            var totalPricePromise = formatPrice(unformattedPriceData, 'finalTotalAmount', 'formattedPrice');
            var totalDiscountPromise = formatPrice(unformattedPriceData, 'totalDiscountAmount', 'formattedDiscountPrice');
            formatDiscountRate(unformattedPriceData);

            return Promise.all([totalPricePromise, totalDiscountPromise]).then(_.spread(function(priceData){
                return priceData;
            }));
        }

        /**
         * From an offerId contained within a string object, get an offer
         * price and insert it into the string object, which will be returned
         *
         * @param  {object} stringObj string object containing a string, placeholder,
         *   and offerId
         *
         * @return {object} data object containing origin string, offerId, offerId
         *   placeholder, and offer price
         */
        function getPrice(stringObj) {
            if (stringObj.offerIds.length) {
                return GamesPricingFactory
                    .getPrice(stringObj.offerIds)
                    .then(formatPrices)
                    .then(function (priceData) {
                        stringObj.prices = priceData;
                        return stringObj;
                    });
            } else {
                return stringObj;
            }
        }

        /**
         * Replace offer placeholder with price
         *
         * @param  {object} stringObj string object containing a string, offerIds, and price
         *
         * @return {string} string with offer placeholder replaced by offer price
         */
        function replacePriceInString(stringObj, field) {
            if (stringObj.prices) {
                var price, offerId;

                _.forEach(stringObj.prices, function (value) {
                    price = Origin.utils.getProperty(value, [field]);
                    offerId = Origin.utils.getProperty(value, ['offerId']);

                    if (price && offerId) {
                        stringObj.stringContent = stringObj.stringContent.replace('%~' + offerId + '~%', price);
                    }
                });
            }

            return stringObj.stringContent;
        }

        /**
         * Similar to getLocalizedStr, but for strings that contain an offer placeholder
         * which must be extracted and then replaced with the price of that offer.
         *
         * @param {object} $scope - the scope of the directive to digest
         * @param {object} $scopelocation - the scope location to attach the translated string to by key
         * @param {String} possibleStr - the attribute/value of the
         *   parameter that is passed down as a parameter to the
         *   directive.  This will either be the already translated
         *   string or it will be empty.
         * @param {String} namespace the dictionary entry that contains the default
         *   translations for the translation lookup in case possibleStr is not defined
         * @param {String} key - the English key for the string to
         *   translate.
         * @param {object} params - *optional* this is the optional list
         *   of parameters to substitute the string with.
         * @param {object} choice - *optional* this is the optional number value that
         *   determines if pluralization within the @possibleStr or @key
         *   needs to be resolved.
         *
         * @param {string} field - the price field to use to replace
         *
         * @return {Promise} resolves to the localized string with parameters substituted,
         *   including offer price
         *
         */
        function insertPriceFieldIntoLocalizedStr($scope, $scopelocation, possibleStr, namespace, key, params, choice, field) {
            return getLocalizedStrPromise(possibleStr, namespace, key, params, choice)
                .then(extractOfferIdFromString)
                .then(getPrice)
                .then(_.partialRight(replacePriceInString, field))
                .catch(function(){
                    return possibleStr;
                })
                .then(function (str) {
                    $scopelocation[UtilFactory.htmlToScopeAttribute(key)] = str;
                    ScopeHelper.digestIfDigestNotInProgress($scope);
                });
        }

        function insertPriceIntoLocalizedStr($scope, $scopelocation, possibleStr, namespace, key, params, choice) {
            return insertPriceFieldIntoLocalizedStr($scope, $scopelocation, possibleStr, namespace, key, params, choice, 'formattedPrice');
        }

        function insertDiscountedPriceIntoLocalizedStr($scope, $scopelocation, possibleStr, namespace, key, params, choice) {
            return insertPriceFieldIntoLocalizedStr($scope, $scopelocation, possibleStr, namespace, key, params, choice, 'formattedDiscountPrice');
        }

        function insertDiscountedPercentageIntoLocalizedStr($scope, $scopelocation, possibleStr, namespace, key, params, choice) {
            return insertPriceFieldIntoLocalizedStr($scope, $scopelocation, possibleStr, namespace, key, params, choice, 'formattedDiscountRate');
        }

        return {
            insertPriceIntoLocalizedStr: insertPriceIntoLocalizedStr,
            insertDiscountedPriceIntoLocalizedStr: insertDiscountedPriceIntoLocalizedStr,
            insertDiscountedPercentageIntoLocalizedStr: insertDiscountedPercentageIntoLocalizedStr
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function PriceInsertionSingleton(UtilFactory, GamesPricingFactory, CurrencyHelper, ScopeHelper, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('PriceInsertionFactory', PriceInsertionFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.PriceInsertionFactory

     * @description
     *
     * PriceInsertionFactory
     */
    angular.module('origin-components')
        .factory('PriceInsertionFactory', PriceInsertionSingleton);
}());

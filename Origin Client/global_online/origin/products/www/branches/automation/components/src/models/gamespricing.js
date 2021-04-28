/**
 * @file game/gamespricing.js
 */
(function() {
    'use strict';
    var MAX_NUMBER_OF_OFFERS = 10;
    var REQUEST_TIMEOUT_PERIOD = 250;
    var NO_RESPONSE_ERR = 'NO RESPONSE';

    function GamesPricingFactory(ComponentsLogFactory) {
        var offersByCurrency = {}; // Lists of offers that are waiting to be requested, keyed by currency.
        var offerMap = {}; // Cache of pending Promise/Rating requests
        var timers = {}; // The timers to fetch with a partially full request, keyed by currency.

        /**
         * Handles an exception in the fetchBatch promise chain
         * @param  {object} error The error object
         */
        function handleFetchBatchError(error) {
            ComponentsLogFactory.error('fetchBatch failed:', error);
        }

        /**
         * Returns and array of objects see below
         *
         * @param  {Array} priceList The result of the ratings call
         * @return {Object}  [
         *     {
         *         "offerId":"Origin.OFR.50.0001000",
         *         "offerType":"Base Game",
         *         "rating":[{
         *             "originalConfiguredPrice":null,
         *             "originalUnitPrice":59.99,
         *             "quantity":1,
         *             "originalTotalPrice":59.99,
         *             "finalTotalAmount":59.99,
         *             "totalDiscountAmount":0,
         *             "totalDiscountRate":0,
         *             "promotions":{
         *                 "promotion":[]
         *             },
         *             "recommendedPromotions":{
         *                 "promotion":[]
         *             },
         *             "currency":"USD"
         *         }]
         *     }
         * ]
         *
         */
        function handlePolymorphicResponse(priceList) {
            return priceList[0] ? priceList[0]: [];
        }

        /**
         * Creates ratings promises for the list of offerIds. If a promise already exists for that ID returns that instead
         * @param  {Array} offerIdList List of offer IDS
         * @return {Array}             List of promise for the offers in order they were requested
         */
        function createPromises(offerIdList) {
            return _.map(offerIdList, function(offerId) {
                offerId = offerId.toLowerCase();
                if(offerMap[offerId]) {
                    return offerMap[offerId].promise;
                } else{
                    var res, rej;
                    var promise = new Promise(function(resolve, reject){
                        res = resolve;
                        rej = reject;
                    });

                    offerMap[offerId] = {
                        promise: promise,
                        resolve: res,
                        reject: rej
                    };
                    return promise;
                }
            });
        }

        /**
         * Helper function to turn and array of rating objects into a hash or rating objects keyed off the offer id
         * @param  {array} results The arrar of ratings objects
         * @return {object}         A HashMap of rating objects.
         */
        function ratingResponseToObject(results) {
            var object = {};
            _.forEach(results, function(ratingObject) {
                object[ratingObject.offerId.toLowerCase()] = ratingObject;
            });
            return object;
        }

        /**
         * Helper function to mimic a server error response
         * @param  {string} offerId The offer id
         * @return {object}         A object mimicking the server error.
         */
        function mimicErrorResponse(offerId) {
            return {
                offerId: offerId,
                offerType: null,
                rating: [],
                error: {
                    errorCode: NO_RESPONSE_ERR,
                    rootCause: {
                        cause: "rating service didn't respond with data",
                        field: "offerId",
                        value: offerId
                    }
                }
            };
        }

        /**
         * When an offer is fetched correctly it is resolved and removed from the offerMap
         * @param  {Array} results The result of the rating call [rating result object, rating result object]
         */
        function resolvePromises(requested, results) {
            var ratingObjects = ratingResponseToObject(results);
            _.forEach(requested, function(offerId) {
                var offerIdLC = offerId.toLowerCase();
                if(offerMap[offerIdLC]) {
                    if(offerIdLC && ratingObjects[offerIdLC]) {
                        offerMap[offerIdLC].resolve(ratingObjects[offerIdLC]);
                    } else {
                        offerMap[offerIdLC].resolve(mimicErrorResponse(offerId));
                    }
                    offerMap[offerIdLC] = null;
                }
            });
        }

        /**
         * If ratings call errors out, log an error and let the promise chain continue
         * @param error
         * @returns {Array}
         */
        function handleRatingsCallError(error) {
            ComponentsLogFactory.error('ERROR: games.fetchPrice call failed ', error);
            return [];
        }

        /**
         * Make a call to the JSSDK to fetch the set of offers
         * @param  {Array} offerList List of offer IDs
         * @param  {string} currency  The currency to fetch
         */
        function fetchBatch(offerList, currency) {
            Origin.games.getPrice(offerList, '', currency)
                .catch(handleRatingsCallError)
                .then(handlePolymorphicResponse)
                .then(_.partial(resolvePromises, offerList))
                .catch(handleFetchBatchError);
        }


        /**
         * Get the set of offers
         * @param  {string} currency The currency requested for the offers
         */
        function fetchInBatches(offerIds, currency) {
            // Loop over the request send them in batched of MAX_NUMBER_OF_OFFERS
            // So if MAX_NUMBER_OF_OFFERS = 5 slice off (0,5) then (5,10) etc
            for(var i = 0, j = 1; j * MAX_NUMBER_OF_OFFERS <= offerIds.length; i++, j++) {
                var batch = offerIds.slice(i * MAX_NUMBER_OF_OFFERS, j * MAX_NUMBER_OF_OFFERS);
                fetchBatch(batch, currency);
            }
        }

        /**
         * Checks the offersByCurrency map for the given currency and initializes it
         * @param {string} currency The currency code to initialize
         */
        function initOffersByCurrency(currency) {
            if(!offersByCurrency[currency]) {
                offersByCurrency[currency] = [];
            }
        }

        /**
         * Add offer IDs to the list of ids that need to be fetched.
         * @param {Array} requestedOfferIds A List of offerIds requested
         * @param {string} currency The currency the offers are priced in
         */
        function addOffers(requestedOfferIds, currency) {
            initOffersByCurrency(currency);
            _.forEach(requestedOfferIds, function(offerId) {
                 if(!offerMap[offerId.toLowerCase()]) {
                    offersByCurrency[currency].push(offerId);
                 }
            });
        }

        /**
         * Stop the timer to fetch ratings for a specific currency
         * @param  {string} currency The currency we want to cancel the timer for
         */
        function stopTimer(currency) {
            clearTimeout(timers[currency]);
            timers[currency] = null;
        }

        /**
         * Start a timer to fetch ratings for offers. Doesn't check if timer already exists
         * @param  {string} currency The currency requested for the offers
         */
        function startTimer(currency) {
            timers[currency] = setTimeout(function() {
                    fetchBatch(offersByCurrency[currency], currency);
                    offersByCurrency[currency] = null;
                    stopTimer(currency);
                }, REQUEST_TIMEOUT_PERIOD);
        }

        /**
         * Get the price of a set of offer
         * @param  {Array} offerIdList Set of offers
         * @param  {string} currency    The currency to fetch the offers in
         * @return {Promise}             A Promise that will resolve when all the ratings have returned
         */
        function getPrice(offerIdList, currency) {

            if(_.isEmpty(offerIdList)) {
                throw new Error('[GamesPricing] Asking for the price of an invalid offer ID/offer ID list: ' + offerIdList);
            }

            offerIdList = _.isArray(offerIdList) ? offerIdList: [offerIdList];

            currency = currency || Origin.locale.currencyCode();

            addOffers(offerIdList, currency);

            var promises = createPromises(offerIdList),
                offerList = offersByCurrency[currency];

            if(offerList && offerList.length >= MAX_NUMBER_OF_OFFERS) {
                if (timers[currency]) {
                    stopTimer(currency);
                }
                // Offers are sent in groups (0,5), (5,10) etc
                // if there are 24 items in offerList 4 remaining after
                // the groups are processed this would then become the offerList
                var remaining = offerList.length % MAX_NUMBER_OF_OFFERS;

                if(remaining > 0){
                    fetchInBatches(offerList.slice(0, -remaining), currency);
                    offersByCurrency[currency] = offerList.slice(-remaining);
                } else {
                    fetchInBatches(offerList, currency);
                    offersByCurrency[currency] = null;
                }
            }

            // If we still have unrequested offers start a timer.
            if (!timers[currency] && offersByCurrency[currency] && offersByCurrency[currency].length > 0) {
                startTimer(currency);
            }

            return Promise.all(promises);
        }

        return {

            /**
             * Get the price information
             * @param {string[]} offerIdList a list of offerIds
             * @param {string} currency the real or virtual currrency code, e.g. 'USD', 'CAD', '_BW' (optional)
             * @return {Promise} a promise for a list of pricing info
             * @method getPrice
             */
            getPrice: getPrice
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesPricingFactorySingleton(ComponentsLogFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesPricingFactory', GamesPricingFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesOcdFactory

     * @description
     *
     * GamesOcdFactory
     */
    angular.module('origin-components')
        .factory('GamesPricingFactory', GamesPricingFactorySingleton);
}());

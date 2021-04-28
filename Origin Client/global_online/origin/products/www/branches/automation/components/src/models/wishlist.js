/**
 * @file models/wishlist.js
 * @see https://confluence.ea.com/pages/viewpage.action?pageId=555199675#Wishlistmodelandpersistence-Wishlistusage
 */
(function() {

    'use strict';

    var ADD_OFFER_EVENT = 'addOffer',
        REMOVE_OFFER_EVENT = 'removeOffer';

    function WishlistFactory(FeatureConfig, DateHelperFactory, WishlistCloudStorageFactory, GamesEntitlementFactory, ObjectHelperFactory, GamesDataFactory, OfferTransformer, ProductInfoHelper, GameRefiner) {
        var isValidDate = DateHelperFactory.isValidDate,
            events = new window.Origin.utils.Communicator(),
            toArray = ObjectHelperFactory.toArray;

        /**
         * Allow this feature to be switched on and off by the feature registry
         * @return {Boolean} true if wishlist is enabled for integration
         */
        function isWishlistEnabled() {
            if(_.has(FeatureConfig, 'isWishlistEnabled') && _.isFunction(FeatureConfig.isWishlistEnabled)) {
                return FeatureConfig.isWishlistEnabled();
            }

            return false;
        }

        /**
         * Update the observer and pass through the response
         * @param  {String} eventName the event name to notify
         * @param  {String} userId the pid id to notify
         * @return {Object} data
         */
        function fireEventAndReturnData(eventName, userId) {
            return function(data) {
                events.fire(['wishlist', eventName, userId].join(':'));

                return data;
            };
        }

        /**
         * Pass through an error
         * @param  {String} eventName the event name to notify
         * @param  {String} userId the pid id to notify
         * @return {Promise.<err>}
         */
        function fireEventAndReturnError(eventName, userId) {
            return function(err) {
                events.fire(['wishlist', eventName, userId].join(':'));

                return Promise.reject(err);
            };
        }

        /**
         * Detect if an offer is in a given list
         * @param {String} offerId the offerId to find in the hastack
         * @return {Function}
         */
        function isOfferInWishlist(offerId) {
            /**
             * Process the data
             * @param {Array} offerList the offerlist to analyze
             * @return {Boolean} true if exists in list
             */
            return function(offerList) {
                if(!_.isArray(offerList)) {
                    return false;
                }

                return _.some(offerList, {id: offerId});
            };
        }

        /**
         * Get a user ID given a wishlist ID
         * @param {string} wishlistId the wishlist Id to lookup
         * @return {Promise.<String, Error>}
         */
        function getUserIdByWishlistId(wishlistId, storage) {
            storage = storage || WishlistCloudStorageFactory;

            return storage.getUserIdByWishlistId(wishlistId);
        }

        /**
         * Filters ownership of the wishlist as well as subscription when viewing a self wishlist
         * Offer will be shown if user owns OR user has subscription and owns the vault offer
         * @param {object} offerData
         * @param {object} wishlistItem
         * @returns {boolean}
         */
        function filterOwnedOffer(offerData, wishlistItem){
            var entitlements = GamesEntitlementFactory.getAllEntitlements(),
                offerId = _.get(wishlistItem, 'id'),
                catalog = offerData[offerId];
            if(entitlements[offerId]) {
                if (ProductInfoHelper.isSubscriptionAvailable(catalog) && !GameRefiner.isOwnedOutright(catalog)){ //Is a vault game and user does not own it permanently
                    return true;
                }
                return false;
            }
            return true;
        }

        /**
         *
         * @param {object[]} offerList
         * @param {object} offerData
         * @returns {object[]}
         */
        function filterEligibility(offerList, offerData){
            return _.filter(offerList, _.partial(filterOwnedOffer, offerData));
        }
        
        /**
         * Get catalog data for the wishlist items
         *
         * @param {object[]} offerList
         * @returns {Promise}
         */
        function getOfferCatalogAndEntitlements(offerList){
            return GamesDataFactory
                    .getCatalogInfo(_.pluck(offerList,'id'))
                    .then(OfferTransformer.transformOffer)
                    .then(function(offerData){
                        return [offerList, offerData];
                    });
        }

        /**
         * Is this user the same as the currently logged in user?
         *
         * @param {String} userId the nucleus user Id
         * @return {Boolean}
         */
        function isSelf(userId) {
            return userId === Origin.user.userPid();
        }

        /**
         * Wishlist constructor
         * @param {String} userId nucleus user id to inspect
         * @param {Object} storage a storage implementation
         */
        function Wishlist(userId, storage) {
            this.userId = userId;
            this.storage = storage || WishlistCloudStorageFactory;
        }

        /**
         * Get the ordered offer list
         * @return {Promise.<Booelan, Error>}
         */
        Wishlist.prototype.isOfferInWishlist = function(offerId) {
            if(!isWishlistEnabled()) {
                return Promise.reject('Wishlist feature not available');
            }

            return this.storage.getOfferList(this.userId)
                .then(isOfferInWishlist(offerId))
                .catch(function() {
                    Promise.resolve(false);
                });
        };

        /**
         * Get the ordered offer list
         * @return {Promise.<[Object], Error>}
         */
        Wishlist.prototype.getOfferList = function() {
            if(!isWishlistEnabled()) {
                return Promise.reject('Wishlist feature not available');
            }

            if(isSelf(this.userId)) {
                return this.storage.getOfferList(this.userId)
                    .then(getOfferCatalogAndEntitlements)
                    .then(_.spread(filterEligibility))
                    .then(toArray);
            } else {
                return this.storage.getOfferList(this.userId);
            }
        };

        /**
         * Add an offer to the wishlist
         * @param {String} offerId the offer id
         * @param {Object} timestamp an optional Date override - defaults to current time
         * @return {Promise.<void, Error>}
         */
        Wishlist.prototype.addOffer = function(offerId, timestamp) {
            if(!isWishlistEnabled()) {
                return Promise.reject('Wishlist feature not available');
            }

            if(!isValidDate(timestamp)) {
                timestamp = new Date();
            }

            return this.storage.addOffer(this.userId, offerId, timestamp.toISOString())
                .then(fireEventAndReturnData(ADD_OFFER_EVENT, this.userId))
                .catch(fireEventAndReturnError(ADD_OFFER_EVENT, this.userId));
        };

        /**
         * Remove an offer from the wishlist
         * @param {String} offerId the offerid
         * @return {Promise.<void, Error>}
         */
        Wishlist.prototype.removeOffer = function(offerId) {
            if(!isWishlistEnabled()) {
                return Promise.reject('Wishlist feature not available');
            }

            return this.storage.removeOffer(this.userId, offerId)
                .then(fireEventAndReturnData(REMOVE_OFFER_EVENT, this.userId))
                .catch(fireEventAndReturnError(REMOVE_OFFER_EVENT, this.userId));
        };

        /**
         * Get the wishlist id for this instance
         * @return {Promise.<String, Error>}
         */
        Wishlist.prototype.getWishlistId = function() {
            if(!isWishlistEnabled()) {
                return Promise.reject('Wishlist feature not available');
            }

            return this.storage.getWishlistId(this.userId);
        };

        /**
         * Get a wishlist instance
         * @param {String} userId nucleus user id to inspect
         * @param {Object} storage a persistence backend
         * @return {Object} an instance of Wishlist
         */
        function getWishlist(userId, storage) {
            return new Wishlist(userId, storage);
        }

        return {
            isWishlistEnabled: isWishlistEnabled,
            getWishlist: getWishlist,
            events: events,
            getUserIdByWishlistId: getUserIdByWishlistId
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.WishlistFactory

     * @description
     *
     * Wishlist Factory
     */
    angular.module('origin-components')
        .factory('WishlistFactory', WishlistFactory);

}());
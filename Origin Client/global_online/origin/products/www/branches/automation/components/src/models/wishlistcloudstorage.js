/**
 * @file models/wishlistcloudstorage.js
 */
(function() {
    'use strict';

    function WishlistCloudStorageFactory(ObjectHelperFactory) {
        var getProperty = ObjectHelperFactory.getProperty,
            mapWith = ObjectHelperFactory.mapWith,
            toArray = ObjectHelperFactory.toArray;

        /**
         * Transform the row into the wishlist schema for each item
         * @param  {Object} item the raw wishlist data from the server
         * @return {Object} the wishlist schema
         */
        function transformSchema(item) {
            var addedAtDate = new Date(_.get(item, 'addedAt', 0));

            return {
                id: _.get(item, 'offerId'),
                ts: addedAtDate.toISOString()
            };
        }

        /**
         * Fetch and cache the wishlist sharing ID
         * @param {String} userId  the nucleus userid
         * @return {Promise.<String, Error>}
         */
        function getWishlistId(userId) {
            return window.Origin.idObfuscate.encodeUserId(userId)
                .then(getProperty('id'));
        }

        /**
         * Fetch and cache the nucleus user ID
         * @param {String} wishlistId  the wishlist ID
         * @return {Promise.<String, Error>}
         */
        function getUserIdByWishlistId(wishlistId) {
            return window.Origin.idObfuscate.decodeUserId(wishlistId)
                .then(getProperty('id'));
        }

        /**
         * Get the offerlist from the backend
         * @param  {String} userId the wishlist user id to view
         * @return {Promise.<Object, Error>} the wishlist item
         */
        function getOfferList(userId) {
            return window.Origin.wishlist.getOfferList(userId)
                .then(getProperty('wishlist'))
                .then(mapWith(transformSchema))
                .then(toArray);
        }

        /**
         * Given a nucleus User ID and an offer id, add an offer to the wishlist
         * @param {String} userId  the nucleus userid
         * @param {String} offerId the offerid to add
         * @return {Promise.<mixed, Error>}
         */
        function addOffer(userId, offerId, isoTimestamp) {
            return window.Origin.wishlist.addOffer(userId, offerId, isoTimestamp);
        }

        /**
         * Given a nucleus User ID and an offer ID, remove the offer id from the user's wishlist
         * @param {String} userId  the nucleus userid
         * @param {String} offerId the offerid to add
         * @return {Promise.<mixed, Error>}
         */
        function removeOffer(userId, offerId) {
            return window.Origin.wishlist.removeOffer(userId, offerId);
        }

        return {
            getOfferList: getOfferList,
            addOffer: addOffer,
            removeOffer: removeOffer,
            getUserIdByWishlistId: _.memoize(getUserIdByWishlistId),
            getWishlistId: _.memoize(getWishlistId)
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.WishlistCloudStorageFactory

     * @description
     *
     * Cloud storage interface for wishlists
     */
    angular.module('origin-components')
        .factory('WishlistCloudStorageFactory', WishlistCloudStorageFactory);

}());
/*jshint unused: false */
/*jshint strict: false */

define([
    'promise',
    'core/logger',
    'core/user',
    'core/dataManager',
    'core/urls',
    'core/errorhandler',
    'core/utils'
], function(Promise, logger, user, dataManager, urls, errorhandler, utils) {
    /**
     * @module module:wishlist
     * @memberof module:Origin
     */

    /**
     * Wishlist Item - defines a wishlist row
     * @typedef {Object} WishlistItem
     * @property {string} offerId the offer id
     * @property {Number} displayOrder the order rank for the item
     * @property {Number} addedAt the timestamp the item was added to the wishlist
     */

    /**
     * Wishlist Sequence - defines a reordering sequence
     * @typedef {Object} WishlistSequence
     * @property {string} targetOfferId the offer id to move
     * @property {string} nextOfferId the next target position in the linked list
     * @property {string} prevOfferId the prev target position in the linked list
     */

    /**
     * Given a nucleus User ID, fetch an array of {@link Origin.module:wishlist~WishlistItem} objects
     * @param {String} userId the nucleus user to inspect
     * @return {Promise.<module:Origin.module:wishlist~WishlistItem[], Error>}
     */
    function getOfferList(userId) {
        if (!userId) {
            return Promise.reject('userId is required');
        }
        var endPoint = urls.endPoints.wishlistGetOfferList;

        var config = {
            atype: 'GET',
            headers: [{
                'label': 'X-Origin-Platform',
                'val': utils.os()
            }],
            parameters: [{
                'label': 'userId',
                'val': userId
            }],
            appendparams: [],
            reqauth: true,
            requser: true
        };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'AuthToken', token);
        }

        return dataManager.enQueue(endPoint, config, 0);
    }

    /**
     * Given a nucleus User ID and an offer id, add an offer to the wishlist
     * @param {String} userId  the nucleus userid
     * @param {String} offerId the offerid to add
     * @return {Promise.<void, Error>}
     */
    function addOffer(userId, offerId) {
        if (!userId || !offerId) {
            return Promise.reject('userId && offerId are required');
        }

        var endPoint = urls.endPoints.wishlistAddOffer;

        var config = {
            atype: 'PUT',
            headers: [{
                'label': 'X-Origin-Platform',
                'val': utils.os()
            }],
            parameters: [{
                'label': 'userId',
                'val': userId
            }, {
                'label': 'offerId',
                'val': offerId
            }],
            appendparams: [],
            reqauth: true,
            requser: true
        };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'AuthToken', token);
        }

        return dataManager.enQueue(endPoint, config, 0);
    }

    /**
     * Given a nucleus User ID and an offer ID, remove the offer id from the user's wishlist
     * @param {String} userId  the nucleus userid
     * @param {String} offerId the offerid to add
     * @return {Promise.<void, Error>}
     */
    function removeOffer(userId, offerId) {
        if (!userId || !offerId) {
            return Promise.reject('userId && offerId are required');
        }
        var endPoint = urls.endPoints.wishlistRemoveOffer;

        var config = {
            atype: 'DELETE',
            headers: [{
                'label': 'X-Origin-Platform',
                'val': utils.os()
            }],
            parameters: [{
                'label': 'userId',
                'val': userId
            }, {
                'label': 'offerId',
                'val': offerId
            }],
            appendparams: [],
            reqauth: true,
            requser: true
        };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'AuthToken', token);
        }

        return dataManager.enQueue(endPoint, config, 0);
    }

    return {
        getOfferList: getOfferList,
        addOffer: addOffer,
        removeOffer: removeOffer
    };
});

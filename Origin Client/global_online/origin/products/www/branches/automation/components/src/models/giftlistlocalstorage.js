/**
 * @file models/giftlistlocalstorage.js
 */
(function() {
    'use strict';

    var GIFTLIST_LOCAL_STORAGE_PREFIX = 'gl',
        allowOpen = true;

    function GiftlistLocalStorageFactory(LocalStorageFactory) {
        function getKey() {
            return GIFTLIST_LOCAL_STORAGE_PREFIX;
        }

        function offerExistsInGiftlist(offerList, offerId) {
            return _.some(offerList, {id: offerId});
        }

        function getOfferList() {
            var key = getKey();

            return LocalStorageFactory.get(key, [], true);
        }

        function getOffer(offerId) {
            var offerList = getOfferList();

            return _.find(offerList, {id: offerId});
        }

        function getUnopenedOfferId() {
            var offerList = getOfferList(),
                offer = _.find(offerList, {opened: false}),
                offerId = null;

            if(offer) {
                offerId = offer.id;
            }

            return offerId;
        }

        function addOffer(offerId, sender, message, isoTimestamp) {
            var key = getKey(),
                offerList = getOfferList(),
                data = {
                    id: offerId,
                    ts: isoTimestamp || new Date(),
                    sender: sender,
                    message: message,
                    opened: false
                };

            if(!offerExistsInGiftlist(offerList, offerId)) {
                offerList.unshift(data);
            }

            LocalStorageFactory.set(key, offerList, true);

            // trigger entitlement refresh via DirtyBits
            Origin.events.fire(Origin.events.DIRTYBITS_ENTITLEMENT);

            return getOfferList();
        }

        function removeOffer(offerId) {
            var key = getKey(),
                offerList = getOfferList();

            LocalStorageFactory.set(key, _.reject(offerList, {id: offerId}), true);

            return getOfferList();
        }

        function openOffer(offerId) {

            if(!allowOpen) {
                return false;
            }

            var key = getKey(),
                offer = getOffer(offerId),
                offerList = getOfferList(),
                offerIndex = _.findIndex(offerList, {id: offerId});

            if(offerIndex > -1) {
                offer.opened = true;
                offerList[offerIndex] = offer;
            }

            LocalStorageFactory.set(key, offerList, true);

            return offer.opened;
        }

        function unopenOffer(offerId) {
            var key = getKey(),
                offer = getOffer(offerId),
                offerList = getOfferList(),
                offerIndex = _.findIndex(offerList, {id: offerId});

            if(offerIndex > -1) {
                offer.opened = false;
                offer.ts = new Date();
                offerList[offerIndex] = offer;
            }

            LocalStorageFactory.set(key, offerList, true);

            // trigger entitlement refresh via DirtyBits
            Origin.events.fire(Origin.events.DIRTYBITS_ENTITLEMENT);

            return offer;
        }

        function emptyGiftlist() {
            var key = getKey();

            LocalStorageFactory.set(key, [], true);

            return getOfferList();
        }

        function setAllowOpen(allow) {
            allowOpen = allow;
        }

        return {
            getOffer: getOffer,
            getUnopenedOfferId: getUnopenedOfferId,
            getOfferList: getOfferList,
            addOffer: addOffer,
            removeOffer: removeOffer,
            openOffer: openOffer,
            unopenOffer: unopenOffer,
            emptyGiftlist: emptyGiftlist,
            setAllowOpen: setAllowOpen
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GiftlistLocalStorageFactory

     * @description
     *
     * This is a model placeholder/interface spec for Giftlist that will store information locally while
     * the Giftlist feature is under developement by our backend developers
     *
     * @TODO: remove when the cloud version is ready.
     */
    angular.module('origin-components')
        .factory('GiftlistLocalStorageFactory', GiftlistLocalStorageFactory);

}());

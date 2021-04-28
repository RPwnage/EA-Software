/**
 * @file factories/common/gifting.js
 */
(function () {
    'use strict';

    function GiftingFactory(ObjectHelperFactory, FeatureConfig, GiftStateConstants) {


        var giftResponse = [], // array of gift information by offer id retrieved from server. will include duplicates
            unopenedGifts = [], // unopened gifts in ordered by receive date; most recently received first
            allGifts = [],
            createGiftModel = ObjectHelperFactory.transformWith({
                'offerId': 'productId',
                'giftId': 'giftId',
                'senderPersonaId': 'senderId',
                'status': 'status',
                'senderName': 'senderName',
                'message': 'message',
                'sendDate': 'sendDate'
            });

        /**
         * Determine if a game is giftable:
         *
         * The following are product types giftable:
         *      - Base Games
         *      - Expansion Packs
         *      - Add-Ons
         *      - Any bundle that is a combination of the above 3 product types
         *      - First party title
         *
         * Non-giftable Products
         *      - Virtual Currency
         *      - Any bundle that includes a virtual currency offer bundled with it
         *      - Subscription
         *      - Pre-Orders (NOTE: Primarily due to credit card UX complications, these will not be giftable)
         *      - Items priced zero dollars (OTH, Demos, Betas, Free)
         *
         * @param game
         * @returns {boolean}
         */
        function isGameGiftable(game) {
            return isGiftingEnabled() && _.get(game, ['giftable'], false);
        }

        /**
         * Whether gifting enabled for store front and user is not underage.
         * @returns {Promise}
         */
        function isGiftingEnabled() {
            return FeatureConfig.isGiftingEnabled() && !Origin.user.underAge() &&
                   Origin.user.userStatus() !== Origin.defines.userStatus.BANNED;
        }

        /**
         * Return latest received unopened gift
         * @return {Object} unopened gift
         */
        function getNextUnopenedGift() {
            return _.head(getUnopenedGifts());
        }

        /**
         * Return whether specified offer is an unopened gift
         * @param  {String}  offerId offer id to check
         * @return {Boolean}         true if offer id is an unopened gift; false otherwise
         */
        function isUnopenedGift(offerId) {
            var unopenedGift = getGift(offerId, GiftStateConstants.RECEIVED);

            return !_.isUndefined(unopenedGift) && unopenedGift !== null;
        }

        /**
         * Return a gift by offer id. Since it is possible to have duplicate offerIds, we need to find all gifts by given offerId
         * then look for gifts that haven't been opened. if there are multiple unopened of the same offerId, then return the newest one
         * @param  {String} offerId offer id of gift to retrieve
         * @param {string} status gift status
         * @return {Object}         gift information
         */
        function getGift(offerId, status) {
            var giftsFound = _.where(getAllGifts(), {offerId: offerId, status: status});

            if (giftsFound.length > 1) {
                giftsFound.sort(sortBySendDate);
            }

            return _.head(giftsFound);
        }

        function handleActivateSuccess(gift) {
            if (gift) {
                gift.status = GiftStateConstants.OPENED;
            }
            return gift;
        }


        function handleOpenGiftError() {
            return Promise.reject(new Error('gift-not-found'));
        }

        /**
         * Open a gift
         * @param  {String} offerId offer id of gift to open
         * @return {Promise<bool>}  promise indicating whether gift was opened successfully
         */
        function openGift(offerId) {

            var gift = getGift(offerId, GiftStateConstants.RECEIVED);

            if (gift && gift.giftId) {
                return Origin.gifts.activateGift(gift.giftId)
                    .then(_.partial(handleActivateSuccess, gift))
                    .catch(handleOpenGiftError);
            } else {
                return handleOpenGiftError();
            }

        }

        /**
         * Converts sendDate to an INT and converts gift response
         * @param {Object} gift response
         * @returns {Object} converted gift
         */
        function convertGiftModel(gift) {
            // converts send date to an int so we can sort by send date
            gift.sendDate = Date.parse(gift.sendDate);

            return createGiftModel(gift);
        }

        /**
         * Converts gift response
         * @param {Object} gift response
         * @returns {Object} converted gift
         */
        function handleGetDataSuccess(response) {
            giftResponse = _.map(_.get(response, 'gifting', []), convertGiftModel);

            return giftResponse;
        }

        /**
         * Sorts in descending receive date order
         * @param {Object} a gift info
         * @param {Object} b gift info
         * @returns {number}
         */
        function sortBySendDate(a, b) {
            if (b.sendDate > a.sendDate) {
                return 1;
            } else if (b.sendDate < a.sendDate) {
                return -1;
            }
            return 0;
        }

        /**
         * Extract and store unopened gifts
         * 1. store all gifts
         * 2. store unopened gifts
         * 3. sort unopened gifts
         * @param {Array<EntitlementObject>} entitlements entitlements response from server
         * @param {Object<gifts>}           gifts        map of gifts by offer id
         * @return {Array<entitlements,unopenedGifts>}   Array containing original passed in entitlements and unopened gifts
         */
        function extractUnopenedGifts(entitlements, gifts) {
            var ent;

            if (gifts) {
                // clear gift info
                unopenedGifts = [];

                // 1. update receive date of gift
                // 2. store each gift into 'allGifts' keyed by offer id
                // 3. store unopened gifts into 'unopenedGifts'
                _.forEach(gifts, function (gift) {
                    // update receive date
                    ent = _.find(entitlements, {'offerId': _.get(gift, 'offerId')});
                    _.set(gift, 'receiveDate', _.get(ent, 'grantDate'));

                    // store gifts
                    allGifts.push(gift);
                    if (gift.status !== GiftStateConstants.OPENED) {
                        unopenedGifts.push(gift);
                    }
                });

                // Sort by send date. Most recently received first.
                unopenedGifts.sort(sortBySendDate);

                return [entitlements, unopenedGifts];
            }

            return [entitlements, null];
        }

        function handleGetDataError() {
            return null;
        }

        /**
         * Retrieve gifts
         * @return {Promise<Object>} a promise for gift response from server
         */
        function retrieveGifts(forceRetrieve) {
            // If we already have the gifts and its not a force refresh, return the cached gifts
            if (giftResponse.length > 0 && !forceRetrieve) {
                return Promise.resolve(giftResponse);
            }

            return Origin.gifts.getGifts()
                .then(handleGetDataSuccess)
                .catch(handleGetDataError);
        }

        function getAllGifts() {
            return allGifts;
        }

        function getUnopenedGifts() {
            return unopenedGifts;
        }


        return {
            isGameGiftable: isGameGiftable,
            isGiftingEnabled: isGiftingEnabled,
            getNextUnopenedGift: getNextUnopenedGift,
            isUnopenedGift: isUnopenedGift,
            extractUnopenedGifts: extractUnopenedGifts,
            getGift: getGift,
            openGift: openGift,
            retrieveGifts: retrieveGifts
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
        .constant('GiftStateConstants', {
            RECEIVED: 'ACCEPTED',
            OPENED: 'ACTIVATED'
        })
        .factory('GiftingFactory', GiftingFactory);

}());

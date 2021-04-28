/*jshint unused: false */
/*jshint strict: false */
define([
    'promise',
    'core/logger',
    'core/user',
    'core/urls',
    'core/dataManager',
    'core/errorhandler',
    'core/locale',
    'core/utils',
    'modules/client/client'
], function(Promise, logger, user, urls, dataManager, errorhandler, locale, utils, client) {

    var PAGE_NUMBER = 1,
        PAGE_SIZE = 500,
        subReqNum = 0;

    function processGiftingEligibilityResponse(response) {
        if (response && response.recipients) {
            return response.recipients;
        } else {
            return [];
        }
    }


    return /** @lends module:Origin.module:gifts */{

        /**
         * Activate (i.e., open) a gift by offerId
         * @param {String} giftId offer id of gift to open
         * @return {Promise<response>} response from gift activation
         */
        activateGift: function(giftId) {
            var endPoint = urls.endPoints.updateGiftStatus;
            var config = {
                atype: 'PUT',
                headers: [],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                },{
                    'label': 'giftId',
                    'val': giftId
                }],
                appendparams: [],
                reqauth: true,
                requser: true
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }

            return dataManager.enQueue(endPoint, config, subReqNum)
                .catch(errorhandler.logAndCleanup('GIFTS:activateGift FAILED'));
        },

        /**
         * @typedef giftDataObject
         * @type {object}
         * @property {string} giftId
         * @property {string} senderPersonaId
         * @property {string} status
         * @property {string} senderName
         * @property {string} message
         * @property {string} productId
         */

        /**
         * This will return a promise for the user's gifts
         *
         * @return {promise<module:Origin.module:gifts~giftDataObject[]>}
         *
         */
        getGift: function(giftId) {
            var endPoint = urls.endPoints.getGift;
            var config = {
                atype: 'GET',
                headers: [],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                },
                {
                    'label': 'giftId',
                    'val': giftId
                }],
                reqauth: true,
                requser: true
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }

            return dataManager.enQueue(endPoint, config, subReqNum)
                .catch(errorhandler.logAndCleanup('GIFTS:getGift FAILED'));
        },

        /**
         * @typedef giftDataObject
         * @type {object}
         * @property {string} giftId
         * @property {string} senderPersonaId
         * @property {string} status
         * @property {string} senderName
         * @property {string} message
         * @property {string} productId
         */

        /**
         * This will return a promise for the user's gifts
         *
         * @return {promise<module:Origin.module:gifts~giftDataObject[]>}
         *
         */
         getGifts: function() {
            var endPoint = urls.endPoints.getGifts;
            var config = {
                atype: 'GET',
                headers: [],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }],
                appendparams: [{
                    'label': 'pageNo',
                    'val': PAGE_NUMBER
                },{
                    'label': 'pageSize',
                    'val': PAGE_SIZE
                }],
                reqauth: true,
                requser: true
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }

            return dataManager.enQueue(endPoint, config, subReqNum)
                .catch(errorhandler.logAndCleanup('GIFTS:getGifts FAILED'));
        },

        /**
         * Determine eligibility of recipients to receive gift from user
         * @param  {string} offerId      Offer ID to gift
         * @param  {string} recipientIds Comma-separated list of user IDs
         * @return {promise}              Recipient response
         */
        getGiftingEligibility: function(offerId, recipientIds) {
            var endPoint = urls.endPoints.giftingEligibility;
            var config = {
                atype: 'GET',
                headers: [{
                    'label': 'X-Origin-Platform',
                    'val': utils.os()
                },{
                    'label': 'Accept',
                    'val': 'application/json'
                }],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }, {
                    'label': 'recipientIds',
                    'val': recipientIds.replace(/ /g, '')
                }, {
                    'label': 'offerId',
                    'val': offerId
                }],
                reqauth: true,
                requser: true
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }

            return dataManager.enQueue(endPoint, config, 0)
                .then(processGiftingEligibilityResponse, errorhandler.logAndCleanup('ATOM:processGiftingEligibilityResponse FAILED'));
        }
    };
});
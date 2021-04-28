/*jshint unused: false */
/*jshint strict: false */
define([
    'promise',
    'core/logger',
    'core/user',
    'core/urls',
    'core/dataManager',
    'core/errorhandler'
], function(Promise, logger, user, urls, dataManager, errorhandler) {
    /**
     * @module module:subscription
     * @memberof module:Origin
     */

    var subReqNum = 0;  //to keep track of the request #

    function userSubscriptionBasic(subscriptionState) {
        var endPoint = urls.endPoints.userSubscription,
            auth = 'Bearer ',
            config = {
                atype: 'GET',
                headers: [],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }, {
                    'label': 'state',
                    'val' : subscriptionState
                }],
                reqauth: true, //set these to false so that dataREST doesn't automatically trigger a relogin
                requser: true
            };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            auth += token;
            dataManager.addHeader(config, 'Authorization', auth);
        }

        return dataManager.enQueue(endPoint, config, subReqNum)
            .catch(errorhandler.logAndCleanup('SUBS:userSubscriptionBasic FAILED'));
    }

    function userSubscriptionDetails(uri, forceRetrieve) {
        var endPoint = urls.endPoints.userSubscriptionDetails,
            auth = 'Bearer ',
            config = {
                atype: 'GET',
                headers: [],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid(),
                }, {
                    'label' : 'uri',
                    'val': uri
                }],
                reqauth: true,
                requser: true
            };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            auth += token;
            dataManager.addHeader(config, 'Authorization', auth);
        }

        if (typeof forceRetrieve !== 'undefined' && forceRetrieve === true) {
            subReqNum++; //update the request #
        }

        return dataManager.enQueue(endPoint, config, subReqNum)
            .catch(errorhandler.logAndCleanup('SUBS:userSubscriptionDetails FAILED'));
    }

    function getUserVaultInfo(forceRetrieve) {
        var endPoint = urls.endPoints.userVaultInfo,
            config = {
                atype: 'GET',
                headers: [{
                    'label': 'Accept',
                    'val': 'application/vnd.origin.v3+json'
                }],
                parameters: [],
                reqauth: true,
                requser: false
            };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'AuthToken', token);
        }

        if (typeof forceRetrieve !== 'undefined' && forceRetrieve === true) {
            subReqNum++; //update the request #
        }
        return dataManager.enQueue(endPoint, config, subReqNum)
            .catch(errorhandler.logAndCleanup('SUBS:getUserVaultInfo FAILED'));
    }

    return /** @lends module:Origin.module:subscription */{

        /**
         * @typedef subscriptionBasicObject
         * @type {object}
         * @property {string} firstSignUpDate date firsg signed up
         * @property {string} firstSignUpSubs path to first signed up subs
         * @property {string[]} subscriptionUri array of subscription uri paths
         */
        /**
         * This will return a promise for the user's basic subscription info
         *
         * @return {promise<module:Origin.module:subscription~subscriptionBasicObject>} responsename user's basic subs info, object will be empty if not a subscriber
         * @method
         */
        userSubscriptionBasic: userSubscriptionBasic,


        /**
         * @typedef subscriptionDetailObject
         * @type {object}
         * @property {string} userUri
         * @property {string} offerUri
         * @property {string} source
         * @property {string} status
         * @property {string} dateCreated
         * @property {string} dateModified
         * @property {string} entitlementsUri
         * @property {string} eventsUri
         * @property {string} invoicesUri
         * @property {string} scheduleOpesUri
         * @property {string} subscriptioinUri
         * @property {string} billingMode
         * @property {string} anniersaryDay
         * @property {string} subsStart
         * @property {string} subsEnd
         * @property {string} nextBillingDate
         * @property {string} freeTrial
         * @property {string} accountUri
         */
        /**
         * @typedef eventObject
         * @type {object}
         * @property {string} eventType
         * @property {string} eventDate
         * @property {string} eventStatus
         * @proeprty {string} eventId
         */
        /**
         * @typedef subscriptionEventReponseObject
         * @type {object}
         * @property {eventObject[]} SubscriptionEvent
         */
        /**
         * @typedef propertiesInfoObject
         * @type {Object}
         * @property {string} name
         * @property {string} value
         */
        /**
         * @typedef scheduleOperationObject
         * @type {object}
         * @property {string} operationId
         * @property {string} operationName
         * @property {string} status
         * @property {string} scheduleDate
         * @property {propertiesInfoObject[]} PropertiesInfo
         */
        /**
         * @typedef scheduleOperationResponseObject
         * @type {object}
         * @property {scheduleOperationObject[]} ScheduleOperation
         */
        /**
         * @typedef subscriptionDetailObject
         * @type {object}
         * @property {subscriptionObject} Subscription
         * @property {subscriptionEventResponseObject} GetSubscriptionEventsResponse
         * @property {scheduleOperationResponseObject} GetScheduleOperationResponse
         */
        /**
         * This will return a promise for the detail of a particualr subscription
         *
         * @param {string} uri uri of subscription
         * @param {boolean} forceRetrieve flag to force a retrieval from the server
         * @return {promise<module:Origin.module:subscription~subscriptionDetailObject>} responsename user's detailed subs info
         * @method
         */
        userSubscriptionDetails: userSubscriptionDetails,


        /**
         * This will return a promise for the details of user's vault
         *
         * @return {promise}
         * @method
         */
        getUserVaultInfo: getUserVaultInfo

    };
});
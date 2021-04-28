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
    'core/events',
    'core/utils',
    'modules/client/client',
    'generated/jssdkconfig.js'
], function(Promise, logger, user, urls, dataManager, errorhandler, locale, events, utils, client, jssdkconfig) {
    /**
     * @module module:subscription
     * @memberof module:Origin
     */

    var subReqNum = 0;  //to keep track of the request #

    function hasUsedTrial() {
        var endPoint = urls.endPoints.hasUsedTrial,
            config = {
                atype: 'GET',
                headers: [{
                    'label': 'Accept',
                    'val': 'application/vnd.origin.v3+json; x-cache/force-write'
                }],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }],
                reqauth: true,
                requser: true
            };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'AuthToken', token);
        }

        return dataManager.enQueue(endPoint, config, subReqNum)
            .catch(errorhandler.logAndCleanup('SUBS:hasUsedTrial FAILED'));
    }

    function userSubscriptionBasic(subscriptionState) {
        var endPoint = urls.endPoints.userSubscription,
            auth = 'Bearer ',
            config = {
                atype: 'GET',
                headers: [{
                    'label': 'Accept',
                    'val': 'application/vnd.origin.v3+json; x-cache/force-write'
                }],
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
                headers: [{
                    'label': 'Accept',
                    'val': 'application/vnd.origin.v3+json; x-cache/force-write'
                }],
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
                    'val': 'application/vnd.origin.v3+json; x-cache/force-write'
                }],
                parameters: [],
                reqauth: true,
                requser: false
            };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'AuthToken', token);
        }

        var cmsstage = utils.getProperty(jssdkconfig, ['overrides', 'cmsstage']) || '';
        if (cmsstage === 'preview' || cmsstage === 'approve') {
            dataManager.addHeader(config, 'X-Origin-Access-Use-Unpublished-Vault', 'true');
        }

        if (typeof forceRetrieve !== 'undefined' && forceRetrieve === true) {
            subReqNum++; //update the request #
        }
        return dataManager.enQueue(endPoint, config, subReqNum)
            .catch(errorhandler.logAndCleanup('SUBS:getUserVaultInfo FAILED'));
    }

    function vaultEntitle(subscriptionId, offerId) {
        var token = user.publicObjs.accessToken();
        if (token.length === 0) {
            return Promise.reject({success: false, message: 'requires-auth'});
        }

        if (typeof subscriptionId === 'undefined' || subscriptionId === null) {
            return Promise.reject({success: false, message: 'requires-subscription'});
        }

        var endPoint = urls.endPoints.vaultEntitle;
        var config = {atype: 'POST', reqauth: true, requser: true };
        var clientOrWeb = client.isEmbeddedBrowser() ? 'CLIENT' : 'WEB';
        var countryCode = locale.countryCode();
        var source = 'ORIGIN-STORE-'+clientOrWeb+'-'+countryCode;

        dataManager.addHeader(config, 'authToken', token);
        dataManager.addAuthHint(config, 'authToken', '{token}');
        dataManager.addParameter(config, 'userId', user.publicObjs.userPid());
        dataManager.addParameter(config, 'subscriptionId', subscriptionId);
        dataManager.addParameter(config, 'offerId', offerId);
        dataManager.appendParameter(config, 'source', source);

        return dataManager.enQueue(endPoint, config, subReqNum)
            .then(function(data){
                // trigger dirtybits entitlement update event and then pass through data
                events.fire(events.DIRTYBITS_ENTITLEMENT);
                return data;
            })
            .catch(errorhandler.logAndCleanup('SUBS:vaultGameCheckout FAILED'));
    }

    function vaultRemove(subscriptionId, offerId) {
        var token = user.publicObjs.accessToken();
        if (token.length === 0) {
            return Promise.reject({success: false, message: 'requires-auth'});
        }

        if (typeof subscriptionId === 'undefined' || subscriptionId === null) {
            return Promise.reject({success: false, message: 'requires-subscription'});
        }

        var endPoint = urls.endPoints.vaultRemove;
        var config = {atype: 'DELETE', reqauth: true, requser: true };

        dataManager.addHeader(config, 'authToken', token);
        dataManager.addAuthHint(config, 'authToken', '{token}');
        dataManager.addParameter(config, 'userId', user.publicObjs.userPid());
        dataManager.addParameter(config, 'subscriptionId', subscriptionId);
        dataManager.appendParameter(config, 'offerId', offerId);

        return dataManager.enQueue(endPoint, config, subReqNum)
            .then(function(){
                // trigger dirtybits entitlement update event and then pass
                // through success response, since no response is returned
                // from request
                events.fire(events.DIRTYBITS_ENTITLEMENT);
                return {success: true};
            })
            .catch(errorhandler.logAndCleanup('SUBS:vaultGameRemove FAILED'));
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
        getUserVaultInfo: getUserVaultInfo,

        /**
         * @typedef fullfillmentResponseObject
         * @type {object}
         * @property {stringp[]} entitlementUris array of entitlement URI paths
         * @property {stringp[]} subscriptionUris  array of subscription URI paths
         */
        /**
         * Grant vault game entitlement to user
         *
         * @param {string} subscriptionId users subscription ID
         * @param {string} offerId offer to grant entitlement for
         * @return {promise<module:Origin.module:subscription~fullfillmentResponseObject>} responsename vault entitle fullfillment response
         * @method
         */
        vaultEntitle: vaultEntitle,

        /**
         * @typedef removalResponseObject
         * @type {object}
         * @property {boolean} success true on successful removal, otherwise false
         */
        /**
         * Remove vault game entitlement from user
         *
         * @param {string} subscriptionId users subscription ID
         * @param {string} offerId offer to remove entitlement for
         * @return {promise<module:Origin.module:subscription~removalResponseObject>} responsename vault removal response
         * @method
         */
        vaultRemove: vaultRemove,

        /**
         * @typedef hasUsedTrialResponseObject
         * @type {object}
         * @property {boolean} hasUsedTrial has the user comsumed the subscription trial
         */
        /**
         * Check the users subscription trial status
         *
         * @return {promise<module:Origin.module:subscription~hasUsedTrialResponseObject>} responsename has used trial fullfillment response
         * @method
         */
        hasUsedTrial: hasUsedTrial
    };
});
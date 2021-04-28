/**
 * @file common/subscription.js
 * handles subscription logic both user-related and game-related
 */
(function() {
    'use strict';

    var SUPPORTED_PLATFORMS = ['PCWIN'];

    function SubscriptionFactory(ComponentsLogFactory, ObjectHelperFactory) {
        var userSubsInfo = {},
            bSubsDetailRetrieved = false,
            bUserHasSubscription = false,
            bUserHasUsedSubscriptionTrial = false,
            bUserHasBasicInfo = false,
            bSubscriptionInfoRetrieved = false,
            ONE_DAY_MS = 24 * 60 * 60 * 1000, //one day in Milliseconds
            SUBSCRIPTION_MAX_OFFLINE_PLAY_TIME = 30 * 24 * 60 * 60 * 1000, //30 days
            SUBSCRIPTION_STATE_NORMAL = 'ENABLED,PENDINGEXPIRED',
            SUBSCRIPTION_STATE_EXPIRED = 'EXPIRED',
            SUBSCRIPTION_STATE_PENDINGEXPIRED = 'PENDINGEXPIRED',
            SUBSCRIPTION_STATE_SUSPENDED = 'SUSPENDED',
            SUBSCRIPTION_CANCEL_REASON = 'CANCEL_REASON',
            expirationChecked = false,
            subscriptionCommunicator = new Origin.utils.Communicator(),
            masterTitleIdToVaultGame = {},
            masterTitleIdToVaultGameCreated = false,
            offerIdToItemId = {},
            offerIdToItemIdCreated = false;

        function extractSubscriptionId(attributeValue) {
            var subscriptionId = null;
            if (attributeValue) {
                var splitValues = attributeValue.split('/');
                if (splitValues.length) {
                    subscriptionId = _.last(splitValues);
                }
            }
            return subscriptionId;

        }

        function rejectIfSubscriptionIdNotFound() {
            return Promise.reject('ERROR: basic subscription info missing subscriptionUri');
        }

        function parseBasicResponse(response) {
            userSubsInfo.basicInfo = {};
            userSubsInfo.basicInfo.firstSignUpDate = new Date (response.firstSignUpDate);
            userSubsInfo.basicInfo.firstSignUpSubs = response.firstSignUpSubs;
            if (_.isArray(response.subscriptionUri) && response.subscriptionUri.length) {
                userSubsInfo.basicInfo.subscriptionUri = ObjectHelperFactory.copy(response.subscriptionUri);
            }
            bUserHasBasicInfo = true;
        }

        /**
         * Parses user's basic info if user has an active subscription and a subscriptionId is found
         * @param response server response
         * @returns {*} response. if subscription Id is missing reject to trigger error handler.
         */
        function parseBasicInfoForActive(response) {
            if (response) {

                if (_.isArray(response.subscriptionUri) && response.subscriptionUri.length) {
                    parseBasicResponse(response);
                    userSubsInfo.basicInfo.subscriptionId = extractSubscriptionId(response.subscriptionUri[0]);
                    if (!userSubsInfo.basicInfo.subscriptionId) {
                        return rejectIfSubscriptionIdNotFound();
                    }
                } //else let promise go thru, probably an expired or non subscriber
                bSubscriptionInfoRetrieved = true;
            }

            return response;
        }

        /**
         * Parses user's basic info if user has an expired subscription
         * @param response server response
         * @returns {*} response. if subscription Id is missing reject to trigger error handler.
         */
        function parseBasicInfoForExpired(response) {
            if (response) {

                if (response.firstSignUpSubs) { //for expired subscriber, subscription id can be extracted from firstSignUpSubs
                    parseBasicResponse(response);
                    userSubsInfo.basicInfo.subscriptionId = extractSubscriptionId(response.firstSignUpSubs);
                    if (!userSubsInfo.basicInfo.subscriptionId) {
                        return rejectIfSubscriptionIdNotFound();
                    }
                }
                bSubscriptionInfoRetrieved = true;
            }

            return response;
        }

        function handleUserBasicInfoError(error) {
            ComponentsLogFactory.error('SUBSCRIPTION - retrieveUserBasicInfo:', error);
        }

        function retrieveUserBasicInfo() {
            expirationChecked = false;  //reset
            return Origin.subscription.userSubscriptionBasic(SUBSCRIPTION_STATE_NORMAL)
                .then(parseBasicInfoForActive)
                .then(checkForExpired)
                .catch(handleUserBasicInfoError);
        }

        function checkForExpired(response) {
            //check once to see if the subs account has expired
            if (response && !bUserHasBasicInfo && !expirationChecked && response.hasOwnProperty('firstSignUpDate') && response.firstSignUpDate !== '') { //if we haven't yet checked expiration
                expirationChecked = true;
                return Origin.subscription.userSubscriptionBasic(SUBSCRIPTION_STATE_EXPIRED)
                    .then(parseBasicInfoForExpired)
                    .catch(handleUserBasicInfoError);
            }
        }

        function convertToDate(object, propertyArray) {
            var date = Origin.utils.getProperty(object, propertyArray);

            if (date !== null) {
                date = new Date(date);
            }

            return date;
        }

        function processUserSubDetailResponse(response) {
            var detailInfo = response,
                userStatus = Origin.utils.getProperty(detailInfo, ['Subscription', 'status']),
                validStates = SUBSCRIPTION_STATE_NORMAL.split(',');

            //convert all dates to date Object
            detailInfo.Subscription.dateCreated = convertToDate(detailInfo.Subscription, ['dateCreated']);
            detailInfo.Subscription.dateModified = convertToDate(detailInfo.Subscription, ['dateModified']);
            detailInfo.Subscription.subsStart = convertToDate(detailInfo.Subscription, ['subsStart']);
            detailInfo.Subscription.subsEnd = convertToDate(detailInfo.Subscription, ['subsEnd']);
            detailInfo.Subscription.nextBillingDate = convertToDate(detailInfo.Subscription, ['nextBillingDate']);
            userSubsInfo.detailInfo = detailInfo;

            if (validStates.indexOf(userStatus) > -1) {
                 bUserHasSubscription = true;
            }
            bSubsDetailRetrieved = true;
        }

        function handleUserSubDetailsError(error) {
            ComponentsLogFactory.error('SUBSCRIPTION - userSubscriptionDetailsError:', error);
        }

        function retrieveUserDetails() {
            //extract out the current subscription uri
            var uri, len, error;

            if (bUserHasBasicInfo) {
                len = _.size(userSubsInfo.basicInfo.subscriptionUri);
                if (len) {
                    uri = userSubsInfo.basicInfo.subscriptionUri [len - 1];
                    return Origin.subscription.userSubscriptionDetails(uri)
                        .then(processUserSubDetailResponse)
                        .catch(handleUserSubDetailsError);
                } else {
                    //shouldn't be here since the error should have been caught in handleUserBasicInfo but...
                   error = new Error('ERROR: basic subscription info missing subscriptionUri');
                   return Promise.reject(error);
                }
            } else {
                return Promise.resolve();      //user is not a subscriber, just resolve the promise
            }
        }

        // creates a mapping of all mastertitle ids to their vault games
        function setMasterTitleIdToVaultGame(game) {
            return function(id) {
                masterTitleIdToVaultGame[id] = game;
            };
        }

        // gets a list of all vault games mastertitle ids
        function getMasterTitleIdArray(masterTitleIdArray) {
            return function(game) {
                masterTitleIdArray = _.get(game, ['masterTitleIds', 'masterTitleId'], []);

                _.forEach(masterTitleIdArray, setMasterTitleIdToVaultGame(game));
            };
        }

        // creates a mapping of matertitle ids to vault games
        function createMasterTitleToGameMapping() {
            var gameArray,
                masterTitleIdArray;

            if (!masterTitleIdToVaultGameCreated) {
                if (userSubsInfo.userVault) {
                    gameArray =  _.get(userSubsInfo, ['userVault', 'game'], []);

                    _.forEach(gameArray, getMasterTitleIdArray(masterTitleIdArray));
                }
                masterTitleIdToVaultGameCreated = true;
            }
        }

        function createOfferIdtoItemIdMapping() {
            var i, j,
                glen, mlen, olen,
                gameArray,
                masterTitleIdArray,
                offerInfoArray;

            if (!offerIdToItemIdCreated) {
                if (userSubsInfo.userVault) {
                    glen = userSubsInfo.userVault.game.length;

                    gameArray = userSubsInfo.userVault.game;

                    for (i = 0; i < glen; i++) {
                        masterTitleIdArray = Origin.utils.getProperty(gameArray[i], ['masterTitleIds', 'masterTitleId']);
                        mlen = masterTitleIdArray.length;

                        // ORSUBS-3013: If the vaultInfo node does not contain a masterTitleId, skip it.  
                        // These are DLC-only bundles like BF4 Premium that the client currently does not support.
                        if (mlen === 0) {
                            continue;
                        }

                        offerInfoArray = Origin.utils.getProperty(gameArray[i], ['offerInfos', 'offerInfo']);
                        olen = offerInfoArray.length;
                        for (j = 0; j < olen; j++) {
                            offerIdToItemId[offerInfoArray[j].offerId] = offerInfoArray[j].itemId;
                        }
                    }
                }
                offerIdToItemIdCreated = true;
            }
        }

        function processUserVaultInfoResponse(response) {
            userSubsInfo.userVault = response;
        }

        function handleUserVaultInfoResponseError(error) {
            ComponentsLogFactory.error('SUBSCRIPTION - retrieveUserVaultInfoResponseError:', error);
        }

        function retrieveUserVaultInfo() {
            if (bUserHasBasicInfo) {
                return Origin.subscription.getUserVaultInfo()
                    .then(processUserVaultInfoResponse)
                    .catch(handleUserVaultInfoResponseError);
            } else {
                return Promise.resolve();      //user is not a subscriber, just resolve the promise
            }
        }

        function processUserTrialInfoResponse(response) {
            bUserHasUsedSubscriptionTrial = _.get(response, 'hasUsedTrial', false);

            return response;
        }

        function handleUserTrialInfoResponseError(error) {
            ComponentsLogFactory.error('SUBSCRIPTION - retrieveUserTrialInfoError:', error);
        }

        function retrieveUserTrialInfo() {
            return Origin.subscription.hasUsedTrial()
                .then(processUserTrialInfoResponse)
                .catch(handleUserTrialInfoResponseError);
        }

        function handleRetrieveSubsInfoError(error) {
            ComponentsLogFactory.error('SUBSCRIPTION - retrieveUserSubsInfoError:', error);
        }

        function retrieveSubscriptionInfo() {
            //we need to clear out our global variables each time
            //the calls below have side effects by storing state in globals in this function
            //so we have to clear them
            clearSubscriptionInfo();

            return retrieveUserBasicInfo()
                .then(retrieveUserDetails)
                .then(retrieveUserVaultInfo)
                .then(retrieveUserTrialInfo)
                .catch(handleRetrieveSubsInfoError);
        }

        function matchesOfferId(offerId) {
            return function(item) {
                return item === offerId;
            };
        }

        function getMatches(offerArray, offerId) {
            var match = [];
            if (offerArray) {
                match = offerArray.filter(matchesOfferId(offerId));
            }
            return match;
        }

        function inUsersVault(offerId) {
            var i,
                len,
                gameArray,
                match = [],
                offerArray;

            if (!userSubsInfo.userVault) {
                return false;
            }

            len = userSubsInfo.userVault.game.length;

            gameArray = userSubsInfo.userVault.game;

            for (i = 0; i < len; i++) {
                //check basegames first
                offerArray = Origin.utils.getProperty(gameArray[i], ['basegames', 'basegame']);
                match = getMatches(offerArray, offerId);

                //no match with basegames, try extra content
                if (match.length === 0) {
                    offerArray = Origin.utils.getProperty(gameArray[i], ['extracontents', 'extracontent']);
                    match = getMatches (offerArray, offerId);
                }

                if (match.length > 0) {
                    return true;
                }
            }
            return false;
        }

        function getVaultGameFromMasterTitleId(masterTitleId) {
            if (!masterTitleIdToVaultGameCreated) {
                createMasterTitleToGameMapping();
            }

            return masterTitleIdToVaultGame[masterTitleId];
        }

        function getItemIdFromOfferId(offerId) {
            if (!offerIdToItemIdCreated) {
                createOfferIdtoItemIdMapping();
            }

            return offerIdToItemId[offerId];
        }

        function getItemIds() {
            if (!offerIdToItemIdCreated) {
                createOfferIdtoItemIdMapping();
            }

            return _.values(offerIdToItemId);
        }

        function clearSubscriptionInfo() {
            userSubsInfo = {};
            bSubsDetailRetrieved = false;
            bUserHasSubscription = false;
            bUserHasBasicInfo = false;
            bSubscriptionInfoRetrieved = false;
            bUserHasUsedSubscriptionTrial = false;
        }

        // data/state  accessor functions
        function getFirstSignUpDate() {
            if (bUserHasSubscription && bUserHasBasicInfo) {
                return userSubsInfo.basicInfo.firstSignUpDate;
            }
            return null;
        }

        function isActive() {
            if(bUserHasSubscription && !Origin.client.onlineStatus.isOnline() && offlinePlayRemaining() <= 0) {
                return false;
            } else {
                return bUserHasSubscription;
            }
        }

        function isPendingExpired() {
            return getStatus() === SUBSCRIPTION_STATE_PENDINGEXPIRED;
        }

        function isExpired() {
            return getStatus() === SUBSCRIPTION_STATE_EXPIRED;
        }

        function isSuspended() {
            return getStatus() === SUBSCRIPTION_STATE_SUSPENDED;
        }

        function isFreeTrial() {
            if (bUserHasSubscription) {
                return userSubsInfo.detailInfo.Subscription.freeTrial;
            }
            return false;
        }

        function getNextBillingDate() {
            if (bUserHasSubscription && bSubsDetailRetrieved) {
                return userSubsInfo.detailInfo.Subscription.nextBillingDate;
            }
            return null;
        }

        function getStatus() {
            if (bSubsDetailRetrieved) {
                return userSubsInfo.detailInfo.Subscription.status;
            }
            return 'UNKNOWN';
        }

        function getUserLastSubscriptionOfferId() {
            if (bSubsDetailRetrieved) {
                var offerUri = Origin.utils.getProperty(userSubsInfo, ['detailInfo', 'Subscription', 'offerUri']) || undefined;

                if (offerUri) {
                    return offerUri.split('/').slice(-1)[0];
                }
            }
            return undefined;
        }

        function expiration() {
            var detail = userSubsInfo.detailInfo,
                subsDetail = detail.Subscription,
                scheduleOperation = Origin.utils.getProperty(detail, ['GetScheduleOperationResponse', 'ScheduleOperation']),
                subsEndMS,
                expirationDate,
                expirationSet = false;

            if (scheduleOperation) {
                if (subsDetail.status === 'ENABLED') {
                    angular.forEach(scheduleOperation, function(operation){
                        //forEach doesn't allow break or return out of the loop
                        if (operation.operationName === 'RENEWED' && !expirationSet) {
                            expirationDate = new Date(operation.scheduleDate);
                            expirationSet = true;
                        }
                    });
                }

                if (!expirationSet && subsDetail.status === SUBSCRIPTION_STATE_PENDINGEXPIRED) {
                    angular.forEach (scheduleOperation, function(operation) {
                        if (operation.operationName === 'CANCELLED' && !expirationSet) {
                            expirationDate = new Date(operation.scheduleDate);
                            expirationSet = true;
                        }
                    });
                }
            }

            if (!expirationSet) {
                //add one date to the subscription
                subsEndMS = Date.parse(subsDetail.subsEnd);
                subsEndMS += ONE_DAY_MS;

                expirationDate = new Date(subsEndMS);
            }
            return expirationDate;
        }

        //returns minutes not milliseconds
        function offlinePlayRemaining() {
            var remaining,
                offlineRemaining,
                expireRemaining;

            //TODO: need to rework this logic to take into account trusted clock and offline
            offlineRemaining = SUBSCRIPTION_MAX_OFFLINE_PLAY_TIME;
            expireRemaining = expiration() - Date.now();

            remaining = offlineRemaining < expireRemaining ? offlineRemaining/1000 : expireRemaining/1000;
            return remaining;
        }

        function getTimeRemaining () {
            var remainingTime;

            if (bUserHasSubscription) {
                //need trusted clock...
                remainingTime = expiration() - Date.now();
                return remainingTime < 0 ? 0: remainingTime;
            }
            return -1;
        }

        function vaultEntitle(offerId) {
            return Origin.subscription.vaultEntitle(Origin.utils.getProperty(userSubsInfo, ['basicInfo', 'subscriptionId']), offerId);
        }

        function vaultRemove(subscriptionId, offerId) {
            return Origin.subscription.vaultRemove(subscriptionId, offerId);
        }

        /**
         * Get reason for subscription cancellation
         * @return {string} cancellation reason
         * @getCancelReason getCancelReason
         */
        function getCancelReason() {
            var properties = Origin.utils.getProperty(userSubsInfo, ['detailInfo', 'Subscription', 'PropertiesInfo']),
                reason = null;

            if (_.isArray(properties)) {
                _.forEach(properties, function(property) {
                    if (property.name === SUBSCRIPTION_CANCEL_REASON) {
                        reason = property.value;
                    }
                });
            }

            return reason;
        }

        /**
         * Get billing mode
         * @return {string} How the subscription is going to be billed. Ex 'Recurring'
         * @getBillingMode getBillingMode
         */
        function getBillingMode() {
            return Origin.utils.getProperty(userSubsInfo, ['detailInfo', 'Subscription', 'billingMode']);
        }

        /**
         * Get has retrieved the basic user subscription info
         * @return {boolean} true if info retrieved, false otherwise
         */
        function getSubscriptionInfoRetrieved() {
            return bSubscriptionInfoRetrieved;
        }

        // end data/state accessor functions


        // handle subscription update events

        function fireSubscriptionUpdateEvent() {
            subscriptionCommunicator.fire('SubscriptionFactory:subscriptionUpdate');
        }


        function onDirtyBitsSubscriptionUpdate() {
            retrieveSubscriptionInfo().then(fireSubscriptionUpdateEvent);
        }

        Origin.events.on(Origin.events.DIRTYBITS_SUBSCRIPTION, onDirtyBitsSubscriptionUpdate);

        // end subscription update event handling

        function hasUsedTrial() {
            return bUserHasUsedSubscriptionTrial;
        }

        /**
         * Return whether user is on a platform that supports Access
         * @return {Boolean}
         */
        function userIsOnSupportedPlatform() {
            return _.indexOf(SUPPORTED_PLATFORMS, Origin.utils.os()) >= 0;
        }

        return {
            events: subscriptionCommunicator,

            /**
             * returns promise for retrieving the user's subscription info, resolves promise once it's all retrieved
             * @return {void}
             * @method retrieveSubscriptionInfo
             */
            retrieveSubscriptionInfo: retrieveSubscriptionInfo,

            /**
             * clears the in-RAM cache of subscription information
             * @return {void}
             * @method clearSubscriptionInfo
             */
            clearSubscriptionInfo: clearSubscriptionInfo,

            /**
             * if the user is a subscriber, returns the first sign up date, if not a subscriber returns null
             * @return {Date}
             * @method firstSignUpDate
             */
            getfirstSignUpDate: getFirstSignUpDate,

            /**
             * given an offerId, returns whether the game is in user's vault
             * @param {string} offerId
             * @return {boolean}
             * @method inUsersVault
             */
            inUsersVault: inUsersVault,

            /**
             * returns whether user's subscription is active
             * @return {boolean}
             * @method isActive
             */
            isActive: isActive,

            /**
             * returns whether user's subscription is expired
             * @return {boolean}
             * @method isExpired
             */
            isExpired: isExpired,

            /**
             * returns whether user's subscription is pending expired
             * @return {boolean}
             * @method isPendingExpired
             */
            isPendingExpired: isPendingExpired,

            /**
             * returns whether user's subscription is suspended
             * @return {boolean}
             * @method isSuspended
             */
            isSuspended: isSuspended,

            /**
             * returns whether subscription is in the free trial period
             * @return {boolean}
             * @method isFreeTrial
             */
            isFreeTrial: isFreeTrial,

            /**
             * if the user is a subscriber, returns the next billing date; if not subscriber, it will return null
             * @return {Date}
             * @method nextBillingDate
             */
            getNextBillingDate: getNextBillingDate,

            /**
             * returns the status of the current subscription
             * @return {string}
             * @method status
             */
            getStatus: getStatus,

            /**
             * returns in milliseconds the amount of time remaining on the subscription, 0 if expired, if no subscription, returns -1
             * @return {integer}
             * @method timeRemaining
             */
            getTimeRemaining: getTimeRemaining,

            /**
             * returns whether user is a subscriber or not
             * @return {boolean}
             * @method userHasSubscription
             */
            userHasSubscription: function() {
                return bUserHasSubscription;
            },

            /**
             * Return whether user is on a platform that supports Access
             * @return {Boolean}
             */
            userIsOnSupportedPlatform: userIsOnSupportedPlatform,

            /**
             * Give vault game entitlement
             * @param {string} offerId Vault offer to add to user entitlements
             * @return {object} fullfillment response object (includes entitlementUris and subscriptionUris)
             * @method vaultEntitle
             */
            vaultEntitle: vaultEntitle,

            /**
             * Remove vault game entitlement
             * NOTE: do not call directly, call from GamesDataFactory
             *
             * @param {string} subscriptionId The subscriptionId used to aquire the entitlement
             * @param {string} offerId Vault offer to remove from user entitlements
             * @return {object} simple success object - success property is true or false
             * @method vaultRemove
             */
            vaultRemove: vaultRemove,

            /**
             * get users subscription info
             * @return {object} users subscriotion information
             * @method getUserSubscriptionInfo
             */
            getUserSubscriptionInfo: function() {
                return userSubsInfo;
            },

            /**
             * get last subscription offer ID for user
             * @return {string} offer ID
             */
            getUserLastSubscriptionOfferId: getUserLastSubscriptionOfferId,

            /**
             * Get reason for subscription cancellation
             * @return {string} cancellation reason
             * @getCancelReason getCancelReason
             */
            getCancelReason: getCancelReason,

            /**
             * Get the expiration date of users subscription
             * @return {date} expiration date
             * @getExpiration expiration
             */
            getExpiration: expiration,

            /**
             * Get billing mode
             * @return {string} How the subscription is going to be billed. Ex 'Recurring'
             * @getBillingMode getBillingMode
             */
            getBillingMode: getBillingMode,

            /**
             * Get has retrieved the basic user subscription info
             * @return {boolean} true if info retrieved, false otherwise
             */
            getSubscriptionInfoRetrieved: getSubscriptionInfoRetrieved,

            /**
             * Get the vault game object that the given masterTitleId should upgrade to.
             * This is retrieved from the 'masterTitleIdToVaultGame' map.
             * @param {string} masterTitleId masterTitleId of the offer
             * @return {gameObject} vault game object
             * @method getVaultGameFromMasterTitleId
             */
            getVaultGameFromMasterTitleId: getVaultGameFromMasterTitleId,

            /**
             * Get itemId of a given offer from the 'offerIdToItemId' map
             * @param {string} offerId offerId
             * @return {string} itemId of offer
             * @method getItemIdFromOfferId
             */
            getItemIdFromOfferId: getItemIdFromOfferId,

            /**
             * Get all item ids of 'offerIdToItemId' map
             * @return {Array} list of item ids
             * @method getItemIds
             */
            getItemIds: getItemIds,

            /**
             * returns whether user has used the subscription free trial yet
             * @return {boolean}
             * @method hasUsedFreeTrial
             */
            hasUsedFreeTrial: hasUsedTrial,

            /**
             * promise method for updating the users trial status
             * @return {Promise}
             * @method retrieveUserTrialInfo
             */
            retrieveUserTrialInfo: retrieveUserTrialInfo
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function SubscriptionFactorySingleton(ComponentsLogFactory, ObjectHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('SubscriptionFactory', SubscriptionFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.UserDataFactory

     * @description
     *
     * UserDataFactory
     */
    angular.module('origin-components')
        .factory('SubscriptionFactory', SubscriptionFactorySingleton);

}());
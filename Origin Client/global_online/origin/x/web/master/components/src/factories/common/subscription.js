/**
 * @file common/subscription.js
 * handles subscription logic both user-related and game-related
 */
(function() {
    'use strict';

    function SubscriptionFactory(ComponentsLogFactory, ObjectHelperFactory) {
        var userSubsInfo = {},
            bSubsDetailRetrieved = false,
            bUserHasSubscription = false,
            bUserHasBasicInfo = false,
            ONE_DAY_MS = 24 * 60 * 60 * 1000, //one day in Milliseconds
            SUBSCRIPTION_MAX_OFFLINE_PLAY_TIME = 30 * 24 * 60 * 60 * 1000, //30 days
            SUBSCRIPTION_STATE_NORMAL = 'ENABLED,PENDINGEXPIRED',
            SUBSCRIPTION_STATE_EXPIRED = 'EXPIRED',
            expirationChecked = false;

        function parseBasicResponse (response) {
            userSubsInfo.basicInfo = {};
            userSubsInfo.basicInfo.firstSignUpDate = new Date (response.firstSignUpDate);
            userSubsInfo.basicInfo.firstSignUpSubs = response.firstSignUpSubs;
            userSubsInfo.basicInfo.subscriptionUri = ObjectHelperFactory.copy(response.subscriptionUri);
        }

        function handleUserBasicInfo(response) {
            var error, len;

            if (response) {
                if (response.hasOwnProperty('subscriptionUri')) {
                    len = response.subscriptionUri.length;
                    if (len === 0) {
                       error = new Error('ERROR: basic subscription info missing subscriptionUri');
                       return Promise.reject(error);
                    } else {
                        bUserHasBasicInfo = true;
                        parseBasicResponse(response);
                    }
                }
            }
            return response;
        }

        function handleUserBasicInfoError(error) {
            ComponentsLogFactory.error('SUBSCRIPTION - retrieveUserBasicInfo:', error.message);
        }

        function retrieveUserBasicInfo() {
            expirationChecked = false;  //reset
            return Origin.subscription.userSubscriptionBasic(SUBSCRIPTION_STATE_NORMAL)
                .then(handleUserBasicInfo)
                .then(checkForExpired)
                .catch(handleUserBasicInfoError);
        }

        function checkForExpired(response) {
            //check once to see if the subs account has expired
            if (response && !bUserHasBasicInfo && !expirationChecked && response.hasOwnProperty('firstSignUpDate') && response.firstSignUpDate !== '') { //if we haven't yet checked expiration
                expirationChecked = true;
                return Origin.subscription.userSubscriptionBasic(SUBSCRIPTION_STATE_EXPIRED)
                    .then(handleUserBasicInfo)
                    .catch(handleUserBasicInfoError);
            }
        }

        function processUserSubDetailResponse(response) {
            var detailInfo = response;

            //convert all dates to date Object
            detailInfo.Subscription.dateCreated = new Date (detailInfo.Subscription.dateCreated);
            detailInfo.Subscription.dateModified = new Date (detailInfo.Subscription.dateModified);
            detailInfo.Subscription.subsStart = new Date(detailInfo.Subscription.subsStart);
            detailInfo.Subscription.subsEnd = new Date (detailInfo.Subscription.subsEnd);
            detailInfo.Subscription.nextBillingDate = new Date (detailInfo.Subscription.nextBillingDate);
            userSubsInfo.detailInfo = detailInfo;
            if (detailInfo.Subscription.subscriptionUri !== '') {
                bUserHasSubscription = true;
            }
            bSubsDetailRetrieved = true;
        }

        function handleUserSubDetailsError(error) {
            ComponentsLogFactory.error('SUBSCRIPTION - userSubscriptionDetailsError:', error.message);
        }

        function retrieveUserDetails() {
            //extract out the current subscription uri
            var uri, len, error;

            if (bUserHasBasicInfo) {
                len = userSubsInfo.basicInfo.subscriptionUri.length;
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

        function processUserVaultInfoResponse(response) {
            userSubsInfo.userVault = response;
        }

        function handleUserVaultInfoResponseError(error) {
            ComponentsLogFactory.error('SUBSCRIPTION - retrieveUserVaultInfoResponseError:', error.message);
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

        function handleRetrieveSubsInfoError(error) {
            ComponentsLogFactory.error('SUBSCRIPTION - retrieveUserSubsInfoError:', error.message);
        }

        function retrieveSubscriptionInfo() {
            return retrieveUserBasicInfo()
                .then(retrieveUserDetails)
                .then (retrieveUserVaultInfo)
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


        function clearSubscriptionInfo() {
            userSubsInfo = {};
            bSubsDetailRetrieved = false;
            bUserHasSubscription = false;
            bUserHasBasicInfo = false;
        }


        // data/state  accessor functions
        function getFirstSignUpDate() {
            if (bUserHasSubscription && bUserHasBasicInfo) {
                return userSubsInfo.basicInfo.firstSignUpDate;
            }
            return null;
        }

        function isActive() {
            if (bUserHasSubscription && (offlinePlayRemaining() > 0 || getTimeRemaining() > 0 )) {
                return (userSubsInfo.detailInfo.Subscription.status === 'ENABLED' || userSubsInfo.detailInfo.Subscription.status === 'PENDINGEXPIRED');
            }
            return false;
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
            if (bUserHasSubscription && bSubsDetailRetrieved) {
                return userSubsInfo.detailInfo.Subscription.status;
            }
            return 'UNKNOWN';
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

                if (!expirationSet && subsDetail.status === 'PENDINGEXPIRED') {
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
        // end data/state accessor functions

        return {
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
            }
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
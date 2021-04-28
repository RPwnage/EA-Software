/**
 * @file refiners/vault.js
 */
(function() {

    'use strict';

    var MAX_DISPLAYABLE_TRIAL_TIME = 60 * 60 * 24 * 30 * 6; // Approximately 6 months
    var ORIGIN_ACCESS = 'origin-access';
    var FREE_TRIAL = 'free-trial';

    function VaultRefiner(DateHelperFactory) {
        var isBetweenDates = DateHelperFactory.isBetweenDates,
            subtractDays = DateHelperFactory.subtractDays,
            isInThePast = DateHelperFactory.isInThePast;

        /**
         * Get the offline play expiration date fom the client data
         * @param  {Object} clientInfo the client info to analyze
         * @return {mixed} the date or undefined
         */
        function getOfflinePlayExpirationDate(clientInfo) {
            return new Date(_.get(clientInfo, 'offlinePlayExpirationDate'));
        }

        /**
         * Test if offline play of vault content is expiring
         * @param {number} withinDays specify the number of days until expiry to check
         * @return {Function}
         */
        function isOfflinePlayExpiryImminent(withinDays) {
            /**
             * @param {Object} clientInfo the game data as returned by the client
             * @return {Boolean}
             */
            return function(clientInfo) {
                var offlinePlayExpirationDate = getOfflinePlayExpirationDate(clientInfo);

                return isBetweenDates(subtractDays(offlinePlayExpirationDate, withinDays), offlinePlayExpirationDate);
            };
        }

        /**
         * Test if offline play of vault content has expired
         * @param {Object} clientInfo the game data as returned by the client
         * @return {Boolean}
         */
        function isOfflinePlayExpired(clientInfo) {
            return isInThePast(getOfflinePlayExpirationDate(clientInfo));
        }

        /**
         * Vault trials require a particular schema
         * @param {Object} trialTimeInfo the time remaining data from the subscription service
         * @return {Boolean}
         */
        function isValidTrial(trialTimeInfo) {
            return _.has(trialTimeInfo, 'hasTimeLeft');
        }

        /**
         * Is the game trial expired?
         * @param {Object} trialTimeInfo the time remaining data from the subscription service
         * @return {Boolean}
         */
        function isTrialExpired(trialTimeInfo) {
            if (!isValidTrial(trialTimeInfo)) {
                return false;
            }

            return _.get(trialTimeInfo, 'hasTimeLeft') ? false : true;
        }

        /**
         * Is the trial duration within the displayable range?
         * @param {Object} trialTimeInfo the time remaining data from the subscription service
         * @return {Boolean}
         */
        function isTrialDurationDisplayable(trialTimeInfo) {
            //this was added to address Titanfall2 custom use of our trial tech
            return _.get(trialTimeInfo, 'totalTrialSec') <= MAX_DISPLAYABLE_TRIAL_TIME;
        }

        /**
         * Is the game trial expired, not released and not hidden?
         * @param {Object} trialTimeInfo the time remaining data from the subscription service
         * @return {Boolean}
         */
        function isTrialExpiredAndVisibleNotReleased(trialTimeInfo, hidden, released) {
            return (!hidden && !released && isTrialExpired(trialTimeInfo));
        }

        /**
         * Is the game trial expired, not released and hidden?
         * @param {Object} trialTimeInfo the time remaining data from the subscription service
         * @return {Boolean}
         */
        function isTrialExpiredAndHiddenNotReleased(trialTimeInfo, hidden, released) {
            return (hidden && !released && isTrialExpired(trialTimeInfo));
        }

        /**
         * Is the game trial expired, released and not hidden?
         * @param {Object} trialTimeInfo the time remaining data from the subscription service
         * @return {Boolean}
         */
        function isTrialExpiredAndVisibleReleased(trialTimeInfo, hidden, released) {
            return (!hidden && released && isTrialExpired(trialTimeInfo));
        }

        /**
         * Is the game trial expired, released and hidden?
         * @param {Object} trialTimeInfo the time remaining data from the subscription service
         * @return {Boolean}
         */
        function isTrialExpiredAndHiddenReleased(trialTimeInfo, hidden, released) {
            return (hidden && released && isTrialExpired(trialTimeInfo));
        }

        /**
         * Is an OA trial game awaiting activation?
         * @param {Object} trialTimeInfo the time remaining data from the subscription service
         * @return {Boolean}
         */
        function isTrialAwaitingActivation(trialTimeInfo) {
            if (!isValidTrial(trialTimeInfo) || !isTrialDurationDisplayable(trialTimeInfo)) {
                return false;
            }

            return (!isTrialExpired(trialTimeInfo) &&
                _.get(trialTimeInfo, 'leftTrialSec') === _.get(trialTimeInfo, 'totalTrialSec')
            ) ? true : false;
        }

        /**
         * Is an OA game trial in progress?
         * @param {Object} trialTimeInfo the time remaining data from the subscription service
         * @return {Boolean}
         */
        function isTrialInProgress(trialTimeInfo) {
            if (!isValidTrial(trialTimeInfo) || !isTrialDurationDisplayable(trialTimeInfo)) {
                return false;
            }

            return (!isTrialExpired(trialTimeInfo) &&
                _.get(trialTimeInfo, 'leftTrialSec') !== _.get(trialTimeInfo, 'totalTrialSec')
            ) ? true : false;
        }

        function isTrialInProgressNotReleased(trialTimeInfo, isReleased) {
            return !isReleased && isTrialInProgress(trialTimeInfo);
        }

        function isTrialInProgressReleased(trialTimeInfo, isReleased) {
            return isReleased && isTrialInProgress(trialTimeInfo);
        }

        function isOfflineTrial(item, online) {
            if (!item.earlyAccess && !item.trial) {
                return false;
            }

            return !online;
        }

        /**
         * check if the subscriber has been active for less than daysActive
         * @param  {string} daysActive     the targeted days active
         * @return {boolean}               true if there's a match
         */
        function isSubsActiveForLessThanNumDays(firstSignUpDate, daysActive) {
            //add the targeting threshold days e.g. 30 to the firstSignUp date
            var targetedEndDate = DateHelperFactory.addDays(firstSignUpDate, daysActive);

            //see if the current date falls between the firstSignUpDate and the targetedEndDate;
            return DateHelperFactory.isBetweenDates(firstSignUpDate, targetedEndDate);
        }

        /**
         * check if the subscriber has been active for daysActive
         * @param  {string} daysActive     the targeted days active
         * @return {boolean}               true if there's a match
         */
        function isSubsActiveForAtLeastNumDays(firstSignUpDate, daysActive) {
            return !isSubsActiveForLessThanNumDays(firstSignUpDate, daysActive);
        }

        /**
         * check if the subscriber has an expired subscription for daysCanceled
         * @param  {string} daysCanceled   the targeted days canceled
         * @return {boolean}               true if there's a match
         */
        function isSubsExpiredForAtLeastNumDays(expiredDate, daysCanceled) {
           return  DateHelperFactory.isInTheFuture(DateHelperFactory.addDays(expiredDate, daysCanceled));
        }
        /**
         * Is a base game eligible for an edition upgrade?
         * @param {Boolean} isActiveSubscription true if the user is an active subscriber
         * @param {Boolean} isVaultEditionOwned true if the vault edition has already been entitled
         * @param {Boolean} isMissingBasegameVaultEditionOrDLC true if missing base game vault edition or any DLCs
         * @return {Boolean}
         */
        function isGameEligibleForUpgrade(isActiveSubscription, isVaultEditionOwned, isMissingBasegameVaultEditionOrDLC, isUpgradeableGame) {
            if(!isActiveSubscription || isVaultEditionOwned || !isUpgradeableGame) {
                return false;
            }

            return isMissingBasegameVaultEditionOrDLC;
        }

        /**
         * check if the an offer is a shell bundle
         * note: checks pre and post offerTransformer data
         *
         * @param  {string} product catalog data
         * @return {boolean} true if offer is a shell bundle
         */
        function isShellBundle(product) {
            if ((_.get(product, 'subscriptionAvailable') && _.get(product, 'vaultOcdPath') !== _.get(product, 'ocdPath')) || (_.get(product, 'vault') && _.get(product, 'offerPath') !== _.get(product, ['vaultInfo', 'path']))){
                return true;
            } else {
                return false;
            }
        }

        /**
         * Check if the offer is upgradeable to the vault game
         * @param {onject} catalogInfo the offer to analyze
         * @return {boolean} true if upgradeable
         */
        function isUpgradeable(catalogInfo) {
            return _.get(catalogInfo, 'isUpgradeable');
        }

        /**
         * Check if the offer is a subscriptions free trial
         * @param {onject} catalogInfo the offer to analyze
         * @return {boolean} true if free trial offer
         */
        function isSubscriptionFreeTrial(catalogInfo) {
            return (_.get(catalogInfo, 'ocdPath', '').indexOf(FREE_TRIAL) > 0 || _.get(catalogInfo, 'offerPath', '').indexOf(FREE_TRIAL) > 0) && 
                    (_.get(catalogInfo, 'gameNameFacetKey', '') === ORIGIN_ACCESS || _.get(catalogInfo, 'gameKey', '') === ORIGIN_ACCESS);
        }

        return {
            isSubsActiveForLessThanNumDays: isSubsActiveForLessThanNumDays,
            isSubsActiveForAtLeastNumDays: isSubsActiveForAtLeastNumDays,
            isSubsExpiredForAtLeastNumDays: isSubsExpiredForAtLeastNumDays,
            isOfflinePlayExpiryImminent: isOfflinePlayExpiryImminent,
            isOfflinePlayExpired: isOfflinePlayExpired,
            isTrialExpired: isTrialExpired,
            isTrialExpiredAndVisibleNotReleased: isTrialExpiredAndVisibleNotReleased,
            isTrialExpiredAndHiddenNotReleased: isTrialExpiredAndHiddenNotReleased,
            isTrialExpiredAndVisibleReleased: isTrialExpiredAndVisibleReleased,
            isTrialExpiredAndHiddenReleased: isTrialExpiredAndHiddenReleased,
            isTrialAwaitingActivation: isTrialAwaitingActivation,
            isTrialInProgress: isTrialInProgress,
            isTrialInProgressNotReleased: isTrialInProgressNotReleased,
            isTrialInProgressReleased: isTrialInProgressReleased,
            isOfflineTrial: isOfflineTrial,
            isGameEligibleForUpgrade: isGameEligibleForUpgrade,
            isValidTrial: isValidTrial,
            isShellBundle: isShellBundle,
            isUpgradeable: isUpgradeable,
            isSubscriptionFreeTrial: isSubscriptionFreeTrial
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.refiners.VaultRefiner

     * @description
     *
     * Helpful methods for filtering content based on vault satus for OA games
     */
    angular.module('origin-components')
        .factory('VaultRefiner', VaultRefiner);
})();
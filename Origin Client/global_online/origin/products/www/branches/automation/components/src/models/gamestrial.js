/**
 * @file game/gamestrial.js
 */
(function() {
    'use strict';

    function GamesTrialFactory(GamesCatalogFactory, GamesEntitlementFactory, AuthFactory, CacheFactory, VaultRefiner, LegacyTrialRefiner, GameRefiner) {
        var NO_TIME = 0;


        /**
         * @typedef getTimeResponseObject
         * @type {object}
         * @property {boolean} hasTimeLeft user has time left in trial
         * @property {integer} leftTrialSec time left in trial (in seconds)
         * @property {integer} totalTrialSec total time for trial (in seconds)
         * @property {integer} totalGrantedSec total time granted (in seconds)
         */
        /**
         * Get the remaining play time for a users trial offer
         * @param  {string} contentId content ID of product
         * @return {getTimeResponseObject} getTime response object
         */
        var getTrialTime = function(contentId) {
            return Origin.trial.getTime(contentId);
        };

        var getCachedTrialTime = CacheFactory.decorate(getTrialTime);

        /**
         * Get the trial time if this is a trial, return undefined if it is not
         * @param {object} catalogInfo the catalogInfo object
         * @return{getTimeResponseObject} getTime response object
         */
        var getTrialTimeIfTrial = function(catalogInfo) {
            //Only query the jssdk if we know this is a trial
            if (AuthFactory.isClientOnline() && AuthFactory.isAppLoggedIn() && (GameRefiner.isEarlyAccess(catalogInfo) || GameRefiner.isTrial(catalogInfo))) {
                return getCachedTrialTime(catalogInfo.contentId);
            }
            return Promise.resolve(undefined);
        };

        /**
         * Get the remaining play time for a user's trial offer (when passed offerId)
         * @param  {string} offerID offer Id of product
         * @return {getTimeResponseObject} getTime response object, or undefined if not a trial
         */
        var getTrialTimeByOfferId = function(offerId) {
            return GamesCatalogFactory.getCatalogInfo([offerId])
                .then(function(catalogInfo) {
                    return getTrialTimeIfTrial(catalogInfo[offerId]);
                });
        };

        function getTrialDependencies(offerId) {
            return GamesCatalogFactory.getCatalogInfo([offerId])
                .then(function(catalogInfo) {
                    return Promise.all([
                        Promise.resolve(catalogInfo[offerId]),
                        GamesEntitlementFactory.getEntitlement(offerId),
                        getTrialTimeIfTrial(catalogInfo[offerId])
                    ]);
                });
        }

        function trialAwaitingActivation(catalogInfo,entitlementInfo,timeInfo) {
            return Promise.resolve(
                        VaultRefiner.isTrialAwaitingActivation(timeInfo) || 
                        LegacyTrialRefiner.isTrialAwaitingActivation(catalogInfo,entitlementInfo)
                    );
          }

        function isTrialAwaitingActivation(offerId) {
            return getTrialDependencies(offerId)
                .then(_.spread(trialAwaitingActivation));
        }

        function trialInProgress(catalogInfo,entitlementInfo,timeInfo) {
            return Promise.resolve(
                    VaultRefiner.isTrialInProgress(timeInfo) ||
                    LegacyTrialRefiner.isTrialInProgress(catalogInfo,entitlementInfo)
                );
        }

        function isTrialInProgress(offerId) {
            return getTrialDependencies(offerId)
                .then(_.spread(trialInProgress));
        }


        function trialExpired(catalogInfo,entitlementInfo,timeInfo) {
            return Promise.resolve(
                    VaultRefiner.isTrialExpired(timeInfo) ||
                    LegacyTrialRefiner.isTrialExpired(catalogInfo,entitlementInfo)
                );
        }

        function isTrialExpired(offerId) {
            return getTrialDependencies(offerId)
                .then(_.spread(trialExpired));
        }


        function trialTimeRemaining(catalogInfo,entitlementInfo,timeInfo) {
            if (VaultRefiner.isValidTrial(timeInfo)) {
                return Promise.resolve(_.get(timeInfo, 'leftTrialSec'));
            } else {
                return Promise.resolve(NO_TIME);
            }
        }

        function getTrialTimeRemaining(offerId) {
            return getTrialDependencies(offerId)
                .then(_.spread(trialTimeRemaining));
        }

        function trialEndDate(catalogInfo,entitlementInfo) {
            if (LegacyTrialRefiner.isLegacyTrial(catalogInfo)) {
                return Promise.resolve(_.get(entitlementInfo, 'terminationDate'));
            } else {
                return Promise.resolve(undefined);
            }
        }

        function getTrialEndDate(offerId) {
            return getTrialDependencies(offerId)
                .then(_.spread(trialEndDate));
        }

        function trialTotalTime(catalogInfo,entitlementInfo,timeInfo) {
            if (VaultRefiner.isValidTrial(timeInfo)) {
                return Promise.resolve(_.get(timeInfo, 'totalTrialSec'));
            } else if (LegacyTrialRefiner.isLegacyTrial(catalogInfo)) {
                return Promise.resolve(_.get(catalogInfo,'trialLaunchDuration'));
            } else {
                return Promise.resolve(NO_TIME);
            }
        }
        
        function getTrialTotalTime(offerId) {
            return getTrialDependencies(offerId)
                .then(_.spread(trialTotalTime));
        }



        return {
            /**
             * @typedef getTimeResponseObject
             * @type {object}
             * @property {boolean} hasTimeLeft user has time left in trial
             * @property {integer} leftTrialSec time left in trial (in seconds)
             * @property {integer} totalTrialSec total time for trial (in seconds)
             * @property {integer} totalGrantedSec total time granted (in seconds)
             */
            /**
             * Get the remaining play time for a user's trial offer (when passed offerId)
             * @param  {string} offerID offer Id of product
             * @return {getTimeResponseObject} getTime response object, or undefined if not a trial
             */
            getTrialTimeByOfferId: getTrialTimeByOfferId,

            /**
             * Check if this offer is a trial awaiting activation
             * @param {string} offerId the offerID of product
             * @return {boolean} if the trial is awaiting activation
             * @method isTrialAwaitingActivation
             */
            isTrialAwaitingActivation:isTrialAwaitingActivation,
            /**
             * Check if this offer is a trial that has started but not completed
             * @param {string} offerId the offerID of product
             * @return {boolean} if the trial is in progress
             * @method isTrialInProgress
             */
            isTrialInProgress: isTrialInProgress,
            /**
             * Check if this offer is a trial that has expired (no longer available)
             * @param {string} offerId the offerID of product
             * @return {boolean} if the trial is expired
             * @method isTrialExpired
             */
            isTrialExpired: isTrialExpired,
            /**
             * Check the amount of trial time remaining, 
             * returns 0 if there is none or if the trial is not time-based
             * @param {string} offerId the offerID of product
             * @return {int} the trial time remaining (in seconds), or 0 if trial is not time-based
             * @method getTrialTimeRemaining
             */
            getTrialTimeRemaining: getTrialTimeRemaining,
            /**
             * Check when this trial should end
             * @param {string} offerId the offerID of product
             * @return {Date} The end date, or undefined if no end date 
             * @method getTrialEndDate
             */
            getTrialEndDate: getTrialEndDate,
            /**
             * Check how many seconds the trial should last (in seconds)
             * @param {string} offerId the offerID of product
             * @return {int} the total trial time (in seconds), or 0 if not time-based
             * @method getTrialTotalTime
             */
            getTrialTotalTime: getTrialTotalTime
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesTrialFactorySingleton(GamesCatalogFactory, GamesEntitlementFactory, AuthFactory, CacheFactory, VaultRefiner, LegacyTrialRefiner, GameRefiner, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesTrialFactory', GamesTrialFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesTrialFactory
     * @description
     *
     * GamesTrialFactory
     */
    angular.module('origin-components')
        .factory('GamesTrialFactory', GamesTrialFactorySingleton);
}());

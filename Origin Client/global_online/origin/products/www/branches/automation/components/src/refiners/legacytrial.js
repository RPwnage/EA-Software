/**
 * @file refiners/vault.js
 */
(function() {

    'use strict';

    function LegacyTrialRefiner(DateHelperFactory, GameRefiner) {
        var isInTheFuture = DateHelperFactory.isInTheFuture,
            isInThePast = DateHelperFactory.isInThePast;

                /**
         * Legacy trials are PC only and part of the limited trial game distribution subtype
         * @return {Boolean}
         */
        function isLegacyTrial(catalogInfo) {
            return (Origin.utils.os() === 'PCWIN' && GameRefiner.isLimitedTrial(catalogInfo));
        }

        /**
         * Is a trial game awaiting activation?
         * @param {Object} catalogInfo the catalog model
         * @param {Object} entitlementInfo the entitlement model
         * @return {Boolean}
         */
        function isTrialAwaitingActivation(catalogInfo, entitlementInfo) {
            if(!isLegacyTrial(catalogInfo)) {
                return false;
            }

            return _.isNull(_.get(entitlementInfo, 'terminationDate'));
        }

        /**
         * Is a game trial in progress?
         * @param {Object} catalogInfo the catalog model
         * @param {Object} entitlementInfo the entitlement model
         * @return {Boolean}
         */
        function isTrialInProgress(catalogInfo, entitlementInfo) {
            if(!isLegacyTrial(catalogInfo)) {
                return false;
            }

            return isInTheFuture(_.get(entitlementInfo, 'terminationDate'));
        }

        /**
         * Is the game trial expired?
         * @param {Object} catalogInfo the catalog model
         * @param {Object} entitlementInfo the entitlement model
         * @return {Boolean}
         */
        function isTrialExpired(catalogInfo, entitlementInfo) {
            if(!isLegacyTrial(catalogInfo)) {
                return false;
            }

            return isInThePast(_.get(entitlementInfo, 'terminationDate'));
        }


        return {
            isLegacyTrial: isLegacyTrial,
            isTrialExpired: isTrialExpired,
            isTrialInProgress: isTrialInProgress,
            isTrialAwaitingActivation: isTrialAwaitingActivation
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.refiners.LegacyTrialRefiner
     
     * @description
     *
     * Helpful methods for filtering content based on vault satus for OA games
     */
    angular.module('origin-components')
        .factory('LegacyTrialRefiner', LegacyTrialRefiner);
})();